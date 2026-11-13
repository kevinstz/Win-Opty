#pragma once

#include "core/engine_config.hpp"

namespace WinOpty::UI {

// Icono persistente en bandeja del sistema 24/7 (Implementado en Parte B - TASK-B5)
class TrayController {
public:
    Core::Result Initialize();
    Core::Result RunMessageLoop();
    void Shutdown();
};

} // namespace WinOpty::UI
