#include "core/module_interfaces.hpp"

#include <cstring>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace WinOpty::Modules {

namespace {

#ifdef _WIN32
constexpr ULONG kSystemMemoryListInformation = 80;

enum class MemoryListCommand : ULONG {
    MemoryEmptyWorkingSets  = 2,
    MemoryFlushModifiedList = 3,
    MemoryPurgeStandbyList  = 4
};

using NtSetSystemInformationFn = LONG (NTAPI*)(ULONG SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength);

Core::Result EnablePrivilege(const char* privilegeName) {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        return Core::Result::Fail(Core::StatusCode::AccessDenied,
            "OpenProcessToken fallo (err=" + std::to_string(GetLastError()) + ")");
    }

    LUID luid{};
    if (!LookupPrivilegeValueA(nullptr, privilegeName, &luid)) {
        DWORD err = GetLastError();
        CloseHandle(token);
        return Core::Result::Fail(Core::StatusCode::SystemApiError,
            "LookupPrivilegeValueA fallo (err=" + std::to_string(err) + ")");
    }

    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    SetLastError(ERROR_SUCCESS);
    BOOL adjusted = AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    DWORD lastErr = GetLastError();
    CloseHandle(token);

    if (!adjusted || lastErr == ERROR_NOT_ALL_ASSIGNED) {
        return Core::Result::Fail(Core::StatusCode::AccessDenied,
            "Privilegio '" + std::string(privilegeName) + "' no asignado (ejecuta como Administrador)");
    }
    return Core::Result::Success();
}

Core::Result IssueMemoryListCommand(NtSetSystemInformationFn fn, MemoryListCommand cmd) {
    ULONG commandValue = static_cast<ULONG>(cmd);
    LONG status = fn(kSystemMemoryListInformation, &commandValue, sizeof(commandValue));
    if (status == 0) {
        return Core::Result::Success();
    }
    if (static_cast<uint32_t>(status) == 0xC0000061u) {
        return Core::Result::Fail(Core::StatusCode::AccessDenied,
            "NTSTATUS STATUS_PRIVILEGE_NOT_HELD al limpiar listas de memoria");
    }
    return Core::Result::Fail(Core::StatusCode::SystemApiError,
        "NtSetSystemInformation retorno NTSTATUS=" + std::to_string(status));
}
#endif

} // namespace

double MemoryCleaner::GetCurrentMemoryLoadPercent() const {
#ifdef _WIN32
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        return static_cast<double>(mem.dwMemoryLoad);
    }
#endif
    return 0.0;
}

bool MemoryCleaner::ShouldAutoClean() const {
    return GetCurrentMemoryLoadPercent() >= static_cast<double>(autoCleanThreshold);
}

Core::Result MemoryCleaner::ExecuteTrim() {
#ifdef _WIN32
    Core::Result privRes = EnablePrivilege(SE_PROF_SINGLE_PROCESS_NAME);
    if (!privRes.ok) {
        return privRes;
    }

    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) {
        return Core::Result::Fail(Core::StatusCode::SystemApiError, "No se encontro ntdll.dll");
    }

    FARPROC proc = GetProcAddress(ntdll, "NtSetSystemInformation");
    NtSetSystemInformationFn ntSetSysInfo = nullptr;
    static_assert(sizeof(ntSetSysInfo) == sizeof(proc));
    std::memcpy(&ntSetSysInfo, &proc, sizeof(ntSetSysInfo));
    if (!ntSetSysInfo) {
        return Core::Result::Fail(Core::StatusCode::SystemApiError,
            "No se pudo resolver NtSetSystemInformation");
    }

    int executedOps = 0;
    if (clearSystemWorkingSet) {
        Core::Result r = IssueMemoryListCommand(ntSetSysInfo, MemoryListCommand::MemoryEmptyWorkingSets);
        if (!r.ok) return r;
        ++executedOps;
    }
    if (clearModifiedPageLists) {
        Core::Result r = IssueMemoryListCommand(ntSetSysInfo, MemoryListCommand::MemoryFlushModifiedList);
        if (!r.ok) return r;
        ++executedOps;
    }
    if (clearStandbyPageLists) {
        Core::Result r = IssueMemoryListCommand(ntSetSysInfo, MemoryListCommand::MemoryPurgeStandbyList);
        if (!r.ok) return r;
        ++executedOps;
    }

    std::ostringstream oss;
    oss << "Limpieza de RAM completada (" << executedOps << " operaciones; carga actual "
        << static_cast<int>(GetCurrentMemoryLoadPercent()) << "%)";
    return Core::Result::Success(oss.str());
#else
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "NtSetSystemInformation solo disponible en Windows (en Linux usar scripts/linux/drop_caches.sh)");
#endif
}

} // namespace WinOpty::Modules
