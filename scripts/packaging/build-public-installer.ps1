# Assemble the Public Installer EXE from a builds/ Bridge artifact (Story 4.1).
# Requires Inno Setup 6 (ISCC.exe). Output under builds/installer/.
#
# Usage:
#   .\scripts\packaging\build-public-installer.ps1
#   .\scripts\packaging\build-public-installer.ps1 -BridgeDir builds\release\Release
#   .\scripts\packaging\build-public-installer.ps1 -AppVersion 0.1.0

[CmdletBinding()]
param(
    [string]$BridgeDir = "",
    [string]$IsccPath = "",
    [string]$AppVersion = "0.1.0"
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Set-Location $repoRoot

function Resolve-BridgeDir
{
    param([string]$Preferred)

    if ($Preferred)
    {
        $exe = Join-Path $Preferred "Bridge.exe"
        if (-not (Test-Path -LiteralPath $exe))
        {
            throw "Bridge.exe not found under -BridgeDir '$Preferred'. Refusing silent fallback."
        }
        return (Resolve-Path -LiteralPath $Preferred).Path
    }

    # Prefer Release layouts for community packaging; Debug is last-resort only.
    $candidates = @(
        "builds\release\Release",
        "builds\release",
        "builds\ci\Release",
        "builds\ci",
        "builds\debug\Release",
        "builds\debug\Debug",
        "builds\debug",
        "builds\ci\Debug"
    )

    foreach ($c in $candidates)
    {
        $exe = Join-Path $c "Bridge.exe"
        if (Test-Path -LiteralPath $exe)
        {
            return (Resolve-Path -LiteralPath $c).Path
        }
    }
    return $null
}

$resolvedBridgeDir = Resolve-BridgeDir -Preferred $BridgeDir
if (-not $resolvedBridgeDir)
{
    throw "Bridge.exe not found under builds/. Build Bridge first, or pass -BridgeDir."
}

$bridgeExe = Join-Path $resolvedBridgeDir "Bridge.exe"
$inf = Join-Path $repoRoot "installer\mt4-winusb.inf"
if (-not (Test-Path -LiteralPath $inf))
{
    throw "Missing installer\mt4-winusb.inf"
}

$iss = Join-Path $repoRoot "installer\public-installer.iss"
if (-not (Test-Path -LiteralPath $iss))
{
    throw "Missing installer\public-installer.iss"
}

$unregisterHelper = Join-Path $repoRoot "installer\unregister-autostart-user.ps1"
if (-not (Test-Path -LiteralPath $unregisterHelper))
{
    throw "Missing installer\unregister-autostart-user.ps1"
}

if (-not $IsccPath)
{
    $candidates = @(
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles}\Inno Setup 6\ISCC.exe",
        "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
    )
    foreach ($c in $candidates)
    {
        if (Test-Path -LiteralPath $c)
        {
            $IsccPath = $c
            break
        }
    }
}

if (-not $IsccPath -or -not (Test-Path -LiteralPath $IsccPath))
{
    throw @"
Inno Setup 6 ISCC.exe not found.
Install Inno Setup 6, or pass -IsccPath to ISCC.exe.
See docs/tests/smoke-epic4-public-installer-mt4.md
"@
}

if ($resolvedBridgeDir -match '(?i)[\\/]debug([\\/]|$)')
{
    Write-Warning "Packaging Bridge from a Debug layout: $resolvedBridgeDir. Prefer builds\\release for community installs."
}

$outDir = Join-Path $repoRoot "builds\installer"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

# Optional Authenticode (Story 4.4) — strongly recommended, not a hard gate.
# Without UNITOR_CODE_SIGNING_CERT_SUBJECT the helper SKIPs (exit 0).
# When the env is set, Bridge is signed before packaging and Setup after.
$signHelper = Join-Path $repoRoot "scripts\packaging\sign-public-artifacts.ps1"
$wantPublicSign = -not [string]::IsNullOrWhiteSpace($env:UNITOR_CODE_SIGNING_CERT_SUBJECT)
if ($wantPublicSign -and -not (Test-Path -LiteralPath $signHelper))
{
    throw "UNITOR_CODE_SIGNING_CERT_SUBJECT is set but missing sign helper: $signHelper"
}

function Invoke-PublicSign
{
    param([string[]]$ArtifactPaths)

    if (-not (Test-Path -LiteralPath $signHelper))
    {
        return
    }

    & $signHelper -Paths $ArtifactPaths
    if ($LASTEXITCODE -ne 0)
    {
        throw "sign-public-artifacts.ps1 exited $LASTEXITCODE"
    }
}

if ($wantPublicSign)
{
    Invoke-PublicSign -ArtifactPaths @($bridgeExe)
}

Write-Host "Compiling Public Installer"
Write-Host "  BridgeSource: $resolvedBridgeDir"
Write-Host "  AppVersion:   $AppVersion"
Write-Host "  ISCC:         $IsccPath"

& $IsccPath "/DBridgeSource=$resolvedBridgeDir" "/DMyAppVersion=$AppVersion" $iss
if ($LASTEXITCODE -ne 0)
{
    throw "ISCC exited $LASTEXITCODE"
}

$setup = Join-Path $outDir "UnitorMt4Bridge-Setup.exe"
if (-not (Test-Path -LiteralPath $setup))
{
    throw "Expected output missing: $setup"
}

Invoke-PublicSign -ArtifactPaths @($setup)

Write-Host "OK: $setup"
exit 0
