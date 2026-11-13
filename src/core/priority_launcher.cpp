#include "core/module_interfaces.hpp"
#include "core/priority_launcher_helpers.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace WinOpty::Modules {

namespace fs = std::filesystem;

namespace {

std::string ResolveProfilesDir() {
    if (fs::exists("src/presets/profiles")) {
        return "src/presets/profiles";
    }
#ifdef WINOPTY_DEFAULT_PRESETS_DIR
    fs::path p = fs::path(WINOPTY_DEFAULT_PRESETS_DIR) / "profiles";
    if (fs::exists(p)) return p.string();
#endif
    return "presets/profiles";
}

bool IsUtilityExecutable(const std::string& filenameLower) {
    return filenameLower.find("unins") != std::string::npos ||
           filenameLower.find("crash") != std::string::npos ||
           filenameLower.find("redist") != std::string::npos ||
           filenameLower.find("setup") != std::string::npos ||
           filenameLower.find("dxweb") != std::string::npos;
}

std::string FindMainExecutableInDir(const fs::path& installDir) {
    std::error_code ec;
    if (!fs::exists(installDir, ec) || !fs::is_directory(installDir, ec)) {
        return {};
    }

    std::string bestExe;
    uintmax_t bestSize = 0;

    // Primero buscar ejecutables en la raiz del juego (suele ser el binario principal)
    for (const auto& entry : fs::directory_iterator(installDir, ec)) {
        if (ec || !entry.is_regular_file(ec)) continue;
        if (Detail::ToLowerAscii(entry.path().extension().string()) != ".exe") continue;
        if (IsUtilityExecutable(Detail::ToLowerAscii(entry.path().filename().string()))) continue;

        uintmax_t sz = entry.file_size(ec);
        if (!ec && sz >= bestSize) {
            bestSize = sz;
            bestExe = entry.path().string();
        }
    }
    return bestExe;
}

void ApplyProfileIfMatched(
    const std::unordered_map<std::string, Detail::CompetitiveProfileInfo>& profiles,
    DetectedGame& game) {
    std::string exeName = Detail::ToLowerAscii(fs::path(game.executablePath).filename().string());
    auto it = profiles.find(exeName);
    if (it != profiles.end() && !it->second.launchArguments.empty()) {
        game.launchArgs = it->second.launchArguments;
    }
}

#ifdef _WIN32
std::string ReadRegString(HKEY root, const char* subkey, const char* valueName) {
    HKEY hKey = nullptr;
    if (RegOpenKeyExA(root, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return {};
    }
    DWORD type = 0;
    DWORD size = 0;
    if (RegQueryValueExA(hKey, valueName, nullptr, &type, nullptr, &size) != ERROR_SUCCESS ||
        (type != REG_SZ && type != REG_EXPAND_SZ) || size == 0) {
        RegCloseKey(hKey);
        return {};
    }
    std::string buf(size, '\0');
    if (RegQueryValueExA(hKey, valueName, nullptr, &type,
            reinterpret_cast<LPBYTE>(buf.data()), &size) == ERROR_SUCCESS) {
        if (!buf.empty() && buf.back() == '\0') buf.pop_back();
    } else {
        buf.clear();
    }
    RegCloseKey(hKey);
    return buf;
}

void ScanSteamGames(
    const std::unordered_map<std::string, Detail::CompetitiveProfileInfo>& profiles,
    std::vector<DetectedGame>& out) {
    std::string steamPath = ReadRegString(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath");
    if (steamPath.empty()) {
        steamPath = ReadRegString(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Valve\\Steam", "InstallPath");
    }
    if (steamPath.empty()) return;

    fs::path vdf = fs::path(steamPath) / "steamapps" / "libraryfolders.vdf";
    auto libraries = Detail::ExtractSteamLibraryPaths(vdf.string(), steamPath);

    std::error_code ec;
    for (const auto& libRoot : libraries) {
        fs::path steamApps = fs::path(libRoot) / "steamapps";
        if (!fs::exists(steamApps, ec) || !fs::is_directory(steamApps, ec)) continue;

        for (const auto& entry : fs::directory_iterator(steamApps, ec)) {
            if (ec || !entry.is_regular_file(ec)) continue;
            std::string fname = entry.path().filename().string();
            if (fname.rfind("appmanifest_", 0) != 0 || entry.path().extension() != ".acf") continue;

            std::ifstream ifs(entry.path(), std::ios::binary);
            if (!ifs) continue;
            std::ostringstream oss;
            oss << ifs.rdbuf();

            std::string title;
            std::string installDir;
            for (const auto& [k, v] : Detail::ParseSteamKeyValues(oss.str())) {
                if (k == "name" && title.empty()) title = v;
                else if (k == "installdir" && installDir.empty()) installDir = v;
            }
            if (title.empty() || installDir.empty()) continue;

            std::string exe = FindMainExecutableInDir(steamApps / "common" / installDir);
            if (exe.empty()) continue;

            DetectedGame g{"Steam", title, exe, ""};
            ApplyProfileIfMatched(profiles, g);
            out.push_back(std::move(g));
        }
    }
}

void ScanEpicGames(
    const std::unordered_map<std::string, Detail::CompetitiveProfileInfo>& profiles,
    std::vector<DetectedGame>& out) {
    fs::path manifestsDir("C:\\ProgramData\\Epic\\EpicGamesLauncher\\Data\\Manifests");
    std::error_code ec;
    if (!fs::exists(manifestsDir, ec) || !fs::is_directory(manifestsDir, ec)) return;

    for (const auto& entry : fs::directory_iterator(manifestsDir, ec)) {
        if (ec || !entry.is_regular_file(ec)) continue;
        if (entry.path().extension() != ".item") continue;

        DetectedGame g;
        if (Detail::ParseEpicManifestItem(entry.path().string(), profiles, g)) {
            out.push_back(std::move(g));
        }
    }
}

void ScanRegistrySubkeysPlatform(
    const char* parentKey,
    const char* platformName,
    const char* titleValName,
    const char* pathValName,
    const char* exeValName,
    const std::unordered_map<std::string, Detail::CompetitiveProfileInfo>& profiles,
    std::vector<DetectedGame>& out) {
    HKEY hParent = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, parentKey, 0, KEY_READ, &hParent) != ERROR_SUCCESS) {
        return;
    }

    char child[256];
    DWORD idx = 0;
    while (true) {
        DWORD len = sizeof(child);
        if (RegEnumKeyExA(hParent, idx++, child, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
            break;
        }
        std::string fullSub = std::string(parentKey) + "\\" + std::string(child, len);
        std::string title = titleValName ? ReadRegString(HKEY_LOCAL_MACHINE, fullSub.c_str(), titleValName) : std::string(child, len);
        std::string dir   = pathValName  ? ReadRegString(HKEY_LOCAL_MACHINE, fullSub.c_str(), pathValName)  : std::string();
        std::string exe   = exeValName   ? ReadRegString(HKEY_LOCAL_MACHINE, fullSub.c_str(), exeValName)   : std::string();

        if (exe.empty() && !dir.empty()) {
            exe = FindMainExecutableInDir(fs::path(dir));
        }
        if (!title.empty() && !exe.empty()) {
            DetectedGame g{platformName, title, exe, ""};
            ApplyProfileIfMatched(profiles, g);
            out.push_back(std::move(g));
        }
    }
    RegCloseKey(hParent);
}

void ScanUninstallForPublishers(
    const std::unordered_map<std::string, Detail::CompetitiveProfileInfo>& profiles,
    std::vector<DetectedGame>& out) {
    const char* uninstallKey = "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
    HKEY hParent = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, uninstallKey, 0, KEY_READ, &hParent) != ERROR_SUCCESS) {
        return;
    }

    char child[256];
    DWORD idx = 0;
    while (true) {
        DWORD len = sizeof(child);
        if (RegEnumKeyExA(hParent, idx++, child, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
            break;
        }
        std::string fullSub = std::string(uninstallKey) + "\\" + std::string(child, len);
        std::string publisher = Detail::ToLowerAscii(ReadRegString(HKEY_LOCAL_MACHINE, fullSub.c_str(), "Publisher"));
        std::string platform;
        if (publisher.find("blizzard") != std::string::npos || publisher.find("activision") != std::string::npos) {
            platform = "Battle.net";
        } else if (publisher.find("rockstar") != std::string::npos) {
            platform = "Rockstar Games";
        } else if (publisher.find("electronic arts") != std::string::npos) {
            platform = "EA App";
        } else {
            continue;
        }

        std::string title = ReadRegString(HKEY_LOCAL_MACHINE, fullSub.c_str(), "DisplayName");
        std::string installLoc = ReadRegString(HKEY_LOCAL_MACHINE, fullSub.c_str(), "InstallLocation");
        if (title.empty() || installLoc.empty()) continue;

        std::string exe = FindMainExecutableInDir(fs::path(installLoc));
        if (exe.empty()) continue;

        DetectedGame g{platform, title, exe, ""};
        ApplyProfileIfMatched(profiles, g);
        out.push_back(std::move(g));
    }
    RegCloseKey(hParent);
}

DWORD_PTR ComputeFastCoresAffinityMask() {
    DWORD bufferSize = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bufferSize);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || bufferSize == 0) {
        return 0;
    }

    std::vector<uint8_t> buffer(bufferSize);
    if (!GetLogicalProcessorInformationEx(
            RelationProcessorCore,
            reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()),
            &bufferSize)) {
        return 0;
    }

    BYTE maxEfficiency = 0;
    size_t offset = 0;
    while (offset < bufferSize) {
        auto* info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data() + offset);
        if (info->Relationship == RelationProcessorCore) {
            maxEfficiency = std::max(maxEfficiency, info->Processor.EfficiencyClass);
        }
        offset += info->Size;
    }

    DWORD_PTR mask = 0;
    offset = 0;
    while (offset < bufferSize) {
        auto* info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data() + offset);
        if (info->Relationship == RelationProcessorCore &&
            info->Processor.EfficiencyClass == maxEfficiency &&
            info->Processor.GroupCount > 0 &&
            info->Processor.GroupMask[0].Group == 0) {
            mask |= info->Processor.GroupMask[0].Mask;
        }
        offset += info->Size;
    }
    return mask;
}
#endif

} // namespace

std::vector<DetectedGame> PriorityLauncher::ScanInstalledGames() const {
    std::vector<DetectedGame> games;
    auto profiles = Detail::LoadCompetitiveProfiles(ResolveProfilesDir());

#ifdef _WIN32
    ScanSteamGames(profiles, games);
    ScanEpicGames(profiles, games);
    ScanRegistrySubkeysPlatform("SOFTWARE\\WOW6432Node\\GOG.com\\Games", "GOG", "gameName", "path", "exe", profiles, games);
    ScanRegistrySubkeysPlatform("SOFTWARE\\WOW6432Node\\Ubisoft\\Launcher\\Installs", "Ubisoft Connect", nullptr, "InstallDir", nullptr, profiles, games);
    ScanUninstallForPublishers(profiles, games);
#else
    (void)profiles;
#endif
    return games;
}

Core::Result PriorityLauncher::LaunchGame(const DetectedGame& game, uint32_t* outPid) {
    if (game.executablePath.empty()) {
        return Core::Result::Fail(Core::StatusCode::InvalidArgument, "Ruta de ejecutable vacia");
    }

    std::error_code ec;
    if (!fs::exists(game.executablePath, ec)) {
        return Core::Result::Fail(Core::StatusCode::IoError,
            "No existe el ejecutable del juego: " + game.executablePath);
    }

    if (preCleanRamOnStart) {
        MemoryCleaner cleaner;
        (void)cleaner.ExecuteTrim();
    }

#ifdef _WIN32
    std::string cmdLine = "\"" + game.executablePath + "\"";
    if (!game.launchArgs.empty()) {
        cmdLine += " " + game.launchArgs;
    }
    std::vector<char> mutableCmd(cmdLine.begin(), cmdLine.end());
    mutableCmd.push_back('\0');

    const std::string workDir = fs::path(game.executablePath).parent_path().string();
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    DWORD creationFlags = CREATE_SUSPENDED;
    if (setHighCpuPriority) {
        creationFlags |= HIGH_PRIORITY_CLASS;
    }

    if (!CreateProcessA(
            game.executablePath.c_str(),
            mutableCmd.data(),
            nullptr,
            nullptr,
            FALSE,
            creationFlags,
            nullptr,
            workDir.empty() ? nullptr : workDir.c_str(),
            &si,
            &pi)) {
        return Core::Result::Fail(Core::StatusCode::SystemApiError,
            "CreateProcessA fallo (err=" + std::to_string(GetLastError()) + ")");
    }

    if (bindFastCoresOnly) {
        DWORD_PTR fastMask = ComputeFastCoresAffinityMask();
        DWORD_PTR procAffinity = 0;
        DWORD_PTR sysAffinity = 0;
        if (fastMask != 0 && GetProcessAffinityMask(pi.hProcess, &procAffinity, &sysAffinity)) {
            DWORD_PTR safeMask = fastMask & sysAffinity;
            if (safeMask != 0) {
                SetProcessAffinityMask(pi.hProcess, safeMask);
            }
        }
    }

    ResumeThread(pi.hThread);
    if (outPid) {
        *outPid = static_cast<uint32_t>(pi.dwProcessId);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return Core::Result::Success("Juego lanzado con prioridad alta (PID=" +
        std::to_string(pi.dwProcessId) + ")");
#else
    (void)outPid;
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "Lanzador nativo Win32 solo disponible en Windows (usar scripts/linux/gamemode_mangohud.sh)");
#endif
}

} // namespace WinOpty::Modules
