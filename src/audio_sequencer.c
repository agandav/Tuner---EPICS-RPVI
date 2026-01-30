/**
 * audio_sequencer_v2.c - Audio sequencing with synthesized tones
 *
 * NO WAV FILES REQUIRED!
 * All audio is generated in real-time using the Teensy Audio Library
 * Uses CFugue-style notation for clean, readable code
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "audio_sequencer.h"
#include "note_parser.h"

/* ============================================================================
 * STATE VARIABLES
 * ========================================================================== */

static int is_playing = 0;
static const TuningResult* current_result = NULL;
static int playback_step = 0;

/* Dynamic beep state */
static uint32_t last_beep_time = 0;
static uint32_t beep_end_time = 0;
static int beeping_active = 0;

/* Current string being tuned */
static int current_string = 1;

/* ============================================================================
 * BEEP RATE CONFIGURATION (Same as original)
 * ========================================================================== */

typedef struct {
	double max_cents;
	uint32_t beep_interval;
	uint32_t beep_duration;
} BeepRateConfig;

static const BeepRateConfig beep_rates[] = {
	{100.0,  100},   // > 100 cents off: fastest
	{75.0,   150},   // 75-100 cents: very fast
	{50.0,   200},   // 50-75 cents: fast
	{40.0,   300},   // 40-50 cents: medium-fast
	{25.0,   500},   // 25-40 cents: medium
	{15.0,   800},   // 15-25 cents: slow
	{5.0,    1200},  // 5-15 cents: very slow
	{0.0,    0},     // < 5 cents: no beep (in tune)
};

#define NUM_BEEP_RATES (sizeof(beep_rates) / sizeof(BeepRateConfig))

/* ============================================================================
 * TEENSY AUDIO LIBRARY INTERFACE
 * Now connected to actual audio hardware!
 * ========================================================================== */

#ifndef ARDUINO
// Native platform stubs - no actual audio output
static void play_tone(float frequency, uint32_t duration_ms) {
	(void)frequency;
	(void)duration_ms;
}

static void play_beep(float frequency, uint32_t duration_ms) {
	(void)frequency;
	(void)duration_ms;
}

static void stop_all_audio(void) {
	// No-op for native
}
#else
// Import functions from teensy_audio_io.cpp for Teensy
void play_tone(float frequency, uint32_t duration_ms);
void play_beep(float frequency, uint32_t duration_ms);
void stop_all_audio(void);
#endif

/**
 * Play a tone using the Teensy Audio Library
 * This now makes REAL SOUND through your speaker!
 */
static void teensy_play_tone(float frequency, uint32_t duration_ms) {
	printf("[TEENSY AUDIO] Playing %.2f Hz for %lu ms\n", frequency, duration_ms);
	play_tone(frequency, duration_ms);  // ? ACTUAL AUDIO OUTPUT!
}

/**
 * Play a beep using the Teensy Audio Library
 * This now makes REAL BEEPS through your speaker!
 */
static void teensy_play_beep(float frequency, uint32_t duration_ms) {
	printf("[BEEP] %.2f Hz for %lu ms\n", frequency, duration_ms);
	play_beep(frequency, duration_ms);  // ? ACTUAL BEEP OUTPUT!
}

/**
 * Stop all audio output
 * This now actually silences the speaker!
 */
static void teensy_stop_audio(void) {
	printf("[TEENSY AUDIO] Stopping all audio\n");
	stop_all_audio();  // ? ACTUALLY STOPS AUDIO!
}

/* ============================================================================
 * SYNTHESIZED TONE PLAYBACK (CFugue-style)
 * ========================================================================== */

void playNoteTone(const char* note, uint32_t duration_ms) {
	float frequency = parseNote(note);
	
	if (frequency > 0.0f) {
		printf("[PLAY NOTE] %s = %.2f Hz for %lu ms\n", note, frequency, duration_ms);
		teensy_play_tone(frequency, duration_ms);
	} else {
		printf("[ERROR] Invalid note: %s\n", note);
	}
}

void playFrequencyTone(float frequency, uint32_t duration_ms) {
	teensy_play_tone(frequency, duration_ms);
}

void playBeep(float frequency, uint32_t duration_ms) {
	teensy_play_beep(frequency, duration_ms);
}

void stopAllTones(void) {
	teensy_stop_audio();
}

/* ============================================================================
 * GUITAR STRING PLAYBACK
 * ========================================================================== */

void playGuitarString(int string_num, uint32_t duration_ms) {
	const char* note = getStringNote(string_num);
	
	if (note != NULL) {
		printf("[PLAY STRING %d] %s\n", string_num, note);
		playNoteTone(note, duration_ms);
	} else {
		printf("[ERROR] Invalid string number: %d\n", string_num);
	}
}

void playCurrentString(void) {
	playGuitarString(current_string, 2000);  // 2 second default
}

/* ============================================================================
 * AUDIO CUE SYNTHESIS (Replaces WAV Files)
 * ========================================================================== */

void playStringIdentifier(int string_num) {
	printf("[STRING ID] String %d: ", string_num);
	
	// Each string has a unique beep pattern
	float beep_freq = 800.0f;  // Base frequency for identifier beeps
	uint32_t beep_duration = 100;
	uint32_t beep_pause = 150;
	
	// Play N beeps for string N
	for (int i = 0; i < string_num; i++) {
		teensy_play_beep(beep_freq, beep_duration);
		if (i < string_num - 1) {
			// Pause between beeps (simulate with delay)
			printf("...");
		}
	}
	printf("\n");
}

void playCentsIndicator(double cents_offset) {
	double abs_cents = fabs(cents_offset);
	
	if (abs_cents < 5.0) {
		printf("[CENTS] In tune! (%.1f cents)\n", cents_offset);
		// Play happy "in tune" tone
		teensy_play_tone(1000.0f, 200);
	} else if (abs_cents < 15.0) {
		printf("[CENTS] Slightly off (~10 cents): %.1f\n", cents_offset);
		teensy_play_beep(600.0f, 100);
	} else if (abs_cents < 25.0) {
		printf("[CENTS] Off (~20 cents): %.1f\n", cents_offset);
		teensy_play_beep(500.0f, 100);
		teensy_play_beep(500.0f, 100);
	} else {
		printf("[CENTS] Way off (>25 cents): %.1f\n", cents_offset);
		teensy_play_beep(400.0f, 100);
		teensy_play_beep(400.0f, 100);
		teensy_play_beep(400.0f, 100);
	}
}

void playDirectionCue(const char* direction) {
	if (strcmp(direction, "UP") == 0) {
		printf("[DIRECTION] Tune UP (tighten string)\n");
		// Rising tone
		teensy_play_tone(400.0f, 150);
		teensy_play_tone(600.0f, 150);
	} else if (strcmp(direction, "DOWN") == 0) {
		printf("[DIRECTION] Tune DOWN (loosen string)\n");
		// Falling tone
		teensy_play_tone(600.0f, 150);
		teensy_play_tone(400.0f, 150);
	} else if (strcmp(direction, "IN_TUNE") == 0) {
		printf("[DIRECTION] IN TUNE!\n");
		// Happy ascending tones
		teensy_play_tone(523.0f, 100);  // C5
		teensy_play_tone(659.0f, 100);  // E5
		teensy_play_tone(784.0f, 200);  // G5
	} else {
		printf("[DIRECTION] Unknown: %s\n", direction);
	}
}

/* ============================================================================
 * ENHANCED FEEDBACK MODES
 * ========================================================================== */

void generate_synthesized_feedback(const TuningResult* result) {
	if (result == NULL) {
		return;
	}
	
	printf("\n=== SYNTHESIZED AUDIO FEEDBACK ===\n");
	
	// 1. Identify which string
	playStringIdentifier(result->detected_string);
	
	// 2. Play reference tone for that string
	printf("[REFERENCE] Playing target note: %s\n", 
	       getStringNote(result->target_string));
	playGuitarString(result->target_string, 1000);
	
	// 3. Indicate how far off
	playCentsIndicator(result->cents_offset);
	
	// 4. Show direction
	playDirectionCue(result->direction);
	
	printf("=== FEEDBACK COMPLETE ===\n\n");
}

void playReferenceMode(int string_num) {
	printf("\n[REFERENCE MODE] Playing string %d\n", string_num);
	playGuitarString(string_num, 3000);  // Longer duration for practice
}

void tuningAssistantMode(int string_num) {
	current_string = string_num;
	
	printf("\n=== TUNING ASSISTANT MODE ===\n");
	printf("Target string: %d (%s)\n", string_num, getStringNote(string_num));
	
	// Play reference tone
	printf("Playing reference tone...\n");
	playGuitarString(string_num, 2000);
	
	printf("Now play your string and listen for feedback beeps\n");
	printf("Faster beeps = further from tune\n");
	printf("Slower beeps = closer to tune\n");
	printf("No beeps = in tune!\n");
	printf("================================\n\n");
}

/* ============================================================================
 * ORIGINAL FUNCTIONS (Maintained for Compatibility)
 * ========================================================================== */

uint32_t calculate_beep_interval(double cents_offset) {
	double abs_cents = fabs(cents_offset);
	
	for (int i = 0; i < NUM_BEEP_RATES; i++) {
		if (abs_cents >= beep_rates[i].max_cents) {
			return beep_rates[i].beep_interval;
		}
	}
	
	return 0;
}

void generate_dynamic_beep_feedback(const TuningResult* result) {
	if (result == NULL) {
		beeping_active = 0;
		return;
	}
	
	uint32_t beep_interval = calculate_beep_interval(result->cents_offset);
	
	if (beep_interval == 0) {
		beeping_active = 0;
		printf("[BEEP] In tune! No beeping.\n");
	} else {
		beeping_active = 1;
		last_beep_time = 0;
		printf("[BEEP] Starting beeps at %lu ms interval (offset: %.1f cents)\n", 
		       beep_interval, result->cents_offset);
	}
	
	current_result = result;
}

void audio_sequencer_init(void) {
	printf("Audio Sequencer V2 initialized (CFugue-style notation)\n");
	printf("  - Synthesized audio mode (NO WAV FILES NEEDED)\n");
	printf("  - CFugue-style note notation support\n");
	printf("  - Dynamic beep feedback\n");
	
	// Initialize note parser
	note_parser_init();
	
	is_playing = 0;
	playback_step = 0;
	beeping_active = 0;
	last_beep_time = 0;
	beep_end_time = 0;
	current_string = 1;
}

void generate_audio_feedback(const TuningResult* result) {
	// Use new synthesized feedback instead
	generate_synthesized_feedback(result);
}

void audio_sequencer_update(void) {
	// Legacy function - kept for compatibility
	if (!is_playing || current_result == NULL) {
		return;
	}
	
	// Original playback sequence (now uses synthesis)
	switch (playback_step) {
		case 0:
			playStringIdentifier(current_result->detected_string);
			playback_step++;
			break;
		case 1:
			if (strcmp(current_result->direction, "IN_TUNE") != 0) {
				playCentsIndicator(current_result->cents_offset);
			}
			playback_step++;
			break;
		case 2:
			playDirectionCue(current_result->direction);
			playback_step++;
			break;
		default:
			is_playing = 0;
			playback_step = 0;
			printf("Audio feedback complete.\n");
			break;
	}
}

void audio_sequencer_update_beeps(uint32_t current_time_ms) {
	if (!beeping_active || current_result == NULL) {
		return;
	}
	
	uint32_t beep_interval = calculate_beep_interval(current_result->cents_offset);
	
	if (beep_interval == 0) {
		beeping_active = 0;
		return;
	}
	
	if (current_time_ms >= last_beep_time + beep_interval) {
		// Time to beep!
		teensy_play_beep(800.0f, 50);  // 800 Hz beep, 50ms duration
		last_beep_time = current_time_ms;
		beep_end_time = current_time_ms + 50;
	}
}