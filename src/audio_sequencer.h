/**
 * audio_sequencer.h - Audio sequencing interface
 *
 * Generates appropriate audio feedback based on tuning results
 */

#ifndef AUDIO_SEQUENCER_H
#define AUDIO_SEQUENCER_H

#include <stdint.h>
#include "string_detection.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * AUDIO FILE DEFINITIONS
 * ========================================================================== */

/* String name audio files */
#define FILE_E      "E.wav"
#define FILE_A      "A.wav"
#define FILE_D      "D.wav"
#define FILE_G      "G.wav"
#define FILE_B      "B.wav"

/* Cents offset audio files */
#define FILE_10_CENTS   "10_cents.wav"
#define FILE_20_CENTS   "20_cents.wav"

/* Direction audio files */
#define FILE_UP         "up.wav"
#define FILE_DOWN       "down.wav"
#define FILE_IN_TUNE    "in_tune.wav"

/* ============================================================================
 * FUNCTION DECLARATIONS
 * ========================================================================== */

/**
 * Initialize audio sequencer
 */
void audio_sequencer_init(void);

/**
 * Play an audio file (wrapper for both implementations)
 * This function works with both SD card and synthesized audio
 * @param filename - Path to audio file
 */
void play_audio_file(const char* filename);

/**
 * Get audio filename for a string number
 * @param string_num - String number (1-6)
 * @return Filename or NULL if invalid
 */
const char* get_string_filename(int string_num);

/**
 * Get audio filename for cents offset
 * @param cents - Cents offset value
 * @return Filename or NULL if in tune
 */
const char* get_cents_filename(double cents);

/**
 * Generate complete audio feedback sequence for tuning result
 * @param result - Tuning analysis result
 */
void generate_audio_feedback(const TuningResult* result);

/**
 * Update audio sequencer state (call in main loop)
 */
void audio_sequencer_update(void);

/**
 * Calculate beep interval based on cents offset
 * @param cents_offset - How far from target (in cents)
 * @return Milliseconds between beeps (0 = in tune, no beep)
 */
uint32_t calculate_beep_interval(double cents_offset);

/**
 * Generate dynamic beep feedback based on tuning accuracy
 * Faster beeps = further from tune, slower beeps = closer to tune
 * @param result - Current tuning result with cents_offset
 */
void generate_dynamic_beep_feedback(const TuningResult* result);

/**
 * Update dynamic beep feedback timing
 * Call this frequently (every 10-50 ms) in main loop
 * @param current_time_ms - Current system time in milliseconds
 * 
 * NOTE: This function triggers actual beep hardware via teensy_audio_io
 */
void audio_sequencer_update_beeps(uint32_t current_time_ms);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_SEQUENCER_H */