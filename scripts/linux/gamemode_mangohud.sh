#!/usr/bin/env bash
# ============================================================================
# Win-Opty — Wrapper de lanzamiento con Feral GameMode y MangoHud
# ============================================================================
set -euo pipefail

DRY_RUN=false
if [[ "${1:-}" == "--dry-run" ]]; then
    DRY_RUN=true
    shift
fi

if [[ "$#" -eq 0 ]]; then
    echo "Uso: $0 [--dry-run] <comando_del_juego> [args...]" >&2
    exit 1
fi

PREFIX=()
if command -v gamemoderun >/dev/null 2>&1; then
    PREFIX+=(gamemoderun)
fi
if command -v mangohud >/dev/null 2>&1; then
    PREFIX+=(mangohud)
fi

if [[ "$DRY_RUN" == "true" ]]; then
    echo "[DRY-RUN] Ejecutaria: ${PREFIX[*]} $*"
    exit 0
fi

exec "${PREFIX[@]}" "$@"
