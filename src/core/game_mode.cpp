#include "core/game_mode_helpers.hpp"
#include "core/module_interfaces.hpp"

#include <mutex>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif

namespace WinOpty::Modules {

namespace {

std::mutex g_gameModeMutex;
std::unordered_map<uint32_t, std::vector<Detail::SavedProcessState>> g_activeGameSnapshots;

#ifdef _WIN32
bool LowerProcessPriority(uint32_t targetPid, const std::string& imageName, Detail::SavedProcessState& outSaved) {
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_INFORMATION, FALSE, targetPid);
    if (!hProc) {
        return false;
    }

    DWORD prevClass = GetPriorityClass(hProc);
    if (prevClass == 0) {
        CloseHandle(hProc);
        return false;
    }

    if (prevClass == IDLE_PRIORITY_CLASS) {
        CloseHandle(hProc);
        return false;
    }

    if (!SetPriorityClass(hProc, IDLE_PRIORITY_CLASS)) {
        CloseHandle(hProc);
        return false;
    }

    CloseHandle(hProc);
    outSaved.pid = targetPid;
    outSaved.imageName = imageName;
    outSaved.previousPriorityClass = static_cast<uint32_t>(prevClass);
    return true;
}

bool RestoreProcessPriority(const Detail::SavedProcessState& saved) {
    HANDLE hProc = OpenProcess(PROCESS_SET_INFORMATION, FALSE, saved.pid);
    if (!hProc) {
        return false;
    }
    BOOL ok = SetPriorityClass(hProc, static_cast<DWORD>(saved.previousPriorityClass));
    CloseHandle(hProc);
    return ok != FALSE;
}
#endif

} // namespace

Core::Result DynamicGameMode::OnGameStart(uint32_t pid) {
    if (pid == 0) {
        return Core::Result::Fail(Core::StatusCode::InvalidArgument, "PID de juego invalido (0)");
    }

#ifdef _WIN32
    const DWORD selfPid = GetCurrentProcessId();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return Core::Result::Fail(Core::StatusCode::SystemApiError,
            "CreateToolhelp32Snapshot fallo (err=" + std::to_string(GetLastError()) + ")");
    }

    std::vector<Detail::SavedProcessState> captured;
    PROCESSENTRY32 pe{};
    pe.dwSize = sizeof(pe);

    if (Process32First(snapshot, &pe)) {
        do {
            const DWORD procPid = pe.th32ProcessID;
            if (procPid == 0 || procPid == selfPid || procPid == pid) {
                continue;
            }
            const std::string exeName(pe.szExeFile);
            if (!Detail::IsThrottableBackgroundProcess(exeName)) {
                continue;
            }

            Detail::SavedProcessState saved;
            if (LowerProcessPriority(procPid, exeName, saved)) {
                captured.push_back(std::move(saved));
            }
        } while (Process32Next(snapshot, &pe));
    }
    CloseHandle(snapshot);

    {
        std::lock_guard<std::mutex> lock(g_gameModeMutex);
        g_activeGameSnapshots[pid] = captured;
    }

    return Core::Result::Success("Modo Juego activo para PID " + std::to_string(pid) +
        " (" + std::to_string(captured.size()) + " procesos secundarios priorizados a Idle)");
#else
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "DynamicGameMode solo disponible en Windows (en Linux usar Feral GameMode)");
#endif
}

Core::Result DynamicGameMode::OnGameExit(uint32_t pid) {
    if (pid == 0) {
        return Core::Result::Fail(Core::StatusCode::InvalidArgument, "PID de juego invalido (0)");
    }

    std::vector<Detail::SavedProcessState> toRestore;
    {
        std::lock_guard<std::mutex> lock(g_gameModeMutex);
        auto it = g_activeGameSnapshots.find(pid);
        if (it == g_activeGameSnapshots.end()) {
            return Core::Result::Success("Sin estado previo pendiente para PID " + std::to_string(pid));
        }
        toRestore = std::move(it->second);
        g_activeGameSnapshots.erase(it);
    }

#ifdef _WIN32
    size_t restored = 0;
    for (const auto& item : toRestore) {
        if (RestoreProcessPriority(item)) {
            ++restored;
        }
    }
    return Core::Result::Success("Modo Juego restaurado para PID " + std::to_string(pid) +
        " (" + std::to_string(restored) + "/" + std::to_string(toRestore.size()) + " procesos)");
#else
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "DynamicGameMode solo disponible en Windows");
#endif
}

} // namespace WinOpty::Modules
