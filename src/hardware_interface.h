/**
 * hardware_interface.h - GPIO and Input/Output Interface
 *
 * Provides abstraction layer for:
 * - Button input (string selection)
 * - Volume control (rotary encoder/potentiometer)
 * - Audio amplifier control
 * - Accessibility features (Braille-labeled tactile buttons)
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
    BUTTON_PRESSED = 1
} button_state_t;

typedef enum {
    STRING_1_BUTTON = 1,    // E4
    STRING_2_BUTTON = 2,    // B3
    STRING_3_BUTTON = 3,    // G3
    STRING_4_BUTTON = 4,    // D3
    STRING_5_BUTTON = 5,    // A2
    STRING_6_BUTTON = 6,    // E2
    VOLUME_KNOB_CLICK = 7,  // Rotary encoder push
} button_id_t;

typedef struct {
    button_id_t button_id;
    button_state_t state;
    uint32_t timestamp_ms;
    uint32_t press_duration_ms;  // How long button was held
} button_event_t;

/* ============================================================================
 * VOLUME CONTROL TYPES
 * ========================================================================== */

typedef struct {
    float current_volume;       // 0.0 (silent) to 1.0 (max)
    uint16_t adc_raw;           // Raw ADC value (0-4095 for 12-bit)
    uint32_t last_update_ms;    // Timestamp of last volume change
} volume_control_t;

/* ============================================================================
 * INITIALIZATION & SETUP
 * ========================================================================== */

/**
 * Initialize GPIO pins for buttons, encoders, and audio control
 * Must be called in setup() before using input/output functions
 * Returns 0 on success, -1 on error
 */
int hardware_interface_init(void);

/* ============================================================================
 * BUTTON INPUT FUNCTIONS
 * ========================================================================== */

/**
 * Poll all string selection buttons
 * Should be called in main loop (every 10-20ms)
 * Returns 1 if button event detected, 0 if no change
 */
int button_poll(void);

/**
 * Get the most recent button event
 * Returns pointer to button_event_t, or NULL if no pending events
 */
button_event_t* button_get_event(void);

/**
 * Check if a specific button is currently pressed
 * Parameters: button_id (STRING_1_BUTTON through STRING_6_BUTTON)
 * Returns: true if pressed, false if released
 */
bool button_is_pressed(button_id_t button_id);

/**
 * Debounce button input (20ms minimum press time)
 * Called internally by button_poll()
 * Manual call only needed for custom button handling
 */
void button_debounce(button_id_t button_id);

/* ============================================================================
 * VOLUME CONTROL FUNCTIONS
 * ========================================================================== */

/**
 * Read analog volume input (potentiometer or encoder)
 * Should be called regularly (every 50ms) to update volume
 * Updates internal volume_control_t structure
 * Returns new volume (0.0 to 1.0)
 */
float volume_read_analog(void);

/**
 * Set volume directly (0.0 = silent, 1.0 = maximum)
 * Applies scaling to audio output immediately
 * Parameters: volume - normalized value [0.0, 1.0]
 */
void volume_set(float volume);

/**
 * Get current volume setting
 * Returns: Volume [0.0, 1.0]
 */
float volume_get(void);

/**
 * Adjust volume by increment (e.g., +0.1 or -0.1)
 * Clamps result to [0.0, 1.0] range
 * Parameters: delta - adjustment amount
 */
void volume_adjust(float delta);

/* ============================================================================
 * AUDIO AMPLIFIER CONTROL
 * ========================================================================== */

/**
 * Enable audio amplifier (sets GPIO high)
 * Call after audio initialization and before playing sounds
 */
void audio_amplifier_enable(void);

/**
 * Disable audio amplifier (sets GPIO low)
 * Call when shutting down or switching to headphones
 */
void audio_amplifier_disable(void);

/**
 * Check if amplifier is currently enabled
 * Returns: true if enabled, false if disabled
 */
bool audio_amplifier_is_enabled(void);

/* ============================================================================
 * MODE SWITCH (Play Tone I / Listen Only O)
 * ========================================================================== */

/**
 * Read the mode switch state
 * Returns: true if Play Tone mode (I), false if Listen Only mode (O)
 */
bool mode_switch_is_play_tone(void);

/**
 * Read the mode switch state
 * Returns: true if Listen Only mode (O), false if Play Tone mode (I)
 */
bool mode_switch_is_listen_only(void);

/* ============================================================================
 * ACCESSIBILITY FEATURES
 * ========================================================================== */

/**
 * Provide tactile feedback for button press (optional)
 * If hardware has piezo buzzer on TACTILE_FEEDBACK_PIN:
 * - Plays short click sound
 * Duration: 50ms
 * Returns: 0 on success, -1 if unavailable
 */
int tactile_feedback_click(void);

/**
 * Provide double-click feedback for confirmation
 * Plays two short beeps
 * Returns: 0 on success, -1 if unavailable
 */
int tactile_feedback_confirm(void);

/**
 * Provide warning feedback (three rapid beeps)
 * Used when tuning is very far off
 * Returns: 0 on success, -1 if unavailable
 */
int tactile_feedback_warning(void);

/* ============================================================================
 * DIAGNOSTIC FUNCTIONS
 * ========================================================================== */

/**
 * Print GPIO configuration and current pin states
 * For debugging during development
 */
void hardware_print_config(void);

/**
 * Print button event log (for accessibility testing)
 */
void hardware_print_button_events(void);

/**
 * Get number of button presses since last reset
 */
uint32_t hardware_get_button_count(void);

#ifdef __cplusplus
}
#endif

#endif /* HARDWARE_INTERFACE_H */
