/**
 * note_parser_test.c
 * 
 * This tests ONLY the note parsing math - can run on laptop!
 * Compile and run on PC to verify frequency calculations
 * 
 * Compile with:
 *   gcc note_parser_test.c -lm -o note_parser_test
 *   ./note_parser_test
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define A4_FREQUENCY 440.0f
#define A4_MIDI_NUMBER 69

// Note parsing structures
typedef struct {
    char note_letter;
    int octave;
    bool is_sharp;
    bool is_flat;
    float frequency;
    bool valid;
} ParsedNote;

// Get semitone offset
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

// Calculate frequency
float calculateNoteFrequency(char note_letter, int octave, bool is_sharp, bool is_flat) {
    int semitone = getNoteSemitoneOffset(note_letter);
    if (semitone < 0) return 0.0f;
    
    if (is_sharp) semitone += 1;
    if (is_flat) semitone -= 1;
    
    int midi_note = (octave + 1) * 12 + semitone;
    float frequency = A4_FREQUENCY * powf(2.0f, (midi_note - A4_MIDI_NUMBER) / 12.0f); // calculates frequency startwith A4 = 440Hz and uses equal temperament which is a wa
    
    return frequency;
}

// Parse note string
ParsedNote parseNoteDetailed(const char* note) {
    ParsedNote result = {0};
    result.valid = false;
    
    if (note == NULL || note[0] == '\0') return result;
    
    int index = 0;
    
    result.note_letter = toupper(note[index]);
    if (getNoteSemitoneOffset(result.note_letter) < 0) return result;
    index++;
    
    result.is_sharp = false;
    result.is_flat = false;
    
    if (note[index] == '#') {
        result.is_sharp = true;
        index++;
    } else if (note[index] == 'b') {
        result.is_flat = true;
        index++;
    }
    
    if (!isdigit(note[index])) return result;
    result.octave = note[index] - '0';
    if (result.octave < 0 || result.octave > 8) return result;
    
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

// Test function
void test_note(const char* note_str, float expected_freq) {
    float actual = parseNote(note_str);
    float diff = fabs(actual - expected_freq);
    
    printf("%-10s -> %8.2f Hz ", note_str, actual);
    
    if (diff < 0.01) {
        printf("[PASS] (expected %.2f)\n", expected_freq);
    } else {
        printf("[FAIL] (expected %.2f, diff: %.2f)\n", expected_freq, diff);
    }
}

int main() {
    printf("\n========================================\n");
    printf("  Note Parser Test (Laptop Version)\n");
    printf("========================================\n\n");
    
    printf("Testing Guitar String Frequencies:\n");
    printf("-----------------------------------\n");
    test_note("E2", 82.41);    // Low E
    test_note("A2", 110.00);   // A string
    test_note("D3", 146.83);   // D string
    test_note("G3", 196.00);   // G string
    test_note("B3", 246.94);   // B string
    test_note("E4", 329.63);   // High E
    
    printf("\nTesting Concert Pitch:\n");
    printf("-----------------------------------\n");
    test_note("A4", 440.00);   // Concert A
    
    printf("\nTesting Accidentals:\n");
    printf("-----------------------------------\n");
    test_note("C4", 261.63);   // Middle C
    test_note("C#4", 277.18);  // C sharp
    test_note("Db4", 277.18);  // D flat (same as C#)
    test_note("F#3", 185.00);  // F sharp
    test_note("Gb3", 185.00);  // G flat (same as F#)
    
    printf("\nTesting Octaves:\n");
    printf("-----------------------------------\n");
    test_note("C0", 16.35);    // Lowest C
    test_note("C1", 32.70);
    test_note("C2", 65.41);
    test_note("C3", 130.81);
    test_note("C4", 261.63);   // Middle C
    test_note("C5", 523.25);
    test_note("C6", 1046.50);
    test_note("C7", 2093.00);
    test_note("C8", 4186.01);  // Highest C
    
    printf("\n========================================\n");
    printf("All tests complete!\n");
    printf("If all PASS, note parser math is correct \n");
    printf("========================================\n\n");
    
    return 0;
}