# Register Auto-Start for the interactive user (Story 4.1 / AD-20).
# Calls existing Bridge CLI — does not invent a second mechanism.
# Run unelevated so HKCU / Task Scheduler write the logged-on user profile.
#
# Not invoked by public-installer.iss (setup uses ExecAsOriginalUser).
# This helper is for operator smoke / manual repair only.
#
# Usage:
#   .\installer\register-autostart-user.ps1
#   .\installer\register-autostart-user.ps1 -BridgePath "C:\Program Files\Ten Square Software\Unitor MT4 Bridge\Bridge.exe"

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

if (Test-IsElevated)
{
    throw @"
Refusing to register Auto-Start while elevated.
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
        throw "Bridge.exe not found. Pass -BridgePath to the installed or builds/ binary."
    }
}

$resolved = (Resolve-Path -LiteralPath $BridgePath).Path
$workDir = Split-Path -Parent $resolved

Write-Host "Registering Auto-Start (interactive user): $resolved --register-auto-start"
$proc = Start-Process -FilePath $resolved -ArgumentList "--register-auto-start" `
    -WorkingDirectory $workDir -Wait -PassThru -NoNewWindow
if ($null -eq $proc.ExitCode)
{
    throw "Bridge did not report an exit code after --register-auto-start."
}
if ($proc.ExitCode -ne 0)
{
    Write-Error "Bridge --register-auto-start exited $($proc.ExitCode)"
    exit $proc.ExitCode
}

exit 0
