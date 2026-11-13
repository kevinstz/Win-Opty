#include "core/tweak_engine.hpp"

#include "core/jsonc_parser.hpp"

#include <algorithm>
#include <unordered_set>

#ifdef _WIN32
#include <windows.h>
#endif

namespace WinOpty::Core {

namespace {

#ifdef _WIN32
HKEY ResolveRootHive(const std::string& hiveName) {
    if (hiveName == "HKLM" || hiveName == "HKEY_LOCAL_MACHINE") return HKEY_LOCAL_MACHINE;
    if (hiveName == "HKCU" || hiveName == "HKEY_CURRENT_USER")  return HKEY_CURRENT_USER;
    return nullptr;
}

RegistryBackupEntry ReadRegistryEntry(const TweakOperation& op) {
    RegistryBackupEntry entry;
    entry.hive = op.hive;
    entry.subkey = op.subkey;
    entry.name = op.name;
    entry.valueType = op.type;

    HKEY root = ResolveRootHive(op.hive);
    if (!root) return entry;

    HKEY hKey = nullptr;
    if (RegOpenKeyExA(root, op.subkey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        entry.existed = false;
        return entry;
    }

    DWORD regType = 0;
    DWORD dataSize = 0;
    LSTATUS q = RegQueryValueExA(hKey, op.name.c_str(), nullptr, &regType, nullptr, &dataSize);
    if (q != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        entry.existed = false;
        return entry;
    }

    entry.existed = true;
    if (regType == REG_DWORD && dataSize == sizeof(DWORD)) {
        DWORD val = 0;
        if (RegQueryValueExA(hKey, op.name.c_str(), nullptr, &regType,
                reinterpret_cast<LPBYTE>(&val), &dataSize) == ERROR_SUCCESS) {
            entry.valueType = "DWORD";
            entry.dwordValue = static_cast<uint32_t>(val);
        }
    } else if ((regType == REG_SZ || regType == REG_EXPAND_SZ) && dataSize > 0) {
        std::string buf(dataSize, '\0');
        if (RegQueryValueExA(hKey, op.name.c_str(), nullptr, &regType,
                reinterpret_cast<LPBYTE>(buf.data()), &dataSize) == ERROR_SUCCESS) {
            if (!buf.empty() && buf.back() == '\0') buf.pop_back();
            entry.valueType = "STRING";
            entry.stringValue = std::move(buf);
        }
    }

    RegCloseKey(hKey);
    return entry;
}

Result WriteRegistryOperation(const TweakOperation& op) {
    HKEY root = ResolveRootHive(op.hive);
    if (!root) {
        return Result::Fail(StatusCode::InvalidArgument, "Hive no soportado: " + op.hive);
    }

    HKEY hKey = nullptr;
    LSTATUS openRes = RegCreateKeyExA(
        root, op.subkey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE | KEY_READ, nullptr, &hKey, nullptr);
    if (openRes != ERROR_SUCCESS) {
        return Result::Fail(
            openRes == ERROR_ACCESS_DENIED ? StatusCode::AccessDenied : StatusCode::SystemApiError,
            "RegCreateKeyExA fallo en " + op.hive + "\\" + op.subkey + " (err=" + std::to_string(openRes) + ")");
    }

    LSTATUS setRes = ERROR_SUCCESS;
    if (op.type == "DWORD") {
        DWORD val = static_cast<DWORD>(op.targetDword);
        setRes = RegSetValueExA(hKey, op.name.c_str(), 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&val), sizeof(val));
    } else {
        setRes = RegSetValueExA(hKey, op.name.c_str(), 0, REG_SZ,
            reinterpret_cast<const BYTE*>(op.targetString.c_str()),
            static_cast<DWORD>(op.targetString.size() + 1));
    }

    RegCloseKey(hKey);
    if (setRes != ERROR_SUCCESS) {
        return Result::Fail(
            setRes == ERROR_ACCESS_DENIED ? StatusCode::AccessDenied : StatusCode::SystemApiError,
            "RegSetValueExA fallo para " + op.name + " (err=" + std::to_string(setRes) + ")");
    }
    return Result::Success();
}

Result RestoreSingleBackupEntry(const RegistryBackupEntry& entry) {
    HKEY root = ResolveRootHive(entry.hive);
    if (!root) {
        return Result::Fail(StatusCode::InvalidArgument, "Hive invalido en rollback: " + entry.hive);
    }

    if (!entry.existed) {
        HKEY hKey = nullptr;
        if (RegOpenKeyExA(root, entry.subkey.c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegDeleteValueA(hKey, entry.name.c_str());
            RegCloseKey(hKey);
        }
        return Result::Success();
    }

    TweakOperation op;
    op.hive = entry.hive;
    op.subkey = entry.subkey;
    op.name = entry.name;
    op.type = entry.valueType;
    op.targetDword = entry.dwordValue;
    op.targetString = entry.stringValue;
    return WriteRegistryOperation(op);
}
#endif

} // namespace

Result TweakEngine::LoadCatalog(const std::string& packsJsoncPath) {
    JsonValue root;
    Result r = JsonValue::ParseFile(packsJsoncPath, root);
    if (!r.ok) return r;

    const auto* packsNode = root.Get("tweakPacks");
    if (!packsNode || !packsNode->IsArray()) {
        return Result::Fail(StatusCode::ParseError, "packs.jsonc no contiene arreglo 'tweakPacks'");
    }

    packs_.clear();
    for (const auto& pNode : packsNode->AsArray()) {
        if (!pNode.IsObject()) continue;
        TweakPack pack;
        if (const auto* v = pNode.Get("id"))          pack.id = v->AsString();
        if (const auto* v = pNode.Get("category"))    pack.category = v->AsString();
        if (const auto* v = pNode.Get("description")) pack.description = v->AsString();
        if (const auto* v = pNode.Get("enabled"))     pack.enabled = v->AsBool(false);
        if (const auto* v = pNode.Get("reversible"))  pack.reversible = v->AsBool(true);
        if (const auto* v = pNode.Get("minPreset"))   pack.minPreset = v->AsString();

        if (const auto* regs = pNode.Get("registryChanges"); regs && regs->IsArray()) {
            for (const auto& rNode : regs->AsArray()) {
                if (!rNode.IsObject()) continue;
                TweakOperation op;
                if (const auto* v = rNode.Get("hive"))   op.hive = v->AsString();
                if (const auto* v = rNode.Get("subkey")) op.subkey = v->AsString();
                if (const auto* v = rNode.Get("name"))   op.name = v->AsString();
                if (const auto* v = rNode.Get("type"))   op.type = v->AsString("DWORD");

                if (op.type == "DWORD") {
                    if (const auto* v = rNode.Get("targetValue"))
                        op.targetDword = static_cast<uint32_t>(v->AsUInt64(0));
                    if (const auto* v = rNode.Get("defaultWindowsValue"))
                        op.defaultDword = static_cast<uint32_t>(v->AsUInt64(0));
                } else {
                    if (const auto* v = rNode.Get("targetValue"))
                        op.targetString = v->AsString();
                    if (const auto* v = rNode.Get("defaultWindowsValue"))
                        op.defaultString = v->AsString();
                }
                pack.registryChanges.push_back(std::move(op));
            }
        }
        if (!pack.id.empty()) {
            packs_.push_back(std::move(pack));
        }
    }

    return Result::Success("Catalogo cargado: " + std::to_string(packs_.size()) + " packs");
}

Result TweakEngine::LoadPreset(const std::string& presetJsoncPath, PresetConfig& outPreset) const {
    JsonValue root;
    Result r = JsonValue::ParseFile(presetJsoncPath, root);
    if (!r.ok) return r;

    if (!root.IsObject()) {
        return Result::Fail(StatusCode::ParseError, "Preset invalido: raiz no es objeto");
    }

    outPreset = PresetConfig{};
    if (const auto* v = root.Get("presetId"))           outPreset.presetId = v->AsString();
    if (const auto* v = root.Get("displayName"))        outPreset.displayName = v->AsString();
    if (const auto* v = root.Get("description"))        outPreset.description = v->AsString();
    if (const auto* v = root.Get("createRestorePoint")) outPreset.createRestorePoint = v->AsBool(true);

    if (const auto* ids = root.Get("enabledPackIds"); ids && ids->IsArray()) {
        for (const auto& item : ids->AsArray()) {
            if (item.IsString()) {
                outPreset.enabledPackIds.push_back(item.AsString());
            }
        }
    }

    if (outPreset.presetId.empty()) {
        return Result::Fail(StatusCode::ParseError, "Preset sin 'presetId'");
    }
    return Result::Success();
}

const TweakPack* TweakEngine::FindPack(const std::string& id) const {
    for (const auto& p : packs_) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

std::vector<TweakOperation> TweakEngine::ExpandWildcardOperations(const TweakOperation& op) const {
    const std::string marker = "\\*";
    if (op.subkey.size() < marker.size() ||
        op.subkey.compare(op.subkey.size() - marker.size(), marker.size(), marker) != 0) {
        return {op};
    }

#ifdef _WIN32
    std::string baseSubkey = op.subkey.substr(0, op.subkey.size() - marker.size());
    HKEY root = ResolveRootHive(op.hive);
    HKEY hBase = nullptr;
    if (!root || RegOpenKeyExA(root, baseSubkey.c_str(), 0, KEY_ENUMERATE_SUB_KEYS, &hBase) != ERROR_SUCCESS) {
        return {};
    }

    std::vector<TweakOperation> expanded;
    char childName[256];
    DWORD index = 0;
    while (true) {
        DWORD childLen = sizeof(childName);
        LSTATUS res = RegEnumKeyExA(hBase, index++, childName, &childLen, nullptr, nullptr, nullptr, nullptr);
        if (res == ERROR_NO_MORE_ITEMS) break;
        if (res == ERROR_SUCCESS) {
            TweakOperation clone = op;
            clone.subkey = baseSubkey + "\\" + std::string(childName, childLen);
            expanded.push_back(std::move(clone));
        }
    }
    RegCloseKey(hBase);
    return expanded;
#else
    return {op};
#endif
}

Result TweakEngine::ApplyPreset(
    const PresetConfig& preset,
    const std::string& backupFilePath,
    bool dryRun,
    std::vector<RegistryBackupEntry>* outCaptured) const {

    if (policy_.autoRestorePoint && preset.createRestorePoint) {
        Result rp = restoreManager_.CreateSystemRestorePoint("Win-Opty Preset " + preset.presetId, dryRun);
        if (!rp.ok && !dryRun) {
            // Advertencia no bloqueante si la proteccion del sistema esta desactivada,
            // pero el backup JSON transaccional sigue siendo obligatorio.
        }
    }

    std::vector<TweakOperation> plannedOps;
    for (const auto& packId : preset.enabledPackIds) {
        const TweakPack* pack = FindPack(packId);
        if (!pack) {
            return Result::Fail(StatusCode::InvalidArgument, "Pack no encontrado en catalogo: " + packId);
        }
        for (const auto& op : pack->registryChanges) {
            auto expanded = ExpandWildcardOperations(op);
            plannedOps.insert(plannedOps.end(), expanded.begin(), expanded.end());
        }
    }

    std::vector<RegistryBackupEntry> captured;
    captured.reserve(plannedOps.size());

#ifdef _WIN32
    for (const auto& op : plannedOps) {
        captured.push_back(ReadRegistryEntry(op));
    }
#else
    for (const auto& op : plannedOps) {
        RegistryBackupEntry stub;
        stub.hive = op.hive;
        stub.subkey = op.subkey;
        stub.name = op.name;
        stub.valueType = op.type;
        captured.push_back(std::move(stub));
    }
#endif

    if (outCaptured) {
        *outCaptured = captured;
    }

    if (policy_.transactionalTweaks && !dryRun && !backupFilePath.empty()) {
        Result br = restoreManager_.SaveStateBackup(backupFilePath, preset.presetId, captured);
        if (!br.ok) return br;
    }

    if (dryRun) {
        return Result::Success("[DRY-RUN] Preset '" + preset.presetId + "' verificado (" +
            std::to_string(plannedOps.size()) + " operaciones planificadas)");
    }

#ifdef _WIN32
    for (const auto& op : plannedOps) {
        Result wr = WriteRegistryOperation(op);
        if (!wr.ok) return wr;
    }
    return Result::Success("Preset '" + preset.presetId + "' aplicado (" +
        std::to_string(plannedOps.size()) + " valores)");
#else
    return Result::Fail(StatusCode::NotImplemented, "Aplicacion de registro solo disponible en Windows");
#endif
}

Result TweakEngine::RollbackFromBackup(const std::string& backupFilePath, bool dryRun) const {
    std::string presetName;
    std::vector<RegistryBackupEntry> entries;
    Result lr = restoreManager_.LoadStateBackup(backupFilePath, presetName, entries);
    if (!lr.ok) return lr;

    if (dryRun) {
        return Result::Success("[DRY-RUN] Rollback verificaria " + std::to_string(entries.size()) +
            " entradas del preset '" + presetName + "'");
    }

#ifdef _WIN32
    for (const auto& entry : entries) {
        Result rr = RestoreSingleBackupEntry(entry);
        if (!rr.ok) return rr;
    }
    return Result::Success("Rollback completado para " + std::to_string(entries.size()) + " entradas");
#else
    return Result::Fail(StatusCode::NotImplemented, "Rollback de registro solo disponible en Windows");
#endif
}

std::vector<TweakDriftItem> TweakEngine::DetectRevertedTweaks(const PresetConfig& preset) const {
    std::vector<TweakDriftItem> drift;
#ifdef _WIN32
    std::unordered_set<std::string> enabled(preset.enabledPackIds.begin(), preset.enabledPackIds.end());
    for (const auto& pack : packs_) {
        if (enabled.find(pack.id) == enabled.end()) continue;
        for (const auto& rawOp : pack.registryChanges) {
            for (const auto& op : ExpandWildcardOperations(rawOp)) {
                RegistryBackupEntry current = ReadRegistryEntry(op);
                if (op.type == "DWORD") {
                    if (!current.existed || current.dwordValue != op.targetDword) {
                        drift.push_back(TweakDriftItem{
                            pack.id, op.hive, op.subkey, op.name,
                            std::to_string(op.targetDword),
                            current.existed ? std::to_string(current.dwordValue) : "<no existe>"
                        });
                    }
                } else {
                    if (!current.existed || current.stringValue != op.targetString) {
                        drift.push_back(TweakDriftItem{
                            pack.id, op.hive, op.subkey, op.name,
                            op.targetString,
                            current.existed ? current.stringValue : "<no existe>"
                        });
                    }
                }
            }
        }
    }
#else
    (void)preset;
#endif
    return drift;
}

} // namespace WinOpty::Core
