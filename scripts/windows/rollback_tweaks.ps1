# ============================================================================
# Win-Opty — Rollback transaccional desde respaldo JSON
# ============================================================================
[CmdletBinding()]
param(
    [string]$BackupPath = "",
    [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($BackupPath)) {
    $repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
    $BackupPath = Join-Path $repoRoot "backups\win_registry_state.json"
}

. (Join-Path $PSScriptRoot "common_backup.ps1")

if (-not (Test-Path -LiteralPath $BackupPath)) {
    throw "No se encontro el archivo de respaldo: $BackupPath"
}

if (-not $DryRun -and -not (Test-IsAdministrator)) {
    throw "Este script requiere ejecutarse como Administrador cuando no se usa -DryRun."
}

$backup = Get-Content -LiteralPath $BackupPath -Raw -Encoding UTF8 | ConvertFrom-Json
$restoredCount = 0

foreach ($entry in $backup.entries) {
    if (-not $entry.existed) {
        if ($DryRun) {
            Write-Host "[DRY-RUN] Eliminaria valor creado por Win-Opty: $($entry.path) -> $($entry.name)" -ForegroundColor Yellow
        }
        else {
            if (Test-Path -LiteralPath $entry.path) {
                Remove-ItemProperty -LiteralPath $entry.path -Name $entry.name -ErrorAction SilentlyContinue
            }
            Write-Host "[ROLLBACK] Eliminado: $($entry.path) -> $($entry.name)" -ForegroundColor Green
        }
    }
    else {
        $psType = if ($entry.kind -eq "DWord") { "DWord" } else { "String" }
        if ($DryRun) {
            Write-Host "[DRY-RUN] Restauraria $($entry.path) -> $($entry.name) = $($entry.value) [$psType]" -ForegroundColor Cyan
        }
        else {
            if (-not (Test-Path -LiteralPath $entry.path)) {
                New-Item -Path $entry.path -Force | Out-Null
            }
            $val = if ($psType -eq "DWord") {
                Convert-ToDWordSignedInt -Value ([uint64]$entry.value)
            } else {
                [string]$entry.value
            }
            Set-ItemProperty -LiteralPath $entry.path -Name $entry.name -Value $val -Type $psType -Force
            Write-Host "[ROLLBACK] Restaurado: $($entry.path) -> $($entry.name) = $($entry.value)" -ForegroundColor Green
        }
    }
    $restoredCount++
}

Write-Host "`n[RESUMEN ROLLBACK] $restoredCount entradas procesadas (DryRun=$DryRun)." -ForegroundColor Cyan
