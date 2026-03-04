# run_pio_tests.ps1 - PowerShell wrapper to build and run the PlatformIO native test binary

Param(
    [switch]$NoRun
)

# Navigate to repository root (assume script is run from repo root or adjacent)
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $scriptDir

Write-Host "Building PlatformIO native environment..." -ForegroundColor Cyan
$p = Start-Process -FilePath "platformio" -ArgumentList 'run','-e','native','-v' -NoNewWindow -Wait -PassThru
if ($p.ExitCode -ne 0) {
    Write-Host "PlatformIO build failed with exit code $($p.ExitCode)." -ForegroundColor Red
    exit $p.ExitCode
}

if ($NoRun) {
    Write-Host "Build completed; skipping execution because -NoRun was provided." -ForegroundColor Yellow
    exit 0
}

$exe = Join-Path -Path ".pio\build\native" -ChildPath "program.exe"
if (-Not (Test-Path $exe)) {
    Write-Host "Cannot find built executable: $exe" -ForegroundColor Red
    exit 2
}

Write-Host "Running test binary: $exe" -ForegroundColor Cyan
& $exe

exit 0
