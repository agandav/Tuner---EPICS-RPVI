/**
 * main.cpp - Enhanced State Machine for RPVI Guitar Tuner
 * 
 * NEW WORKFLOW:
 * 1. User presses string button (1-6)
 * 2. [Mode I] Device plays target tone FIRST (optional)
 * 3. User plays their guitar string
 * 4. Device plays back what user played (detected frequency)
 * 5. Device plays the target note (what user should aim for)
 * 6. Device provides dynamic beep feedback until in tune
 * 7. Beeps get slower as user gets closer to target
 * 8. When in tune (< 5 cents), beeping stops
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
    #include "audio_sequencer_v2.h"
    #include "note_parser.h"
    #include "hardware_interface.h"
    #include "teensy_audio_io.h"
    #include "config.h"
}

/* ============================================================================
 * STATE MACHINE DEFINITIONS
 * ========================================================================== */

typedef enum {
    TUNER_STATE_IDLE = 0,              // Waiting for user to press button
    TUNER_STATE_PLAYING_REFERENCE,     // Playing target note (Mode I only)
    TUNER_STATE_WAITING_READY_BEEP,    // Playing "ready" beep before listening
    TUNER_STATE_LISTENING,             // Capturing audio and analyzing frequency
    TUNER_STATE_PLAYBACK_USER_NOTE,    // NEW: Play back what user played
    TUNER_STATE_PLAYBACK_TARGET_NOTE,  // NEW: Play the target note
    TUNER_STATE_PROVIDING_FEEDBACK,    // Providing dynamic beep feedback
    TUNER_STATE_ERROR_RECOVERY         // Handling errors (weak signal, timeout, etc.)
} tuner_state_t;

typedef enum {
    MODE_PLAY_TONE = 0,    // Mode I: Play tone before tuning
    MODE_LISTEN_ONLY = 1   // Mode O: Listen only (no tone playback)
} tuner_mode_t;

/* Guitar string frequencies using CFugue-style notation */
static const char* STRING_NOTES[6] = {
    NOTE_E4,  // String 1: E4 (high E) - 329.63 Hz
    NOTE_B3,  // String 2: B3 - 246.94 Hz
    NOTE_G3,  // String 3: G3 - 196.00 Hz
    NOTE_D3,  // String 4: D3 - 146.83 Hz
    NOTE_A2,  // String 5: A2 - 110.00 Hz
    NOTE_E2   // String 6: E2 (low E) - 82.41 Hz
};

// Computed at runtime from note names
static double STRING_FREQUENCIES[6];

/* ============================================================================
 * STATE MACHINE VARIABLES
 * ========================================================================== */

static tuner_state_t current_state = TUNER_STATE_IDLE;
static tuner_mode_t tuner_mode = MODE_PLAY_TONE;
static int target_string = 0;                     // 1-6, or 0 for none
static double target_frequency = 0.0;
static double detected_frequency = 0.0;           // NEW: Store what user played
static uint32_t state_entry_time = 0;
static uint32_t last_beep_update = 0;

/* Audio capture buffer */
#define AUDIO_CAPTURE_SIZE 1024
static int16_t audio_buffer[AUDIO_CAPTURE_SIZE];
static int audio_samples_captured = 0;

/* Error recovery */
#define MAX_NO_SIGNAL_TIME_MS 5000
#define MAX_WEAK_SIGNAL_COUNT 10
static int weak_signal_count = 0;

/* Timing constants */
#define REFERENCE_TONE_DURATION_MS 2000   // 2 seconds for reference tone
#define PLAYBACK_DURATION_MS 1500         // 1.5 seconds for playback
#define TARGET_TONE_DURATION_MS 1500      // 1.5 seconds for target
#define READY_BEEP_DURATION_MS 200
#define BEEP_UPDATE_INTERVAL_MS 10
#define PAUSE_BETWEEN_TONES_MS 300        // Short pause between playback and target

/* Latest tuning result for feedback */
static TuningResult latest_result;
static bool tuning_in_progress = false;

/* ============================================================================
 * MODE SWITCH READING
 * ========================================================================== */

void read_mode_switch(void) {
#ifdef MODE_SWITCH_PIN
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
            target_string = event->button_id;
            
            if (target_string < 1 || target_string > 6) {
                Serial.print("[ERROR] Invalid button ID: ");
                Serial.println(target_string);
                return;
            }
            
            target_frequency = STRING_FREQUENCIES[target_string - 1];
            
            Serial.println("\n========================================");
            Serial.print("[STATE] User selected String ");
            Serial.print(target_string);
            Serial.print(" (");
            Serial.print(STRING_NOTES[target_string - 1]);
            Serial.print(") - Target: ");
            Serial.print(target_frequency, 2);
            Serial.println(" Hz");
            Serial.println("========================================");
            
            audio_amplifier_enable();
            read_mode_switch();
            
            // Transition based on mode
            if (tuner_mode == MODE_PLAY_TONE) {
                current_state = TUNER_STATE_PLAYING_REFERENCE;
                state_entry_time = millis();
                
                Serial.println("[STEP 1] Playing reference tone...");
                playGuitarString(target_string, REFERENCE_TONE_DURATION_MS);
            } else {
                // Skip reference, go straight to ready beep
                current_state = TUNER_STATE_WAITING_READY_BEEP;
                state_entry_time = millis();
                play_ready_beep();
                Serial.println("[MODE O] Skipping reference tone - Listen Only");
            }
        }
    }
}

/**
 * STATE: PLAYING_REFERENCE
 * Playing the target note as reference (Mode I only)
 */
static void state_playing_reference(void) {
    uint32_t elapsed = millis() - state_entry_time;
    
    if (elapsed >= REFERENCE_TONE_DURATION_MS) {
        Serial.println("[STEP 1] Reference tone complete");
        current_state = TUNER_STATE_WAITING_READY_BEEP;
        state_entry_time = millis();
        play_ready_beep();
    }
    
    // Allow user to cancel
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
        Serial.println("\n[STEP 2] Ready! Play your string now...");
        Serial.println("========================================");
        current_state = TUNER_STATE_LISTENING;
        state_entry_time = millis();
        audio_samples_captured = 0;
        weak_signal_count = 0;
        detected_frequency = 0.0;
    }
    
    // Allow user to cancel
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
    double freq = read_frequency_from_microphone(NULL, 0);
    
    if (freq > 0.0) {
        detected_frequency = freq;
        
        Serial.print("[DETECTED] You played: ");
        Serial.print(detected_frequency, 2);
        Serial.print(" Hz (");
        
        // Show which note was detected
        const char* detected_note = frequencyToNote((float)detected_frequency);
        if (detected_note) {
            Serial.print(detected_note);
        } else {
            Serial.print("?");
        }
        Serial.println(")");
        
        // Analyze tuning
        latest_result = analyze_tuning(detected_frequency, target_string);
        
        Serial.print("[ANALYSIS] Off by ");
        Serial.print(latest_result.cents_offset, 1);
        Serial.print(" cents - Direction: ");
        Serial.println(latest_result.direction);
        
        // Transition to playback sequence
        Serial.println("\n[STEP 3] Playing back what you played...");
        current_state = TUNER_STATE_PLAYBACK_USER_NOTE;
        state_entry_time = millis();
        
        // Play back user's note
        playFrequencyTone((float)detected_frequency, PLAYBACK_DURATION_MS);
        
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
    
    // Allow user to cancel
    if (!button_is_pressed((button_id_t)target_string)) {
        Serial.println("[STATE] Button released - returning to IDLE");
        current_state = TUNER_STATE_IDLE;
        target_string = 0;
        audio_amplifier_disable();
    }
}

/**
 * STATE: PLAYBACK_USER_NOTE
 * NEW: Playing back what the user played
 */
static void state_playback_user_note(void) {
    uint32_t elapsed = millis() - state_entry_time;
    
    if (elapsed >= PLAYBACK_DURATION_MS) {
        Serial.println("[STEP 3] Playback complete");
        
        // Short pause before target note
        delay(PAUSE_BETWEEN_TONES_MS);
        
        Serial.println("\n[STEP 4] Playing target note...");
        current_state = TUNER_STATE_PLAYBACK_TARGET_NOTE;
        state_entry_time = millis();
        
        // Play target note
        playGuitarString(target_string, TARGET_TONE_DURATION_MS);
    }
    
    // Allow user to cancel
    if (!button_is_pressed((button_id_t)target_string)) {
        Serial.println("[STATE] Button released - returning to IDLE");
        current_state = TUNER_STATE_IDLE;
        target_string = 0;
        audio_amplifier_disable();
    }
}

/**
 * STATE: PLAYBACK_TARGET_NOTE
 * NEW: Playing the target note (what user should aim for)
 */
static void state_playback_target_note(void) {
    uint32_t elapsed = millis() - state_entry_time;
    
    if (elapsed >= TARGET_TONE_DURATION_MS) {
        Serial.println("[STEP 4] Target note complete");
        Serial.println("\n========================================");
        Serial.println("[STEP 5] Starting dynamic beep feedback");
        Serial.print("        Current offset: ");
        Serial.print(latest_result.cents_offset, 1);
        Serial.println(" cents");
        Serial.println("        Faster beeps = further from tune");
        Serial.println("        Slower beeps = closer to tune");
        Serial.println("        No beeps = IN TUNE!");
        Serial.println("========================================\n");
        
        current_state = TUNER_STATE_PROVIDING_FEEDBACK;
        generate_dynamic_beep_feedback(&latest_result);
        last_beep_update = millis();
        tuning_in_progress = true;
    }
    
    // Allow user to cancel
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
 * Beeps continue until user reaches target note (< 5 cents)
 */
static void state_providing_feedback(void) {
    uint32_t current_time = millis();
    
    // Update beep timing
    if (current_time - last_beep_update >= BEEP_UPDATE_INTERVAL_MS) {
        audio_sequencer_update_beeps(current_time);
        last_beep_update = current_time;
    }
    
    // Continue monitoring frequency
    double freq = read_frequency_from_microphone(NULL, 0);
    
    if (freq > 0.0) {
        latest_result = analyze_tuning(freq, target_string);
        generate_dynamic_beep_feedback(&latest_result);
        weak_signal_count = 0;
        
        // Check if user reached target
        if (fabs(latest_result.cents_offset) < 5.0) {
            static bool in_tune_announced = false;
            if (!in_tune_announced) {
                Serial.println("\n****************************************");
                Serial.println("          ?? IN TUNE! ??");
                Serial.println("****************************************");
                Serial.print("Final offset: ");
                Serial.print(latest_result.cents_offset, 2);
                Serial.println(" cents");
                Serial.println("Beeping stopped - hold this tuning!");
                Serial.println("****************************************\n");
                in_tune_announced = true;
            }
        } else {
            // Print periodic updates (not every loop)
            static int update_count = 0;
            if (++update_count % 50 == 0) {
                Serial.print("[FEEDBACK] Offset: ");
                Serial.print(latest_result.cents_offset, 1);
                Serial.print(" cents - ");
                Serial.println(latest_result.direction);
            }
        }
    } else {
        weak_signal_count++;
        if (weak_signal_count >= MAX_WEAK_SIGNAL_COUNT) {
            Serial.println("[FEEDBACK] Signal lost - play your string again");
            Serial.println("Returning to listening mode...\n");
            current_state = TUNER_STATE_LISTENING;
            state_entry_time = millis();
        }
    }
    
    // Allow user to end session by releasing button
    if (!button_is_pressed((button_id_t)target_string)) {
        Serial.println("\n[STATE] Button released - tuning session complete");
        Serial.println("========================================\n");
        current_state = TUNER_STATE_IDLE;
        target_string = 0;
        tuning_in_progress = false;
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
        Serial.println("\n========================================");
        Serial.println("[ERROR] Weak signal detected!");
        Serial.println("Please:");
        Serial.println("  - Play your string louder");
        Serial.println("  - Move closer to microphone");
        Serial.println("  - Check microphone connection");
        Serial.println("========================================\n");
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
    
    // Allow user to cancel
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

    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("  RPVI Guitar Tuner - Enhanced Edition");
    Serial.println("  CFugue-Style Notation");
    Serial.println("  NO WAV FILES - Synthesized Audio");
    Serial.println("========================================");
    Serial.println();
    Serial.println("NEW WORKFLOW:");
    Serial.println("1. Press string button");
    Serial.println("2. [Mode I] Hear reference tone");
    Serial.println("3. Play your string");
    Serial.println("4. Hear playback of what you played");
    Serial.println("5. Hear the target note");
    Serial.println("6. Follow beep feedback to tune");
    Serial.println("7. Beeps stop when in tune!");
    Serial.println("========================================");
    Serial.println();

    Serial.println("Initializing audio system...");
    init_audio_system();
    
    Serial.println("Initializing hardware interface...");
    hardware_interface_init();
    
    Serial.println("Initializing audio processing...");
    audio_processing_init();
    
    Serial.println("Initializing string detection...");
    string_detection_init();
    
    Serial.println("Initializing audio sequencer (CFugue-style)...");
    audio_sequencer_init();
    
    Serial.println("\nComputing frequencies from note notation...");
    for (int i = 0; i < 6; i++) {
        STRING_FREQUENCIES[i] = parseNote(STRING_NOTES[i]);
        Serial.print("  String ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(STRING_NOTES[i]);
        Serial.print(" = ");
        Serial.print(STRING_FREQUENCIES[i], 2);
        Serial.println(" Hz");
    }
    Serial.println();
    
    Serial.println("Reading mode switch...");
    read_mode_switch();
    
    hardware_print_config();
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("  ?? Tuner Ready! ??");
    Serial.print("  Mode: ");
    Serial.println(tuner_mode == MODE_PLAY_TONE ? "Play Tone (I)" : "Listen Only (O)");
    Serial.println("  Press a string button to begin tuning");
    Serial.println("========================================");
    Serial.println();
    
    current_state = TUNER_STATE_IDLE;
    tuning_in_progress = false;
}

void loop(void) {
    // Update tone playback timing
    update_tone_playback();
    
    // State machine dispatch
    switch (current_state) {
        case TUNER_STATE_IDLE:
            state_idle();
            break;
            
        case TUNER_STATE_PLAYING_REFERENCE:
            state_playing_reference();
            break;
            
        case TUNER_STATE_WAITING_READY_BEEP:
            state_waiting_ready_beep();
            break;
            
        case TUNER_STATE_LISTENING:
            state_listening();
            break;
            
        case TUNER_STATE_PLAYBACK_USER_NOTE:
            state_playback_user_note();
            break;
            
        case TUNER_STATE_PLAYBACK_TARGET_NOTE:
            state_playback_target_note();
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
    
    delayMicroseconds(100);
}