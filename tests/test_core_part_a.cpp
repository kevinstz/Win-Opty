#include "core/jsonc_parser.hpp"
#include "core/module_interfaces.hpp"
#include "core/restore_point.hpp"
#include "core/tweak_engine.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace {

void TestJsoncCommentStrippingPreservesStrings() {
    const std::string raw = R"({
        // Comentario de linea completo
        "url": "https://github.com/kevinstz/Win-Opty", /* bloque inline */
        "maxUint32": 4294967295,
        "enabled": true
    })";

    WinOpty::Core::JsonValue val;
    auto res = WinOpty::Core::JsonValue::Parse(raw, val);
    assert(res.ok);
    assert(val.IsObject());
    assert(val.Get("url")->AsString() == "https://github.com/kevinstz/Win-Opty");
    assert(val.Get("maxUint32")->AsUInt64() == 4294967295ULL);
    assert(val.Get("enabled")->AsBool() == true);
    std::cout << "[PASS] TestJsoncCommentStrippingPreservesStrings\n";
}

void TestBackupRoundTrip() {
    fs::path tmpDir = fs::temp_directory_path() / "winopty_test_backup";
    fs::remove_all(tmpDir);
    fs::create_directories(tmpDir);
    fs::path backupFile = tmpDir / "last_known_good.json";

    WinOpty::Core::RestorePointManager mgr;
    std::vector<WinOpty::Core::RegistryBackupEntry> entries = {
        {"HKLM", "SOFTWARE\\Test\\Sub", "ValA", true, "DWORD", 20, ""},
        {"HKCU", "System\\GameConfigStore", "NewKey", false, "DWORD", 0, ""},
        {"HKLM", "SOFTWARE\\Test\\Sub", "Mode", true, "STRING", 0, "Medium"}
    };

    auto saveRes = mgr.SaveStateBackup(backupFile.string(), "EXTREMO", entries);
    assert(saveRes.ok);

    std::string loadedPreset;
    std::vector<WinOpty::Core::RegistryBackupEntry> loadedEntries;
    auto loadRes = mgr.LoadStateBackup(backupFile.string(), loadedPreset, loadedEntries);
    assert(loadRes.ok);
    assert(loadedPreset == "EXTREMO");
    assert(loadedEntries.size() == 3);
    assert(loadedEntries[0].dwordValue == 20);
    assert(loadedEntries[1].existed == false);
    assert(loadedEntries[2].stringValue == "Medium");

    fs::remove_all(tmpDir);
    std::cout << "[PASS] TestBackupRoundTrip\n";
}

void TestPresetsAndTweakEngineDryRun() {
#ifdef WINOPTY_DEFAULT_PRESETS_DIR
    std::string presetsDir = WINOPTY_DEFAULT_PRESETS_DIR;
#else
    std::string presetsDir = "src/presets";
#endif
    WinOpty::Core::TweakEngine engine;
    auto catRes = engine.LoadCatalog(presetsDir + "/packs.jsonc");
    assert(catRes.ok);
    assert(!engine.Packs().empty());

    WinOpty::Core::PresetConfig extreme;
    auto preRes = engine.LoadPreset(presetsDir + "/extreme.jsonc", extreme);
    assert(preRes.ok);
    assert(extreme.presetId == "EXTREMO");

    std::vector<WinOpty::Core::RegistryBackupEntry> captured;
    auto applyRes = engine.ApplyPreset(extreme, "", true, &captured);
    assert(applyRes.ok);
    assert(!captured.empty());
    std::cout << "[PASS] TestPresetsAndTweakEngineDryRun (" << captured.size() << " ops)\n";
}

void TestCacheCleanerSandbox() {
    fs::path sandbox = fs::temp_directory_path() / "winopty_test_cache";
    fs::remove_all(sandbox);
    fs::create_directories(sandbox / "os_temp");
    fs::create_directories(sandbox / "shader_cache");
    fs::create_directories(sandbox / "game_logs");

    {
        std::ofstream f1(sandbox / "os_temp" / "a.tmp", std::ios::binary);
        f1 << "12345";
        std::ofstream f2(sandbox / "shader_cache" / "dx.bin", std::ios::binary);
        f2 << "1234567890";
        std::ofstream f3(sandbox / "game_logs" / "crash.dmp", std::ios::binary);
        f3 << "123";
        std::ofstream f4(sandbox / "game_logs" / "keep.txt", std::ios::binary);
        f4 << "keep";
    }

    WinOpty::Modules::CacheCleaner cleaner;
    cleaner.testSandboxRoot = sandbox.string();
    cleaner.cleanOsTempFiles = true;
    cleaner.cleanShaderCaches = true;
    cleaner.cleanBrowserCaches = false;
    cleaner.cleanGameLogs = true;

    uint64_t bytes = cleaner.ScanRecoverableBytes();
    assert(bytes == 18); // 5 + 10 + 3 (keep.txt ignorado por filtro .dmp/.log/.tmp)

    WinOpty::Modules::CacheCleanSummary summary;
    auto cleanRes = cleaner.CleanSelected(false, &summary);
    assert(cleanRes.ok);
    assert(summary.deletedFiles == 3);
    assert(summary.deletedBytes == 18);
    assert(fs::exists(sandbox / "game_logs" / "keep.txt"));

    fs::remove_all(sandbox);
    std::cout << "[PASS] TestCacheCleanerSandbox\n";
}

} // namespace

int main() {
    TestJsoncCommentStrippingPreservesStrings();
    TestBackupRoundTrip();
    TestPresetsAndTweakEngineDryRun();
    TestCacheCleanerSandbox();
    std::cout << "\nTodos los tests unitarios de Parte A pasaron correctamente.\n";
    return 0;
}
