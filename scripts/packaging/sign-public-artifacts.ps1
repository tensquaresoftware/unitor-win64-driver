# Optional: Authenticode-sign public Bridge / Setup artifacts (Story 4.4).
# Distinct from installer/sign-lab-package.ps1 (lab INF catalog / self-signed Root staging).
#
# Gate: runs only when UNITOR_CODE_SIGNING_CERT_SUBJECT is set (or -CertSubject is passed).
# Without a cert, exit 0 and print SKIP - Authenticode is strongly recommended, not a hard V1 gate.
#
# Usage:
#   $env:UNITOR_CODE_SIGNING_CERT_SUBJECT = 'Ten Square Software'
#   .\scripts\packaging\sign-public-artifacts.ps1 -Paths @('dist\Unitor-MT4-Bridge-0.1.0-win11-wms-setup.exe','dist\Unitor-MT4-Bridge-0.1.0-win10-virtualmidi-setup.exe')
#   .\scripts\packaging\sign-public-artifacts.ps1 -Paths @('builds\release\Release\Bridge.exe') -CertSubject 'Ten Square Software'

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string[]]$Paths,

    [string]$CertSubject = "",

    [string]$TimestampUrl = "http://timestamp.digicert.com"
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Set-Location $repoRoot

if (-not $CertSubject) {
    $CertSubject = $env:UNITOR_CODE_SIGNING_CERT_SUBJECT
}

if (-not $CertSubject) {
    Write-Host "SKIP: public Authenticode - set UNITOR_CODE_SIGNING_CERT_SUBJECT (or -CertSubject) to sign. Unsigned packaging remains allowed for V1 with SmartScreen docs."
    exit 0
}

$resolvedPaths = @()
foreach ($raw in $Paths) {
    if (-not $raw) { continue }
    $path = $raw
    if (-not [System.IO.Path]::IsPathRooted($path)) {
        $path = Join-Path $repoRoot $path
    }
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing artifact to sign: $path"
    }
    $resolvedPaths += (Resolve-Path -LiteralPath $path).Path
}

if ($resolvedPaths.Count -eq 0) {
    throw "No artifact paths to sign (Paths was empty or blank)."
}

function Find-KitTool([string]$name) {
    $roots = @(
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin",
        "$env:ProgramFiles\Windows Kits\10\bin"
    )
    $candidates = @()
    foreach ($root in $roots) {
        if (-not (Test-Path -LiteralPath $root)) { continue }
        $candidates += Get-ChildItem -Path $root -Recurse -Filter $name -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\(x64|arm64)\\' }
    }
    if (-not $candidates) {
        throw "Could not find $name under Windows Kits (x64/arm64). Install Windows SDK / WDK tools."
    }

    $ranked = $candidates | ForEach-Object {
        $ver = [version]'0.0.0.0'
        if ($_.FullName -match '\\bin\\(\d+\.\d+\.\d+\.\d+)\\') {
            $ver = [version]$Matches[1]
        }
        [pscustomobject]@{ File = $_; Version = $ver }
    } | Sort-Object Version -Descending

    return $ranked[0].File.FullName
}

function Assert-SignedSubject([string]$path, [string]$expectedSubject) {
    $sig = Get-AuthenticodeSignature -FilePath $path
    if ($sig.Status -ne 'Valid') {
        throw "Authenticode status for $path is '$($sig.Status)' (expected Valid)."
    }
    $subject = $sig.SignerCertificate.Subject
    if ($subject -notlike "*$expectedSubject*") {
        throw "Signed subject for $path is '$subject' (expected to contain '$expectedSubject')."
    }
}

$signTool = Find-KitTool 'signtool.exe'
$tempCopies = @()

try {
    foreach ($path in $resolvedPaths) {
        $tmp = "$path.signing-tmp"
        Copy-Item -LiteralPath $path -Destination $tmp -Force
        $tempCopies += $tmp

        Write-Host "Signing (Authenticode): $path"
        Write-Host "  Subject: $CertSubject"
        Write-Host "  SignTool: $signTool"

        & $signTool sign /fd SHA256 /tr $TimestampUrl /td SHA256 /n $CertSubject $tmp
        if ($LASTEXITCODE -ne 0) {
            throw "SignTool exited $LASTEXITCODE for $path"
        }

        Assert-SignedSubject -path $tmp -expectedSubject $CertSubject
    }

    foreach ($path in $resolvedPaths) {
        $tmp = "$path.signing-tmp"
        Move-Item -LiteralPath $tmp -Destination $path -Force
    }
    $tempCopies = @()
}
finally {
    foreach ($tmp in $tempCopies) {
        if (Test-Path -LiteralPath $tmp) {
            Remove-Item -LiteralPath $tmp -Force -ErrorAction SilentlyContinue
        }
    }
}

Write-Host "OK: public Authenticode signing finished"
exit 0
