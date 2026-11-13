#include "core/restore_point.hpp"

#include "core/jsonc_parser.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace WinOpty::Core {

namespace {

std::string CurrentUtcIso8601() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &tt);
#else
    gmtime_r(&tt, &utc);
#endif
    std::ostringstream oss;
    oss << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string SanitizeDescription(const std::string& raw) {
    std::string clean;
    clean.reserve(raw.size());
    for (unsigned char ch : raw) {
        if (std::isalnum(ch) || ch == ' ' || ch == '-' || ch == '_') {
            clean.push_back(static_cast<char>(ch));
        }
    }
    return clean.empty() ? "Win-Opty Pre-Optimization" : clean;
}

} // namespace

Result RestorePointManager::CreateSystemRestorePoint(const std::string& description, bool dryRun) const {
    const std::string safeDesc = SanitizeDescription(description);
    if (dryRun) {
        return Result::Success("[DRY-RUN] Crearia punto de restauracion: " + safeDesc);
    }

#ifdef _WIN32
    std::string psCmd =
        "-NoProfile -NonInteractive -ExecutionPolicy Bypass -Command "
        "\"Checkpoint-Computer -Description '" + safeDesc + "' -RestorePointType 'MODIFY_SETTINGS' -ErrorAction Stop\"";

    SHELLEXECUTEINFOA sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpFile = "powershell.exe";
    sei.lpParameters = psCmd.c_str();
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExA(&sei) || sei.hProcess == nullptr) {
        return Result::Fail(StatusCode::SystemApiError,
            "No se pudo lanzar PowerShell para Checkpoint-Computer");
    }

    WaitForSingleObject(sei.hProcess, 60000);
    DWORD exitCode = 1;
    GetExitCodeProcess(sei.hProcess, &exitCode);
    CloseHandle(sei.hProcess);

    if (exitCode != 0) {
        return Result::Fail(StatusCode::SystemApiError,
            "Checkpoint-Computer finalizo con codigo " + std::to_string(exitCode) +
            " (requiere permisos de Administrador y Proteccion del Sistema activa)");
    }
    return Result::Success("Punto de restauracion creado: " + safeDesc);
#else
    return Result::Fail(StatusCode::NotImplemented,
        "Checkpoint-Computer solo aplica en Windows");
#endif
}

Result RestorePointManager::SaveStateBackup(
    const std::string& backupFilePath,
    const std::string& presetName,
    const std::vector<RegistryBackupEntry>& entries) const {
    namespace fs = std::filesystem;
    std::error_code ec;

    fs::path targetPath(backupFilePath);
    if (targetPath.has_parent_path()) {
        fs::create_directories(targetPath.parent_path(), ec);
        if (ec) {
            return Result::Fail(StatusCode::IoError,
                "No se pudo crear carpeta de respaldo: " + ec.message());
        }
    }

    fs::path tempPath = targetPath;
    tempPath += ".tmp";

    std::ofstream ofs(tempPath, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        return Result::Fail(StatusCode::IoError,
            "No se pudo escribir respaldo temporal: " + tempPath.string());
    }

    ofs << "{\n"
        << "  \"schemaVersion\": \"1.0.0\",\n"
        << "  \"preset\": " << JsonValue::EscapeString(presetName) << ",\n"
        << "  \"createdAtUtc\": " << JsonValue::EscapeString(CurrentUtcIso8601()) << ",\n"
        << "  \"entries\": [\n";

    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        ofs << "    {\n"
            << "      \"hive\": " << JsonValue::EscapeString(e.hive) << ",\n"
            << "      \"subkey\": " << JsonValue::EscapeString(e.subkey) << ",\n"
            << "      \"name\": " << JsonValue::EscapeString(e.name) << ",\n"
            << "      \"existed\": " << (e.existed ? "true" : "false") << ",\n"
            << "      \"valueType\": " << JsonValue::EscapeString(e.valueType) << ",\n"
            << "      \"dwordValue\": " << e.dwordValue << ",\n"
            << "      \"stringValue\": " << JsonValue::EscapeString(e.stringValue) << "\n"
            << "    }" << (i + 1 < entries.size() ? "," : "") << "\n";
    }

    ofs << "  ]\n}\n";
    ofs.flush();
    if (!ofs) {
        return Result::Fail(StatusCode::IoError, "Error al volcar respaldo JSON");
    }
    ofs.close();

    fs::rename(tempPath, targetPath, ec);
    if (ec) {
        fs::remove(targetPath, ec);
        ec.clear();
        fs::rename(tempPath, targetPath, ec);
        if (ec) {
            return Result::Fail(StatusCode::IoError,
                "No se pudo mover respaldo atomico: " + ec.message());
        }
    }

    return Result::Success("Respaldo guardado en " + targetPath.string());
}

Result RestorePointManager::LoadStateBackup(
    const std::string& backupFilePath,
    std::string& outPresetName,
    std::vector<RegistryBackupEntry>& outEntries) const {
    JsonValue root;
    Result pr = JsonValue::ParseFile(backupFilePath, root);
    if (!pr.ok) return pr;

    if (!root.IsObject()) {
        return Result::Fail(StatusCode::ParseError, "Respaldo JSON invalido: raiz no es objeto");
    }

    if (const auto* preset = root.Get("preset")) {
        outPresetName = preset->AsString();
    }

    outEntries.clear();
    const auto* entriesNode = root.Get("entries");
    if (!entriesNode || !entriesNode->IsArray()) {
        return Result::Fail(StatusCode::ParseError, "Respaldo JSON sin arreglo 'entries'");
    }

    for (const auto& item : entriesNode->AsArray()) {
        if (!item.IsObject()) continue;
        RegistryBackupEntry e;
        if (const auto* v = item.Get("hive"))        e.hive = v->AsString();
        if (const auto* v = item.Get("subkey"))      e.subkey = v->AsString();
        if (const auto* v = item.Get("name"))        e.name = v->AsString();
        if (const auto* v = item.Get("existed"))     e.existed = v->AsBool(false);
        if (const auto* v = item.Get("valueType"))   e.valueType = v->AsString("DWORD");
        if (const auto* v = item.Get("dwordValue"))  e.dwordValue = static_cast<uint32_t>(v->AsUInt64(0));
        if (const auto* v = item.Get("stringValue")) e.stringValue = v->AsString();
        outEntries.push_back(std::move(e));
    }

    return Result::Success("Respaldo cargado (" + std::to_string(outEntries.size()) + " entradas)");
}

} // namespace WinOpty::Core
