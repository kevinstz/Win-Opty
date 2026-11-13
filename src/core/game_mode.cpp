#include "core/module_interfaces.hpp"

// ============================================================================
// STUB EN COLA DE AGENTE A — Ver TASKS.md [TASK-A9] (rama: feat/part-a-game-mode)
// ============================================================================

namespace WinOpty::Modules {

Core::Result DynamicGameMode::OnGameStart(uint32_t pid) {
    (void)pid;
    // TODO(TASK-A9): Bajar prioridad / suspender procesos secundarios no criticos (con whitelist segura)
    // y guardar su estado previo para restaurarlo exactamente en OnGameExit.
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "DynamicGameMode::OnGameStart pendiente en Agente A [TASK-A9]");
}

Core::Result DynamicGameMode::OnGameExit(uint32_t pid) {
    (void)pid;
    // TODO(TASK-A9): Restaurar prioridades y servicios suspendidos para el PID finalizado.
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "DynamicGameMode::OnGameExit pendiente en Agente A [TASK-A9]");
}

} // namespace WinOpty::Modules
