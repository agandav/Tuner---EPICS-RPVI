/**
 * audio_processing.h - Audio processing interface
 *
 * FFT-based frequency detection for guitar tuning.
 *
 * CHANGES FROM ORIGINAL:
 *   1. Fixed unclosed comment block before function declarations -
 *      original had an unclosed /* which hid all declarations from
 *      the compiler, causing linker errors on every call site.
 *   2. Removed MIN_AMPLITUDE define - it conflicted with config.h
 *      which also defines MIN_AMPLITUDE (with a different value).
 *      Single source of truth is config.h (value: 100).
 */

#ifndef AUDIO_PROCESSING_H
#define AUDIO_PROCESSING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONFIGURATION CONSTANTS
 * ========================================================================== */

#define SAMPLE_RATE  10000   /* 10 kHz sample rate - sufficient for guitar 82-330 Hz */
#define SAMPLE_SIZE  1024    /* Number of ADC samples captured per FFT frame          */

/* NOTE: MIN_AMPLITUDE is defined in config.h (value: 100).
   Do not redefine it here. */

/* ============================================================================
 * FUNCTION DECLARATIONS
 * ========================================================================== */

/**
 * Initialize audio processing subsystem.
 * Must be called before using any other audio processing functions.
 */
void audio_processing_init(void);

/**
 * Remove DC offset from audio samples.
 * Subtracts the mean value to center signal around zero.
 *
 * @param samples     - Array of audio samples (modified in place)
 * @param num_samples - Number of samples
 */
void remove_dc_offset(int16_t* samples, int num_samples);

/**
 * Apply gain to audio samples with saturation (prevents clipping).
 *
 * @param samples     - Array of audio samples (modified in place)
 * @param num_samples - Number of samples
 * @param gain_factor - Multiplication factor (e.g., 2.0 = 2x amplification)
 */
void apply_gain(int16_t* samples, int num_samples, float gain_factor);

/**
 * Perform FFT and detect fundamental frequency.
 *
 * Pipeline:
 *   1. Validate signal amplitude (reject noise below MIN_AMPLITUDE)
 *   2. Convert int16_t samples to normalized float [-1, 1]
 *   3. Apply Hann window to reduce spectral leakage
 *   4. Run Cooley-Tukey radix-2 FFT (256-point)
 *   5. Compute magnitude spectrum
 *   6. Find peak frequency bin and convert to Hz
 *
 * @param samples     - Audio samples in int16_t PCM format
 * @param num_samples - Number of samples to process
 * @return Detected frequency in Hz, or 0.0 if no valid signal
 */
double apply_fft(const int16_t* samples, int num_samples);

/**
 * Capture audio and detect frequency (test/stub wrapper).
 *
 * @param detected_frequency - Output: detected frequency in Hz
 * @return 1 if frequency detected, 0 otherwise
 */
int audio_processing_capture(double* detected_frequency);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PROCESSING_H */