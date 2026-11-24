# GPL-3.0-only
param([Parameter(Mandatory)][string]$Archive)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'installer/Mafft-Files.ps1')
Add-Type -AssemblyName System.IO.Compression.FileSystem
$root = Join-Path ([IO.Path]::GetTempPath()) ('kleiotide-mafft-test-' + [Guid]::NewGuid())
$tools = Join-Path $root 'Kleiotide Tools'
$installed = Join-Path $tools 'MAFFT-7.526'
$stage = Join-Path $root 'stage'
$extracted = Join-Path $stage 'mafft-win'
try {
    [void][IO.Directory]::CreateDirectory($tools)
    $archivePath = (Resolve-Path -LiteralPath $Archive).ProviderPath
    [IO.Compression.ZipFile]::ExtractToDirectory($archivePath, $stage)
    Publish-MafftDirectory $extracted $installed $root
    if (!(Test-Path -LiteralPath (Join-Path $installed 'tmp') -PathType Container)) {
        throw 'The MAFFT fixture must include the empty tmp folder.'
    }
    Remove-MafftDirectory $installed $root
    if (Test-Path -LiteralPath $installed) { throw 'Uninstall left a MAFFT folder behind.' }
    Write-Output 'PASS MAFFT 7.526 installs and uninstalls without empty directory leftovers.'

    # Recreate the empty directory tree left by Kleiotide 0.30.3.
    [void][IO.Directory]::CreateDirectory((Join-Path $installed 'tmp'))
    [IO.Compression.ZipFile]::ExtractToDirectory($archivePath, $stage)
    Publish-MafftDirectory $extracted $installed $root
    if (!(Test-Path -LiteralPath (Join-Path $installed 'mafft.bat') -PathType Leaf)) {
        throw 'Reinstall failed to publish the MAFFT launcher.'
    }
    Write-Output 'PASS Reinstall repairs the exact leftover directory from 0.30.3.'

    $userFile = Join-Path $installed 'tmp/user.txt'
    [IO.File]::WriteAllText($userFile, 'Keep this file')
    Remove-MafftDirectory $installed $root
    [IO.Compression.ZipFile]::ExtractToDirectory($archivePath, $stage)
    $blocked = $false
    try {
        Publish-MafftDirectory $extracted $installed $root
    } catch {
        if ($_.Exception.Message -notmatch 'contains existing files or folders') { throw }
        $blocked = $true
    }
    if (!$blocked -or [IO.File]::ReadAllText($userFile) -ne 'Keep this file') {
        throw 'Existing files must survive uninstall and attempted reinstall.'
    }
    Write-Output 'PASS Uninstall and attempted reinstall preserve user files inside tmp.'
} finally {
    if ([IO.Directory]::Exists($root)) { [IO.Directory]::Delete($root, $true) }
}
