# ============================================================================
# Win-Opty — Suite Automatizada de Validacion (JSONC, PowerShell, Rollback)
# ============================================================================
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$failures = 0

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )
    if ($Condition) {
        Write-Host "[PASS] $Message" -ForegroundColor Green
    } else {
        Write-Host "[FAIL] $Message" -ForegroundColor Red
        $script:failures++
    }
}

function Strip-JsonCommentsSafe {
    param([string]$InputText)
    $sb = New-Object System.Text.StringBuilder
    $inString = $false
    $inLineComment = $false
    $inBlockComment = $false

    for ($i = 0; $i -lt $InputText.Length; $i++) {
        $c = $InputText[$i]
        $next = if ($i + 1 -lt $InputText.Length) { $InputText[$i + 1] } else { [char]0 }

        if ($inLineComment) {
            if ($c -eq "`n") {
                $inLineComment = $false
                [void]$sb.Append($c)
            }
            continue
        }

        if ($inBlockComment) {
            if ($c -eq '*' -and $next -eq '/') {
                $inBlockComment = $false
                $i++
            } elseif ($c -eq "`n") {
                [void]$sb.Append("`n")
            }
            continue
        }

        if ($inString) {
            [void]$sb.Append($c)
            if ($c -eq '\' -and ($i + 1) -lt $InputText.Length) {
                $i++
                [void]$sb.Append($InputText[$i])
            } elseif ($c -eq '"') {
                $inString = $false
            }
            continue
        }

        if ($c -eq '"') {
            $inString = $true
            [void]$sb.Append($c)
        } elseif ($c -eq '/' -and $next -eq '/') {
            $inLineComment = $true
            $i++
        } elseif ($c -eq '/' -and $next -eq '*') {
            $inBlockComment = $true
            $i++
        } else {
            [void]$sb.Append($c)
        }
    }
    return $sb.ToString()
}

Write-Host "=== 1. Validando archivos JSONC en src/presets ===" -ForegroundColor Cyan
$jsoncFiles = Get-ChildItem -Path (Join-Path $repoRoot "src\presets") -Filter "*.jsonc" -Recurse
Assert-True ($jsoncFiles.Count -ge 7) "Se encontraron $($jsoncFiles.Count) archivos .jsonc en src/presets"

foreach ($file in $jsoncFiles) {
    try {
        $raw = Get-Content -LiteralPath $file.FullName -Raw -Encoding UTF8
        $clean = Strip-JsonCommentsSafe -InputText $raw
        $parsed = $clean | ConvertFrom-Json
        Assert-True ($null -ne $parsed) "JSONC valido: $($file.FullName.Substring($repoRoot.Length + 1))"
    }
    catch {
        Assert-True $false "JSONC invalido en $($file.Name): $($_.Exception.Message)"
    }
}

# Verificar esquema clave de packs y presets
$packsObj = (Strip-JsonCommentsSafe (Get-Content -LiteralPath (Join-Path $repoRoot "src\presets\packs.jsonc") -Raw -Encoding UTF8)) | ConvertFrom-Json
Assert-True ($packsObj.tweakPacks.Count -ge 5) "packs.jsonc define $($packsObj.tweakPacks.Count) packs"
Assert-True ($packsObj.safetyRestore.createWindowsRestorePoint -eq $true) "packs.jsonc exige restore point por defecto"

foreach ($presetName in @("safe", "medium", "extreme")) {
    $pObj = (Strip-JsonCommentsSafe (Get-Content -LiteralPath (Join-Path $repoRoot "src\presets\$presetName.jsonc") -Raw -Encoding UTF8)) | ConvertFrom-Json
    Assert-True (-not [string]::IsNullOrWhiteSpace($pObj.presetId)) "Preset $presetName.jsonc contiene presetId=$($pObj.presetId)"
}

Write-Host "`n=== 2. Validando sintaxis AST de scripts PowerShell ===" -ForegroundColor Cyan
$psFiles = Get-ChildItem -Path $repoRoot -Filter "*.ps1" -Recurse
foreach ($psFile in $psFiles) {
    $tokens = $null
    $parseErrors = $null
    [void][System.Management.Automation.Language.Parser]::ParseFile($psFile.FullName, [ref]$tokens, [ref]$parseErrors)
    Assert-True ($parseErrors.Count -eq 0) "Sintaxis PowerShell limpia: $($psFile.FullName.Substring($repoRoot.Length + 1))"
}

Write-Host "`n=== 3. Ejecutando pruebas funcionales en modo lectura / -DryRun ===" -ForegroundColor Cyan
$verifyScript = Join-Path $repoRoot "scripts\windows\verify_state.ps1"
$applyScript  = Join-Path $repoRoot "scripts\windows\apply_extreme.ps1"
$rollbackScript = Join-Path $repoRoot "scripts\windows\rollback_tweaks.ps1"

$verifyOut = & $verifyScript | Out-String
Assert-True (-not [string]::IsNullOrWhiteSpace($verifyOut) -and $verifyOut -match "Win32PrioritySeparation") "verify_state.ps1 ejecuto correctamente (solo lectura)"

$applyOut = (& $applyScript -DryRun *>&1) | Out-String
Assert-True ($applyOut -match "\[RESUMEN\]" -and $applyOut -match "DryRun=True") "apply_extreme.ps1 -DryRun completo sin modificar el sistema"

# Probar rollback_tweaks.ps1 -DryRun contra un respaldo sintetico temporal
$tempBackup = Join-Path $env:TEMP "winopty_test_rollback.json"
@{
    schemaVersion = "1.0.0"
    preset = "EXTREMO"
    entries = @(
        @{ path = "HKLM:\SOFTWARE\Test"; name = "ValA"; existed = $true;  kind = "DWord"; value = 20 },
        @{ path = "HKLM:\SOFTWARE\Test"; name = "ValB"; existed = $false; kind = "DWord"; value = $null }
    )
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $tempBackup -Encoding UTF8

$rbOut = (& $rollbackScript -BackupPath $tempBackup -DryRun *>&1) | Out-String
Remove-Item -LiteralPath $tempBackup -Force -ErrorAction SilentlyContinue
Assert-True ($rbOut -match "Restauraria" -and $rbOut -match "Eliminaria" -and $rbOut -match "2 entradas procesadas") "rollback_tweaks.ps1 -DryRun valida restauracion y eliminacion"

Write-Host "`n=== 4. Verificando existencia de archivos declarados en CMakeLists.txt ===" -ForegroundColor Cyan
$expectedCppFiles = @(
    "src\main.cpp",
    "src\core\engine_config.hpp",
    "src\core\module_interfaces.hpp",
    "src\core\jsonc_parser.hpp",
    "src\core\restore_point.hpp",
    "src\core\restore_point.cpp",
    "src\core\tweak_engine.hpp",
    "src\core\tweak_engine.cpp",
    "src\core\memory_cleaner.cpp",
    "src\core\cache_cleaner.cpp",
    "src\core\hardware_monitor.cpp",
    "src\core\priority_launcher.cpp",
    "src\core\game_mode.cpp",
    "src\core\overlay_hud.cpp",
    "src\ui\tray_controller.hpp",
    "src\ui\tray_controller.cpp",
    "src\ui\main_window.hpp",
    "src\ui\main_window.cpp",
    "tests\test_core_part_a.cpp"
)

foreach ($rel in $expectedCppFiles) {
    $full = Join-Path $repoRoot $rel
    Assert-True (Test-Path -LiteralPath $full) "Archivo presente: $rel"
}

Write-Host "`n=== 5. Validando scripts Multi-OS (Linux/macOS) con Bash ===" -ForegroundColor Cyan
$bashExe = "C:\Program Files\Git\bin\bash.exe"
if (Test-Path -LiteralPath $bashExe) {
    $shFiles = Get-ChildItem -Path (Join-Path $repoRoot "scripts") -Filter "*.sh" -Recurse
    Assert-True ($shFiles.Count -ge 6) "Se encontraron $($shFiles.Count) scripts .sh"
    foreach ($sh in $shFiles) {
        $unixPath = $sh.FullName.Replace('\', '/')
        & $bashExe -n $unixPath
        Assert-True ($LASTEXITCODE -eq 0) "Sintaxis Bash limpia: $($sh.FullName.Substring($repoRoot.Length + 1))"
    }

    $linuxDry = (& $bashExe ((Join-Path $repoRoot "scripts\linux\apply_linux.sh").Replace('\', '/')) --dry-run *>&1) | Out-String
    Assert-True ($LASTEXITCODE -eq 0 -and $linuxDry -match "\[DRY-RUN\]") "apply_linux.sh --dry-run ok"

    $macDry = (& $bashExe ((Join-Path $repoRoot "scripts\macos\apply_macos.sh").Replace('\', '/')) --dry-run *>&1) | Out-String
    Assert-True ($LASTEXITCODE -eq 0 -and $macDry -match "\[DRY-RUN\]") "apply_macos.sh --dry-run ok"
} else {
    Write-Host "[WARN] Git Bash no encontrado; se omite bash -n" -ForegroundColor Yellow
}

Write-Host "`nResultado final: $failures fallos." -ForegroundColor $(if ($failures -eq 0) { "Green" } else { "Red" })
if ($failures -gt 0) {
    exit 1
}
exit 0
