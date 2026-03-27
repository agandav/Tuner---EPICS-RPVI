/**
 * hardware_interface.c - GPIO and Input/Output Interface Implementation
 *
 * CHANGES FROM ORIGINAL:
 *   1. Removed audio_amplifier_enable() and audio_amplifier_disable() entirely.
 *      LM386 has no enable or shutdown pin. Power is controlled by the physical
 *      rocker switch connected to the PowerBoost 1000 Charge EN pin.
 *   2. Removed audio_amplifier_is_enabled() - no longer applicable.
 *   3. Removed rotary encoder - not part of this build.
 *   4. Removed volume potentiometer - handled by physical pot on LM386 module.
 *   5. Updated pin assignments to match confirmed electrical configuration.
 *   6. STRING_1_BUTTON_PIN updated to pin 22 (was pin 2, now occupied by amp).
 *   7. Buttons confirmed ACTIVE HIGH (connect to 3.3V when pressed, not GND).
 *      Changed INPUT_PULLUP -> INPUT_PULLDOWN, detection LOW->HIGH throughout.
 */

#include "hardware_interface.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * INTERNAL STATE
 * ========================================================================== */

typedef struct {
    uint16_t raw_state;
    uint16_t debounced_state;
    uint32_t last_change_time_ms;
    uint32_t press_time_ms;
    uint8_t  press_count;
} button_state_machine_t;

static button_state_machine_t button_states[6] = {0};
static button_event_t         pending_event     = {0};
static bool                   pending_event_ready = false;
static uint32_t               button_event_count  = 0;

#define DEBOUNCE_COUNT 2   // Require 2 consecutive stable reads to confirm state change

/* ============================================================================
 * INITIALIZATION
 * ========================================================================== */

int hardware_interface_init(void) {
#ifdef __INCLUDE_TEENSY_LIBS__
    /* String selection buttons - active HIGH (connect to 3.3V when pressed)
       INPUT_PULLDOWN holds pin LOW at rest; pressing drives it HIGH */
    pinMode(STRING_1_BUTTON_PIN, INPUT_PULLDOWN);  // Pin 22
    pinMode(STRING_2_BUTTON_PIN, INPUT_PULLDOWN);  // Pin 3
    pinMode(STRING_3_BUTTON_PIN, INPUT_PULLDOWN);  // Pin 4
    pinMode(STRING_4_BUTTON_PIN, INPUT_PULLDOWN);  // Pin 5
    pinMode(STRING_5_BUTTON_PIN, INPUT_PULLDOWN);  // Pin 6
    pinMode(STRING_6_BUTTON_PIN, INPUT_PULLDOWN);  // Pin 9

    /* Mode switch - active HIGH, same logic */
    pinMode(MODE_SWITCH_PIN, INPUT_PULLDOWN);       // Pin 11

    if (ENABLE_DEBUG_PRINTS) {
        Serial.begin(SERIAL_BAUD_RATE);
        delay(100);
        Serial.println("[HW] Hardware interface initialized");
        Serial.println("[HW] Buttons: pins 22, 3, 4, 5, 6, 9");
        Serial.println("[HW] Mode switch: pin 11");
        Serial.println("[HW] Amp: LM386 - no enable pin, always on when powered");
    }
#else
    printf("[HW] Hardware interface initialized (non-Teensy stub)\n");
#endif

    return 0;
}

/* ============================================================================
 * BUTTON INPUT
 * ========================================================================== */

int button_poll(void) {
    bool any_change = false;

    const uint8_t button_pins[] = {
        STRING_1_BUTTON_PIN,   // Pin 22
        STRING_2_BUTTON_PIN,   // Pin 3
        STRING_3_BUTTON_PIN,   // Pin 4
        STRING_4_BUTTON_PIN,   // Pin 5
        STRING_5_BUTTON_PIN,   // Pin 6
        STRING_6_BUTTON_PIN    // Pin 9
    };

    uint32_t current_time = 0;
#ifdef __INCLUDE_TEENSY_LIBS__
    current_time = millis();
#endif

    for (int i = 0; i < 6; i++) {
        button_state_machine_t* state = &button_states[i];

        uint16_t raw_state = 0;  // Default: released (pin held LOW by pull-down)
#ifdef __INCLUDE_TEENSY_LIBS__
        raw_state = (digitalRead(button_pins[i]) == HIGH) ? 1 : 0;
#endif

        state->raw_state = raw_state;

        if (state->raw_state == state->debounced_state) {
            state->press_count = 0;
        } else {
            state->press_count++;

            if (state->press_count >= DEBOUNCE_COUNT) {
                state->debounced_state    = state->raw_state;
                state->last_change_time_ms = current_time;

                pending_event.button_id        = (button_id_t)(i + 1);
                pending_event.state            = state->debounced_state ?
                                                 BUTTON_PRESSED : BUTTON_RELEASED;
                pending_event.timestamp_ms     = current_time;
                pending_event.press_duration_ms = 0;

                pending_event_ready = true;
                any_change          = true;
                button_event_count++;

                if (ENABLE_DEBUG_PRINTS) {
                    printf("[BTN] String %d: %s\n", i + 1,
                           state->debounced_state ? "PRESSED" : "RELEASED");
                }
            }
        }

        if (state->debounced_state == BUTTON_PRESSED) {
            pending_event.press_duration_ms = current_time - state->press_time_ms;
        } else if (state->press_time_ms == 0 && state->debounced_state == BUTTON_PRESSED) {
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
    if (button_id < 1 || button_id > 6) return false;
    return button_states[button_id - 1].debounced_state == 1;
}

void button_debounce(button_id_t button_id) {
    if (button_id >= 1 && button_id <= 6) {
        button_states[button_id - 1].press_count = 0;
    }
}

/* ============================================================================
 * MODE SWITCH
 * ========================================================================== */

bool mode_switch_is_play_tone(void) {
#ifdef __INCLUDE_TEENSY_LIBS__
    return digitalRead(MODE_SWITCH_PIN) == HIGH;
#else
    return false;
#endif
}

bool mode_switch_is_listen_only(void) {
    return !mode_switch_is_play_tone();
}

/* ============================================================================
 * TACTILE FEEDBACK
 * ========================================================================== */

int tactile_feedback_click(void) {
    if (ENABLE_DEBUG_PRINTS) printf("[TACTILE] Click\n");
    return 0;
}

int tactile_feedback_confirm(void) {
    if (ENABLE_DEBUG_PRINTS) printf("[TACTILE] Confirm\n");
    return 0;
}

int tactile_feedback_warning(void) {
    if (ENABLE_DEBUG_PRINTS) printf("[TACTILE] Warning\n");
    return 0;
}

/* ============================================================================
 * DIAGNOSTICS
 * ========================================================================== */

void hardware_print_config(void) {
    printf("\n=== GUITAR TUNER HARDWARE CONFIGURATION ===\n");
    printf("Microcontroller: Teensy 4.1\n");
    printf("Amplifier:       HiLetgo LM386 module (no enable pin)\n");
    printf("Power:           Adafruit PowerBoost 1000 Charge\n\n");

    printf("STRING BUTTONS (active HIGH - connect to 3.3V when pressed):\n");
    printf("  String 1 (E4, 329.63 Hz): GPIO %d\n", STRING_1_BUTTON_PIN);
    printf("  String 2 (B3, 246.94 Hz): GPIO %d\n", STRING_2_BUTTON_PIN);
    printf("  String 3 (G3, 196.00 Hz): GPIO %d\n", STRING_3_BUTTON_PIN);
    printf("  String 4 (D3, 146.83 Hz): GPIO %d\n", STRING_4_BUTTON_PIN);
    printf("  String 5 (A2, 110.00 Hz): GPIO %d\n", STRING_5_BUTTON_PIN);
    printf("  String 6 (E2,  82.41 Hz): GPIO %d\n\n", STRING_6_BUTTON_PIN);

    printf("MODE SWITCH:\n");
    printf("  Pin %d (HIGH = Play Tone, LOW = Listen Only)\n\n", MODE_SWITCH_PIN);

    printf("AUDIO OUTPUT (MQS):\n");
    printf("  Pin 10 (MQSR) -> LM386 IN\n");
    printf("  Pin 12 (MQSL) -> unconnected (mono)\n\n");

    printf("MICROPHONE:\n");
    printf("  Pin 39 (A17) - analog electret\n\n");

    printf("DSP:\n");
    printf("  FFT size: %d points\n", FFT_SIZE);
    printf("  Resolution: ~%d Hz/bin\n", FFT_HZ_PER_BIN);
    printf("  Sample rate: %d Hz\n\n", FFT_INPUT_SAMPLE_RATE);
}

void hardware_print_button_events(void) {
    printf("Button events captured: %lu\n", button_event_count);
}

uint32_t hardware_get_button_count(void) {
    return button_event_count;
}