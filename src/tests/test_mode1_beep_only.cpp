/**
 * MODE 1 TEST: Proximity Beeping Only - REALISTIC SIMULATION
 * Simulates a user tuning their guitar from out-of-tune to in-tune
 * Beeps start FAST (far away) and gradually SLOW DOWN as they approach target
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
 * Audio System - Sine Wave via MQS (Pin 10)
 * ========================================================================== */

AudioSynthWaveformSine   sine1;
AudioOutputMQS           mqs_output;
AudioConnection          patchCord1(sine1, 0, mqs_output, 0);
AudioConnection          patchCord2(sine1, 0, mqs_output, 1);

/* ============================================================================
 * CFugue Note Parser - Clean Implementation
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
 * Audio Output - Clean Beep Implementation using Sine Waves
 * ========================================================================== */

void playBeep(float frequency, uint32_t duration_ms) {
    sine1.frequency(frequency);
    sine1.amplitude(1.0);  // Maximum volume
    delay(duration_ms);
    sine1.amplitude(0.0);
}

void playInTuneJingle() {
    Serial.println("  ** IN TUNE! Playing success jingle **");
    playBeep(523.25, 150); delay(50);  // C5
    playBeep(659.25, 150); delay(50);  // E5
    playBeep(783.99, 300);             // G5
}

void continuousProximityFeedback(float target_freq, float start_freq, float end_freq, int steps) {
    Serial.println();
    Serial.print("Simulating tuning from ");
    Serial.print(start_freq, 2);
    Serial.print(" Hz to ");
    Serial.print(end_freq, 2);
    Serial.println(" Hz");
    Serial.print("Target: ");
    Serial.print(target_freq, 2);
    Serial.println(" Hz");
    Serial.println();
    
    float freq_step = (end_freq - start_freq) / steps;
    
    for (int i = 0; i <= steps; i++) {
        float current_freq = start_freq + (freq_step * i);
        float cents = 1200.0f * log2f(current_freq / target_freq);
        float abs_cents = fabs(cents);
        
        // Check if in tune
        if (abs_cents < 5.0f) {
            Serial.println();
            Serial.print("Step ");
            Serial.print(i + 1);
            Serial.print("/");
            Serial.print(steps + 1);
            Serial.print(" - ");
            Serial.print(current_freq, 2);
            Serial.print(" Hz (");
            Serial.print(cents, 1);
            Serial.println(" cents) -> IN TUNE!");
            playInTuneJingle();
            return;  // Stop once in tune
        }
        
        // Calculate beep parameters:
        // Far from target (out of tune): HIGH pitch (1200Hz) + FAST beeps
        // Close to target (in tune): LOW pitch (400Hz) + SLOW beeps
        float beep_freq = map(constrain(abs_cents, 5, 100), 5, 100, 400, 1200);
        int gap_ms = (int)map(constrain(abs_cents, 5, 100), 5, 100, 900, 100);
        
        // Print status
        Serial.print("Step ");
        Serial.print(i + 1);
        Serial.print("/");
        Serial.print(steps + 1);
        Serial.print(" - ");
        Serial.print(current_freq, 2);
        Serial.print(" Hz (");
        Serial.print(cents, 1);
        Serial.print(" cents) -> freq=");
        Serial.print((int)beep_freq);
        Serial.print("Hz, gap=");
        Serial.print(gap_ms);
        Serial.println("ms");
        
        // Play ONE beep at current gap speed
        playBeep(beep_freq, 80);
        delay(gap_ms);
    }
}

/* ============================================================================
 * Test Sequences
 * ========================================================================== */

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // Initialize Audio Library
    AudioMemory(20);
    sine1.amplitude(0.0);
    
   /*
    Serial.println("========================================");
    Serial.println("MODE 1 TEST: Continuous Beeping Feedback");
    Serial.println("Using SINE WAVE via MQS on Pin 10");
    Serial.println("========================================");
    Serial.println("NOTE: Connect LM386 input to PIN 10!");
    Serial.println();
    Serial.println("You will hear:");
    Serial.println("  1. Fast beeps when far from target");
    Serial.println("  2. Beeps gradually slow down");
    Serial.println("  3. Success jingle when in tune");
    Serial.println("========================================");
    
    // Startup test beep
    Serial.println("Playing startup test beep...");
    playBeep(1000.0, 300);
    delay(500);
    
    // Scenario 1: Tuning A4 (440Hz) from 80 cents flat to in-tune
    Serial.println("\n=== SCENARIO 1: A4 (440Hz) - Tuning UP from flat ===");
    float target_A4 = parseNote("A4");
    float start_flat = target_A4 * 0.954f;   // 80 cents flat
    continuousProximityFeedback(target_A4, start_flat, target_A4, 15);
    
    delay(3000);
    
    // Scenario 2: Tuning E5 (659Hz) from 60 cents sharp to in-tune
    Serial.println("\n=== SCENARIO 2: E5 (659Hz) - Tuning DOWN from sharp ===");
    float target_E5 = parseNote("E5");
    float start_sharp = target_E5 * 1.035f;  // 60 cents sharp
    continuousProximityFeedback(target_E5, start_sharp, target_E5, 15);
    
    delay(3000);
    */
    // Scenario 3: Tuning C5 (523Hz) from very flat to in-tune (longer sequence)
    Serial.println("\n=== SCENARIO 1 C5 (523Hz) - Long tuning sequence ===");
    float target_C5 = parseNote("C5");
    float start_very_flat = target_C5 * 0.94f;  // ~100 cents flat
    continuousProximityFeedback(target_C5, start_very_flat, target_C5, 20);
    
    Serial.println("\n========================================");
    Serial.println("MODE 1 TEST COMPLETE");
    Serial.println("All strings successfully tuned!");
    Serial.println("========================================");
    
    // Final celebration
    delay(1000);
    playInTuneJingle();
}

void loop() {
    delay(1000);
}


/**
## **What This Test Does:**

### **Scenario 1: A4 String (80 cents flat → in tune)**
```
Step 1:  419 Hz (80 cents flat)  -> beep [100ms gap] RAPID
Step 2:  421 Hz (75 cents flat)  -> beep [130ms gap] 
Step 3:  423 Hz (70 cents flat)  -> beep [160ms gap]
...
Step 14: 438 Hz (8 cents flat)   -> beep [820ms gap] SLOW
Step 15: 440 Hz (2 cents flat)   -> ** IN TUNE! C-E-G jingle **
```

### **Scenario 2: E2 String (60 cents sharp → in tune)**
```
Starts high, beeps slow down as frequency DROPS to target
```

### **Scenario 3: G3 String (100 cents flat → in tune)**
```
Longer sequence (20 steps) showing full tuning journey
 */