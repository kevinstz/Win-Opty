#include "core/module_interfaces.hpp"

// ============================================================================
// STUB DE PARTE B — Ver TASKS.md [TASK-B2] (rama sugerida: feat/part-b-priority-launcher)
// ============================================================================

namespace WinOpty::Modules {

std::vector<DetectedGame> PriorityLauncher::ScanInstalledGames() const {
    // TODO(TASK-B2): Detectar bibliotecas locales (Steam, Epic, GOG, Ubisoft, EA, Battle.net, Rockstar).
    return {};
}

Core::Result PriorityLauncher::LaunchGame(const DetectedGame& game, uint32_t* outPid) {
    (void)game;
    (void)outPid;
    // TODO(TASK-B2): Si preCleanRamOnStart == true, invocar MemoryCleaner::ExecuteTrim()
    // y lanzar el ejecutable con HIGH_PRIORITY_CLASS + afinidad de nucleos preferidos.
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "PriorityLauncher::LaunchGame pendiente en Parte B [TASK-B2]");
}

} // namespace WinOpty::Modules
