/**
 * MODE 2 TEST: Reference Tones Only - CLEAN SINE WAVES
 * Plays detected frequency, then target frequency as MUSICAL NOTES
 * Uses Teensy Audio Library with MQS output for clean sine waves
 * 
 * Hardware: Teensy 4.1 + LM386 module + Speaker
 * Wiring: Pin 10 (MQS) -> LM386 IN, VIN -> LM386 VDD, GND -> LM386 GND
 *         (NOT pin 9 - MQS output is on pin 10!)
 */

#include <Arduino.h>
#include <AudioStream.h>
#include <synth_sine.h>
#include <output_mqs.h>

#define A4_FREQUENCY 440.0f

/* ============================================================================
 * Audio System - Sine Wave Synthesis via MQS
 * MQS outputs clean audio on Pin 10 (left) and Pin 12 (right)
 * ========================================================================== */

AudioSynthWaveformSine   sine1;
AudioOutputMQS           mqs_output;
AudioConnection          patchCord1(sine1, 0, mqs_output, 0);  // Left channel (pin 10)
AudioConnection          patchCord2(sine1, 0, mqs_output, 1);  // Right channel (pin 12)

void playMusicalTone(float frequency, uint32_t duration_ms) {
    Serial.print("  Playing musical tone: ");
    Serial.print(frequency, 2);
    Serial.println(" Hz");
    
    sine1.frequency(frequency);
    sine1.amplitude(0.8);  // 80% volume
    delay(duration_ms);
    sine1.amplitude(0.0);  // Stop
    delay(50);
}

void playInTuneChime() {
    Serial.println("  ** IN TUNE! Playing success chime **");
    playMusicalTone(523.25, 150);  // C5
    playMusicalTone(659.25, 150);  // E5
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
 * Test Sequence - Simple Comparison
 * ========================================================================== */

void playComparison(const char* detected_note, const char* target_note) {
    float detected_freq = parseNote(detected_note);
    float target_freq = parseNote(target_note);
    
    Serial.println();
    Serial.println("===================");
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
    Serial.println("===================");
    
    // Check if in tune (same note)
    if (fabs(detected_freq - target_freq) < 5.0f) {
        // Still play both notes, then the jingle
        Serial.println("Playing detected note...");
        playMusicalTone(detected_freq, 1000);
        delay(300);
        Serial.println("Playing target note...");
        playMusicalTone(target_freq, 1000);
        delay(300);
        Serial.println("STATUS: IN TUNE!");
        playInTuneChime();
    } else {
        Serial.println("Playing detected note...");
        playMusicalTone(detected_freq, 1000);
        delay(300);
        Serial.println("Playing target note...");
        playMusicalTone(target_freq, 1000);
    }
    
    delay(1500);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // Initialize Audio Library
    AudioMemory(20);
    sine1.amplitude(0.0);
    
    Serial.println("========================================");
    Serial.println("MODE 2 TEST: Musical Reference Tones");
    Serial.println("Using SINE WAVE via MQS on Pin 10");
    Serial.println("========================================");
    Serial.println("NOTE: Connect LM386 input to PIN 10!");
    
    // Startup test tone - should sound clean!
   // Serial.println("Playing startup test tone (A4 = 440Hz)...");
    //playMusicalTone(440.0, 500);
    // delay(500);
    
    // Test sequence: All targeting A4 (440Hz), approaching from far to in-tune
    
    Serial.println("\n=== Test 1: Detected D5, Target A4 ===");
    playComparison("D5", "A4");  // D5 (587Hz) -> A4 (440Hz)
    
    Serial.println("\n=== Test 2: Detected B4, Target A4 ===");
    playComparison("B4", "A4");  // B4 (494Hz) -> A4 (440Hz)
    
    
    Serial.println("\n=== Test 3: Detected A4, Target A4 (IN TUNE!) ===");
    playComparison("A4", "A4");  // A4 (440Hz) = A4 (440Hz) -> jingle!
    
    Serial.println("\n========================================");
    Serial.println("MODE 2 TEST COMPLETE");
    Serial.println("All strings tested!");
    Serial.println("========================================");
}

void loop() {
    delay(1000);
}