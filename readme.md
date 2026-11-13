# Win-Opty

**Suite de optimizacion de bajo nivel, telemetria en tiempo real y gestion de recursos de codigo abierto.**  
Repositorio oficial: [https://github.com/kevinstz/Win-Opty](https://github.com/kevinstz/Win-Opty)

**Win-Opty** unifica en un solo binario nativo y auditable las herramientas que normalmente requieren tener varios programas abiertos en segundo plano: la liberacion de listas de memoria al estilo de [Mem Reduct](https://github.com/henrypp/memreduct), el HUD de metricas y percentiles de *frametime* en partida de [MSI Afterburner](https://www.msi.com/Landing/afterburner) junto con [RivaTuner Statistics Server (RTSS)](https://en.wikipedia.org/wiki/RivaTuner) (o [MangoHud](https://github.com/flightlessmango/Mangohud) y [Feral GameMode](https://github.com/feralinteractive/gamemode) en Linux), un limpiador selectivo de caches/shaders y un motor transaccional de ajustes del sistema operativo.

Todo el codigo es local, sin telemetria oculta, sin cuentas obligatorias ni descargas de scripts cerrados desde servidores externos. Cada ajuste funciona como un interruptor con estado persistente y *rollback* inmediato.

## Principios de Ingenieria

```cpp
// Principios base de ejecucion en Win-Opty (src/core/engine_config.hpp)
namespace WinOpty::Core {
    struct EnginePolicy {
        bool runInSystemTray      = true;  // Ejecucion persistente en bandeja con consumo < 15 MB RAM
        bool transactionalTweaks  = true;  // Cada interruptor guarda el valor previo antes de escribir
        bool offlineFirst         = true;  // Cero dependencia de servidores externos o cuentas de usuario
        bool autoRestorePoint     = true;  // Snapshot / punto de restauracion previo a cambios criticos
    };
}
```

## Arquitectura y Especificacion de Modulos

Al abrir este archivo en **Visual Studio Code**, los bloques a continuacion utilizan resaltado de sintaxis (`cpp`, `jsonc`, `powershell`, `bash`) y comentarios `//` para documentar de forma directa que hace cada modulo del proyecto.

### 1. Nucleo de Memoria, Telemetria, Overlay y Lanzador

```cpp
// ============================================================================
// Win-Opty Core API — Definicion de los 17 modulos integrados
// ============================================================================

namespace WinOpty::Modules {

    // 1. Limpieza Automatica y Manual de RAM
    // Basado en llamadas nativas NtSetSystemInformation (igual que Mem Reduct)
    // y ajuste de pagecache/zram en Linux sin provocar micro-stuttering.
    struct MemoryCleaner {
        bool clearSystemWorkingSet  = true;  // Vacia el working set del kernel y procesos inactivos
        bool clearStandbyPageLists  = true;  // Libera paginas en espera sin tocar paginas activas del juego
        bool clearModifiedPageLists = false; // Opcional: vuelca paginas modificadas pendientes a disco
        uint32_t autoCleanThreshold = 75;    // Dispara limpieza automatica al superar el 75% de uso de RAM
        uint32_t intervalSeconds    = 300;   // Intervalo opcional en segundo plano (0 = solo por umbral)
        void ExecuteTrim();                  // Ejecuta la purga con privilegios SE_PROFILE_SINGLE_PROCESS
    };

    // 2. Overlay de FPS en Juego (HUD DirectX / Vulkan / OpenGL)
    // Alternativa directa a MSI Afterburner + RTSS y MangoHud sin configurar servidores externos.
    struct InGameOverlay {
        bool showFPS           = true;       // Fotogramas por segundo actuales
        bool showFrametimeGraph = true;      // Grafica en vivo en milisegundos (ms) para detectar tirones
        bool showCpuGpuMetrics = true;       // Uso (%), temperaturas (C) y frecuencias reales (MHz)
        bool showMemoryMetrics = true;       // Consumo de RAM del sistema y VRAM dedicada
        int32_t positionX      = 24;         // Posicion arrastrable en pantalla (coordenada X)
        int32_t positionY      = 24;         // Posicion arrastrable en pantalla (coordenada Y)
    };

    // 3. Modo Benchmark Competitivo
    // Mide estabilidad real de la partida y exporta los resultados en CSV/JSON.
    struct BenchmarkSession {
        float fpsMin           = 0.0f;       // FPS minimos registrados en la sesion
        float fpsMax           = 0.0f;       // FPS maximos registrados
        float fpsAvg           = 0.0f;       // Media aritmetica de FPS
        float fps1PercentLow   = 0.0f;       // 1% Low: fluidez real en caidas de carga pesada
        float fps0_1PercentLow = 0.0f;       // 0.1% Low: deteccion de micro-cortes severos
        uint64_t totalFrames   = 0;          // Total de cuadros renderizados
        float timeAbove60Pct   = 0.0f;       // % del tiempo total corriendo por encima de 60 FPS
        float timeAbove120Pct  = 0.0f;       // % del tiempo total corriendo por encima de 120 FPS
    };

    // 4. Monitor de Recursos en Tiempo Real
    // Panel principal y vista rapida en bandeja del sistema.
    struct HardwareMonitor {
        void PollCpuSensors();               // Carga por hilo, temperatura de encapsulado, reloj efectivo
        void PollGpuSensors();               // Uso de nucleo, controlador de memoria, hotspot y consumo (W)
        void PollStorageAndNet();            // Tasa de lectura/escritura NVMe/SSD y trafico TCP/UDP en vivo
    };

    // 5. Lanzador de Juegos con Prioridad (Game Launcher)
    // Escanea bibliotecas instaladas o permite agregar ejecutables manualmente.
    struct PriorityLauncher {
        // Plataformas detectadas automaticamente:
        // Steam, Epic Games, GOG, Ubisoft Connect, EA App, Battle.net, Rockstar Games
        bool setHighCpuPriority = true;      // Inicia el proceso con HIGH_PRIORITY_CLASS / nice -10
        bool bindFastCoresOnly  = true;      // Asigna afinidad a los nucleos preferidos (evita parking)
        bool preCleanRamOnStart = true;      // Ejecuta MemoryCleaner::ExecuteTrim() antes de abrir el juego
    };

    // 6. Modo Juego Dinamico (Game Booster)
    // Suspende procesos y servicios secundarios al abrir un juego y los restaura al salir.
    struct DynamicGameMode {
        void OnGameStart(uint32_t pid);      // Congela indexadores, actualizadores y clientes pesados
        void OnGameExit(uint32_t pid);       // Restaura exactamente los servicios y prioridades previas
    };

    // 7. Limpieza de Cache y Disco
    // Analiza espacio recuperable antes de borrar y permite marcar que categorias limpiar.
    struct CacheCleaner {
        bool cleanOsTempFiles   = true;      // %TEMP%, C:\Windows\Temp, Prefetch huérfano, /var/tmp
        bool cleanShaderCaches  = true;      // DirectX Shader Cache, NVIDIA DXCache/GLCache, AMD DxCache, Vulkan
        bool cleanBrowserCaches = false;     // Cache de disco de Chrome, Brave, Edge, Firefox (sin tocar sesiones)
        bool cleanGameLogs      = true;      // Crash dumps (.dmp) y logs antiguos de motores (Unreal, Source, Unity)
        uint64_t ScanRecoverableBytes();     // Devuelve el tamano exacto antes de confirmar el borrado
    };

    // 8. Captura de Pantalla Instantanea
    // Captura directa del framebuffer por hotkey sin depender de overlays pesados.
    struct ScreenshotCapture {
        const char* hotkey     = "F12";      // Atajo global configurable
        const char* format     = "png";      // Compresion sin perdida (PNG) o baja latencia (WebP)
        void CaptureActiveWindow();          // Guarda al instante sin minimizar ni pausar el renderizado
    };
}
```

### 2. Resumen de Referencias Tecnicas por Modulo

| Modulo | Funcion Principal en [Win-Opty](https://github.com/kevinstz/Win-Opty) | Base Tecnica / Referencia Open Source |
| :--- | :--- | :--- |
| **Limpieza de RAM** | Vaciado de *Working Set*, *Standby Lists* y *Modified Page Lists* por umbral o intervalo. | API nativa de Windows usada en [Mem Reduct](https://github.com/henrypp/memreduct) y parametros de [Linux `/proc/sys/vm`](https://docs.kernel.org/admin-guide/sysctl/vm.html). |
| **Overlay & Benchmark** | HUD en partida con FPS, *frametime*, temperaturas, 1 % Low, 0,1 % Low y % > 60/120 FPS. | Alternativa integrada a [MSI Afterburner](https://www.msi.com/Landing/afterburner), [RivaTuner (RTSS)](https://en.wikipedia.org/wiki/RivaTuner) y [MangoHud](https://github.com/flightlessmango/Mangohud). |
| **Lanzador y Modo Juego** | Deteccion de bibliotecas (Steam, Epic, GOG, EA, Ubisoft, Battle.net, Rockstar), prioridad de CPU y suspension de servicios. | Priorizacion con [Multimedia Class Scheduler Service (MMCSS)](https://learn.microsoft.com/es-es/windows/win32/procthread/multimedia-class-scheduler-service) y [Feral GameMode](https://github.com/feralinteractive/gamemode). |
| **Red para Competitivo** | Reduccion de latencia TCP/UDP desactivando el algoritmo de Nagle y el retardo de ACK. | Ajustes de registro [`TcpAckFrequency` y `TCPNoDelay`](https://learn.microsoft.com/en-us/troubleshoot/windows-server/networking/registry-entry-control-tcp-acknowledgment-behavior) y control de congestion BBR. |
| **Punto de Restauracion** | Creacion de respaldo del sistema antes de aplicar cualquier pack o preset. | Cmdlet oficial [`Checkpoint-Computer`](https://learn.microsoft.com/es-es/powershell/module/microsoft.powershell.management/checkpoint-computer?view=powershell-5.1) y respaldo transaccional JSON. |
| **Planificacion de GPU** | Configuracion de planificador de hardware, modo ventana (*Flip Model*) y limpieza de *shaders*. | Soporte para [Hardware-Accelerated GPU Scheduling (HAGS)](https://devblogs.microsoft.com/directx/hardware-accelerated-gpu-scheduling/). |

## Motor de Packs Reversibles, Presets y Perfiles

En lugar de aplicar cambios opacos, cada pack de optimizacion en [Win-Opty](https://github.com/kevinstz/Win-Opty) se define en archivos `jsonc` legibles. Cada interruptor almacena su estado original para poder apagarlo individualmente o reaplicar todos los ajustes en un clic si una actualizacion del sistema operativo los deshace.

```jsonc
// ============================================================================
// Estructura declarativa de Packs, Presets y Perfiles (src/presets/packs.jsonc)
// ============================================================================
{
  // Niveles preconfigurados (Presets de 1 clic con vista previa de cambios):
  // "SEGURO"  -> Limpieza de RAM/cache, punto de restauracion y ajustes sin impacto en servicios base.
  // "MEDIO"   -> Gaming diario: MMCSS, HAGS, Red Competitiva, Modo Juego y desactivacion de telemetria.
  // "EXTREMO" -> Maximo rendimiento competitivo: recorte agresivo de demonios/servicios y afinidad pura.
  "activePreset": "EXTREMO",

  // Punto de seguridad automatico antes de aplicar cambios
  "safetyRestore": {
    "createWindowsRestorePoint": true,     // Ejecuta Checkpoint-Computer antes de modificar el registro
    "exportStateBackupPath": "./backups/last_known_good.json",
    "reapplyOnOsUpdate": true              // Detecta si Windows Update revirtio llaves y las reaplica en 1 clic
  },

  // Categorias de Packs (cada ajuste es un interruptor independiente ON/OFF)
  "tweakPacks": [
    {
      "id": "gaming.mmcss_scheduler",
      "category": "Gaming y Rendimiento",
      "description": "Reserva el 90% de tiempo de CPU para procesos multimedia/juegos y sube la prioridad de GPU a 8.",
      "enabled": true,
      "reversible": true
    },
    {
      "id": "network.competitive_tcp",
      "category": "Red para Competitivo",
      "description": "Desactiva el algoritmo de Nagle (TCPNoDelay=1) y fuerza confirmacion inmediata de paquetes (TcpAckFrequency=1).",
      "enabled": true,
      "reversible": true
    },
    {
      "id": "privacy.telemetry_debloat",
      "category": "Privacidad y Debloat",
      "description": "Detiene recolectores de telemetria (DiagTrack), sugerencias del sistema y tareas de diagnostico en segundo plano.",
      "enabled": true,
      "reversible": true
    },
    {
      "id": "system.startup_manager",
      "category": "Apps de Inicio",
      "description": "Desactiva lanzadores y actualizadores del arranque de Windows/systemd sin borrar sus entradas por si deseas reactivarlos.",
      "enabled": true,
      "reversible": true
    },
    {
      "id": "setup.fresh_os_wizard",
      "category": "Asistente Post-Instalacion",
      "description": "Configura un sistema recien instalado en una sesion: instala runtimes (VC++, DirectX), activa plan de energia y limpia bloatware.",
      "enabled": false,
      "reversible": true
    }
  ],

  // Perfiles dedicados para eSports (ejemplo: Perfil Fortnite)
  "gameProfiles": {
    "FortniteClient-Win64-Shipping.exe": {
      "profileName": "Perfil Competitivo Fortnite",
      "cpuPriority": "High",
      "disableFullscreenOptimizations": false, // Usa Flip Model directo en DX12 / Modo Rendimiento
      "clearShaderCacheOnUpdate": true,        // Evita tirones tras cada parche de temporada
      "launchArguments": "-USEALLAVAILABLECORES -nosplash -high",
      "suspendBackgroundServices": true
    }
  }
}
```

## Seccion de Optimizacion Muy Alta (Multi-Sistema Operativo)

Esta seccion documenta los ajustes de nivel experto que el motor de [Win-Opty](https://github.com/kevinstz/Win-Opty) aplica cuando se activa el perfil de **Optimizacion Muy Alta (`EXTREMO`)**, adaptados de forma nativa para **Windows 10/11**, **Linux (Kernel 6.x+)** y **macOS**.

### 1. Windows 10 / 11 (Registro, Planificador MMCSS, Red y Memoria Nativa)

Ajustes basados en la documentacion de [Multimedia Class Scheduler Service (MMCSS)](https://learn.microsoft.com/es-es/windows/win32/procthread/multimedia-class-scheduler-service), [Hardware-Accelerated GPU Scheduling (HAGS)](https://devblogs.microsoft.com/directx/hardware-accelerated-gpu-scheduling/), [TcpAckFrequency / TCPNoDelay](https://learn.microsoft.com/en-us/troubleshoot/windows-server/networking/registry-entry-control-tcp-acknowledgment-behavior) y puntos de restauracion con [Checkpoint-Computer](https://learn.microsoft.com/es-es/powershell/module/microsoft.powershell.management/checkpoint-computer?view=powershell-5.1):

```powershell
# ============================================================================
# Win-Opty — Perfil de Optimizacion Muy Alta para Windows 10/11 (PowerShell)
# Todos los valores originales se respaldan en ./backups/win_registry_state.json
# ============================================================================

# // 1. Crear punto de restauracion del sistema antes de tocar el registro
Checkpoint-Computer -Description "Win-Opty Pre-Extreme Optimization" -RestorePointType "MODIFY_SETTINGS"

# // 2. Planificador Multimedia (MMCSS) y prioridad de hilos para juegos
$mmcssPath = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile"
Set-ItemProperty -Path $mmcssPath -Name "SystemResponsiveness" -Value 10 -Type DWord      # Reserva solo 10% a tareas de fondo
Set-ItemProperty -Path $mmcssPath -Name "NetworkThrottlingIndex" -Value 0xffffffff -Type DWord # Desactiva estrangulamiento de red

$gamesTask = "$mmcssPath\Tasks\Games"
Set-ItemProperty -Path $gamesTask -Name "GPU Priority" -Value 8 -Type DWord               # Prioridad maxima de contexto GPU
Set-ItemProperty -Path $gamesTask -Name "Priority" -Value 6 -Type DWord                   # Prioridad alta en el scheduler
Set-ItemProperty -Path $gamesTask -Name "Scheduling Category" -Value "High" -Type String
Set-ItemProperty -Path $gamesTask -Name "SFIO Priority" -Value "High" -Type String        # Prioridad alta de E/S programada

# // 3. Planificacion de GPU por Hardware (HAGS) y desactivacion de Game DVR
Set-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\GraphicsDrivers" -Name "HwSchMode" -Value 2 -Type DWord
Set-ItemProperty -Path "HKCU:\System\GameConfigStore" -Name "GameDVR_Enabled" -Value 0 -Type DWord
Set-ItemProperty -Path "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\GameDVR" -Name "AppCaptureEnabled" -Value 0 -Type DWord

# // 4. Red para Competitivo: Nagle Off (TCPNoDelay) y ACK Inmediato (TcpAckFrequency)
$interfaces = "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces"
Get-ChildItem $interfaces | ForEach-Object {
    Set-ItemProperty -Path $_.PSPath -Name "TcpAckFrequency" -Value 1 -Type DWord         # Envia ACK sin esperar 200ms
    Set-ItemProperty -Path $_.PSPath -Name "TCPNoDelay" -Value 1 -Type DWord              # Desactiva agrupacion de paquetes Nagle
    Set-ItemProperty -Path $_.PSPath -Name "TcpDelAckTicks" -Value 0 -Type DWord          # Elimina ticks de retardo TCP
}

# // 5. Separacion de prioridad del procesador (Win32PrioritySeparation = 0x26 / 38 decimal)
# // Intervalos cortos y variables con prioridad 3:1 para la ventana en primer plano (el juego)
Set-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\PriorityControl" -Name "Win32PrioritySeparation" -Value 38 -Type DWord
```

### 2. Linux (Kernel 6.x+ — Ubuntu, Bazzite, Arch, Fedora)

Ajustes de kernel basados en la documentacion oficial de [`/proc/sys/vm`](https://docs.kernel.org/admin-guide/sysctl/vm.html), dispositivos de bloque comprimidos en RAM con [`zram`](https://docs.kernel.org/admin-guide/blockdev/zram.html), [Feral GameMode](https://github.com/feralinteractive/gamemode) y [MangoHud](https://github.com/flightlessmango/Mangohud):

```bash
#!/usr/bin/env bash
# ============================================================================
# Win-Opty — Perfil de Optimizacion Muy Alta para Linux (/etc/sysctl.d/99-winopty.conf)
# ============================================================================

# // 1. Subsistema de Memoria Virtual (/proc/sys/vm)
sysctl -w vm.swappiness=10                     # Prioriza RAM fisica; usa zram solo bajo presion real
sysctl -w vm.vfs_cache_pressure=50             # Conserva inodos y dentries en cache para cargas rapidas
sysctl -w vm.compaction_proactiveness=0        # Evita picos de latencia de CPU por compactacion en vivo
sysctl -w vm.page_lock_unfairness=1            # Reduce bloqueos de lectura en paginas compartidas
sysctl -w vm.dirty_ratio=15                    # Limite maximo de paginas sucias antes de volcar a NVMe
sysctl -w vm.dirty_background_ratio=5          # Volcado suave en segundo plano sin saturar el bus I/O
sysctl -w vm.max_map_count=2147483642          # Necesario para juegos modernos pesados en Vulkan/Proton

# // 2. Limpieza manual segura de PageCache / Dentries (equivalente a Mem Reduct en Linux)
sync && echo 1 > /proc/sys/vm/drop_caches      # Libera PageCache sin matar procesos ni causar swap thrashing

# // 3. Pila de Red de Baja Latencia (TCP BBR + FastOpen)
sysctl -w net.core.default_qdisc=fq            # Cola Fair Queueing para reducir bufferbloat
sysctl -w net.ipv4.tcp_congestion_control=bbr  # Algoritmo BBR para menor latencia y mayor throughput
sysctl -w net.ipv4.tcp_fastopen=3              # Acelera apertura de conexiones cliente/servidor
sysctl -w net.ipv4.tcp_mtu_probing=1           # Evita fragmentacion de paquetes en rutas inestables

# // 4. Gobernador de CPU y GPU en modo rendimiento maximo
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    echo "performance" > "$cpu"                # Elimina latencia de escalado de frecuencia durante el juego
done
```

### 3. macOS (Apple Silicon M-Series / Intel)

Ajustes de baja latencia de red, priorizacion de nucleos de rendimiento (*P-Cores*) y limpieza de caches de *shaders* de Metal:

```bash
#!/usr/bin/env bash
# ============================================================================
# Win-Opty — Perfil de Optimizacion Muy Alta para macOS (Unix / Darwin)
# ============================================================================

# // 1. Red competitiva: desactivar Delayed ACK en la pila TCP de Darwin
sudo sysctl -w net.inet.tcp.delayed_ack=0      # Confirmacion inmediata de paquetes (equivalente a TcpAckFrequency=1)
sudo sysctl -w net.inet.tcp.mssdflt=1460       # Tamano de segmento maximo optimizado para Ethernet/Fibra

# // 2. Purga de memoria inactiva y caches de Shaders (Metal / Sistema)
sudo purge                                     # Fuerza liberacion de memoria de disco inactiva en RAM
rm -rf ~/Library/Caches/com.apple.metal/*      # Limpia cache de shaders corruptos u obsoletos

# // 3. Lanzamiento de proceso con maxima prioridad en P-Cores (QoS User Interactive)
# // Evita que el planificador XNU mueva hilos del juego a los nucleos de eficiencia (E-Cores)
taskpolicy -b -p "$GAME_PID"                   # Asegura ejecucion en primer plano con QoS alta
```

### 4. Comparativa de Impacto por Subsistema

| Subsistema | Parametro Clave Aplicado | Resultado Medible en el Sistema |
| :--- | :--- | :--- |
| **Memoria RAM** | `NtSetSystemInformation` ([Mem Reduct](https://github.com/henrypp/memreduct)) / [`/proc/sys/vm`](https://docs.kernel.org/admin-guide/sysctl/vm.html) + [`zram`](https://docs.kernel.org/admin-guide/blockdev/zram.html) | Reduccion del 15 % al 45 % en consumo de memoria en reposo y eliminacion de *stuttering* por paginacion. |
| **CPU Scheduler** | [MMCSS](https://learn.microsoft.com/es-es/windows/win32/procthread/multimedia-class-scheduler-service) (`SystemResponsiveness=10`) / [Feral GameMode](https://github.com/feralinteractive/gamemode) | Mayor estabilidad en los percentiles **1 % Low y 0,1 % Low** medidos en el modo Benchmark del overlay. |
| **GPU Pipeline** | [HAGS](https://devblogs.microsoft.com/directx/hardware-accelerated-gpu-scheduling/) + Purga de caches DirectX / Vulkan | Menor latencia en la cola de *frames* (*input lag*) y carga limpia de *shaders* tras actualizar drivers. |
| **Pila de Red** | [`TcpAckFrequency=1` y `TCPNoDelay=1`](https://learn.microsoft.com/en-us/troubleshoot/windows-server/networking/registry-entry-control-tcp-acknowledgment-behavior) / TCP BBR | Reduccion de *jitter* y envio inmediato de paquetes de entrada en titulos competitivos. |

## Estructura del Repositorio

```text
Win-Opty/
├── src/
│   ├── core/
│   │   ├── memory_cleaner.cpp      // Liberacion nativa de RAM (Working Set / Standby List / drop_caches)
│   │   ├── hardware_monitor.cpp    // Lectura de sensores de CPU, GPU, RAM, VRAM, red y disco
│   │   ├── overlay_hud.cpp         // Overlay arrastrable en juego + motor Benchmark (1% y 0.1% Low)
│   │   ├── priority_launcher.cpp   // Deteccion de Steam, Epic, GOG, Ubisoft, EA, Battle.net, Rockstar
│   │   ├── game_mode.cpp           // Suspension dinamica de procesos y restauracion al cerrar el juego
│   │   ├── cache_cleaner.cpp       // Escaner y limpieza de temporales, navegadores y caches de shaders
│   │   ├── tweak_engine.cpp        // Motor de interruptores reversibles, presets y reaplicacion post-update
│   │   └── restore_point.cpp       // Respaldo con Checkpoint-Computer y snapshots JSON
│   ├── presets/
│   │   ├── packs.jsonc             // Definicion auditable de todos los interruptores y sus valores por defecto
│   │   ├── safe.jsonc              // Preset SEGURO
│   │   ├── medium.jsonc            // Preset MEDIO
│   │   ├── extreme.jsonc           // Preset EXTREMO (Optimizacion Muy Alta Multi-OS)
│   │   └── profiles/               // Perfiles competitivos por juego (Fortnite, Valorant, CS2)
│   └── ui/
│       ├── tray_controller.cpp     // Icono persistente en bandeja del sistema (24/7)
│       └── main_window.cpp         // Interfaz de paneles: Monitor, Lanzador, Packs, Presets y Tu Equipo
├── scripts/
│   ├── windows/                    // Scripts PowerShell verificables (MMCSS, HAGS, Red, Debloat)
│   ├── linux/                      // Perfiles sysctl (/proc/sys/vm), zram, MangoHud y GameMode
│   └── macos/                      // Ajustes sysctl Darwin, taskpolicy y limpieza de caches
├── docs/                           // Documentacion de auditoria y pruebas de rendimiento
├── LICENSE
└── README.md
```

## Instalacion y Compilacion

```bash
# // 1. Clonar el repositorio oficial de Win-Opty
git clone https://github.com/kevinstz/Win-Opty.git
cd Win-Opty

# // 2. Verificar la integridad de los packs de ajustes (sin binarios ocultos)
cat src/presets/packs.jsonc
```

## Contribuir

Cualquier mejora en los packs de optimizacion o en los modulos nativos es bienvenida en [github.com/kevinstz/Win-Opty](https://github.com/kevinstz/Win-Opty):

1. Haz un *fork* del proyecto y crea una rama descriptiva (`git checkout -b feat/modulo-nuevo`).
2. Todo ajuste nuevo en `packs.jsonc` debe incluir obligatoriamente su llave o comando de reversion (*rollback*) y comentarios `//` explicando el impacto tecnico.
3. Abre un *Pull Request* adjuntando capturas o metricas del **Modo Benchmark** (FPS medios, 1 % Low y 0,1 % Low) antes y despues del cambio.

## Licencia

Proyecto de codigo abierto publicado bajo licencia libre. Consulta el repositorio en [https://github.com/kevinstz/Win-Opty](https://github.com/kevinstz/Win-Opty) para ver los terminos completos.