/**
 * button_input_tests.c - Unit tests for button input handler with auto-octave detection
 * 
 * Tests the conversion of 7-button input (A-G only, no modifiers) to target frequencies
 * WITH AUTOMATIC OCTAVE DETECTION FROM DETECTED FREQUENCY
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "button_input.h"

#define TOLERANCE 0.5  /* Frequency tolerance in Hz */

/* Test counter */
static int total_tests = 0;
static int passed_tests = 0;

void assert_freq_equal(double detected, double expected, double tolerance, const char* test_name) {
    total_tests++;
    double diff = fabs(detected - expected);
    if (diff <= tolerance) {
        passed_tests++;
        printf("✓ PASS: %s (%.2f Hz, expected %.2f Hz, diff %.2f Hz)\n", 
               test_name, detected, expected, diff);
    } else {
        printf("✗ FAIL: %s (%.2f Hz, expected %.2f Hz, diff %.2f Hz)\n", 
               test_name, detected, expected, diff);
    }
}

void assert_octave_equal(int detected, int expected, const char* test_name) {
    total_tests++;
    if (detected == expected) {
        passed_tests++;
        printf("✓ PASS: %s (octave %d)\n", test_name, detected);
    } else {
        printf("✗ FAIL: %s (got octave %d, expected %d)\n", test_name, detected, expected);
    }
}

/**
 * TEST 1: Auto-octave detection from frequency
 */
void run_auto_octave_detection_tests(void) {
    printf("\n");
    printf("========================================================\n");
    printf("TEST 1: AUTO-OCTAVE DETECTION FROM FREQUENCY\n");
    printf("========================================================\n\n");
    
    /* Test octave boundaries */
    printf("Octave 2 (82-164 Hz):\n");
    assert_octave_equal(detect_octave_from_frequency(82.41), 2, "E2 (lowest)");
    assert_octave_equal(detect_octave_from_frequency(110.00), 2, "A2");
    assert_octave_equal(detect_octave_from_frequency(163.81), 2, "B2");
    
    printf("\nOctave 3 (164-328 Hz):\n");
    assert_octave_equal(detect_octave_from_frequency(164.81), 3, "E3");
    assert_octave_equal(detect_octave_from_frequency(220.00), 3, "A3");
    assert_octave_equal(detect_octave_from_frequency(246.94), 3, "B3");
    
    printf("\nOctave 4 (328-656 Hz):\n");
    assert_octave_equal(detect_octave_from_frequency(329.63), 4, "E4");
    assert_octave_equal(detect_octave_from_frequency(440.00), 4, "A4 (reference)");
    assert_octave_equal(detect_octave_from_frequency(587.33), 4, "D4");
    
    printf("\nOctave 5 (656-1312 Hz):\n");
    assert_octave_equal(detect_octave_from_frequency(659.25), 5, "E5");
    assert_octave_equal(detect_octave_from_frequency(880.00), 5, "A5");
    
    printf("\nOut of Range:\n");
    assert_octave_equal(detect_octave_from_frequency(41.20), 0, "Below range");
    assert_octave_equal(detect_octave_from_frequency(1400.0), 0, "Above range");
}

/**
 * TEST 2: Button input validation
 */
void run_validation_tests(void) {
    printf("\n");
    printf("========================================================\n");
    printf("TEST 2: BUTTON INPUT VALIDATION\n");
    printf("========================================================\n\n");
    
    total_tests++;
    ButtonInput valid = {NOTE_A};
    if (is_valid_button_input(&valid)) {
        passed_tests++;
        printf("✓ PASS: Valid input (A)\n");
    } else {
        printf("✗ FAIL: Valid input rejected\n");
    }
    
    total_tests++;
    ButtonInput invalid_note = {NOTE_NONE};
    if (!is_valid_button_input(&invalid_note)) {
        passed_tests++;
        printf("✓ PASS: Invalid note rejected (NOTE_NONE)\n");
    } else {
        printf("✗ FAIL: Invalid note accepted\n");
    }
    
    total_tests++;
    ButtonInput boundary_g = {NOTE_G};
    if (is_valid_button_input(&boundary_g)) {
        passed_tests++;
        printf("✓ PASS: Valid boundary note (G)\n");
    } else {
        printf("✗ FAIL: Valid boundary note rejected\n");
    }
}

/**
 * TEST 3: Button to frequency conversion (natural notes only)
 */
void run_button_to_frequency_tests(void) {
    printf("\n");
    printf("========================================================\n");
    printf("TEST 3: BUTTON TO FREQUENCY CONVERSION\n");
    printf("========================================================\n\n");
    
    printf("Octave 2 Notes:\n");
    ButtonInput btn_e2 = {NOTE_E};
    double target_e2 = button_to_frequency(&btn_e2, 82.41);
    assert_freq_equal(target_e2, 82.41, TOLERANCE, "E2 @ 82.41 Hz");
    
    ButtonInput btn_a2 = {NOTE_A};
    double target_a2 = button_to_frequency(&btn_a2, 110.00);
    assert_freq_equal(target_a2, 110.00, TOLERANCE, "A2 @ 110.00 Hz");
    
    printf("\nOctave 3 Notes:\n");
    ButtonInput btn_e3 = {NOTE_E};
    double target_e3 = button_to_frequency(&btn_e3, 164.81);
    assert_freq_equal(target_e3, 164.81, TOLERANCE, "E3 @ 164.81 Hz");
    
    ButtonInput btn_a3 = {NOTE_A};
    double target_a3 = button_to_frequency(&btn_a3, 220.00);
    assert_freq_equal(target_a3, 220.00, TOLERANCE, "A3 @ 220.00 Hz");
    
    printf("\nOctave 4 Notes:\n");
    ButtonInput btn_e4 = {NOTE_E};
    double target_e4 = button_to_frequency(&btn_e4, 329.63);
    assert_freq_equal(target_e4, 329.63, TOLERANCE, "E4 @ 329.63 Hz");
    
    ButtonInput btn_a4 = {NOTE_A};
    double target_a4 = button_to_frequency(&btn_a4, 440.00);
    assert_freq_equal(target_a4, 440.00, TOLERANCE, "A4 (reference) @ 440.00 Hz");
    
    ButtonInput btn_g4 = {NOTE_G};
    double target_g4 = button_to_frequency(&btn_g4, 392.00);
    assert_freq_equal(target_g4, 392.00, TOLERANCE, "G4 @ 392.00 Hz");
    
    printf("\nOctave 5 Notes:\n");
    ButtonInput btn_a5 = {NOTE_A};
    double target_a5 = button_to_frequency(&btn_a5, 880.00);
    assert_freq_equal(target_a5, 880.00, TOLERANCE, "A5 @ 880.00 Hz");
}

/**
 * TEST 4: Octave boundary transitions
 */
void run_octave_boundary_tests(void) {
    printf("\n");
    printf("========================================================\n");
    printf("TEST 4: OCTAVE BOUNDARY TRANSITIONS\n");
    printf("========================================================\n\n");
    
    printf("Boundary between Octave 2 and 3 (164 Hz):\n");
    assert_octave_equal(detect_octave_from_frequency(163.99), 2, "163.99 Hz → Octave 2");
    assert_octave_equal(detect_octave_from_frequency(164.00), 3, "164.00 Hz → Octave 3");
    assert_octave_equal(detect_octave_from_frequency(164.01), 3, "164.01 Hz → Octave 3");
    
    printf("\nBoundary between Octave 3 and 4 (328 Hz):\n");
    assert_octave_equal(detect_octave_from_frequency(327.99), 3, "327.99 Hz → Octave 3");
    assert_octave_equal(detect_octave_from_frequency(328.00), 4, "328.00 Hz → Octave 4");
    assert_octave_equal(detect_octave_from_frequency(328.01), 4, "328.01 Hz → Octave 4");
    
    printf("\nBoundary between Octave 4 and 5 (656 Hz):\n");
    assert_octave_equal(detect_octave_from_frequency(655.99), 4, "655.99 Hz → Octave 4");
    assert_octave_equal(detect_octave_from_frequency(656.00), 5, "656.00 Hz → Octave 5");
    assert_octave_equal(detect_octave_from_frequency(656.01), 5, "656.01 Hz → Octave 5");
}

/**
 * TEST 5: Octave doubling relationship
 * Each octave should have frequencies exactly 2x the previous octave
 */
void run_octave_relationship_tests(void) {
    printf("\n");
    printf("========================================================\n");
    printf("TEST 5: OCTAVE DOUBLING RELATIONSHIP\n");
    printf("========================================================\n\n");
    
    printf("Testing: A notes double across octaves\n");
    
    /* A2 = 110 Hz, A3 = 220 Hz, A4 = 440 Hz, A5 = 880 Hz */
    ButtonInput btn_a = {NOTE_A};
    
    double a2 = button_to_frequency(&btn_a, 110.00);   /* Octave 2 */
    double a3 = button_to_frequency(&btn_a, 220.00);   /* Octave 3 */
    double a4 = button_to_frequency(&btn_a, 440.00);   /* Octave 4 */
    double a5 = button_to_frequency(&btn_a, 880.00);   /* Octave 5 */
    
    printf("A2 = %.2f Hz\n", a2);
    printf("A3 = %.2f Hz (should be 2x A2)\n", a3);
    printf("A4 = %.2f Hz (should be 2x A3)\n", a4);
    printf("A5 = %.2f Hz (should be 2x A4)\n", a5);
    
    /* Verify octave doubling */
    total_tests++;
    if (fabs(a3 / a2 - 2.0) < 0.01) {
        passed_tests++;
        printf("✓ PASS: A3 / A2 = %.4f (should be 2.0)\n", a3 / a2);
    } else {
        printf("✗ FAIL: A3 / A2 = %.4f (should be 2.0)\n", a3 / a2);
    }
    
    total_tests++;
    if (fabs(a4 / a3 - 2.0) < 0.01) {
        passed_tests++;
        printf("✓ PASS: A4 / A3 = %.4f (should be 2.0)\n", a4 / a3);
    } else {
        printf("✗ FAIL: A4 / A3 = %.4f (should be 2.0)\n", a4 / a3);
    }
    
    total_tests++;
    if (fabs(a5 / a4 - 2.0) < 0.01) {
        passed_tests++;
        printf("✓ PASS: A5 / A4 = %.4f (should be 2.0)\n", a5 / a4);
    } else {
        printf("✗ FAIL: A5 / A4 = %.4f (should be 2.0)\n", a5 / a4);
    }
}

/**
 * TEST 6: All 7 notes in octave 4
 */
void run_all_notes_test(void) {
    printf("\n");
    printf("========================================================\n");
    printf("TEST 6: ALL 7 NATURAL NOTES IN OCTAVE 4\n");
    printf("========================================================\n\n");
    
    double freq_octave_4[] = {
        440.00,    /* A4 - reference */
        493.88,    /* B4 */
        261.63,    /* C4 (this is actually in octave 4 but lower) */
        293.66,    /* D4 */
        329.63,    /* E4 */
        349.23,    /* F4 */
        392.00     /* G4 */
    };
    
    NoteButton notes[] = {NOTE_A, NOTE_B, NOTE_C, NOTE_D, NOTE_E, NOTE_F, NOTE_G};
    const char* note_names[] = {"A4", "B4", "C4", "D4", "E4", "F4", "G4"};
    
    for (int i = 0; i < 7; i++) {
        ButtonInput btn = {notes[i]};
        double target = button_to_frequency(&btn, freq_octave_4[i]);
        assert_freq_equal(target, freq_octave_4[i], TOLERANCE, note_names[i]);
    }
}

/**
 * TEST 7: Invalid input handling
 */
void run_error_handling_tests(void) {
    printf("\n");
    printf("========================================================\n");
    printf("TEST 7: ERROR HANDLING\n");
    printf("========================================================\n\n");
    
    printf("Invalid button input:\n");
    ButtonInput invalid = {NOTE_NONE};
    double result = button_to_frequency(&invalid, 440.0);
    total_tests++;
    if (result < 0.0) {
        passed_tests++;
        printf("✓ PASS: Invalid note returns negative value (%.2f)\n", result);
    } else {
        printf("✗ FAIL: Invalid note should return negative, got %.2f\n", result);
    }
    
    printf("\nFrequency out of range:\n");
    ButtonInput valid = {NOTE_A};
    double result_low = button_to_frequency(&valid, 40.0);
    total_tests++;
    if (result_low < 0.0) {
        passed_tests++;
        printf("✓ PASS: Frequency too low returns negative (%.2f)\n", result_low);
    } else {
        printf("✗ FAIL: Frequency too low should return negative, got %.2f\n", result_low);
    }
    
    double result_high = button_to_frequency(&valid, 1500.0);
    total_tests++;
    if (result_high < 0.0) {
        passed_tests++;
        printf("✓ PASS: Frequency too high returns negative (%.2f)\n", result_high);
    } else {
        printf("✗ FAIL: Frequency too high should return negative, got %.2f\n", result_high);
    }
}

/**
 * Main test runner
 */
int main(void) {
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║   BUTTON INPUT SYSTEM - UNIT TESTS (NATURAL NOTES)    ║\n");
    printf("║   Testing 7-button interface (A-G, no modifiers)      ║\n");
    printf("║   With automatic octave detection from FFT frequency  ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    
    /* Run all tests */
    run_auto_octave_detection_tests();
    run_validation_tests();
    run_button_to_frequency_tests();
    run_octave_boundary_tests();
    run_octave_relationship_tests();
    run_all_notes_test();
    run_error_handling_tests();
    
    /* Print summary */
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                    TEST SUMMARY                        ║\n");
    printf("╠════════════════════════════════════════════════════════╣\n");
    printf("║  Total Tests:  %d\n", total_tests);
    printf("║  Passed:       %d\n", passed_tests);
    printf("║  Failed:       %d\n", total_tests - passed_tests);
    printf("║  Pass Rate:    %.1f%%\n", (100.0 * passed_tests) / total_tests);
    printf("╚════════════════════════════════════════════════════════╝\n\n");
    
    return (passed_tests == total_tests) ? 0 : 1;
}
