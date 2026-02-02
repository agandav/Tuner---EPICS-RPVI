/**
 * string_detection.c - Guitar string detection and tuning analysis
 *
 * Implements algorithms for:
 * - Identifying which guitar string is being played
 * - Calculating cents offset from perfect tuning
 * - Determining tuning direction (up/down)
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "string_detection.h"

typedef struct {
	double frequency;
	const char* note_name;
	int string_number;
	int octave;
} NoteFrequency;

const NoteFrequency guitar_notes[] = { // Frequency, Note, String, Octave
	{82, "E", 6, 2}, // 82hz, Note E, String 6, Octave 2
	{87, "F", 6, 2},
	{93, "F#", 6, 2},
	{98, "G", 6, 2},
	{104, "G#", 6, 2},
	{110, "A", 5, 2},
	{116., "A#", 5, 2},
	{124, "B", 5, 2},
	{131, "C", 5, 3},
	{139, "C#", 5, 3},
	{147, "D", 4, 3},
	{156, "D#", 4, 3},
	{165, "E", 4, 3},
	{175, "F", 4, 3},
	{185, "F#", 4, 3},
	{196, "G", 3, 3},
	{208, "G#", 3, 3},
	{220, "A", 3, 3},
	{233, "A#", 3, 3},
	{247, "B", 2, 3},
	{262, "C", 2, 4},
	{278, "C#", 2, 4},
	{294, "D", 2, 4},
	{311, "D#", 2, 4},
	{330, "E", 1, 4},
	{349, "F", 1, 4},
	{370, "F#", 1, 4},
	{392, "G", 1, 4},
	{415, "G#", 1, 4},
	{440, "A", 1, 4},
	{466, "A#", 1, 4},
	{494, "B", 1, 4},
	{523, "C", 1, 5} 
};

#define NUM_NOTES (sizeof(guitar_notes) / sizeof(guitar_notes[0]))

const double string_frequencies[] = {
	GUITAR_STRING_1_FREQ,
	GUITAR_STRING_2_FREQ,
	GUITAR_STRING_3_FREQ,
	GUITAR_STRING_4_FREQ,
	GUITAR_STRING_5_FREQ,
	GUITAR_STRING_6_FREQ
};

void string_detection_init(void) {
	printf("String detection module initialized.\n");
	printf("Number of notes in database: %ld\n", NUM_NOTES);
}

double calculate_cents_offset(double detected_freq, double target_freq) {
	if (target_freq <= 0 || detected_freq <= 0) {
		return 0.0;
	}
	return 1200.0 * log2(detected_freq / target_freq);
}

const char* get_tuning_direction(double cents) {
	const double TUNING_TOLERANCE = 2.0; // +/-2 cents considered "in tune"
	if (cents < -TUNING_TOLERANCE) {
		return "UP";
	} else if (cents > TUNING_TOLERANCE) {
		return "DOWN";
	} else {
		return "IN_TUNE";
	}
}

int find_closest_string(double frequency, double* closest_freq) {
	double min_diff = 1000.0;
	int closest_string = -1;
	for (int i = 0; i < 6; i++) {
		double diff = fabs(frequency - string_frequencies[i]);
		if (diff < min_diff) {
			min_diff = diff;
			closest_string = i + 1;
			*closest_freq = string_frequencies[i];
		}
	}
	return closest_string;
}

int find_closest_note(double frequency, double* closest_freq, int* string_num) {
	double min_diff = 1000.0;
	int closest_index = -1;
	for (int i = 0; i < NUM_NOTES; i++) {
		double diff = fabs(frequency - guitar_notes[i].frequency);
		if (diff < min_diff) {
			min_diff = diff;
			closest_index = i;
			*closest_freq = guitar_notes[i].frequency;
			*string_num = guitar_notes[i].string_number;
		}
	}
	return closest_index;
}

TuningResult analyze_tuning(double detected_frequency, int target_string) {
	TuningResult result;
	if (target_string < 1 || target_string > 6) {
		return analyze_tuning_auto(detected_frequency);
	}
	result.target_frequency = string_frequencies[target_string - 1];
	result.target_string = target_string;
	double detected_string_freq;
	result.detected_string = find_closest_string(detected_frequency, &detected_string_freq);
	result.detected_frequency = detected_frequency;
	result.cents_offset = calculate_cents_offset(detected_frequency, result.target_frequency);
	if (detected_frequency <= 0.0) {
		result.direction = "UNKNOWN";
	} else {
		result.direction = get_tuning_direction(result.cents_offset);
	}
	double closest_note_freq;
	int note_string;
	int note_index = find_closest_note(detected_frequency, &closest_note_freq, &note_string);
	if (note_index >= 0) {
		result.note_name = guitar_notes[note_index].note_name;
		result.octave = guitar_notes[note_index].octave;
	} else {
		result.note_name = "?";
		result.octave = 0;
	}
	return result;
}

TuningResult analyze_tuning_auto(double detected_frequency) {
	TuningResult result;
	double target_freq;
	result.detected_string = find_closest_string(detected_frequency, &target_freq);
	result.target_string = result.detected_string;
	result.detected_frequency = detected_frequency;
	result.target_frequency = target_freq;
	result.cents_offset = calculate_cents_offset(detected_frequency, target_freq);
	if (detected_frequency <= 0.0) {
		result.direction = "UNKNOWN";
	} else {
		result.direction = get_tuning_direction(result.cents_offset);
	}
	double closest_note_freq;
	int note_string;
	int note_index = find_closest_note(detected_frequency, &closest_note_freq, &note_string);
	if (note_index >= 0) {
		result.note_name = guitar_notes[note_index].note_name;
		result.octave = guitar_notes[note_index].octave;
	} else {
		result.note_name = "?";
		result.octave = 0;
	}
	return result;
}
