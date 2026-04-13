/**
 * config.h - Hardware configuration for Teensy 4.1 Guitar Tuner
 *
 * FINAL CONFIRMED PINOUT - reconciled with electrical team
 *
 * CHANGES FROM ORIGINAL:
 *   1. Microphone corrected from A0 (pin 14) to A17 (pin 39)
 *   2. Audio output changed from I2S to MQS (pin 10) - Teensy 4.1 has no DAC
 *   3. String 1 button moved from pin 2 to pin 22 - pin 2 occupied by amp wiring
 *   4. Pin 23 amp enable REMOVED - LM386 has no enable/shutdown pin
 *   5. Rotary encoder removed - not part of build
 *   6. Volume potentiometer removed - handled by physical pot on LM386 module
 *   7. I2S audio definitions removed - not used
 *
 * HARDWARE:
 *   Amplifier:   HiLetgo LM386 module
 *   Power:       Adafruit PowerBoost 1000 Charge
 *   Microphone:  Analog electret capsule on pin 39 (A17)
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
 * Active HIGH - buttons connect pin to 3.3V when pressed
 * Use INPUT_PULLDOWN so pin rests LOW and rises HIGH on press
 * ========================================================================== */

#define STRING_1_BUTTON_PIN 22   // E4 string (329.63 Hz) - moved from pin 2
#define STRING_2_BUTTON_PIN 3    // B3 string (246.94 Hz)
#define STRING_3_BUTTON_PIN 4    // G3 string (196.00 Hz)
#define STRING_4_BUTTON_PIN 5    // D3 string (146.83 Hz)
#define STRING_5_BUTTON_PIN 6    // A2 string (110.00 Hz)
#define STRING_6_BUTTON_PIN 9    // E2 string (82.41 Hz)

/* ============================================================================
 * MODE SWITCH (Play Tone I vs Listen Only O)
 * Connect between pin 11 and GND
 * HIGH = Play Tone (Mode I), LOW = Listen Only (Mode O)
 * ========================================================================== */

#define MODE_SWITCH_PIN 11

/* ============================================================================
 * AUDIO OUTPUT - MQS
 *
 * MQS is the only audio output on Teensy 4.1 without an external DAC chip.
 * Pins are fixed by hardware and cannot be reassigned.
 *
 * WIRING:
 *   Pin 10 (MQSR) -> LM386 module IN pin
 *   Pin 12 (MQSL) -> leave unconnected (mono setup)
 * ========================================================================== */

#define AUDIO_MQS_RIGHT_PIN  10   // MQS Right - connect to LM386 IN
#define AUDIO_MQS_LEFT_PIN   12   // MQS Left  - leave unconnected

/* NOTE: No amp enable pin. LM386 has no shutdown pin.
   Power is controlled by the physical rocker switch via PowerBoost EN pin.
   All audio_amplifier_enable() and audio_amplifier_disable() calls
   have been removed from the codebase. */

/* ============================================================================
 * MICROPHONE INPUT
 *
 * Analog electret microphone on pin 39 (A17).
 * Confirmed by electrical team.
 * Previously incorrectly set to A0 (pin 14) in original code.
 * ========================================================================== */

#define MICROPHONE_INPUT_PIN    39    // Pin 39 - confirmed by electrical team
#define MICROPHONE_SAMPLE_RATE  10000  // 10 kHz - sufficient for guitar range 82-330 Hz

/* ADC configuration */
#define ADC_RESOLUTION    12           // 12-bit ADC (0 to 4095)
#define ADC_CENTER_VALUE  2048         // Center point for signed conversion

/* ============================================================================
 * DIGITAL SIGNAL PROCESSING (FFT)
 * ========================================================================== */

#define FFT_SIZE                1024   // 1024-point FFT: 10kHz / 1024 = 9.77 Hz/bin
#define FFT_INPUT_SAMPLE_RATE   10000  // 10 kHz sample rate
#define FFT_HZ_PER_BIN  (FFT_INPUT_SAMPLE_RATE / FFT_SIZE)  // ~10.23 Hz per bin  // ~9.77 Hz per bin
                                        // Parabolic interpolation: ±0.6 Hz typical accuracy
                                        // 3-frame moving average: smoother detections

/* Frequency detection range */
#define MIN_DETECTABLE_FREQ     82.0f  // Skip noisy low bins; start from bin 7
#define MAX_DETECTABLE_FREQ     400.0f // Above high E (329.63 Hz)

/* Audio buffer */
#define SAMPLE_SIZE             1024   // ADC samples per FFT frame (matches FFT_SIZE)
#define MIN_AMPLITUDE           40

/* ============================================================================
 * TUNING PARAMETERS
 * ========================================================================== */

#define TUNING_TOLERANCE_CENTS   2.0   // Within 2 cents = in tune
#define CENTS_THRESHOLD_WARN    10.0
#define CENTS_THRESHOLD_CRITICAL 50.0

/* ============================================================================
 * AUDIO PLAYBACK CONFIGURATION
 * ========================================================================== */

#define TONE_AMPLITUDE_DEFAULT  1.0f   // MQS tone volume (0.0 to 1.0)
#define BEEP_AMPLITUDE_DEFAULT  1.0f   // MQS beep volume

/* ============================================================================
 * TEENSY 4.1 HARDWARE
 * ========================================================================== */

#define TEENSY_VERSION      41
#define TEENSY_CLOCK_SPEED  600
#define TEENSY_RAM_KB       1024

/* ============================================================================
 * DEBUG
 * ========================================================================== */

#define ENABLE_DEBUG_PRINTS  1
#define SERIAL_BAUD_RATE     115200

/* ============================================================================
 * FINAL PIN SUMMARY
 *
 * DIGITAL INPUTS (internal pull-ups, active LOW):
 *   Pin 3   - Button: String 2 (B3, 246.94 Hz)
 *   Pin 4   - Button: String 3 (G3, 196.00 Hz)
 *   Pin 5   - Button: String 4 (D3, 146.83 Hz)
 *   Pin 6   - Button: String 5 (A2, 110.00 Hz)
 *   Pin 9   - Button: String 6 (E2, 82.41 Hz)
 *   Pin 11  - Mode Switch
 *   Pin 22  - Button: String 1 (E4, 329.63 Hz)
 *
 * AUDIO OUTPUT:
 *   Pin 10  - MQS Right -> LM386 IN
 *   Pin 12  - MQS Left  -> leave unconnected
 *
 * ANALOG INPUT:
 *   Pin 39 (A17) - Electret microphone
 *
 * POWER (not Teensy GPIO):
 *   Vin     - 5V from PowerBoost 1000 Charge
 *   GND     - Common ground
 *
 * PINS TO AVOID:
 *   Pin 0, 1    - USB serial RX/TX
 *   Pin 7, 8    - I2S data (claimed by audio library)
 *   Pin 13      - Onboard LED
 *   Pin 20, 21  - I2S clock (claimed by audio library)
 * ========================================================================== */

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */