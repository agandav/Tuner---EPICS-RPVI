/**
 * hardware_interface.h - GPIO and Input/Output Interface
 *
 * CHANGES FROM ORIGINAL:
 *   1. Removed audio_amplifier_enable(), audio_amplifier_disable(),
 *      audio_amplifier_is_enabled() - LM386 has no enable pin.
 *   2. Removed volume_read_analog(), volume_set(), volume_get(), volume_adjust()
 *      - volume is controlled by physical pot on LM386 module, no software control.
 *   3. Removed VOLUME_KNOB_CLICK from button_id_t - rotary encoder not in build.
 */

#ifndef HARDWARE_INTERFACE_H
#define HARDWARE_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * BUTTON INPUT TYPES
 * ========================================================================== */

typedef enum {
    BUTTON_RELEASED = 0,
    BUTTON_PRESSED  = 1
} button_state_t;

typedef enum {
    STRING_1_BUTTON = 1,    // E4 - pin 22
    STRING_2_BUTTON = 2,    // B3 - pin 3
    STRING_3_BUTTON = 3,    // G3 - pin 4
    STRING_4_BUTTON = 4,    // D3 - pin 5
    STRING_5_BUTTON = 5,    // A2 - pin 6
    STRING_6_BUTTON = 6     // E2 - pin 9
} button_id_t;

typedef struct {
    button_id_t    button_id;
    button_state_t state;
    uint32_t       timestamp_ms;
    uint32_t       press_duration_ms;
} button_event_t;

/* ============================================================================
 * INITIALIZATION
 * ========================================================================== */

/**
 * Initialize GPIO pins for buttons and mode switch.
 * Must be called in setup() before using input functions.
 * Returns 0 on success, -1 on error.
 */
int hardware_interface_init(void);

/* ============================================================================
 * BUTTON INPUT FUNCTIONS
 * ========================================================================== */

/**
 * Poll all string selection buttons.
 * Call every loop() iteration.
 * Returns 1 if button event detected, 0 if no change.
 */
int button_poll(void);

/**
 * Get the most recent button event.
 * Returns pointer to button_event_t, or NULL if no pending events.
 */
button_event_t* button_get_event(void);

/**
 * Check if a specific button is currently pressed.
 * Returns true if pressed, false if released.
 */
bool button_is_pressed(button_id_t button_id);

/**
 * Debounce a specific button.
 * Called internally by button_poll().
 */
void button_debounce(button_id_t button_id);

/* ============================================================================
 * MODE SWITCH
 * ========================================================================== */

/**
 * Returns true if mode switch is in Play Tone position (I).
 */
bool mode_switch_is_play_tone(void);

/**
 * Returns true if mode switch is in Listen Only position (O).
 */
bool mode_switch_is_listen_only(void);

/* ============================================================================
 * TACTILE FEEDBACK
 * ========================================================================== */

/**
 * Short click feedback for button press.
 * Returns 0 on success, -1 if unavailable.
 */
int tactile_feedback_click(void);

/**
 * Double-click feedback for confirmation.
 * Returns 0 on success, -1 if unavailable.
 */
int tactile_feedback_confirm(void);

/**
 * Three rapid beeps for warning (signal too weak, etc.).
 * Returns 0 on success, -1 if unavailable.
 */
int tactile_feedback_warning(void);

/* ============================================================================
 * DIAGNOSTICS
 * ========================================================================== */

void hardware_print_config(void);
void hardware_print_button_events(void);
uint32_t hardware_get_button_count(void);

#ifdef __cplusplus
}
#endif

#endif /* HARDWARE_INTERFACE_H */