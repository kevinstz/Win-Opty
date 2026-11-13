# Tablero de Tareas — Win-Opty (Division 50% / 50%)

> Este tablero funciona como plan de trabajo vivo para coordinar a **Agente A (Yo)** y **Agente B (Otra IA / Colaborador)** como si fuera un equipo de ingenieria real.
>
> **Leyenda:** `[x]` Completado · `[~]` En progreso ahora mismo · `[ ]` Pendiente por tomar · `[!]` Bloqueado / Requiere decision humana

---

## Donde estoy empezando ahora (Agente A — Parte A, ~50%)

Estoy iniciando la construccion desde los **cimientos seguros del proyecto**: estructura de carpetas, flujo Git con ramas, contratos C++ compartidos, presets declarativos `jsonc`, scripts verificables de Windows PowerShell, el motor transaccional de respaldo/tweaks, el limpiador nativo de RAM y el limpiador selectivo de cache.

### Sprint A1 — Infraestructura, Flujo y Contratos
- [x] **TASK-A0: Estructura base, CMake y Flujo Git** (`feat/part-a-structure-and-tasks`)
  - Crear arbol de carpetas (`src/`, `scripts/`, `docs/`, `tests/`), `.gitignore`, `CMakeLists.txt`, `docs/WORKFLOW.md`, `docs/ARCHITECTURE.md` y este `TASKS.md`.
- [~] **TASK-A1: Contratos compartidos e interfaces** (`feat/part-a-core-modules`)
  - Implementar `src/core/engine_config.hpp` y `src/core/module_interfaces.hpp` con todas las estructuras de `README.md` para desbloquear el trabajo en paralelo de Parte B.

### Sprint A2 — Capa Declarativa y Scripts Verificables (Windows)
- [x] **TASK-A2: Catalogo de Presets y Perfiles JSONC** (`feat/part-a-presets-and-scripts`)
  - Crear `src/presets/packs.jsonc`, `safe.jsonc`, `medium.jsonc`, `extreme.jsonc` y `profiles/{fortnite,valorant,cs2}.jsonc`.
- [x] **TASK-A3: Scripts PowerShell auditables con respaldo y rollback** (`feat/part-a-presets-and-scripts`)
  - Crear `scripts/windows/common_backup.ps1`, `apply_extreme.ps1`, `rollback_tweaks.ps1` y `verify_state.ps1` con soporte `-DryRun`.

### Sprint A3 — Nucleo C++ Parte A + Pruebas
- [ ] **TASK-A4: Motor de Respaldo y Tweaks Transaccionales** (`feat/part-a-core-modules`)
  - Implementar `src/core/restore_point.{hpp,cpp}` y `src/core/tweak_engine.{hpp,cpp}` (incluye parser JSONC propio sin dependencias externas, lectura/escritura segura en registro Windows y rollback).
- [ ] **TASK-A5: Limpiador Nativo de RAM y Limpiador de Cache** (`feat/part-a-core-modules`)
  - Implementar `src/core/memory_cleaner.{hpp,cpp}` (`NtSetSystemInformation` + `SE_PROFILE_SINGLE_PROCESS_NAME`) y `src/core/cache_cleaner.{hpp,cpp}` (`ScanRecoverableBytes` + limpieza segura).
- [ ] **TASK-A6: CLI Bootstrap, Stubs de Parte B y Suite de Pruebas** (`feat/part-a-core-modules`)
  - Implementar `src/main.cpp`, stubs compilables para los modulos de Parte B, `tests/test_presets_and_scripts.ps1` y `tests/test_core_part_a.cpp`.

---

## Backlog para la Otra IA / Colaborador (Agente B — Parte B, ~50%)

> **Instrucciones para tomar una tarea:**
> 1. Parte desde `dev` y crea una rama nueva con el nombre sugerido (ver `docs/WORKFLOW.md`).
> 2. Reemplaza el stub correspondiente en `src/core/` o `src/ui/` manteniendo intactas las firmas de `src/core/module_interfaces.hpp`.
> 3. Agrega pruebas unitarias y marca tu tarea `[x]` antes de hacer merge `--no-ff` a `dev`.

### Sprint B1 — Telemetria de Hardware y Gaming Dinamico
- [ ] **TASK-B1: Monitor de Recursos en Tiempo Real (`hardware_monitor.cpp`)**
  - **Rama sugerida:** `feat/part-b-hardware-monitor`
  - **Archivos:** `src/core/hardware_monitor.cpp` (y helpers si hace falta)
  - **Que hacer:** Implementar `HardwareMonitor::PollCpuSensors()`, `PollGpuSensors()`, `PollStorageAndNet()`. Usar PDH/WMI/DXGI/IP Helper nativos en Windows para carga por hilo, reloj efectivo, RAM/VRAM, I/O NVMe y tráfico TCP/UDP. Documentar estrategia sin drivers inseguros para temperaturas/watts de CPU/GPU.
  - **Criterio de aceptacion:** Devuelve snapshots validos sin fugas de memoria y pasa prueba unitaria con valores reales > 0 en Windows.

- [ ] **TASK-B2: Lanzador de Juegos con Prioridad (`priority_launcher.cpp`)**
  - **Rama sugerida:** `feat/part-b-priority-launcher`
  - **Archivos:** `src/core/priority_launcher.cpp`
  - **Que hacer:** Escanear instalaciones locales de Steam (`libraryfolders.vdf`), Epic Games (`ProgramData\Epic\...\Manifests`), GOG, Ubisoft Connect, EA App, Battle.net y Rockstar leyendo registro/manifests locales. Lanzar con `HIGH_PRIORITY_CLASS`, afinidad a nucleos rapidos, argumentos de `src/presets/profiles/*.jsonc` y llamada previa a `MemoryCleaner::ExecuteTrim()`.
  - **Criterio de aceptacion:** Detecta al menos juegos de Steam/Epic o rutas mock en tests y construye el comando/afinidad correctamente.

- [ ] **TASK-B3: Modo Juego Dinamico (`game_mode.cpp`)**
  - **Rama sugerida:** `feat/part-b-game-mode`
  - **Archivos:** `src/core/game_mode.cpp`
  - **Que hacer:** Implementar `OnGameStart(pid)` y `OnGameExit(pid)`. Al iniciar un juego, bajar prioridad o suspender procesos secundarios permitidos usando una **whitelist/blacklist estricta** (nunca congelar `csrss.exe`, `lsass.exe`, `dwm.exe`, servicios de audio/red o anti-cheat). Guardar estado previo y restaurarlo exactamente al cerrar el PID del juego.
  - **Criterio de aceptacion:** Test automatizado sobre procesos dummy verifica suspension/bajada de prioridad y restauracion exacta.

### Sprint B2 — Overlay, Benchmark, Capturas e Interfaz (UI)
- [ ] **TASK-B4: Overlay HUD, Benchmark y Capturas (`overlay_hud.cpp`)**
  - **Rama sugerida:** `feat/part-b-overlay-benchmark`
  - **Archivos:** `src/core/overlay_hud.cpp`
  - **Que hacer:**
    - `BenchmarkSession`: calcular `fpsMin`, `fpsMax`, `fpsAvg`, `fps1PercentLow`, `fps0_1PercentLow`, `timeAbove60Pct`, `timeAbove120Pct` a partir de timestamps de *frametime* y exportar a CSV/JSON.
    - `InGameOverlay`: usar ventana overlay transparente `WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED` + lectura de frames via ETW/PresentMon (evitar inyeccion DLL para no chocar con anti-cheats como Vanguard/EAC/VAC).
    - `ScreenshotCapture`: captura de ventana activa o pantalla con hotkey `F12` en PNG (WIC/GDI+) o WebP.
  - **Criterio de aceptacion:** Calculo matematico exacto de percentiles 1% / 0.1% Low cubierto con tests unitarios y exportacion verificada.

- [ ] **TASK-B5: Bandeja del Sistema y Ventana Principal (`tray_controller.cpp`, `main_window.cpp`)**
  - **Rama sugerida:** `feat/part-b-ui-tray`
  - **Archivos:** `src/ui/tray_controller.cpp`, `src/ui/main_window.cpp`, actualizacion minima de `src/main.cpp`
  - **Que hacer:** Implementar icono 24/7 en bandeja (`Shell_NotifyIcon`) con menu rapido (Limpiar RAM, Preset Seguro/Medio/Extremo, Abrir/Salir) y ventana ligera Win32/Direct2D (o ImGui) con los 5 paneles del README: **Monitor**, **Lanzador**, **Packs**, **Presets** y **Tu Equipo**. Vigilar que el consumo en reposo se mantenga `< 15 MB` RAM.
  - **Criterio de aceptacion:** Arranque estable en modo `--ui` / bandeja, consumo medido `< 15 MB` en reposo y ejecucion sin consola bloqueante.

### Sprint B3 — Multi-OS, Documentacion Final, Licencia y CI
- [ ] **TASK-B6: Scripts Linux y macOS + Auditoria y Benchmarks**
  - **Rama sugerida:** `feat/part-b-multi-os-docs`
  - **Archivos:** `scripts/linux/*`, `scripts/macos/*`, `docs/AUDIT_AND_BENCHMARKS.md`
  - **Que hacer:** Implementar scripts verificables para Linux (`99-winopty.conf`, `drop_caches.sh`, `gamemode_mangohud.sh`) y macOS (`apply_macos.sh`, `purge_and_metal.sh`, `launch_pcores.sh`), validar sintaxis con `bash -n` y crear plantilla/resultados de auditoria.

- [ ] **TASK-B7: Toolchain C++, Licencia y GitHub Actions CI**
  - **Rama sugerida:** `feat/part-b-ci-and-release`
  - **Archivos:** `.github/workflows/ci.yml`, `LICENSE` (confirmar con el autor del repo si sera MIT, Apache-2.0 o GPLv3)
  - **Que hacer:** Configurar workflow CI en Windows (MSVC + CMake + CTest) y checks de PowerShell/JSONC/Bash. En local (si aun no hay compilador), documentar o ejecutar la instalacion de CMake + Visual Studio Build Tools.
