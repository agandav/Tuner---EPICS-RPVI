/**
 * audio_processing.h - Audio processing interface
 *
 * FFT-based frequency detection for guitar tuning
 */

#ifndef AUDIO_PROCESSING_H
#define AUDIO_PROCESSING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CONFIGURATION CONSTANTS
 */

#define SAMPLE_RATE 10000           /* 10 kHz sample rate for guitar tuning */
#define SAMPLE_SIZE 1024            /* Number of samples to capture */
#define MIN_AMPLITUDE 500           /* Minimum signal amplitude threshold */

/* 
 * FUNCTION DECLARATIONS

/**
 * Initialize audio processing subsystem
 * Must be called before using any other audio processing functions
 */
void audio_processing_init(void);

/**
 * Remove DC offset from audio samples
 * Subtracts the mean value to center signal around zero
 * 
 * @param samples - Array of audio samples
 * @param num_samples - Number of samples in array
 */
void remove_dc_offset(int16_t* samples, int num_samples);

/**
 * Apply gain to audio samples with saturation
 * Amplifies signal while preventing clipping
 * 
 * @param samples - Array of audio samples
 * @param num_samples - Number of samples in array
 * @param gain_factor - Multiplication factor for gain (e.g., 2.0 = 2x)
 */
void apply_gain(int16_t* samples, int num_samples, float gain_factor);

/**
 * Perform FFT and detect fundamental frequency
 * 
 * This is the main frequency detection function that:
 * 1. Validates signal amplitude
 * 2. Converts samples to float
 * 3. Performs FFT computation
 * 4. Computes magnitude spectrum
 * 5. Finds peak frequency
 * 
 * @param samples - Array of audio samples (int16_t PCM format)
 * @param num_samples - Number of samples to process
 * @return Detected frequency in Hz, or 0.0 if no valid signal
 */
double apply_fft(const int16_t* samples, int num_samples);

/**
 * Capture audio and detect frequency (wrapper function)
 * 
 * @param detected_frequency - Output: detected frequency in Hz
 * @return 1 if frequency detected, 0 otherwise
 */
int audio_processing_capture(double* detected_frequency);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PROCESSING_H */