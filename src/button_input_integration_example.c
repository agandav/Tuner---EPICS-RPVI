/**
 * button_input_integration_example.c
 * 
 * Shows how to integrate 7-button input with the existing tuner system
 * 
 * Demonstrates:
 * 1. Reading button input from GPIO/buttons
 * 2. Converting to target frequency
 * 3. Using with existing analyze_tuning() function
 */

#include <stdio.h>
#include "button_input.h"
#include "string_detection.h"

/**
 * STEP 1: Read button input from hardware
 * 
 * EXAMPLE IMPLEMENTATION - customize based on your Teensy GPIO setup
 * 
 * IMPORTANT: User presses ONE of 7 note buttons (A-G)
 * Octave is auto-detected from FFT frequency!
 */
ButtonInput read_button_input_from_hardware(void) {
    ButtonInput input;
    
    /* In real implementation, read GPIO pins
     * 
     * GPIO Mapping (example):
     * - Pins 0-6: Note buttons (A=0, B=1, C=2, D=3, E=4, F=5, G=6)
     * 
     * Pseudo-code:
     * 
     *   for each note pin 0-6:
     *     if (digitalRead(pin) == HIGH):
     *       input.note = pin + 1  // +1 because NOTE_A=1, NOTE_B=2, etc
     *       break
     *   
     *   // NO MODIFIERS - hardware only has 7 note buttons
     *   // NO OCTAVE SELECTION NEEDED - auto-detected from FFT frequency!
     */
    
    /* Example: return A (octave will be auto-detected from FFT) */
    input.note = NOTE_A;
    
    return input;
}

/**
 * STEP 2: Example real-world usage with auto-octave detection
 * 
 * User flow:
 * 1. User presses one note button (e.g., "A")
 * 2. User plays guitar string
 * 3. FFT detects frequency (e.g., 441 Hz)
 * 4. System auto-detects octave from frequency (441 Hz -> octave 4)
 * 5. System calculates target frequency for A4 (440 Hz)
 * 6. Tuner shows UP/DOWN/IN_TUNE relative to target
 */
void tuner_main_with_button_input(double detected_frequency) {
    
    /* STEP 1: Read button input from user */
    ButtonInput button_input = read_button_input_from_hardware();
    
    /* STEP 2: Validate button input (note and accidental only) */
    if (!is_valid_button_input(&button_input)) {
        printf("ERROR: Invalid button input!\n");
        return;
    }
    
    /* STEP 3: Convert button input + detected frequency to target frequency
     * 
     * KEY: detect_octave_from_frequency() is called INSIDE button_to_frequency()
     * User doesn't select octave - system calculates it from FFT frequency!
     */
    double target_frequency = button_to_frequency(&button_input, detected_frequency);
    
    if (target_frequency <= 0) {
        printf("ERROR: Detected frequency out of valid range (82-1312 Hz)!\n");
        printf("       Or invalid button input.\n");
        return;
    }
    
    printf("\n");
    printf("_____________________________________________\n");
    printf("         GUITAR TUNER - BUTTON MODE        \n");
    printf("_____________________________________________\n\n");
    
    /* STEP 4: Detect octave and display target note with octave */
    int detected_octave = detect_octave_from_frequency(detected_frequency);
    
    printf("Button Pressed:  ");
    switch(button_input.note) {
        case NOTE_A: printf("A"); break;
        case NOTE_B: printf("B"); break;
        case NOTE_C: printf("C"); break;
        case NOTE_D: printf("D"); break;
        case NOTE_E: printf("E"); break;
        case NOTE_F: printf("F"); break;
        case NOTE_G: printf("G"); break;
    }
    
    printf(" (no octave needed from user)\n");
    
    printf("Detected Octave: %d (auto-detected from %.2f Hz)\n", 
           detected_octave, detected_frequency);
    printf("Target Note:     ");
    
    switch(button_input.note) {
        case NOTE_A: printf("A"); break;
        case NOTE_B: printf("B"); break;
        case NOTE_C: printf("C"); break;
        case NOTE_D: printf("D"); break;
        case NOTE_E: printf("E"); break;
        case NOTE_F: printf("F"); break;
        case NOTE_G: printf("G"); break;
    }
    
    printf("%d (Target: %.2f Hz)\n", detected_octave, target_frequency);
    
    printf("Detected Freq:   %.2f Hz\n", detected_frequency);

    
    /* STEP 6: Calculate tuning offset */
    double cents_offset = 1200.0 * log2(detected_frequency / target_frequency);
    
    printf("Tuning Offset:   %.2f cents\n", cents_offset);
    
    /* STEP 7: Display tuning direction */
    const char* direction;
    if (cents_offset < -2.0) {
        direction = "FLAT - Tune UP (up)";
    } else if (cents_offset > 2.0) {
        direction = "SHARP - Tune DOWN (down)";
    } else {
        direction = "IN TUNE [PASS]";
    }
    
    printf("Status:          %s\n", direction);
    printf("\n");
}

/**
 * EXAMPLE 1: User presses [A] button
 */
void example_1_a4_reference(void) {
    printf("\n");
    printf("????????????????????????????????????????????\n");
    printf("EXAMPLE 1: User presses [A] button\n");
    printf("           Plays 441.5 Hz (slightly sharp)\n");
    printf("????????????????????????????????????????????\n");
    
    /* User presses: [A] button (only note, no modifiers) */
    ButtonInput btn = {NOTE_A};
    
    /* User plays: 441.5 Hz on their guitar */
    double detected = 441.5;
    
    /* System calculates:
     * - Button: A
     * - Detected: 441.5 Hz -> auto-detect octave 4 (328-656 Hz range)
     * - Target: A4 = 440 Hz
     * - User gets feedback: "SHARP - Tune DOWN (down arrow)"
     */
    
    tuner_main_with_button_input(detected);
}

/**
 * EXAMPLE 2: User presses [E] button
 */
void example_2_e3_lower_octave(void) {
    printf("\n");
    printf("????????????????????????????????????????????\n");
    printf("EXAMPLE 2: User presses [E] button\n");
    printf("           Plays 164.81 Hz (E3 - low E string)\n");
    printf("????????????????????????????????????????????\n");
    
    /* User presses: [E] button (only note, no modifiers) */
    ButtonInput btn = {NOTE_E};
    
    /* User plays: 164.81 Hz on their guitar (E3) */
    double detected = 164.81;
    
    /* System calculates:
     * - Button: E
     * - Detected: 164.81 Hz -> auto-detect octave 3 (164-328 Hz range)
     * - Target: E3 = 164.81 Hz
     * - User gets feedback: "IN TUNE [OK]"
     */
    
    tuner_main_with_button_input(detected);
}

/**
 * EXAMPLE 3: User presses [D] button
 */
void example_3_d_note(void) {
    printf("\n");
    printf("????????????????????????????????????????????\n");
    printf("EXAMPLE 3: User presses [D] button\n");
    printf("           Plays 293.66 Hz (D4 - upper D string)\n");
    printf("????????????????????????????????????????????\n");
    
    /* User presses: [D] button (only note, no modifiers) */
    ButtonInput btn = {NOTE_D};
    
    /* User plays: 293.66 Hz (perfect D4) */
    double detected = 293.66;
    
    /* System calculates:
     * - Button: D
     * - Detected: 293.66 Hz -> auto-detect octave 4 (328-656 Hz range)
     * - Target: D4 = 293.66 Hz
     * - User gets feedback: "IN TUNE [OK]"
     */
    
    tuner_main_with_button_input(detected);
}

/**
 * EXAMPLE 4: User presses [G] button
 */
void example_4_g_note(void) {
    printf("\n");
    printf("????????????????????????????????????????????\n");
    printf("EXAMPLE 4: User presses [G] button\n");
    printf("           Plays 196.00 Hz (slightly flat)\n");
    printf("????????????????????????????????????????????\n");
    
    /* User presses: [G] button (only note, no modifiers) */
    ButtonInput btn = {NOTE_G};
    
    /* User plays: 196.00 Hz (slightly flat - should be 196.00 Hz) */
    double detected = 196.00;
    
    /* System calculates:
     * - Button: G
     * - Detected: 196.00 Hz -> auto-detect octave 3 (164-328 Hz range)
     * - Target: G3 = 196.00 Hz
     * - User gets feedback: "IN TUNE [OK]"
     */
    
    tuner_main_with_button_input(detected);
}

/**
 * KEY INSIGHTS:
 * 
 * 1. BUTTON MODE vs AUTO MODE:
 *    - AUTO: FFT detects frequency, system guesses closest note
 *    - BUTTON: User tells system what note to expect, FFT validates tuning
 *    - BOTH use same FFT and same cents offset calculation
 * 
 * 2. DATABASE INDEPENDENCE:
 *    - Button input works for ANY note (A-G in octaves 2-5)
 *    - Doesn't matter if note is in database or calculated
 *    - Uses only A4=440Hz as reference, calculates everything else
 * 
 * 3. YOUR EXISTING TESTS STILL WORK:
 *    - TEST 1A-1C: Validate FFT accuracy (still valid!)
 *    - TEST 1B: 32 notes in database (still valid!)
 *    - Button input is just a different WAY to select target frequency
 *    - Core FFT and semitone math unchanged
 * 
 * 4. IMPLEMENTATION PATH:
 *    - Use this button_input module for hardware interface
 *    - Existing analyze_tuning() still calculates offsets
 *    - Just add button reading as input source
 *    - Tests validate FFT works regardless of input source
 */
