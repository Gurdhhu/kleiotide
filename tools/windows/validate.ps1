param(
    [Parameter(Mandatory=$true)][string]$QtRoot,
    [string]$BuildDir = (Join-Path $PSScriptRoot '../../build-release')
)
$ErrorActionPreference = 'Stop'
$sourceDir = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$qtPath = (Resolve-Path $QtRoot).Path
if (!(Test-Path (Join-Path $qtPath 'bin/qmake.exe'))) { throw 'QtRoot must be the MSVC Qt kit directory.' }
if (!(Get-Command cl.exe -ErrorAction SilentlyContinue)) { throw 'Run from the x64 Native Tools for VS 2022 environment.' }
$env:PATH = (Join-Path $qtPath 'bin') + ';' + $env:PATH
New-Item -ItemType Directory -Force $BuildDir | Out-Null
$BuildDir = (Resolve-Path $BuildDir).Path
function Run-Checked([string]$Log, [scriptblock]$Command) {
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $Command 2>&1 | Tee-Object -FilePath (Join-Path $BuildDir $Log)
        $exitCode = $LASTEXITCODE
    } finally { $ErrorActionPreference = $previousPreference }
    if ($exitCode -ne 0) { throw "Validation failed; see $Log" }
}
Run-Checked 'configure.log' { cmake -S $sourceDir -B $BuildDir -G Ninja "-DCMAKE_PREFIX_PATH=$qtPath" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DKLEIOTIDE_BUILD_RELEASE_CHECKS=ON }
Run-Checked 'build.log' { cmake --build $BuildDir --parallel 2 }
# CTest sets offscreen mode for registered GUI tests. Native dialogs are checked manually.
Run-Checked 'tests.log' { ctest --test-dir $BuildDir -C Release --output-on-failure --no-tests=error --output-junit (Join-Path $BuildDir 'tests.xml') }
@{
    date = (Get-Date).ToString('o')
    qtRoot = $qtPath
    compiler = (Get-Command cl.exe).Source
    result = 'Automated tests passed; native acceptance remains separate.'
} | ConvertTo-Json | Set-Content (Join-Path $BuildDir 'validation.json') -Encoding UTF8
Write-Host "Validation passed. Retain configure.log, build.log, tests.log, tests.xml and validation.json from $BuildDir"
