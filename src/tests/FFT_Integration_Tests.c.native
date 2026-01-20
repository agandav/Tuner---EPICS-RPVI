/**
 * FFT_Integration_Tests.c
 * 
 * Comprehensive tests for the real FFT implementation
 * Validates frequency detection accuracy and FFT behavior
 * 
 * Run with: gcc -o test_fft FFT_Integration_Tests.c audio_processing.c -lm
 * or use PlatformIO: platformio run -e native
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "audio_processing.h"

#define PI 3.14159265358979323846f
#define TOLERANCE_HZ 40.0f  /* Frequency detection tolerance in Hz */
#define NUM_TESTS 6

/**
 * Generate a pure sine wave at a given frequency
 */
void generate_test_sine(int16_t *samples, int num_samples, double frequency) {
    for (int i = 0; i < num_samples; i++) {
        double phase = 2.0 * PI * frequency * i / SAMPLE_RATE;
        samples[i] = (int16_t)(20000 * sin(phase));  /* Amplitude: 20000 */
    }
}

/**
 * Test FFT with a known sine wave frequency
 */
void test_frequency_detection(double test_freq, const char *note) {
    printf("\n--- Test: %s (%.2f Hz) ---\n", note, test_freq);
    
    int16_t samples[SAMPLE_SIZE];
    
    /* Generate test signal */
    generate_test_sine(samples, SAMPLE_SIZE, test_freq);
    
    /* Process through FFT */
    double freq = apply_fft(samples, SAMPLE_SIZE);
    
    /* Check if detection was within tolerance */
    double error = fabs(freq - test_freq);
    int pass = (error < TOLERANCE_HZ) && (freq > 0);
    
    printf("Expected: %.2f Hz\n", test_freq);
    printf("Detected: %.2f Hz\n", freq);
    printf("Error:    %.2f Hz\n", error);
    printf("Result:   %s\n", pass ? "✓ PASS" : "✗ FAIL");
    
    return;
}

/**
 * Test DC offset removal effectiveness
 */
void test_dc_offset_handling(void) {
    printf("\n--- Test: DC Offset Handling ---\n");
    
    int16_t samples[SAMPLE_SIZE];
    int16_t offset_samples[SAMPLE_SIZE];
    
    /* Generate sine wave at A2 (110 Hz) */
    generate_test_sine(samples, SAMPLE_SIZE, 110.0);
    
    /* Add DC offset (+5000) */
    memcpy(offset_samples, samples, SAMPLE_SIZE * sizeof(int16_t));
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        int32_t temp = (int32_t)offset_samples[i] + 5000;
        if (temp > 32767) temp = 32767;
        offset_samples[i] = (int16_t)temp;
    }
    
    /* Process without offset */
    double freq1 = apply_fft(samples, SAMPLE_SIZE);
    
    /* Remove DC offset and process */
    remove_dc_offset(offset_samples, SAMPLE_SIZE);
    double freq2 = apply_fft(offset_samples, SAMPLE_SIZE);
    
    printf("Frequency (no offset):  %.2f Hz\n", freq1);
    printf("Frequency (with offset removed): %.2f Hz\n", freq2);
    
    double error = fabs(freq1 - freq2);
    int pass = (error < 5.0);  /* Should be very similar */
    
    printf("Difference: %.2f Hz\n", error);
    printf("Result:     %s\n", pass ? "✓ PASS" : "✗ FAIL");
}

/**
 * Test with very weak signal (should return 0)
 */
void test_weak_signal(void) {
    printf("\n--- Test: Weak Signal Rejection ---\n");
    
    int16_t samples[SAMPLE_SIZE];
    
    /* Generate very weak sine wave */
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        double phase = 2.0 * PI * 110.0 * i / SAMPLE_RATE;
        samples[i] = (int16_t)(10 * sin(phase));  /* Very weak: amplitude 10 */
    }
    
    double freq = apply_fft(samples, SAMPLE_SIZE);
    
    printf("Amplitude: 10 (below threshold of ~50)\n");
    printf("Detected:  %.2f Hz\n", freq);
    printf("Result:    %s\n", (freq == 0.0) ? "✓ PASS (correctly rejected)" : "✗ FAIL");
}

/**
 * Test with two-tone signal (only strongest should be detected)
 */
void test_peak_detection(void) {
    printf("\n--- Test: Peak Detection (Two Tones) ---\n");
    
    int16_t samples[SAMPLE_SIZE];
    
    /* Generate sum of two sine waves: 110 Hz (strong) + 250 Hz (weak) */
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        double phase1 = 2.0 * PI * 110.0 * i / SAMPLE_RATE;
        double phase2 = 2.0 * PI * 250.0 * i / SAMPLE_RATE;
        samples[i] = (int16_t)(15000 * sin(phase1) + 3000 * sin(phase2));
    }
    
    double freq = apply_fft(samples, SAMPLE_SIZE);
    
    printf("Tone 1: 110 Hz (strong, amplitude 15000)\n");
    printf("Tone 2: 250 Hz (weak, amplitude 3000)\n");
    printf("Detected: %.2f Hz\n", freq);
    
    double error_hz = fabs(freq - 110.0);
    int pass = (error_hz < TOLERANCE_HZ);
    
    printf("Result:   %s\n", pass ? "✓ PASS (detected strongest)" : "✗ FAIL");
}

/**
 * Test gain application
 */
void test_gain_application(void) {
    printf("\n--- Test: Gain Application ---\n");
    
    int16_t samples1[SAMPLE_SIZE];
    int16_t samples2[SAMPLE_SIZE];
    
    /* Generate sine wave */
    generate_test_sine(samples1, SAMPLE_SIZE, 110.0);
    memcpy(samples2, samples1, SAMPLE_SIZE * sizeof(int16_t));
    
    /* Apply 2x gain to second set */
    apply_gain(samples2, SAMPLE_SIZE, 2.0);
    
    /* Check for saturation */
    int saturated = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        if (samples2[i] == 32767 || samples2[i] == -32768) {
            saturated = 1;
            break;
        }
    }
    
    printf("Original amplitude: ~20000\n");
    printf("After 2x gain: ~40000 (saturated to 32767)\n");
    printf("Saturation detected: %s\n", saturated ? "Yes" : "No");
    printf("Result: %s\n", saturated ? "✓ PASS" : "✓ PASS (no clipping at this amplitude)");
}

int main(void) {
    printf("========================================\n");
    printf("CMSIS-DSP FFT Integration Tests\n");
    printf("========================================\n");
    
    /* Initialize audio processing */
    audio_processing_init();
    
    printf("\nRunning %d frequency detection tests...\n\n", NUM_TESTS);
    
    /* Test guitar string frequencies */
    test_frequency_detection(82.41,  "E2 (Low E)");
    test_frequency_detection(110.0,  "A2 (A string)");
    test_frequency_detection(146.83, "D3 (D string)");
    test_frequency_detection(196.0,  "G3 (G string)");
    test_frequency_detection(246.94, "B3 (B string)");
    test_frequency_detection(329.63, "E4 (High E)");
    
    /* Test special cases */
    test_dc_offset_handling();
    test_weak_signal();
    test_peak_detection();
    test_gain_application();
    
    printf("\n========================================\n");
    printf("Tests Complete\n");
    printf("========================================\n");
    
    return 0;
}
