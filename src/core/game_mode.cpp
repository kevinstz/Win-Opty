#include "core/module_interfaces.hpp"

// ============================================================================
// STUB DE PARTE B — Ver TASKS.md [TASK-B3] (rama sugerida: feat/part-b-game-mode)
// ============================================================================

namespace WinOpty::Modules {

Core::Result DynamicGameMode::OnGameStart(uint32_t pid) {
    (void)pid;
    // TODO(TASK-B3): Bajar prioridad / suspender procesos secundarios no criticos (con whitelist segura)
    // y guardar su estado previo para restaurarlo exactamente en OnGameExit.
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "DynamicGameMode::OnGameStart pendiente en Parte B [TASK-B3]");
}

Core::Result DynamicGameMode::OnGameExit(uint32_t pid) {
    (void)pid;
    // TODO(TASK-B3): Restaurar prioridades y servicios suspendidos para el PID finalizado.
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "DynamicGameMode::OnGameExit pendiente en Parte B [TASK-B3]");
}

} // namespace WinOpty::Modules
