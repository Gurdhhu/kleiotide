# GPL-3.0-only
function Assert-OrdinaryPath([string]$Path, [string]$Root) {
    while ($Path -and $Path -ne $Root) {
        $item = Get-Item -LiteralPath $Path -Force -ErrorAction SilentlyContinue
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Refusing to change a redirected path: $Path"
        }
        $Path = [IO.Path]::GetDirectoryName($Path)
    }
}

function Remove-EmptyToolDirectory([string]$Path, [string]$Root) {
    Assert-OrdinaryPath $Path $Root
    if ([IO.Directory]::Exists($Path) -and
        !(Get-ChildItem -LiteralPath $Path -Force | Select-Object -First 1)) {
        [IO.Directory]::Delete($Path)
    }
}

function Publish-MafftDirectory([string]$Source, [string]$Destination, [string]$Root) {
    Assert-OrdinaryPath $Destination $Root
    if ([IO.Directory]::Exists($Destination)) {
        # Repair a 0.30.3 uninstall that left the empty tmp directory.
        $leftovers = @(Get-ChildItem -LiteralPath $Destination -Force)
        if ($leftovers.Count -eq 1 -and $leftovers[0].Name -eq 'tmp' -and $leftovers[0].PSIsContainer) {
            Remove-EmptyToolDirectory $leftovers[0].FullName $Root
        }
        Remove-EmptyToolDirectory $Destination $Root
    }
    if (Test-Path -LiteralPath $Destination) {
        throw "MAFFT cannot be installed at $Destination because the path contains existing files or folders."
    }
    [IO.Directory]::Move($Source, $Destination)
}

function Remove-MafftDirectory([string]$Folder, [string]$Root) {
    Assert-OrdinaryPath $Folder $Root
    $directories = @{}
    $directories[$Folder] = $true
    # The archive includes an empty tmp directory, so it has no file entry.
    $directories[(Join-Path $Folder 'tmp')] = $true
    foreach ($entry in (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'mafft-7.526-files.txt'))) {
        $file = Join-Path $Folder $entry
        Assert-OrdinaryPath $file $Root
        if ([IO.File]::Exists($file)) { [IO.File]::Delete($file) }
        $parent = [IO.Path]::GetDirectoryName($file)
        while ($parent -and $parent.Length -ge $Folder.Length) {
            $directories[$parent] = $true
            $parent = [IO.Path]::GetDirectoryName($parent)
        }
    }
    foreach ($dir in ($directories.Keys | Sort-Object Length -Descending)) {
        Remove-EmptyToolDirectory $dir $Root
    }
}
