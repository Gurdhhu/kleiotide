# GPL-3.0-only; copyright 2026 Oleg Shchepin.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$QtRoot,
    [Parameter(Mandatory=$true)][string]$BuildDir,
    [Parameter(Mandatory=$true)][string]$SourceArchive,
    [Parameter(Mandatory=$true)][string]$QtSourceArchive,
    [Parameter(Mandatory=$true)][string]$VcRedist,
    [string]$CmakeExecutable = 'cmake',
    [string]$NsisCompiler,
    [ValidatePattern('^[1-9][0-9]*$')][string]$TestRevision = '1',
    [string]$OutputRoot
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:OS -ne 'Windows_NT') { throw 'Build the real installer on Windows with the MSVC x64 Release kit.' }
function Run-Native([scriptblock]$Command) {
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $Command | Out-Host
        $exitCode = $LASTEXITCODE
    } finally { $ErrorActionPreference = $previousPreference }
    if ($exitCode -ne 0) { throw "Installer command failed with exit code $exitCode." }
}
function Existing-Path([string]$Value) { return (Resolve-Path -LiteralPath $Value).Path }
function Nsis-Literal([string]$Value) { return $Value.Replace('$', '$$').Replace('"', '$\"') }
function Archive-Relative([string]$Base, [string]$Name) {
    $parts = [Collections.Generic.List[string]]::new()
    foreach ($part in ($Base + '/' + $Name).Replace('\', '/').Split('/')) {
        if ($part -eq '..') {
            if (!$parts.Count) { throw 'License path escapes the Qt archive.' }
            $parts.RemoveAt($parts.Count - 1)
        } elseif ($part -and $part -ne '.') { $parts.Add($part) }
    }
    return [string]::Join('/', $parts)
}
$sourceDir = Existing-Path (Join-Path $PSScriptRoot '../..')
$BuildDir = Existing-Path $BuildDir
$QtRoot = Existing-Path $QtRoot
$SourceArchive = Existing-Path $SourceArchive
$QtSourceArchive = Existing-Path $QtSourceArchive
$VcRedist = Existing-Path $VcRedist
if (!$OutputRoot) { $OutputRoot = Join-Path $BuildDir 'test-installers' }
$cmakeText = Get-Content -LiteralPath (Join-Path $sourceDir 'CMakeLists.txt') -Raw
if ($cmakeText -notmatch 'project\(Kleiotide VERSION ([0-9.]+)') { throw 'Cannot read application version.' }
$version = $Matches[1]
$cache = Get-Content -LiteralPath (Join-Path $BuildDir 'CMakeCache.txt') -Raw
if ($cache -notmatch '(?m)^CMAKE_BUILD_TYPE:STRING=Release\r?$') { throw 'Select the single-configuration MSVC x64 Release build used by Qt Creator.' }
if ($cache -notmatch '(?m)^CMAKE_HOME_DIRECTORY:INTERNAL=(.+)\r?$') { throw 'Missing CMake source directory.' }
if ((Existing-Path $Matches[1].Trim()) -ne $sourceDir) { throw 'Build directory belongs to different source code. Open this source in Qt Creator and build it first.' }
if ($cache -notmatch '(?m)^Qt6_DIR:PATH=(.+)\r?$') { throw 'Qt kit is missing from the build cache.' }
if ((Existing-Path $Matches[1].Trim()) -ne (Existing-Path (Join-Path $QtRoot 'lib/cmake/Qt6'))) { throw 'QtRoot does not match the selected build kit.' }
$deploy = Join-Path $QtRoot 'bin/windeployqt.exe'
$qmake = Join-Path $QtRoot 'bin/qmake.exe'
if (!(Test-Path -LiteralPath $deploy)) { throw 'The selected Qt kit has no windeployqt.exe.' }
$qtVersion = (& $qmake -query QT_VERSION).Trim()
if ($LASTEXITCODE -ne 0 -or $qtVersion -notmatch '^6\.[0-9]+\.[0-9]+$') { throw 'Cannot query the selected Qt version.' }
$signature = Get-AuthenticodeSignature -LiteralPath $VcRedist
if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch '(?:^|, )O=Microsoft Corporation(?:,|$)') {
    throw 'Use an unmodified Microsoft-signed vc_redist.x64.exe downloaded from Microsoft.'
}
if ([IO.Path]::GetFileName($VcRedist) -ine 'vc_redist.x64.exe') { throw 'Choose the Microsoft x64 Redistributable, vc_redist.x64.exe.' }
if (!$NsisCompiler) {
    foreach ($candidate in @("${env:ProgramFiles(x86)}/NSIS/makensis.exe", "$env:ProgramFiles/NSIS/makensis.exe")) {
        if (Test-Path -LiteralPath $candidate) { $NsisCompiler = $candidate; break }
    }
    if (!$NsisCompiler) { $NsisCompiler = (Get-Command makensis.exe -ErrorAction Stop).Source }
}
$NsisCompiler = Existing-Path $NsisCompiler
$nsisLicense = Join-Path (Split-Path $NsisCompiler) 'COPYING'
if (!(Test-Path -LiteralPath $nsisLicense)) { throw 'The NSIS installation must include its COPYING license file.' }
Add-Type -AssemblyName System.IO.Compression.FileSystem
# Verify the supplied Kleiotide ZIP against this source tree before embedding it.
$archive = [IO.Compression.ZipFile]::OpenRead($SourceArchive)
try {
    $prefix = "Kleiotide_$version/"
    $archivedSources = @{}
    foreach ($entry in $archive.Entries) {
        if (!$entry.Name) { continue }
        if (!$entry.FullName.StartsWith($prefix) -or $entry.FullName.Contains('../')) { throw 'Choose the complete source ZIP for this version.' }
        $relative = $entry.FullName.Substring($prefix.Length)
        $local = Join-Path $sourceDir $relative
        $stream = $entry.Open()
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $zipHash = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '') }
        finally { $stream.Dispose(); $sha.Dispose() }
        if (!(Test-Path -LiteralPath $local -PathType Leaf) -or (Get-FileHash -LiteralPath $local -Algorithm SHA256).Hash -ne $zipHash) {
            throw "Source ZIP differs from the working source: $relative. Extract a fresh copy and build that copy."
        }
        $archivedSources[$relative] = $true
    }
    foreach ($file in Get-ChildItem (Join-Path $sourceDir 'src') -File -Recurse) {
        $relative = $file.FullName.Substring($sourceDir.Length + 1).Replace('\', '/')
        if (!$archivedSources.ContainsKey($relative)) { throw "Source ZIP omits $relative." }
    }
    if (!$archivedSources.ContainsKey('CMakeLists.txt') -or !$archivedSources.ContainsKey('LICENSE')) { throw 'Incomplete source ZIP.' }
} finally { $archive.Dispose() }
# Read notices from the exact official Qt source ZIP, without unpacking its binaries/tools.
$qtArchive = [IO.Compression.ZipFile]::OpenRead($QtSourceArchive)
try {
    $qtPrefix = "qt-everywhere-src-$qtVersion/"
    if (!$qtArchive.GetEntry("${qtPrefix}qtbase/CMakeLists.txt")) { throw "Use the official qt-everywhere-src-$qtVersion.zip source archive for this stock Qt kit." }
    New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
    $OutputRoot = Existing-Path $OutputRoot
    $releaseDir = Join-Path $OutputRoot "Kleiotide_${version}_test$TestRevision"
    if (Test-Path -LiteralPath $releaseDir) { throw 'This installer output exists. Increase KLEIOTIDE_INSTALLER_REVISION or choose a new output folder.' }
    New-Item -ItemType Directory -Path $releaseDir | Out-Null
    $payload = Join-Path $releaseDir 'work/payload'
    Run-Native { & $CmakeExecutable --install $BuildDir --config Release --prefix $payload }
    $exe = Join-Path $payload 'kleiotide.exe'
    $pe = [IO.File]::OpenRead($exe)
    $reader = [IO.BinaryReader]::new($pe)
    try {
        $pe.Position = 0x3c; $offset = $reader.ReadInt32(); $pe.Position = $offset
        if ($reader.ReadUInt32() -ne 0x4550 -or $reader.ReadUInt16() -ne 0x8664) { throw 'The application is not an x64 Windows executable.' }
    } finally { $reader.Dispose(); $pe.Dispose() }
    # Use the selected kit, never the Qt version embedded in Qt Creator itself.
    $env:PATH = (Join-Path $QtRoot 'bin') + ';' + $env:PATH
    Run-Native { & $deploy --release --no-compiler-runtime --no-opengl-sw --no-system-d3d-compiler --dir $payload $exe }
    foreach ($required in @('Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Widgets.dll', 'Qt6Network.dll', 'platforms/qwindows.dll', 'tls/qschannelbackend.dll')) {
        if (!(Test-Path -LiteralPath (Join-Path $payload $required))) { throw "Missing deployed file: $required" }
    }
    # Use Windows native TLS; no untracked OpenSSL runtime dependency.
    Remove-Item -LiteralPath (Join-Path $payload 'tls/qopensslbackend.dll') -ErrorAction SilentlyContinue
    $noticeRoot = Join-Path $payload 'third-party/Qt'
    $noticeEntries = @{}
    # These deployed modules live in qtbase, qtsvg and qtimageformats. Avoid
    # unrelated Chromium/WebEngine notices and their very long source paths.
    foreach ($entry in $qtArchive.Entries) {
        if (!$entry.FullName.StartsWith($qtPrefix) -or !$entry.Name) { continue }
        $relative = $entry.FullName.Substring($qtPrefix.Length)
        if ($relative -notmatch '^(qtbase|qtsvg|qtimageformats)/|^[^/]+$|^LICENSES/') { continue }
        if ($relative -match '(^|/)(LICENSES/.*|[^/]*(LICENSE|LICENCE|COPYING|COPYRIGHT|NOTICE)[^/]*|FTL\.TXT|qt_attribution\.json)$') { $noticeEntries[$relative] = $entry }
        if ($entry.Name -ine 'qt_attribution.json') { continue }
        $streamReader = [IO.StreamReader]::new($entry.Open())
        try { $attribution = $streamReader.ReadToEnd() } finally { $streamReader.Dispose() }
        # Qt attribution files allow multiline strings; extract LicenseFile
        # fields rather than relying on a strict JSON parser.
        foreach ($match in [regex]::Matches($attribution, '"LicenseFile"\s*:\s*"([^"\r\n]+)"')) {
            $parent = $relative.Substring(0, $relative.LastIndexOf('/'))
            $licenseRelative = Archive-Relative $parent $match.Groups[1].Value
            $licenseEntry = $qtArchive.GetEntry($qtPrefix + $licenseRelative)
            if (!$licenseEntry) { throw "Qt attribution references a missing license: $licenseRelative" }
            $noticeEntries[$licenseRelative] = $licenseEntry
        }
    }
    $noticeCount = 0
    foreach ($entry in $noticeEntries.Values) {
        $relative = $entry.FullName.Substring($qtPrefix.Length)
        if ($relative.Contains('../') -or $relative.Contains('\')) { throw 'Unsafe path in Qt source archive.' }
        $destination = Join-Path $noticeRoot $relative
        New-Item -ItemType Directory -Force (Split-Path $destination) | Out-Null
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $destination, $false)
        $noticeCount++
    }
    if ($noticeCount -eq 0) { throw 'No Qt license notices found.' }
} finally { $qtArchive.Dispose() }
New-Item -ItemType Directory -Force (Join-Path $payload 'source') | Out-Null
Copy-Item -LiteralPath $SourceArchive -Destination (Join-Path $payload 'source/Kleiotide_Source.zip')
Copy-Item -LiteralPath $nsisLicense -Destination (Join-Path $payload 'third-party/NSIS-COPYING.txt')
Copy-Item -LiteralPath $SourceArchive -Destination (Join-Path $releaseDir "Kleiotide_${version}_Source.zip")
Copy-Item -LiteralPath $QtSourceArchive -Destination (Join-Path $releaseDir "qt-everywhere-src-$qtVersion.zip")
$sourceNotice = @"
Kleiotide $version test $TestRevision, copyright 2026 Oleg Shchepin, GPL-3.0-only.
Exact application source: source/Kleiotide_Source.zip inside the installation.
This application uses Qt $qtVersion, copyright The Qt Company Ltd. and contributors.
This is an x64 application. ARM64 Windows 11 with x64 emulation is permitted
for compatibility testing; native ARM64 binaries are not included or qualified.
Qt and embedded third-party notices: third-party/Qt, preserving source paths.
The unchanged corresponding Qt source is qt-everywhere-src-$qtVersion.zip
alongside this setup in the release folder supplied by the distributor.
Keep that source archive available to every recipient, including on download sites.
Qt shared libraries can be replaced with compatible modified builds.
Microsoft Visual C++ runtime: separate Microsoft license; installed by its original installer.
NSIS installer engine: see third-party/NSIS-COPYING.txt. Compression uses zlib.
MAFFT is optional, external and not bundled. The official artwork is not yet included.
"@
$sourceNotice | Set-Content -LiteralPath (Join-Path $payload 'THIRD_PARTY.txt') -Encoding UTF8
$sourceNotice | Set-Content -LiteralPath (Join-Path $releaseDir 'READ-ME.txt') -Encoding UTF8
$manifest = [ordered]@{
    applicationVersion = $version; installerRevision = $TestRevision
    builtUtc = [DateTime]::UtcNow.ToString('o'); qtVersion = $qtVersion
    qtSourceSha256 = (Get-FileHash -LiteralPath $QtSourceArchive -Algorithm SHA256).Hash
    sourceSha256 = (Get-FileHash -LiteralPath $SourceArchive -Algorithm SHA256).Hash
    vcRedistVersion = (Get-Item -LiteralPath $VcRedist).VersionInfo.FileVersion
    vcRedistSha256 = (Get-FileHash -LiteralPath $VcRedist -Algorithm SHA256).Hash
    nsisVersion = (& $NsisCompiler /VERSION | Out-String).Trim()
    packageAcceptance = 'PENDING: test setup, startup and uninstall on Windows without developer tools'
}
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $payload 'BUILD-MANIFEST.json') -Encoding UTF8
Copy-Item -LiteralPath (Join-Path $payload 'BUILD-MANIFEST.json') -Destination $releaseDir
$deleteList = Join-Path $releaseDir 'work/delete-files.nsh'
$deleteCommands = @()
foreach ($file in Get-ChildItem -LiteralPath $payload -File -Recurse | Sort-Object FullName) {
    $relative = $file.FullName.Substring($payload.Length + 1)
    if ($relative -ieq 'kleiotide.exe') { continue }
    $deleteCommands += 'Delete "$INSTDIR\' + (Nsis-Literal $relative) + '"'
}
foreach ($dir in Get-ChildItem -LiteralPath $payload -Directory -Recurse | Sort-Object { $_.FullName.Length } -Descending) {
    $deleteCommands += 'RMDir "$INSTDIR\' + (Nsis-Literal $dir.FullName.Substring($payload.Length + 1)) + '"'
}
[IO.File]::WriteAllLines($deleteList, [string[]]$deleteCommands, [Text.UTF8Encoding]::new($true))
$setup = Join-Path $releaseDir "Kleiotide_${version}_test${TestRevision}_Windows_x64_Setup.exe"
Run-Native { & $NsisCompiler /V2 "/DAPP_VERSION=$version" "/DTEST_REVISION=$TestRevision" "/DPAYLOAD_DIR=$payload" "/DSETUP_FILE=$setup" "/DVC_REDIST_FILE=$VcRedist" "/DDELETE_LIST=$deleteList" (Join-Path $PSScriptRoot 'installer/Kleiotide.nsi') }
if (!(Test-Path -LiteralPath $setup)) { throw 'NSIS did not produce an installer.' }
# Only this newly created staging directory is removed; installer/user paths are never recursive-deleted.
Remove-Item -LiteralPath (Join-Path $releaseDir 'work') -Recurse
Get-ChildItem -LiteralPath $releaseDir -File | Sort-Object Name | ForEach-Object {
    "$((Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash)  $($_.Name)"
} | Set-Content -LiteralPath (Join-Path $releaseDir 'SHA256SUMS.txt') -Encoding UTF8
Write-Host "Created: $setup"
Write-Host "Keep/share the entire release folder (including corresponding sources): $releaseDir"
Write-Host 'Native installer acceptance remains pending. Follow WINDOWS_INSTALLER.md before inviting wider testing.'
