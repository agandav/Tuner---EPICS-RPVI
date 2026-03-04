@echo off
:: run_pio_tests_from_anywhere.bat - Run the PlatformIO test runner from any current directory
:: It resolves the script's directory and runs run_pio_tests.bat from the repository root.

SETLOCAL
:: Resolve script directory
SET "SCRIPT_DIR=%~dp0"
:: Change to script dir (repo root)
PUSHD "%SCRIPT_DIR%"
CALL run_pio_tests.bat
POPd
ENDLOCAL
