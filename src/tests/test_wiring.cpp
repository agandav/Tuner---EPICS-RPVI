/**
 * test_wiring.cpp - Hardware Wiring Verification Test
 *
 * PURPOSE:
 *   Upload this FIRST before anything else. It tests every wire
 *   independently so you know exactly what is and isn't connected
 *   correctly before running the real tuner code.
 *
 * UPLOAD COMMAND:
 *   pio run -e teensy41_test_wiring -t upload
 *
 * THEN OPEN SERIAL MONITOR:
 *   pio device monitor -e teensy41_test_wiring
 *
 * WHAT TO EXPECT:
 *   The test walks through each hardware component one at a time.
 *   Serial monitor tells you exactly what it is testing and what
 *   you should hear or see. Follow the prompts.
 *
 * WIRING BEING TESTED:
 *   Pin 10  -> LM386 IN       (MQS audio output)
 *   Pin 39  -> Electret mic   (A17 analog input)
 *   Pin 22  -> String 1 btn   (E4)
 *   Pin 3   -> String 2 btn   (B3)
 *   Pin 4   -> String 3 btn   (G3)
 *   Pin 5   -> String 4 btn   (D3)
 *   Pin 6   -> String 5 btn   (A2)
 *   Pin 9   -> String 6 btn   (E2)
 *   Pin 11  -> Mode switch
 *   Vin     -> PowerBoost 5V
 */

#include <Arduino.h>
#include <AudioStream.h>
#include <synth_sine.h>
#include <output_mqs.h>

/* ============================================================================
 * PIN DEFINITIONS 
 * ========================================================================== */

#define MIC_PIN            39   // Pin 39
#define STRING_1_PIN       22
#define STRING_2_PIN       3
#define STRING_3_PIN       4
#define STRING_4_PIN       5
#define STRING_5_PIN       6
#define STRING_6_PIN       9
#define MODE_SWITCH_PIN    11

/* ============================================================================
 * AUDIO
 * ========================================================================== */

AudioSynthWaveformSine sine1;
AudioOutputMQS         mqs_output;
AudioConnection        patchCord1(sine1, 0, mqs_output, 0);
AudioConnection        patchCord2(sine1, 0, mqs_output, 1);

void playTone(float freq, uint32_t duration_ms) {
    sine1.frequency(freq);
    sine1.amplitude(0.8f);
    delay(duration_ms);
    sine1.amplitude(0.0f);
    delay(50);
}

/* ============================================================================
 * TEST HELPERS
 * ========================================================================== */

void printSeparator() {
    Serial.println("----------------------------------------");
}

void printHeader(const char* title) {
    Serial.println();
    printSeparator();
    Serial.println(title);
    printSeparator();
}

void pass(const char* msg) {
    Serial.print("[PASS] ");
    Serial.println(msg);
}

void fail(const char* msg) {
    Serial.print("[FAIL] ");
    Serial.println(msg);
}

void info(const char* msg) {
    Serial.print("[INFO] ");
    Serial.println(msg);
}

void waitForSerial() {
    Serial.println("       Press any key in Serial Monitor to continue...");
    while (!Serial.available()) delay(50);
    while (Serial.available()) Serial.read();
}

/* ============================================================================
 * TEST 2: AUDIO OUTPUT (Speaker + LM386 + MQS Pin 10)
 *
 * PASS condition: You hear a tone from the speaker
 * FAIL condition: Silence
 *
 * If FAIL:
 *   - Check LM386 IN is wired to Teensy pin 10 (not pin 9, not pin 12)
 *   - Check LM386 Vdd is wired to PowerBoost 5V output (not 3.3V)
 *   - Check both LM386 GND pins are connected to common ground
 *   - Check speaker is connected to LM386 OUT+ and GND
 *   - Turn the LM386 onboard potentiometer clockwise (may be at zero)
 * ========================================================================== */

void test_audio_output() {
    printHeader("TEST 2: AUDIO OUTPUT (Pin 10 -> LM386 -> Speaker)");

    info("Playing 1000 Hz tone for 1 second...");
    playTone(1000.0f, 1000);
    Serial.println();
    Serial.println("  Did you hear a tone from the speaker?");
    Serial.println("  Y = wiring correct");
    Serial.println("  N = check LM386 IN connected to pin 10, LM386 Vdd to 5V");
    waitForSerial();

    info("Playing guitar string frequencies (E2 through E4)...");
    float freqs[] = {82.41f, 110.0f, 146.83f, 196.0f, 246.94f, 329.63f};
    const char* names[] = {"E2", "A2", "D3", "G3", "B3", "E4"};
    for (int i = 0; i < 6; i++) {
        Serial.print("       Playing ");
        Serial.print(names[i]);
        Serial.print(" (");
        Serial.print(freqs[i], 2);
        Serial.println(" Hz)...");
        playTone(freqs[i], 700);
    }

    Serial.println();
    Serial.println("  Did all 6 tones play clearly with no distortion?");
    Serial.println("  If buzzing/distortion: LM386 R2/C4 component issue on module");
    Serial.println("  Fix: add 10 ohm resistor across speaker terminals");
    waitForSerial();

    pass("Audio output test complete");
}

/* ============================================================================
 * TEST 3: MICROPHONE INPUT (Pin 39 / A17)
 *
 * PASS condition: ADC reads varying values when you speak/tap near mic
 * FAIL condition: ADC stuck at 0, 4095, or 2048 with no variation
 *
 * If FAIL:
 *   - Check electret mic output wire is on pin 39 (not pin 14/A0)
 *   - Check mic has bias resistor (2.2k-10k to 3.3V) - should be on PCB
 *   - Check mic GND is connected to common ground
 * ========================================================================== */

void test_microphone() {
    printHeader("TEST 3: MICROPHONE INPUT (Pin 39 / A17)");

    analogReadResolution(12);
    analogReadAveraging(4);

    info("Reading microphone for 5 seconds. Make noise near the mic.");
    info("You should see ADC values varying between ~1800 and ~2300 at idle,");
    info("and swinging wider (toward 0 or 4095) when you speak loudly.");
    Serial.println();

    int min_val = 4095;
    int max_val = 0;
    int sample_count = 0;

    uint32_t start = millis();
    while (millis() - start < 5000) {
        int val = analogRead(MIC_PIN);
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
        sample_count++;

        if (sample_count % 500 == 0) {
            Serial.print("       ADC: ");
            Serial.print(val);
            Serial.print("  (min=");
            Serial.print(min_val);
            Serial.print(", max=");
            Serial.print(max_val);
            Serial.println(")");
        }
        delayMicroseconds(100);
    }

    Serial.println();
    int range = max_val - min_val;
    Serial.print("  ADC range observed: ");
    Serial.print(range);
    Serial.println(" counts");

    if (range < 10) {
        fail("ADC completely static - mic not connected or bias resistor missing");
        Serial.println("       Check: pin 39, bias resistor to 3.3V, mic GND");
    } else if (range < 50) {
        Serial.println("[WARN] Very small ADC range - mic may be connected but weak");
        Serial.println("       Confirm bias resistor value (should be 2.2k-10k)");
        Serial.println("       Signal should still improve tuning accuracy is limited");
    } else {
        pass("Microphone responding to sound");
    }
}

/* ============================================================================
 * TEST 4: STRING SELECTION BUTTONS (Pins 22, 3, 4, 5, 6, 9)
 *
 * PASS condition: Each button reads HIGH when pressed, LOW when released
 * FAIL condition: Always LOW (not connected or wrong pin)
 *                 Always HIGH (shorted to 3.3V)
 *
 * Buttons are ACTIVE HIGH - they connect the pin to 3.3V when pressed.
 * INPUT_PULLDOWN holds pin LOW at rest.
 *
 * If FAIL:
 *   - Verify button connects its pin to 3.3V (not GND) when pressed
 *   - Check button pin matches config.h exactly
 * ========================================================================== */

void test_buttons() {
    printHeader("TEST 4: STRING SELECTION BUTTONS");

    const uint8_t pins[]        = {STRING_1_PIN, STRING_2_PIN, STRING_3_PIN,
                                   STRING_4_PIN, STRING_5_PIN, STRING_6_PIN};
    const char*   string_names[] = {"String 1 (E4) - Pin 22",
                                    "String 2 (B3) - Pin 3",
                                    "String 3 (G3) - Pin 4",
                                    "String 4 (D3) - Pin 5",
                                    "String 5 (A2) - Pin 6",
                                    "String 6 (E2) - Pin 9"};
    const float freqs[]          = {329.63f, 246.94f, 196.0f,
                                    146.83f, 110.0f, 82.41f};

    for (int i = 0; i < 6; i++) {
        pinMode(pins[i], INPUT_PULLDOWN);   // Active HIGH - rest LOW, pressed HIGH
    }

    for (int i = 0; i < 6; i++) {
        Serial.println();
        Serial.print("  >>> PRESS AND HOLD: ");
        Serial.println(string_names[i]);
        Serial.println("      Active HIGH expectation: LOW at rest, HIGH when pressed, LOW when released");
        Serial.println("      Waiting 5 seconds...");
        bool low_at_rest = (digitalRead(pins[i]) == LOW);
        Serial.print("      Rest state before press: ");
        Serial.println(low_at_rest ? "LOW (good)" : "HIGH (unexpected)");

        bool detected = false;
        bool release_timeout = false;
        bool low_after_release = false;
        uint32_t start = millis();

        while (millis() - start < 5000) {
            if (digitalRead(pins[i]) == HIGH) {
                detected = true;
                Serial.print("      DETECTED HIGH on pin ");
                Serial.println(pins[i]);
                Serial.println("      Playing pure sine tone while held...");
                sine1.frequency(freqs[i]);
                sine1.amplitude(0.8f);

                // Wait for release, but don't block forever if wiring is stuck HIGH
                uint32_t release_start = millis();
                while (digitalRead(pins[i]) == HIGH) {
                    if (millis() - release_start > 2000) {
                        release_timeout = true;
                        Serial.println("      Release timeout (pin stayed HIGH >2s). Continuing...");
                        break;
                    }
                    delay(10);
                }
                sine1.amplitude(0.0f);
                if (!release_timeout) {
                    low_after_release = (digitalRead(pins[i]) == LOW);
                    Serial.println("      Released.");
                    Serial.print("      Release state: ");
                    Serial.println(low_after_release ? "LOW (good)" : "HIGH (unexpected)");
                }
                break;
            }
            delay(20);
        }

        if (!detected) {
            fail(string_names[i]);
            Serial.println("      ACTIVE HIGH fail: never reached HIGH while pressed");
            Serial.println("      Check button wiring to 3.3V and correct Teensy pin");
        } else if (release_timeout) {
            fail(string_names[i]);
            Serial.println("      ACTIVE HIGH fail: pin stayed HIGH and did not return LOW");
            Serial.println("      Check for short to 3.3V or stuck button");
        } else if (!low_at_rest || !low_after_release) {
            fail(string_names[i]);
            Serial.println("      ACTIVE HIGH fail: expected LOW before/after press");
            Serial.println("      Check pulldown behavior and wiring");
        } else {
            pass(string_names[i]);
            Serial.println("      ACTIVE HIGH verified: LOW -> HIGH -> LOW");
        }

        if (i < 5) {
            Serial.println("      Waiting 7 seconds before next string test...");
            delay(7000);
        }
    }
}

/* ============================================================================
 * TEST 5: MODE SWITCH (Pin 11)
 *
 * PASS condition: Reads HIGH in one position, LOW in other
 * FAIL condition: Always same value regardless of switch position
 *
 * If FAIL:
 *   - Check switch is wired between pin 11 and GND
 *   - Internal pull-up is enabled - switch should pull LOW when closed
 * ========================================================================== */

void test_mode_switch() {
    printHeader("TEST 5: MODE SWITCH (Pin 11)");

    pinMode(MODE_SWITCH_PIN, INPUT_PULLDOWN);  // Active HIGH - rest LOW, closed HIGH

    info("Reading mode switch for 8 seconds.");
    info("Toggle the switch between both positions.");
    Serial.println();

    bool saw_high = false;
    bool saw_low  = false;
    int  last_val = -1;

    uint32_t start = millis();
    while (millis() - start < 8000) {
        int val = digitalRead(MODE_SWITCH_PIN);
        if (val == HIGH) saw_high = true;
        if (val == LOW)  saw_low  = true;

        if (val != last_val) {
            Serial.print("      Switch state: ");
            Serial.println(val == HIGH ? "HIGH (Play Tone mode)" : "LOW  (Listen Only mode)");
            last_val = val;
        }
        delay(50);
    }

    Serial.println();
    if (saw_high && saw_low) {
        pass("Mode switch toggling correctly between HIGH and LOW");
    } else if (!saw_low) {
        fail("Switch never went LOW - check wire from pin 11 to switch, switch to GND");
    } else {
        fail("Switch never went HIGH - check internal pull-up is working");
    }
}

/* ============================================================================
 * TEST 1: POWER SUPPLY (Vin must be 5V)
 *
 * Requirement: PowerBoost output to Teensy Vin should be 5V.
 * Prefer validating with a multimeter: Vin-to-GND should be ~5.0V.
 * ========================================================================== */

void test_power() {
    printHeader("TEST 1: POWER SUPPLY");

    info("If you are reading this message, Teensy is powered and running.");
    info("Power requirement: Vin should be 5V from the PowerBoost.");
    info("If USB is powering Teensy, this does NOT verify PowerBoost 5V on Vin.");
    Serial.println();
    Serial.println("  Confirm with multimeter: Vin to GND is about 5.0V (target 4.8V-5.2V)");
    Serial.println("  Confirm: LiPo battery is connected to PowerBoost");
    Serial.println("  Confirm: Power switch is ON and PowerBoost LED indicates output");
    waitForSerial();

    pass("Power check complete (Vin should be 5V)");
}

/* ============================================================================
 * SETUP / MAIN
 * ========================================================================== */

void setup() {
    Serial.begin(115200);
    delay(2000);

    AudioMemory(20);
    sine1.amplitude(0.0f);

    Serial.println();
    Serial.println("========================================");
    Serial.println("  RPVI Guitar Tuner - Wiring Test");
    Serial.println("  Run this before uploading main code");
    Serial.println("========================================");
    Serial.println();
    Serial.println("This test checks every hardware connection.");
    Serial.println("Follow the prompts in this Serial Monitor.");
    Serial.println("Each test tells you exactly what to do.");
    Serial.println();
    Serial.println("Tests:");
    Serial.println("  1. Power supply");
    Serial.println("  2. Audio output (speaker)");
    Serial.println("  3. Microphone input");
    Serial.println("  4. All 6 string buttons");
    Serial.println("  5. Mode switch");
    Serial.println();

    waitForSerial();

    test_power();
    test_audio_output();
    test_microphone();
    test_buttons();
    test_mode_switch();

    /* ===== FINAL SUMMARY ===== */
    printHeader("WIRING TEST COMPLETE");
    Serial.println("Review [PASS] / [FAIL] / [WARN] results above.");
    Serial.println();
    Serial.println("All PASS? -> Upload main tuner firmware:");
    Serial.println("  pio run -e teensy41 -t upload");
    Serial.println();
    Serial.println("Any FAIL? -> Fix that wire before proceeding.");
    Serial.println("  Each FAIL message tells you exactly what to check.");
    printSeparator();

    // Final tone - two beeps = test complete
    playTone(880.0f, 200);
    delay(100);
    playTone(880.0f, 200);
}

void loop() {
    delay(1000);
}