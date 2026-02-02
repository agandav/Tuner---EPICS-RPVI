/**
 * button_input.h - Button input handler for 7-button note selection interface
 * 
 * Interface: 7 note buttons (A-G)
 * 
 * Usage:
 *   User presses one of the 7 note buttons: A, B, C, D, E, F, or G
 *   System auto-detects octave from FFT frequency
 *   System returns target frequency for that natural note in the detected octave
 */

#ifndef BUTTON_INPUT_H
#define BUTTON_INPUT_H

#include <stdint.h>

/* Button codes for note selection */
typedef enum {
    NOTE_NONE = 0,
    NOTE_A = 1,
    NOTE_B = 2,
    NOTE_C = 3,
    NOTE_D = 4,
    NOTE_E = 5,
    NOTE_F = 6,
    NOTE_G = 7
} NoteButton;

/* Button press structure - USER INPUT ONLY (no octave required) */
typedef struct {
    NoteButton note;           // Which note button was pressed (A-G)
} ButtonInput;

/* Function prototypes */

/**
 * Detect octave from detected frequency
 * 
 * Maps frequency ranges to octaves using standard guitar ranges:
 *   82-164 Hz   -> Octave 2 (partial: E2-B2)
 *   164-328 Hz  -> Octave 3 (complete: C3-B3)
 *   328-656 Hz  -> Octave 4 (complete: C4-B4)
 *   656-1312 Hz -> Octave 5 (complete: C5-B5)
 * 
 * @param detected_freq - The frequency detected by FFT
 * @return Octave (2-5), or 0 if frequency is out of valid range
 */
int detect_octave_from_frequency(double detected_freq);

/**
 * Convert button input to target frequency
 * 
 * Combines user button selection with auto-detected octave to determine
 * the target frequency the user wants to tune to.
 * 
 * @param button - Button input structure (note only, NO octave or accidentals)
 * @param detected_freq - Frequency detected by FFT (used to auto-detect octave)
 * @return Target frequency in Hz, or -1.0 if invalid input
 * 
 * Example:
 *   ButtonInput btn = {NOTE_A};  // User presses A button
 *   double detected = 441.5;  // FFT detected 441.5 Hz
 *   double target = button_to_frequency(&btn, detected);  // Returns 440.0 Hz
 *   // System auto-detected octave 4 from 441.5 Hz -> A4 = 440.0 Hz
 */
double button_to_frequency(const ButtonInput* button, double detected_freq);

/**
 * Convert note letter to semitone offset from A
 * Helper function used internally
 * 
 * A=0, B=2, C=3, D=5, E=7, F=8, G=10
 * (relative to A as the reference)
 */
int note_to_semitone_offset(NoteButton note);

/**
 * Validate button input (note only)
 * 
 * @param button - Button input to validate
 * @return 1 if valid, 0 if invalid
 * 
 * Note: Does NOT validate octave (octave is auto-detected from frequency)
 */
int is_valid_button_input(const ButtonInput* button);

#endif // BUTTON_INPUT_H
