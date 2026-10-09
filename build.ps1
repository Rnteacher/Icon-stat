$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$env:GEODE_SDK = Join-Path $ProjectRoot "vendor\geode"
$env:GEODE_BINDINGS_REPO_PATH = Join-Path $ProjectRoot "vendor\bindings"
$env:CPM_SOURCE_CACHE = Join-Path $ProjectRoot "vendor\cpm-cache"
$VsDevCmd = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if (-not (Test-Path -LiteralPath $VsDevCmd)) { throw "Visual Studio 2022 Build Tools were not found." }
if (-not (Test-Path -LiteralPath $env:GEODE_SDK)) { throw "Run .\scripts\bootstrap.ps1 first." }

$Command = "`"$VsDevCmd`" && cmake -S `"$ProjectRoot\mod`" -B `"$ProjectRoot\mod\build-release`" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DGEODE_DISABLE_PRECOMPILED_HEADERS=ON -DGEODE_DONT_INSTALL_MODS=ON && cmake --build `"$ProjectRoot\mod\build-release`" -- -j1"
& cmd.exe /c $Command
if ($LASTEXITCODE -ne 0) { throw "Mod build failed." }

$Artifact = Join-Path $ProjectRoot "mod\build-release\axolodlaat.icon-stat.geode"
if (-not (Test-Path -LiteralPath $Artifact)) { throw "Expected .geode artifact was not produced." }
Write-Host "Built $Artifact"
