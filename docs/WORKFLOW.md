# Flujo de Trabajo Colaborativo (Multi-IA / Equipo en Simultaneo)

Este proyecto divide el trabajo en paralelo entre **Agente A** y **Agente B (Otra IA / Colaborador)** para trabajar al mismo tiempo con **cero conflictos de merge**.

## 1. Estructura de Ramas (Git Flow Ligero)

- `main`: rama estable publicada (contiene la base verificada + `TASKS.md` actualizado).
- `dev`: rama de integracion donde convergen las entregas de Agente A y Agente B.
- `feat/part-a-*`: ramas trabajadas por **Agente A**.
- `feat/part-b-*`: ramas trabajadas por **Agente B (Otra IA / Colaborador)**.

## 2. Mapa de Propiedad de Archivos (Trabajo Simultaneo)

| Carpeta / Archivo | Responsable | Regla anti-conflicto |
| :--- | :--- | :--- |
| `src/core/engine_config.hpp`, `src/core/module_interfaces.hpp` | Compartido (estable) | No cambiar firmas publicas sin coordinar en `dev`. |
| `src/core/restore_point.*`, `tweak_engine.*`, `memory_cleaner.*`, `cache_cleaner.*` | **Agente A** | Base ya completada y validada. |
| `src/core/priority_launcher.cpp`, `src/core/game_mode.cpp` | **Agente A** | En implementacion activa en paralelo (`TASK-A8`, `TASK-A9`). |
| `scripts/windows/*`, `scripts/linux/*`, `scripts/macos/*` | **Agente A** | Windows listo; Linux/macOS en `TASK-A10`. |
| `src/core/hardware_monitor.cpp`, `src/core/overlay_hud.cpp` | **Agente B** | Exclusivos de Parte B (`TASK-B1`, `TASK-B4`). |
| `src/ui/tray_controller.*`, `src/ui/main_window.*` | **Agente B** | Exclusivos de Parte B (`TASK-B5`). |
| `.github/workflows/ci.yml`, `LICENSE`, `docs/AUDIT_AND_BENCHMARKS.md` | **Agente B** | Exclusivos de Parte B (`TASK-B7`). |
| `tests/test_part_b_*.cpp` | **Agente B** | Crear archivos de test separados para no chocar con `tests/test_core_part_a.cpp`. |

## 3. Ciclo para Trabajar en Simultaneo

```bash
# 1. Partir siempre desde dev actualizado
git fetch origin
git checkout dev
git pull origin dev

# 2. Crear una rama exclusiva para tu tarea de TASKS.md
git checkout -b feat/part-b-hardware-monitor

# 3. Implementar solo los archivos asignados a tu tarea + tus tests
# 4. Verificar antes de integrar
powershell -ExecutionPolicy Bypass -File tests/test_presets_and_scripts.ps1
cmake -B build && cmake --build build && ctest --test-dir build --output-on-failure

# 5. Commit atomico, actualizar tu check [x] en TASKS.md y hacer merge a dev
git add <tus_archivos> TASKS.md
git commit -m "feat(monitor): implement native CPU/GPU/RAM telemetry polling"
git checkout dev
git pull origin dev
git merge --no-ff feat/part-b-hardware-monitor
git push origin dev
```

## 4. Criterios de Calidad Obligatorios

1. **Rollback primero:** ningun ajuste del sistema operativo se aplica sin guardar antes el estado anterior.
2. **Offline-first:** cero llamadas de red ocultas ni descargas de ejecutables externos.
3. **Sin inyeccion riesgosa:** para `overlay_hud`, evitar inyeccion DLL en juegos con anti-cheat (Fortnite, Valorant, CS2); usar ventana overlay `topmost` + ETW/PresentMon.
4. **Compilacion limpia:** cada merge a `dev` debe dejar `win-opty` y `win-opty-tests` compilando sin errores.
