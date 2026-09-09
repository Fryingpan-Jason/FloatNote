[CmdletBinding()]
param(
    [ValidateSet('x64', 'x86', 'arm64')]
    [string]$Architecture = 'x64',
    [string]$OutputDirectory = 'dist',
    [string]$BinaryPath = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$versionHeader = Get-Content -LiteralPath (Join-Path $projectRoot 'src\version.h') -Raw
if ($versionHeader -notmatch '#define\s+FLOATNOTE_VERSION_STRING\s+"([^"]+)"') {
    throw 'Could not read FLOATNOTE_VERSION_STRING from src/version.h.'
}
$version = $Matches[1]

$outputPath = [IO.Path]::GetFullPath((Join-Path $projectRoot $OutputDirectory))
if (-not $outputPath.StartsWith([IO.Path]::GetFullPath($projectRoot), [StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputDirectory must stay inside the FloatNote project.'
}

if (-not $BinaryPath) {
    $buildPath = Join-Path $projectRoot "build\package-$Architecture"
    & (Join-Path $projectRoot 'build.ps1') -Architecture $Architecture -OutputDirectory "build\package-$Architecture"
    $BinaryPath = Join-Path $buildPath 'FloatNote.exe'
}
if (-not [IO.Path]::IsPathRooted($BinaryPath)) { $BinaryPath = Join-Path $projectRoot $BinaryPath }
$BinaryPath = [IO.Path]::GetFullPath($BinaryPath)
if (-not (Test-Path -LiteralPath $BinaryPath)) { throw "Binary not found: $BinaryPath" }

$packageName = "FloatNote-$version-windows-$Architecture"
$stagePath = Join-Path $outputPath "stage\$packageName"
$archivePath = Join-Path $outputPath "$packageName.zip"

if (Test-Path -LiteralPath $stagePath) { Remove-Item -LiteralPath $stagePath -Recurse -Force }
New-Item -ItemType Directory -Force -Path $stagePath | Out-Null
Copy-Item -LiteralPath $BinaryPath -Destination (Join-Path $stagePath 'FloatNote.exe')
Copy-Item -LiteralPath (Join-Path $projectRoot 'README.md') -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $projectRoot 'README.en.md') -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $projectRoot 'LICENSE') -Destination (Join-Path $stagePath 'LICENSE.txt')

if (Test-Path -LiteralPath $archivePath) { Remove-Item -LiteralPath $archivePath -Force }
Compress-Archive -Path (Join-Path $stagePath '*') -DestinationPath $archivePath -CompressionLevel Optimal
$hash = Get-FileHash -LiteralPath $archivePath -Algorithm SHA256
Write-Host ("Packaged {0}`nSHA256 {1}" -f $hash.Path, $hash.Hash)
