#pragma once

#include "core/engine_config.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace WinOpty::Core {

struct RegistryBackupEntry {
    std::string hive;        // "HKLM" | "HKCU"
    std::string subkey;
    std::string name;
    bool existed        = false;
    std::string valueType = "DWORD"; // "DWORD" | "STRING"
    uint32_t dwordValue = 0;
    std::string stringValue;
};

class RestorePointManager {
public:
    Result CreateSystemRestorePoint(const std::string& description, bool dryRun = false) const;

    Result SaveStateBackup(
        const std::string& backupFilePath,
        const std::string& presetName,
        const std::vector<RegistryBackupEntry>& entries) const;

    Result LoadStateBackup(
        const std::string& backupFilePath,
        std::string& outPresetName,
        std::vector<RegistryBackupEntry>& outEntries) const;
};

} // namespace WinOpty::Core
