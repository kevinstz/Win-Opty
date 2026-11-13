# ============================================================================
# Win-Opty — Utilidades compartidas de respaldo y cambios transaccionales
# ============================================================================

Set-StrictMode -Version Latest

function Test-IsAdministrator {
    $identity  = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Convert-ToDWordSignedInt {
    param([Parameter(Mandatory)][uint64]$Value)
    $u32 = [uint32]($Value -band 0xFFFFFFFF)
    return [BitConverter]::ToInt32([BitConverter]::GetBytes($u32), 0)
}

function Get-WinOptyRegistrySnapshot {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Name
    )

    $key = Get-Item -LiteralPath $Path -ErrorAction SilentlyContinue
    if ($null -eq $key) {
        return [pscustomobject]@{
            path    = $Path
            name    = $Name
            existed = $false
            kind    = $null
            value   = $null
        }
    }

    $valueNames = $key.GetValueNames()
    if ($valueNames -notcontains $Name) {
        return [pscustomobject]@{
            path    = $Path
            name    = $Name
            existed = $false
            kind    = $null
            value   = $null
        }
    }

    $kind = $key.GetValueKind($Name).ToString()
    $val  = $key.GetValue($Name, $null, [Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames)

    return [pscustomobject]@{
        path    = $Path
        name    = $Name
        existed = $true
        kind    = $kind
        value   = $val
    }
}

function New-WinOptyRestorePoint {
    param(
        [string]$Description = "Win-Opty Pre-Optimization",
        [switch]$DryRun
    )

    if ($DryRun) {
        Write-Host "[DRY-RUN] Crearia punto de restauracion: '$Description'" -ForegroundColor Yellow
        return $true
    }

    if (-not (Test-IsAdministrator)) {
        Write-Warning "Se requieren permisos de Administrador para ejecutar Checkpoint-Computer."
        return $false
    }

    try {
        Checkpoint-Computer -Description $Description -RestorePointType "MODIFY_SETTINGS" -ErrorAction Stop
        Write-Host "[OK] Punto de restauracion creado: '$Description'" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Warning "No se pudo crear el punto de restauracion (puede estar deshabilitado o en cooldown de 24h): $($_.Exception.Message)"
        return $false
    }
}

function Set-RegistryValueTransactional {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)]$Value,
        [Parameter(Mandatory)][ValidateSet("DWord", "String")][string]$Type,
        [Parameter(Mandatory)][System.Collections.IList]$BackupList,
        [switch]$DryRun
    )

    $snapshot = Get-WinOptyRegistrySnapshot -Path $Path -Name $Name
    $BackupList.Add($snapshot) | Out-Null

    if ($DryRun) {
        $prevText = if ($snapshot.existed) { "$($snapshot.value) ($($snapshot.kind))" } else { "<NO EXISTIA>" }
        Write-Host "[DRY-RUN] $Path -> $Name = $Value [$Type] (anterior: $prevText)" -ForegroundColor Cyan
        return
    }

    if (-not (Test-Path -LiteralPath $Path)) {
        New-Item -Path $Path -Force | Out-Null
    }

    $writeValue = if ($Type -eq "DWord") {
        Convert-ToDWordSignedInt -Value ([uint64]$Value)
    } else {
        [string]$Value
    }

    Set-ItemProperty -LiteralPath $Path -Name $Name -Value $writeValue -Type $Type -Force
    Write-Host "[OK] $Path -> $Name = $Value [$Type]" -ForegroundColor Green
}

function Save-WinOptyBackupFile {
    param(
        [Parameter(Mandatory)][string]$BackupPath,
        [Parameter(Mandatory)][string]$PresetName,
        [Parameter(Mandatory)][System.Collections.IList]$Entries,
        [switch]$DryRun
    )

    $payload = [pscustomobject]@{
        schemaVersion = "1.0.0"
        preset        = $PresetName
        createdAtUtc  = (Get-Date).ToUniversalTime().ToString("o")
        entries       = @($Entries)
    }

    if ($DryRun) {
        Write-Host "[DRY-RUN] Guardaria respaldo JSON con $($Entries.Count) entradas en: $BackupPath" -ForegroundColor Yellow
        return
    }

    $dir = Split-Path -Parent $BackupPath
    if ($dir -and -not (Test-Path -LiteralPath $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }

    $payload | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $BackupPath -Encoding UTF8
    Write-Host "[OK] Respaldo de estado guardado en: $BackupPath" -ForegroundColor Green
}
