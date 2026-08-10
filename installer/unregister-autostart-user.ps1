# Unregister Auto-Start for the interactive user (Story 4.1 uninstall hygiene).
# Clears Task Scheduler and HKCU Run via Bridge --unregister-auto-start.
#
# Used by public-installer.iss [UninstallRun] (runascurrentuser) and by operators.
# Writes ProgramData exit marker for elevated-uninstall verification (not LocalAppData).
#
# Usage:
#   .\installer\unregister-autostart-user.ps1
#   .\installer\unregister-autostart-user.ps1 -BridgePath "C:\Program Files\Ten Square Software\Unitor MT4 Bridge\Bridge.exe"

[CmdletBinding()]
param(
    [string]$BridgePath = ""
)

$ErrorActionPreference = "Stop"

function Test-IsElevated
{
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($id)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Write-UnregisterExitMarker
{
    param([int]$Code)

    # ProgramData is visible to both runascurrentuser helper and elevated uninstall Code.
    $dir = Join-Path $env:ProgramData "Ten Square Software\Unitor MT4 Bridge"
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    $marker = Join-Path $dir "last-autostart-unregister.exit"
    Set-Content -LiteralPath $marker -Value "$Code" -Encoding ascii
}

if (Test-IsElevated)
{
    Write-UnregisterExitMarker -Code 2
    throw @"
Refusing to unregister Auto-Start while elevated.
Run this script from a normal (non-Administrator) PowerShell so Task Scheduler / HKCU
target the interactive user profile (AD-20 / Story 3.1), not the admin profile.
"@
}

if (-not $BridgePath)
{
    $default = Join-Path ${env:ProgramFiles} "Ten Square Software\Unitor MT4 Bridge\Bridge.exe"
    if (Test-Path -LiteralPath $default)
    {
        $BridgePath = $default
    }
    else
    {
        Write-UnregisterExitMarker -Code 1
        throw "Bridge.exe not found. Pass -BridgePath to the installed or builds/ binary."
    }
}

$resolved = (Resolve-Path -LiteralPath $BridgePath).Path
$workDir = Split-Path -Parent $resolved

Write-Host "Unregistering Auto-Start (interactive user): $resolved --unregister-auto-start"
$proc = Start-Process -FilePath $resolved -ArgumentList "--unregister-auto-start" `
    -WorkingDirectory $workDir -Wait -PassThru -NoNewWindow
if ($null -eq $proc.ExitCode)
{
    Write-UnregisterExitMarker -Code 1
    throw "Bridge did not report an exit code after --unregister-auto-start."
}

Write-UnregisterExitMarker -Code ([int]$proc.ExitCode)

if ($proc.ExitCode -ne 0)
{
    Write-Error "Bridge --unregister-auto-start exited $($proc.ExitCode)"
    exit $proc.ExitCode
}

exit 0
