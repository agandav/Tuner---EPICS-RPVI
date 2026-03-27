/**
 * teensy_audio_io.h - Teensy Audio Library Interface
 *
 * CHANGES FROM ORIGINAL:
 *   1. Removed audio_amplifier_enable() and audio_amplifier_disable() -
 *      LM386 has no enable pin. Power is controlled by physical rocker
 *      switch connected to PowerBoost 1000 Charge EN pin.
 */

#ifndef TEENSY_AUDIO_IO_H
#define TEENSY_AUDIO_IO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * INITIALIZATION
 * ========================================================================== */

/**
 * Initialize the Teensy audio system (MQS output + ADC mic input).
 * Must be called once in setup().
 */
void init_audio_system(void);

/* ============================================================================
 * TONE PLAYBACK
 * ========================================================================== */

/**
 * Play a tone at a specific frequency (non-blocking).
 * Call update_tone_playback() every loop() to stop on schedule.
 *
 * @param frequency   - Frequency in Hz
 * @param duration_ms - Duration in milliseconds
 */
void play_tone(float frequency, uint32_t duration_ms);

/**
 * Play a short beep (blocking - returns after beep completes).
 *
 * @param frequency   - Beep frequency in Hz (typically 400-1200 Hz)
 * @param duration_ms - Duration in milliseconds (typically 50-200 ms)
 */
void play_beep(float frequency, uint32_t duration_ms);

/**
 * Play a "ready" beep to signal user can play their string.
 * Blocking - 200ms.
 */
void play_ready_beep(void);

/**
 * Stop all audio output immediately.
 */
void stop_all_audio(void);

/**
 * Update non-blocking tone timing.
 * Must be called every loop() iteration.
 */
void update_tone_playback(void);

/* ============================================================================
 * MICROPHONE INPUT
 * ========================================================================== */

/**
 * Sample microphone on pin 39 (A17) and return detected frequency via FFT.
 *
 * @param buffer      - Unused (kept for API compatibility), pass NULL
 * @param buffer_size - Unused, pass 0
 * @return Detected frequency in Hz, or 0.0 if no valid signal
 */
double read_frequency_from_microphone(int16_t* buffer, int buffer_size);

/* ============================================================================
 * DIAGNOSTICS
 * ========================================================================== */

/**
 * Print audio system CPU/memory usage to Serial.
 */
void print_audio_status(void);

/**
 * Play all 6 guitar string reference tones in sequence.
 * Run at startup to verify speaker and LM386 are working.
 */
void test_audio_playback(void);

#ifdef __cplusplus
}
#endif

#endif /* TEENSY_AUDIO_IO_H */