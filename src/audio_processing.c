/**
 * audio_processing.c - Audio processing with custom Radix-2 FFT
 *
 * CHANGES FROM ORIGINAL:
 *   1. Removed arm_math.h include entirely. The original file included
 *      CMSIS-DSP (arm_math.h) but never actually called any CMSIS function -
 *      the FFT used is the custom simple_radix2_fft() written in pure C.
 *      The include caused a build failure unless CMSIS-DSP was installed
 *      and USE_ARM_MATH_MOCK was defined. Both guards removed.
 *   2. MIN_AMPLITUDE now comes from config.h (value: 100) rather than
 *      audio_processing.h (which had it at 500, now removed to avoid conflict).
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "audio_processing.h"
#include "config.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

/* FFT configuration */
#define FFT_SIZE 256   /* 256-point FFT: 10kHz / 256 = ~39 Hz/bin */

/* Static FFT buffers */
static float fft_real[FFT_SIZE];
static float fft_imag[FFT_SIZE];
static float magnitude_spectrum[FFT_SIZE / 2];
static int   fft_initialized = 0;

/* ============================================================================
 * BIT REVERSAL
 * ========================================================================== */

static void bit_reverse_permute(float *data_real, float *data_imag, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        uint32_t j        = i;
        uint32_t reversed = 0;

        for (uint32_t k = 0; k < 8; k++) {   /* log2(256) = 8 */
            reversed = (reversed << 1) | (j & 1);
            j >>= 1;
        }

        if (i < reversed) {
            float tmp      = data_real[i];
            data_real[i]   = data_real[reversed];
            data_real[reversed] = tmp;

            tmp            = data_imag[i];
            data_imag[i]   = data_imag[reversed];
            data_imag[reversed] = tmp;
        }
    }
}

/* ============================================================================
 * COOLEY-TUKEY RADIX-2 FFT (pure C, no CMSIS, no NEON)
 * ========================================================================== */

static void simple_radix2_fft(float *real, float *imag, uint32_t n) {
    bit_reverse_permute(real, imag, n);

    for (uint32_t stage = 0; stage < 8; stage++) {       /* log2(256) = 8 stages */
        uint32_t stage_size   = 1u << stage;
        uint32_t stage_stride = stage_size << 1;

        for (uint32_t i = 0; i < n; i += stage_stride) {
            for (uint32_t j = 0; j < stage_size; j++) {
                uint32_t idx_a = i + j;
                uint32_t idx_b = i + j + stage_size;

                float angle  = -2.0f * PI * j / (float)stage_stride;
                float w_real = cosf(angle);
                float w_imag = sinf(angle);

                float t_real = w_real * real[idx_b] - w_imag * imag[idx_b];
                float t_imag = w_real * imag[idx_b] + w_imag * real[idx_b];

                real[idx_b] = real[idx_a] - t_real;
                imag[idx_b] = imag[idx_a] - t_imag;
                real[idx_a] = real[idx_a] + t_real;
                imag[idx_a] = imag[idx_a] + t_imag;
            }
        }
    }
}

/* ============================================================================
 * HANN WINDOW
 * ========================================================================== */

static void apply_hann_window(float *data, int num_samples) {
    for (int i = 0; i < num_samples; i++) {
        float window = 0.5f * (1.0f - cosf(2.0f * PI * i / (num_samples - 1)));
        data[i] *= window;
    }
}

/* ============================================================================
 * PEAK DETECTION
 * ========================================================================== */

static double find_peak_frequency(const float *magnitude, uint32_t num_bins,
                                  uint32_t sampling_rate) {
    uint32_t peak_bin       = 0;
    float    peak_magnitude = 0.0f;

    /* Search up to 2000 Hz - covers all guitar fundamentals and first harmonics */
    uint32_t search_limit = (num_bins * 2000) / sampling_rate;
    if (search_limit > num_bins) search_limit = num_bins;

    for (uint32_t i = 1; i < search_limit; i++) {
        if (magnitude[i] > peak_magnitude) {
            peak_magnitude = magnitude[i];
            peak_bin       = i;
        }
    }

    if (peak_magnitude < 0.5f) {
        return 0.0;   /* No significant peak - noise or silence */
    }

    return (double)peak_bin * sampling_rate / FFT_SIZE;
}

/* ============================================================================
 * PUBLIC API
 * ========================================================================== */

void audio_processing_init(void) {
    printf("Audio processing initialized (Radix-2 FFT, pure C).\n");
    printf("Sample rate: %d Hz | FFT size: %d | Buffer: %d samples\n",
           SAMPLE_RATE, FFT_SIZE, SAMPLE_SIZE);
    fft_initialized = 1;
}

void remove_dc_offset(int16_t *samples, int num_samples) {
    long sum = 0;
    for (int i = 0; i < num_samples; i++) sum += samples[i];
    int16_t dc = (int16_t)(sum / num_samples);
    for (int i = 0; i < num_samples; i++) samples[i] -= dc;
}

void apply_gain(int16_t *samples, int num_samples, float gain_factor) {
    for (int i = 0; i < num_samples; i++) {
        int32_t scaled = (int32_t)(samples[i] * gain_factor);
        if      (scaled >  32767) samples[i] =  32767;
        else if (scaled < -32768) samples[i] = -32768;
        else                      samples[i] = (int16_t)scaled;
    }
}

double apply_fft(const int16_t *samples, int num_samples) {
    if (!fft_initialized) {
        printf("ERROR: FFT not initialized - call audio_processing_init() first\n");
        return 0.0;
    }
    if (samples == NULL || num_samples == 0) return 0.0;

    /* STEP 1: Check signal amplitude - reject noise */
    int max_amplitude = 0;
    for (int i = 0; i < num_samples; i++) {
        int a = abs(samples[i]);
        if (a > max_amplitude) max_amplitude = a;
    }
    if (max_amplitude < MIN_AMPLITUDE) return 0.0;

    /* STEP 2: Convert to float, normalize to [-1, 1], zero-pad to FFT_SIZE */
    uint32_t input_size = (num_samples < FFT_SIZE) ? (uint32_t)num_samples : FFT_SIZE;
    for (uint32_t i = 0; i < input_size; i++) {
        fft_real[i] = (float)samples[i] / 32768.0f;
        fft_imag[i] = 0.0f;
    }
    for (uint32_t i = input_size; i < FFT_SIZE; i++) {
        fft_real[i] = 0.0f;
        fft_imag[i] = 0.0f;
    }

    /* STEP 3: Apply Hann window to reduce spectral leakage */
    apply_hann_window(fft_real, FFT_SIZE);

    /* STEP 4: Run FFT */
    simple_radix2_fft(fft_real, fft_imag, FFT_SIZE);

    /* STEP 5: Compute magnitude spectrum */
    uint32_t num_bins = FFT_SIZE / 2;
    for (uint32_t i = 0; i < num_bins; i++) {
        magnitude_spectrum[i] = sqrtf(fft_real[i] * fft_real[i] +
                                      fft_imag[i] * fft_imag[i]);
    }

    /* STEP 6: Find and return peak frequency */
    return find_peak_frequency(magnitude_spectrum, num_bins, SAMPLE_RATE);
}

int audio_processing_capture(double *detected_frequency) {
    int16_t samples[SAMPLE_SIZE];
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        samples[i] = (int16_t)(1000 * sinf(2.0f * PI * 440.0f * i / SAMPLE_RATE));
    }
    remove_dc_offset(samples, SAMPLE_SIZE);
    apply_gain(samples, SAMPLE_SIZE, 2.0f);
    double freq = apply_fft(samples, SAMPLE_SIZE);
    if (freq > 0.0) {
        *detected_frequency = freq;
        return 1;
    }
    return 0;
}