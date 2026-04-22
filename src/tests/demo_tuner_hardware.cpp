/**
 * demo_tuner_hardware.cpp - RPVI Guitar Tuner
 *
 * UPLOAD:  pio run -e teensy41_demo -t upload
 * MONITOR: pio device monitor -e teensy41_demo  (115200 baud)
 *
 * BEHAVIOR: Reference tone + beep feedback for every string session.
 *
 * ON/OFF: Hardware only — PowerBoost EN pin to GND via rocker switch.
 */

#include <Arduino.h>
#include <AudioStream.h>
#include <synth_sine.h>
#include <output_mqs.h>
#include <math.h>
#include <mixer.h>

#define GUITAR_MIN_HZ  60.0f
#define GUITAR_MAX_HZ 400.0f

extern "C" {
    #include "audio_processing.h"
    #include "hardware_interface.h"
    #include "note_parser.h"
    #include "config.h"
}

/* ============================================================================
 * AUDIO OBJECTS
 * ========================================================================== */
AudioSynthWaveformSine  tone_gen;
AudioSynthWaveformSine  beep_gen;
AudioMixer4             mixer;
AudioOutputMQS          mqs_out;
AudioConnection         conn1(tone_gen, 0, mixer,   0);
AudioConnection         conn2(beep_gen, 0, mixer,   1);
AudioConnection         conn3(mixer,    0, mqs_out, 0);
AudioConnection         conn4(mixer,    0, mqs_out, 1);

/* ============================================================================
 * AUDIO HELPERS
 * ========================================================================== */
void playTone(float freq, uint32_t dur_ms) {
    tone_gen.frequency(freq);
    tone_gen.amplitude(1.0f);
    delay(dur_ms);
    tone_gen.amplitude(0.0f);
    delay(50);
}

void playBeep(float freq, uint32_t dur_ms) {
    beep_gen.frequency(freq);
    beep_gen.amplitude(1.0f);
    delay(dur_ms);
    beep_gen.amplitude(0.0f);
}

void playJingle() {
    playTone(523.25f, 200);
    delay(80);
    playTone(659.25f, 200);
    delay(80);
    playTone(783.99f, 500);
}

/* ============================================================================
 * MIC CAPTURE
 * ========================================================================== */
double captureMic() {
    int16_t samples[SAMPLE_SIZE];
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        samples[i] = (int16_t)analogRead(MICROPHONE_INPUT_PIN);
        delayMicroseconds(75);
    }
    long sum = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) sum += samples[i];
    int16_t dc = (int16_t)(sum / SAMPLE_SIZE);
    for (int i = 0; i < SAMPLE_SIZE; i++) samples[i] -= dc;
    return apply_fft(samples, SAMPLE_SIZE);
}

/* ============================================================================
 * STRING DEFINITIONS
 * ========================================================================== */
struct DemoString {
    int         button_id;
    const char* note;
    float       freq;
    const char* name;
};

static const DemoString STRINGS[6] = {
    {1, "E4", 329.63f, "High E"},
    {2, "B3", 246.94f, "B"},
    {3, "G3", 196.00f, "G"},
    {4, "D3", 146.83f, "D"},
    {5, "A2", 110.00f, "A"},
    {6, "E2",  82.41f, "Low E"}
};

/* ============================================================================
 * DISPLAY HELPERS
 * ========================================================================== */
void banner(const char* text) {
    Serial.println();
    Serial.println(F("======================================================"));
    Serial.print(F("  ")); Serial.println(text);
    Serial.println(F("======================================================"));
}

void section(const char* text) {
    Serial.println();
    Serial.print(F("  -- ")); Serial.println(text);
}

void printMenu() {
    Serial.println();
    Serial.println(F("  +----------------------------------------------------+"));
    Serial.println(F("  |          PRESS A BUTTON TO SELECT STRING           |"));
    Serial.println(F("  +----------------------------------------------------+"));
    Serial.println(F("  |  Button 1  (Pin 22)  ->  E4   329 Hz  High E       |"));
    Serial.println(F("  |  Button 2  (Pin  3)  ->  B3   247 Hz               |"));
    Serial.println(F("  |  Button 3  (Pin  4)  ->  G3   196 Hz               |"));
    Serial.println(F("  |  Button 4  (Pin  5)  ->  D3   147 Hz               |"));
    Serial.println(F("  |  Button 5  (Pin  6)  ->  A2   110 Hz               |"));
    Serial.println(F("  |  Button 6  (Pin  9)  ->  E2    82 Hz  Low E        |"));
    Serial.println(F("  +----------------------------------------------------+"));
    Serial.println();
}

/* ============================================================================
 * LISTEN FOR FREQUENCY
 * ========================================================================== */
double listenForFrequency(uint32_t timeout_ms) {
    uint32_t start = millis();
    int attempts = 0;

    while (millis() - start < timeout_ms) {
        double detected = captureMic();
        attempts++;

        if (detected > 0.0 &&
            detected >= GUITAR_MIN_HZ &&
            detected <= GUITAR_MAX_HZ) {
            Serial.print(F("     Detected: "));
            Serial.print(detected, 2);
            Serial.print(F(" Hz  (attempt "));
            Serial.print(attempts);
            Serial.println(F(")"));
            return detected;
        }

        if (attempts % 8 == 0) {
            Serial.print(F("     Listening... "));
            Serial.print((millis() - start) / 1000);
            Serial.println(F("s"));
        }
    }
    return 0.0;
}

/* ============================================================================
 * BEEP TIMING — based on Hz distance from target
 * ========================================================================== */
static void getBeepTiming(double hz_from_target,
                          uint32_t* gap_ms, uint32_t* dur_ms) {
    double d = fabs(hz_from_target);

    if      (d > 80.0) { *gap_ms =   50; *dur_ms =  50; }
    else if (d > 70.0) { *gap_ms =  100; *dur_ms =  60; }
    else if (d > 60.0) { *gap_ms =  300; *dur_ms =  70; }
    else if (d > 50.0) { *gap_ms =  500; *dur_ms =  80; }
    else if (d > 40.0) { *gap_ms =  700; *dur_ms = 100; }
    else if (d > 30.0) { *gap_ms =  900; *dur_ms = 120; }
    else if (d > 20.0) { *gap_ms = 1100; *dur_ms = 140; }
    else if (d > 10.0) { *gap_ms = 1300; *dur_ms = 160; }
    else if (d >  5.0) { *gap_ms = 1500; *dur_ms = 200; }
    else               { *gap_ms = 2000; *dur_ms = 300; }
}

/* ============================================================================
 * BEEP FEEDBACK LOOP
 * ========================================================================== */
void proximityBeepFeedback(float target_freq, double first_detected) {
    section("STATE: PROVIDING FEEDBACK");
    Serial.println(F("     HIGH beep (1100 Hz) = TUNE UP   (string is flat)"));
    Serial.println(F("     LOW  beep ( 600 Hz) = TUNE DOWN (string is sharp)"));
    Serial.println(F("     FAST beeps = far from tune"));
    Serial.println(F("     SLOW beeps = close to tune"));
    Serial.println(F("     Jingle     = IN TUNE!"));
    Serial.println();

    uint32_t session_start = millis();
    double current_detected = first_detected;
    int in_tune_count = 0;

    while (millis() - session_start < 90000) {

        double hz_diff  = current_detected - (double)target_freq;
        double cents    = 1200.0 * log2(current_detected / (double)target_freq);
        bool   is_sharp = (hz_diff > 0);
        float  bfreq    = is_sharp ? 600.0f : 1100.0f;

        uint32_t gap_ms, beep_dur;
        getBeepTiming(hz_diff, &gap_ms, &beep_dur);

        // In-tune: 6 Hz window, 5 consecutive readings
        if (fabs(hz_diff) < 6.0 && current_detected > 0.0) {
            in_tune_count++;
            if (in_tune_count >= 5) {
                Serial.println();
                banner("IN TUNE!");
                Serial.print(F("  Final offset: "));
                Serial.print(cents, 2);
                Serial.println(F(" cents"));
                playJingle();
                return;
            }
        } else {
            in_tune_count = 0;
        }

        Serial.print(F("  "));
        Serial.print(current_detected, 1);
        Serial.print(F(" Hz  |  "));
        Serial.print(hz_diff, 1);
        Serial.print(F(" Hz off  |  "));
        Serial.print(cents, 1);
        Serial.print(F(" c  |  "));
        Serial.print(is_sharp ? F("TUNE DOWN") : F("TUNE UP  "));
        Serial.print(F("  |  gap="));
        Serial.print(gap_ms);
        Serial.println(F("ms"));

        playBeep(bfreq, beep_dur);
        delay(800);

        double new_detected = captureMic();
        if (new_detected > 0.0 &&
            new_detected >= GUITAR_MIN_HZ &&
            new_detected <= GUITAR_MAX_HZ &&
            fabs(new_detected - current_detected) < 50.0) {
            if (fabs(new_detected - current_detected) > 5.0) {
                Serial.print(F("  [UPDATE] "));
                Serial.print(current_detected, 1);
                Serial.print(F(" -> "));
                Serial.print(new_detected, 1);
                Serial.println(F(" Hz"));
            }
            current_detected = new_detected;
        }

        if (gap_ms > 800) delay(gap_ms - 800);
    }

    Serial.println(F("     Session timed out. Press a button to try again."));
}

/* ============================================================================
 * FULL TUNING SESSION
 * ========================================================================== */
void runTuningSession(const DemoString& s) {
    char buf[64];
    snprintf(buf, sizeof(buf), "STRING %d  %s  %.0f Hz  %s",
             s.button_id, s.note, s.freq, s.name);
    banner(buf);

    // Play reference tone
    section("STATE: PLAYING REFERENCE TONE");
    Serial.print(F("     Target: "));
    Serial.print(s.note);
    Serial.print(F("  ("));
    Serial.print(s.freq, 2);
    Serial.println(F(" Hz)"));
    Serial.println(F("     Listen carefully — this is the note you are tuning to."));
    playTone(s.freq, 3000);
    delay(800);

    // Ready beep
    section("STATE: READY");
    Serial.println(F("     Ready — play your string near the mic now."));
    playBeep(1000.0f, 300);
    delay(300);

    // Listen
    section("STATE: LISTENING");
    Serial.print(F("     Listening for "));
    Serial.print(s.note);
    Serial.println(F("  (15 second window)"));

    double detected = listenForFrequency(15000);

    if (detected <= 0.0) {
        section("TIMEOUT — No signal detected");
        Serial.println(F("     Hold your sound source directly against the mic."));
        Serial.println(F("     Returning to string selection."));
        playBeep(300.0f, 800);
        return;
    }

    // Beep feedback
    proximityBeepFeedback(s.freq, detected);

    section("SESSION COMPLETE");
    Serial.println(F("     Press any button to tune another string."));
}

/* ============================================================================
 * SETUP
 * ========================================================================== */
void setup() {
    Serial.begin(115200);
    delay(2000);  // Short delay — works both on USB and in the box

    AudioMemory(20);
    mixer.gain(0, 1.0f);
    mixer.gain(1, 1.0f);
    mixer.gain(2, 0.0f);
    mixer.gain(3, 0.0f);
    tone_gen.amplitude(0.0f);
    beep_gen.amplitude(0.0f);
    analogReadResolution(12);
    analogReadAveraging(4);

    audio_processing_init();
    hardware_interface_init();

    banner("RPVI Guitar Tuner");
    Serial.println();
    Serial.println(F("  For blind and visually impaired users."));
    Serial.println(F("  All feedback is audio only."));
    Serial.println();
    Serial.println(F("  HOW TO USE:"));
    Serial.println(F("    1. Press a button to select a string"));
    Serial.println(F("    2. Listen to the reference tone"));
    Serial.println(F("    3. Play your string near the microphone"));
    Serial.println(F("    4. Follow the beeps:"));
    Serial.println(F("         HIGH = TUNE UP   (flat)"));
    Serial.println(F("         LOW  = TUNE DOWN (sharp)"));
    Serial.println(F("         FAST = far from tune"));
    Serial.println(F("         SLOW = close to tune"));
    Serial.println(F("         Jingle = IN TUNE!"));
    Serial.println();
    Serial.println(F("  Playing startup tones..."));

    // Two startup tones confirm the device is on and audio works
    playTone(440.0f, 400);
    delay(200);
    playTone(880.0f, 300);

    printMenu();
}

/* ============================================================================
 * LOOP
 * ========================================================================== */
void loop() {
    static uint32_t last_menu_print = 0;

    if (button_poll()) {
        button_event_t* ev = button_get_event();
        if (ev && ev->state == BUTTON_PRESSED) {
            int id = (int)ev->button_id;
            if (id >= 1 && id <= 6) {
                runTuningSession(STRINGS[id - 1]);
                printMenu();
                last_menu_print = millis();
            }
        }
    }

    if (millis() - last_menu_print > 10000) {
        printMenu();
        last_menu_print = millis();
    }

    delay(10);
}