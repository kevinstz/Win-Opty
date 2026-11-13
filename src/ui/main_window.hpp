#pragma once

#include "core/engine_config.hpp"

namespace WinOpty::UI {

enum class DashboardPanel {
    Monitor = 0,
    Lanzador,
    Packs,
    Presets,
    TuEquipo
};

// Ventana principal de paneles (Implementado en Parte B - TASK-B5)
class MainWindow {
public:
    Core::Result CreateAndShow(DashboardPanel initialPanel = DashboardPanel::Monitor);
    void Close();
};

} // namespace WinOpty::UI
