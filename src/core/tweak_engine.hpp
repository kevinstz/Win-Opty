#pragma once

#include "core/engine_config.hpp"
#include "core/restore_point.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace WinOpty::Core {

struct TweakOperation {
    std::string hive;       // "HKLM" | "HKCU"
    std::string subkey;
    std::string name;
    std::string type;       // "DWORD" | "STRING"
    uint32_t targetDword   = 0;
    std::string targetString;
    uint32_t defaultDword  = 0;
    std::string defaultString;
};

struct TweakPack {
    std::string id;
    std::string category;
    std::string description;
    bool enabled    = false;
    bool reversible = true;
    std::string minPreset;
    std::vector<TweakOperation> registryChanges;
};

struct PresetConfig {
    std::string presetId;
    std::string displayName;
    std::string description;
    bool createRestorePoint = true;
    std::vector<std::string> enabledPackIds;
};

struct TweakDriftItem {
    std::string packId;
    std::string hive;
    std::string subkey;
    std::string name;
    std::string expectedValue;
    std::string currentValue;
};

class TweakEngine {
public:
    explicit TweakEngine(EnginePolicy policy = {}) : policy_(policy) {}

    Result LoadCatalog(const std::string& packsJsoncPath);
    Result LoadPreset(const std::string& presetJsoncPath, PresetConfig& outPreset) const;

    const std::vector<TweakPack>& Packs() const { return packs_; }
    const TweakPack* FindPack(const std::string& id) const;

    Result ApplyPreset(
        const PresetConfig& preset,
        const std::string& backupFilePath,
        bool dryRun,
        std::vector<RegistryBackupEntry>* outCaptured = nullptr) const;

    Result RollbackFromBackup(const std::string& backupFilePath, bool dryRun) const;

    std::vector<TweakDriftItem> DetectRevertedTweaks(const PresetConfig& preset) const;

private:
    EnginePolicy policy_;
    std::vector<TweakPack> packs_;
    RestorePointManager restoreManager_;

    std::vector<TweakOperation> ExpandWildcardOperations(const TweakOperation& op) const;
};

} // namespace WinOpty::Core
