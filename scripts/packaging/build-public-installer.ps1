# Assemble Public Installer EXE(s) from a builds/ Bridge artifact (Stories 4.1 / 6.2).
# Requires Inno Setup 6 (ISCC.exe). Output under dist/ (Luthier-style distributable folder).
#
# Dual community flavors (same semantic version; distinct artifact names):
#   win11-wms          — no teVirtualMIDI.dll gate; --midi-backend=wms
#   win10-virtualmidi  — DLL presence gate; --midi-backend=virtualmidi
#
# Version SSOT: CMake project(VERSION) → BridgeVersion*.in / bridge-version.txt /
# Bridge --version. This script defaults MyAppVersion from that same source
# (bridge-version.txt beside the build tree, else parse CMakeLists.txt).
# Pass -AppVersion only to override deliberately.
#
# Usage:
#   .\scripts\packaging\build-public-installer.ps1
#   .\scripts\packaging\build-public-installer.ps1 -Flavor both
#   .\scripts\packaging\build-public-installer.ps1 -Flavor win11-wms
#   .\scripts\packaging\build-public-installer.ps1 -BridgeDir builds\release\Release
#   .\scripts\packaging\build-public-installer.ps1 -AppVersion 0.2.0

[CmdletBinding()]
param(
    [string]$BridgeDir = "",
    [string]$IsccPath = "",
    [string]$AppVersion = "",
    [ValidateSet("win11-wms", "win10-virtualmidi", "both")]
    [string]$Flavor = "both"
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

function Get-VersionFromBridgeTree
{
    param([string]$StartDir)

    $dir = $StartDir
    for ($i = 0; $i -lt 5; $i++)
    {
        $candidate = Join-Path $dir "bridge-version.txt"
        if (Test-Path -LiteralPath $candidate)
        {
            $raw = (Get-Content -LiteralPath $candidate -Raw).Trim()
            if ($raw -match '^\d+\.\d+\.\d+')
            {
                return $Matches[0]
            }
        }
        $parent = Split-Path -Parent $dir
        if (-not $parent -or $parent -eq $dir)
        {
            break
        }
        $dir = $parent
    }
    return $null
}

function Get-VersionFromCMakeLists
{
    param([string]$RepoRoot)

    $cmake = Join-Path $RepoRoot "CMakeLists.txt"
    if (-not (Test-Path -LiteralPath $cmake))
    {
        return $null
    }
    $text = Get-Content -LiteralPath $cmake -Raw
    # SSOT: project(unitor-win64-driver VERSION x.y.z ...)
    if ($text -match 'project\s*\(\s*unitor-win64-driver\s+VERSION\s+(\d+\.\d+\.\d+)')
    {
        return $Matches[1]
    }
    return $null
}

function Get-FourPartVersion
{
    param([string]$Version)

    $parts = @($Version.Split('.'))
    while ($parts.Count -lt 4)
    {
        $parts += "0"
    }
    return ($parts[0..3] -join ".")
}

function Get-FlavorDefines
{
    param([string]$FlavorName)

    switch ($FlavorName)
    {
        "win11-wms" {
            return @{
                FlavorToken = "win11-wms"
                RequireVirtualMidi = "0"
                MidiBackendArg = "wms"
            }
        }
        "win10-virtualmidi" {
            return @{
                FlavorToken = "win10-virtualmidi"
                RequireVirtualMidi = "1"
                MidiBackendArg = "virtualmidi"
            }
        }
        default {
            throw "Unknown flavor '$FlavorName' (expected win11-wms or win10-virtualmidi)."
        }
    }
}

$resolvedBridgeDir = Resolve-BridgeDir -Preferred $BridgeDir
if (-not $resolvedBridgeDir)
{
    throw "Bridge.exe not found under builds/. Build Bridge first, or pass -BridgeDir."
}

if (-not $AppVersion)
{
    $AppVersion = Get-VersionFromBridgeTree -StartDir $resolvedBridgeDir
    if (-not $AppVersion)
    {
        $AppVersion = Get-VersionFromCMakeLists -RepoRoot $repoRoot
    }
    if (-not $AppVersion)
    {
        throw "Could not resolve AppVersion from bridge-version.txt or CMakeLists.txt project(VERSION). Pass -AppVersion to override."
    }
}

if ($AppVersion -notmatch '^\d+\.\d+\.\d+')
{
    throw "AppVersion '$AppVersion' must look like major.minor.patch (e.g. 0.1.0)."
}

$AppVersionInfo = Get-FourPartVersion -Version $AppVersion

$bridgeExe = Join-Path $resolvedBridgeDir "Bridge.exe"

# Cross-check packaged Bridge --version against resolved AppVersion (same CMake SSOT).
$bridgeVersionOut = & $bridgeExe --version 2>&1 | Out-String
if ($LASTEXITCODE -ne 0)
{
    throw "Bridge.exe --version failed (exit $LASTEXITCODE). Refusing to package."
}
if ($bridgeVersionOut -notmatch [regex]::Escape($AppVersion))
{
    throw @"
Bridge.exe --version does not contain AppVersion '$AppVersion'.
Output: $($bridgeVersionOut.Trim())
Refusing to package a mismatched Setup.
"@
}

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

$outDir = Join-Path $repoRoot "dist"
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

$flavorsToBuild = @()
if ($Flavor -eq "both")
{
    $flavorsToBuild = @("win11-wms", "win10-virtualmidi")
}
else
{
    $flavorsToBuild = @($Flavor)
}

$builtSetups = @()
foreach ($flavorName in $flavorsToBuild)
{
    $defs = Get-FlavorDefines -FlavorName $flavorName
    $setupName = "Unitor-MT4-Bridge-$AppVersion-$($defs.FlavorToken)-setup.exe"
    $setupPath = Join-Path $outDir $setupName

    Write-Host "Compiling Public Installer ($flavorName)"
    Write-Host "  BridgeSource:   $resolvedBridgeDir"
    Write-Host "  AppVersion:     $AppVersion (CMake SSOT unless -AppVersion override)"
    Write-Host "  VersionInfo:    $AppVersionInfo"
    Write-Host "  FlavorToken:    $($defs.FlavorToken)"
    Write-Host "  RequireVM:      $($defs.RequireVirtualMidi)"
    Write-Host "  MidiBackend:    $($defs.MidiBackendArg)"
    Write-Host "  ISCC:           $IsccPath"

    & $IsccPath `
        "/DBridgeSource=$resolvedBridgeDir" `
        "/DMyAppVersion=$AppVersion" `
        "/DMyAppVersionInfo=$AppVersionInfo" `
        "/DFlavorToken=$($defs.FlavorToken)" `
        "/DRequireVirtualMidi=$($defs.RequireVirtualMidi)" `
        "/DMidiBackendArg=$($defs.MidiBackendArg)" `
        $iss
    if ($LASTEXITCODE -ne 0)
    {
        foreach ($prior in $builtSetups)
        {
            if (Test-Path -LiteralPath $prior)
            {
                Remove-Item -LiteralPath $prior -Force
                Write-Warning "Removed sibling Setup from this failed dual build: $prior"
            }
        }
        if (Test-Path -LiteralPath $setupPath)
        {
            Remove-Item -LiteralPath $setupPath -Force
            Write-Warning "Removed partial Setup after ISCC failure: $setupPath"
        }
        throw "ISCC exited $LASTEXITCODE for flavor $flavorName (sibling Setups from this run were removed)"
    }

    if (-not (Test-Path -LiteralPath $setupPath))
    {
        foreach ($prior in $builtSetups)
        {
            if (Test-Path -LiteralPath $prior)
            {
                Remove-Item -LiteralPath $prior -Force
                Write-Warning "Removed sibling Setup after missing output: $prior"
            }
        }
        throw "Expected output missing: $setupPath"
    }

    Invoke-PublicSign -ArtifactPaths @($setupPath)
    Write-Host "OK: $setupPath"
    $builtSetups += $setupPath
}

Write-Host "Built $($builtSetups.Count) Setup artifact(s)."
exit 0
