# Flujo de Trabajo Colaborativo (Multi-IA / Equipo)

Este proyecto divide la implementacion en **Parte A** y **Parte B** para que dos agentes o desarrolladores trabajen en paralelo con **cero conflictos de merge**.

## 1. Estructura de Ramas (Git Flow Ligero)

- `main`: rama estable y revisada (solo recibe merges verificados desde `dev`).
- `dev`: rama de integracion donde convergen las entregas de Parte A y Parte B.
- `feat/part-a-*`: ramas usadas en la primera mitad (cimientos, presets, backups, tweaks, RAM, cache, scripts Windows).
- `feat/part-b-*`: ramas asignadas a la segunda IA / colaborador (monitor, lanzador, modo juego, overlay/benchmark, UI, Linux/macOS).

## 2. Regla de Propiedad de Archivos (Anti-Conflictos)

| Carpeta / Archivo | Responsable Principal | Regla |
| :--- | :--- | :--- |
| `src/core/engine_config.hpp`, `src/core/module_interfaces.hpp` | Compartido (definido en Parte A) | Parte B solo **consume** estas interfaces; si necesita ampliarlas, hacerlo en un commit pequeno previo en `dev`. |
| `src/core/restore_point.*`, `tweak_engine.*`, `memory_cleaner.*`, `cache_cleaner.*` | **Parte A** | No modificar desde ramas `feat/part-b-*` salvo bugfix puntual. |
| `src/presets/*`, `scripts/windows/*` | **Parte A** | Base declarativa lista para ser leida por Parte B (`priority_launcher`, `main_window`). |
| `src/core/hardware_monitor.*`, `priority_launcher.*`, `game_mode.*`, `overlay_hud.*` | **Parte B** | Actualmente son stubs en `dev`; Parte B los reemplaza con la implementacion real. |
| `src/ui/tray_controller.*`, `src/ui/main_window.*` | **Parte B** | Implementan la GUI y llaman a las APIs de Parte A y Parte B. |
| `scripts/linux/*`, `scripts/macos/*`, `docs/AUDIT_AND_BENCHMARKS.md` | **Parte B** | Completar paridad multi-OS y documentacion de pruebas. |
| `TASKS.md` | Ambos | Cada agente actualiza el estado (`[ ]` -> `[~]` -> `[x]`) de su tarea al terminarla. |

## 3. Ciclo Diario Recomendado para la Otra IA / Colaborador

```bash
# 1. Asegurar estar al dia con dev
git checkout dev
git pull origin dev   # (o usar la rama dev local)

# 2. Crear una rama por tarea del tablero TASKS.md
git checkout -b feat/part-b-hardware-monitor

# 3. Implementar solo los archivos de esa tarea y sus tests
# 4. Ejecutar pruebas antes de hacer merge
powershell -ExecutionPolicy Bypass -File tests/test_presets_and_scripts.ps1
# Si hay CMake/compilador instalado:
cmake -B build && cmake --build build && ctest --test-dir build --output-on-failure

# 5. Commit atomico + actualizar TASKS.md + merge a dev
git add <archivos> TASKS.md
git commit -m "feat(monitor): implement native CPU/GPU/RAM telemetry polling"
git checkout dev
git merge --no-ff feat/part-b-hardware-monitor
```

## 4. Criterios de Calidad Obligatorios

1. **Rollback primero:** ningun ajuste del sistema operativo se aplica sin guardar antes el estado anterior.
2. **Offline-first:** cero llamadas de red ocultas ni descargas de ejecutables externos.
3. **Sin inyeccion riesgosa:** para `overlay_hud`, evitar inyeccion DLL en juegos con anti-cheat (Fortnite, Valorant, CS2); usar ventana overlay `topmost` + ETW/PresentMon.
4. **Compilacion limpia:** cada merge a `dev` debe dejar `win-opty` y `win-opty-tests` compilando sin errores.
