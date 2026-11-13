#!/usr/bin/env bash
# ============================================================================
# Win-Opty — Purga de memoria inactiva y cache de shaders Metal en macOS
# ============================================================================
set -euo pipefail

DRY_RUN=false
if [[ "${1:-}" == "--dry-run" ]]; then
    DRY_RUN=true
fi

METAL_CACHE_DIR="${HOME}/Library/Caches/com.apple.metal"

if [[ "$DRY_RUN" == "true" ]]; then
    echo "[DRY-RUN] sudo purge"
    echo "[DRY-RUN] rm -rf ${METAL_CACHE_DIR}/*"
    exit 0
fi

sudo purge
if [[ -d "$METAL_CACHE_DIR" ]]; then
    rm -rf "${METAL_CACHE_DIR:?}/"*
fi
echo "[OK] Memoria purgada y cache Metal limpiada."
