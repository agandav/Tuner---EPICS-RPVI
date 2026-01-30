/**
 * hardware_interface.c - GPIO and Input/Output Interface Implementation
 *
 * Handles:
 * - Button debouncing and event generation
 * - Volume control via potentiometer or rotary encoder
 * - Audio amplifier enable/disable
 * - Accessibility tactile feedback
 */

#include "hardware_interface.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * INTERNAL STATE
 * ========================================================================== */

/* Button state tracking with debouncing */
typedef struct {
    uint16_t raw_state;              // Raw GPIO state
    uint16_t debounced_state;        // Debounced state
    uint32_t last_change_time_ms;    // Timestamp of last state change
    uint32_t press_time_ms;          // When button was first pressed
    uint8_t press_count;             // Number of consecutive reads with same state
} button_state_machine_t;

static button_state_machine_t button_states[6] = {0};
static button_event_t pending_event = {0};
static bool pending_event_ready = false;

static volume_control_t volume_control = {
    .current_volume = VOLUME_DEFAULT,
    .adc_raw = 0,
    .last_update_ms = 0
};

static bool audio_amplifier_enabled = false;
static uint32_t button_event_count = 0;

#define DEBOUNCE_TIME_MS 20  // 20ms debounce period
#define DEBOUNCE_COUNT 2     // Require 2 consecutive stable reads

/* ============================================================================
 * INITIALIZATION
 * ========================================================================== */

int hardware_interface_init(void) {
    /* Initialize button pins as INPUT with pull-ups (Active LOW) */
#ifdef __INCLUDE_TEENSY_LIBS__
    pinMode(STRING_1_BUTTON_PIN, INPUT_PULLUP);
    pinMode(STRING_2_BUTTON_PIN, INPUT_PULLUP);
    pinMode(STRING_3_BUTTON_PIN, INPUT_PULLUP);
    pinMode(STRING_4_BUTTON_PIN, INPUT_PULLUP);
    pinMode(STRING_5_BUTTON_PIN, INPUT_PULLUP);
    pinMode(STRING_6_BUTTON_PIN, INPUT_PULLUP);
    
    /* Mode switch (Play Tone I / Listen Only O) */
    pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
    
    /* Rotary encoder / knob pins (optional - if present) */
    pinMode(ROTARY_ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ROTARY_ENCODER_DT_PIN, INPUT_PULLUP);
    pinMode(ROTARY_ENCODER_SW_PIN, INPUT_PULLUP);
    
    /* Analog input for volume potentiometer (optional - if present) */
    pinMode(VOLUME_POTENTIOMETER_PIN, INPUT);
    
    /* Audio amplifier control */
    pinMode(AUDIO_AMP_ENABLE_PIN, OUTPUT);
    digitalWrite(AUDIO_AMP_ENABLE_PIN, LOW);  /* Start disabled */
    
    /* Optionally: Serial for debug output */
    if (ENABLE_DEBUG_PRINTS) {
        Serial.begin(SERIAL_BAUD_RATE);
        delay(100);
        printf("Hardware interface initialized\n");
    }
#else
    /* Non-Teensy platform: stub implementation */
    printf("[STUB] Hardware interface init (not on Teensy)\n");
#endif

    return 0;
}

/* ============================================================================
 * BUTTON INPUT IMPLEMENTATION
 * ========================================================================== */

int button_poll(void) {
    bool any_change = false;
    
    /* Array of pin definitions matching button states */
    const uint8_t button_pins[] = {
        STRING_1_BUTTON_PIN,
        STRING_2_BUTTON_PIN,
        STRING_3_BUTTON_PIN,
        STRING_4_BUTTON_PIN,
        STRING_5_BUTTON_PIN,
        STRING_6_BUTTON_PIN
    };
    
    uint32_t current_time = 0;
#ifdef __INCLUDE_TEENSY_LIBS__
    current_time = millis();
#endif
    
    /* Poll each string button */
    for (int i = 0; i < 6; i++) {
        button_state_machine_t *state = &button_states[i];
        
        /* Read current GPIO state (HIGH = released, LOW = pressed for pullup) */
        uint16_t raw_state = 1;  /* Default: released */
#ifdef __INCLUDE_TEENSY_LIBS__
        raw_state = (digitalRead(button_pins[i]) == LOW) ? 1 : 0;
#endif
        
        state->raw_state = raw_state;
        
        /* Debouncing: check if state has stabilized */
        if (state->raw_state == state->debounced_state) {
            state->press_count = 0;
        } else {
            state->press_count++;
            
            /* State change confirmed after DEBOUNCE_COUNT stable reads */
            if (state->press_count >= DEBOUNCE_COUNT) {
                state->debounced_state = state->raw_state;
                state->last_change_time_ms = current_time;
                
                /* Generate button event */
                pending_event.button_id = (button_id_t)(i + 1);  /* 1-6 for strings */
                pending_event.state = state->debounced_state ? 
                                     BUTTON_PRESSED : BUTTON_RELEASED;
                pending_event.timestamp_ms = current_time;
                pending_event.press_duration_ms = 0;  /* Set on release */
                
                pending_event_ready = true;
                any_change = true;
                button_event_count++;
                
                if (ENABLE_DEBUG_PRINTS) {
                    printf("[BTN] String %d: %s\n", i + 1, 
                           state->debounced_state ? "PRESSED" : "RELEASED");
                }
            }
        }
        
        /* Track press duration if currently pressed */
        if (state->debounced_state == BUTTON_PRESSED) {
            pending_event.press_duration_ms = current_time - state->press_time_ms;
        } else if (state->last_change_time_ms == current_time && 
                   state->debounced_state == BUTTON_RELEASED) {
            /* Button was just released */
            state->press_time_ms = 0;
        } else if (state->press_time_ms == 0 && 
                   state->debounced_state == BUTTON_PRESSED) {
            /* Button was just pressed */
            state->press_time_ms = current_time;
        }
    }
    
    return any_change ? 1 : 0;
}

button_event_t* button_get_event(void) {
    if (pending_event_ready) {
        pending_event_ready = false;
        return &pending_event;
    }
    return NULL;
}

bool button_is_pressed(button_id_t button_id) {
    if (button_id < 1 || button_id > 6) {
        return false;
    }
    return button_states[button_id - 1].debounced_state == 1;
}

void button_debounce(button_id_t button_id) {
    if (button_id >= 1 && button_id <= 6) {
        button_state_machine_t *state = &button_states[button_id - 1];
        state->press_count = 0;
    }
}

/* ============================================================================
 * VOLUME CONTROL IMPLEMENTATION
 * ========================================================================== */

float volume_read_analog(void) {
    uint16_t adc_raw = 0;
    
#ifdef __INCLUDE_TEENSY_LIBS__
    adc_raw = analogRead(VOLUME_POTENTIOMETER_PIN);
#else
    /* Stub: return current volume if not on Teensy */
    return volume_control.current_volume;
#endif
    
    volume_control.adc_raw = adc_raw;
    
    /* Convert 12-bit ADC (0-4095) to volume (0.0-1.0) */
    float normalized = (float)adc_raw / 4095.0f;
    
    /* Apply deadzone at extremes (0-10% and 90-100%) */
    if (normalized < 0.05f) {
        normalized = 0.0f;
    } else if (normalized > 0.95f) {
        normalized = 1.0f;
    }
    
    volume_control.current_volume = normalized;
    volume_control.last_update_ms = 0;
#ifdef __INCLUDE_TEENSY_LIBS__
    volume_control.last_update_ms = millis();
#endif
    
    if (ENABLE_DEBUG_PRINTS && (adc_raw % 512 == 0)) {
        printf("[VOL] ADC: %u -> Volume: %.2f\n", adc_raw, normalized);
    }
    
    return normalized;
}

void volume_set(float volume) {
    /* Clamp to valid range */
    if (volume < VOLUME_MIN) volume = VOLUME_MIN;
    if (volume > VOLUME_MAX) volume = VOLUME_MAX;
    
    volume_control.current_volume = volume;
    
    /* Apply volume scaling to audio DAC (implemented in teensy_audio_io.cpp) */
    
    if (ENABLE_DEBUG_PRINTS) {
        printf("[VOL] Set to: %.2f\n", volume);
    }
}

float volume_get(void) {
    return volume_control.current_volume;
}

void volume_adjust(float delta) {
    float new_volume = volume_control.current_volume + delta;
    volume_set(new_volume);
}

/* ============================================================================
 * AUDIO AMPLIFIER CONTROL
 * ========================================================================== */

void audio_amplifier_enable(void) {
#ifdef __INCLUDE_TEENSY_LIBS__
    digitalWrite(AUDIO_AMP_ENABLE_PIN, HIGH);
#endif
    audio_amplifier_enabled = true;
    if (ENABLE_DEBUG_PRINTS) {
        printf("[AMP] Enabled\n");
    }
}

void audio_amplifier_disable(void) {
#ifdef __INCLUDE_TEENSY_LIBS__
    digitalWrite(AUDIO_AMP_ENABLE_PIN, LOW);
#endif
    audio_amplifier_enabled = false;
    if (ENABLE_DEBUG_PRINTS) {
        printf("[AMP] Disabled\n");
    }
}

bool audio_amplifier_is_enabled(void) {
    return audio_amplifier_enabled;
}

/* ============================================================================
 * MODE SWITCH (Play Tone I / Listen Only O)
 * ========================================================================== */

bool mode_switch_is_play_tone(void) {
#ifdef __INCLUDE_TEENSY_LIBS__
    /* HIGH = Play Tone mode (I), LOW = Listen Only mode (O) */
    return digitalRead(MODE_SWITCH_PIN) == HIGH;
#else
    /* Stub: default to Listen Only mode on non-Teensy platforms */
    return false;
#endif
}

bool mode_switch_is_listen_only(void) {
    return !mode_switch_is_play_tone();
}

/* ============================================================================
 * ACCESSIBILITY FEATURES
 * ========================================================================== */

int tactile_feedback_click(void) {
    /* Stub: Would control piezo buzzer on dedicated GPIO */
    if (ENABLE_DEBUG_PRINTS) {
        printf("[TACTILE] Click feedback\n");
    }
    return 0;
}

int tactile_feedback_confirm(void) {
    /* Stub: Two beeps */
    if (ENABLE_DEBUG_PRINTS) {
        printf("[TACTILE] Confirm feedback (double-click)\n");
    }
    return 0;
}

int tactile_feedback_warning(void) {
    /* Stub: Three rapid beeps */
    if (ENABLE_DEBUG_PRINTS) {
        printf("[TACTILE] Warning feedback (triple-click)\n");
    }
    return 0;
}

/* ============================================================================
 * DIAGNOSTIC FUNCTIONS
 * ========================================================================== */

void hardware_print_config(void) {
    printf("\n=== GUITAR TUNER HARDWARE CONFIGURATION ===\n");
    printf("Microcontroller: Teensy %d.1\n", TEENSY_VERSION);
    printf("Clock Speed: %d MHz\n", TEENSY_CLOCK_SPEED);
    printf("RAM: %d KB\n\n", TEENSY_RAM_KB);
    
    printf("STRING BUTTONS:\n");
    printf("  String 1 (E4): GPIO %d\n", STRING_1_BUTTON_PIN);
    printf("  String 2 (B3): GPIO %d\n", STRING_2_BUTTON_PIN);
    printf("  String 3 (G3): GPIO %d\n", STRING_3_BUTTON_PIN);
    printf("  String 4 (D3): GPIO %d\n", STRING_4_BUTTON_PIN);
    printf("  String 5 (A2): GPIO %d\n", STRING_5_BUTTON_PIN);
    printf("  String 6 (E2): GPIO %d\n\n", STRING_6_BUTTON_PIN);
    
    printf("VOLUME CONTROL:\n");
    printf("  Potentiometer: ADC %d\n", VOLUME_POTENTIOMETER_PIN);
    printf("  Current Volume: %.2f\n\n", volume_control.current_volume);
    
    printf("AUDIO OUTPUT:\n");
    printf("  I2S BitClock: GPIO %d\n", AUDIO_I2S_BCLK_PIN);
    printf("  I2S Frame Clock: GPIO %d\n", AUDIO_I2S_LRCLK_PIN);
    printf("  I2S Data Out: GPIO %d\n", AUDIO_I2S_OUT_PIN);
    // printf("  Amplifier Enable: GPIO %d (%s)\n", AUDIO_AMP_ENABLE_PIN,
    //        audio_amplifier_enabled ? "ON" : "OFF");
    printf("  Sample Rate: %d Hz\n", AUDIO_SAMPLE_RATE);
    printf("  Block Size: %d samples\n\n", AUDIO_BLOCK_SIZE);
    
    printf("DSP CONFIGURATION:\n");
    printf("  FFT Size: %d\n", FFT_SIZE);
    printf("  Resolution: %d Hz/bin\n", FFT_HZ_PER_BIN);
    printf("  Frequency Range: %.0f - %.0f Hz\n",
           MIN_DETECTABLE_FREQ, MAX_DETECTABLE_FREQ);
    printf("\n");
}

void hardware_print_button_events(void) {
    printf("Button Events Captured: %lu\n", button_event_count);
}

uint32_t hardware_get_button_count(void) {
    return button_event_count;
}

/* ============================================================================
 * MAIN LOOP INTEGRATION (Example)
 * ========================================================================== */

/**
 * Example main loop structure:
 *
 * void loop() {
 *     // Poll buttons every 10ms
 *     if (button_poll()) {
 *         button_event_t *evt = button_get_event();
 *         if (evt && evt->state == BUTTON_PRESSED) {
 *             // User pressed string button
 *             int target_string = evt->button_id;
 *             tuner_set_target_string(target_string);
 *         }
 *     }
 *
 *     // Update volume every 50ms
 *     static uint32_t last_vol_update = 0;
 *     if (millis() - last_vol_update > VOLUME_UPDATE_INTERVAL_MS) {
 *         float vol = volume_read_analog();
 *         set_audio_volume(vol);  // Apply to DAC
 *         last_vol_update = millis();
 *     }
 *
 *     // Process audio and tuning
 *     process_audio_frame();
 * }
 */
