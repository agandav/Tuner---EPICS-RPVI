#include <Arduino.h>

extern "C" {
#include "audio_processing.h"
#include "config.h"
}

static int16_t mic_samples[SAMPLE_SIZE];

static const char* classifyFrame(int minRaw, int maxRaw, int rangeRaw, int centerOffset) {
    bool clipped = (minRaw <= 5 || maxRaw >= 4090);
    bool centeredOK = (abs(centerOffset) <= 2000);

    if (clipped) {
        return "rejected: clipped";
    }

    if (!centeredOK) {
        return "rejected: centering off";
    }

    if (rangeRaw < MIN_AMPLITUDE) {
        return "rejected: low swing";
    }

    return "accepted";
}

static void captureMicFrame(int* outMinRaw, int* outMaxRaw, int* outRangeRaw,
                            int* outMinCentered, int* outMaxCentered) {
    int minRaw = 4095;
    int maxRaw = 0;

    /* Capture raw samples and find min/max */
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        int raw = analogRead(MICROPHONE_INPUT_PIN);
        if (raw < minRaw) minRaw = raw;
        if (raw > maxRaw) maxRaw = raw;
        mic_samples[i] = (int16_t)raw;  /* Store raw for now */
        delayMicroseconds(75);
    }

    /* Calculate the ACTUAL DC offset of THIS frame (not hardcoded 2048) */
    long sum = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) sum += mic_samples[i];
    int16_t actual_center = (int16_t)(sum / SAMPLE_SIZE);

    /* Convert to centered and compute centered min/max */
    int minCentered = 32767;
    int maxCentered = -32768;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        int centered = (int)mic_samples[i] - actual_center;
        mic_samples[i] = (int16_t)centered;
        if (centered < minCentered) minCentered = centered;
        if (centered > maxCentered) maxCentered = centered;
    }

    if (outMinRaw) *outMinRaw = minRaw;
    if (outMaxRaw) *outMaxRaw = maxRaw;
    if (outRangeRaw) *outRangeRaw = maxRaw - minRaw;
    if (outMinCentered) *outMinCentered = minCentered;
    if (outMaxCentered) *outMaxCentered = maxCentered;
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    analogReadResolution(12);
    analogReadAveraging(4);
    audio_processing_init();

    Serial.println();
    Serial.println("========================================");
    Serial.println("LIVE MICROPHONE DIAGNOSTIC");
    Serial.println("Pin 39 (A17) raw ADC + FFT frequency");
    Serial.println("========================================");
    Serial.print("MIN_AMPLITUDE threshold: ");
    Serial.println(MIN_AMPLITUDE);
}

void loop() {
    int minRaw = 0;
    int maxRaw = 0;
    int rangeRaw = 0;
    int minCentered = 0;
    int maxCentered = 0;

    captureMicFrame(&minRaw, &maxRaw, &rangeRaw, &minCentered, &maxCentered);
    double detectedHz = apply_fft(mic_samples, SAMPLE_SIZE);
    bool clipped = (minRaw <= 5 || maxRaw >= 4090);
    int centerOffset = ((minRaw + maxRaw) / 2) - ADC_CENTER_VALUE;
    bool centeredOK = (abs(centerOffset) <= 2000);
    const char* frameStatus = classifyFrame(minRaw, maxRaw, rangeRaw, centerOffset);

    Serial.print("ADC min=");
    Serial.print(minRaw);
    Serial.print(" max=");
    Serial.print(maxRaw);
    Serial.print(" range=");
    Serial.print(rangeRaw);
    Serial.print(" centered[min=");
    Serial.print(minCentered);
    Serial.print(", max=");
    Serial.print(maxCentered);
    Serial.print("] offset=");
    Serial.print(centerOffset);
    if (clipped) {
        Serial.print(" [CLIPPED]");
    }
    if (centeredOK) {
        Serial.print(" [CENTERED OK]");
    } else {
        Serial.print(" [CENTERING OFF]");
    }
    Serial.print(" | FRAME=");
    Serial.print(frameStatus);
    Serial.print(" | FFT=");

    if (detectedHz > 0.0) {
        Serial.print(detectedHz, 2);
        Serial.println(" Hz");
    } else {
        Serial.println("no signal");
    }

    delay(200);
}
