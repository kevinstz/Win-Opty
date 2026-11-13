#!/usr/bin/env bash
# ============================================================================
# Win-Opty — Aplicador / Rollback de perfil Linux (sysctl + cpufreq)
# ============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
CONF_FILE="$SCRIPT_DIR/99-winopty.conf"
BACKUP_FILE="$REPO_ROOT/backups/linux_sysctl_state.conf"

DRY_RUN=false
ROLLBACK=false
SET_GOVERNOR=false

for arg in "$@"; do
    case "$arg" in
        --dry-run)      DRY_RUN=true ;;
        --rollback)     ROLLBACK=true ;;
        --performance)  SET_GOVERNOR=true ;;
        -h|--help)
            echo "Uso: $0 [--dry-run] [--performance] [--rollback]"
            exit 0
            ;;
        *)
            echo "Argumento desconocido: $arg" >&2
            exit 1
            ;;
    esac
done

require_root() {
    if [[ "$DRY_RUN" == "false" && "${EUID:-$(id -u)}" -ne 0 ]]; then
        echo "Error: ejecuta con sudo o usa --dry-run." >&2
        exit 1
    fi
}

backup_sysctl_keys() {
    mkdir -p "$(dirname "$BACKUP_FILE")"
    : > "$BACKUP_FILE"
    while IFS='=' read -r raw_key _; do
        key="$(echo "$raw_key" | xargs || true)"
        [[ -z "$key" || "$key" == \#* ]] && continue
        if sysctl -n "$key" >/dev/null 2>&1; then
            val="$(sysctl -n "$key" | tr '\t' ' ')"
            echo "$key = $val" >> "$BACKUP_FILE"
        fi
    done < "$CONF_FILE"
    echo "[OK] Respaldo sysctl guardado en: $BACKUP_FILE"
}

apply_conf_file() {
    local target="$1"
    while IFS='=' read -r raw_key raw_val; do
        key="$(echo "$raw_key" | xargs || true)"
        val="$(echo "${raw_val:-}" | xargs || true)"
        [[ -z "$key" || "$key" == \#* ]] && continue
        if [[ "$DRY_RUN" == "true" ]]; then
            echo "[DRY-RUN] sysctl -w $key=$val"
        else
            sysctl -w "$key=$val"
        fi
    done < "$target"
}

if [[ "$ROLLBACK" == "true" ]]; then
    require_root
    if [[ ! -f "$BACKUP_FILE" ]]; then
        echo "Error: no existe respaldo en $BACKUP_FILE" >&2
        exit 1
    fi
    apply_conf_file "$BACKUP_FILE"
    echo "[OK] Rollback Linux completado."
    exit 0
fi

require_root
if [[ "$DRY_RUN" == "true" ]]; then
    echo "[DRY-RUN] Guardaria respaldo en $BACKUP_FILE"
else
    backup_sysctl_keys
fi

apply_conf_file "$CONF_FILE"

if [[ "$SET_GOVERNOR" == "true" ]]; then
    for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
        [[ -e "$cpu" ]] || continue
        if [[ "$DRY_RUN" == "true" ]]; then
            echo "[DRY-RUN] echo performance > $cpu"
        else
            echo "performance" > "$cpu"
        fi
    done
fi
