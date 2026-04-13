/**
 * test_wiring.cpp - Hardware Wiring Verification Test
 *
 * UPLOAD:   pio run -e teensy41_test_wiring -t upload
 * MONITOR:  pio device monitor -e teensy41_test_wiring   (115200 baud)
 *
 * Tests every hardware connection in order.
 * Follow prompts exactly. Each test prints real measured values.
 *
 * TESTS:
 *   [1/5] Power supply    - prompts multimeter check
 *   [2/5] Audio output    - plays tones, uses mic to objectively verify speaker
 *   [3/5] Microphone      - prints ADC min/max/range for baseline and active
 *   [4/5] Buttons         - prints pin state, transition, and release state
 *   [5/5] Mode switch     - counts and timestamps every transition
 *
 * AUDIO CUES:
 *   Two high beeps (1kHz) = PASS
 *   One long low  (250Hz) = FAIL
 *   Two medium    (800Hz) = prompt / action required now
 */

#include <Arduino.h>
#include <AudioStream.h>
#include <synth_sine.h>
#include <output_mqs.h>

/* ============================================================================
 * PIN DEFINITIONS
 * ========================================================================== */
#define MIC_PIN         39
#define STRING_1_PIN    22
#define STRING_2_PIN    3
#define STRING_3_PIN    4
#define STRING_4_PIN    5
#define STRING_5_PIN    6
#define STRING_6_PIN    9
#define MODE_PIN        11

/* ============================================================================
 * AUDIO
 * ========================================================================== */
AudioSynthWaveformSine sine1;
AudioOutputMQS         mqs_output;
AudioConnection        patch1(sine1, 0, mqs_output, 0);
AudioConnection        patch2(sine1, 0, mqs_output, 1);

void beep(float freq, uint32_t dur, float amp = 0.8f) {
    sine1.frequency(freq); sine1.amplitude(amp);
    delay(dur);
    sine1.amplitude(0.0f); delay(50);
}

void signalPass()   { beep(1000, 80); delay(60); beep(1000, 80); }
void signalFail()   { beep(250,  600); }
void signalPrompt() { beep(800,  100); delay(80); beep(800, 100); }

/* ============================================================================
 * PRINT HELPERS
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
    signalPass();
    delay(300);
}

void fail(const char* msg) {
    Serial.print("  [FAIL]  "); Serial.println(msg);
    total_fail++;
    signalFail();
    delay(300);
}

void warn(const char* msg) {
    Serial.print("  [WARN]  "); Serial.println(msg);
}

void countdown(int seconds, const char* action) {
    Serial.println();
    Serial.print("  >>> "); Serial.println(action);
    for (int i = seconds; i > 0; i--) {
        Serial.print("        "); Serial.print(i); Serial.println("...");
        delay(1000);
    }
    Serial.println("  >>> GO!");
    signalPrompt();
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

int captureMicStats(uint32_t duration_ms, int* out_min, int* out_max) {
    int mn = 4095, mx = 0;
    uint32_t t = millis();
    while (millis() - t < duration_ms) {
        int v = analogRead(MIC_PIN);
        if (v < mn) mn = v;
        if (v > mx) mx = v;
        delayMicroseconds(100);
    }
    if (out_min) *out_min = mn;
    if (out_max) *out_max = mx;
    return mx - mn;
}

bool waitForPinState(uint8_t pin, int target_state, uint32_t timeout_ms) {
    uint32_t t0 = millis();
    while (millis() - t0 < timeout_ms) {
        if (digitalRead(pin) == target_state) return true;
        delay(10);
    }
    return false;
}

/* ============================================================================
 * TEST 1/5 — POWER SUPPLY
 * ========================================================================== */
void test_power() {
    header(1, 5, "POWER SUPPLY");

    Serial.println("  Teensy is running — USB power confirmed.");
    Serial.println("  This does NOT automatically verify the PowerBoost 5V rail.");
    Serial.println();
    Serial.println("  MULTIMETER CHECK:");
    Serial.println("    Red probe  ->  Teensy Vin pin");
    Serial.println("    Black probe ->  GND");
    Serial.println("    Expected:      4.8 V – 5.2 V");
    Serial.println();
    Serial.println("  ALSO CONFIRM:");
    Serial.println("    - LiPo battery plugged into PowerBoost BAT+/BAT-");
    Serial.println("    - Power rocker switch is ON");
    Serial.println("    - PowerBoost green LED is lit (= 5V output active)");

    waitKey("Vin reads 5V on multimeter");
    pass("Power supply confirmed");
}

/* ============================================================================
 * TEST 2/5 — AUDIO OUTPUT
 * Uses mic as objective sensor to confirm speaker is producing sound.
 * Prints quiet baseline and tone range side by side.
 * ========================================================================== */
void test_audio() {
    header(2, 5, "AUDIO OUTPUT  [Pin 10 -> LM386 -> Speaker]");

    analogReadResolution(12);
    analogReadAveraging(4);

    Serial.println("  Place the microphone CLOSE to the speaker for this test.");
    Serial.println("  The mic ADC will be compared quiet vs. tone playing.");
    Serial.println("  A clear increase in ADC range = speaker path is working.");
    waitKey("mic is positioned near speaker");

    // Quiet baseline
    Serial.println("  Measuring quiet baseline (1.5 seconds)...");
    int q_min, q_max;
    int quiet_range = captureMicStats(1500, &q_min, &q_max);
    Serial.println("  QUIET:");
    Serial.print("    ADC min: "); Serial.print(q_min);
    Serial.print("   max: "); Serial.print(q_max);
    Serial.print("   range: "); Serial.println(quiet_range);

    // Tone active
    Serial.println("  Playing 1000 Hz tone and measuring mic (1.5 seconds)...");
    sine1.frequency(1000.0f); sine1.amplitude(0.8f);
    int t_min, t_max;
    int tone_range = captureMicStats(1500, &t_min, &t_max);
    sine1.amplitude(0.0f);

    Serial.println("  TONE PLAYING:");
    Serial.print("    ADC min: "); Serial.print(t_min);
    Serial.print("   max: "); Serial.print(t_max);
    Serial.print("   range: "); Serial.println(tone_range);
    Serial.println();
    Serial.print("  DELTA (tone - quiet): "); Serial.print(tone_range - quiet_range);
    Serial.println("  counts  (need >25 for clear detection)");
    Serial.println();

    if (quiet_range < 10 && tone_range < 15) {
        warn("Mic appears static — cannot verify speaker objectively.");
        Serial.println("    Complete mic test first, then re-run audio test.");
    } else if (tone_range > quiet_range + 25) {
        Serial.println("  Speaker confirmed audible via microphone.");
    } else {
        warn("Tone range did not clearly exceed quiet range.");
        Serial.println("    If you HEARD the tone: speaker works, try moving mic closer.");
        Serial.println("    If SILENT: check LM386 IN->Pin10, LM386 Vdd->5V, speaker wires.");
        Serial.println("    LM386 pot: turn clockwise to increase volume.");
    }

    // Play all 6 strings regardless for human ear check
    Serial.println();
    Serial.println("  Playing 6 string tones for listening check:");
    float  f[] = {82.41f, 110.0f, 146.83f, 196.0f, 246.94f, 329.63f};
    const char* n[] = {"E2  82.41 Hz", "A2 110.00 Hz", "D3 146.83 Hz",
                       "G3 196.00 Hz", "B3 246.94 Hz", "E4 329.63 Hz"};
    for (int i = 0; i < 6; i++) {
        Serial.print("    String "); Serial.print(6-i);
        Serial.print("  "); Serial.print(n[i]); Serial.print("  ...");
        beep(f[i], 600, 0.9f);
        Serial.println("  done");
    }
    Serial.println();
    Serial.println("  Distortion/buzzing: add 10 ohm resistor across speaker terminals.");
    pass("Audio output test complete");
}

/* ============================================================================
 * TEST 3/5 — MICROPHONE INPUT
 * Two windows: quiet baseline then tap/speak. Prints all ADC values.
 * ========================================================================== */
void test_mic() {
    header(3, 5, "MICROPHONE INPUT  [Pin 39 / A17]");

    analogReadResolution(12);
    analogReadAveraging(4);

    Serial.println("  ADC range: 0 – 4095  (12-bit, 3.3V reference)");
    Serial.println("  Idle mic with bias resistor: center ~1800 – 2200");
    Serial.println("  Loud sound: readings push toward 0 or 4095");
    Serial.println();

    // Baseline
    countdown(3, "Stay QUIET — measuring baseline in");
    int mn, mx;
    int baseline = captureMicStats(2000, &mn, &mx);
    Serial.println();
    Serial.println("  BASELINE (quiet):");
    Serial.print("    ADC min:  "); Serial.println(mn);
    Serial.print("    ADC max:  "); Serial.println(mx);
    Serial.print("    Range:    "); Serial.print(baseline); Serial.println(" counts");
    divider();

    if (mn < 100)  warn("Min ADC < 100 — mic may lack bias voltage. Check 2.2k–10k resistor to 3.3V.");
    if (mx > 3900) warn("Max ADC > 3900 — mic may be clipping at idle. Check bias network.");
    if (baseline > 300) warn("Baseline range > 300 — noisy environment or electrical interference.");

    // Active
    countdown(3, "TAP the mic or clap loudly near it, starting in");
    int baseline2, mn2, mx2;
    int active = captureMicStats(3000, &mn2, &mx2);
    Serial.println();
    Serial.println("  ACTIVE (tapping/speaking):");
    Serial.print("    ADC min:  "); Serial.println(mn2);
    Serial.print("    ADC max:  "); Serial.println(mx2);
    Serial.print("    Range:    "); Serial.print(active); Serial.println(" counts");
    Serial.print("    Delta:    "); Serial.print(active - baseline);
    Serial.println(" counts  (need >50 for reliable tuning)");
    Serial.println();

    if (baseline < 8 && active < 12) {
        fail("ADC static in both windows — mic not connected or bias missing.");
        Serial.println("    Check: wire on pin 39, bias resistor to 3.3V, mic GND.");
    } else if (active < baseline + 20) {
        fail("ADC did not respond to sound. Mic connected but not picking up audio.");
        Serial.println("    Check: mic orientation, solder joints, bias resistor value.");
    } else if (active < 50) {
        warn("Signal weak. FFT will work but tuning accuracy may be limited.");
        pass("Microphone connected (weak — see WARN above)");
    } else {
        pass("Microphone responding correctly to sound");
    }
}

/* ============================================================================
 * TEST 4/5 — STRING SELECTION BUTTONS
 * Each button tested individually. Prints pin number, rest state,
 * press state, and release state. Polarity-agnostic detection.
 * ========================================================================== */
void test_buttons() {
    header(4, 5, "STRING SELECTION BUTTONS");

    const uint8_t pins[]   = {STRING_1_PIN, STRING_2_PIN, STRING_3_PIN,
                              STRING_4_PIN, STRING_5_PIN, STRING_6_PIN};
    const char*  labels[]  = {"String 1  E4  Pin 22",
                              "String 2  B3  Pin 3 ",
                              "String 3  G3  Pin 4 ",
                              "String 4  D3  Pin 5 ",
                              "String 5  A2  Pin 6 ",
                              "String 6  E2  Pin 9 "};
    const float  freqs[]   = {329.63f, 246.94f, 196.0f, 146.83f, 110.0f, 82.41f};

    // Active HIGH — buttons connect to supply voltage when pressed
    for (int i = 0; i < 6; i++) pinMode(pins[i], INPUT_PULLDOWN);

    Serial.println("  Buttons wired ACTIVE HIGH (connect supply when pressed).");
    Serial.println("  INPUT_PULLDOWN used — pins rest LOW, rise HIGH when pressed.");
    Serial.println("  Each button tested one at a time. Wait for the prompt.");
    Serial.println("  TIP: If pressing is difficult, use a jumper from 3.3V to the target pin briefly.");
    Serial.println("  If no change is detected, test will auto-run a GND/pullup sanity check.");
    Serial.println();

    int btn_pass = 0, btn_fail = 0;

    for (int i = 0; i < 6; i++) {
        divider();
        Serial.print("  BUTTON "); Serial.print(i+1); Serial.print("/6  —  ");
        Serial.println(labels[i]);

        int rest = digitalRead(pins[i]);
        Serial.print("    Pin "); Serial.print(pins[i]);
        Serial.print(" at rest: "); Serial.print(rest == HIGH ? "HIGH" : "LOW");
        Serial.println(rest == HIGH ? "  [WARNING: unexpected HIGH at rest]" : "  [good]");

        Serial.print("  >>> QUICK ELECTRICAL CHECK [Pin ");
        Serial.print(pins[i]);
        Serial.println("] — touch 3.3V jumper now (3s window)");
        bool forced_high = waitForPinState(pins[i], HIGH, 3000);

        if (!forced_high) {
            Serial.println("    Did NOT read HIGH from direct 3.3V test.");
            Serial.println("    Running pull-up + GND sanity check...");
            pinMode(pins[i], INPUT_PULLUP);
            delay(2);

            int pu_rest = digitalRead(pins[i]);
            Serial.print("    Pull-up rest state: ");
            Serial.println(pu_rest == HIGH ? "HIGH" : "LOW");
            Serial.print("    Touch GND jumper to pin ");
            Serial.print(pins[i]);
            Serial.println(" now (3s window)");

            bool forced_low = waitForPinState(pins[i], LOW, 3000);
            pinMode(pins[i], INPUT_PULLDOWN);
            delay(2);

            if (forced_low) {
                Serial.println("    Pin responds to GND in pull-up mode.");
                Serial.println("    This strongly suggests ACTIVE-LOW button wiring.");
                Serial.println("    Current test expects ACTIVE-HIGH (to 3.3V), so press events will fail.");
                fail(labels[i]);
                btn_fail++;
                if (i < 5) {
                    Serial.println("    (3 second pause before next button)");
                    delay(3000);
                }
                continue;
            } else {
                Serial.println("    Pin did not respond to 3.3V or GND jumper tests.");
                Serial.println("    Likely wrong physical pin, bad jumper contact, unsoldered header, or hard short.");
                fail(labels[i]);
                btn_fail++;
                if (i < 5) {
                    Serial.println("    (3 second pause before next button)");
                    delay(3000);
                }
                continue;
            }
        }

        Serial.print("  >>> PRESS AND HOLD ["); Serial.print(labels[i]);
        Serial.println("] — 7 seconds");
        Serial.print("      Jumper option: touch 3.3V -> pin ");
        Serial.print(pins[i]);
        Serial.println(" to force a clean HIGH.");
        signalPrompt();

        bool detected = false;
        bool stuck    = false;
        int  pressed_state = rest;
        bool saw_other_pin_change = false;
        int  other_pin_index = -1;
        uint32_t t = millis();

        int baseline[6];
        for (int j = 0; j < 6; j++) baseline[j] = digitalRead(pins[j]);

        while (millis() - t < 7000) {
            bool found_transition = false;
            int transition_idx = -1;
            int transition_val = LOW;

            for (int j = 0; j < 6; j++) {
                int cur_j = digitalRead(pins[j]);
                if (cur_j != baseline[j]) {
                    found_transition = true;
                    transition_idx = j;
                    transition_val = cur_j;
                    baseline[j] = cur_j;
                    break;
                }
            }

            if (found_transition && transition_idx != i) {
                saw_other_pin_change = true;
                other_pin_index = transition_idx;
                Serial.print("    NOTE: saw activity on pin ");
                Serial.print(pins[transition_idx]);
                Serial.print(" while testing pin ");
                Serial.print(pins[i]);
                Serial.println(".");
            }

            if (found_transition && transition_idx == i) {
                int cur = transition_val;
                detected      = true;
                pressed_state = cur;
                Serial.print("    State changed at t=");
                Serial.print((millis()-t)/1000.0f, 2); Serial.print("s");
                Serial.print("  Pin "); Serial.print(pins[i]);
                Serial.print(": "); Serial.println(cur == HIGH ? "HIGH" : "LOW");

                // Play tone while held
                sine1.frequency(freqs[i]); sine1.amplitude(0.9f);
                uint32_t held = millis();
                while (digitalRead(pins[i]) == pressed_state) {
                    if (millis() - held > 3000) { stuck = true; break; }
                    delay(10);
                }
                sine1.amplitude(0.0f);

                int after = digitalRead(pins[i]);
                Serial.print("    Released:  Pin ");
                Serial.print(pins[i]); Serial.print(" = ");
                Serial.print(after == HIGH ? "HIGH" : "LOW");
                Serial.println(after == rest ? "  (returned to rest — good)" : "  [WARNING: did not return to rest]");
                break;
            }
            delay(20);
        }

        if (!detected) {
            Serial.println("    No state change detected.");
            Serial.println("    Check: button wire connects pin to supply voltage when pressed");
            if (saw_other_pin_change && other_pin_index >= 0) {
                Serial.print("    Likely mapping/wiring mismatch: expected pin ");
                Serial.print(pins[i]);
                Serial.print(", but saw pin ");
                Serial.print(pins[other_pin_index]);
                Serial.println(" change.");
            }

            Serial.println("    3-second live poll (100ms):");
            Serial.println("      Format: p22 p3 p4 p5 p6 p9");
            uint32_t diag_t = millis();
            while (millis() - diag_t < 3000) {
                for (int j = 0; j < 6; j++) {
                    Serial.print("p"); Serial.print(pins[j]);
                    Serial.print("="); Serial.print(digitalRead(pins[j]));
                    Serial.print("  ");
                }
                Serial.println();
                delay(100);
            }

            fail(labels[i]);
            btn_fail++;
        } else if (stuck) {
            Serial.println("    Pin stayed changed >3s — short or mechanically stuck button.");
            fail(labels[i]);
            btn_fail++;
        } else {
            beep(freqs[i], 350, 0.9f);
            pass(labels[i]);
            btn_pass++;
        }

        if (i < 5) {
            Serial.println("    (3 second pause before next button)");
            delay(3000);
        }
    }

    Serial.println();
    Serial.print("  Button results:  "); Serial.print(btn_pass);
    Serial.print(" passed  /  "); Serial.print(btn_fail); Serial.println(" failed");
}

/* ============================================================================
 * TEST 5/5 — MODE SWITCH
 * ========================================================================== */
void test_mode_switch() {
    header(5, 5, "MODE SWITCH  [Pin 11]");

    pinMode(MODE_PIN, INPUT_PULLDOWN);

    int initial = digitalRead(MODE_PIN);
    Serial.print("  Pin 11 initial state: ");
    Serial.println(initial == HIGH ? "HIGH" : "LOW");
    Serial.println();
    Serial.println("  Toggle the switch back and forth at least 3 times.");
    Serial.println("  Each transition will be printed with its timestamp.");
    countdown(2, "Start toggling in");

    bool saw_high    = false;
    bool saw_low     = false;
    int  transitions = 0;
    int  last_val    = -1;
    uint32_t start   = millis();

    while (millis() - start < 8000) {
        int val = digitalRead(MODE_PIN);
        if (val != last_val) {
            float ts = (millis() - start) / 1000.0f;
            if (last_val == -1) {
                Serial.print("  t="); Serial.print(ts, 1);
                Serial.print("s  Initial:  ");
                Serial.println(val == HIGH ? "HIGH (Play Tone mode)" : "LOW (Listen Only mode)");
            } else {
                transitions++;
                Serial.print("  t="); Serial.print(ts, 1);
                Serial.print("s  Transition "); Serial.print(transitions);
                Serial.print(":  "); Serial.print(last_val == HIGH ? "HIGH" : "LOW");
                Serial.print(" -> "); Serial.println(val == HIGH ? "HIGH" : "LOW");
            }
            if (val == HIGH) saw_high = true;
            if (val == LOW)  saw_low  = true;
            last_val = val;
        }
        delay(30);
    }

    Serial.println();
    Serial.print("  Total transitions: "); Serial.println(transitions);
    Serial.print("  Saw HIGH: "); Serial.println(saw_high ? "YES" : "NO");
    Serial.print("  Saw LOW:  "); Serial.println(saw_low  ? "YES" : "NO");
    Serial.println();

    if (saw_high && saw_low && transitions >= 2) {
        pass("Mode switch toggling correctly between HIGH and LOW");
    } else if (!saw_high && !saw_low) {
        fail("Pin 11 completely unresponsive — check switch wiring and GND reference");
    } else if (!saw_high) {
        fail("Never went HIGH — switch not connected to supply in one position");
    } else if (!saw_low) {
        fail("Never went LOW — check INPUT_PULLDOWN and switch wiring");
    } else {
        fail("Fewer than 2 transitions detected — toggle switch more or check contacts");
    }
}

/* ============================================================================
 * SETUP
 * ========================================================================== */
void setup() {
    Serial.begin(115200);
    delay(2000);

    AudioMemory(20);
    sine1.amplitude(0.0f);

    Serial.println();
    Serial.println("==========================================");
    Serial.println("   RPVI Guitar Tuner  —  Wiring Test");
    Serial.println("   Run FIRST before any other firmware");
    Serial.println("==========================================");
    Serial.println();
    Serial.println("  5 tests in order. Follow each prompt.");
    Serial.println("  Real measured values printed at every step.");
    Serial.println();
    Serial.println("  TESTS:");
    Serial.println("    [1/5] Power supply");
    Serial.println("    [2/5] Audio output   (Pin 10 -> LM386 -> Speaker)");
    Serial.println("    [3/5] Microphone     (Pin 39 / A17)");
    Serial.println("    [4/5] 6 string buttons");
    Serial.println("    [5/5] Mode switch    (Pin 11)");
    Serial.println();
    Serial.println("  AUDIO CUES:");
    Serial.println("    Two high beeps  (1 kHz) = PASS");
    Serial.println("    One long low    (250 Hz) = FAIL");
    Serial.println("    Two medium      (800 Hz) = action required");

    waitKey("start test sequence");

    test_power();
    test_audio();
    test_mic();
    test_buttons();
    test_mode_switch();

    Serial.println();
    Serial.println("==========================================");
    Serial.println("              FINAL RESULTS");
    Serial.println("==========================================");
    Serial.print("  PASSED: "); Serial.println(total_pass);
    Serial.print("  FAILED: "); Serial.println(total_fail);
    Serial.println();

    if (total_fail == 0) {
        Serial.println("  All checks passed.");
        Serial.println("  Next: pio run -e teensy41_test_basic -t upload");
        beep(523, 150); beep(659, 150); beep(784, 300);
    } else {
        Serial.print("  "); Serial.print(total_fail);
        Serial.println(" check(s) failed. Fix before continuing.");
        Serial.println("  Each FAIL message above says exactly what to check.");
        beep(300, 600);
    }
}

void loop() { delay(1000); }