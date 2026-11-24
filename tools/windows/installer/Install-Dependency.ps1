# GPL-3.0-only; copyright 2026 Oleg Shchepin.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('MAFFT','BLAST')][string]$Tool,
    [Parameter(Mandatory)][string]$ProgramFiles64
)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
. (Join-Path $PSScriptRoot 'Mafft-Files.ps1')
$toolsRoot = Join-Path $ProgramFiles64 'Kleiotide Tools'
$stage = $null
$machine = $null
$key = $null

function Test-BlastSuite([string]$Directory) {
    if (!$Directory) { return $false }
    foreach ($name in 'blastn','blastp','blastx','tblastn','tblastx','makeblastdb') {
        if (!(Test-Path -LiteralPath (Join-Path $Directory "$name.exe") -PathType Leaf)) { return $false }
    }
    return $true
}

function Test-MafftSuite([string]$Directory) {
    if (!$Directory) { return $false }
    foreach ($name in 'mafft.bat','usr/bin/bash.exe','usr/bin/mafft','usr/lib/mafft/disttbfast.exe') {
        if (!(Test-Path -LiteralPath (Join-Path $Directory $name) -PathType Leaf)) { return $false }
    }
    return $true
}

try {
    $machine = [Microsoft.Win32.RegistryKey]::OpenBaseKey('LocalMachine', 'Registry64')
    $key = $machine.CreateSubKey('SOFTWARE\Kleiotide\Tools')
    if ($Tool -eq 'MAFFT') {
        $valueName = 'MafftDirectory'
        $directory = Join-Path $toolsRoot 'MAFFT-7.526'
        $uri = 'https://mafft.cbrc.jp/alignment/software/mafft-7.526-win64-signed.zip'
        $hash = '7c3a508c8512af04007d68019c6fbd4ff00d891244b58f87ec9aa0e53fbae8a7'
        $filename = 'mafft.zip'
        $stored = $key.GetValue($valueName)
        if (Test-MafftSuite $stored) { $directory = $stored }
        $ready = Test-MafftSuite $directory
    } else {
        $valueName = 'BlastDirectory'
        $ncbi = Join-Path $ProgramFiles64 'NCBI'
        $installRoot = Join-Path $ncbi 'blast-2.17.0+'
        $directory = Join-Path $installRoot 'bin'
        $uri = 'https://ftp.ncbi.nlm.nih.gov/blast/executables/blast+/2.17.0/ncbi-blast-2.17.0+-win64.exe'
        $hash = '8b670655a446f172983567029c99702c5fb75919acb26c2cdddaa01bc930134e'
        $filename = 'blast-setup.exe'
        $stored = $key.GetValue($valueName)
        if (Test-BlastSuite $stored) { $directory = $stored }
        if (!(Test-BlastSuite $directory) -and (Test-Path -LiteralPath $ncbi)) {
            foreach ($folder in (Get-ChildItem -LiteralPath $ncbi -Directory -Filter 'blast-*' | Sort-Object LastWriteTime -Descending)) {
                $candidate = Join-Path $folder.FullName 'bin'
                if (Test-BlastSuite $candidate) { $directory = $candidate; break }
            }
        }
        $ready = Test-BlastSuite $directory
    }
    if (!$ready) {
        Assert-OrdinaryPath $toolsRoot $ProgramFiles64
        [void][IO.Directory]::CreateDirectory($toolsRoot)
        $stage = Join-Path $toolsRoot ([Guid]::NewGuid().ToString())
        [void][IO.Directory]::CreateDirectory($stage)
        $download = Join-Path $stage $filename
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Write-Output "Downloading $Tool from $uri"
        Invoke-WebRequest -UseBasicParsing -Uri $uri -OutFile $download -TimeoutSec 300
        if ((Get-FileHash -LiteralPath $download -Algorithm SHA256).Hash -ne $hash) {
            throw "The downloaded $Tool package differs from the verified release."
        }
        if ($Tool -eq 'MAFFT') {
            Write-Output 'Extracting MAFFT...'
            Add-Type -AssemblyName System.IO.Compression.FileSystem
            [IO.Compression.ZipFile]::ExtractToDirectory($download, $stage)
            $extracted = Join-Path $stage 'mafft-win'
            if (!(Test-MafftSuite $extracted)) { throw 'The archive has an incomplete MAFFT installation.' }
            Publish-MafftDirectory $extracted $directory $ProgramFiles64
        } else {
            Write-Output 'Running the NCBI BLAST+ installer...'
            # NSIS requires /D= as its final, unquoted argument, even for paths with spaces.
            $result = Start-Process -FilePath $download -ArgumentList "/S /D=$installRoot" -WorkingDirectory $stage -PassThru -Wait
            if ($result.ExitCode -ne 0) { throw "NCBI installer returned $($result.ExitCode)." }
            if (!(Test-BlastSuite $directory)) { throw "Could not find the six BLAST+ tools in $directory." }
        }
        if ($Tool -eq 'BLAST') {
            $uninstaller = Join-Path $installRoot 'Uninstall-ncbi-blast-2.17.0+.exe'
            $key.SetValue('BlastUninstallerSHA256', (Get-FileHash -LiteralPath $uninstaller).Hash)
        }
        $key.SetValue("Owned$Tool", $directory)
    }
    $key.SetValue($valueName, $directory)
    Write-Output "$Tool is available in $directory"
} catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
} finally {
    if ($key) { $key.Dispose() }
    if ($machine) { $machine.Dispose() }
    if ($stage -and (Test-Path -LiteralPath $stage)) { Remove-Item -LiteralPath $stage -Recurse -Force }
}
