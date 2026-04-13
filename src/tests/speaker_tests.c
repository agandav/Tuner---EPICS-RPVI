/*
 * speaker_tests.c - Shared tone tables for speaker-only tests
 *
 * The actual speaker playback lives in speaker_tests_runner.cpp so it can
 * use the same direct MQS sine-wave pattern as the known-good Mode 3 test.
 */

#include <stdint.h>

const float SPEAKER_TEST_REFERENCE_FREQS[] = {120.0f, 440.0f, 1200.0f};
const float SPEAKER_TEST_GUITAR_FREQS[] = {82.41f, 110.00f, 146.83f, 196.00f, 246.94f, 329.63f};
const float SPEAKER_TEST_ALERT_FREQS[] = {600.0f, 600.0f, 1100.0f, 1100.0f};
const float SPEAKER_TEST_CHIME_FREQS[] = {523.25f, 659.25f, 783.99f};

const uint32_t SPEAKER_TEST_REFERENCE_MS[] = {1500, 1500, 1500};
const uint32_t SPEAKER_TEST_GUITAR_MS[] = {1200, 1200, 1200, 1200, 1200, 1200};
const uint32_t SPEAKER_TEST_ALERT_MS[] = {600, 600, 600, 600};
const uint32_t SPEAKER_TEST_CHIME_MS[] = {500, 500, 1000};
