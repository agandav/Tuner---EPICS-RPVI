#include <Arduino.h>
#include <AudioStream.h>
#include <synth_sine.h>
#include <output_mqs.h>

extern "C" {
#include <stdint.h>

extern const float SPEAKER_TEST_REFERENCE_FREQS[];
extern const float SPEAKER_TEST_GUITAR_FREQS[];
extern const float SPEAKER_TEST_ALERT_FREQS[];
extern const float SPEAKER_TEST_CHIME_FREQS[];
extern const uint32_t SPEAKER_TEST_REFERENCE_MS[];
extern const uint32_t SPEAKER_TEST_GUITAR_MS[];
extern const uint32_t SPEAKER_TEST_ALERT_MS[];
extern const uint32_t SPEAKER_TEST_CHIME_MS[];
}

#define SPEAKER_TEST_REFERENCE_COUNT 3
#define SPEAKER_TEST_GUITAR_COUNT 6
#define SPEAKER_TEST_ALERT_COUNT 4
#define SPEAKER_TEST_CHIME_COUNT 3

/* Direct MQS output, matching the known-good Mode 3 pattern */
AudioSynthWaveformSine   speaker_sine;
AudioOutputMQS           speaker_mqs;
AudioConnection          speaker_patch1(speaker_sine, 0, speaker_mqs, 0);
AudioConnection          speaker_patch2(speaker_sine, 0, speaker_mqs, 1);

static void playTone(float frequency, uint32_t duration_ms, float amplitude) {
    speaker_sine.frequency(frequency);
    speaker_sine.amplitude(amplitude);
    delay(duration_ms);
    speaker_sine.amplitude(0.0f);
}

static void playReferenceTone(float frequency, uint32_t duration_ms) {
    Serial.print("[SPEAKER] Reference tone: ");
    Serial.print(frequency, 2);
    Serial.println(" Hz");
    playTone(frequency, duration_ms, 1.0f);
    delay(150);
}

static void playStringTone(float frequency, uint32_t duration_ms) {
    Serial.print("[SPEAKER] Guitar tone: ");
    Serial.print(frequency, 2);
    Serial.println(" Hz");
    playTone(frequency, duration_ms, 1.0f);
    delay(150);
}

static void playAlertTone(float frequency, uint32_t duration_ms) {
    Serial.print("[SPEAKER] Alert beep: ");
    Serial.print(frequency, 0);
    Serial.println(" Hz");
    playTone(frequency, duration_ms, 1.0f);
    delay(80);
}

static void playChimeTone(float frequency, uint32_t duration_ms) {
    Serial.print("[SPEAKER] Chime tone: ");
    Serial.print(frequency, 2);
    Serial.println(" Hz");
    playTone(frequency, duration_ms, 1.0f);
    delay(50);
}

static void runReferenceSweep() {
    Serial.println();
    Serial.println("-- REFERENCE SWEEP --");
    for (int i = 0; i < SPEAKER_TEST_REFERENCE_COUNT; i++) {
        playReferenceTone(SPEAKER_TEST_REFERENCE_FREQS[i], SPEAKER_TEST_REFERENCE_MS[i]);
    }
}

static void runGuitarSweep() {
    Serial.println();
    Serial.println("-- GUITAR STRING SWEEP --");
    for (int i = 0; i < SPEAKER_TEST_GUITAR_COUNT; i++) {
        playStringTone(SPEAKER_TEST_GUITAR_FREQS[i], SPEAKER_TEST_GUITAR_MS[i]);
    }
}

static void runAlertSweep() {
    Serial.println();
    Serial.println("-- ALERT BEEP SWEEP --");
    for (int i = 0; i < SPEAKER_TEST_ALERT_COUNT; i++) {
        playAlertTone(SPEAKER_TEST_ALERT_FREQS[i], SPEAKER_TEST_ALERT_MS[i]);
    }
}

static void runChimeSweep() {
    Serial.println();
    Serial.println("-- SUCCESS CHIME --");
    for (int i = 0; i < SPEAKER_TEST_CHIME_COUNT; i++) {
        playChimeTone(SPEAKER_TEST_CHIME_FREQS[i], SPEAKER_TEST_CHIME_MS[i]);
    }
}

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && millis() - start < 5000) {
        delay(10);
    }
    delay(500);

    AudioMemory(20);
    speaker_sine.amplitude(0.0f);

    Serial.println();
    Serial.println("========================================");
    Serial.println("SPEAKER-ONLY TEST SUITE");
    Serial.println("Direct MQS sine output at max amplitude");
    Serial.println("========================================");
    Serial.println("Pin 10 MQS -> LM386 IN -> Speaker");
    Serial.println("No mic or buttons required");
    Serial.println();

    runReferenceSweep();
    runGuitarSweep();
    runAlertSweep();
    runChimeSweep();

    Serial.println();
    Serial.println("[SPEAKER TEST] Completed blocks: 4");
    Serial.println("[SPEAKER TEST] Done.");
}

void loop() {
    delay(1000);
}
