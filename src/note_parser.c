/**
 * note_parser.c - CFugue-style music notation parser implementation
 *
 * Converts musical notation strings to frequencies for embedded audio synthesis
 * Based on equal temperament tuning with A4 = 440 Hz reference
 */

#include "note_parser.h"
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ========================================================================== */

#define A4_FREQUENCY 440.0f        // Concert A reference frequency
#define A4_MIDI_NUMBER 69          // MIDI note number for A4
#define SEMITONES_PER_OCTAVE 12    // Equal temperament system

/* ============================================================================
 * GUITAR STRING DEFINITIONS
 * Standard tuning from high (string 1) to low (string 6)
 * ========================================================================== */

const GuitarString STANDARD_TUNING[6] = {
    {1, NOTE_E4, 329.63f},  // 1st string (thinnest) - High E
    {2, NOTE_B3, 246.94f},  // 2nd string - B
    {3, NOTE_G3, 196.00f},  // 3rd string - G
    {4, NOTE_D3, 146.83f},  // 4th string - D
    {5, NOTE_A2, 110.00f},  // 5th string - A
    {6, NOTE_E2, 82.41f}    // 6th string (thickest) - Low E
};

/* ============================================================================
 * NOTE NAME TO SEMITONE OFFSET MAPPING
 * Semitone offset from C (C=0, C#=1, D=2, ..., B=11)
 * ========================================================================== */

/**
 * Get semitone offset from C for a given note letter
 * @param note_letter - 'C', 'D', 'E', 'F', 'G', 'A', or 'B' (case insensitive)
 * @return Semitone offset (0-11), or -1 if invalid
 */
static int getNoteSemitoneOffset(char note_letter) {
    switch (toupper(note_letter)) {
        case 'C': return 0;
        case 'D': return 2;
        case 'E': return 4;
        case 'F': return 5;
        case 'G': return 7;
        case 'A': return 9;
        case 'B': return 11;
        default: return -1;
    }
}

/* ============================================================================
 * FREQUENCY CALCULATION
 * Uses equal temperament: f = 440 * 2^((n-69)/12)
 * where n is the MIDI note number
 * ========================================================================== */

float calculateNoteFrequency(char note_letter, int octave, bool is_sharp, bool is_flat) {
    // Get base semitone offset from C
    int semitone = getNoteSemitoneOffset(note_letter);
    if (semitone < 0) {
        return 0.0f; // Invalid note
    }
    
    // Apply sharp/flat
    if (is_sharp) {
        semitone += 1;
    }
    if (is_flat) {
        semitone -= 1;
    }
    
    // Calculate MIDI note number
    // MIDI: C0 = 12, C1 = 24, ..., A4 = 69
    int midi_note = (octave + 1) * 12 + semitone;
    
    // Calculate frequency using equal temperament formula
    // f = 440 * 2^((midi_note - 69) / 12)
    float frequency = A4_FREQUENCY * powf(2.0f, (midi_note - A4_MIDI_NUMBER) / 12.0f);
    
    return frequency;
}

/* ============================================================================
 * NOTE STRING PARSING
 * ========================================================================== */

ParsedNote parseNoteDetailed(const char* note) {
    ParsedNote result = {0};
    result.valid = false;
    
    if (note == NULL || note[0] == '\0') {
        return result;
    }
    
    int index = 0;
    
    // Parse note letter (required)
    result.note_letter = toupper(note[index]);
    if (getNoteSemitoneOffset(result.note_letter) < 0) {
        return result; // Invalid note letter
    }
    index++;
    
    // Parse accidental (optional: # or b)
    result.is_sharp = false;
    result.is_flat = false;
    
    if (note[index] == '#') {
        result.is_sharp = true;
        index++;
    } else if (note[index] == 'b') {
        result.is_flat = true;
        index++;
    }
    
    // Parse octave (required)
    if (!isdigit(note[index])) {
        return result; // Missing octave number
    }
    
    result.octave = note[index] - '0';
    
    // Validate octave range (0-8 is standard)
    if (result.octave < 0 || result.octave > 8) {
        return result;
    }
    
    // Calculate frequency
    result.frequency = calculateNoteFrequency(
        result.note_letter,
        result.octave,
        result.is_sharp,
        result.is_flat
    );
    
    result.valid = true;
    return result;
}

float parseNote(const char* note) {
    ParsedNote parsed = parseNoteDetailed(note);
    return parsed.valid ? parsed.frequency : 0.0f;
}

/* ============================================================================
 * GUITAR STRING HELPERS
 * ========================================================================== */

float getStringFrequency(int string_num) {
    if (string_num < 1 || string_num > 6) {
        return 0.0f;
    }
    return STANDARD_TUNING[string_num - 1].frequency;
}

const char* getStringNote(int string_num) {
    if (string_num < 1 || string_num > 6) {
        return NULL;
    }
    return STANDARD_TUNING[string_num - 1].note;
}

/* ============================================================================
 * FREQUENCY TO NOTE CONVERSION
 * ========================================================================== */

/**
 * Note name lookup table for common frequencies
 * Used by frequencyToNote() for reverse lookup
 */
typedef struct {
    float frequency;
    const char* note_name;
} FrequencyNote;

static const FrequencyNote frequency_table[] = {
    // Octave 2
    {73.42f,  "D2"},
    {77.78f,  "Eb2"},
    {82.41f,  "E2"},   // Low E string
    {87.31f,  "F2"},
    {92.50f,  "Gb2"},
    {98.00f,  "G2"},
    {103.83f, "Ab2"},
    {110.00f, "A2"},   // A string
    {116.54f, "Bb2"},
    {123.47f, "B2"},
    
    // Octave 3
    {130.81f, "C3"},
    {138.59f, "Db3"},
    {146.83f, "D3"},   // D string
    {155.56f, "Eb3"},
    {164.81f, "E3"},
    {174.61f, "F3"},
    {185.00f, "Gb3"},
    {196.00f, "G3"},   // G string
    {207.65f, "Ab3"},
    {220.00f, "A3"},
    {233.08f, "Bb3"},
    {246.94f, "B3"},   // B string
    
    // Octave 4
    {261.63f, "C4"},
    {277.18f, "Db4"},
    {293.66f, "D4"},
    {311.13f, "Eb4"},
    {329.63f, "E4"},   // High E string
    {349.23f, "F4"},
    {369.99f, "Gb4"},
    {392.00f, "G4"},
    {415.30f, "Ab4"},
    {440.00f, "A4"},   // Concert A
    {466.16f, "Bb4"},
    {493.88f, "B4"},
    
    // Octave 5
    {523.25f, "C5"},
};

#define FREQUENCY_TABLE_SIZE (sizeof(frequency_table) / sizeof(FrequencyNote))

const char* frequencyToNote(float frequency) {
    if (frequency <= 0.0f) {
        return NULL;
    }
    
    // Find closest frequency in table
    float min_diff = 1000.0f;
    const char* closest_note = NULL;
    
    for (int i = 0; i < FREQUENCY_TABLE_SIZE; i++) {
        float diff = fabsf(frequency - frequency_table[i].frequency);
        if (diff < min_diff) {
            min_diff = diff;
            closest_note = frequency_table[i].note_name;
        }
    }
    
    // Only return if within reasonable tolerance (+/-5 Hz)
    if (min_diff <= 5.0f) {
        return closest_note;
    }
    
    return NULL;
}

/* ============================================================================
 * VALIDATION
 * ========================================================================== */

bool isValidNote(const char* note) {
    ParsedNote parsed = parseNoteDetailed(note);
    return parsed.valid;
}

/* ============================================================================
 * INITIALIZATION
 * ========================================================================== */

void note_parser_init(void) {
    printf("Note Parser initialized (CFugue-style notation)\n");
    printf("  Standard tuning frequencies:\n");
    for (int i = 0; i < 6; i++) {
        printf("    String %d: %s = %.2f Hz\n",
               STANDARD_TUNING[i].string_number,
               STANDARD_TUNING[i].note,
               STANDARD_TUNING[i].frequency);
    }
}