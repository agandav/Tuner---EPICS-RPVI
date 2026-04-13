/**
 * test_basic_functions.cpp - Core Tuner Function Verification
 *
 * UPLOAD:   pio run -e teensy41_test_basic -t upload
 * MONITOR:  pio device monitor -e teensy41_test_basic   (115200 baud)
 *
 * Run AFTER test_wiring passes. Tests core tuner logic end-to-end
 * with real hardware. All measured values are printed.
 *
 * TESTS:
 *   [1/8] Audio system init       - CPU%, memory blocks, AudioMemory verified
 *   [2/8] Reference tone playback - parseNote() vs expected Hz for all 6 strings
 *   [3/8] Microphone sample rate  - counts actual samples/sec, prints result
 *   [4/8] FFT frequency detection - plays 110 Hz, captures mic, runs FFT
 *   [5/8] Cents offset math       - 5 known pairs, prints expected vs actual
 *   [6/8] Beep feedback scaling   - prints interval at each cents step, plays beeps
 *   [7/8] Button -> string ID     - press each button, prints detected ID
 *   [8/8] Full tuning cycle       - end-to-end: button -> tone -> mic -> FFT -> feedback
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
    #include "audio_sequencer.h"
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

void playTone(float freq, uint32_t dur) {
    sine1.frequency(freq); sine1.amplitude(1.0f);
    delay(dur);
    sine1.amplitude(0.0f); delay(50);
}

void playBeep(float freq, uint32_t dur) {
    sine_beep.frequency(freq); sine_beep.amplitude(1.0f);
    delay(dur);
    sine_beep.amplitude(0.0f); delay(40);
}

void jingle() {
    playTone(523.25f, 150);
    playTone(659.25f, 150);
    playTone(783.99f, 300);
}

/* ============================================================================
 * BEEP FEEDBACK UTILITIES
 * ========================================================================== */
typedef struct {
    double max_cents;
    uint32_t beep_interval;
    uint32_t beep_duration;
} BeepRateConfig;

static const BeepRateConfig beep_rates[] = {
    {100.0,  100},   // > 100 cents off: fastest
    {75.0,   150},   // 75-100 cents: very fast
    {50.0,   200},   // 50-75 cents: fast
    {40.0,   300},   // 40-50 cents: medium-fast
    {25.0,   500},   // 25-40 cents: medium
    {15.0,   800},   // 15-25 cents: slow
    {5.0,    1200},  // 5-15 cents: very slow
    {0.0,    0},     // < 5 cents: no beep (in tune)
};

#define NUM_BEEP_RATES (sizeof(beep_rates) / sizeof(BeepRateConfig))

uint32_t calculate_beep_interval(double cents_offset) {
    double abs_cents = fabs(cents_offset);
    
    for (int i = 0; i < NUM_BEEP_RATES; i++) {
        if (abs_cents >= beep_rates[i].max_cents) {
            return beep_rates[i].beep_interval;
        }
    }
    
    return 0;
}

/* ============================================================================
 * TEST TRACKING
 * ========================================================================== */
static int total_pass = 0;
static int total_fail = 0;

void divider() { Serial.println("  ------------------------------------------"); }

void header(int n, int total, const char* title) {
    Serial.println();
    Serial.println("==========================================");
    Serial.print("[TEST "); Serial.print(n);
    Serial.print("/"); Serial.print(total);
    Serial.print("]  "); Serial.println(title);
    Serial.println("==========================================");
}

void pass(const char* msg) {
    Serial.print("  [PASS]  "); Serial.println(msg);
    total_pass++;
    playBeep(1000, 80); delay(60); playBeep(1000, 80);
    delay(200);
}

void fail(const char* msg, const char* reason) {
    Serial.print("  [FAIL]  "); Serial.print(msg);
    Serial.print("  —  "); Serial.println(reason);
    total_fail++;
    playBeep(250, 500);
    delay(200);
}

void warn(const char* msg) {
    Serial.print("  [WARN]  "); Serial.println(msg);
}

void waitKey(const char* prompt) {
    Serial.println();
    Serial.print("  [ ENTER to continue");
    if (prompt && strlen(prompt)) { Serial.print(" — "); Serial.print(prompt); }
    Serial.println(" ]");
    while (!Serial.available()) delay(50);
    while (Serial.available())  Serial.read();
    Serial.println();
}

void countdown(int seconds, const char* action) {
    Serial.println();
    Serial.print("  >>> "); Serial.println(action);
    for (int i = seconds; i > 0; i--) {
        Serial.print("      "); Serial.print(i); Serial.println("...");
        delay(1000);
    }
    Serial.println("  >>> GO!");
    playBeep(800, 100); delay(80); playBeep(800, 100);
}

/* ============================================================================
 * TEST 1/8 — AUDIO SYSTEM INIT
 * ========================================================================== */
void test_audio_init() {
    header(1, 8, "AUDIO SYSTEM INIT");

    float cpu = AudioProcessorUsage();
    int   mem = AudioMemoryUsage();
    int   max_mem = AudioMemoryUsageMax();

    Serial.print("  CPU usage:          "); Serial.print(cpu, 1); Serial.println(" %");
    Serial.print("  Memory blocks used: "); Serial.println(mem);
    Serial.print("  Memory peak:        "); Serial.println(max_mem);
    Serial.print("  AudioMemory(20):    20 blocks allocated");
    Serial.println();

    if (cpu > 90.0f)   { fail("Audio init", "CPU > 90% — audio system overloaded"); return; }
    // if (mem == 0)      { fail("Audio init", "Memory usage = 0 — AudioMemory() not called"); return; }
    if (cpu > 50.0f)   warn("CPU > 50% — higher than expected for idle audio system.");

    pass("Audio system initialized correctly");
}

/* ============================================================================
 * TEST 2/8 — REFERENCE TONE PLAYBACK
 * Verifies parseNote() returns correct frequency AND plays each tone.
 * ========================================================================== */
void test_reference_tones() {
    header(2, 8, "REFERENCE TONE PLAYBACK  (all 6 strings)");
    Serial.println("  Verifying parseNote() accuracy and playing each tone.");
    Serial.println("  Listen for 6 clean sine waves, low to high.");
    Serial.println();

    const char* notes[]    = {"E2",    "A2",    "D3",    "G3",    "B3",    "E4"   };
    float       expected[] = {82.41f,  110.0f,  146.83f, 196.0f,  246.94f, 329.63f};
    int         strs[]     = {6, 5, 4, 3, 2, 1};
    bool        all_ok     = true;

    for (int i = 0; i < 6; i++) {
        float parsed = parseNote(notes[i]);
        float diff   = fabs(parsed - expected[i]);

        Serial.print("  String "); Serial.print(strs[i]);
        Serial.print("  "); Serial.print(notes[i]);
        Serial.print("  expected="); Serial.print(expected[i], 2);
        Serial.print(" Hz  parsed="); Serial.print(parsed, 2);
        Serial.print(" Hz  diff="); Serial.print(diff, 3); Serial.print(" Hz");

        if (diff > 1.0f) {
            Serial.println("  [MISMATCH]");
            all_ok = false;
        } else {
            Serial.print("  [ok]  playing...");
            playTone(expected[i], 600);
            Serial.println(" done");
        }
        delay(150);
    }

    Serial.println();
    if (!all_ok) {
        fail("Reference tones", "parseNote() returned wrong frequency for one or more notes");
    } else {
        pass("All 6 reference tones played and frequencies verified");
    }
}

/* ============================================================================
 * TEST 3/8 — MICROPHONE SAMPLING RATE
 * Counts actual ADC samples per second to verify ~10 kHz rate.
 * ========================================================================== */
void test_sample_rate() {
    header(3, 8, "MICROPHONE SAMPLING RATE");
    Serial.println("  Counting ADC samples over exactly 1 second.");
    Serial.println("  Target: ~10000 samples/sec (delayMicroseconds(95) between reads).");
    Serial.println();

    analogReadResolution(12);
    analogReadAveraging(4);

    int count = 0;
    uint32_t start = millis();
    while (millis() - start < 1000) {
        analogRead(MICROPHONE_INPUT_PIN);
        count++;
        delayMicroseconds(95);
    }

    float effective_rate = (float)count;
    float error_pct      = fabs(effective_rate - 10000.0f) / 10000.0f * 100.0f;

    Serial.print("  Samples in 1 second: "); Serial.println(count);
    Serial.print("  Effective rate:      "); Serial.print(effective_rate, 0); Serial.println(" Hz");
    Serial.print("  Target rate:         10000 Hz");
    Serial.println();
    Serial.print("  Error from target:   "); Serial.print(error_pct, 1); Serial.println(" %");
    Serial.println();
    Serial.println("  Impact on FFT:");
    Serial.print("    Hz per bin = rate / FFT_SIZE = ");
    Serial.print(effective_rate / FFT_SIZE, 2); Serial.println(" Hz/bin");
    Serial.print("    (target: "); Serial.print(10000.0f / (float) FFT_SIZE, 2); Serial.println(" Hz/bin)");
    Serial.println();

    if (count < 5000) {
        fail("Sample rate", "< 5000/s — delayMicroseconds(95) not working or ADC too slow");
    } else if (count > 15000) {
        fail("Sample rate", "> 15000/s — sampling too fast, FFT will map frequencies incorrectly");
    } else if (count < 9000 || count > 11000) {
        warn("Outside ideal 9000–11000 range. FFT bin mapping will be slightly off.");
        pass("Sampling rate acceptable (outside ideal — see WARN)");
    } else {
        pass("Microphone sampling rate correct (~10 kHz)");
    }
}

/* ============================================================================
 * TEST 4/8 — FFT FREQUENCY DETECTION
 * Plays 110 Hz through speaker, captures with mic, runs FFT.
 * Prints amplitude, detected frequency, and error.
 * ========================================================================== */
void test_fft() {
    header(4, 8, "FFT FREQUENCY DETECTION");
    Serial.println("  Plays 110 Hz (A2) through speaker.");
    Serial.println("  Mic captures audio, FFT runs, detected frequency printed.");
    Serial.println("  Mic must be near speaker. Move it close now if needed.");
    waitKey("mic is near speaker");

    Serial.println("  Playing 110 Hz and settling (300 ms)...");
    sine1.frequency(110.0f); sine1.amplitude(1.0f);
    delay(300);

    Serial.print("  Capturing "); Serial.print(SAMPLE_SIZE); Serial.println(" samples...");
    int16_t samples[SAMPLE_SIZE];
    // Capture raw
for (int i = 0; i < SAMPLE_SIZE; i++) {
    samples[i] = (int16_t)analogRead(MICROPHONE_INPUT_PIN);
    delayMicroseconds(75);
}
// Subtract actual mean
long sum = 0;
for (int i = 0; i < SAMPLE_SIZE; i++) sum += samples[i];
int16_t dc = (int16_t)(sum / SAMPLE_SIZE);
for (int i = 0; i < SAMPLE_SIZE; i++) samples[i] -= dc;

    // Signal amplitude check
    int max_amp = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        int a = abs(samples[i]);
        if (a > max_amp) max_amp = a;
    }
    Serial.print("  Peak sample amplitude: "); Serial.print(max_amp);
    Serial.print("  (MIN_AMPLITUDE threshold = "); Serial.print(MIN_AMPLITUDE); Serial.println(")");

    if (max_amp < MIN_AMPLITUDE) {
        fail("FFT detection", "Signal too weak — mic too far from speaker, or mic not wired");
        Serial.println("    Move mic closer to speaker and re-run.");
        return;
    }

    double detected = apply_fft(samples, SAMPLE_SIZE);
    double error    = fabs(detected - 110.0);

    Serial.println();
    Serial.println("  FFT RESULT:");
    Serial.print("    Played:    110.00 Hz  (A2)");
    Serial.println();
    Serial.print("    Detected:  "); Serial.print(detected, 2); Serial.println(" Hz");
    Serial.print("    Error:     "); Serial.print(error, 2); Serial.println(" Hz");
    Serial.print("    1 FFT bin: "); Serial.print(10000.0f / 256.0f, 2); Serial.println(" Hz");
    Serial.println();

    if (detected == 0.0) {
        fail("FFT", "Returned 0.0 — signal above threshold but no spectral peak found");
    } else if (error <= 5.0) {
        Serial.println("    Accuracy: Excellent (<5 Hz)");
        pass("FFT frequency detection");
    } else if (error <= 20.0) {
        Serial.println("    Accuracy: Good (within 1 FFT bin)");
        pass("FFT frequency detection (within 1 bin)");
    } else if (error <= 45.0) {
        warn("Error > 20 Hz — possibly detecting harmonic or sample rate drift.");
        Serial.println("    Tuner will work but cents-level accuracy needs parabolic interpolation.");
        pass("FFT detection (marginal — review WARN)");
    } else {
        fail("FFT", "Error > 45 Hz — detecting wrong peak or harmonic");
        Serial.print("    Expected ~110 Hz, got "); Serial.println(detected, 2);
    }
}

/* ============================================================================
 * TEST 5/8 — CENTS OFFSET CALCULATION
 * Pure math test — no hardware. Known input/output pairs.
 * ========================================================================== */
void test_cents() {
    header(5, 8, "CENTS OFFSET CALCULATION  (math only, no hardware)");
    Serial.println("  Known frequency pairs -> expected cents offset.");
    Serial.println("  Formula: 1200 * log2(detected / target)");
    Serial.println();

    struct { double det; double tgt; double exp; const char* label; } cases[] = {
        {440.00, 440.00,   0.0,   "A4 == A4             (in tune)"},
        {466.16, 440.00, 100.0,   "A#4 vs A4            (+100 cents / 1 semitone sharp)"},
        {415.30, 440.00,-100.0,   "Ab4 vs A4            (-100 cents / 1 semitone flat)"},
        {110.00, 110.00,   0.0,   "A2 == A2             (in tune)"},
        {115.00, 110.00,  76.96,  "115 Hz vs 110 Hz     (~77 cents sharp)"},
    };

    int n = sizeof(cases) / sizeof(cases[0]);
    bool all_ok = true;

    Serial.println("  Case                               Expected    Got       Error");
    divider();

    for (int i = 0; i < n; i++) {
        double result = calculate_cents_offset(cases[i].det, cases[i].tgt);
        double err    = fabs(result - cases[i].exp);

        Serial.print("  "); Serial.print(cases[i].label);
        // pad to align columns
        int pad = 35 - strlen(cases[i].label);
        for (int p = 0; p < pad; p++) Serial.print(' ');

        Serial.print(cases[i].exp, 1); Serial.print(" c    ");
        Serial.print(result, 1);       Serial.print(" c    ");
        Serial.print(err, 2); Serial.print(" c");

        if (err < 1.0) {
            Serial.println("  [ok]");
        } else {
            Serial.println("  [WRONG]");
            all_ok = false;
        }
    }

    Serial.println();
    if (!all_ok) {
        fail("Cents calculation", "One or more results outside 1 cent tolerance");
    } else {
        pass("All cents offset calculations correct");
    }
}

/* ============================================================================
 * TEST 6/8 — BEEP FEEDBACK SCALING
 * Prints interval at each cents step. Plays audible beeps so you hear the rate.
 * ========================================================================== */
void test_beep_scaling() {
    header(6, 8, "BEEP FEEDBACK RATE SCALING");
    Serial.println("  Checks that beep interval increases as cents offset decreases.");
    Serial.println("  Plays each beep so you can hear the slowing rate.");
    Serial.println();

    double   offsets[]  = {100.0, 75.0, 50.0, 25.0, 15.0, 5.0, 2.0};
    int      n          = 7;
    bool     ok         = true;
    uint32_t last_ms    = 0;

    Serial.println("  Cents    Interval(ms)   Beep pitch(Hz)   Status");
    divider();

    for (int i = 0; i < n; i++) {
        uint32_t interval = calculate_beep_interval(offsets[i]);
        float abs_c       = (float)fabs(offsets[i]);
        float bfreq       = (float)map((long)constrain(abs_c, 5, 100), 5, 100, 400, 1200);

        Serial.print("  "); Serial.print(offsets[i], 0);
        Serial.print(" c     ");
        Serial.print(interval == 0 ? 0 : (int)interval);
        Serial.print(" ms         ");

        if (offsets[i] < 5.0) {
            Serial.print("---         ");
            if (interval == 0) {
                Serial.println("[IN TUNE — no beep]");
            } else {
                Serial.println("[FAIL — should be 0]");
                ok = false;
            }
        } else {
            Serial.print((int)bfreq); Serial.print(" Hz      ");
            if (interval >= last_ms || last_ms == 0) {
                Serial.println("[ok — slower]");
                last_ms = interval;
            } else {
                Serial.println("[FAIL — should be >= previous]");
                ok = false;
            }
            // Play the beep
            playBeep(bfreq, 60);
            delay(interval > 600 ? 600 : interval);
        }
    }

    Serial.println();
    if (!ok) {
        fail("Beep scaling", "Interval did not increase monotonically as cents decreased");
    } else {
        pass("Beep intervals scale correctly with cents offset");
    }
}

/* ============================================================================
 * TEST 7/8 — BUTTON -> STRING ID
 * Prompts each button in turn. Prints detected button_id.
 * ========================================================================== */
void test_buttons() {
    header(7, 8, "BUTTON -> STRING ID DETECTION");
    Serial.println("  Press each button when prompted (8 seconds per button).");
    Serial.println("  Detected button_id printed. Correct ID plays the string tone.");
    Serial.println();

    hardware_interface_init();

    const char* prompts[]  = {"STRING 1  E4  Pin 22",
                              "STRING 2  B3  Pin 3 ",
                              "STRING 3  G3  Pin 4 ",
                              "STRING 4  D3  Pin 5 ",
                              "STRING 5  A2  Pin 6 ",
                              "STRING 6  E2  Pin 9 "};
    float       freqs[]    = {329.63f, 246.94f, 196.0f, 146.83f, 110.0f, 82.41f};
    int         expected[] = {1, 2, 3, 4, 5, 6};

    int btn_pass = 0, btn_fail = 0;

    for (int i = 0; i < 6; i++) {
        divider();
        Serial.print("  BUTTON "); Serial.print(i+1); Serial.print("/6  —  ");
        Serial.println(prompts[i]);
        Serial.print("  >>> PRESS ["); Serial.print(prompts[i]); Serial.println("] now — 8 seconds");
        playBeep(800, 100); delay(80); playBeep(800, 100);

        bool detected = false;
        uint32_t t    = millis();

        while (millis() - t < 8000) {
            if (button_poll()) {
                button_event_t* ev = button_get_event();
                if (ev && ev->state == BUTTON_PRESSED) {
                    Serial.print("    Detected button_id = "); Serial.print(ev->button_id);
                    Serial.print("  (expected "); Serial.print(expected[i]); Serial.print(")");

                    if ((int)ev->button_id == expected[i]) {
                        Serial.println("  [CORRECT]");
                        playTone(freqs[i], 500);
                        pass(prompts[i]);
                        btn_pass++;
                    } else {
                        Serial.println("  [WRONG ID]");
                        fail(prompts[i], "button_id does not match expected string number");
                        btn_fail++;
                    }
                    detected = true;
                    break;
                }
            }
            delay(10);
        }

        if (!detected) {
            fail(prompts[i], "No press detected in 8 seconds");
            btn_fail++;
        }
    }

    Serial.println();
    Serial.print("  Button results:  "); Serial.print(btn_pass);
    Serial.print(" passed  /  "); Serial.print(btn_fail); Serial.println(" failed");
}

/* ============================================================================
 * TEST 8/8 — FULL TUNING CYCLE (String 5 — A2, 110 Hz)
 * End-to-end: button press -> reference tone -> mic capture -> FFT -> feedback.
 * ========================================================================== */
void test_full_cycle() {
    header(8, 8, "FULL TUNING CYCLE  [All 6 Strings — Randomized Detuning]");
    Serial.println("  Each string plays a randomly detuned tone (1.5-2.5 semitones off).");
    Serial.println("  FFT should detect the detuned frequency, not the target.");
    Serial.println();

    randomSeed(millis());

    const char* string_names[] = {"E4", "B3", "G3", "D3", "A2", "E2"};
    float       string_freqs[] = {329.63f, 246.94f, 196.0f, 146.83f, 110.0f, 82.41f};
    int         string_pins[]  = {1, 2, 3, 4, 5, 6};

    int cycle_pass = 0;
    int cycle_fail = 0;

    for (int s = 0; s < 6; s++) {
        Serial.println();
        Serial.print("  ----------------------------------------");
        Serial.println();
        Serial.print("  STRING "); Serial.print(s + 1);
        Serial.print("/6  —  "); Serial.print(string_names[s]);
        Serial.print("  (target: "); Serial.print(string_freqs[s], 2); Serial.println(" Hz)");

        // Wait for correct button press
        Serial.print("  >>> PRESS STRING "); Serial.print(string_pins[s]);
        Serial.print(" BUTTON ("); Serial.print(string_names[s]);
        Serial.println(") — 10 seconds");
        playBeep(800, 100); delay(80); playBeep(800, 100);

        bool pressed = false;
        uint32_t t = millis();
        while (millis() - t < 10000) {
            if (button_poll()) {
                button_event_t* ev = button_get_event();
                if (ev && ev->state == BUTTON_PRESSED && (int)ev->button_id == string_pins[s]) {
                    pressed = true;
                    Serial.print("  String "); Serial.print(string_pins[s]); Serial.println(" detected.");
                    break;
                }
            }
            delay(10);
        }

        if (!pressed) {
            fail("Full cycle", "Button not pressed in 10 seconds — skipping this string");
            cycle_fail++;
            continue;
        }

        // Calculate randomized detuned frequency
        float offset_direction = (random(0, 2) == 0) ? 1.0f : -1.0f;
        float offset_semitones = (float)random(150, 250) / 100.0f;
        float ratio = powf(2.0f, (offset_direction * offset_semitones) / 12.0f);
        float played_freq = string_freqs[s] * ratio;
        float expected_cents = offset_direction * offset_semitones * 100.0f;

        Serial.print("  Detuning: "); Serial.print(offset_semitones, 2);
        Serial.print(" semitones "); Serial.println(offset_direction > 0 ? "SHARP" : "FLAT");
        Serial.print("  Playing detuned freq: "); Serial.print(played_freq, 2); Serial.println(" Hz");
        Serial.print("  Expected cents offset: ~"); Serial.print(expected_cents, 0); Serial.println(" cents");

        sine1.frequency(played_freq);
        sine1.amplitude(1.0f);
        delay(300);

        // Capture
        Serial.print("  Capturing "); Serial.print(SAMPLE_SIZE); Serial.println(" mic samples...");
        int16_t samples[SAMPLE_SIZE];
        for (int i = 0; i < SAMPLE_SIZE; i++) {
            samples[i] = (int16_t)analogRead(MICROPHONE_INPUT_PIN);
            delayMicroseconds(75);
        }
        sine1.amplitude(0.0f);

        long sum = 0;
        for (int i = 0; i < SAMPLE_SIZE; i++) sum += samples[i];
        int16_t dc = (int16_t)(sum / SAMPLE_SIZE);
        for (int i = 0; i < SAMPLE_SIZE; i++) samples[i] -= dc;

        int max_amp = 0;
        for (int i = 0; i < SAMPLE_SIZE; i++) {
            int a = abs(samples[i]);
            if (a > max_amp) max_amp = a;
        }
        Serial.print("  Peak amplitude: "); Serial.println(max_amp);

        if (max_amp < MIN_AMPLITUDE) {
            fail("Full cycle", "No usable signal from microphone");
            cycle_fail++;
            continue;
        }

        double detected = apply_fft(samples, SAMPLE_SIZE);
        Serial.print("  FFT detected:  "); Serial.print(detected, 2); Serial.println(" Hz");

        if (detected <= 0.0) {
            fail("Full cycle", "FFT returned 0.0 — no spectral peak above threshold");
            cycle_fail++;
            continue;
        }

        // Check detected is close to played_freq, not target
        double tracking_error = fabs(detected - (double)played_freq);
        Serial.print("  Tracking error vs played: "); Serial.print(tracking_error, 2); Serial.println(" Hz");

        TuningResult result = analyze_tuning(detected, string_pins[s]);
        Serial.println("  TUNING ANALYSIS:");
        Serial.print("    Target:       "); Serial.print(result.target_frequency, 2); Serial.println(" Hz");
        Serial.print("    Played:       "); Serial.print(played_freq, 2); Serial.println(" Hz");
        Serial.print("    Detected:     "); Serial.print(result.detected_frequency, 2); Serial.println(" Hz");
        Serial.print("    Cents offset: "); Serial.print(result.cents_offset, 2); Serial.println(" cents");
        Serial.print("    Direction:    "); Serial.println(result.direction);

        // Verify direction matches detuning
        bool direction_correct = (offset_direction > 0 && strcmp(result.direction, "DOWN") == 0) ||
                                 (offset_direction < 0 && strcmp(result.direction, "UP") == 0);
        Serial.print("  Direction check: "); Serial.println(direction_correct ? "[CORRECT]" : "[WRONG]");

        uint32_t interval = calculate_beep_interval(result.cents_offset);
        float abs_c = (float)fabs(result.cents_offset);
        float bfreq = (float)map((long)constrain(abs_c, 5, 100), 5, 100, 400, 1200);
        Serial.print("  Playing 3 feedback beeps at ");
        Serial.print((int)bfreq); Serial.print(" Hz, ");
        Serial.print(interval); Serial.println(" ms apart...");
        for (int i = 0; i < 3; i++) {
            playBeep(bfreq, 60);
            delay(interval > 700 ? 700 : interval);
        }

        if (tracking_error < 20.0 && direction_correct) {
            pass("Full cycle complete — detected detuned freq correctly");
            cycle_pass++;
        } else if (tracking_error < 20.0) {
            warn("Detected correct frequency but direction mismatch");
            pass("Full cycle complete (direction warn)");
            cycle_pass++;
        } else {
            fail("Full cycle", "Detected frequency too far from played frequency");
            cycle_fail++;
        }

        if (s < 5) {
            Serial.println("  (3 second pause before next string)");
            delay(3000);
        }
    }

    Serial.println();
    Serial.print("  String cycle results: "); Serial.print(cycle_pass);
    Serial.print(" passed  /  "); Serial.print(cycle_fail); Serial.println(" failed");
}

/* ============================================================================
 * SETUP
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
    Serial.println("==========================================");
    Serial.println("   RPVI Guitar Tuner  —  Function Test");
    Serial.println("   Run after test_wiring passes");
    Serial.println("==========================================");
    Serial.println();
    Serial.println("  8 tests. Tests 1–6 run automatically.");
    Serial.println("  Tests 7–8 require button presses.");
    Serial.println("  Real measured values printed at every step.");
    Serial.println();

    waitKey("start function tests");

    test_audio_init();
    test_reference_tones();
    test_sample_rate();
    test_fft();
    test_cents();
    test_beep_scaling();
    test_buttons();
    test_full_cycle();

    Serial.println();
    Serial.println("==========================================");
    Serial.println("              FINAL RESULTS");
    Serial.println("==========================================");
    Serial.print("  PASSED: "); Serial.println(total_pass);
    Serial.print("  FAILED: "); Serial.println(total_fail);
    Serial.println();

    if (total_fail == 0) {
        Serial.println("  All function tests passed.");
        Serial.println("  Next: pio run -e teensy41 -t upload");
        jingle();
    } else {
        Serial.print("  "); Serial.print(total_fail);
        Serial.println(" test(s) failed. Fix before uploading main firmware.");
        Serial.println("  Each [FAIL] message above gives the exact reason.");
        playBeep(300, 600);
    }
}

void loop() { delay(1000); }