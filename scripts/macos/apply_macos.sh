#!/usr/bin/env bash
# ============================================================================
# Win-Opty — Ajustes de baja latencia de red para macOS (Darwin)
# ============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BACKUP_FILE="$REPO_ROOT/backups/macos_sysctl_state.conf"

DRY_RUN=false
ROLLBACK=false

for arg in "$@"; do
    case "$arg" in
        --dry-run)  DRY_RUN=true ;;
        --rollback) ROLLBACK=true ;;
        -h|--help)
            echo "Uso: $0 [--dry-run] [--rollback]"
            exit 0
            ;;
    esac
done

KEYS=("net.inet.tcp.delayed_ack" "net.inet.tcp.mssdflt")

if [[ "$ROLLBACK" == "true" ]]; then
    if [[ ! -f "$BACKUP_FILE" ]]; then
        echo "Error: no existe respaldo en $BACKUP_FILE" >&2
        exit 1
    fi
    while IFS='=' read -r k v; do
        [[ -z "$k" ]] && continue
        if [[ "$DRY_RUN" == "true" ]]; then
            echo "[DRY-RUN] sudo sysctl -w $k=$v"
        else
            sudo sysctl -w "$k=$v"
        fi
    done < "$BACKUP_FILE"
    exit 0
fi

if [[ "$DRY_RUN" == "true" ]]; then
    echo "[DRY-RUN] Guardaria valores previos de ${KEYS[*]} en $BACKUP_FILE"
    echo "[DRY-RUN] sudo sysctl -w net.inet.tcp.delayed_ack=0"
    echo "[DRY-RUN] sudo sysctl -w net.inet.tcp.mssdflt=1460"
    exit 0
fi

mkdir -p "$(dirname "$BACKUP_FILE")"
: > "$BACKUP_FILE"
for k in "${KEYS[@]}"; do
    prev="$(sysctl -n "$k" 2>/dev/null || true)"
    [[ -n "$prev" ]] && echo "$k=$prev" >> "$BACKUP_FILE"
done

sudo sysctl -w net.inet.tcp.delayed_ack=0
sudo sysctl -w net.inet.tcp.mssdflt=1460
echo "[OK] Perfil Darwin aplicado (respaldo en $BACKUP_FILE)."
