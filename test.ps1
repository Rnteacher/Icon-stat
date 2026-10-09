$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Push-Location (Join-Path $ProjectRoot "backend")
try {
    & .\node_modules\.bin\tsc.CMD --noEmit
    if ($LASTEXITCODE -ne 0) { throw "Backend typecheck failed." }
    & .\node_modules\.bin\vitest.CMD run --maxWorkers=1 --minWorkers=1
    if ($LASTEXITCODE -ne 0) { throw "Backend tests failed." }
} finally { Pop-Location }
