#pragma once

#include "core/engine_config.hpp"

#include <cctype>
#include <cstdint>
#include <string>
#include <unordered_set>

namespace WinOpty::Modules::Detail {

struct SavedProcessState {
    uint32_t pid = 0;
    std::string imageName;
    uint32_t previousPriorityClass = 0;
};

inline std::string LowerAsciiCopy(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

inline bool IsProtectedSystemProcess(const std::string& exeName) {
    static const std::unordered_set<std::string> kProtected = {
        "system", "idle", "registry", "smss.exe", "csrss.exe", "wininit.exe",
        "services.exe", "lsass.exe", "lsm.exe", "svchost.exe", "dwm.exe",
        "winlogon.exe", "explorer.exe", "fontdrvhost.exe", "audiodg.exe",
        "sihost.exe", "taskhostw.exe", "conhost.exe", "runtimebroker.exe",
        "win-opty.exe", "win-opty-tests.exe",
        // Servicios anti-cheat que nunca deben alterarse
        "vgc.exe", "vgtray.exe", "easyanticheat.exe", "easyanticheat_eos.exe",
        "beservice.exe", "bedaisy.exe", "faceitclient.exe", "faceitservice.exe"
    };
    return kProtected.find(LowerAsciiCopy(exeName)) != kProtected.end();
}

inline bool IsThrottableBackgroundProcess(const std::string& exeName) {
    const std::string lower = LowerAsciiCopy(exeName);
    if (IsProtectedSystemProcess(lower)) {
        return false;
    }
    static const std::unordered_set<std::string> kThrottable = {
        "searchindexer.exe",
        "searchprotocolhost.exe",
        "searchfilterhost.exe",
        "onedrive.exe",
        "googleupdate.exe",
        "microsoftedgeupdate.exe",
        "adobearm.exe",
        "ccxprocess.exe",
        "acrotray.exe",
        "jusched.exe",
        "steamwebhelper.exe",
        "epicwebhelper.exe",
        "crashpad_handler.exe"
    };
    return kThrottable.find(lower) != kThrottable.end();
}

} // namespace WinOpty::Modules::Detail
