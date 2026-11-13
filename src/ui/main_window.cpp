#include "ui/main_window.hpp"

// ============================================================================
// STUB DE PARTE B — Ver TASKS.md [TASK-B5] (rama sugerida: feat/part-b-ui-tray)
// ============================================================================

namespace WinOpty::UI {

Core::Result MainWindow::CreateAndShow(DashboardPanel initialPanel) {
    (void)initialPanel;
    // TODO(TASK-B5): Implementar ventana ligera con los 5 paneles:
    // Monitor, Lanzador, Packs, Presets y Tu Equipo.
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "MainWindow::CreateAndShow pendiente en Parte B [TASK-B5]");
}

void MainWindow::Close() {}

} // namespace WinOpty::UI
