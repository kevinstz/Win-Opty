# ============================================================================
# Win-Opty — Verificador de estado actual del registro (Solo Lectura)
# ============================================================================
[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common_backup.ps1")

$mmcssPath = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile"
$gamesTask = "$mmcssPath\Tasks\Games"

$checks = @(
    @{ Label = "MMCSS SystemResponsiveness";   Path = $mmcssPath; Name = "SystemResponsiveness";   Target = 10 },
    @{ Label = "MMCSS NetworkThrottlingIndex"; Path = $mmcssPath; Name = "NetworkThrottlingIndex"; Target = -1 },
    @{ Label = "Games GPU Priority";           Path = $gamesTask; Name = "GPU Priority";           Target = 8 },
    @{ Label = "Games Priority";               Path = $gamesTask; Name = "Priority";               Target = 6 },
    @{ Label = "Games Scheduling Category";    Path = $gamesTask; Name = "Scheduling Category";    Target = "High" },
    @{ Label = "Games SFIO Priority";          Path = $gamesTask; Name = "SFIO Priority";          Target = "High" },
    @{ Label = "HAGS HwSchMode";               Path = "HKLM:\SYSTEM\CurrentControlSet\Control\GraphicsDrivers"; Name = "HwSchMode"; Target = 2 },
    @{ Label = "GameDVR_Enabled";              Path = "HKCU:\System\GameConfigStore"; Name = "GameDVR_Enabled"; Target = 0 },
    @{ Label = "AppCaptureEnabled";            Path = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\GameDVR"; Name = "AppCaptureEnabled"; Target = 0 },
    @{ Label = "Win32PrioritySeparation";      Path = "HKLM:\SYSTEM\CurrentControlSet\Control\PriorityControl"; Name = "Win32PrioritySeparation"; Target = 38 }
)

$results = foreach ($c in $checks) {
    $snap = Get-WinOptyRegistrySnapshot -Path $c.Path -Name $c.Name
    $currentDisplay = if ($snap.existed) { "$($snap.value)" } else { "<No configurado>" }
    $matchesExtreme = $snap.existed -and ("$($snap.value)" -eq "$($c.Target)")
    [pscustomobject]@{
        Ajuste         = $c.Label
        ValorActual    = $currentDisplay
        ObjetivoExtremo = $c.Target
        Coincide       = $matchesExtreme
    }
}

$results | Format-Table -AutoSize
