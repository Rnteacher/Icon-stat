$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Vendor = Join-Path $ProjectRoot "vendor"
New-Item -ItemType Directory -Force -Path $Vendor | Out-Null

function Clone-IfMissing([string]$Url, [string]$Path, [string]$Ref = "") {
    if (Test-Path -LiteralPath $Path) { return }
    if ($Ref) { git clone --depth 1 --branch $Ref $Url $Path }
    else { git clone --depth 1 $Url $Path }
    if ($LASTEXITCODE -ne 0) { throw "Failed to clone $Url" }
}

Clone-IfMissing "https://github.com/geode-sdk/geode.git" (Join-Path $Vendor "geode") "v5.10.1"
Clone-IfMissing "https://github.com/geode-sdk/bindings.git" (Join-Path $Vendor "bindings")
Clone-IfMissing "https://github.com/GlobedGD/argon.git" (Join-Path $Vendor "argon") "v1.5.1"

$env:GEODE_SDK = Join-Path $Vendor "geode"
geode sdk install-binaries -p win -v 5.10.1
if ($LASTEXITCODE -ne 0) { throw "Failed to install Geode SDK binaries." }

Push-Location (Join-Path $ProjectRoot "backend")
try { pnpm install --frozen-lockfile } finally { Pop-Location }
