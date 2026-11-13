#pragma once

#include "core/jsonc_parser.hpp"
#include "core/module_interfaces.hpp"

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace WinOpty::Modules::Detail {

struct CompetitiveProfileInfo {
    std::string executable;
    std::string profileName;
    std::string launchArguments;
    bool preCleanRamOnStart = true;
    bool bindFastCoresOnly  = true;
};

inline std::string ToLowerAscii(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

inline std::unordered_map<std::string, CompetitiveProfileInfo> LoadCompetitiveProfiles(const std::string& profilesDir) {
    namespace fs = std::filesystem;
    std::unordered_map<std::string, CompetitiveProfileInfo> map;
    std::error_code ec;
    if (!fs::exists(profilesDir, ec) || !fs::is_directory(profilesDir, ec)) {
        return map;
    }

    for (const auto& entry : fs::directory_iterator(profilesDir, ec)) {
        if (ec || !entry.is_regular_file(ec)) continue;
        if (entry.path().extension() != ".jsonc") continue;

        Core::JsonValue root;
        if (!Core::JsonValue::ParseFile(entry.path().string(), root).ok || !root.IsObject()) continue;

        CompetitiveProfileInfo info;
        if (const auto* v = root.Get("executable"))         info.executable = v->AsString();
        if (const auto* v = root.Get("profileName"))        info.profileName = v->AsString();
        if (const auto* v = root.Get("launchArguments"))    info.launchArguments = v->AsString();
        if (const auto* v = root.Get("preCleanRamOnStart")) info.preCleanRamOnStart = v->AsBool(true);
        if (const auto* v = root.Get("bindFastCoresOnly"))  info.bindFastCoresOnly = v->AsBool(true);

        if (!info.executable.empty()) {
            map[ToLowerAscii(info.executable)] = std::move(info);
        }
    }
    return map;
}

// Extrae pares "clave" "valor" de archivos VDF/ACF simples de Steam
inline std::vector<std::pair<std::string, std::string>> ParseSteamKeyValues(const std::string& content) {
    std::vector<std::pair<std::string, std::string>> pairs;
    std::vector<std::string> tokens;

    for (size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '"') {
            ++i;
            std::string tok;
            while (i < content.size() && content[i] != '"') {
                if (content[i] == '\\' && i + 1 < content.size()) {
                    char next = content[++i];
                    if (next == '\\') tok.push_back('\\');
                    else tok.push_back(next);
                } else {
                    tok.push_back(content[i]);
                }
                ++i;
            }
            tokens.push_back(std::move(tok));
        } else if (content[i] == '{' || content[i] == '}') {
            tokens.emplace_back(1, content[i]);
        }
    }

    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
        if (tokens[i] != "{" && tokens[i] != "}" && tokens[i + 1] != "{" && tokens[i + 1] != "}") {
            pairs.emplace_back(ToLowerAscii(tokens[i]), tokens[i + 1]);
            ++i;
        }
    }
    return pairs;
}

inline std::vector<std::string> ExtractSteamLibraryPaths(const std::string& vdfPath, const std::string& fallbackSteamRoot) {
    namespace fs = std::filesystem;
    std::vector<std::string> libraries;
    if (!fallbackSteamRoot.empty()) {
        libraries.push_back(fallbackSteamRoot);
    }

    std::ifstream ifs(vdfPath, std::ios::binary);
    if (!ifs) return libraries;

    std::ostringstream oss;
    oss << ifs.rdbuf();
    for (const auto& [k, v] : ParseSteamKeyValues(oss.str())) {
        if (k == "path" && !v.empty()) {
            fs::path p(v);
            std::string normalized = p.lexically_normal().string();
            bool dup = false;
            for (const auto& existing : libraries) {
                if (ToLowerAscii(existing) == ToLowerAscii(normalized)) {
                    dup = true;
                    break;
                }
            }
            if (!dup) libraries.push_back(std::move(normalized));
        }
    }
    return libraries;
}

inline bool ParseEpicManifestItem(
    const std::string& itemFilePath,
    const std::unordered_map<std::string, CompetitiveProfileInfo>& profiles,
    DetectedGame& outGame) {
    namespace fs = std::filesystem;
    Core::JsonValue root;
    if (!Core::JsonValue::ParseFile(itemFilePath, root).ok || !root.IsObject()) {
        return false;
    }

    std::string displayName;
    std::string installLocation;
    std::string launchExecutable;
    std::string launchCommand;

    if (const auto* v = root.Get("DisplayName"))      displayName = v->AsString();
    if (const auto* v = root.Get("InstallLocation"))  installLocation = v->AsString();
    if (const auto* v = root.Get("LaunchExecutable")) launchExecutable = v->AsString();
    if (const auto* v = root.Get("LaunchCommand"))    launchCommand = v->AsString();

    if (displayName.empty() || installLocation.empty() || launchExecutable.empty()) {
        return false;
    }

    fs::path exePath = fs::path(installLocation) / launchExecutable;
    outGame.platform = "Epic Games";
    outGame.title = displayName;
    outGame.executablePath = exePath.lexically_normal().string();
    outGame.launchArgs = launchCommand;

    std::string exeNameLower = ToLowerAscii(exePath.filename().string());
    auto it = profiles.find(exeNameLower);
    if (it != profiles.end() && !it->second.launchArguments.empty()) {
        outGame.launchArgs = it->second.launchArguments;
    }
    return true;
}

} // namespace WinOpty::Modules::Detail
