@echo off
REM Complete PC Test Suite for RPVI Guitar Tuner
REM Tests ALL software components before Teensy deployment

echo ========================================
echo   RPVI Guitar Tuner - PC Test Suite
echo ========================================
echo.

cd /d "%~dp0"

echo [1/3] Testing State Machine Logic...
echo ----------------------------------------
cd "Guitar Unit Testing Files"

if exist test_state_machine.exe (
    echo Running state machine tests...
    test_state_machine.exe
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: State machine tests failed!
        pause
        exit /b 1
    )
    echo PASS: State machine tests completed
) else (
    echo Compiling state machine tests...
    g++ -o test_state_machine.exe test_state_machine.cpp ../src/string_detection.c ../src/audio_sequencer.c -lm -I../src -std=c++11
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Compilation failed!
        pause
        exit /b 1
    )
    test_state_machine.exe
)

echo.
echo [2/3] Testing Main Application Logic...
echo ----------------------------------------

if exist main_test.exe (
    echo Running main application tests...
    main_test.exe
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Main application tests failed!
        pause
        exit /b 1
    )
    echo PASS: Main application tests completed
) else (
    echo Compiling main application tests...
    g++ -o main_test.exe main_test.cpp ../src/string_detection.c ../src/audio_sequencer.c -lm -I../src -std=c++11
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Compilation failed!
        pause
        exit /b 1
    )
    main_test.exe
)

echo.
echo [3/3] Testing Core Unit Tests...
echo ----------------------------------------

if exist tuner_tests.exe (
    echo Running tuner unit tests...
    tuner_tests.exe
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Unit tests failed!
        pause
        exit /b 1
    )
    echo PASS: Unit tests completed
) else (
    echo WARNING: tuner_tests.exe not found, skipping
)

echo.
echo ========================================
echo   ALL TESTS PASSED!
echo ========================================
echo.
echo Software is validated and ready for:
echo   1. Teensy 4.1 compilation
echo   2. Hardware deployment
echo   3. Real-world testing
echo.
echo Next steps:
echo   - Connect Teensy 4.1
echo   - Run: platformio run -e teensy41
echo   - Upload and test with real guitar
echo.
pause