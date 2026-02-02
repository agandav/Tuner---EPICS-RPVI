/**
 * teensy_audio_io.cpp - Teensy Audio Library Implementation
 * 
 * Handles all audio input/output using the Teensy Audio Library
 * - Sine wave synthesis for tone playback
 * - Microphone input for frequency detection
 * - I2S audio output to speaker/amplifier
 * 
 * Hardware: Teensy 4.1 with PAM8302A amplifier (from schematic)
 */

#include <Arduino.h>
// Include only the specific Audio library components we need (not Audio.h which includes SD card headers)
#include <AudioStream.h>
#include <synth_sine.h>
#include <input_i2s.h>
#include <output_i2s.h>
#include "teensy_audio_io.h"

/* ============================================================================
 * AUDIO OBJECTS - Teensy Audio Library
 * ========================================================================== */

// Synthesis objects (for playing tones)
AudioSynthWaveformSine   sine1;          // Main sine wave generator
AudioSynthWaveformSine   beep_sine;      // Separate generator for beeps

// Input object (for microphone)
AudioInputI2S            i2s_input;      // I2S audio input (microphone)

// Output object (for speaker)
AudioOutputI2S           i2s_output;     // I2S audio output (speaker/amp)

// Connections (audio routing)
AudioConnection          patchCord1(sine1, 0, i2s_output, 0);       // Tone -> Left
AudioConnection          patchCord2(sine1, 0, i2s_output, 1);       // Tone -> Right
AudioConnection          patchCord3(beep_sine, 0, i2s_output, 0);   // Beep -> Left (mixed)
AudioConnection          patchCord4(beep_sine, 0, i2s_output, 1);   // Beep -> Right (mixed)

// Optional: If using SGTL5000 audio shield
// AudioControlSGTL5000     audioShield;

/* ============================================================================
 * TONE PLAYBACK STATE
 * ========================================================================== */

static bool tone_playing = false;
static uint32_t tone_start_time = 0;
static uint32_t tone_duration = 0;

/* ============================================================================
 * INITIALIZATION
 * ========================================================================== */

/**
 * Initialize the Teensy audio system
 * Call this once in setup()
 */
void init_audio_system(void) {
    Serial.println("[AUDIO] Initializing Teensy Audio Library...");
    
    // Allocate audio memory (required!)
    // Each block is 128 samples, allocate enough for audio processing
    AudioMemory(20);
    
    // If using SGTL5000 audio shield (uncomment if you have it):
    // audioShield.enable();
    // audioShield.volume(0.7);  // 70% volume
    // audioShield.inputSelect(AUDIO_INPUT_LINEIN);  // or AUDIO_INPUT_MIC
    
    // Start with no output
    sine1.amplitude(0.0);
    beep_sine.amplitude(0.0);
    
    // Set default waveform type (sine wave)
    sine1.frequency(440.0);  // Default to A4
    beep_sine.frequency(800.0);  // Default beep frequency
    
    Serial.println("[AUDIO] Audio system initialized");
    Serial.print("[AUDIO] CPU Usage: ");
    Serial.print(AudioProcessorUsage());
    Serial.println("%");
    Serial.print("[AUDIO] Memory Usage: ");
    Serial.print(AudioMemoryUsage());
    Serial.println(" blocks");
}

/* ============================================================================
 * TONE PLAYBACK FUNCTIONS
 * ========================================================================== */

/**
 * Play a tone at a specific frequency
 * This is the main function used for playback
 * 
 * @param frequency - Frequency in Hz (e.g., 82.41, 440.0, 329.63)
 * @param duration_ms - How long to play in milliseconds
 * 
 * Example:
 *   play_tone(329.63, 2000);  // Play E4 for 2 seconds
 */
void play_tone(float frequency, uint32_t duration_ms) {
    Serial.print("[AUDIO] Playing ");
    Serial.print(frequency, 2);
    Serial.print(" Hz for ");
    Serial.print(duration_ms);
    Serial.println(" ms");
    
    // Set frequency
    sine1.frequency(frequency);
    
    // Set amplitude (volume) - 0.0 to 1.0
    sine1.amplitude(0.7);  // 70% volume
    
    // Store timing info for non-blocking playback
    tone_playing = true;
    tone_start_time = millis();
    tone_duration = duration_ms;
    
    // Blocking version (simpler but locks up processor):
    // delay(duration_ms);
    // sine1.amplitude(0.0);
    
    // For blocking playback, uncomment above and comment out state tracking
}

/**
 * Play a short beep (for feedback)
 * 
 * @param frequency - Beep frequency in Hz (typically 400-1200 Hz)
 * @param duration_ms - Beep duration in milliseconds (typically 50-200 ms)
 */
void play_beep(float frequency, uint32_t duration_ms) {
    beep_sine.frequency(frequency);
    beep_sine.amplitude(0.5);  // 50% volume (quieter than main tone)
    delay(duration_ms);
    beep_sine.amplitude(0.0);
}

/**
 * Play a "ready" beep to signal user can play their string
 */
void play_ready_beep(void) {
    Serial.println("[AUDIO] BEEP! (Ready)");
    play_beep(1000.0, 200);  // 1000 Hz for 200ms
}

/**
 * Stop all audio output immediately
 */
void stop_all_audio(void) {
    sine1.amplitude(0.0);
    beep_sine.amplitude(0.0);
    tone_playing = false;
    Serial.println("[AUDIO] Audio stopped");
}

/**
 * Update tone playback timing
 * Call this in your main loop() to handle non-blocking playback
 */
void update_tone_playback(void) {
    if (tone_playing) {
        uint32_t elapsed = millis() - tone_start_time;
        
        if (elapsed >= tone_duration) {
            // Tone duration complete, stop playing
            sine1.amplitude(0.0);
            tone_playing = false;
            
            Serial.println("[AUDIO] Tone complete");
        }
    }
}

/* ============================================================================
 * AUDIO AMPLIFIER CONTROL (PAM8302A from schematic)
 * Note: Amplifier control functions are in hardware_interface.c
 * ========================================================================== */

/* ============================================================================
 * MICROPHONE INPUT & FREQUENCY DETECTION
 * ========================================================================== */

/**
 * Read frequency from microphone using FFT
 * 
 * @param buffer - Audio sample buffer (optional, can be NULL)
 * @param buffer_size - Size of buffer (optional, can be 0)
 * @return Detected frequency in Hz, or 0.0 if no signal
 * 
 * This function interfaces with the audio_processing module for FFT analysis
 */
double read_frequency_from_microphone(int16_t* buffer, int buffer_size) {
    /* Note: Direct microphone reading requires custom implementation.
     * The Teensy Audio Library uses a callback/update system via AudioStream.
     * Audio data should be accessed through AudioAnalyzeFFT1024 or similar analysis objects.
     * This function is a placeholder for integration with audio_processing.c
     */
    (void)buffer;
    (void)buffer_size;
    
    return 0.0;  // Placeholder - actual frequency detection via AudioAnalyzeFFT1024
}

/* ============================================================================
 * DIAGNOSTIC / DEBUG FUNCTIONS
 * ========================================================================== */

/**
 * Print audio system status
 * Useful for debugging
 */
void print_audio_status(void) {
    Serial.println("\n=== Audio System Status ===");
    Serial.print("CPU Usage: ");
    Serial.print(AudioProcessorUsage());
    Serial.println("%");
    
    Serial.print("CPU Max: ");
    Serial.print(AudioProcessorUsageMax());
    Serial.println("%");
    
    Serial.print("Memory Usage: ");
    Serial.print(AudioMemoryUsage());
    Serial.println(" blocks");
    
    Serial.print("Memory Max: ");
    Serial.print(AudioMemoryUsageMax());
    Serial.println(" blocks");
    
    Serial.print("Tone Playing: ");
    Serial.println(tone_playing ? "Yes" : "No");
    
    Serial.println("===========================\n");
}

/**
 * Test audio system by playing a scale
 */
void test_audio_playback(void) {
    Serial.println("[TEST] Playing test scale...");
    
    // Play C major scale
    float notes[] = {261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88, 523.25};
    const char* names[] = {"C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5"};
    
    for (int i = 0; i < 8; i++) {
        Serial.print("[TEST] Playing ");
        Serial.print(names[i]);
        Serial.print(" (");
        Serial.print(notes[i], 2);
        Serial.println(" Hz)");
        
        play_tone(notes[i], 500);
        delay(500);  // Wait for tone to finish
        delay(100);  // Short pause between notes
    }
    
    Serial.println("[TEST] Test complete!");
}