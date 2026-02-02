/**
 * audio_sequencer_v2.h - Audio sequencing with CFugue-style notation
 *
 * Enhanced version that uses readable note notation instead of raw frequencies
 * NO WAV FILES REQUIRED - all audio is synthesized in real-time
 */

#ifndef AUDIO_SEQUENCER_V2_H
#define AUDIO_SEQUENCER_V2_H

#include <stdint.h>
#include "string_detection.h"
#include "note_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * SYNTHESIZED AUDIO - NO WAV FILES NEEDED!
 * ========================================================================== */

/**
 * Play a synthesized tone using note notation (CFugue-style)
 * 
 * @param note - Note string (e.g., "E2", "A4", "F#3")
 * @param duration_ms - How long to play the tone (milliseconds)
 * 
 * Examples:
 *   playNoteTone("E2", 1000);  // Play low E for 1 second
 *   playNoteTone("A4", 500);   // Play concert A for 0.5 seconds
 */
void playNoteTone(const char* note, uint32_t duration_ms);

/**
 * Play a synthesized tone using frequency (legacy support)
 * 
 * @param frequency - Frequency in Hz
 * @param duration_ms - How long to play the tone (milliseconds)
 */
void playFrequencyTone(float frequency, uint32_t duration_ms);

/**
 * Play a short beep for tuning feedback
 * 
 * @param frequency - Beep frequency in Hz (higher = more urgent)
 * @param duration_ms - Beep duration in milliseconds
 */
void playBeep(float frequency, uint32_t duration_ms);

/**
 * Stop all currently playing tones
 */
void stopAllTones(void);

/* ============================================================================
 * GUITAR STRING PLAYBACK (Using Note Notation)
 * ========================================================================== */

/**
 * Play the reference tone for a guitar string
 * 
 * @param string_num - String number (1-6)
 * @param duration_ms - How long to play (default: 2000ms)
 * 
 * Examples:
 *   playGuitarString(1, 2000);  // Play high E (E4) for 2 seconds
 *   playGuitarString(6, 2000);  // Play low E (E2) for 2 seconds
 */
void playGuitarString(int string_num, uint32_t duration_ms);

/**
 * Play the reference tone for the currently selected string
 * Useful in "Play Tone" mode before user plays their note
 */
void playCurrentString(void);

/* ============================================================================
 * SPOKEN FEEDBACK (Synthesized Speech Tones)
 * These replace WAV file playback with synthesized audio cues
 * ========================================================================== */

/**
 * Play audio cue for string name
 * Uses distinct tone patterns to identify each string
 * 
 * @param string_num - String number (1-6)
 * 
 * Tone patterns:
 *   String 1 (E4): Single high beep
 *   String 2 (B3): Two medium beeps
 *   String 3 (G3): Three mid-low beeps
 *   String 4 (D3): Four low beeps
 *   String 5 (A2): Five very low beeps
 *   String 6 (E2): Six ultra-low beeps
 */
void playStringIdentifier(int string_num);

/**
 * Play audio cue for cents offset
 * Uses beep rate to indicate how far off tune
 * 
 * @param cents_offset - Cents offset from target
 */
void playCentsIndicator(double cents_offset);

/**
 * Play audio cue for tuning direction
 * 
 * @param direction - "UP", "DOWN", or "IN_TUNE"
 * 
 * Audio cues:
 *   UP: Rising tone (tune string tighter)
 *   DOWN: Falling tone (tune string looser)
 *   IN_TUNE: Happy ascending tones
 */
void playDirectionCue(const char* direction);

/* ============================================================================
 * ORIGINAL FUNCTIONS (Maintained for Compatibility)
 * ========================================================================== */

void audio_sequencer_init(void);
void generate_audio_feedback(const TuningResult* result);
void audio_sequencer_update(void);
uint32_t calculate_beep_interval(double cents_offset);
void generate_dynamic_beep_feedback(const TuningResult* result);
void audio_sequencer_update_beeps(uint32_t current_time_ms);

/* ============================================================================
 * NEW ENHANCED FEEDBACK MODES
 * ========================================================================== */

/**
 * Generate complete audio feedback using synthesized tones
 * This replaces WAV file playback with real-time synthesis
 * 
 * @param result - Tuning analysis result
 * 
 * Sequence:
 * 1. Play string identifier (tone pattern)
 * 2. Play reference note for that string
 * 3. Play cents indicator (beep rate)
 * 4. Play direction cue (up/down/in-tune)
 */
void generate_synthesized_feedback(const TuningResult* result);

/**
 * Simple mode: just play the reference tone
 * For "Play Tone" mode where user wants to hear the target note
 * 
 * @param string_num - String number to play (1-6)
 */
void playReferenceMode(int string_num);

/**
 * Full tuning assistant mode
 * Combines reference tone, listening, and dynamic feedback
 * 
 * @param string_num - String number to tune (1-6)
 */
void tuningAssistantMode(int string_num);

#ifdef __cplusplus
}
#endif

#endif