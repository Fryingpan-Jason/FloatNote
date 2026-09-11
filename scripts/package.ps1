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
if (-not $outputPath.StartsWith([IO.Path]::GetFullPath($projectRoot) + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
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

$fileVersion = (Get-Item -LiteralPath $BinaryPath).VersionInfo.FileVersion
if ($fileVersion -ne "$version.0") { throw "Binary version $fileVersion does not match package $version." }
$reader = [IO.BinaryReader]::new([IO.File]::OpenRead($BinaryPath))
try {
    if ($reader.ReadUInt16() -ne 0x5A4D) { throw 'Binary has no DOS header.' }
    $reader.BaseStream.Position = 0x3C
    $peOffset = $reader.ReadInt32()
    $reader.BaseStream.Position = $peOffset
    if ($reader.ReadUInt32() -ne 0x4550) { throw 'Binary has no PE signature.' }
    $machine = $reader.ReadUInt16()
    $expectedMachine = @{ x64=0x8664; x86=0x014C; arm64=0xAA64 }[$Architecture]
    if ($machine -ne $expectedMachine) { throw 'Binary architecture does not match package name.' }
} finally { $reader.Dispose() }

$packageName = "FloatNote-$version-windows-$Architecture"
$stagePath = Join-Path $outputPath "stage\$packageName"
$archivePath = Join-Path $outputPath "$packageName.zip"

if (-not ([IO.Path]::GetFullPath($stagePath)).StartsWith($outputPath + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Stage path must remain below the package output directory.'
}
if ((Test-Path -LiteralPath $stagePath) -and ((Get-Item -LiteralPath $stagePath).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
    throw 'Refusing to replace a linked staging directory.'
}

if (Test-Path -LiteralPath $stagePath) { Remove-Item -LiteralPath $stagePath -Recurse -Force }
New-Item -ItemType Directory -Force -Path $stagePath | Out-Null
Copy-Item -LiteralPath $BinaryPath -Destination (Join-Path $stagePath 'FloatNote.exe')
Copy-Item -LiteralPath (Join-Path $projectRoot 'README.md') -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $projectRoot 'README.en.md') -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $projectRoot 'LICENSE') -Destination (Join-Path $stagePath 'LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $projectRoot 'THIRD_PARTY_NOTICES.md') -Destination $stagePath
Copy-Item -LiteralPath (Join-Path $projectRoot 'docs\QUICKSTART.md') -Destination $stagePath

if (Test-Path -LiteralPath $archivePath) { Remove-Item -LiteralPath $archivePath -Force }
Compress-Archive -Path (Join-Path $stagePath '*') -DestinationPath $archivePath -CompressionLevel Optimal
$archive = [IO.Compression.ZipFile]::OpenRead($archivePath)
try {
    $expected = @('FloatNote.exe','README.md','README.en.md','LICENSE.txt','THIRD_PARTY_NOTICES.md','QUICKSTART.md') | Sort-Object
    $actual = @($archive.Entries | ForEach-Object FullName) | Sort-Object
    if (Compare-Object $expected $actual) { throw 'Archive contains missing or unexpected files.' }
} finally { $archive.Dispose() }
$hash = Get-FileHash -LiteralPath $archivePath -Algorithm SHA256
Write-Host ("Packaged {0}`nSHA256 {1}" -f $hash.Path, $hash.Hash)
