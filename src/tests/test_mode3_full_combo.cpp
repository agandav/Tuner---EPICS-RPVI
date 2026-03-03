/**
 * MODE 3 TEST: Full Feedback Combo - BEEPS + MUSICAL TONES
 * Combines proximity beeping (fast->slow) + musical tone comparison
 * Uses Teensy Audio Library with MQS output for clean sine waves
 * 
 * Hardware: Teensy 4.1 + LM386 module + Speaker
 * Wiring: Pin 10 (MQS) -> LM386 IN, VIN -> LM386 VDD, GND -> LM386 GND
 */

#include <Arduino.h>
#include <AudioStream.h>
#include <synth_sine.h>
#include <output_mqs.h>

#define A4_FREQUENCY 440.0f

/* ============================================================================
 * Audio System - Sine Wave Synthesis via MQS
 * ========================================================================== */

AudioSynthWaveformSine   sine1;
AudioOutputMQS           mqs_output;
AudioConnection          patchCord1(sine1, 0, mqs_output, 0);
AudioConnection          patchCord2(sine1, 0, mqs_output, 1);

void playTone(float frequency, uint32_t duration_ms, float amplitude) {
    sine1.frequency(frequency);
    sine1.amplitude(amplitude);
    delay(duration_ms);
    sine1.amplitude(0.0);
}

void playBeep(float frequency, uint32_t duration_ms) {
    // Sharp beeps for proximity feedback (maximum volume)
    playTone(frequency, duration_ms, 1.0);
}

void playMusicalTone(float frequency, uint32_t duration_ms) {
    // Smooth musical notes (80% volume)
    playTone(frequency, duration_ms, 0.8);
}

void playInTuneChime() {
    Serial.println("  ** IN TUNE! Playing success chime **");
    playMusicalTone(523.25, 150);  // C5
    delay(50);
    playMusicalTone(659.25, 150);  // E5
    delay(50);
    playMusicalTone(783.99, 300);  // G5
}

/* ============================================================================
 * CFugue Note Parser
 * ========================================================================== */

int getNoteSemitoneOffset(char note_letter) {
    switch (toupper(note_letter)) {
        case 'C': return 0;
        case 'D': return 2;
        case 'E': return 4;
        case 'F': return 5;
        case 'G': return 7;
        case 'A': return 9;
        case 'B': return 11;
        default: return -1;
    }
}

float parseNote(const char* note) {
    if (!note || !note[0]) return 0.0f;
    
    int idx = 0;
    char letter = toupper(note[idx++]);
    int semitone = getNoteSemitoneOffset(letter);
    if (semitone < 0) return 0.0f;
    
    bool sharp = (note[idx] == '#');
    if (sharp) { semitone++; idx++; }
    
    if (!isdigit(note[idx])) return 0.0f;
    int octave = note[idx] - '0';
    
    int midi = (octave + 1) * 12 + semitone;
    return A4_FREQUENCY * powf(2.0f, (midi - 69) / 12.0f);
}

/* ============================================================================
 * Full Combo Feedback: Tone Comparison + Guiding Beeps to Target
 * ========================================================================== */

void guidingBeepsToTarget(float start_freq, float target_freq, int steps) {
    // Simulate tuning from start_freq toward target_freq
    // Beeps start FAST (far away) and gradually SLOW DOWN as we approach
    
    float freq_step = (target_freq - start_freq) / steps;
    
    for (int i = 0; i <= steps; i++) {
        float current_freq = start_freq + (freq_step * i);
        float cents_off = 1200.0f * log2f(current_freq / target_freq);
        float abs_cents = fabs(cents_off);
        
        // Check if in tune
        if (abs_cents < 5.0f) {
            Serial.println("  >>> IN TUNE! <<<");
            playInTuneChime();
            return;
        }
        
        // Beep parameters: pitch AND speed change together
        // Far from target (out of tune): HIGH pitch (1200Hz) + FAST beeps
        // Close to target (in tune): LOW pitch (400Hz) + SLOW beeps
        // D5 to A4 is ~500 cents, so use range 5-500
        float beep_freq = map(constrain(abs_cents, 5, 500), 5, 500, 400, 1200);
        int gap_ms = (int)map(constrain(abs_cents, 5, 500), 5, 500, 900, 80);
        
        Serial.print("  Step ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(abs_cents, 0);
        Serial.print(" cents, freq=");
        Serial.print((int)beep_freq);
        Serial.print("Hz, gap=");
        Serial.print(gap_ms);
        Serial.println("ms");
        
        // Play one beep
        playBeep(beep_freq, 80);
        delay(gap_ms);
    }
}

void fullComboTest(const char* detected_note, const char* target_note) {
    float detected_freq = parseNote(detected_note);
    float target_freq = parseNote(target_note);
    
    Serial.println();
    Serial.println("===========================");
    Serial.print("Detected: ");
    Serial.print(detected_note);
    Serial.print(" (");
    Serial.print(detected_freq, 1);
    Serial.println(" Hz)");
    Serial.print("Target: ");
    Serial.print(target_note);
    Serial.print(" (");
    Serial.print(target_freq, 1);
    Serial.println(" Hz)");
    Serial.println("===========================");
    
    // STEP 1: Play the detected note
    Serial.println("STEP 1: Playing detected note...");
    playMusicalTone(detected_freq, 1000);
    delay(300);
    
    // STEP 2: Play the target note
    Serial.println("STEP 2: Playing target note...");
    playMusicalTone(target_freq, 1000);
    delay(500);
    
    // STEP 3: Guiding beeps - simulate tuning from detected to target
    Serial.println("STEP 3: Guiding beeps (fast -> slow -> jingle)...");
    guidingBeepsToTarget(detected_freq, target_freq, 20);
    
    delay(1000);
}

/* ============================================================================
 * Test Sequences - All 6 Guitar Strings
 * ========================================================================== */

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // Initialize Audio Library
    AudioMemory(20);
    sine1.amplitude(0.0);
    
    Serial.println("========================================");
    Serial.println("MODE 3 TEST: Full Feedback Combo");
    Serial.println("MQS output on Pin 10");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Sequence:");
    Serial.println("  1. Play detected note (D5)");
    Serial.println("  2. Play target note (A4)");
    Serial.println("  3. Beeps: FAST -> gradually SLOW -> jingle");
    Serial.println("========================================");
    
    delay(2000);
    
    // Single test: detected D5, target A4
    // More steps (20) for smoother beep progression
    Serial.println("\n>>> Detected: D5 | Target: A4 <<<");
    fullComboTest("D5", "A4");
    
    Serial.println("\n========================================");
    Serial.println("MODE 3 TEST COMPLETE!");
    Serial.println("========================================");
}

void loop() {
    delay(1000);
}

/**
 * MODE 3: Full Combo Test
 * 
 * Combines Mode 1 (beeps) and Mode 2 (tones):
 * 
 * 1. Play detected note (e.g., D5 at 587Hz)
 * 2. Play target note (A4 at 440Hz)
 * 3. Guiding beeps that simulate tuning:
 *    - Fast beeps when far from target
 *    - Beeps slow down as you approach
 *    - Success jingle when you reach the target
 * 
 * Test Sequence:
 *   Test 1: D5 -> A4 (far, ~500 cents) -> fast beeps
 *   Test 2: B4 -> A4 (close, ~200 cents) -> medium beeps
 *   Test 3: A4 -> A4 (in tune) -> immediate jingle
 */