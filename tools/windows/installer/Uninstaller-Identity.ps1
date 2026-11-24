# GPL-3.0-only
param([ValidateSet('Record','Stage')]$Mode, [string]$RegistryPath,
      [string]$TrustedCopy, [string]$InstalledCopy)
$ErrorActionPreference = 'Stop'
$machine = [Microsoft.Win32.RegistryKey]::OpenBaseKey('LocalMachine','Registry64')
try {
    $key = $machine.CreateSubKey($RegistryPath)
    if ($Mode -eq 'Stage') {
        Copy-Item -LiteralPath $InstalledCopy -Destination $TrustedCopy -Force
        if ((Get-FileHash -LiteralPath $TrustedCopy).Hash -ne $key.GetValue('UninstallerSHA256')) {
            throw 'The installed uninstaller differs from the registered copy.'
        }
    } else {
        $key.SetValue('UninstallerSHA256', (Get-FileHash -LiteralPath $TrustedCopy).Hash)
    }
} finally {
    if ($key) { $key.Dispose() }
    $machine.Dispose()
}
