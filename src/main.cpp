#include "core/engine_config.hpp"
#include "core/module_interfaces.hpp"
#include "core/tweak_engine.hpp"
#include "ui/main_window.hpp"
#include "ui/tray_controller.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::string ResolvePresetsDir() {
    namespace fs = std::filesystem;
    if (fs::exists("src/presets/packs.jsonc")) {
        return "src/presets";
    }
#ifdef WINOPTY_DEFAULT_PRESETS_DIR
    if (fs::exists(std::string(WINOPTY_DEFAULT_PRESETS_DIR) + "/packs.jsonc")) {
        return WINOPTY_DEFAULT_PRESETS_DIR;
    }
#endif
    return "presets";
}

void PrintUsage() {
    std::cout << "Win-Opty v" << WinOpty::Core::kAppVersion << " (CLI / Core Engine)\n"
              << "Uso:\n"
              << "  win-opty --scan-cache\n"
              << "  win-opty --clean-cache [--dry-run]\n"
              << "  win-opty --trim-ram\n"
              << "  win-opty --apply-preset <safe|medium|extreme> [--dry-run]\n"
              << "  win-opty --verify-preset <safe|medium|extreme>\n"
              << "  win-opty --rollback [backup_json_path] [--dry-run]\n"
              << "  win-opty --ui | --tray  (Parte B - ver TASKS.md [TASK-B5])\n";
}

bool HasFlag(const std::vector<std::string>& args, const std::string& flag) {
    for (const auto& a : args) {
        if (a == flag) return true;
    }
    return false;
}

} // namespace

int main(int argc, char** argv) {
    using namespace WinOpty;

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    if (args.empty() || HasFlag(args, "--help") || HasFlag(args, "-h")) {
        PrintUsage();
        return 0;
    }

    const bool dryRun = HasFlag(args, "--dry-run");
    const std::string presetsDir = ResolvePresetsDir();

    if (args[0] == "--scan-cache") {
        Modules::CacheCleaner cleaner;
        uint64_t bytes = cleaner.ScanRecoverableBytes();
        std::cout << "Espacio recuperable detectado: " << bytes << " bytes ("
                  << (bytes / (1024.0 * 1024.0)) << " MB)\n";
        return 0;
    }

    if (args[0] == "--clean-cache") {
        Modules::CacheCleaner cleaner;
        Modules::CacheCleanSummary summary;
        Core::Result res = cleaner.CleanSelected(dryRun, &summary);
        std::cout << res.message << "\n";
        return res.ok ? 0 : 1;
    }

    if (args[0] == "--trim-ram") {
        Modules::MemoryCleaner cleaner;
        Core::Result res = cleaner.ExecuteTrim();
        std::cout << res.message << "\n";
        return res.ok ? 0 : 1;
    }

    if ((args[0] == "--apply-preset" || args[0] == "--verify-preset") && args.size() >= 2) {
        Core::EnginePolicy policy;
        Core::TweakEngine engine(policy);

        Core::Result catRes = engine.LoadCatalog(presetsDir + "/packs.jsonc");
        if (!catRes.ok) {
            std::cerr << "Error catalogo: " << catRes.message << "\n";
            return 1;
        }

        Core::PresetConfig preset;
        Core::Result preRes = engine.LoadPreset(presetsDir + "/" + args[1] + ".jsonc", preset);
        if (!preRes.ok) {
            std::cerr << "Error preset: " << preRes.message << "\n";
            return 1;
        }

        if (args[0] == "--verify-preset") {
            auto drift = engine.DetectRevertedTweaks(preset);
            std::cout << "Preset '" << preset.presetId << "' -> diferencias detectadas: "
                      << drift.size() << "\n";
            for (const auto& d : drift) {
                std::cout << "  [" << d.packId << "] " << d.hive << "\\" << d.subkey
                          << " -> " << d.name << " (actual=" << d.currentValue
                          << ", esperado=" << d.expectedValue << ")\n";
            }
            return 0;
        }

        Core::Result appRes = engine.ApplyPreset(preset, Core::kDefaultBackupFile, dryRun);
        std::cout << appRes.message << "\n";
        return appRes.ok ? 0 : 1;
    }

    if (args[0] == "--rollback") {
        std::string backupPath = (args.size() >= 2 && args[1].rfind("--", 0) != 0)
            ? args[1]
            : Core::kDefaultBackupFile;
        Core::TweakEngine engine;
        Core::Result rbRes = engine.RollbackFromBackup(backupPath, dryRun);
        std::cout << rbRes.message << "\n";
        return rbRes.ok ? 0 : 1;
    }

    if (args[0] == "--tray") {
        UI::TrayController tray;
        Core::Result r = tray.Initialize();
        if (!r.ok) {
            std::cerr << r.message << "\n";
            return 2;
        }
        return tray.RunMessageLoop().ok ? 0 : 1;
    }

    if (args[0] == "--ui") {
        UI::MainWindow win;
        Core::Result r = win.CreateAndShow();
        if (!r.ok) {
            std::cerr << r.message << "\n";
            return 2;
        }
        return 0;
    }

    PrintUsage();
    return 1;
}
