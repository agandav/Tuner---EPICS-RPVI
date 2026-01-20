/**
 * main.cpp - Complete State Machine Implementation for RPVI Guitar Tuner
 * 
 * Implements the full user interaction sequence from the user manual:
 * 1. User selects string via braille-labeled button (1-6)
 * 2. [Mode I] Device plays target tone
 * 3. Device beeps to signal "ready for input"
 * 4. User plays their guitar string
 * 5. Device provides dynamic beep feedback (faster = worse tune)
 * 6. Loop continues until button released or different button pressed
 * 
 * HARDWARE REQUIREMENTS (from user manual):
 * - 6 tactile buttons with braille labels (Strings 1-6)
 * - Mode switch: Position I (play tone) or Position O (listen only)
 * - Microphone input (internal or 3.5mm jack)
 * - Speaker output (internal or 3.5mm jack)
 * - Battery powered with on/off switch
 */

#include <Arduino.h>
#include <cstdio>
#include <cstdint>

extern "C" {
    #include "audio_processing.h"
    #include "string_detection.h"
    #include "audio_sequencer.h"
    #include "hardware_interface.h"
    #include "teensy_audio_io.h"
    #include "config.h"
}

/* ============================================================================
 * STATE MACHINE DEFINITIONS
 * ========================================================================== */

typedef enum {
    TUNER_STATE_IDLE = 0,              // Waiting for user to press button
    TUNER_STATE_PLAYING_TONE,          // Playing target note (Mode I only)
    TUNER_STATE_WAITING_READY_BEEP,    // Playing "ready" beep before listening
    TUNER_STATE_LISTENING,             // Capturing audio and analyzing frequency
    TUNER_STATE_PROVIDING_FEEDBACK,    // Providing dynamic beep feedback
    TUNER_STATE_ERROR_RECOVERY         // Handling errors (weak signal, timeout, etc.)
} tuner_state_t;

typedef enum {
    MODE_PLAY_TONE = 0,    // Mode I: Play tone before tuning
    MODE_LISTEN_ONLY = 1   // Mode O: Listen only (no tone playback)
} tuner_mode_t;

/* Guitar string frequencies (standard tuning, equal temperament) */
static const double STRING_FREQUENCIES[6] = {
    329.63,  // String 1: E4 (high E)
    246.94,  // String 2: B3
    196.00,  // String 3: G3
    146.83,  // String 4: D3
    110.00,  // String 5: A2
    82.41    // String 6: E2 (low E)
};

static const char* STRING_NAMES[6] = {
    "E4", "B3", "G3", "D3", "A2", "E2"
};

/* ============================================================================
 * STATE MACHINE VARIABLES
 * ========================================================================== */

static tuner_state_t current_state = TUNER_STATE_IDLE;
static tuner_mode_t tuner_mode = MODE_PLAY_TONE;  // Default: play tone mode
static int target_string = 0;                     // 1-6, or 0 for none
static double target_frequency = 0.0;
static uint32_t state_entry_time = 0;             // Time when state was entered
static uint32_t last_beep_update = 0;             // For beep timing

/* Audio capture buffer */
#define AUDIO_CAPTURE_SIZE 1024
static int16_t audio_buffer[AUDIO_CAPTURE_SIZE];
static int audio_samples_captured = 0;

/* Error recovery */
#define MAX_NO_SIGNAL_TIME_MS 5000    // 5 seconds timeout
#define MAX_WEAK_SIGNAL_COUNT 10      // Max consecutive weak signals
static int weak_signal_count = 0;

/* Timing constants */
#define TONE_PLAYBACK_DURATION_MS 1000    // 1 second tone playback
#define READY_BEEP_DURATION_MS 200        // 200ms "ready" beep
#define BEEP_UPDATE_INTERVAL_MS 10        // Update beeps every 10ms

/* ============================================================================
 * MODE SWITCH READING
 * ========================================================================== */

/**
 * Read mode switch and update tuner_mode
 * Call this in setup() and optionally in loop() to support runtime switching
 * 
 * Hardware connections (verify with electrical team):
 * - MODE_SWITCH_PIN defined in config.h
 * - Switch logic: HIGH = Play Tone (Mode I), LOW = Listen Only (Mode O)
 *   OR opposite - verify with electrical team!
 */
void read_mode_switch(void) {
#ifdef MODE_SWITCH_PIN
    // Read switch state (assuming active HIGH = Play Tone)
    // Electrical team: verify this logic!
    int switch_state = digitalRead(MODE_SWITCH_PIN);
    
    if (switch_state == HIGH) {
        tuner_mode = MODE_PLAY_TONE;
    } else {
        tuner_mode = MODE_LISTEN_ONLY;
    }
    
    if (ENABLE_DEBUG_PRINTS) {
        static tuner_mode_t last_mode = (tuner_mode_t)-1;
        if (tuner_mode != last_mode) {
            Serial.print("[MODE] Switch changed to: ");
            Serial.println(tuner_mode == MODE_PLAY_TONE ? "Play Tone (I)" : "Listen Only (O)");
            last_mode = tuner_mode;
        }
    }
#else
    // No mode switch defined - use default
    // tuner_mode remains at MODE_PLAY_TONE (set in variable initialization)
    if (ENABLE_DEBUG_PRINTS) {
        static bool warning_printed = false;
        if (!warning_printed) {
            Serial.println("[WARNING] MODE_SWITCH_PIN not defined - using default Play Tone mode");
            warning_printed = true;
        }
    }
#endif
}

/* ============================================================================
 * STATE MACHINE IMPLEMENTATION
 * ========================================================================== */

/**
 * STATE: IDLE
 * Waiting for user to press a string button
 */
static void state_idle(void) {
    if (button_poll()) {
        button_event_t* event = button_get_event();
        
        if (event && event->state == BUTTON_PRESSED) {
            // User pressed a string button (1-6)
            target_string = event->button_id;
            
            // Validate button ID is in range
            if (target_string < 1 || target_string > 6) {
                Serial.print("[ERROR] Invalid button ID: ");
                Serial.println(target_string);
                return;
            }
            
            target_frequency = STRING_FREQUENCIES[target_string - 1];
            
            Serial.print("\n[STATE] User selected String ");
            Serial.print(target_string);
            Serial.print(" (");
            Serial.print(STRING_NAMES[target_string - 1]);
            Serial.print(") - Target: ");
            Serial.print(target_frequency, 2);
            Serial.println(" Hz");
            
            // Enable audio amplifier
            audio_amplifier_enable();
            
            // Read current mode switch setting
            read_mode_switch();
            
            // Transition based on mode
            if (tuner_mode == MODE_PLAY_TONE) {
                current_state = TUNER_STATE_PLAYING_TONE;
                state_entry_time = millis();
                play_tone(target_frequency, TONE_PLAYBACK_DURATION_MS);
                Serial.println("[MODE] Playing reference tone (Mode I)");
            } else {
                // Skip tone, go straight to ready beep
                current_state = TUNER_STATE_WAITING_READY_BEEP;
                state_entry_time = millis();
                play_ready_beep();
                Serial.println("[MODE] Skipping tone (Mode O - Listen Only)");
            }
        }
    }
}

/**
 * STATE: PLAYING_TONE
 * Playing the target frequency tone for user reference
 */
static void state_playing_tone(void) {
    uint32_t elapsed = millis() - state_entry_time;
    
    if (elapsed >= TONE_PLAYBACK_DURATION_MS) {
        Serial.println("[STATE] Tone playback complete");
        current_state = TUNER_STATE_WAITING_READY_BEEP;
        state_entry_time = millis();
        play_ready_beep();
    }
    
    // Allow user to cancel by releasing button
    if (!button_is_pressed((button_id_t)target_string)) {
        Serial.println("[STATE] Button released - returning to IDLE");
        current_state = TUNER_STATE_IDLE;
        target_string = 0;
        audio_amplifier_disable();
    }
}

/**
 * STATE: WAITING_READY_BEEP
 * Playing "ready" beep to signal user can play their string
 */
static void state_waiting_ready_beep(void) {
    uint32_t elapsed = millis() - state_entry_time;
    
    if (elapsed >= READY_BEEP_DURATION_MS) {
        Serial.println("[STATE] Ready beep complete - listening for guitar input");
        current_state = TUNER_STATE_LISTENING;
        state_entry_time = millis();
        audio_samples_captured = 0;
        weak_signal_count = 0;
    }
    
    // Allow user to cancel by releasing button
    if (!button_is_pressed((button_id_t)target_string)) {
        Serial.println("[STATE] Button released - returning to IDLE");
        current_state = TUNER_STATE_IDLE;
        target_string = 0;
        audio_amplifier_disable();
    }
}

/**
 * STATE: LISTENING
 * Capturing audio from microphone and analyzing frequency
 */
static void state_listening(void) {
    // Use direct FFT reading from microphone
    double detected_freq = read_frequency_from_microphone(NULL, 0);
    
    if (detected_freq > 0.0) {
        Serial.print("[FFT] Detected: ");
        Serial.print(detected_freq, 2);
        Serial.println(" Hz");
        
        TuningResult result = analyze_tuning(detected_freq, target_string);
        
        Serial.print("[TUNING] String ");
        Serial.print(result.detected_string);
        Serial.print(", Cents: ");
        Serial.print(result.cents_offset, 1);
        Serial.print(", Direction: ");
        Serial.println(result.direction);
        
        current_state = TUNER_STATE_PROVIDING_FEEDBACK;
        generate_dynamic_beep_feedback(&result);
        last_beep_update = millis();
        weak_signal_count = 0;
        
    } else {
        weak_signal_count++;
        
        if (weak_signal_count >= MAX_WEAK_SIGNAL_COUNT) {
            Serial.println("[ERROR] Too many weak signals - entering error recovery");
            current_state = TUNER_STATE_ERROR_RECOVERY;
            state_entry_time = millis();
        }
    }
    
    // Check for timeout
    uint32_t elapsed = millis() - state_entry_time;
    if (elapsed >= MAX_NO_SIGNAL_TIME_MS) {
        Serial.println("[ERROR] Timeout waiting for signal");
        current_state = TUNER_STATE_ERROR_RECOVERY;
        state_entry_time = millis();
    }
    
    // Allow user to cancel by releasing button
    if (!button_is_pressed((button_id_t)target_string)) {
        Serial.println("[STATE] Button released - returning to IDLE");
        current_state = TUNER_STATE_IDLE;
        target_string = 0;
        audio_amplifier_disable();
    }
}

/**
 * STATE: PROVIDING_FEEDBACK
 * Providing dynamic beep feedback based on tuning accuracy
 */
static void state_providing_feedback(void) {
    uint32_t current_time = millis();
    
    // Update beep timing
    if (current_time - last_beep_update >= BEEP_UPDATE_INTERVAL_MS) {
        audio_sequencer_update_beeps(current_time);
        last_beep_update = current_time;
    }
    
    // Continue monitoring frequency
    double detected_freq = read_frequency_from_microphone(NULL, 0);
    
    if (detected_freq > 0.0) {
        TuningResult result = analyze_tuning(detected_freq, target_string);
        generate_dynamic_beep_feedback(&result);
        weak_signal_count = 0;
        
        // Print periodic updates (not every loop to avoid spam)
        static int update_count = 0;
        if (++update_count % 50 == 0) {
            Serial.print("[FEEDBACK] Cents: ");
            Serial.print(result.cents_offset, 1);
            Serial.print(", Direction: ");
            Serial.println(result.direction);
        }
    } else {
        weak_signal_count++;
        if (weak_signal_count >= MAX_WEAK_SIGNAL_COUNT) {
            Serial.println("[FEEDBACK] Signal lost - returning to listening");
            current_state = TUNER_STATE_LISTENING;
            state_entry_time = millis();
        }
    }
    
    // Allow user to end session by releasing button
    if (!button_is_pressed((button_id_t)target_string)) {
        Serial.println("[STATE] Button released - tuning session complete");
        current_state = TUNER_STATE_IDLE;
        target_string = 0;
        audio_amplifier_disable();
    }
}

/**
 * STATE: ERROR_RECOVERY
 * Handling errors like weak signals, timeouts, or invalid input
 */
static void state_error_recovery(void) {
    static int warning_played = 0;
    if (!warning_played) {
        tactile_feedback_warning();
        Serial.println("[ERROR] Please play your string louder or closer to microphone");
        warning_played = 1;
    }
    
    uint32_t elapsed = millis() - state_entry_time;
    if (elapsed >= 2000) {
        Serial.println("[RECOVERY] Returning to listening state");
        current_state = TUNER_STATE_LISTENING;
        state_entry_time = millis();
        weak_signal_count = 0;
        warning_played = 0;
    }
    
    // Allow user to cancel by releasing button
    if (!button_is_pressed((button_id_t)target_string)) {
        Serial.println("[STATE] Button released - returning to IDLE");
        current_state = TUNER_STATE_IDLE;
        target_string = 0;
        audio_amplifier_disable();
        warning_played = 0;
    }
}

/* ============================================================================
 * ARDUINO FRAMEWORK ENTRY POINTS
 * ========================================================================== */

void setup(void) {
    Serial.begin(115200);
    delay(1000);

    Serial.println("Initializing audio system...");
    init_audio_system();
    
    Serial.println("Initializing hardware interface...");
    hardware_interface_init();
    
    Serial.println("Initializing audio processing...");
    audio_processing_init();
    
    Serial.println("Initializing string detection...");
    string_detection_init();
    
    Serial.println("Initializing audio sequencer...");
    audio_sequencer_init();
    
    // Read initial mode switch position
    Serial.println("Reading mode switch...");
    read_mode_switch();
    
    // Print hardware configuration
    hardware_print_config();
    
    Serial.println();
    Serial.println("===========================================");
    Serial.println("  Tuner Ready!");
    Serial.print("  Mode: ");
    Serial.println(tuner_mode == MODE_PLAY_TONE ? "Play Tone (I)" : "Listen Only (O)");
    Serial.println("  Press a string button to begin tuning");
    Serial.println("===========================================");
    Serial.println();
    
    current_state = TUNER_STATE_IDLE;
}

void loop(void) {
    // Update tone playback timing
    update_tone_playback();
    
    // State machine dispatch
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
            Serial.println("[ERROR] Invalid state! Returning to IDLE");
            current_state = TUNER_STATE_IDLE;
            break;
    }
    
    // Small delay to prevent overwhelming the serial output
    delayMicroseconds(100);
}