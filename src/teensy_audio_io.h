/**
 * teensy_audio_io.h - Teensy Audio Library Interface
 * 
 * Header file for audio input/output functions
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
 * Initialize the Teensy audio system
 * Must be called once in setup()
 */
void init_audio_system(void);

/* ============================================================================
 * TONE PLAYBACK
 * ========================================================================== */

/**
 * Play a tone at a specific frequency
 * 
 * @param frequency - Frequency in Hz (e.g., 82.41, 440.0, 329.63)
 * @param duration_ms - How long to play in milliseconds
 * 
 * Examples:
 *   play_tone(329.63, 2000);  // Play E4 for 2 seconds
 *   play_tone(440.0, 1000);   // Play A4 for 1 second
 */
void play_tone(float frequency, uint32_t duration_ms);

/**
 * Play a short beep (for feedback)
 * 
 * @param frequency - Beep frequency in Hz (typically 400-1200 Hz)
 * @param duration_ms - Beep duration in milliseconds (typically 50-200 ms)
 */
void play_beep(float frequency, uint32_t duration_ms);

/**
 * Play a "ready" beep to signal user can play their string
 */
void play_ready_beep(void);

/**
 * Stop all audio output immediately
 */
void stop_all_audio(void);

/**
 * Update tone playback timing
 * Call this in your main loop() for non-blocking playback
 */
void update_tone_playback(void);

/* ============================================================================
 * AMPLIFIER CONTROL
 * ========================================================================== */

/**
 * Enable the audio amplifier (PAM8302A)
 * Call this before playing audio
 */
void audio_amplifier_enable(void);

/**
 * Disable the audio amplifier to save power
 * Call this when not using audio
 */
void audio_amplifier_disable(void);

/* ============================================================================
 * MICROPHONE INPUT
 * ========================================================================== */

/**
 * Read frequency from microphone using FFT
 * 
 * @param buffer - Audio sample buffer (optional, can be NULL)
 * @param buffer_size - Size of buffer (optional, can be 0)
 * @return Detected frequency in Hz, or 0.0 if no signal
 */
double read_frequency_from_microphone(int16_t* buffer, int buffer_size);

/* ============================================================================
 * DIAGNOSTICS
 * ========================================================================== */

/**
 * Print audio system status (CPU usage, memory, etc.)
 * Useful for debugging
 */
void print_audio_status(void);

/**
 * Test audio system by playing a scale
 * Good for verifying speaker/amp connection
 */
void test_audio_playback(void);

#ifdef __cplusplus
}
#endif

#endif /* TEENSY_AUDIO_IO_H */