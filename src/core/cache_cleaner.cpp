#include "core/module_interfaces.hpp"

#include <cstdlib>
#include <filesystem>
#include <system_error>
#include <vector>

namespace WinOpty::Modules {

namespace fs = std::filesystem;

namespace {

struct CacheCategoryTarget {
    fs::path directory;
    bool onlyDumpAndLogExtensions = false;
};

std::string GetEnvString(const char* name) {
    const char* val = std::getenv(name);
    return val ? std::string(val) : std::string();
}

bool HasDumpOrLogExtension(const fs::path& p) {
    std::string ext = p.extension().string();
    for (char& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return ext == ".dmp" || ext == ".log" || ext == ".tmp";
}

std::vector<CacheCategoryTarget> BuildTargets(const CacheCleaner& cfg) {
    std::vector<CacheCategoryTarget> targets;

    if (!cfg.testSandboxRoot.empty()) {
        fs::path root(cfg.testSandboxRoot);
        if (cfg.cleanOsTempFiles)   targets.push_back({root / "os_temp", false});
        if (cfg.cleanShaderCaches)  targets.push_back({root / "shader_cache", false});
        if (cfg.cleanBrowserCaches) targets.push_back({root / "browser_cache", false});
        if (cfg.cleanGameLogs)      targets.push_back({root / "game_logs", true});
        return targets;
    }

#ifdef _WIN32
    const std::string tempDir      = GetEnvString("TEMP");
    const std::string localAppData = GetEnvString("LOCALAPPDATA");
    const std::string winDir       = GetEnvString("WINDIR");

    if (cfg.cleanOsTempFiles) {
        if (!tempDir.empty()) targets.push_back({fs::path(tempDir), false});
        if (!winDir.empty())  targets.push_back({fs::path(winDir) / "Temp", false});
    }

    if (cfg.cleanShaderCaches && !localAppData.empty()) {
        fs::path lad(localAppData);
        targets.push_back({lad / "D3DSCache", false});
        targets.push_back({lad / "NVIDIA" / "DXCache", false});
        targets.push_back({lad / "NVIDIA" / "GLCache", false});
        targets.push_back({lad / "AMD" / "DxCache", false});
        targets.push_back({lad / "AMD" / "VkCache", false});
    }

    if (cfg.cleanBrowserCaches && !localAppData.empty()) {
        fs::path lad(localAppData);
        // Solo cache de disco/codigo; nunca cookies, historial ni credenciales.
        targets.push_back({lad / "Microsoft" / "Edge" / "User Data" / "Default" / "Cache" / "Cache_Data", false});
        targets.push_back({lad / "Google" / "Chrome" / "User Data" / "Default" / "Cache" / "Cache_Data", false});
        targets.push_back({lad / "BraveSoftware" / "Brave-Browser" / "User Data" / "Default" / "Cache" / "Cache_Data", false});
    }

    if (cfg.cleanGameLogs && !localAppData.empty()) {
        targets.push_back({fs::path(localAppData) / "CrashDumps", true});
    }
#endif

    return targets;
}

void ProcessTargets(const std::vector<CacheCategoryTarget>& targets, bool deleteFiles, bool dryRun, CacheCleanSummary& summary) {
    std::error_code ec;
    for (const auto& target : targets) {
        if (!fs::exists(target.directory, ec) || !fs::is_directory(target.directory, ec)) {
            ec.clear();
            continue;
        }

        fs::recursive_directory_iterator it(
            target.directory,
            fs::directory_options::skip_permission_denied,
            ec);
        fs::recursive_directory_iterator end;

        for (; it != end; it.increment(ec)) {
            if (ec) {
                ec.clear();
                continue;
            }
            const fs::directory_entry& entry = *it;
            if (!entry.is_regular_file(ec)) {
                ec.clear();
                continue;
            }
            if (target.onlyDumpAndLogExtensions && !HasDumpOrLogExtension(entry.path())) {
                continue;
            }

            uint64_t fsize = entry.file_size(ec);
            if (ec) {
                ec.clear();
                ++summary.skippedFiles;
                continue;
            }
            summary.scannedBytes += fsize;

            if (deleteFiles) {
                if (dryRun) {
                    summary.deletedBytes += fsize;
                    ++summary.deletedFiles;
                } else {
                    if (fs::remove(entry.path(), ec) && !ec) {
                        summary.deletedBytes += fsize;
                        ++summary.deletedFiles;
                    } else {
                        ec.clear();
                        ++summary.skippedFiles;
                    }
                }
            }
        }
    }
}

} // namespace

uint64_t CacheCleaner::ScanRecoverableBytes() const {
    CacheCleanSummary summary;
    ProcessTargets(BuildTargets(*this), false, true, summary);
    return summary.scannedBytes;
}

Core::Result CacheCleaner::CleanSelected(bool dryRun, CacheCleanSummary* outSummary) const {
    CacheCleanSummary summary;
    ProcessTargets(BuildTargets(*this), true, dryRun, summary);
    if (outSummary) {
        *outSummary = summary;
    }
    return Core::Result::Success(
        (dryRun ? "[DRY-RUN] " : "") +
        std::string("Cache escaneada: ") + std::to_string(summary.scannedBytes) +
        " bytes, recuperables/borrados: " + std::to_string(summary.deletedBytes) + " bytes");
}

} // namespace WinOpty::Modules
