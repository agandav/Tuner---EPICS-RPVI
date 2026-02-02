/**
 * button_input.c - Button input handler implementation
 * 
 * Converts 7-button input (A-G notes + Flat/Sharp modifiers) to target frequencies
 */

#include <math.h>
#include "button_input.h"

/**
 * Reference frequency: A4 = 440 Hz (international tuning standard)
 * This is the ONLY hardcoded frequency needed
 * All other frequencies are calculated mathematically
 */
#define A4_REFERENCE 440.0

/**
 * Semitone ratio: Each semitone is 2^(1/12) ~= 1.05946
 * Used to calculate frequencies relative to A4
 */
#define SEMITONE_RATIO 1.05946309436

/**
 * Convert note letter to semitone offset from A
 * 
 * A is our reference (offset 0)
 * Counting up from A: A(0), A#(1), B(2), C(3), C#(4), D(5), D#(6), E(7), F(8), F#(9), G(10), G#(11)
 * 
 * @param note - Note button pressed
 * @return Semitone offset from A, or -1 if invalid
 */
int note_to_semitone_offset(NoteButton note) {
    switch (note) {
        case NOTE_A: return 0;   // A is reference
        case NOTE_B: return 2;   // B is 2 semitones above A
        case NOTE_C: return 3;   // C is 3 semitones above A
        case NOTE_D: return 5;   // D is 5 semitones above A
        case NOTE_E: return 7;   // E is 7 semitones above A
        case NOTE_F: return 8;   // F is 8 semitones above A
        case NOTE_G: return 10;  // G is 10 semitones above A
        default:
            return -1;
    }
}

/**
 * Detect octave from detected frequency
 * 
 * Maps frequency ranges to octaves based on guitar tuning range:
 *   41-82 Hz    -> Octave 1 (sub-bass, below practical guitar)
 *   82-164 Hz   -> Octave 2 (low strings)
 *   164-328 Hz  -> Octave 3 (mid strings)
 *   328-656 Hz  -> Octave 4 (high strings)
 *   656-1312 Hz -> Octave 5 (ultra-high frets)
 * 
 * @param detected_freq - Frequency detected by FFT
 * @return Octave (2-5), or 0 if frequency is out of valid range
 */
int detect_octave_from_frequency(double detected_freq) {
    /* Octave boundaries based on A frequency doublings:
     * A1 = 27.5 Hz
     * A2 = 55 Hz
     * A3 = 110 Hz
     * A4 = 220 Hz
     * A5 = 440 Hz
     * 
     * But we use midpoints between octaves for cleaner detection:
     * Octave 2 boundary: around 82 Hz (E2)
     * Octave 3 boundary: around 164 Hz (E3)
     * Octave 4 boundary: around 328 Hz (E4)
     * Octave 5 boundary: around 656 Hz (E5)
     */
    
    if (detected_freq < 82.0) {
        return 0;  /* Too low for practical guitar */
    } else if (detected_freq < 164.0) {
        return 2;  /* E2-B2 range (lowest guitar notes) */
    } else if (detected_freq < 328.0) {
        return 3;  /* C3-B3 range (mid strings) */
    } else if (detected_freq < 656.0) {
        return 4;  /* C4-B4 range (standard high strings) */
    } else if (detected_freq < 1312.0) {
        return 5;  /* C5-B5 range (ultra-high frets) */
    } else {
        return 0;  /* Too high for practical guitar */
    }
}

/**
 * Validate button input (note only)
 * 
 * Valid ranges:
 *   - Note: A-G (NOTE_A through NOTE_G)
 * 
 * Note: Octave is NOT validated (it's auto-detected from frequency)
 */
int is_valid_button_input(const ButtonInput* button) {
    if (!button) return 0;
    
    /* Check note is valid (A-G) */
    if (button->note < NOTE_A || button->note > NOTE_G) {
        return 0;
    }
    
    return 1;
}

/**
 * Convert button input to target frequency
 * 
 * Combines user button selection with auto-detected octave to calculate
 * the target frequency for tuning.
 * 
 * Algorithm:
 * 1. Validate button input (note only)
 * 2. Auto-detect octave from detected frequency
 * 3. Get semitone offset of the note from A
 * 4. Calculate semitones from A4 reference based on detected octave
 * 5. Apply semitone ratio formula: freq = A4 * (2^(1/12))^semitones
 * 
 * Example: User presses [A] button while playing 441.5 Hz frequency
 *   - Button: A (natural, no modifiers)
 *   - Detected: 441.5 Hz
 *   - Auto-octave: 441.5 Hz falls in 328-656 range -> Octave 4
 *   - Note A: offset = 0
 *   - Octave 4: same as reference octave A4
 *   - Semitones from A4: 0
 *   - Frequency: 440 * (2^(1/12))^0 = 440 Hz
 *   - Result: Target frequency is 440.0 Hz (A4)
 */
double button_to_frequency(const ButtonInput* button, double detected_freq) {
    if (!button || !is_valid_button_input(button)) {
        return -1.0;  /* Invalid button input */
    }
    
    /* Step 1: Auto-detect octave from detected frequency */
    int octave = detect_octave_from_frequency(detected_freq);
    if (octave == 0) {
        return -1.0;  /* Frequency out of valid range */
    }
    
    /* Step 2: Get semitone offset of the note from A (within octave) */
    int note_offset = note_to_semitone_offset(button->note);
    if (note_offset < 0) {
        return -1.0;  /* Invalid note */
    }
    
    /* Step 3: Calculate total semitones from A4 reference
     * 
     * Octave reference:
     *   A2: 2 octaves below A4 = 2 * 12 = 24 semitones below
     *   A3: 1 octave below A4 = 12 semitones below
     *   A4: reference octave = 0 semitones
     *   A5: 1 octave above A4 = 12 semitones above
     * 
     * Then add the note's offset within its octave
     */
    int octaves_from_a4 = octave - 4;  /* 4 is A4's octave */
    int total_semitones = (octaves_from_a4 * 12) + note_offset;
    
    /* Step 4: Calculate frequency using semitone formula
     * 
     * Formula: f = f_ref * (semitone_ratio)^n
     * Where:
     *   f_ref = 440 Hz (A4 reference)
     *   semitone_ratio = 2^(1/12) ~= 1.05946
     *   n = number of semitones from reference
     * 
     * More precisely: f = 440 * pow(2.0, n/12.0)
     */
    double frequency = A4_REFERENCE * pow(2.0, (double)total_semitones / 12.0);
    
    return frequency;
}

/**
 * HELPER FUNCTION: Get note name string for display
 * (Useful for UI/debugging)
 * 
 * Note: Does not include octave in the name since octave is auto-detected
 * 
 * Returns: Single letter note name (A, B, C, D, E, F, or G)
 */
const char* get_note_name(const ButtonInput* button) {
    if (!button || !is_valid_button_input(button)) {
        return "?";
    }
    
    switch (button->note) {
        case NOTE_A: return "A";
        case NOTE_B: return "B";
        case NOTE_C: return "C";
        case NOTE_D: return "D";
        case NOTE_E: return "E";
        case NOTE_F: return "F";
        case NOTE_G: return "G";
        default:    return "?";
    }
}
