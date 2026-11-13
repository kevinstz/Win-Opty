#include "core/module_interfaces.hpp"

// ============================================================================
// STUB EN COLA DE AGENTE A — Ver TASKS.md [TASK-A8] (rama: feat/part-a-priority-launcher)
// ============================================================================

namespace WinOpty::Modules {

std::vector<DetectedGame> PriorityLauncher::ScanInstalledGames() const {
    // TODO(TASK-A8): Detectar bibliotecas locales (Steam, Epic, GOG, Ubisoft, EA, Battle.net, Rockstar).
    return {};
}

Core::Result PriorityLauncher::LaunchGame(const DetectedGame& game, uint32_t* outPid) {
    (void)game;
    (void)outPid;
    // TODO(TASK-A8): Si preCleanRamOnStart == true, invocar MemoryCleaner::ExecuteTrim()
    // y lanzar el ejecutable con HIGH_PRIORITY_CLASS + afinidad de nucleos preferidos.
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "PriorityLauncher::LaunchGame pendiente en Agente A [TASK-A8]");
}

} // namespace WinOpty::Modules
