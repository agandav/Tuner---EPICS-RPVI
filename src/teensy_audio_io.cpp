#include <Arduino.h>
#include <AudioStream.h>
#include <synth_sine.h>
#include <output_mqs.h>
#include "config.h"
#include "teensy_audio_io.h"

extern "C" {
    #include "audio_processing.h"
}

/* ============================================================================
 * AUDIO OBJECTS - MQS OUTPUT
 * ========================================================================== */

AudioSynthWaveformSine   sine1;         // Main sine wave for reference tones
AudioSynthWaveformSine   beep_sine;     // Separate sine wave for feedback beeps
AudioOutputMQS           mqs_output;    // MQS output: pin 10 (R) and pin 12 (L)

/* Route both generators into both MQS channels */
AudioConnection          patchCord1(sine1,     0, mqs_output, 0);  // tone -> right (pin 10)
AudioConnection          patchCord2(sine1,     0, mqs_output, 1);  // tone -> left  (pin 12)
AudioConnection          patchCord3(beep_sine, 0, mqs_output, 0);  // beep -> right (pin 10)
AudioConnection          patchCord4(beep_sine, 0, mqs_output, 1);  // beep -> left  (pin 12)

/* ============================================================================
 * TONE PLAYBACK STATE
 * ========================================================================== */

static bool     tone_playing     = false;
static uint32_t tone_start_time  = 0;
static uint32_t tone_duration_ms = 0;
static float    current_freq     = 0.0f;

/* ============================================================================
 * INITIALIZATION
 * ========================================================================== */

void init_audio_system(void) {
    Serial.println("[AUDIO] Initializing...");

    /* MQS audio library setup */
    AudioMemory(20);
    sine1.amplitude(0.0f);
    beep_sine.amplitude(0.0f);
    sine1.frequency(440.0f);
    beep_sine.frequency(800.0f);

    /* ADC setup for microphone on pin 39 (A17) */
    analogReadResolution(12);    // 12-bit: values 0 to 4095
    analogReadAveraging(4);      // Average 4 reads per sample to reduce noise

    Serial.println("[AUDIO] MQS output ready on pin 10");
    Serial.println("[AUDIO] Microphone ADC ready on pin 39 (A17)");
    Serial.print("[AUDIO] CPU usage: ");
    Serial.print(AudioProcessorUsage());
    Serial.println("%");
}

/* ============================================================================
 * TONE PLAYBACK
 * Non-blocking - call update_tone_playback() every loop() to stop on time
 * ========================================================================== */

void play_tone(float frequency, uint32_t duration_ms) {
    if (frequency <= 0.0f || duration_ms == 0) return;

    Serial.print("[TONE] ");
    Serial.print(frequency, 2);
    Serial.print(" Hz for ");
    Serial.print(duration_ms);
    Serial.println(" ms");

    current_freq     = frequency;
    tone_playing     = true;
    tone_start_time  = millis();
    tone_duration_ms = duration_ms;

    sine1.frequency(frequency);
    sine1.amplitude(TONE_AMPLITUDE_DEFAULT);
}

/* ============================================================================
 * BEEP PLAYBACK
 * Blocking - returns after beep is complete
 * Used for short feedback beeps where blocking is acceptable
 * ========================================================================== */

void play_beep(float frequency, uint32_t duration_ms) {
    if (frequency <= 0.0f || duration_ms == 0) return;

    Serial.print("[BEEP] ");
    Serial.print(frequency, 0);
    Serial.print(" Hz for ");
    Serial.print(duration_ms);
    Serial.println(" ms");

    beep_sine.frequency(frequency);
    beep_sine.amplitude(BEEP_AMPLITUDE_DEFAULT);
    delay(duration_ms);
    beep_sine.amplitude(0.0f);
}

void play_ready_beep(void) {
    Serial.println("[AUDIO] Ready beep");
    play_beep(1000.0f, 200);
}

void stop_all_audio(void) {
    sine1.amplitude(0.0f);
    beep_sine.amplitude(0.0f);
    tone_playing = false;
    Serial.println("[AUDIO] Stopped");
}

/**
 * update_tone_playback()
 * Call this every loop() iteration.
 * Stops the non-blocking tone once its duration has elapsed.
 * Beeps are always blocking and do not need this function.
 */
void update_tone_playback(void) {
    if (!tone_playing) return;

    if (millis() - tone_start_time >= tone_duration_ms) {
        sine1.amplitude(0.0f);
        tone_playing = false;
        Serial.println("[AUDIO] Tone complete");
    }
}

/* ============================================================================
 * MICROPHONE INPUT AND FREQUENCY DETECTION
 *
 * WHAT THIS DOES (the original version always returned 0.0 - it was broken):
 *
 *   1. Samples the electret microphone on pin 39 (A17) at approximately 10 kHz
 *      by using delayMicroseconds(95) between reads (~5us per analogRead = ~100us total)
 *   2. Converts 12-bit unsigned ADC values (0-4095) to signed int16 centered at zero
 *      by subtracting ADC_CENTER_VALUE (2048)
 *   3. Passes the sample buffer to apply_fft() in audio_processing.c
 *   4. Returns detected frequency in Hz, or 0.0 if signal is too weak
 *
 * CALIBRATION NOTE:
 *   If detected frequencies are consistently off by a fixed ratio, the actual
 *   sample rate differs from 10 kHz. Adjust the delayMicroseconds value below
 *   to compensate. Lower value = higher sample rate, higher value = lower rate.
 * ========================================================================== */
double read_frequency_from_microphone(int16_t* external_buffer, int buffer_size) {
    (void)external_buffer;
    (void)buffer_size;

    int16_t samples[SAMPLE_SIZE];

    // Sample ADC at ~10 kHz
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        samples[i] = (int16_t)analogRead(MICROPHONE_INPUT_PIN);
        delayMicroseconds(75);
    }

    // Calculate actual mean of this buffer and subtract it
    // This replaces the hardcoded ADC_CENTER_VALUE = 2048 subtraction
    // which was wrong because the mic bias sits at ~2450, not 2048
    long sum = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) sum += samples[i];
    int16_t actual_center = (int16_t)(sum / SAMPLE_SIZE);
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        samples[i] = (int16_t)(samples[i] - actual_center);
    }

    // Pass DC-removed samples to FFT pipeline
    return apply_fft(samples, SAMPLE_SIZE);
}

/* ============================================================================
 * DIAGNOSTICS
 * ========================================================================== */

void print_audio_status(void) {
    Serial.println("\n=== Audio System Status ===");
    Serial.println("Output: MQS on pins 10/12");
    Serial.print("CPU usage: ");
    Serial.print(AudioProcessorUsage());
    Serial.println("%");
    Serial.print("Memory usage: ");
    Serial.print(AudioMemoryUsage());
    Serial.println(" blocks");
    Serial.print("Tone playing: ");
    Serial.println(tone_playing ? "Yes" : "No");
    if (tone_playing) {
        Serial.print("  Frequency: ");
        Serial.print(current_freq, 2);
        Serial.println(" Hz");
        Serial.print("  Elapsed: ");
        Serial.print(millis() - tone_start_time);
        Serial.print(" / ");
        Serial.print(tone_duration_ms);
        Serial.println(" ms");
    }
    Serial.println("===========================\n");
}

/**
 * test_audio_playback()
 * Plays all 6 guitar string reference tones in sequence.
 * Run this at startup to verify speaker and amp are working
 * before entering the main tuning loop.
 */
void test_audio_playback(void) {
    Serial.println("[TEST] Playing guitar string reference tones...");

    float       freqs[] = {82.41f, 110.00f, 146.83f, 196.00f, 246.94f, 329.63f};
    const char* names[] = {"E2",   "A2",    "D3",    "G3",    "B3",    "E4"   };

    for (int i = 0; i < 6; i++) {
        Serial.print("[TEST] String ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(names[i]);
        Serial.print(" (");
        Serial.print(freqs[i], 2);
        Serial.println(" Hz)");

        play_tone(freqs[i], 800);
        delay(800);
        stop_all_audio();
        delay(200);
    }

    Serial.println("[TEST] Done. If no sound check LM386 IN is on pin 10.");
}