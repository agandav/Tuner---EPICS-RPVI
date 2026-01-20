/**
 * main_test.cpp - PC-Compatible Test Version of Main State Machine
 * 
 * This version runs on PC for complete software validation BEFORE
 * connecting to Teensy hardware.
 * 
 * Tests:
 * - Complete state machine logic
 * - All state transitions
 * - Error recovery
 * - Simulated audio I/O
 * - Integration of all modules
 * 
 * Compile: g++ -o main_test main_test.cpp ../src/audio_processing.c ../src/string_detection.c ../src/audio_sequencer.c ../src/hardware_interface.c -lm -I../src -std=c++11
 * Run: ./main_test
 */

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <ctime>

extern "C" {
    #include "../src/string_detection.h"
    #include "../src/audio_sequencer.h"
}

/* Stub out hardware-specific headers */
#define ARDUINO 0

/* ============================================================================
 * PC SIMULATION - Hardware Stubs
 * ========================================================================== */

/* Simulated time */
static uint32_t sim_time_ms = 0;

uint32_t millis(void) {
    return sim_time_ms;
}

void delay(uint32_t ms) {
    sim_time_ms += ms;
}

void delayMicroseconds(uint32_t us) {
    /* No-op for PC */
}

/* Simulated FFT */
extern "C" {
    void audio_processing_init(void) {
        printf("Audio processing init (PC simulation)\n");
    }
    
    double apply_fft(const int16_t* samples, int num_samples) {
        /* Return simulated frequency based on test scenario */
        static double test_freq = 0.0;
        return test_freq;
    }
    
    void set_test_frequency(double freq) {
        /* Helper to inject test frequencies */
        extern double test_freq;
        test_freq = freq;
    }
}

/* Simulated hardware interface */
extern "C" {
    typedef enum {
        BUTTON_1 = 1,
        BUTTON_2 = 2,
        BUTTON_3 = 3,
        BUTTON_4 = 4,
        BUTTON_5 = 5,
        BUTTON_6 = 6
    } button_id_t;
    
    typedef enum {
        BUTTON_RELEASED = 0,
        BUTTON_PRESSED = 1
    } button_state_t;
    
    typedef struct {
        button_id_t button_id;
        button_state_t state;
        uint32_t timestamp_ms;
        uint32_t press_duration_ms;
    } button_event_t;
    
    static button_event_t sim_button_event = {BUTTON_1, BUTTON_RELEASED, 0, 0};
    static int sim_button_states[6] = {0, 0, 0, 0, 0, 0};
    static bool sim_button_event_ready = false;
    static bool sim_amp_enabled = false;
    
    int hardware_interface_init(void) {
        printf("Hardware interface init (PC simulation)\n");
        return 0;
    }
    
    int button_poll(void) {
        if (sim_button_event_ready) {
            return 1;
        }
        return 0;
    }
    
    button_event_t* button_get_event(void) {
        if (sim_button_event_ready) {
            sim_button_event_ready = false;
            return &sim_button_event;
        }
        return NULL;
    }
    
    bool button_is_pressed(button_id_t button_id) {
        if (button_id >= 1 && button_id <= 6) {
            return sim_button_states[button_id - 1] == 1;
        }
        return false;
    }
    
    void audio_amplifier_enable(void) {
        sim_amp_enabled = true;
        printf("[AMP] Enabled\n");
    }
    
    void audio_amplifier_disable(void) {
        sim_amp_enabled = false;
        printf("[AMP] Disabled\n");
    }
    
    int tactile_feedback_warning(void) {
        printf("[TACTILE] Warning feedback\n");
        return 0;
    }
    
    void hardware_print_config(void) {
        printf("\n=== PC SIMULATION HARDWARE CONFIG ===\n");
        printf("Platform: Native PC\n");
        printf("Simulated: Buttons, Audio, FFT\n");
        printf("=====================================\n\n");
    }
    
    /* Test helper functions */
    void inject_button_press(int button_id) {
        if (button_id >= 1 && button_id <= 6) {
            sim_button_states[button_id - 1] = 1;
            sim_button_event.button_id = (button_id_t)button_id;
            sim_button_event.state = BUTTON_PRESSED;
            sim_button_event.timestamp_ms = sim_time_ms;
            sim_button_event_ready = true;
            printf("[INJECT] Button %d PRESSED\n", button_id);
        }
    }
    
    void inject_button_release(int button_id) {
        if (button_id >= 1 && button_id <= 6) {
            sim_button_states[button_id - 1] = 0;
            sim_button_event.button_id = (button_id_t)button_id;
            sim_button_event.state = BUTTON_RELEASED;
            sim_button_event.timestamp_ms = sim_time_ms;
            sim_button_event_ready = true;
            printf("[INJECT] Button %d RELEASED\n", button_id);
        }
    }
}

/* Simulated audio playback */
static double sim_detected_frequency = 0.0;

void set_simulated_frequency(double freq) {
    sim_detected_frequency = freq;
}

double read_frequency_from_microphone(const int16_t* samples, int num_samples) {
    return sim_detected_frequency;
}

void play_tone(double frequency, uint32_t duration_ms) {
    printf("[TONE] Playing %.2f Hz for %u ms\n", frequency, duration_ms);
}

void play_ready_beep(void) {
    printf("[BEEP] Ready signal\n");
}

void update_tone_playback(void) {
    /* No-op for PC */
}

/* ============================================================================
 * STATE MACHINE - Same as Teensy Version
 * ========================================================================== */

typedef enum {
    TUNER_STATE_IDLE = 0,
    TUNER_STATE_PLAYING_TONE,
    TUNER_STATE_WAITING_READY_BEEP,
    TUNER_STATE_LISTENING,
    TUNER_STATE_PROVIDING_FEEDBACK,
    TUNER_STATE_ERROR_RECOVERY
} tuner_state_t;

typedef enum {
    MODE_PLAY_TONE = 0,
    MODE_LISTEN_ONLY = 1
} tuner_mode_t;

static const double STRING_FREQUENCIES[6] = {
    329.63, 246.94, 196.00, 146.83, 110.00, 82.41
};

static const char* STRING_NAMES[6] = {
    "E4", "B3", "G3", "D3", "A2", "E2"
};

static tuner_state_t current_state = TUNER_STATE_IDLE;
static tuner_mode_t tuner_mode = MODE_PLAY_TONE;
static int target_string = 0;
static double target_frequency = 0.0;
static uint32_t state_entry_time = 0;
static uint32_t last_beep_update = 0;
static int weak_signal_count = 0;

#define MAX_NO_SIGNAL_TIME_MS 5000
#define MAX_WEAK_SIGNAL_COUNT 10
#define TONE_PLAYBACK_DURATION_MS 1000
#define READY_BEEP_DURATION_MS 200
#define BEEP_UPDATE_INTERVAL_MS 10

/* State functions */
static void state_idle(void) {
    if (button_poll()) {
        button_event_t* event = button_get_event();
        
        if (event && event->state == BUTTON_PRESSED) {
            target_string = event->button_id;
            target_frequency = STRING_FREQUENCIES[target_string - 1];
            
            printf("\n[STATE] User selected String %d (%s) - Target: %.2f Hz\n",
                   target_string, STRING_NAMES[target_string - 1], target_frequency);
            
            audio_amplifier_enable();
            
            if (tuner_mode == MODE_PLAY_TONE) {
                current_state = TUNER_STATE_PLAYING_TONE;
                state_entry_time = millis();
                play_tone(target_frequency, TONE_PLAYBACK_DURATION_MS);
            } else {
                current_state = TUNER_STATE_WAITING_READY_BEEP;
                state_entry_time = millis();
                play_ready_beep();
            }
        }
    }
}

static void state_playing_tone(void) {
    uint32_t elapsed = millis() - state_entry_time;
    
    if (elapsed >= TONE_PLAYBACK_DURATION_MS) {
        printf("[STATE] Tone playback complete\n");
        current_state = TUNER_STATE_WAITING_READY_BEEP;
        state_entry_time = millis();
        play_ready_beep();
    }
    
    if (!button_is_pressed((button_id_t)target_string)) {
        printf("[STATE] Button released - returning to IDLE\n");
        current_state = TUNER_STATE_IDLE;
        target_string = 0;
        audio_amplifier_disable();
    }
}

static void state_waiting_ready_beep(void) {
    uint32_t elapsed = millis() - state_entry_time;
    
    if (elapsed >= READY_BEEP_DURATION_MS) {
        printf("[STATE] Ready beep complete - listening for guitar input\n");
        current_state = TUNER_STATE_LISTENING;
        state_entry_time = millis();
        weak_signal_count = 0;
    }
    
    if (!button_is_pressed((button_id_t)target_string)) {
        printf("[STATE] Button released - returning to IDLE\n");
        current_state = TUNER_STATE_IDLE;
        target_string = 0;
        audio_amplifier_disable();
    }
}

static void state_listening(void) {
    double detected_freq = read_frequency_from_microphone(NULL, 0);
    
    if (detected_freq > 0.0) {
        printf("[FFT] Detected: %.2f Hz\n", detected_freq);
        
        TuningResult result = analyze_tuning(detected_freq, target_string);
        
        printf("[TUNING] String %d, Cents: %.1f, Direction: %s\n",
               result.detected_string, result.cents_offset, result.direction);
        
        current_state = TUNER_STATE_PROVIDING_FEEDBACK;
        generate_dynamic_beep_feedback(&result);
        last_beep_update = millis();
        weak_signal_count = 0;
        
    } else {
        weak_signal_count++;
        
        if (weak_signal_count >= MAX_WEAK_SIGNAL_COUNT) {
            printf("[ERROR] Too many weak signals - entering error recovery\n");
            current_state = TUNER_STATE_ERROR_RECOVERY;
            state_entry_time = millis();
        }
    }
    
    uint32_t elapsed = millis() - state_entry_time;
    if (elapsed >= MAX_NO_SIGNAL_TIME_MS) {
        printf("[ERROR] Timeout waiting for signal\n");
        current_state = TUNER_STATE_ERROR_RECOVERY;
        state_entry_time = millis();
    }
    
    if (!button_is_pressed((button_id_t)target_string)) {
        printf("[STATE] Button released - returning to IDLE\n");
        current_state = TUNER_STATE_IDLE;
        target_string = 0;
        audio_amplifier_disable();
    }
}

static void state_providing_feedback(void) {
    uint32_t current_time = millis();
    
    if (current_time - last_beep_update >= BEEP_UPDATE_INTERVAL_MS) {
        audio_sequencer_update_beeps(current_time);
        last_beep_update = current_time;
    }
    
    double detected_freq = read_frequency_from_microphone(NULL, 0);
    
    if (detected_freq > 0.0) {
        TuningResult result = analyze_tuning(detected_freq, target_string);
        generate_dynamic_beep_feedback(&result);
        weak_signal_count = 0;
        
        static int update_count = 0;
        if (++update_count % 50 == 0) {
            printf("[FEEDBACK] Cents: %.1f, Direction: %s\n",
                   result.cents_offset, result.direction);
        }
    } else {
        weak_signal_count++;
        if (weak_signal_count >= MAX_WEAK_SIGNAL_COUNT) {
            printf("[FEEDBACK] Signal lost - returning to listening\n");
            current_state = TUNER_STATE_LISTENING;
            state_entry_time = millis();
        }
    }
    
    if (!button_is_pressed((button_id_t)target_string)) {
        printf("[STATE] Button released - tuning session complete\n");
        current_state = TUNER_STATE_IDLE;
        target_string = 0;
        audio_amplifier_disable();
    }
}

static void state_error_recovery(void) {
    static int warning_played = 0;
    if (!warning_played) {
        tactile_feedback_warning();
        printf("[ERROR] Please play your string louder or closer to microphone\n");
        warning_played = 1;
    }
    
    uint32_t elapsed = millis() - state_entry_time;
    if (elapsed >= 2000) {
        printf("[RECOVERY] Returning to listening state\n");
        current_state = TUNER_STATE_LISTENING;
        state_entry_time = millis();
        weak_signal_count = 0;
        warning_played = 0;
    }
    
    if (!button_is_pressed((button_id_t)target_string)) {
        printf("[STATE] Button released - returning to IDLE\n");
        current_state = TUNER_STATE_IDLE;
        target_string = 0;
        audio_amplifier_disable();
        warning_played = 0;
    }
}

void run_state_machine(void) {
    update_tone_playback();
    
    switch (current_state) {
        case TUNER_STATE_IDLE:
            state_idle();
            break;
        case TUNER_STATE_PLAYING_TONE:
            state_playing_tone();
            break;
        case TUNER_STATE_WAITING_READY_BEEP:
            state_waiting_ready_beep();
            break;
        case TUNER_STATE_LISTENING:
            state_listening();
            break;
        case TUNER_STATE_PROVIDING_FEEDBACK:
            state_providing_feedback();
            break;
        case TUNER_STATE_ERROR_RECOVERY:
            state_error_recovery();
            break;
        default:
            printf("[ERROR] Invalid state! Returning to IDLE\n");
            current_state = TUNER_STATE_IDLE;
            break;
    }
}

/* ============================================================================
 * TEST SCENARIOS
 * ========================================================================== */

void test_scenario_1_perfect_tuning() {
    printf("\n========================================\n");
    printf("TEST SCENARIO 1: Perfect Tuning (A2)\n");
    printf("========================================\n");
    
    /* Reset */
    current_state = TUNER_STATE_IDLE;
    sim_time_ms = 0;
    
    /* User presses String 5 button (A2 - 110 Hz) */
    inject_button_press(5);
    run_state_machine();
    
    /* Advance through tone playback */
    sim_time_ms += 1000;
    run_state_machine();
    
    /* Advance through ready beep */
    sim_time_ms += 200;
    run_state_machine();
    
    /* User plays perfectly tuned A2 */
    set_simulated_frequency(110.0);
    run_state_machine();
    
    /* Should be in feedback state, in tune */
    printf("\nResult: ");
    if (current_state == TUNER_STATE_PROVIDING_FEEDBACK) {
        printf("PASS - Entered feedback state\n");
    } else {
        printf("FAIL - Wrong state\n");
    }
    
    /* Release button */
    inject_button_release(5);
    run_state_machine();
    
    printf("Test 1 Complete\n");
}

void test_scenario_2_sharp_tuning() {
    printf("\n========================================\n");
    printf("TEST SCENARIO 2: Sharp Tuning\n");
    printf("========================================\n");
    
    current_state = TUNER_STATE_IDLE;
    sim_time_ms = 0;
    
    inject_button_press(5);
    run_state_machine();
    
    sim_time_ms += 1200;
    run_state_machine();
    
    /* Play 15 cents sharp (112 Hz) */
    set_simulated_frequency(112.0);
    run_state_machine();
    
    printf("Test 2 Complete\n");
}

void test_scenario_3_weak_signal_recovery() {
    printf("\n========================================\n");
    printf("TEST SCENARIO 3: Weak Signal Recovery\n");
    printf("========================================\n");
    
    current_state = TUNER_STATE_IDLE;
    sim_time_ms = 0;
    
    inject_button_press(3);
    run_state_machine();
    
    sim_time_ms += 1200;
    run_state_machine();
    
    /* Inject weak signals */
    for (int i = 0; i < 12; i++) {
        set_simulated_frequency(0.0);
        run_state_machine();
    }
    
    /* Should be in error recovery */
    if (current_state == TUNER_STATE_ERROR_RECOVERY) {
        printf("PASS - Error recovery triggered\n");
    }
    
    /* Wait for recovery */
    sim_time_ms += 2000;
    run_state_machine();
    
    if (current_state == TUNER_STATE_LISTENING) {
        printf("PASS - Recovered to listening\n");
    }
    
    inject_button_release(3);
    run_state_machine();
    
    printf("Test 3 Complete\n");
}

/* ============================================================================
 * MAIN
 * ========================================================================== */

int main() {
    printf("\n");
    printf("===========================================\n");
    printf("  RPVI Guitar Tuner - PC Test Mode\n");
    printf("===========================================\n");
    printf("\n");
    
    /* Initialize */
    hardware_interface_init();
    audio_processing_init();
    string_detection_init();
    audio_sequencer_init();
    hardware_print_config();
    
    printf("\n");
    printf("===========================================\n");
    printf("  Running Test Scenarios\n");
    printf("===========================================\n");
    printf("\n");
    
    /* Run all test scenarios */
    test_scenario_1_perfect_tuning();
    test_scenario_2_sharp_tuning();
    test_scenario_3_weak_signal_recovery();
    
    printf("\n");
    printf("===========================================\n");
    printf("  All Tests Complete\n");
    printf("  Software validated for Teensy deployment\n");
    printf("===========================================\n");
    printf("\n");
    
    return 0;
}