#pragma once

#include <cstdint>
#include <string>

namespace WinOpty::Core {

// Politica base de ejecucion (README.md -> Principios de Ingenieria)
struct EnginePolicy {
    bool runInSystemTray     = true;  // Ejecucion persistente en bandeja con consumo < 15 MB RAM
    bool transactionalTweaks = true;  // Cada interruptor guarda el valor previo antes de escribir
    bool offlineFirst        = true;  // Cero dependencia de servidores externos o cuentas de usuario
    bool autoRestorePoint    = true;  // Snapshot / punto de restauracion previo a cambios criticos
};

enum class StatusCode {
    Ok = 0,
    NotImplemented,
    InvalidArgument,
    IoError,
    AccessDenied,
    SystemApiError,
    ParseError
};

struct Result {
    bool ok = true;
    StatusCode code = StatusCode::Ok;
    std::string message;

    static Result Success(std::string msg = {}) {
        return Result{true, StatusCode::Ok, std::move(msg)};
    }

    static Result Fail(StatusCode errCode, std::string msg) {
        return Result{false, errCode, std::move(msg)};
    }
};

inline constexpr const char* kAppVersion = "0.1.0";
inline constexpr const char* kDefaultBackupFile = "./backups/last_known_good.json";

} // namespace WinOpty::Core
