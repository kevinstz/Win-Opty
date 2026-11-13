#!/usr/bin/env bash
# ============================================================================
# Win-Opty — Priorizacion de proceso en P-Cores (Apple Silicon / Darwin)
# ============================================================================
set -euo pipefail

DRY_RUN=false
if [[ "${1:-}" == "--dry-run" ]]; then
    DRY_RUN=true
    shift
fi

if [[ "$#" -lt 1 ]]; then
    echo "Uso: $0 [--dry-run] <PID | comando...>" >&2
    exit 1
fi

if [[ "$1" =~ ^[0-9]+$ ]]; then
    if [[ "$DRY_RUN" == "true" ]]; then
        echo "[DRY-RUN] taskpolicy -b -p $1"
    else
        taskpolicy -b -p "$1"
    fi
    exit 0
fi

if [[ "$DRY_RUN" == "true" ]]; then
    echo "[DRY-RUN] taskpolicy -B $*"
    exit 0
fi

exec taskpolicy -B "$@"
