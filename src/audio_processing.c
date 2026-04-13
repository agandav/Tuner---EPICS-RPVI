/**
 * audio_processing.c - Audio processing with custom Radix-2 FFT
 *
 * CHANGES FROM ORIGINAL:
 *   1. Removed arm_math.h include entirely.
 *   2. MIN_AMPLITUDE now comes from config.h.
 *   3. 1024-point FFT with parabolic interpolation.
 *   4. Moving average removed — caused echo contamination.
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

static float fft_real[1024];
static float fft_imag[1024];
static float magnitude_spectrum[FFT_SIZE / 2];
static int   fft_initialized = 0;

/* ============================================================================
 * BIT REVERSAL
 * ========================================================================== */
static void bit_reverse_permute(float *data_real, float *data_imag, uint32_t n) {
    uint32_t num_bits = 0;
    uint32_t temp = n;
    while (temp > 1) { num_bits++; temp >>= 1; }

    for (uint32_t i = 0; i < n; i++) {
        uint32_t j = i, reversed = 0;
        for (uint32_t k = 0; k < num_bits; k++) {
            reversed = (reversed << 1) | (j & 1);
            j >>= 1;
        }
        if (i < reversed) {
            float tmp = data_real[i];
            data_real[i] = data_real[reversed];
            data_real[reversed] = tmp;
            tmp = data_imag[i];
            data_imag[i] = data_imag[reversed];
            data_imag[reversed] = tmp;
        }
    }
}

/* ============================================================================
 * COOLEY-TUKEY RADIX-2 FFT
 * ========================================================================== */
static void simple_radix2_fft(float *real, float *imag, uint32_t n) {
    bit_reverse_permute(real, imag, n);

    uint32_t num_stages = 0;
    uint32_t temp = n;
    while (temp > 1) { num_stages++; temp >>= 1; }

    for (uint32_t stage = 0; stage < num_stages; stage++) {
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
 * PEAK DETECTION with HPS and Parabolic Interpolation
 * ========================================================================== */
static double find_peak_frequency(const float *magnitude, uint32_t num_bins,
                                  uint32_t sampling_rate) {
    uint32_t min_bin = (uint32_t)((MIN_DETECTABLE_FREQ * FFT_SIZE) / sampling_rate);
    uint32_t max_bin = (uint32_t)((MAX_DETECTABLE_FREQ * FFT_SIZE) / sampling_rate);

    if (min_bin < 1) min_bin = 1;
    if (max_bin >= num_bins) max_bin = num_bins - 1;
    if (max_bin <= min_bin) return 0.0;

    uint32_t peak_bin = 0;
    float    peak_hps = 0.0f;

    for (uint32_t i = min_bin; i <= max_bin; i++) {
        uint32_t bin2 = i * 2;
        uint32_t bin3 = i * 3;

        float m1 = magnitude[i];
        float m2 = (bin2 < num_bins) ? magnitude[bin2] : 0.0f;
        float m3 = (bin3 < num_bins) ? magnitude[bin3] : 0.0f;

        float hps = m1 * m2 * m3;
        if (hps > peak_hps) {
            peak_hps = hps;
            peak_bin = i;
        }
    }

    if (peak_hps < 0.000001f) return 0.0;

    /* Parabolic interpolation */
    double offset = 0.0;
    if (peak_bin > 0 && peak_bin < num_bins - 1) {
        float m_prev = magnitude[peak_bin - 1];
        float m_curr = magnitude[peak_bin];
        float m_next = magnitude[peak_bin + 1];
        float denom  = m_prev - 2.0f * m_curr + m_next;
        if (fabsf(denom) > 1e-10f) {
            offset = 0.5 * (m_prev - m_next) / denom;
            if (offset >  0.5) offset =  0.5;
            if (offset < -0.5) offset = -0.5;
        }
    }

    return ((double)peak_bin + offset) * sampling_rate / FFT_SIZE;
}

/* ============================================================================
 * PUBLIC API
 * ========================================================================== */
void audio_processing_init(void) {
    printf("Audio processing initialized (Radix-2 FFT, pure C).\n");
    printf("Sample rate: %d Hz | FFT size: %d | Buffer: %d samples\n",
           SAMPLE_RATE, FFT_SIZE, SAMPLE_SIZE);
    printf("Resolution: %.2f Hz/bin | With interpolation: ~+-1 Hz typical accuracy\n",
           (float)SAMPLE_RATE / FFT_SIZE);
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
    if (!fft_initialized) return 0.0;
    if (samples == NULL || num_samples == 0) return 0.0;

    int32_t sum = 0;
    for (int i = 0; i < num_samples; i++) sum += samples[i];
    int16_t dc_offset = (int16_t)(sum / num_samples);

    int max_amplitude = 0;
    for (int i = 0; i < num_samples; i++) {
        int a = abs(samples[i] - dc_offset);
        if (a > max_amplitude) max_amplitude = a;
    }
    if (max_amplitude < MIN_AMPLITUDE) return 0.0;

    uint32_t input_size = (num_samples < FFT_SIZE) ? (uint32_t)num_samples : FFT_SIZE;
    for (uint32_t i = 0; i < input_size; i++) {
        fft_real[i] = (float)(samples[i] - dc_offset) / 32768.0f;
        fft_imag[i] = 0.0f;
    }
    for (uint32_t i = input_size; i < FFT_SIZE; i++) {
        fft_real[i] = 0.0f;
        fft_imag[i] = 0.0f;
    }

    apply_hann_window(fft_real, FFT_SIZE);
    simple_radix2_fft(fft_real, fft_imag, FFT_SIZE);

    uint32_t num_bins = FFT_SIZE / 2;
    for (uint32_t i = 0; i < num_bins; i++) {
        magnitude_spectrum[i] = sqrtf(fft_real[i] * fft_real[i] +
                                      fft_imag[i] * fft_imag[i]);
    }

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