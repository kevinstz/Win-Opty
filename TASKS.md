# Tablero de Tareas — Win-Opty (Trabajo Simultaneo)

> Este tablero coordina el trabajo **en paralelo** entre **Agente A (Yo)** y **Agente B (Otra IA / Colaborador)** para que ambos puedan trabajar al mismo tiempo en ramas separadas sin pisarse archivos.
>
> **Leyenda:** `[x]` Completado · `[~]` En progreso ahora mismo · `[ ]` Pendiente / Listo para tomar · `[!]` Requiere decision humana

---

## Base ya completada y publicada (Sprint A0 - A3)

- [x] **TASK-A0:** Estructura del repo, `CMakeLists.txt`, flujo Git (`docs/WORKFLOW.md`), arquitectura (`docs/ARCHITECTURE.md`) y `TASKS.md`.
- [x] **TASK-A1:** Contratos C++ compartidos (`src/core/engine_config.hpp`, `src/core/module_interfaces.hpp`).
- [x] **TASK-A2:** Catalogo de Presets y Perfiles JSONC (`src/presets/{packs,safe,medium,extreme}.jsonc` y `profiles/{fortnite,valorant,cs2}.jsonc`).
- [x] **TASK-A3:** Scripts PowerShell auditables con respaldo y rollback (`scripts/windows/*.ps1`).
- [x] **TASK-A4:** Motor de Respaldo y Tweaks Transaccionales + parser JSONC propio (`src/core/restore_point.*`, `src/core/tweak_engine.*`, `src/core/jsonc_parser.hpp`).
- [x] **TASK-A5:** Limpiador Nativo de RAM y Limpiador de Cache (`src/core/memory_cleaner.cpp`, `src/core/cache_cleaner.cpp`).
- [x] **TASK-A6:** CLI bootstrap (`src/main.cpp`), stubs compilables y suite de pruebas (`tests/test_presets_and_scripts.ps1`, `tests/test_core_part_a.cpp`).

---

## Cola Asignada a Agente A (Yo — En curso en paralelo)

> **Propiedad exclusiva de archivos mientras se ejecutan estas tareas:**
> `CMakeLists.txt`, `src/core/priority_launcher.cpp`, `src/core/game_mode.cpp`, `scripts/linux/*`, `scripts/macos/*`, `tests/test_core_part_a.cpp`, `tests/test_presets_and_scripts.ps1`.

- [~] **TASK-A7: Toolchain C++, Compilacion Real y Pruebas Unitarias**
  - **Rama:** `feat/part-a-toolchain-and-tests`
  - **Que hare:** Instalar `CMake` + compilador C++ (`WinLibs` / `Ninja`), compilar `win-opty.exe` y `win-opty-tests.exe`, ejecutar `ctest --output-on-failure` y verificar la CLI (`--scan-cache`, `--apply-preset extreme --dry-run`).
- [ ] **TASK-A8: Lanzador de Juegos con Prioridad (`src/core/priority_launcher.cpp`)**
  - **Rama:** `feat/part-a-priority-launcher`
  - **Que hare:** Escanear bibliotecas instaladas (Steam `libraryfolders.vdf`, Epic Manifests, GOG, Ubisoft, EA, Battle.net, Rockstar), asociar perfiles de `src/presets/profiles/*.jsonc`, limpiar RAM previa con `MemoryCleaner::ExecuteTrim()`, asignar afinidad de nucleos preferidos y lanzar con `HIGH_PRIORITY_CLASS` (con pruebas unitarias sobre manifests de prueba).
- [ ] **TASK-A9: Modo Juego Dinamico (`src/core/game_mode.cpp`)**
  - **Rama:** `feat/part-a-game-mode`
  - **Que hare:** Implementar `OnGameStart(pid)` y `OnGameExit(pid)` con whitelist/blacklist estricta (nunca tocar procesos criticos de Windows ni anti-cheats), bajada segura de prioridad de procesos secundarios permitidos y restauracion exacta al cerrar el juego (con pruebas automatizadas).
- [ ] **TASK-A10: Scripts Multi-OS Linux y macOS (`scripts/linux/*`, `scripts/macos/*`)**
  - **Rama:** `feat/part-a-multi-os-scripts`
  - **Que hare:** Crear `99-winopty.conf`, `apply_linux.sh`, `drop_caches.sh`, `gamemode_mangohud.sh`, `apply_macos.sh`, `purge_and_metal.sh`, `launch_pcores.sh` y validar sintaxis con `bash -n`.

---

## Cola Asignada a Colaborador / Otra IA (Agente B — Trabajo simultaneo)

> **Instrucciones para empezar ahora mismo sin conflictos con Agente A:**
> 1. Clona o haz `git fetch origin` y parte desde `origin/dev` (o `dev`).
> 2. Toma **una** tarea de esta lista y crea la rama indicada (`git checkout -b <rama>`).
> 3. **No edites** `priority_launcher.cpp`, `game_mode.cpp`, `scripts/linux/*` ni `scripts/macos/*` (los esta trabajando Agente A en paralelo).
> 4. Agrega tus propias pruebas en un archivo separado (p. ej. `tests/test_part_b.cpp`) para evitar conflictos en `tests/test_core_part_a.cpp`.

- [ ] **TASK-B1: Monitor de Recursos en Tiempo Real (`src/core/hardware_monitor.cpp`)**
  - **Rama sugerida:** `feat/part-b-hardware-monitor`
  - **Archivos exclusivos:** `src/core/hardware_monitor.cpp` (y `tests/test_hardware_monitor.cpp` si agregas target de test)
  - **Que hacer:** Implementar `HardwareMonitor::PollCpuSensors()`, `PollGpuSensors()`, `PollStorageAndNet()`. Usar APIs nativas de Windows (PDH, DXGI para VRAM/adaptador, IP Helper para red, contadores de disco) para carga por hilo, reloj efectivo, uso de RAM/VRAM, I/O NVMe/SSD y trafico TCP/UDP en vivo. Documentar en comentarios como leer temperaturas/watts sin drivers inseguros.
  - **Criterio de aceptacion:** Snapshots poblados sin fugas de handles y prueba unitaria pasando en Windows.

- [ ] **TASK-B4: Overlay HUD, Benchmark y Capturas (`src/core/overlay_hud.cpp`)**
  - **Rama sugerida:** `feat/part-b-overlay-benchmark`
  - **Archivos exclusivos:** `src/core/overlay_hud.cpp` (y `tests/test_overlay_benchmark.cpp`)
  - **Que hacer:**
    - `BenchmarkSession`: calcular `fpsMin`, `fpsMax`, `fpsAvg`, `fps1PercentLow`, `fps0_1PercentLow`, `timeAbove60Pct`, `timeAbove120Pct` y exportar CSV/JSON.
    - `InGameOverlay`: ventana overlay transparente `WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED` usando metricas ETW/PresentMon (**sin inyeccion DLL** para respetar anti-cheats como Vanguard/EAC/VAC).
    - `ScreenshotCapture`: captura instantanea por hotkey `F12` en PNG (WIC/GDI+) o WebP.
  - **Criterio de aceptacion:** Calculo exacto de percentiles `1% Low` y `0.1% Low` cubierto con pruebas unitarias + exportacion CSV/JSON verificada.

- [ ] **TASK-B5: Bandeja del Sistema y Ventana Principal (`src/ui/tray_controller.cpp`, `src/ui/main_window.cpp`)**
  - **Rama sugerida:** `feat/part-b-ui-tray`
  - **Archivos exclusivos:** `src/ui/tray_controller.cpp`, `src/ui/main_window.cpp` (y ajustes minimos solo en la rama `--ui`/`--tray` de `src/main.cpp` al integrar)
  - **Que hacer:** Implementar icono persistente en bandeja (`Shell_NotifyIcon`) con consumo `< 15 MB` RAM y menu rapido (Limpiar RAM, cambiar preset SEGURO/MEDIO/EXTREMO, abrir/salir). Implementar ventana ligera Win32/Direct2D (o ImGui) con los 5 paneles: **Monitor**, **Lanzador**, **Packs**, **Presets** y **Tu Equipo**.
  - **Criterio de aceptacion:** Ejecucion estable de `--tray` y `--ui`, consumo en reposo `< 15 MB` RAM.

- [ ] **TASK-B7: CI en GitHub Actions, Licencia y Documentacion de Benchmarks**
  - **Rama sugerida:** `feat/part-b-ci-and-docs`
  - **Archivos exclusivos:** `.github/workflows/ci.yml`, `LICENSE` (confirmar con el dueno del repo entre MIT / Apache-2.0 / GPLv3), `docs/AUDIT_AND_BENCHMARKS.md`
  - **Que hacer:** Configurar workflow de GitHub Actions en `windows-latest` para compilar CMake, correr `ctest` y ejecutar `tests/test_presets_and_scripts.ps1`; redactar guia de auditoria y plantilla de resultados de benchmark antes/despues.
  - **Criterio de aceptacion:** Pipeline CI verde en Windows y documentacion lista para contribuidores.
