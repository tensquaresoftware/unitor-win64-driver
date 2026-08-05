# Lab-only: build a self-signed catalog for mt4-winusb.inf and stage it with pnputil.
# Requires elevated PowerShell + Windows SDK (MakeCat + SignTool).
# Not for public release (Story 4.4 Authenticode is separate).

[CmdletBinding()]
param(
    [switch]$SkipPnPUtil
)

$ErrorActionPreference = 'Stop'
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).
    IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    throw 'Run this script in elevated PowerShell (Administrator).'
}

$installerDir = $PSScriptRoot
$infPath = Join-Path $installerDir 'mt4-winusb.inf'
$catPath = Join-Path $installerDir 'mt4-winusb.cat'
$cdfPath = Join-Path $installerDir 'mt4-winusb.cdf'
$certSubject = 'CN=Ten Square Software Lab (MT4 WinUSB)'

if (-not (Test-Path -LiteralPath $infPath)) {
    throw "Missing INF: $infPath"
}

function Find-KitTool([string]$name) {
    $roots = @(
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin",
        "$env:ProgramFiles\Windows Kits\10\bin"
    )
    foreach ($root in $roots) {
        if (-not (Test-Path -LiteralPath $root)) { continue }
        $hit = Get-ChildItem -Path $root -Recurse -Filter $name -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\x64\\' } |
            Select-Object -First 1
        if ($hit) { return $hit.FullName }
    }
    throw "Could not find $name under Windows Kits. Install Windows SDK / WDK tools."
}

$makeCat = Find-KitTool 'makecat.exe'
$signTool = Find-KitTool 'signtool.exe'

$existing = Get-ChildItem Cert:\LocalMachine\My -CodeSigningCert -ErrorAction SilentlyContinue |
    Where-Object { $_.Subject -eq $certSubject -and $_.NotAfter -gt (Get-Date) } |
    Select-Object -First 1

if (-not $existing) {
    Write-Host "Creating lab code-signing certificate: $certSubject"
    $existing = New-SelfSignedCertificate `
        -Type CodeSigningCert `
        -Subject $certSubject `
        -CertStoreLocation 'Cert:\LocalMachine\My' `
        -KeyExportPolicy Exportable `
        -HashAlgorithm SHA256 `
        -NotAfter (Get-Date).AddYears(5)
}

foreach ($storeName in @('Root', 'TrustedPublisher')) {
    $store = New-Object System.Security.Cryptography.X509Certificates.X509Store($storeName, 'LocalMachine')
    $store.Open('ReadWrite')
    $already = $store.Certificates | Where-Object { $_.Thumbprint -eq $existing.Thumbprint }
    if (-not $already) {
        Write-Host "Installing certificate into LocalMachine\$storeName"
        $store.Add($existing)
    }
    $store.Close()
}

$cdf = @"
[CatalogHeader]
Name=mt4-winusb.cat
ResultDir=.
PublicVersion=0x0000001
EncodingType=0x00010001
CATATTR1=0x10010001:OSAttr:2:6.1,2:6.2,2:6.3,2:10.0

[CatalogFiles]
<hash>mt4-winusb.inf=mt4-winusb.inf
"@
Set-Content -LiteralPath $cdfPath -Value $cdf -Encoding ASCII

if (Test-Path -LiteralPath $catPath) {
    Remove-Item -LiteralPath $catPath -Force
}

Push-Location $installerDir
try {
    Write-Host "Building catalog with MakeCat..."
    & $makeCat -v mt4-winusb.cdf
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $catPath)) {
        throw "MakeCat failed (exit $LASTEXITCODE)"
    }

    Write-Host "Signing catalog..."
    & $signTool sign /fd SHA256 /sm /s My /n 'Ten Square Software Lab (MT4 WinUSB)' /v $catPath
    if ($LASTEXITCODE -ne 0) {
        throw "SignTool failed (exit $LASTEXITCODE)"
    }
}
finally {
    Pop-Location
    Remove-Item -LiteralPath $cdfPath -Force -ErrorAction SilentlyContinue
}

Write-Host "Lab package ready:"
Write-Host "  $infPath"
Write-Host "  $catPath"

if (-not $SkipPnPUtil) {
    Write-Host "Staging with pnputil..."
    & pnputil /add-driver $infPath /install
    Write-Host "pnputil exit: $LASTEXITCODE"
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "pnputil did not succeed. Try Device Manager > Have Disk on installer\mt4-winusb.inf"
    }
}

Write-Host "Done. Re-test: builds\debug\Debug\Bridge.exe --probe-usb"
