# ============================================================================
# Win-Opty — Perfil de Optimizacion Muy Alta para Windows 10/11 (Auditable)
# ============================================================================
[CmdletBinding()]
param(
    [string]$BackupPath = "",
    [switch]$SkipRestorePoint,
    [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($BackupPath)) {
    $repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
    $BackupPath = Join-Path $repoRoot "backups\win_registry_state.json"
}

. (Join-Path $PSScriptRoot "common_backup.ps1")

if (-not $DryRun -and -not (Test-IsAdministrator)) {
    throw "Este script requiere ejecutarse como Administrador cuando no se usa -DryRun."
}

$backupEntries = New-Object System.Collections.ArrayList

if (-not $SkipRestorePoint) {
    New-WinOptyRestorePoint -Description "Win-Opty Pre-Extreme Optimization" -DryRun:$DryRun | Out-Null
}

# 1. Planificador Multimedia (MMCSS) y prioridad de hilos para juegos
$mmcssPath = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile"
$gamesTask = "$mmcssPath\Tasks\Games"

$tweaks = @(
    @{ Path = $mmcssPath; Name = "SystemResponsiveness";   Value = 10;         Type = "DWord" },
    @{ Path = $mmcssPath; Name = "NetworkThrottlingIndex"; Value = 4294967295; Type = "DWord" },
    @{ Path = $gamesTask; Name = "GPU Priority";           Value = 8;          Type = "DWord" },
    @{ Path = $gamesTask; Name = "Priority";               Value = 6;          Type = "DWord" },
    @{ Path = $gamesTask; Name = "Scheduling Category";    Value = "High";     Type = "String" },
    @{ Path = $gamesTask; Name = "SFIO Priority";          Value = "High";     Type = "String" },

    # 2. Planificacion de GPU por Hardware (HAGS) y Game DVR
    @{ Path = "HKLM:\SYSTEM\CurrentControlSet\Control\GraphicsDrivers"; Name = "HwSchMode";         Value = 2; Type = "DWord" },
    @{ Path = "HKCU:\System\GameConfigStore";                           Name = "GameDVR_Enabled";   Value = 0; Type = "DWord" },
    @{ Path = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\GameDVR"; Name = "AppCaptureEnabled"; Value = 0; Type = "DWord" },

    # 3. Separacion de prioridad del procesador (Win32PrioritySeparation = 0x26 / 38)
    @{ Path = "HKLM:\SYSTEM\CurrentControlSet\Control\PriorityControl"; Name = "Win32PrioritySeparation"; Value = 38; Type = "DWord" }
)

foreach ($item in $tweaks) {
    Set-RegistryValueTransactional `
        -Path $item.Path `
        -Name $item.Name `
        -Value $item.Value `
        -Type $item.Type `
        -BackupList $backupEntries `
        -DryRun:$DryRun
}

# 4. Red para Competitivo: Nagle Off (TCPNoDelay) y ACK Inmediato (TcpAckFrequency)
$interfacesRoot = "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces"
if (Test-Path -LiteralPath $interfacesRoot) {
    Get-ChildItem -LiteralPath $interfacesRoot | ForEach-Object {
        $ifPath = $_.PSPath
        Set-RegistryValueTransactional -Path $ifPath -Name "TcpAckFrequency" -Value 1 -Type "DWord" -BackupList $backupEntries -DryRun:$DryRun
        Set-RegistryValueTransactional -Path $ifPath -Name "TCPNoDelay"      -Value 1 -Type "DWord" -BackupList $backupEntries -DryRun:$DryRun
        Set-RegistryValueTransactional -Path $ifPath -Name "TcpDelAckTicks"  -Value 0 -Type "DWord" -BackupList $backupEntries -DryRun:$DryRun
    }
}

Save-WinOptyBackupFile -BackupPath $BackupPath -PresetName "EXTREMO" -Entries $backupEntries -DryRun:$DryRun
Write-Host "`n[RESUMEN] $($backupEntries.Count) valores procesados (DryRun=$DryRun)." -ForegroundColor Cyan
