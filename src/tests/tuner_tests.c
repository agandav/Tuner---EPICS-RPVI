/**
 * tuner_tests.c - Comprehensive test suite for guitar tuner
 * Run these tests to verify all algorithms work correctly
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "string_detection.h"
#include "audio_processing.h"
#include "audio_sequencer.h"

/**
 * Test cents calculation with known values
 */
void test_cents_calculation() {
    printf("\n=== TESTING CENTS CALCULATION ===\n");
    
    // Test perfect tuning (should be 0 cents)
    double cents = calculate_cents_offset(440.0, 440.0);
    printf("440.0 Hz vs 440.0 Hz: %.2f cents (expected: 0.00) - %s\n", 
           cents, fabs(cents) < 0.1 ? "PASS" : "FAIL");
    
    // Test sharp (positive cents)
    cents = calculate_cents_offset(445.0, 440.0);
    printf("445.0 Hz vs 440.0 Hz: %.2f cents (expected: +19.56) - %s\n", 
           cents, fabs(cents - 19.56) < 0.1 ? "PASS" : "FAIL");
    
    // Test flat (negative cents)  
    cents = calculate_cents_offset(435.0, 440.0);
    printf("435.0 Hz vs 440.0 Hz: %.2f cents (expected: -19.56) - %s\n", 
           cents, fabs(cents + 19.56) < 0.1 ? "PASS" : "FAIL");
    
    // Test exact semitone (100 cents) - A to A# from chart
    cents = calculate_cents_offset(466.0, 440.0);
    printf("466.0 Hz vs 440.0 Hz: %.2f cents (expected: ~100.00) - %s\n", 
           cents, fabs(cents - 100.0) < 1.0 ? "PASS" : "FAIL");
}

/**
 * Test string detection with standard guitar frequencies
 */
void test_string_detection() {
    printf("\n=== TESTING STRING DETECTION ===\n");
    
    // Test each guitar string's fundamental frequency
    // Frequencies verified against equal temperament standard tuning (A4 = 440 Hz)
    double test_frequencies[] = {
        82,    // String 6 - Low E
        110,   // String 5 - A  
        147,   // String 4 - D
        196,   // String 3 - G
        247,   // String 2 - B
        330    // String 1 - High E
    };
    
    const char* expected_strings[] = {"E2", "A2", "D3", "G3", "B3", "E4"};
    
    for (int i = 0; i < 6; i++) {
        TuningResult result = analyze_tuning_auto(test_frequencies[i]);
        printf("Frequency %.2f Hz: Detected String %d (%s) - %s\n", 
               test_frequencies[i], result.detected_string, result.note_name,
               (result.detected_string == (6-i)) ? "PASS" : "FAIL");
    }
}

/**
 * Test tuning direction logic
 */
void test_tuning_direction() {
    printf("\n=== TESTING TUNING DIRECTION ===\n");
    
    // Test cases: {detected_freq, target_freq, expected_direction}
    struct TestCase {
        double detected;
        double target;
        const char* expected;
    } test_cases[] = {
        {439.0, 440.0, "UP"},      // 3.9 cents flat
        {441.0, 440.0, "DOWN"},    // 3.9 cents sharp  
        {440.0, 440.0, "IN_TUNE"}, // Perfect
        {430.0, 440.0, "UP"},      // 38.9 cents flat
        {450.0, 440.0, "DOWN"},    // 38.9 cents sharp
        {440.5, 440.0, "IN_TUNE"}  // 2.0 cents - within tolerance
    };
    
    for (int i = 0; i < 6; i++) {
        double cents = calculate_cents_offset(test_cases[i].detected, test_cases[i].target);
        const char* direction = get_tuning_direction(cents);
        printf("%.1f Hz -> %.1f Hz: %s (expected: %s) - %s\n",
               test_cases[i].detected, test_cases[i].target, direction, test_cases[i].expected,
               strcmp(direction, test_cases[i].expected) == 0 ? "PASS" : "FAIL");
    }
}

/**
 * Test specific string tuning (user selects target string)
 */
void test_specific_string_tuning() {
    printf("\n=== TESTING SPECIFIC STRING TUNING ===\n");
    
    // Test tuning to A string (110 Hz) with various inputs
    double test_frequencies[] = {108.0, 110.0, 112.0, 105.0, 115.0};
    const char* expected_directions[] = {"UP", "IN_TUNE", "DOWN", "UP", "DOWN"};
    
    for (int i = 0; i < 5; i++) {
        TuningResult result = analyze_tuning(test_frequencies[i], 5); // String 5 = A
        printf("Input %.1f Hz -> Target A (110 Hz): %s, %.1f cents - %s\n",
               test_frequencies[i], result.direction, result.cents_offset,
               strcmp(result.direction, expected_directions[i]) == 0 ? "PASS" : "FAIL");
    }
}

/**
 * Test edge cases and error conditions
 */
void test_edge_cases() {
    printf("\n=== TESTING EDGE CASES ===\n");
    
    // Test very low frequency (below guitar range)
    TuningResult result = analyze_tuning_auto(50.0);
    printf("50.0 Hz: String %d - %s\n", result.detected_string, 
           result.detected_string == 6 ? "PASS (Low E)" : "CHECK");
    
    // Test very high frequency (above guitar range)
    result = analyze_tuning_auto(1000.0);
    printf("1000.0 Hz: String %d - %s\n", result.detected_string,
           result.detected_string == 1 ? "PASS (High E)" : "CHECK");
    
    // Test invalid string number
    result = analyze_tuning(440.0, 7); // Invalid string
    printf("Invalid string 7: String %d - %s\n", result.detected_string,
           result.detected_string > 0 ? "PASS (auto-detected)" : "FAIL");
    
    // Test zero frequency
    result = analyze_tuning_auto(0.0);
    printf("0.0 Hz: Direction %s - %s\n", result.direction,
           strcmp(result.direction, "UNKNOWN") == 0 ? "PASS" : "FAIL");
}

/**
 * Test audio sequencing logic
 */
void test_audio_sequencing() {
    printf("\n=== TESTING AUDIO SEQUENCING ===\n");
    
    // Create test tuning results with frequencies from equal temperament chart
    // Using realistic FFT detection variations (small decimal errors from interpolation)
    TuningResult test_cases[] = {
        {5, 5, 0.5, "IN_TUNE", 110.31, 110.0, "A", 2},      // A string, 110.31 Hz detected (slight FFT variation)
        {1, 1, -0.3, "IN_TUNE", 329.87, 330.0, "E", 4},     // E string, 329.87 Hz detected (slight FFT variation)
        {3, 3, 0.2, "IN_TUNE", 196.15, 196.0, "G", 3},      // G string, 196.15 Hz detected (slight FFT variation)
        {2, 2, -0.4, "IN_TUNE", 247.42, 247.0, "B", 3},     // B string, 247.42 Hz detected (slight FFT variation)
    };
    
    for (int i = 0; i < 4; i++) {
        printf("\nTest Case %d:\n", i+1);
        printf("  String %d, %.1f cents, Direction: %s\n", 
               test_cases[i].detected_string, test_cases[i].cents_offset, test_cases[i].direction);
        generate_audio_feedback(&test_cases[i]);
        
        // Simulate audio playback steps
        for (int step = 0; step < 4; step++) {
            audio_sequencer_update();
        }
    }
}

/**
 * Run complete test suite
 */
void run_all_tests() {
    printf("Starting Guitar Tuner Test Suite\n");
    printf("===============================\n");
    
    test_cents_calculation();
    test_string_detection(); 
    test_tuning_direction();
    test_specific_string_tuning();
    test_edge_cases();
    test_audio_sequencing();
    
    printf("\n=== TEST SUITE COMPLETE ===\n");
}

int main() {
    // Initialize modules
    string_detection_init();
    audio_sequencer_init();
    
    // Run tests
    run_all_tests();
    
    return 0;
}
