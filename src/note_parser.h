/**
 * note_parser.h - CFugue-style music notation parser for embedded systems
 *
 * Provides simple, readable note notation similar to CFugue but designed
 * for bare-metal embedded systems (Teensy 4.1) without OS dependencies.
 *
 * Usage Examples:
 *   parseNote("E2")  -> 82.41 Hz  (Low E guitar string)
 *   parseNote("A2")  -> 110.00 Hz (A guitar string)
 *   parseNote("D3")  -> 146.83 Hz (D guitar string)
 *   parseNote("G3")  -> 196.00 Hz (G guitar string)
 *   parseNote("B3")  -> 246.94 Hz (B guitar string)
 *   parseNote("E4")  -> 329.63 Hz (High E guitar string)
 *
 * Author: RPVI Guitar Tuner Team
 * Date: January 2026
 */

#ifndef NOTE_PARSER_H
#define NOTE_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * NOTE NOTATION DEFINITIONS
 * ========================================================================== */

/**
 * Guitar string note definitions (CFugue-style notation)
 * Use these constants instead of raw frequencies for better readability
 */
#define NOTE_E2  "E2"    // Low E string (6th string) - 82.41 Hz
#define NOTE_A2  "A2"    // A string (5th string) - 110.00 Hz
#define NOTE_D3  "D3"    // D string (4th string) - 146.83 Hz
#define NOTE_G3  "G3"    // G string (3rd string) - 196.00 Hz
#define NOTE_B3  "B3"    // B string (2nd string) - 246.94 Hz
#define NOTE_E4  "E4"    // High E string (1st string) - 329.63 Hz

/* Alternative tunings (if needed later) */
#define NOTE_D2  "D2"    // Drop D tuning - 73.42 Hz
#define NOTE_Eb2 "Eb2"   // Half-step down - 77.78 Hz
#define NOTE_F2  "F2"    // 87.31 Hz
#define NOTE_Gb2 "Gb2"   // 92.50 Hz
#define NOTE_G2  "G2"    // 98.00 Hz
#define NOTE_Ab2 "Ab2"   // 103.83 Hz
#define NOTE_Bb2 "Bb2"   // 116.54 Hz
#define NOTE_C3  "C3"    // 130.81 Hz
#define NOTE_Db3 "Db3"   // 138.59 Hz
#define NOTE_Eb3 "Eb3"   // 155.56 Hz
#define NOTE_F3  "F3"    // 174.61 Hz
#define NOTE_Gb3 "Gb3"   // 185.00 Hz
#define NOTE_Ab3 "Ab3"   // 207.65 Hz
#define NOTE_C4  "C4"    // 261.63 Hz
#define NOTE_Db4 "Db4"   // 277.18 Hz
#define NOTE_Eb4 "Eb4"   // 311.13 Hz
#define NOTE_F4  "F4"    // 349.23 Hz
#define NOTE_Gb4 "Gb4"   // 369.99 Hz
#define NOTE_G4  "G4"    // 392.00 Hz
#define NOTE_Ab4 "Ab4"   // 415.30 Hz
#define NOTE_A4  "A4"    // 440.00 Hz (Concert A)
#define NOTE_Bb4 "Bb4"   // 466.16 Hz

/* ============================================================================
 * STRUCTURE DEFINITIONS
 * ========================================================================== */

/**
 * Represents a parsed musical note with all its properties
 */
typedef struct {
    char note_letter;      // 'C', 'D', 'E', 'F', 'G', 'A', or 'B'
    int octave;           // 0-8 (guitar typically uses 2-4)
    bool is_sharp;        // true if note has '#' (sharp)
    bool is_flat;         // true if note has 'b' (flat)
    float frequency;      // Calculated frequency in Hz
    bool valid;           // true if parsing succeeded
} ParsedNote;

/**
 * Guitar string configuration
 * Maps string numbers to their standard tuning notes
 */
typedef struct {
    int string_number;    // 1-6
    const char* note;     // Note notation (e.g., "E4", "B3")
    float frequency;      // Frequency in Hz
} GuitarString;

/* ============================================================================
 * FUNCTION DECLARATIONS
 * ========================================================================== */

/**
 * Initialize the note parser module
 * Call this once at startup
 */
void note_parser_init(void);

/**
 * Parse a note string and return its frequency
 * 
 * @param note - Note string in format "C4", "Eb3", "F#2", etc.
 * @return Frequency in Hz, or 0.0 if parsing failed
 * 
 * Examples:
 *   parseNote("E2")  -> 82.41 Hz
 *   parseNote("A4")  -> 440.00 Hz
 *   parseNote("F#3") -> 185.00 Hz
 *   parseNote("Gb3") -> 185.00 Hz (enharmonic equivalent)
 */
float parseNote(const char* note);

/**
 * Parse a note string and return detailed information
 * 
 * @param note - Note string in format "C4", "Eb3", "F#2", etc.
 * @return ParsedNote structure with all note properties
 * 
 * Use this when you need more than just the frequency
 */
ParsedNote parseNoteDetailed(const char* note);

/**
 * Get frequency for a guitar string number (1-6)
 * 
 * @param string_num - String number (1 = high E, 6 = low E)
 * @return Frequency in Hz, or 0.0 if invalid string number
 * 
 * Examples:
 *   getStringFrequency(1) -> 329.63 Hz (high E)
 *   getStringFrequency(6) -> 82.41 Hz (low E)
 */
float getStringFrequency(int string_num);

/**
 * Get note name for a guitar string number (1-6)
 * 
 * @param string_num - String number (1 = high E, 6 = low E)
 * @return Note string (e.g., "E4", "B3"), or NULL if invalid
 * 
 * Examples:
 *   getStringNote(1) -> "E4"
 *   getStringNote(5) -> "A2"
 */
const char* getStringNote(int string_num);

/**
 * Convert frequency to nearest note name
 * 
 * @param frequency - Frequency in Hz
 * @return Note string (e.g., "E2", "A4"), or NULL if out of range
 * 
 * Examples:
 *   frequencyToNote(82.41)  -> "E2"
 *   frequencyToNote(440.00) -> "A4"
 */
const char* frequencyToNote(float frequency);

/**
 * Calculate frequency for a given note and octave
 * Uses equal temperament tuning with A4 = 440 Hz
 * 
 * @param note_letter - 'C', 'D', 'E', 'F', 'G', 'A', or 'B'
 * @param octave - Octave number (0-8)
 * @param is_sharp - true if note is sharp
 * @param is_flat - true if note is flat
 * @return Frequency in Hz
 */
float calculateNoteFrequency(char note_letter, int octave, bool is_sharp, bool is_flat);

/**
 * Check if a note string is valid
 * 
 * @param note - Note string to validate
 * @return true if valid, false otherwise
 */
bool isValidNote(const char* note);

/* ============================================================================
 * GUITAR STRING MAPPING
 * ========================================================================== */

/**
 * Standard guitar tuning array (high to low)
 * Use these for easy access to standard tuning frequencies
 */
extern const GuitarString STANDARD_TUNING[6];

#ifdef __cplusplus
}
#endif

#endif /* NOTE_PARSER_H */