#include "ui/tray_controller.hpp"

// ============================================================================
// STUB DE PARTE B — Ver TASKS.md [TASK-B5] (rama sugerida: feat/part-b-ui-tray)
// ============================================================================

namespace WinOpty::UI {

Core::Result TrayController::Initialize() {
    // TODO(TASK-B5): Registrar icono en bandeja (Shell_NotifyIcon) manteniendo < 15 MB RAM.
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "TrayController::Initialize pendiente en Parte B [TASK-B5]");
}

Core::Result TrayController::RunMessageLoop() {
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "TrayController::RunMessageLoop pendiente en Parte B [TASK-B5]");
}

void TrayController::Shutdown() {}

} // namespace WinOpty::UI
