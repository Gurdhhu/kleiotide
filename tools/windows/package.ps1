param(
    [Parameter(Mandatory=$true)][string]$QtRoot,
    [string]$BuildDir = (Join-Path $PSScriptRoot '../../build-release'),
    [string]$OutputRoot = (Join-Path $PSScriptRoot '../../dist')
)
$ErrorActionPreference = 'Stop'
function Run-Native([scriptblock]$Command) {
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $Command
        $exitCode = $LASTEXITCODE
    } finally { $ErrorActionPreference = $previousPreference }
    if ($exitCode -ne 0) { throw "Packaging command failed with exit code $exitCode." }
}
$sourceDir = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$qtPath = (Resolve-Path $QtRoot).Path
$deployTool = Join-Path $qtPath 'bin/windeployqt.exe'
if (!(Test-Path $deployTool)) { throw 'Cannot find windeployqt.exe in the chosen Qt kit.' }
$cmakeText = Get-Content (Join-Path $sourceDir 'CMakeLists.txt') -Raw
if ($cmakeText -notmatch 'project\(Kleiotide VERSION ([0-9.]+)') { throw 'Cannot determine project version.' }
$version = $Matches[1]
New-Item -ItemType Directory -Force $OutputRoot | Out-Null
$OutputRoot = (Resolve-Path $OutputRoot).Path
$packageDir = Join-Path $OutputRoot "Kleiotide_$version"
$archive = "$packageDir.zip"
if ((Test-Path $packageDir) -or (Test-Path $archive)) { throw 'Package output already exists. Choose a new output folder.' }
$env:PATH = (Join-Path $qtPath 'bin') + ';' + $env:PATH
# Build/validate first using validate.ps1. An incomplete package is never archived.
Run-Native { cmake --install $BuildDir --config Release --prefix $packageDir }
Run-Native { & $deployTool --release --dir $packageDir (Join-Path $packageDir 'kleiotide.exe') }
if (!(Test-Path (Join-Path $packageDir 'platforms/qwindows.dll'))) { throw 'Windows platform plugin was not deployed.' }
# Retain the exact kit's notices; the release checklist requires reviewing them
# together with GPLv3 requirements before public distribution.
if (Test-Path (Join-Path $qtPath 'LICENSES')) { Copy-Item (Join-Path $qtPath 'LICENSES') (Join-Path $packageDir 'Qt-LICENSES') -Recurse }
Get-ChildItem $packageDir -Recurse -File | ForEach-Object {
    $hash = Get-FileHash $_.FullName -Algorithm SHA256
    "$($hash.Hash)  $($_.FullName.Substring($packageDir.Length + 1))"
} | Set-Content (Join-Path $packageDir 'SHA256SUMS.txt') -Encoding UTF8
Compress-Archive -Path $packageDir -DestinationPath $archive
Write-Host "Internal test package: $archive"
Write-Host 'Complete native/clean-machine acceptance and dependency/source distribution review in RELEASE_CHECKLIST.md before public release.'
