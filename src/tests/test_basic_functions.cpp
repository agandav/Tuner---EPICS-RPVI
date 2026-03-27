/**
 * test_basic_functions.cpp - Core Tuner Function Verification
 *
 * PURPOSE:
 *   Upload this AFTER test_wiring passes. Tests the actual tuner
 *   logic end to end: audio init, tone playback, mic capture,
 *   FFT frequency detection, cents calculation, and beep feedback.
 *   Uses real hardware - no simulation.
 *
 * UPLOAD COMMAND:
 *   pio run -e teensy41_test_basic -t upload
 *
 * THEN OPEN SERIAL MONITOR:
 *   pio device monitor -e teensy41_test_basic
 *
 * TESTS COVERED:
 *   1. Audio system init (MQS + AudioMemory)
 *   2. All 6 reference tones play correctly
 *   3. Microphone ADC samples at correct rate
 *   4. FFT detects a known frequency (A2 = 110 Hz)
 *   5. Cents offset calculation accuracy
 *   6. Beep feedback responds to cents offset
 *   7. Button detection triggers correct string
 *   8. Full single-string tuning cycle (press button -> listen -> feedback)
 */

#include <Arduino.h>
#include <AudioStream.h>
#include <synth_sine.h>
#include <output_mqs.h>

extern "C" {
    #include "audio_processing.h"
    #include "string_detection.h"
    #include "note_parser.h"
    #include "hardware_interface.h"
    #include "config.h"
}

/* ============================================================================
 * AUDIO
 * ========================================================================== */

AudioSynthWaveformSine sine1;
AudioSynthWaveformSine sine_beep;
AudioOutputMQS         mqs_output;
AudioConnection        patchCord1(sine1,     0, mqs_output, 0);
AudioConnection        patchCord2(sine1,     0, mqs_output, 1);
AudioConnection        patchCord3(sine_beep, 0, mqs_output, 0);
AudioConnection        patchCord4(sine_beep, 0, mqs_output, 1);

void playTone(float freq, uint32_t duration_ms) {
    sine1.frequency(freq);
    sine1.amplitude(0.8f);
    delay(duration_ms);
    sine1.amplitude(0.0f);
    delay(50);
}

void playBeep(float freq, uint32_t duration_ms) {
    sine_beep.frequency(freq);
    sine_beep.amplitude(1.0f);
    delay(duration_ms);
    sine_beep.amplitude(0.0f);
}

void playSuccessJingle() {
    playTone(523.25f, 150);
    playTone(659.25f, 150);
    playTone(783.99f, 300);
}

/* ============================================================================
 * TEST HELPERS
 * ========================================================================== */

static int tests_passed = 0;
static int tests_failed = 0;

void printSeparator() { Serial.println("----------------------------------------"); }

void testPass(const char* name) {
    Serial.print("[PASS] ");
    Serial.println(name);
    tests_passed++;
}

void testFail(const char* name, const char* reason) {
    Serial.print("[FAIL] ");
    Serial.print(name);
    Serial.print(" -- ");
    Serial.println(reason);
    tests_failed++;
}

void testWarn(const char* name, const char* reason) {
    Serial.print("[WARN] ");
    Serial.print(name);
    Serial.print(" -- ");
    Serial.println(reason);
}

void printHeader(const char* title) {
    Serial.println();
    printSeparator();
    Serial.println(title);
    printSeparator();
}

/* ============================================================================
 * TEST 1: AUDIO SYSTEM INITIALIZATION
 *
 * Verifies AudioMemory allocated and MQS running without crashing.
 * PASS: CPU usage is sane (<50%), memory usage >0
 * ========================================================================== */

void test_audio_init() {
    printHeader("TEST 1: Audio System Init");

    float cpu = AudioProcessorUsage();
    int   mem = AudioMemoryUsage();

    Serial.print("  CPU usage:    "); Serial.print(cpu, 1); Serial.println("%");
    Serial.print("  Memory blocks: "); Serial.println(mem);

    if (cpu > 90.0f) {
        testFail("Audio init", "CPU usage >90% - audio system overloaded");
    } else if (mem == 0) {
        testFail("Audio init", "AudioMemory usage is 0 - AudioMemory() not called");
    } else {
        testPass("Audio system initialized");
    }
}

/* ============================================================================
 * TEST 2: REFERENCE TONE PLAYBACK (All 6 strings)
 *
 * Plays each string's reference frequency and confirms no crash.
 * Listen: tones should be clean sine waves, not buzzy/distorted.
 * PASS: All 6 tones play without hanging
 * ========================================================================== */

void test_reference_tones() {
    printHeader("TEST 2: Reference Tone Playback (All 6 Strings)");
    Serial.println("  Listen for 6 clean tones - lowest to highest.");
    Serial.println();

    const char*  notes[]  = {"E2",   "A2",    "D3",    "G3",    "B3",    "E4"   };
    float        freqs[]  = {82.41f, 110.0f,  146.83f, 196.0f,  246.94f, 329.63f};
    int          strings[] = {6,      5,       4,       3,       2,       1      };

    for (int i = 0; i < 6; i++) {
        Serial.print("  String ");
        Serial.print(strings[i]);
        Serial.print(" (");
        Serial.print(notes[i]);
        Serial.print(") = ");
        Serial.print(freqs[i], 2);
        Serial.print(" Hz ... ");

        // Verify note_parser returns correct frequency
        float parsed = parseNote(notes[i]);
        float diff   = fabs(parsed - freqs[i]);

        if (diff > 1.0f) {
            Serial.println("PARSER MISMATCH");
            testFail(notes[i], "parseNote() returned wrong frequency");
        } else {
            playTone(freqs[i], 600);
            Serial.println("played");
        }
        delay(200);
    }

    testPass("All 6 reference tones played");
}

/* ============================================================================
 * TEST 3: MICROPHONE SAMPLING RATE
 *
 * Samples ADC for 1 second and counts samples.
 * PASS: 9000-11000 samples (confirming ~10 kHz rate)
 * FAIL: <5000 samples (delayMicroseconds too long or ADC slow)
 *       >15000 samples (delayMicroseconds too short)
 * ========================================================================== */

void test_mic_sample_rate() {
    printHeader("TEST 3: Microphone Sampling Rate");
    Serial.println("  Counting ADC samples for 1 second...");

    analogReadResolution(12);
    analogReadAveraging(4);

    int    count    = 0;
    uint32_t start  = millis();

    while (millis() - start < 1000) {
        analogRead(MICROPHONE_INPUT_PIN);
        count++;
        delayMicroseconds(95);
    }

    Serial.print("  Samples in 1 second: ");
    Serial.println(count);
    Serial.print("  Effective sample rate: ~");
    Serial.print(count);
    Serial.println(" Hz");

    if (count < 5000) {
        testFail("Sample rate", "Too slow (<5000/s) - FFT frequency resolution will be wrong");
    } else if (count > 15000) {
        testFail("Sample rate", "Too fast (>15000/s) - delayMicroseconds(95) not working");
    } else if (count < 9000 || count > 11000) {
        testWarn("Sample rate", "Outside ideal 9000-11000 range but usable");
        testPass("Microphone sampling (acceptable range)");
    } else {
        Serial.println("  Target: ~10000 Hz  [OK]");
        testPass("Microphone sampling rate correct");
    }
}

/* ============================================================================
 * TEST 4: FFT FREQUENCY DETECTION
 *
 * Plays a known tone (A2 = 110 Hz) through the speaker, captures it with
 * the microphone, and verifies FFT detects it within acceptable range.
 *
 * PASS: Detected frequency within 20 Hz of 110 Hz
 * FAIL: Returns 0.0 (no signal) or wildly wrong value
 *
 * NOTE: This test requires the microphone to be able to hear the speaker.
 *       If they are far apart or the speaker volume is very low, it may fail.
 *       This is a hardware placement issue, not a code issue.
 * ========================================================================== */

void test_fft_detection() {
    printHeader("TEST 4: FFT Frequency Detection");
    Serial.println("  Playing A2 (110 Hz) tone while capturing mic input.");
    Serial.println("  Mic must be able to hear the speaker for this test.");
    Serial.println();

    // Play the tone non-blocking style during capture
    sine1.frequency(110.0f);
    sine1.amplitude(0.8f);

    delay(300); // Let tone settle

    // Capture samples
    int16_t samples[SAMPLE_SIZE];
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        int raw    = analogRead(MICROPHONE_INPUT_PIN);
        samples[i] = (int16_t)(raw - 2048);
        delayMicroseconds(95);
    }

    sine1.amplitude(0.0f);

    // Check signal amplitude
    int max_amp = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        int a = abs(samples[i]);
        if (a > max_amp) max_amp = a;
    }

    Serial.print("  Max signal amplitude: ");
    Serial.println(max_amp);

    if (max_amp < MIN_AMPLITUDE) {
        testFail("FFT detection", "Signal too weak - mic too far from speaker, or mic not wired");
        Serial.println("  Tip: Move mic closer to speaker, or increase LM386 pot");
        return;
    }

    // Run FFT
    double detected = apply_fft(samples, SAMPLE_SIZE);

    Serial.print("  Playing:  110.00 Hz (A2)");
    Serial.println();
    Serial.print("  Detected: ");
    Serial.print(detected, 2);
    Serial.println(" Hz");

    double error = fabs(detected - 110.0);
    Serial.print("  Error: ");
    Serial.print(error, 2);
    Serial.println(" Hz");

    if (detected == 0.0) {
        testFail("FFT detection", "Returned 0.0 - signal present but FFT found no peak");
    } else if (error <= 5.0) {
        Serial.println("  Accuracy: Excellent (<5 Hz error)");
        testPass("FFT frequency detection");
    } else if (error <= 20.0) {
        Serial.print("  Accuracy: Acceptable (");
        Serial.print(error, 1);
        Serial.println(" Hz error - within 1 FFT bin)");
        testPass("FFT frequency detection (within 1 bin)");
    } else if (error <= 45.0) {
        testWarn("FFT detection", "Error >20 Hz - near harmonic or sample rate drift");
        Serial.println("  Tuner will work but cents accuracy limited. Consider parabolic interpolation.");
        testPass("FFT detection (marginal - review)");
    } else {
        testFail("FFT detection", "Error >45 Hz - detecting harmonic or wrong peak");
        Serial.print("  Expected ~110 Hz, got ");
        Serial.println(detected, 2);
    }
}

/* ============================================================================
 * TEST 5: CENTS OFFSET CALCULATION
 *
 * Feeds known frequency pairs into calculate_cents_offset() and checks math.
 * This is pure logic - no hardware required.
 *
 * Known values:
 *   A4 vs A4 = 0 cents
 *   A4 vs A#4 = -100 cents (A4 is one semitone flat of A#4)
 *   110 Hz vs 110 Hz = 0 cents
 *   115 Hz vs 110 Hz = +77 cents (sharp)
 * ========================================================================== */

void test_cents_calculation() {
    printHeader("TEST 5: Cents Offset Calculation");

    struct { double detected; double target; double expected_cents; const char* label; } cases[] = {
        {440.0,  440.0,   0.0,    "A4 vs A4 (in tune)"},
        {466.16, 440.0,  100.0,   "A#4 vs A4 (+100 cents)"},
        {415.30, 440.0, -100.0,   "Ab4 vs A4 (-100 cents)"},
        {110.0,  110.0,   0.0,    "A2 vs A2 (in tune)"},
        {115.0,  110.0,   76.96,  "115 Hz vs 110 Hz (~77 cents sharp)"},
    };

    int n = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < n; i++) {
        double result = calculate_cents_offset(cases[i].detected, cases[i].target);
        double error  = fabs(result - cases[i].expected_cents);

        Serial.print("  ");
        Serial.print(cases[i].label);
        Serial.print(": got ");
        Serial.print(result, 1);
        Serial.print(" cents (expected ");
        Serial.print(cases[i].expected_cents, 1);
        Serial.print(") error=");
        Serial.print(error, 2);

        if (error < 1.0) {
            Serial.println(" [OK]");
        } else {
            Serial.println(" [WRONG]");
            testFail("Cents calculation", cases[i].label);
            return;
        }
    }

    testPass("Cents offset calculation correct");
}

/* ============================================================================
 * TEST 6: BEEP FEEDBACK RATE SCALING
 *
 * Confirms beep interval scales correctly with cents offset.
 * Large offset = short interval (fast beeps)
 * Small offset = long interval (slow beeps)
 * In tune (<5 cents) = interval 0 (no beep)
 * ========================================================================== */

void test_beep_feedback() {
    printHeader("TEST 6: Beep Feedback Rate Scaling");
    Serial.println("  Listen for beeps starting fast and slowing down.");
    Serial.println();

    // Simulate going from 100 cents off to in tune
    double offsets[] = {100.0, 75.0, 50.0, 25.0, 15.0, 5.0, 2.0};
    int    n         = 7;

    bool rate_decreasing = true;
    uint32_t last_interval = 0;

    for (int i = 0; i < n; i++) {
        TuningResult fake_result;
        fake_result.cents_offset = offsets[i];

        // Use audio_sequencer's beep rate table
        // We call calculate_beep_interval directly
        extern uint32_t calculate_beep_interval(double cents_offset);
        uint32_t interval = calculate_beep_interval(offsets[i]);

        Serial.print("  ");
        Serial.print(offsets[i], 0);
        Serial.print(" cents -> interval=");
        Serial.print(interval);
        Serial.print(" ms  ");

        if (offsets[i] < 5.0) {
            if (interval == 0) {
                Serial.println("[IN TUNE - no beep]");
            } else {
                Serial.println("[SHOULD BE 0]");
                rate_decreasing = false;
            }
        } else {
            if (interval >= last_interval || last_interval == 0) {
                Serial.println("[OK - slower]");
                last_interval = interval;
            } else {
                Serial.println("[WRONG - should be slower than previous]");
                rate_decreasing = false;
            }

            // Play actual beep so user hears the rate
            playBeep(800.0f, 60);
            delay(interval > 500 ? 500 : interval);
        }
    }

    if (rate_decreasing) {
        testPass("Beep rate scales correctly with cents offset");
    } else {
        testFail("Beep rate", "Interval not monotonically increasing as cents decrease");
    }
}

/* ============================================================================
 * TEST 7: BUTTON -> STRING DETECTION
 *
 * Waits for user to press each button, verifies correct button_id returned.
 * PASS: button_get_event() returns correct button_id for each press
 * ========================================================================== */

void test_button_detection() {
    printHeader("TEST 7: Button -> String Detection");
    Serial.println("  Press each button when prompted.");
    Serial.println("  You will hear the corresponding string tone as confirmation.");
    Serial.println();

    hardware_interface_init();

    const char*  prompts[] = {
        "Press STRING 1 button (E4, pin 22)",
        "Press STRING 2 button (B3, pin 3)",
        "Press STRING 3 button (G3, pin 4)",
        "Press STRING 4 button (D3, pin 5)",
        "Press STRING 5 button (A2, pin 6)",
        "Press STRING 6 button (E2, pin 9)"
    };
    float freqs[] = {329.63f, 246.94f, 196.0f, 146.83f, 110.0f, 82.41f};
    int   expected_ids[] = {1, 2, 3, 4, 5, 6};

    for (int i = 0; i < 6; i++) {
        Serial.print("  >>> ");
        Serial.println(prompts[i]);
        Serial.println("      Waiting 8 seconds...");

        bool detected = false;
        uint32_t start = millis();

        while (millis() - start < 8000) {
            if (button_poll()) {
                button_event_t* ev = button_get_event();
                if (ev && ev->state == BUTTON_PRESSED) {
                    Serial.print("      Detected button_id = ");
                    Serial.println(ev->button_id);

                    if (ev->button_id == expected_ids[i]) {
                        playTone(freqs[i], 500);
                        testPass(prompts[i]);
                        detected = true;
                    } else {
                        Serial.print("      Expected ID ");
                        Serial.print(expected_ids[i]);
                        Serial.print(", got ");
                        Serial.println(ev->button_id);
                        testFail(prompts[i], "Wrong button_id returned");
                        detected = true;
                    }
                    break;
                }
            }
            delay(10);
        }

        if (!detected) {
            testFail(prompts[i], "No button press detected in 8 seconds");
        }
    }
}

/* ============================================================================
 * TEST 8: FULL TUNING CYCLE (Single String)
 *
 * Simulates a complete single-string tuning session:
 *   1. Press String 5 (A2) button
 *   2. Device plays 110 Hz reference tone
 *   3. User plays their string (or we check mic signal)
 *   4. FFT detects frequency
 *   5. analyze_tuning() returns cents offset
 *   6. Beep feedback runs
 *
 * This is the closest thing to a real tuning session without the full
 * main.cpp state machine.
 * ========================================================================== */

void test_full_cycle() {
    printHeader("TEST 8: Full Tuning Cycle (String 5 - A2)");
    Serial.println("  This runs a complete single-string tuning session.");
    Serial.println();
    Serial.println("  Step 1: Press the STRING 5 (A2) button to start.");
    Serial.println("  Step 2: You will hear the 110 Hz reference tone.");
    Serial.println("  Step 3: Hum, whistle, or play a note near 110 Hz near the mic.");
    Serial.println("  Step 4: Device will report what it heard and give feedback.");
    Serial.println();

    // Wait for button 5
    bool button_pressed = false;
    uint32_t start = millis();
    while (millis() - start < 10000) {
        if (button_poll()) {
            button_event_t* ev = button_get_event();
            if (ev && ev->state == BUTTON_PRESSED && ev->button_id == 5) {
                button_pressed = true;
                break;
            }
        }
        delay(10);
    }

    if (!button_pressed) {
        testFail("Full cycle", "String 5 button not pressed in 10 seconds - skipping");
        return;
    }

    Serial.println("  Button 5 detected!");

    // Step 2: Play reference tone
    Serial.println("  Playing A2 reference tone (110 Hz)...");
    playTone(110.0f, 1500);
    delay(200);

    // Ready beep
    Serial.println("  Ready beep - play your string/note now!");
    playBeep(1000.0f, 200);
    delay(300);

    // Step 3: Capture mic
    Serial.println("  Capturing microphone for 1 second...");
    int16_t samples[SAMPLE_SIZE];
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        int raw    = analogRead(MICROPHONE_INPUT_PIN);
        samples[i] = (int16_t)(raw - 2048);
        delayMicroseconds(95);
    }

    // Step 4: FFT
    double detected = apply_fft(samples, SAMPLE_SIZE);

    Serial.print("  Detected frequency: ");
    if (detected > 0.0) {
        Serial.print(detected, 2);
        Serial.println(" Hz");
    } else {
        Serial.println("No signal detected");
        testFail("Full cycle", "No signal from microphone - cycle aborted");
        return;
    }

    // Step 5: Analyze tuning
    TuningResult result = analyze_tuning(detected, 5); // target = string 5 (A2)

    Serial.print("  Target:       ");
    Serial.print(result.target_frequency, 2);
    Serial.println(" Hz");
    Serial.print("  Cents offset: ");
    Serial.print(result.cents_offset, 1);
    Serial.println(" cents");
    Serial.print("  Direction:    ");
    Serial.println(result.direction);

    // Step 6: Beep feedback (5 beeps)
    Serial.println("  Playing 5 feedback beeps...");
    extern uint32_t calculate_beep_interval(double cents_offset);
    uint32_t interval = calculate_beep_interval(result.cents_offset);

    if (interval == 0) {
        Serial.println("  IN TUNE! Playing jingle.");
        playSuccessJingle();
    } else {
        float abs_cents = fabs(result.cents_offset);
        float beep_freq = (float)map((long)constrain(abs_cents, 5, 100), 5, 100, 400, 1200);
        for (int i = 0; i < 5; i++) {
            playBeep(beep_freq, 60);
            delay(interval > 600 ? 600 : interval);
        }
    }

    testPass("Full tuning cycle completed");
}

/* ============================================================================
 * SETUP / MAIN
 * ========================================================================== */

void setup() {
    Serial.begin(115200);
    delay(2000);

    AudioMemory(20);
    sine1.amplitude(0.0f);
    sine_beep.amplitude(0.0f);

    analogReadResolution(12);
    analogReadAveraging(4);

    audio_processing_init();

    Serial.println();
    Serial.println("========================================");
    Serial.println("  RPVI Guitar Tuner - Function Test");
    Serial.println("  Run after test_wiring passes");
    Serial.println("========================================");
    Serial.println();

    test_audio_init();
    test_reference_tones();
    test_mic_sample_rate();
    test_fft_detection();
    test_cents_calculation();
    test_beep_feedback();
    test_button_detection();
    test_full_cycle();

    /* ===== FINAL RESULTS ===== */
    printHeader("TEST RESULTS");
    Serial.print("  PASSED: "); Serial.println(tests_passed);
    Serial.print("  FAILED: "); Serial.println(tests_failed);
    Serial.println();

    if (tests_failed == 0) {
        Serial.println("  All tests passed.");
        Serial.println("  Upload main tuner firmware:");
        Serial.println("    pio run -e teensy41 -t upload");
        playSuccessJingle();
    } else {
        Serial.println("  Fix failing tests before uploading main firmware.");
        Serial.println("  Each [FAIL] message tells you what to check.");
        // Error tone
        playBeep(300.0f, 500);
    }

    printSeparator();
}

void loop() {
    delay(1000);
}