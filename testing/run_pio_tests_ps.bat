@echo off
:: run_pio_tests_ps.bat - Shim to invoke the PowerShell runner from any location
SETLOCAL
SET "SCRIPT_DIR=%~dp0"
PUSHD "%SCRIPT_DIR%"
:: Use PowerShell to run the PS wrapper with ExecutionPolicy Bypass
powershell -NoProfile -ExecutionPolicy Bypass -Command "& '.\run_pio_tests.ps1'"
POPd
ENDLOCAL
