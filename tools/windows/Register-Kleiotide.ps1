# Optional, per-user Open With registration. Does not change existing defaults.
param([Parameter(Mandatory=$true)][string]$Executable)
$ErrorActionPreference = 'Stop'
$resolved = (Get-Item -LiteralPath $Executable).FullName
if ([IO.Path]::GetFileName($resolved) -ine 'kleiotide.exe') { throw 'Choose the built kleiotide.exe.' }
$root = 'HKCU:\Software\Classes'
$progId = "$root\Kleiotide.File"
New-Item -Path "$progId\shell\open\command" -Force | Out-Null
Set-Item -Path $progId -Value 'Kleiotide alignment, trace or session'
Set-Item -Path "$progId\shell\open\command" -Value ('"{0}" "%1"' -f $resolved)
New-ItemProperty -Path "$progId\shell\open" -Name MultiSelectModel -Value Player -PropertyType String -Force | Out-Null
$app = "$root\Applications\kleiotide.exe"
New-Item -Path "$app\shell\open\command" -Force | Out-Null
Set-Item -Path "$app\shell\open\command" -Value ('"{0}" "%1"' -f $resolved)
New-ItemProperty -Path $app -Name FriendlyAppName -Value Kleiotide -PropertyType String -Force | Out-Null
New-ItemProperty -Path "$app\shell\open" -Name MultiSelectModel -Value Player -PropertyType String -Force | Out-Null
New-Item -Path "$app\SupportedTypes" -Force | Out-Null
foreach ($extension in '.ab1','.abi','.kleio','.fasta','.fas','.fa','.fna','.ffn','.faa','.frn','.fsa') {
    New-Item -Path "$root\$extension\OpenWithProgids" -Force | Out-Null
    New-ItemProperty -Path "$root\$extension\OpenWithProgids" -Name Kleiotide.File -Value '' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path "$app\SupportedTypes" -Name $extension -Value '' -PropertyType String -Force | Out-Null
}
# Tell Explorer to refresh cached Open With commands.
Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public static class KleiotideShellNotify { [DllImport("shell32.dll")] public static extern void SHChangeNotify(uint eventId, uint flags, IntPtr item1, IntPtr item2); }'
[KleiotideShellNotify]::SHChangeNotify(0x08000000,0,[IntPtr]::Zero,[IntPtr]::Zero)
Write-Host 'Registered for the current user. Existing default applications are unchanged.'
Write-Host 'Select files in Explorer and choose Open with -> Kleiotide. Use File -> Open for larger batches.'
