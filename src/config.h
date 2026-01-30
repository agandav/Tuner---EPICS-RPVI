/**
 * config.h - Hardware configuration for Teensy 4.1 Guitar Tuner
 *
 * Defines GPIO pins, audio configuration, and hardware-specific constants
 * for the Teensy 4.1 microcontroller implementation.
 * 
 * January 2025 - Audio system using I2S (Teensy Audio Library)
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * STRING SELECTION BUTTONS (6 buttons for 6 guitar strings)
 * Maps guitar strings to physical tactile buttons with braille labels
 * Active LOW with internal pull-ups (buttons connect to GND)
 * ========================================================================== */

#define STRING_1_BUTTON_PIN 0   // E4 string (high E, 329.63 Hz)
#define STRING_2_BUTTON_PIN 1   // B3 string (246.94 Hz)
#define STRING_3_BUTTON_PIN 2   // G3 string (196.00 Hz)
#define STRING_4_BUTTON_PIN 3   // D3 string (146.83 Hz)
#define STRING_5_BUTTON_PIN 4   // A2 string (110.00 Hz)
#define STRING_6_BUTTON_PIN 5   // E2 string (low E, 82.41 Hz)
/* Button logic: Pressed = LOW (0V), Released = HIGH (3.3V via internal pull-up) */

/* ============================================================================
 * MODE SWITCH (Play Tone I vs Listen Only O)
 * Physical toggle switch on device to select operating mode
 * ========================================================================== */

#define MODE_SWITCH_PIN 6       // Mode I/O switch

// Switch logic: HIGH = Play Tone (Mode I), LOW = Listen Only (Mode O)
// Uses internal pull-up resistor

/* ============================================================================
 * AUDIO AMPLIFIER CONTROL
 * PAM8302AAS audio amplifier enable/shutdown control
 * ========================================================================== */

#define AUDIO_AMP_ENABLE_PIN 23 // PAM8302AAS shutdown control

// Amplifier logic: HIGH = Amp ON, LOW = Amp OFF (low power)

/* ============================================================================
 * AUDIO HARDWARE - TEENSY AUDIO LIBRARY (I2S)
 * 
 * UPDATED: Using Teensy Audio Library with I2S interface
 * NOT using external SGTL5000 codec - using Teensy's built-in I2S + DAC
 * 
 * Audio Output Path:
 *   AudioSynthWaveformSine → AudioOutputI2S → I2S pins → Internal DAC → 
 *   PAM8302AAS Amplifier → Speaker
 * 
 * THESE PINS ARE FIXED BY TEENSY 4.1 HARDWARE (cannot be changed)
 * ========================================================================== */

/* I2S Audio Output Pins (FIXED - Teensy 4.1 hardware) */
#define AUDIO_I2S_BCLK_PIN  21  // Bit Clock (BCLK) - FIXED
#define AUDIO_I2S_LRCLK_PIN 20  // Left/Right Clock (LRCLK / Word Select) - FIXED
#define AUDIO_I2S_OUT_PIN   7   // Serial Data Out (TX) - FIXED
#define AUDIO_I2S_IN_PIN    8   // Serial Data In (RX) - for microphone - FIXED

/* These pins are hardwired in the Teensy 4.1 and cannot be reassigned */

/* ============================================================================
 * AUDIO CONFIGURATION - TEENSY AUDIO LIBRARY
 * Settings for AudioSynthWaveformSine and AudioOutputI2S
 * ========================================================================== */

#define AUDIO_SAMPLE_RATE       44100   // 44.1 kHz (CD quality)
#define AUDIO_BLOCK_SIZE        128     // Samples per audio block (~2.9ms at 44.1kHz)
#define AUDIO_BIT_DEPTH         16      // 16-bit audio
#define AUDIO_MEMORY_BLOCKS     20      // Audio memory allocation (adjustable)

/* Volume control parameters */
#define VOLUME_MIN              0.0f    // Silent
#define VOLUME_MAX              1.0f    // Full volume (100%)
#define VOLUME_DEFAULT          0.7f    // Default startup volume (70%)

/* Tone playback parameters */
#define TONE_AMPLITUDE_DEFAULT  0.7f    // Default tone volume (70%)
#define BEEP_AMPLITUDE_DEFAULT  0.5f    // Default beep volume (50%)

/* ============================================================================
 * MICROPHONE INPUT
 * 
 * UPDATED: Using I2S audio input (AudioInputI2S)
 * Can also use ADC input if microphone connects to analog pin
 * ========================================================================== */

// Option 1: I2S Microphone Input (if using I2S mic)
// Uses AUDIO_I2S_IN_PIN (pin 8) automatically via AudioInputI2S

// Option 2: ADC Microphone Input (if using MAX4466 → ADC)
#define MICROPHONE_INPUT_PIN A0         // ADC pin for microphone - CONFIRMED
#define MICROPHONE_SAMPLE_RATE 44100    // 44.1 kHz sampling rate
#define MICROPHONE_GAIN 36              // dB gain (adjustable in software)

/* ADC configuration for microphone input */
#define ADC_RESOLUTION 12               // 12-bit ADC (0-4095)
#define ADC_AVERAGING 4                 // Average 4 samples per reading (noise reduction)
#define MIN_AMPLITUDE 100               // Minimum signal amplitude to process (0-4095 scale)

/* ============================================================================
 * VOLUME CONTROL (Optional - if present on hardware)
 * Option 1: Rotary potentiometer for analog volume adjustment
 * Option 2: Rotary encoder for digital volume control
 * ========================================================================== */

#define VOLUME_POTENTIOMETER_PIN 15     // ADC A1 (Pin 15) for volume knob (analog)
#define VOLUME_ADC_RESOLUTION 12        // 12-bit ADC = 4096 levels
#define VOLUME_UPDATE_INTERVAL_MS 50    // Update volume every 50ms

/* Rotary Encoder pins (if using encoder instead of potentiometer) */
#define ROTARY_ENCODER_CLK_PIN 9        // Encoder CLK (A phase)
#define ROTARY_ENCODER_DT_PIN 10        // Encoder DT (B phase)
#define ROTARY_ENCODER_SW_PIN 11        // Encoder push button (switch)

/* ============================================================================
 * DIGITAL SIGNAL PROCESSING (FFT)
 * Configuration for frequency detection using FFT
 * ========================================================================== */

#define FFT_SIZE                256     // 256-point FFT
#define FFT_INPUT_SAMPLE_RATE   10000   // Downsampled to 10kHz for guitar tuning
#define FFT_HZ_PER_BIN          (FFT_INPUT_SAMPLE_RATE / FFT_SIZE)  // ~39 Hz resolution

/* Frequency detection range (Hz) - covers all guitar strings */
#define MIN_DETECTABLE_FREQ     50.0f   // Below low E (82.41 Hz)
#define MAX_DETECTABLE_FREQ     400.0f  // Well above high E (329.63 Hz)

/* Audio buffer sizes */
#define AUDIO_BUFFER_SIZE       4096    // 4K sample buffer
#define FFT_BUFFER_SIZE         (FFT_SIZE * sizeof(float))
#define SAMPLE_SIZE             1024    // Number of samples to collect

/* ============================================================================
 * TUNING PARAMETERS
 * Thresholds and tolerances for tuning accuracy
 * ========================================================================== */

#define TUNING_TOLERANCE_CENTS  2.0     // within 2 cents considered "in tune"
#define CENTS_THRESHOLD_WARN    10.0    // Warn if > 10 cents off
#define CENTS_THRESHOLD_CRITICAL 50.0   // Critical if > 50 cents off

/* ============================================================================
 * TEENSY HARDWARE SPECIFICS
 * ========================================================================== */

#define TEENSY_VERSION          41      // Teensy 4.1
#define TEENSY_CLOCK_SPEED      600     // MHz
#define TEENSY_RAM_KB           1024    // 1 MB RAM available

/* ============================================================================
 * DEBUG & DIAGNOSTICS
 * ========================================================================== */

#define ENABLE_DEBUG_PRINTS     1       // Set to 0 to disable console output
#define SERIAL_BAUD_RATE        115200  // USB serial monitor speed
#define DEBUG_FFT_OUTPUT        0       // Print FFT magnitude spectrum (very verbose!)
#define DEBUG_TUNING_RESULTS    1       // Print tuning analysis results

/* ============================================================================
 * BATTERY MANAGEMENT (Battery powered device)
 * ========================================================================== */

// Uncomment if battery monitoring is implemented:
// #define BATTERY_VOLTAGE_PIN A2          // ADC pin for battery voltage monitoring
// #define BATTERY_LOW_THRESHOLD 3.3f      // Voltage threshold for low battery warning

/* ============================================================================
 * NO SD CARD / NO WAV FILES
 * 
 * This implementation uses REAL-TIME AUDIO SYNTHESIS via Teensy Audio Library
 * All audio is generated mathematically using AudioSynthWaveformSine
 * ========================================================================== */

// #define USE_SD_CARD  // NOT USED - Keeping for reference only

#ifdef USE_SD_CARD
#define SD_CHIP_SELECT_PIN BUILTIN_SDCARD   // Teensy 4.1 built-in SD card
#define AUDIO_FILES_PATH "/AUDIO/"

/* Pre-recorded audio file names (NOT USED - for reference only) */
#define AUDIO_FILE_E_STRING     "/AUDIO/STRING_E.wav"
#define AUDIO_FILE_B_STRING     "/AUDIO/STRING_B.wav"
#define AUDIO_FILE_G_STRING     "/AUDIO/STRING_G.wav"
#define AUDIO_FILE_D_STRING     "/AUDIO/STRING_D.wav"
#define AUDIO_FILE_A_STRING     "/AUDIO/STRING_A.wav"
#define AUDIO_FILE_10_CENTS     "/AUDIO/CENTS_10.wav"
#define AUDIO_FILE_20_CENTS     "/AUDIO/CENTS_20.wav"
#define AUDIO_FILE_TUNE_UP      "/AUDIO/TUNE_UP.wav"
#define AUDIO_FILE_TUNE_DOWN    "/AUDIO/TUNE_DOWN.wav"
#define AUDIO_FILE_IN_TUNE      "/AUDIO/IN_TUNE.wav"
#endif

/* ============================================================================
 * PIN USAGE SUMMARY - UPDATED FOR I2S AUDIO
 * Quick reference for all pin assignments
 * ========================================================================== */

/*
 * DIGITAL INPUTS (with internal pull-ups):
 *   Pin 0  - Button 1 (String E4)
 *   Pin 1  - Button 2 (String B3)
 *   Pin 2  - Button 3 (String G3)
 *   Pin 3  - Button 4 (String D3)
 *   Pin 4  - Button 5 (String A2)
 *   Pin 5  - Button 6 (String E2)
 *   Pin 6  - Mode Switch (I/O)
 *
 * DIGITAL OUTPUTS:
 *   Pin 23 - Audio Amp Enable (PAM8302AAS)
 *
 * ROTARY ENCODER (Optional - for volume control):
 *   Pin 9  - Encoder CLK (A phase)
 *   Pin 10 - Encoder DT (B phase)
 *   Pin 11 - Encoder push button
 *
 * I2S AUDIO (FIXED - Teensy 4.1 hardware):
 *   Pin 7  - I2S Data Out (audio to amplifier)
 *   Pin 8  - I2S Data In (microphone input, if using I2S mic)
 *   Pin 20 - I2S LRCLK (Left/Right Clock)
 *   Pin 21 - I2S BCLK (Bit Clock)
 *
 * ANALOG INPUTS:
 *   A0 (Pin 14) - Microphone (MAX4466 output, if using ADC)
 *   A1 (Pin 15) - Volume Potentiometer (if present)
 *
 * ROTARY ENCODER (Optional - alternative to potentiometer):
 *   Pin 9  - Encoder CLK (A phase)
 *   Pin 10 - Encoder DT (B phase)  
 *   Pin 11 - Encoder push button
 *
 * PINS TO AVOID (reserved for future use or special functions):
 *   Pin 13     - Onboard LED (avoid for critical functions)
 *   Pins 18-19 - I2C (reserved if needed)
 *   Pins 11-13 - SPI (reserved if SD card added)
 *
 * AUDIO ARCHITECTURE:
 *   Software: Teensy Audio Library
 *   Synthesis: AudioSynthWaveformSine (real-time sine wave generation)
 *   Output: AudioOutputI2S → I2S pins → Internal DAC → PAM8302AAS → Speaker
 *   Input: AudioInputI2S or ADC → FFT → Frequency detection
 *   
 *   NO SGTL5000 codec
 *   NO SD card
 *   NO WAV files
 *   Pure real-time synthesis
 */

/* ============================================================================
 * CFUGUE-STYLE NOTE NOTATION SUPPORT
 * 
 * This configuration supports the new CFugue-style notation system:
 *   - parseNote("E2") → 82.41 Hz
 *   - playNoteTone("E4", 2000) → plays high E for 2 seconds
 *   - playGuitarString(1, 2000) → plays string 1 (E4)
 * 
 * All frequencies are calculated mathematically - no lookup tables needed
 * ========================================================================== */

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */