# Tuner - EPICS RPVI (Guitar Tuner)

Software for Purdue Engineering Projects in Community Service - Resources for People with Visual Impairments (Tuner team). A guitar tuner application that detects strings, calculates tuning accuracy in cents, and provides audio feedback.

---

## Source Files Overview

### Core Audio Processing (`src/`)

| File | Purpose |
|------|---------|
| `audio_processing.c` | Implements FFT-based frequency detection from audio samples for pitch analysis. |
| `string_detection.c` | Identifies which guitar string is being played and calculates cents offset from target frequency. |
| `audio_sequencer.c` | Generates audio feedback sequences (note names, cent values, tuning direction). |
| `teensy_audio_io.h/cpp` | Platform-independent audio I/O interface with abstracted hardware operations. |
| `tuner_main.c` | Main entry point for the tuner application. |

### Utility/Testing Files

| File | Purpose |
|------|---------|
| `tuner_tests.c` | Comprehensive test suite validating string detection, cent calculations, and audio sequencing. |
| `wrapper.c` | Stub file for PlatformIO integration. |
| `*_impl.c` | Stub implementations to avoid duplicate symbols during linking. |
| `buffer_manager.cpp`, `signal_processing.cpp`, `fft_processor.cpp`, `config.h` | Reserved for future expansion. |

---

## CMSIS-DSP Integration

**CMSIS-DSP** (ARM Cortex Microcontroller Software Interface Standard - Digital Signal Processing) is an optimized DSP library for ARM Cortex-M processors. This project uses CMSIS-DSP to provide efficient, hardware-optimized implementations of FFT and other signal processing algorithms on Teensy 4.1 and similar ARM microcontrollers. The library significantly reduces computation overhead for real-time audio analysis.

### CMSIS-DSP Test Suite (`CMSIS-DSP-Tests/`)

| File | Purpose |
|------|---------|
| `cmsis_dsp_tests.h` | Defines test infrastructure and test functions for CMSIS-DSP operations. |
| `cmsis_dsp_tests.c` | Core test implementations for FFT, filtering, and other DSP functions. |
| `cmsis_dsp_native_main.c` | Test runner for native (Windows/Linux) platform using PlatformIO. |
| `cmsis_dsp_teensy_bare_metal.c` | Test runner for Teensy 4.1 bare-metal environment. |
| `cmsis_dsp_test_utilities.c` | Helper functions for test execution and result reporting. |

---

## Running Tests

### Tuner Test Suite (Guitar Tuning Algorithms) - PC
```cmd
cd "C:\Users\User\OneDrive - purdue.edu\Desktop\EPCS 41200\Tuner---EPICS-RPVI\Guitar Unit Testing Files"
.\tuner_tests.exe
```
Tests: string detection, cents calculation, tuning direction, audio sequencing.

### CMSIS-DSP Test Suite (Signal Processing) - PC
```cmd
cd "C:\Users\User\OneDrive - purdue.edu\Desktop\EPCS 41200\Tuner---EPICS-RPVI"
platformio run -e native
.\.pio\build\native\program.exe
```
Tests: FFT operations, filtering, and ARM DSP library functions on native platform.

### CMSIS-DSP Test Suite on Teensy 4.1 (Plugged In)
```cmd
cd "C:\Users\User\OneDrive - purdue.edu\Desktop\EPCS 41200\Tuner---EPICS-RPVI\CMSIS-DSP-Tests"
run_cmsis_dsp_tests_teensy.bat
```
This builds, uploads firmware to your Teensy 4.1, opens a serial monitor, and runs tests in real-time. Press ENTER in the serial monitor to start the test suite.

### Prerequisites
- PlatformIO Core (CLI): https://platformio.org/install/cli
- Teensy 4.1 board (for Teensy tests)

---

## Senior Design Implementation (Fall 2025 - Spring 2026)

- Get user input for which string they are tuning
- Map string to expected note with chromatic variations
  - Low E string: F, F#, G, G#, A, A#, B, C, C#, D, D#, E
  - A string: A#, B, C, C#, D, D#, E, F, F#, G, G#, A
  - D string: D#, E, F, F#, G, G#, A, A#, B, C, C#, D
  - G string: G#, A, A#, B, C, C#, D, D#, E, F, F#, G
  - B string: C, C#, D, D#, E, F, F#, G, G#, A, A#, B
  - High E string: F, F#, G, G#, A, A#, B, C, C#, D, D#, E
- Calculate tuning directions based on cents offset
  - Negative = flat = tune up
  - Positive = sharp = tune down
- Play audio feedback sequence (string name, cent offset, direction)

---

## Software Architecture

![Flowchart](img/Software%20Flowchart.drawio.png "Software Flowchart")
