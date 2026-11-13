#!/usr/bin/env bash
# ============================================================================
# Win-Opty — Limpieza segura de PageCache en Linux (equivalente Mem Reduct)
# ============================================================================
set -euo pipefail

DRY_RUN=false
LEVEL=1 # 1 = PageCache (mas seguro, sin tocar inodos/dentries en caliente)

for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=true ;;
        --all)     LEVEL=3 ;;
        -h|--help)
            echo "Uso: $0 [--dry-run] [--all]"
            exit 0
            ;;
    esac
done

if [[ "$DRY_RUN" == "true" ]]; then
    echo "[DRY-RUN] sync && echo $LEVEL > /proc/sys/vm/drop_caches"
    exit 0
fi

if [[ "${EUID:-$(id -u)}" -ne 0 ]]; then
    echo "Error: se requiere root (sudo) para escribir en /proc/sys/vm/drop_caches." >&2
    exit 1
fi

sync
echo "$LEVEL" > /proc/sys/vm/drop_caches
echo "[OK] PageCache liberada (nivel $LEVEL)."
