/**
 * config.h - Hardware configuration for Teensy 4.1 Guitar Tuner
 *
 * Defines GPIO pins, audio configuration, and hardware-specific constants
 * for the Teensy 4.1 microcontroller implementation.
 * 
 * IMPORTANT: Verify ALL pin assignments with electrical team before uploading!
 * Wrong pin configuration can damage hardware!
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
 * String 1 (E4 - high E) through String 6 (E2 - low E)
 * 
 * TODO: VERIFY THESE PIN NUMBERS WITH ELECTRICAL TEAM!
 * ========================================================================== */

#define STRING_1_BUTTON_PIN 2   // E4 string (high E) - VERIFY WITH EE TEAM
#define STRING_2_BUTTON_PIN 3   // B3 string - VERIFY WITH EE TEAM
#define STRING_3_BUTTON_PIN 4   // G3 string - VERIFY WITH EE TEAM
#define STRING_4_BUTTON_PIN 5   // D3 string - VERIFY WITH EE TEAM
#define STRING_5_BUTTON_PIN 6   // A2 string - VERIFY WITH EE TEAM
#define STRING_6_BUTTON_PIN 7   // E2 string (low E) - VERIFY WITH EE TEAM

/* ============================================================================
 * MODE SWITCH (Play Tone I vs Listen Only O)
 * Physical switch on device to select operating mode
 * 
 * TODO: VERIFY WITH ELECTRICAL TEAM!
 * - Is there a mode switch on the PCB?
 * - Which GPIO pin is it connected to?
 * - What is the logic? (HIGH = Play Tone or HIGH = Listen Only?)
 * ========================================================================== */

// Uncomment this line once electrical team confirms the pin number:
// #define MODE_SWITCH_PIN 8   // Mode I/O switch - VERIFY WITH EE TEAM

// Mode switch logic (verify with electrical team):
// If defined, assume: HIGH = Play Tone (Mode I), LOW = Listen Only (Mode O)
// If electrical team says opposite, we'll flip the logic in main.cpp

/* ============================================================================
 * VOLUME CONTROL (Optional - if present on hardware)
 * Rotary potentiometer or encoder for volume adjustment
 * 
 * TODO: VERIFY WITH ELECTRICAL TEAM!
 * ========================================================================== */

#define VOLUME_POTENTIOMETER_PIN 14     // ADC pin for volume knob - VERIFY WITH EE TEAM
#define VOLUME_ADC_RESOLUTION 12        // 12-bit ADC = 4096 levels
#define VOLUME_UPDATE_INTERVAL_MS 50    // Update volume every 50ms

/* Alternative: Rotary Encoder with Click (if using encoder instead of pot) */
#define ROTARY_ENCODER_CLK_PIN 15       // Rotation phase A
#define ROTARY_ENCODER_DT_PIN  16       // Rotation phase B
#define ROTARY_ENCODER_SW_PIN  17       // Push button (click)

/* ============================================================================
 * AUDIO HARDWARE
 * Teensy 4.1 I²S interface for stereo audio output
 * 
 * THESE PINS ARE FIXED ON TEENSY 4.1 (do not change unless using different board)
 * ========================================================================== */

/* I²S Audio Output Pins (fixed on Teensy 4.1 for SGTL5000) */
#define AUDIO_I2S_BCLK_PIN  21  // Bit Clock (BCLK)
#define AUDIO_I2S_LRCLK_PIN 20  // Left/Right Clock (LRCLK / Frame Sync)
#define AUDIO_I2S_OUT_PIN   7   // Serial Data Out (TX)
#define AUDIO_I2S_IN_PIN    8   // Serial Data In (RX) - for microphone

/* Audio amplifier enable/shutdown pin (if external amp present) */
// TODO: VERIFY WITH ELECTRICAL TEAM!
// Uncomment if there's an amplifier enable pin:
// #define AUDIO_AMP_ENABLE_PIN 23 // GPIO to enable external amplifier - VERIFY WITH EE TEAM

/* ============================================================================
 * AUDIO CONFIGURATION
 * ========================================================================== */

#define AUDIO_SAMPLE_RATE       44100   // Professional quality (Hz)
#define AUDIO_BLOCK_SIZE        128     // ~2.9ms per block at 44.1kHz
#define AUDIO_BIT_DEPTH         16      // 16-bit PCM audio
#define AUDIO_CHANNELS          2       // Stereo (both speaker & headphone)

/* Volume control parameters */
#define VOLUME_MIN              0.0f    // Silent
#define VOLUME_MAX              1.0f    // Full volume
#define VOLUME_DEFAULT          0.7f    // Recommended startup volume (70%)

/* ============================================================================
 * SD CARD / AUDIO FILE STORAGE (Optional)
 * For accessibility features - spoken string names and directions
 * 
 * TODO: VERIFY WITH ELECTRICAL TEAM!
 * ========================================================================== */

#define SD_CHIP_SELECT_PIN  BUILTIN_SDCARD  // Use Teensy 4.1 built-in SD card
// If using external SD card via SPI, uncomment and define CS pin:
// #define SD_CHIP_SELECT_PIN  10  // SPI chip select - VERIFY WITH EE TEAM

#define AUDIO_FILES_PATH    "/AUDIO/"
#define MAX_FILENAME_LENGTH 256

/* Pre-recorded audio file names (if SD card is present) */
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

/* ============================================================================
 * DIGITAL SIGNAL PROCESSING (FFT)
 * ========================================================================== */

#define FFT_SIZE                256     // 256-point FFT
#define FFT_INPUT_SAMPLE_RATE   10000   // Downsampled to 10kHz for guitar tuning
#define FFT_HZ_PER_BIN          (FFT_INPUT_SAMPLE_RATE / FFT_SIZE)  // ~39 Hz resolution

/* Frequency detection range (Hz) - covers all guitar strings */
#define MIN_DETECTABLE_FREQ     50.0f   // Below low E (82.41 Hz)
#define MAX_DETECTABLE_FREQ     400.0f  // Well above high E (329.63 Hz)

/* ============================================================================
 * TUNING PARAMETERS
 * ========================================================================== */

#define TUNING_TOLERANCE_CENTS  2.0     // ±2 cents considered "in tune"
#define CENTS_THRESHOLD_WARN    10.0    // Warn if > 10 cents off
#define CENTS_THRESHOLD_CRITICAL 50.0   // Critical if > 50 cents off

/* ============================================================================
 * MICROPHONE INPUT
 * 
 * TODO: VERIFY WITH ELECTRICAL TEAM!
 * - Is microphone connected to SGTL5000 line-in?
 * - Or connected to a Teensy ADC pin?
 * - Or is it an I²S MEMS microphone?
 * ========================================================================== */

// If using ADC for microphone (uncomment if applicable):
// #define MICROPHONE_INPUT_PIN    A0      // ADC pin for microphone - VERIFY WITH EE TEAM

#define MICROPHONE_SAMPLE_RATE  44100   // 44.1 kHz recording rate
#define MICROPHONE_GAIN         20      // dB gain amplification (if adjustable)

/* ============================================================================
 * TEENSY HARDWARE SPECIFICS
 * ========================================================================== */

#define TEENSY_VERSION          41      // Teensy 4.1
#define TEENSY_CLOCK_SPEED      600     // MHz
#define TEENSY_RAM_KB           1024    // 1 MB RAM available

/* Temporary buffer sizes for audio processing */
#define AUDIO_BUFFER_SIZE       4096    // 4K sample buffer
#define FFT_BUFFER_SIZE         FFT_SIZE * sizeof(float)

/* ============================================================================
 * DEBUG & DIAGNOSTICS
 * ========================================================================== */

#define ENABLE_DEBUG_PRINTS     1       // Set to 0 to disable console output
#define SERIAL_BAUD_RATE        115200  // USB serial monitor speed
#define DEBUG_FFT_OUTPUT        0       // Print FFT magnitude spectrum (very verbose!)
#define DEBUG_TUNING_RESULTS    1       // Print tuning analysis results

/* ============================================================================
 * BATTERY MANAGEMENT (If applicable)
 * 
 * TODO: VERIFY WITH ELECTRICAL TEAM!
 * - Is there battery voltage monitoring?
 * - Low battery detection circuit?
 * ========================================================================== */

// Uncomment if battery monitoring is present:
// #define BATTERY_VOLTAGE_PIN     A1      // ADC pin for battery voltage - VERIFY WITH EE TEAM
// #define BATTERY_LOW_THRESHOLD   3.3f    // Voltage threshold for low battery warning

/* ============================================================================
 * AUDIO PROCESSING CONFIGURATION
 * ========================================================================== */

#define SAMPLE_RATE    10000    // 10 kHz sample rate for FFT processing
#define SAMPLE_SIZE    1024     // Number of samples to collect
#define MIN_AMPLITUDE  100      // Minimum signal amplitude to process

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */