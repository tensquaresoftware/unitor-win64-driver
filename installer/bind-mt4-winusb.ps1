# Guided MT4 WinUSB bind helper (contributor / lab machines).
# Primary community path is this INF package — not Zadig.
# Run from an elevated PowerShell session on Windows 10/11 x64.
#
# Usage:
#   .\installer\bind-mt4-winusb.ps1
#   .\installer\bind-mt4-winusb.ps1 -InfPath .\installer\mt4-winusb.inf

[CmdletBinding()]
param(
    [string]$InfPath = (Join-Path $PSScriptRoot "mt4-winusb.inf")
)

$ErrorActionPreference = "Stop"

if (-not [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
        [System.Runtime.InteropServices.OSPlatform]::Windows))
{
    throw "This script must run on Windows."
}

$resolvedInf = (Resolve-Path -LiteralPath $InfPath).Path
Write-Host "Staging driver package from: $resolvedInf"

# /install adds the package to the driver store and attempts PnP install.
# Unsigned INF may require test signing or manual Device Manager update — see docs/dev/winusb-bind.md.
pnputil /add-driver $resolvedInf /install
if ($LASTEXITCODE -ne 0)
{
    Write-Warning "pnputil exited with code $LASTEXITCODE. If the INF is unsigned, use Device Manager Update driver and browse to the installer folder (see docs/dev/winusb-bind.md)."
    exit $LASTEXITCODE
}

Write-Host "Driver package staged. Verify in Device Manager that interface USB\VID_086A&PID_0003&MI_02 is bound to WinUSB and exposes DeviceInterfaceGUID {aa209017-cf8a-49ad-a0e7-701187ff7e05}."
exit 0
