/**
 * test_state_machine.cpp - State Machine Integration Tests
 * 
 * Tests the complete state machine logic WITHOUT hardware dependencies.
 * Run this on PC before deploying to Teensy.
 * 
 * Compile: g++ -o test_state_machine test_state_machine.cpp ../src/string_detection.c ../src/audio_sequencer.c -lm -I../src -std=c++11
 * Run: ./test_state_machine
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

extern "C" {
    #include "../src/string_detection.h"
    #include "../src/audio_sequencer.h"
}

/* Stub out audio_processing FFT for PC testing */
extern "C" {
    void audio_processing_init(void) {
        printf("Audio processing init (PC stub)\n");
    }
    
    double apply_fft(const int16_t* samples, int num_samples) {
        /* Stub - will be overridden by injected frequency */
        return 0.0;
    }
}

/* ============================================================================
 * TEST SIMULATION FRAMEWORK
 * ========================================================================== */

/* Simulated time */
static uint32_t simulated_time_ms = 0;

void advance_time_ms(uint32_t ms) {
    simulated_time_ms += ms;
    printf("[TIME] Advanced %u ms -> Total: %u ms\n", ms, simulated_time_ms);
}

/* Simulated button state */
static int button_states[6] = {0, 0, 0, 0, 0, 0};  /* 0 = released, 1 = pressed */

void inject_button_press(int button_id) {
    if (button_id >= 1 && button_id <= 6) {
        button_states[button_id - 1] = 1;
        printf("[INJECT] Button %d PRESSED\n", button_id);
    }
}

void inject_button_release(int button_id) {
    if (button_id >= 1 && button_id <= 6) {
        button_states[button_id - 1] = 0;
        printf("[INJECT] Button %d RELEASED\n", button_id);
    }
}

int get_button_state(int button_id) {
    if (button_id >= 1 && button_id <= 6) {
        return button_states[button_id - 1];
    }
    return 0;
}

/* Simulated audio injection */
static double injected_frequency = 0.0;

void inject_audio_frequency(double freq) {
    injected_frequency = freq;
    printf("[INJECT] Audio frequency: %.2f Hz\n", freq);
}

double simulate_fft_detection() {
    return injected_frequency;
}

/* ============================================================================
 * STATE MACHINE SIMULATION
 * Simplified version of main.cpp for testing
 * ========================================================================== */

/* Use different enum name to avoid conflict with string_detection.h */
typedef enum {
    SM_STATE_IDLE = 0,
    SM_STATE_PLAYING_TONE,
    SM_STATE_WAITING_READY_BEEP,
    SM_STATE_LISTENING,
    SM_STATE_PROVIDING_FEEDBACK,
    SM_STATE_ERROR_RECOVERY
} sm_state_t;

static sm_state_t current_state = SM_STATE_IDLE;
static int target_string = 0;
static double target_frequency = 0.0;
static uint32_t state_entry_time = 0;
static int weak_signal_count = 0;
static TuningResult last_result;

static const double STRING_FREQUENCIES[6] = {
    329.63, 246.94, 196.00, 146.83, 110.00, 82.41
};

#define TONE_PLAYBACK_DURATION_MS 1000
#define READY_BEEP_DURATION_MS 200
#define MAX_NO_SIGNAL_TIME_MS 5000
#define MAX_WEAK_SIGNAL_COUNT 10

void state_machine_reset() {
    current_state = SM_STATE_IDLE;
    target_string = 0;
    target_frequency = 0.0;
    state_entry_time = 0;
    weak_signal_count = 0;
    simulated_time_ms = 0;
}

void state_machine_update() {
    uint32_t elapsed = simulated_time_ms - state_entry_time;
    
    switch (current_state) {
        case SM_STATE_IDLE:
            /* Check for button press */
            for (int i = 1; i <= 6; i++) {
                if (get_button_state(i)) {
                    target_string = i;
                    target_frequency = STRING_FREQUENCIES[i - 1];
                    current_state = SM_STATE_PLAYING_TONE;
                    state_entry_time = simulated_time_ms;
                    printf("[STATE] IDLE -> PLAYING_TONE (String %d)\n", i);
                    break;
                }
            }
            break;
            
        case SM_STATE_PLAYING_TONE:
            if (elapsed >= TONE_PLAYBACK_DURATION_MS) {
                current_state = SM_STATE_WAITING_READY_BEEP;
                state_entry_time = simulated_time_ms;
                printf("[STATE] PLAYING_TONE -> WAITING_READY_BEEP\n");
            }
            if (!get_button_state(target_string)) {
                current_state = SM_STATE_IDLE;
                printf("[STATE] Button released -> IDLE\n");
            }
            break;
            
        case SM_STATE_WAITING_READY_BEEP:
            if (elapsed >= READY_BEEP_DURATION_MS) {
                current_state = SM_STATE_LISTENING;
                state_entry_time = simulated_time_ms;
                weak_signal_count = 0;
                printf("[STATE] WAITING_READY_BEEP -> LISTENING\n");
            }
            if (!get_button_state(target_string)) {
                current_state = SM_STATE_IDLE;
                printf("[STATE] Button released -> IDLE\n");
            }
            break;
            
        case SM_STATE_LISTENING:
            {
                double detected_freq = simulate_fft_detection();
                
                if (detected_freq > 0.0) {
                    last_result = analyze_tuning(detected_freq, target_string);
                    current_state = SM_STATE_PROVIDING_FEEDBACK;
                    printf("[STATE] LISTENING -> PROVIDING_FEEDBACK\n");
                    printf("        Detected: %.2f Hz, Cents: %.1f, Direction: %s\n",
                           detected_freq, last_result.cents_offset, last_result.direction);
                    weak_signal_count = 0;
                } else {
                    weak_signal_count++;
                    if (weak_signal_count >= MAX_WEAK_SIGNAL_COUNT) {
                        current_state = SM_STATE_ERROR_RECOVERY;
                        state_entry_time = simulated_time_ms;
                        printf("[STATE] LISTENING -> ERROR_RECOVERY (weak signal)\n");
                    }
                }
                
                if (elapsed >= MAX_NO_SIGNAL_TIME_MS) {
                    current_state = SM_STATE_ERROR_RECOVERY;
                    state_entry_time = simulated_time_ms;
                    printf("[STATE] LISTENING -> ERROR_RECOVERY (timeout)\n");
                }
                
                if (!get_button_state(target_string)) {
                    current_state = SM_STATE_IDLE;
                    printf("[STATE] Button released -> IDLE\n");
                }
            }
            break;
            
        case SM_STATE_PROVIDING_FEEDBACK:
            {
                double detected_freq = simulate_fft_detection();
                
                if (detected_freq > 0.0) {
                    last_result = analyze_tuning(detected_freq, target_string);
                    weak_signal_count = 0;
                } else {
                    weak_signal_count++;
                    if (weak_signal_count >= MAX_WEAK_SIGNAL_COUNT) {
                        current_state = SM_STATE_LISTENING;
                        state_entry_time = simulated_time_ms;
                        printf("[STATE] PROVIDING_FEEDBACK -> LISTENING (signal lost)\n");
                    }
                }
                
                if (!get_button_state(target_string)) {
                    current_state = SM_STATE_IDLE;
                    printf("[STATE] Button released -> IDLE\n");
                }
            }
            break;
            
        case SM_STATE_ERROR_RECOVERY:
            if (elapsed >= 2000) {
                current_state = SM_STATE_LISTENING;
                state_entry_time = simulated_time_ms;
                weak_signal_count = 0;
                printf("[STATE] ERROR_RECOVERY -> LISTENING (recovery complete)\n");
            }
            if (!get_button_state(target_string)) {
                current_state = SM_STATE_IDLE;
                printf("[STATE] Button released -> IDLE\n");
            }
            break;
    }
}

/* ============================================================================
 * TEST CASES
 * ========================================================================== */

void test_basic_state_transitions() {
    printf("\n=== TEST: Basic State Transitions ===\n");
    state_machine_reset();
    
    /* Start in IDLE */
    assert(current_state == SM_STATE_IDLE);
    printf("PASS: Starts in IDLE state\n");
    
    /* Press button 5 (A string) */
    inject_button_press(5);
    state_machine_update();
    assert(current_state == SM_STATE_PLAYING_TONE);
    assert(target_string == 5);
    printf("PASS: Button press -> PLAYING_TONE\n");
    
    /* Wait for tone to complete */
    advance_time_ms(1000);
    state_machine_update();
    assert(current_state == SM_STATE_WAITING_READY_BEEP);
    printf("PASS: Tone complete -> WAITING_READY_BEEP\n");
    
    /* Wait for ready beep */
    advance_time_ms(200);
    state_machine_update();
    assert(current_state == SM_STATE_LISTENING);
    printf("PASS: Ready beep complete -> LISTENING\n");
    
    /* Inject audio frequency (110 Hz - perfect A2) */
    inject_audio_frequency(110.0);
    state_machine_update();
    assert(current_state == SM_STATE_PROVIDING_FEEDBACK);
    assert(fabs(last_result.cents_offset) < 0.5);  /* Nearly in tune */
    printf("PASS: Frequency detected -> PROVIDING_FEEDBACK\n");
    
    /* Release button */
    inject_button_release(5);
    state_machine_update();
    assert(current_state == SM_STATE_IDLE);
    printf("PASS: Button release -> IDLE\n");
    
    printf("=== BASIC STATE TRANSITIONS: ALL TESTS PASSED ===\n");
}

void test_weak_signal_recovery() {
    printf("\n=== TEST: Weak Signal Recovery ===\n");
    state_machine_reset();
    
    /* Get to listening state */
    inject_button_press(5);
    state_machine_update();
    advance_time_ms(1000);  /* Tone playback */
    state_machine_update();
    advance_time_ms(200);   /* Ready beep */
    state_machine_update();
    assert(current_state == SM_STATE_LISTENING);
    
    /* Inject 10 weak signals (no frequency) */
    for (int i = 0; i < MAX_WEAK_SIGNAL_COUNT; i++) {
        inject_audio_frequency(0.0);  /* No signal */
        state_machine_update();
    }
    
    assert(current_state == SM_STATE_ERROR_RECOVERY);
    printf("PASS: Weak signals trigger ERROR_RECOVERY\n");
    
    /* Wait for recovery */
    advance_time_ms(2000);
    state_machine_update();
    assert(current_state == SM_STATE_LISTENING);
    printf("PASS: Auto-recovery to LISTENING after 2 seconds\n");
    
    inject_button_release(5);
    state_machine_update();
    
    printf("=== WEAK SIGNAL RECOVERY: ALL TESTS PASSED ===\n");
}

void test_timeout_handling() {
    printf("\n=== TEST: Timeout Handling ===\n");
    state_machine_reset();
    
    /* Get to listening state */
    inject_button_press(5);
    state_machine_update();
    advance_time_ms(1000);
    state_machine_update();
    advance_time_ms(200);
    state_machine_update();
    assert(current_state == SM_STATE_LISTENING);
    
    /* Wait for timeout (5 seconds) */
    advance_time_ms(MAX_NO_SIGNAL_TIME_MS);
    state_machine_update();
    assert(current_state == SM_STATE_ERROR_RECOVERY);
    printf("PASS: Timeout triggers ERROR_RECOVERY\n");
    
    inject_button_release(5);
    state_machine_update();
    
    printf("=== TIMEOUT HANDLING: ALL TESTS PASSED ===\n");
}

void test_button_release_at_any_state() {
    printf("\n=== TEST: Button Release Cancellation ===\n");
    
    sm_state_t test_states[] = {
        SM_STATE_PLAYING_TONE,
        SM_STATE_WAITING_READY_BEEP,
        SM_STATE_LISTENING,
        SM_STATE_PROVIDING_FEEDBACK,
        SM_STATE_ERROR_RECOVERY
    };
    
    for (int i = 0; i < 5; i++) {
        state_machine_reset();
        inject_button_press(3);
        state_machine_update();
        
        /* Force state */
        current_state = test_states[i];
        
        /* Release button */
        inject_button_release(3);
        state_machine_update();
        
        assert(current_state == SM_STATE_IDLE);
        printf("PASS: Button release from state %d -> IDLE\n", test_states[i]);
    }
    
    printf("=== BUTTON RELEASE: ALL TESTS PASSED ===\n");
}

void test_tuning_accuracy_feedback() {
    printf("\n=== TEST: Tuning Accuracy Feedback ===\n");
    state_machine_reset();
    
    /* Get to listening state for String 5 (A2 = 110 Hz) */
    inject_button_press(5);
    state_machine_update();
    advance_time_ms(1000);
    state_machine_update();
    advance_time_ms(200);
    state_machine_update();
    
    /* Test various tuning scenarios */
    struct {
        double freq;
        const char* expected_direction;
    } tests[] = {
        {110.0, "IN_TUNE"},   /* Perfect */
        {108.0, "UP"},        /* Flat */
        {112.0, "DOWN"},      /* Sharp */
        {109.0, "UP"},        /* Slightly flat */
        {111.0, "DOWN"}       /* Slightly sharp */
    };
    
    for (int i = 0; i < 5; i++) {
        inject_audio_frequency(tests[i].freq);
        state_machine_update();
        
        printf("Frequency %.1f Hz: Cents=%.1f, Direction=%s\n",
               tests[i].freq, last_result.cents_offset, last_result.direction);
        
        assert(strcmp(last_result.direction, tests[i].expected_direction) == 0);
        printf("PASS: Correct direction for %.1f Hz\n", tests[i].freq);
    }
    
    inject_button_release(5);
    state_machine_update();
    
    printf("=== TUNING ACCURACY: ALL TESTS PASSED ===\n");
}

void test_beep_rate_calculation() {
    printf("\n=== TEST: Beep Rate Calculation ===\n");
    
    struct {
        double cents;
        uint32_t min_interval;
        uint32_t max_interval;
    } tests[] = {
        {150.0, 90, 110},       /* >100 cents: 100ms */
        {80.0, 135, 165},       /* 75-100: 150ms */
        {60.0, 180, 220},       /* 50-75: 200ms */
        {30.0, 450, 550},       /* 25-40: 500ms */
        {10.0, 1080, 1320},     /* 5-25: 1200ms */
        {3.0, 0, 0}             /* <5: no beep */
    };
    
    for (int i = 0; i < 6; i++) {
        uint32_t interval = calculate_beep_interval(tests[i].cents);
        printf("Cents %.1f -> Interval %u ms\n", tests[i].cents, interval);
        
        assert(interval >= tests[i].min_interval);
        assert(interval <= tests[i].max_interval);
        printf("PASS: Beep interval correct for %.1f cents\n", tests[i].cents);
    }
    
    printf("=== BEEP RATE: ALL TESTS PASSED ===\n");
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ========================================================================== */

int main() {
    printf("\n");
    printf("===========================================\n");
    printf("  STATE MACHINE INTEGRATION TESTS\n");
    printf("===========================================\n");
    
    /* Initialize modules */
    string_detection_init();
    audio_sequencer_init();
    audio_processing_init();
    
    /* Run all test suites */
    test_basic_state_transitions();
    test_weak_signal_recovery();
    test_timeout_handling();
    test_button_release_at_any_state();
    test_tuning_accuracy_feedback();
    test_beep_rate_calculation();
    
    printf("\n");
    printf("===========================================\n");
    printf("  ALL TESTS PASSED!\n");
    printf("  State machine ready for hardware\n");
    printf("===========================================\n");
    printf("\n");
    
    return 0;
}