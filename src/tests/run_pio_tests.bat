@echo off
rem run_pio_tests.bat - Build with PlatformIO (native) and run produced executable.
setlocal enabledelayedexpansion
pushd "%~dp0"
echo Building with PlatformIO (native)...
platformio run -e native || (
  echo PlatformIO build failed or PlatformIO not found. & popd & exit /b 1
)
rem Find the produced executable in .pio\build\native
set "EXE="
for /f "delims=" %%F in ('dir /b /a:-d ".pio\build\native\*.exe" 2^>nul') do (
  set "EXE=.pio\build\native\%%F"
  goto :found
)
if "%EXE%"=="" (
  echo No executable found in .pio\build\native. & popd & exit /b 1
)
:found
echo Running !EXE! ...
call "!EXE!"
set rc=!errorlevel!
popd
exit /b %rc%
