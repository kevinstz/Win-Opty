# Arquitectura Tecnica y Contratos (Win-Opty)

Este documento resume la arquitectura del binario nativo `win-opty` y la asignacion actual de modulos entre **Agente A** y **Agente B** para desarrollo simultaneo.

## Mapa de Modulos

```text
+-------------------------------------------------------------------+
|                        src/main.cpp (CLI / App)                   |
+---------------------------------+---------------------------------+
|        UI & Tray (Agente B)     |     Motor Core (Agente A + B)   |
|  [B] tray_controller.cpp        |  [A] restore_point.cpp          |
|  [B] main_window.cpp            |  [A] tweak_engine.cpp           |
+---------------------------------+  [A] memory_cleaner.cpp         |
                                  |  [A] cache_cleaner.cpp          |
                                  |  [A] priority_launcher.cpp      |
                                  |  [A] game_mode.cpp              |
                                  |  [B] hardware_monitor.cpp       |
                                  |  [B] overlay_hud.cpp            |
+---------------------------------+---------------------------------+
|   Datos Declarativos & Scripts Multi-OS (Agente A)                |
|  - src/presets/*.jsonc + profiles/*.jsonc                         |
|  - scripts/windows/*.ps1 | scripts/linux/*.sh | scripts/macos/*.sh|
+-------------------------------------------------------------------+
```

## Contratos Clave (`src/core/module_interfaces.hpp`)

- `WinOpty::Core::Result`: tipo uniforme de retorno (`bool ok`, `StatusCode code`, `std::string message`) para que todos los modulos informen errores sin excepciones no controladas.
- `WinOpty::Core::EnginePolicy`: politicas globales en `src/core/engine_config.hpp` (`runInSystemTray`, `transactionalTweaks`, `offlineFirst`, `autoRestorePoint`).
- **Agente A:**
  - `RestorePointManager`: backups JSON antes de tocar el sistema + `Checkpoint-Computer`.
  - `TweakEngine`: carga `src/presets/*.jsonc`, aplica packs/presets, rollback y deteccion post-Windows Update.
  - `MemoryCleaner`: `NtSetSystemInformation` con `SE_PROFILE_SINGLE_PROCESS_NAME`.
  - `CacheCleaner`: `ScanRecoverableBytes` y limpieza selectiva por categoria.
  - `PriorityLauncher`: escaneo de tiendas locales, afinidad/prioridad y pre-trim de RAM (`TASK-A8`).
  - `DynamicGameMode`: suspension/bajada segura de procesos secundarios y restauracion al salir (`TASK-A9`).
- **Agente B:**
  - `HardwareMonitor`: lecturas en vivo CPU/GPU/RAM/VRAM/Disco/Red (`TASK-B1`).
  - `InGameOverlay`, `BenchmarkSession` y `ScreenshotCapture`: HUD seguro sin anti-cheat flags, percentiles `1% Low` / `0.1% Low` y capturas (`TASK-B4`).
  - `TrayController` y `MainWindow`: bandeja `< 15 MB` RAM y 5 paneles (`TASK-B5`).
