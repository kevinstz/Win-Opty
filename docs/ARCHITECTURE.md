# Arquitectura Tecnica y Contratos (Win-Opty)

Este documento resume la arquitectura del binario nativo `win-opty` y el contrato exacto entre **Parte A** y **Parte B**.

## Mapa de Modulos

```text
+-------------------------------------------------------------------+
|                        src/main.cpp (CLI / App)                   |
+---------------------------------+---------------------------------+
|         UI & Tray (Parte B)     |     Motor Core (Parte A + B)    |
|  - tray_controller.cpp          |  [A] restore_point.cpp          |
|  - main_window.cpp              |  [A] tweak_engine.cpp           |
+---------------------------------+  [A] memory_cleaner.cpp         |
                                  |  [A] cache_cleaner.cpp          |
                                  |  [B] hardware_monitor.cpp       |
                                  |  [B] priority_launcher.cpp      |
                                  |  [B] game_mode.cpp              |
                                  |  [B] overlay_hud.cpp            |
+---------------------------------+---------------------------------+
|      Datos Declarativos & Scripts (Parte A Windows / B Multi-OS)  |
|  - src/presets/*.jsonc + profiles/*.jsonc                         |
|  - scripts/windows/*.ps1  |  scripts/linux/*.sh  | scripts/macos  |
+-------------------------------------------------------------------+
```

## Contratos Clave (`src/core/module_interfaces.hpp`)

- `WinOpty::Core::Result`: tipo uniforme de retorno (`bool ok`, `StatusCode code`, `std::string message`) para que todos los modulos informen errores sin excepciones no controladas.
- `WinOpty::Core::EnginePolicy`: politicas globales en `src/core/engine_config.hpp` (`runInSystemTray`, `transactionalTweaks`, `offlineFirst`, `autoRestorePoint`).
- **Parte A** expone:
  - `RestorePointManager`: crea backups JSON antes de tocar el sistema y dispara `Checkpoint-Computer`.
  - `TweakEngine`: carga `src/presets/*.jsonc` (soportando comentarios `//`), aplica packs por ID o preset (`SEGURO`, `MEDIO`, `EXTREMO`), ejecuta rollback y detecta regresiones post-Windows Update.
  - `MemoryCleaner`: vacia Working Set / Standby Page Lists mediante `NtSetSystemInformation` con privilegio `SE_PROFILE_SINGLE_PROCESS_NAME`.
  - `CacheCleaner`: calcula tamano recuperable (`ScanRecoverableBytes`) y limpia de forma selectiva por categoria.
- **Parte B** implementa:
  - `HardwareMonitor`: lecturas en vivo (`CpuSnapshot`, `GpuSnapshot`, `SystemIoSnapshot`).
  - `PriorityLauncher`: escanea tiendas de juegos y lanza ejecutables con prioridad/afinidad + `MemoryCleaner::ExecuteTrim()` previo.
  - `DynamicGameMode`: suspende procesos secundarios no criticos durante el juego y restaura estado previo al salir.
  - `InGameOverlay`, `BenchmarkSession` y `ScreenshotCapture`: HUD en vivo (sin violar anti-cheats), estadisticas `1% Low` / `0.1% Low` y capturas por hotkey.
  - `TrayController` y `MainWindow`: interfaz ligera (`< 15 MB` RAM en segundo plano).
