# VirtualMIDI presence probe for Public Installer / operator smoke (Story 4.1).
# Aligns with Bridge fail-closed intent: teVirtualMIDI.dll via System32
# (VirtualMidiBackend LoadLibraryEx + LOAD_LIBRARY_SEARCH_SYSTEM32).
#
# Not invoked by public-installer.iss (SSOT for setup gates is the .iss Pascal code).
# Exit 0 = present. Exit 1 = missing (English fix path on stdout).
#
# Usage:
#   .\installer\check-virtualmidi.ps1

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

function Resolve-System32DllPath
{
    param([string]$FileName)

    # 32-bit PowerShell on 64-bit Windows sees SysWOW64 as System32 — use Sysnative.
    $isWow64 = [Environment]::Is64BitOperatingSystem -and -not [Environment]::Is64BitProcess
    if ($isWow64)
    {
        $sysnative = Join-Path $env:WINDIR "Sysnative\$FileName"
        if (Test-Path -LiteralPath $sysnative)
        {
            return $sysnative
        }
    }

    return (Join-Path $env:WINDIR "System32\$FileName")
}

$dll = Resolve-System32DllPath -FileName "teVirtualMIDI.dll"
if (Test-Path -LiteralPath $dll)
{
    Write-Host "VirtualMIDI present: $dll"
    exit 0
}

Write-Host @"
VirtualMIDI driver/DLL missing (teVirtualMIDI.dll not found in System32).
Install loopMIDI or rtpMIDI so the VirtualMIDI driver is present, then retry.
An empty MIDI port list is not a successful install.
Licensed VirtualMIDI MSI embedding is a future release gate (OQ-1) and is not shipped in the Public Installer yet.
"@
exit 1
