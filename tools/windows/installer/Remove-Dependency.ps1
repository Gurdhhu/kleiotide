# GPL-3.0-only
param([Parameter(Mandatory)][ValidateSet('MAFFT','BLAST')][string]$Tool,
      [Parameter(Mandatory)][string]$ProgramFiles64)
$ErrorActionPreference = 'Stop'
$machine = [Microsoft.Win32.RegistryKey]::OpenBaseKey('LocalMachine','Registry64')
. (Join-Path $PSScriptRoot 'Mafft-Files.ps1')
try {
    $key = $machine.OpenSubKey('SOFTWARE\Kleiotide\Tools', $true)
    if (!$key) { throw 'No setup-managed tools are registered.' }
    if ($Tool -eq 'MAFFT') {
        $name = 'MafftDirectory'
        $folder = Join-Path $ProgramFiles64 'Kleiotide Tools\MAFFT-7.526'
    } else {
        $name = 'BlastDirectory'
        $root = Join-Path $ProgramFiles64 'NCBI\blast-2.17.0+'
        $folder = Join-Path $root 'bin'
    }
    if ($key.GetValue("Owned$Tool") -ne $folder) { throw 'This copy is not registered as installed by setup.' }
    Assert-OrdinaryPath $folder $ProgramFiles64
    if ($Tool -eq 'MAFFT') {
        Remove-MafftDirectory $folder $ProgramFiles64
    } else {
        $uninstaller = Join-Path $root 'Uninstall-ncbi-blast-2.17.0+.exe'
        Assert-OrdinaryPath $uninstaller $ProgramFiles64
        if ((Get-FileHash -LiteralPath $uninstaller).Hash -ne $key.GetValue('BlastUninstallerSHA256')) {
            throw 'The registered NCBI uninstaller has changed. Use Windows Installed apps to remove this version.'
        }
        $process = Start-Process -FilePath $uninstaller -ArgumentList "/S _?=$root" -Wait -PassThru
        if ($process.ExitCode -ne 0 -or [IO.File]::Exists((Join-Path $folder 'blastn.exe'))) {
            throw 'The NCBI uninstaller did not finish removing BLAST+.'
        }
        [IO.File]::Delete($uninstaller)
        if ([IO.Directory]::Exists($root) -and !(Get-ChildItem -LiteralPath $root -Force | Select-Object -First 1)) {
            [IO.Directory]::Delete($root)
        }
        $key.DeleteValue('BlastUninstallerSHA256', $false)
    }
    if ($key.GetValue($name) -eq $folder) { $key.DeleteValue($name, $false) }
    $key.DeleteValue("Owned$Tool", $false)
} finally {
    if ($key) { $key.Dispose() }
    $machine.Dispose()
}
