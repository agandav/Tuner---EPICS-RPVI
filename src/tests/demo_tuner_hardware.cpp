/**
 * demo_tuner_hardware.cpp - RPVI Guitar Tuner Client Demo
 *
 * UPLOAD:  pio run -e teensy41_demo -t upload
 * MONITOR: pio device monitor -e teensy41_demo  (115200 baud)
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
 * AUDIO
 * ========================================================================== */
AudioSynthWaveformSine  tone_gen;
AudioSynthWaveformSine  beep_gen;
AudioMixer4             mixer;
AudioOutputMQS          mqs_out;
AudioConnection         conn1(tone_gen, 0, mixer,   0);
AudioConnection         conn2(beep_gen, 0, mixer,   1);
AudioConnection         conn3(mixer,    0, mqs_out, 0);
AudioConnection         conn4(mixer,    0, mqs_out, 1);

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
    playTone(523.25f, 180);
    delay(60);
    playTone(659.25f, 180);
    delay(60);
    playTone(783.99f, 350);
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
/* ============================================================================
    * Guitar standard tuning frequencies:
 *   600 Hz / N:  300, 200, 150, 120, 100, 85
 *   1100 Hz / N: 550, 367, 275, 220, 183, 157, 138
 * ========================================================================== */

static const DemoString STRINGS[6] = {
    {1, "E4", 329.63f, "High E  (thinnest)"},
    {2, "B3", 246.94f, "B"},
    {3, "G3", 196.00f, "G"},
    {4, "D3", 146.83f, "D"},
    {5, "A2", 110.00f, "A"},
    {6, "E2",  82.41f, "Low E   (thickest)"}
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

void status(const char* text) {
    Serial.print(F("     ")); Serial.println(text);
}

void printStringMenu() {
    Serial.println();
    Serial.println(F("  +----------------------------------------------------+"));
    Serial.println(F("  |          PRESS A BUTTON TO SELECT STRING           |"));
    Serial.println(F("  +----------------------------------------------------+"));
    Serial.println(F("  |  Button 1  (Pin 22)  ->  E4  329 Hz  High E        |"));
    Serial.println(F("  |  Button 2  (Pin  3)  ->  B3  247 Hz                |"));
    Serial.println(F("  |  Button 3  (Pin  4)  ->  G3  196 Hz                |"));
    Serial.println(F("  |  Button 4  (Pin  5)  ->  D3  147 Hz                |"));
    Serial.println(F("  |  Button 5  (Pin  6)  ->  A2  110 Hz                |"));
    Serial.println(F("  |  Button 6  (Pin  9)  ->  E2   82 Hz  Low E         |"));
    Serial.println(F("  +----------------------------------------------------+"));
    Serial.println();
    Serial.println(F("  TIP: Start your phone ~2-3 semitones flat then slide up."));
}

/* ============================================================================
 * LISTEN FOR FREQUENCY
 * ========================================================================== */
double listenForFrequency(float target_freq, uint32_t timeout_ms,
                          float min_hz, float max_hz) {
    uint32_t start = millis();
    int attempts = 0;

    while (millis() - start < timeout_ms) {
        double detected = captureMic();
        attempts++;

        if (detected > 0.0) {
            if (detected >= min_hz && detected <= max_hz) {
                Serial.print(F("     Detected: "));
                Serial.print(detected, 2);
                Serial.print(F(" Hz  (attempt "));
                Serial.print(attempts);
                Serial.println(F(")"));
                return detected;
            } else {
                Serial.print(F("     Rejected: "));
                Serial.print(detected, 2);
                Serial.print(F(" Hz  (need "));
                Serial.print(min_hz, 0);
                Serial.print(F("-"));
                Serial.print(max_hz, 0);
                Serial.println(F(" Hz)"));
            }
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
 * PROXIMITY BEEP FEEDBACK
 * ========================================================================== */
void proximityBeepFeedback(float target_freq, double first_detected,
                           float min_hz, float max_hz) {
    section("STATE: PROVIDING FEEDBACK");
    status("Slide your frequency toward the target note.");
    status("HIGH beep (1100 Hz) = tune UP   (you are flat)");
    status("LOW  beep ( 600 Hz) = tune DOWN (you are sharp)");
    status("FAST beeps = far from tune");
    status("SLOW beeps = close to tune");
    Serial.println();

    uint32_t session_start = millis();
    double current_detected = first_detected;
    int in_tune_count = 0;

    while (millis() - session_start < 60000) {
        double cents     = 1200.0 * log2(current_detected / (double)target_freq);
        double abs_cents = fabs(cents);

        // In-tune check: within 15 cents AND within 12 Hz of target
        if (abs_cents < 50.0 &&
            fabs(current_detected - (double)target_freq) < 25.0 &&
            current_detected > 0.0) {
            in_tune_count++;
            if (in_tune_count >= 3) {
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

        bool is_sharp = (cents > 0);
        float beep_freq = is_sharp ? 600.0f : 1100.0f;
        double clamped  = (abs_cents > 100.0) ? 100.0
                        : (abs_cents <   5.0) ?   5.0
                        : abs_cents;
        uint32_t gap_ms   = (uint32_t)(100.0 + ((100.0 - clamped) / 95.0) * 700.0);
        uint32_t beep_dur = (uint32_t)(150.0 + ((100.0 - clamped) / 95.0) * 250.0);

        Serial.print(F("  "));
        Serial.print(current_detected, 1);
        Serial.print(F(" Hz  |  "));
        Serial.print(cents, 1);
        Serial.print(F(" c  |  "));
        Serial.print(is_sharp ? F("TUNE DOWN") : F("TUNE UP  "));
        Serial.print(F("  |  gap="));
        Serial.print(gap_ms);
        Serial.println(F("ms"));

        // Play beep then wait for echo to die before sampling
        playBeep(beep_freq, beep_dur);
        delay(400);  // echo decay — critical for avoiding beep subharmonics

        // Re-sample mic
        double new_detected = captureMic();
        if (new_detected > 0.0 &&
            new_detected >= min_hz &&
            new_detected <= max_hz) {
            if (fabs(new_detected - current_detected) > 5.0) {
                Serial.print(F("  [UPDATE] "));
                Serial.print(current_detected, 1);
                Serial.print(F(" -> "));
                Serial.print(new_detected, 1);
                Serial.println(F(" Hz"));
            }
            current_detected = new_detected;
        }

        // Remainder of gap after the 400ms echo decay
        if (gap_ms > 400) delay(gap_ms - 400);
    }

    status("Session timed out. Press a button to try again.");
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
    Serial.print(F("     Target note: "));
    Serial.print(s.note);
    Serial.print(F("  ("));
    Serial.print(s.freq, 2);
    Serial.println(F(" Hz)"));
    Serial.println(F("     Listen carefully — this is what you are tuning to."));
    playTone(s.freq, 2000);
    delay(1200);

    // Ready beep
    section("STATE: READY");
    Serial.print(F("     Valid range: "));
    Serial.print(GUITAR_MIN_HZ, 0);
    Serial.print(F(" - "));
    Serial.print(GUITAR_MAX_HZ, 0);
    Serial.println(F(" Hz"));
    Serial.println(F("     Ready beep — play your string near the mic now."));
    playBeep(1000.0f, 200);
    delay(200);

    // Listen
    section("STATE: LISTENING");
    Serial.print(F("     Listening for "));
    Serial.print(s.note);
    Serial.print(F(" near "));
    Serial.print(s.freq, 0);
    Serial.println(F(" Hz  (15 second window)"));

    double detected = listenForFrequency(s.freq, 15000, GUITAR_MIN_HZ, GUITAR_MAX_HZ);

    if (detected <= 0.0) {
        section("TIMEOUT — No valid signal");
        Serial.print(F("     Need frequency between "));
        Serial.print(GUITAR_MIN_HZ, 0);
        Serial.print(F(" and "));
        Serial.print(GUITAR_MAX_HZ, 0);
        Serial.println(F(" Hz"));
        status("Returning to string selection.");
        playBeep(300.0f, 600);
        return;
    }

    double cents = 1200.0 * log2(detected / (double)s.freq);

    // Playback detected note
    section("STATE: PLAYING BACK WHAT WAS DETECTED");
    Serial.print(F("     Detected:  ")); Serial.print(detected, 2); Serial.println(F(" Hz"));
    Serial.print(F("     Offset:    ")); Serial.print(cents, 1);    Serial.println(F(" cents"));
    Serial.print(F("     Direction: "));
    Serial.println(cents > 0 ? F("TUNE DOWN (too sharp)") : F("TUNE UP (too flat)"));
    Serial.println(F("     Playing back what was detected..."));
    playTone((float)detected, 1500);
    delay(400);

    // Playback target
    section("STATE: PLAYING TARGET NOTE");
    Serial.print(F("     Playing target: "));
    Serial.print(s.note);
    Serial.print(F("  ("));
    Serial.print(s.freq, 2);
    Serial.println(F(" Hz)  — aim for this."));
    playTone(s.freq, 1500);
    delay(400);

    // Feedback
    proximityBeepFeedback(s.freq, detected, GUITAR_MIN_HZ, GUITAR_MAX_HZ);

    section("SESSION COMPLETE");
    status("Press any button to tune another string.");
}

/* ============================================================================
 * SETUP
 * ========================================================================== */
void setup() {
    Serial.begin(115200);
    delay(2000);

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

    banner("RPVI Guitar Tuner  -  Client Demo");
    Serial.println();
    Serial.println(F("  Designed for blind and visually impaired users."));
    Serial.println(F("  Tunes a guitar using only audio feedback."));
    Serial.println();
    Serial.println(F("  HOW TO USE:"));
    Serial.println(F("    1. Press a button to select which string to tune"));
    Serial.println(F("    2. Listen to the reference tone (target pitch)"));
    Serial.println(F("    3. Play your string near the microphone"));
    Serial.println(F("    4. Device plays back what it heard"));
    Serial.println(F("    5. Device plays the target note again"));
    Serial.println(F("    6. Follow the beep feedback:"));
    Serial.println(F("         HIGH beep (1100 Hz) = TUNE UP   (too flat)"));
    Serial.println(F("         LOW  beep  (600 Hz) = TUNE DOWN (too sharp)"));
    Serial.println(F("         FAST beeps = far from tune"));
    Serial.println(F("         SLOW beeps = close to tune"));
    Serial.println(F("         Jingle     = IN TUNE!"));
    Serial.println();
    Serial.println(F("  Playing startup tone..."));
    playTone(440.0f, 500);
    delay(300);
    playTone(880.0f, 300);

    printStringMenu();
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
                printStringMenu();
                last_menu_print = millis();
            }
        }
    }

    // Reprint menu every 10 seconds while idle so it's always visible
    if (millis() - last_menu_print > 10000) {
        printStringMenu();
        last_menu_print = millis();
    }

    delay(10);
}