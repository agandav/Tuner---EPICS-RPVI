/**
 * teensy_audio_io.cpp - Hybrid Audio I/O Implementation for Teensy 4.1
 * 
 * Implements BOTH:
 * - Synthesized tones/beeps (for pitch reference and real-time feedback)
 * - SD card audio playback (for accessibility - spoken string names, directions)
 * 
 * This gives the best of both worlds:
 * - Simple tone generation without needing audio files
 * - Rich accessibility with spoken feedback
 */

#include "teensy_audio_io.h"
#include <Arduino.h>
#include <Audio.h>
#ifndef NO_SD_CARD
#include <SD.h>
#endif
#include <SPI.h>
#include <math.h>

/* ============================================================================
 * AUDIO OBJECTS - Teensy Audio Library
 * ========================================================================== */

/* Audio Input Chain: Microphone -> ADC -> FFT Analysis */
AudioInputI2S            i2s_input;       /* I2S microphone input */
AudioAnalyzeFFT256       fft256;          /* 256-point FFT for frequency detection */

/* Audio Output Chain: Multiple sources mixed together */
AudioSynthWaveformSine   sineWave;        /* Sine wave generator for tones */
#ifndef NO_SD_CARD
AudioPlaySdWav           playWav;         /* WAV file player from SD card */
#endif
AudioMixer4              mixer;           /* Mix sine wave and WAV playback */
AudioOutputI2S           i2s_output;      /* I2S output to speaker/headphone */
AudioControlSGTL5000     audioShield;     /* SGTL5000 audio codec control */

/* Audio Connections */
AudioConnection          patchCord1(i2s_input, 0, fft256, 0);        /* Input -> FFT */
AudioConnection          patchCord2(sineWave, 0, mixer, 0);          /* Tone -> Mixer Ch0 */
#ifndef NO_SD_CARD
AudioConnection          patchCord3(playWav, 0, mixer, 1);           /* WAV Left -> Mixer Ch1 */
AudioConnection          patchCord4(playWav, 1, mixer, 2);           /* WAV Right -> Mixer Ch2 */
#endif
AudioConnection          patchCord5(mixer, 0, i2s_output, 0);        /* Mixer -> Left Out */
AudioConnection          patchCord6(mixer, 0, i2s_output, 1);        /* Mixer -> Right Out */

/* ============================================================================
 * STATE VARIABLES
 * ========================================================================== */

static bool audio_initialized = false;
static bool sd_card_available = false;
static float current_volume = 0.7f;
static bool tone_playing = false;
static uint32_t tone_start_time = 0;
static uint32_t tone_duration_ms = 0;

/* ============================================================================
 * INITIALIZATION
 * ========================================================================== */

/**
 * Initialize Teensy audio system
 * Sets up audio codec, SD card, allocates memory, configures I/O
 */
teensy_audio_error_t init_audio_system(void) {
    Serial.println("Initializing Teensy Audio System...");
    
    /* Allocate audio memory - 40 blocks for audio processing + SD playback */
    AudioMemory(40);
    
#ifndef NO_SD_CARD
    /* Initialize SD card */
    SPI.setMOSI(11);  /* Teensy 4.1 SPI pins */
    SPI.setSCK(13);
    
    if (SD.begin(BUILTIN_SDCARD)) {
        sd_card_available = true;
        Serial.println("  - SD card initialized successfully");
        Serial.println("  - Audio files available for playback");
    } else {
        sd_card_available = false;
        Serial.println("  - WARNING: SD card not found");
        Serial.println("  - Will use synthesized audio only");
    }
#else
    sd_card_available = false;
    Serial.println("  - SD card disabled (NO_SD_CARD)");
    Serial.println("  - Will use synthesized audio only");
#endif
    
    /* Enable and configure SGTL5000 audio codec */
    if (!audioShield.enable()) {
        Serial.println("ERROR: Audio shield not responding!");
        return TEENSY_AUDIO_ERROR;
    }
    
    /* Set volume (0.0 - 1.0) */
    audioShield.volume(current_volume);
    
    /* Configure input - use MIC for onboard microphone */
    audioShield.inputSelect(AUDIO_INPUT_MIC);
    audioShield.micGain(36);  /* Microphone gain: 0-63 dB */
    
    /* Configure FFT for frequency detection */
    fft256.windowFunction(AudioWindowHanning256);
    
    /* Configure mixer - balance between synthesized and SD audio */
    mixer.gain(0, 1.0);  /* Channel 0: Sine wave - full volume */
    mixer.gain(1, 1.0);  /* Channel 1: WAV left - full volume */
    mixer.gain(2, 1.0);  /* Channel 2: WAV right - full volume */
    mixer.gain(3, 0.0);  /* Channel 3: unused */
    
    /* Initialize sine wave generator */
    sineWave.amplitude(0.0);  /* Start silent */
    sineWave.frequency(440);   /* Default A4 */
    
    audio_initialized = true;
    Serial.println("Audio system initialized successfully");
    Serial.println("  - Microphone input: ENABLED");
    Serial.println("  - Tone generator: READY");
    Serial.println("  - FFT analysis: ACTIVE");
    Serial.println("  - SD card playback: " + String(sd_card_available ? "AVAILABLE" : "UNAVAILABLE"));
    
    return TEENSY_AUDIO_OK;
}

/* ============================================================================
 * MICROPHONE INPUT - Audio Capture & Frequency Detection
 * ========================================================================== */

/**
 * Capture audio samples from microphone
 * Note: For this implementation, we use FFT directly rather than buffering samples
 */
int capture_audio_samples(int16_t* buffer, int max_samples) {
    if (!audio_initialized || !buffer) {
        return 0;
    }
    
    /* Signal that FFT data is available */
    if (fft256.available()) {
        return max_samples;  /* Indicate successful capture */
    }
    
    return 0;  /* No samples available yet */
}

/**
 * Read frequency from microphone using FFT
 * This is the main frequency detection function
 */
double read_frequency_from_microphone(const int16_t* samples, int num_samples) {
    if (!audio_initialized) {
        return 0.0;
    }
    
    if (!fft256.available()) {
        return 0.0;
    }
    
    /* Find peak frequency bin */
    float max_magnitude = 0.0f;
    int peak_bin = 0;
    
    /* FFT256 produces 128 bins (0 to 127)
     * Each bin: (44100 / 256) = 172.27 Hz per bin
     * Guitar range (82-330 Hz) = bins 1-2
     */
    
    for (int i = 1; i < 128; i++) {  /* Skip DC bin (0) */
        float magnitude = fft256.read(i);
        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
            peak_bin = i;
        }
    }
    
    /* Check if signal is strong enough */
    if (max_magnitude < 0.01) {
        return 0.0;
    }
    
    /* Convert bin to frequency */
    float bin_width = 44100.0f / 256.0f;
    double frequency = peak_bin * bin_width;
    
    return frequency;
}

/* ============================================================================
 * SYNTHESIZED AUDIO - Tone & Beep Generation
 * ========================================================================== */

/**
 * Play a synthesized tone at specified frequency
 * Used for "play tone" mode reference pitch
 */
void play_tone(double frequency, uint32_t duration_ms) {
    if (!audio_initialized) {
        Serial.println("ERROR: Audio not initialized");
        return;
    }
    
    Serial.print("Playing synthesized tone: ");
    Serial.print(frequency, 2);
    Serial.print(" Hz for ");
    Serial.print(duration_ms);
    Serial.println(" ms");
    
    sineWave.frequency(frequency);
    sineWave.amplitude(0.5 * current_volume);
    
    tone_playing = true;
    tone_start_time = millis();
    tone_duration_ms = duration_ms;
}

/**
 * Play a short "ready" beep (synthesized)
 */
void play_ready_beep(void) {
    if (!audio_initialized) {
        return;
    }
    
    Serial.println("Playing ready beep");
    play_tone(1000.0, 200);  /* 1000 Hz for 200ms */
}

/**
 * Generate a beep for dynamic feedback (synthesized)
 */
void play_beep(uint32_t duration_ms) {
    if (!audio_initialized) {
        return;
    }
    
    sineWave.frequency(800.0);
    sineWave.amplitude(0.3 * current_volume);
    
    tone_playing = true;
    tone_start_time = millis();
    tone_duration_ms = duration_ms;
}

/**
 * Update tone playback timing
 * Call this in main loop to auto-stop tones
 */
void update_tone_playback(void) {
    if (tone_playing) {
        uint32_t elapsed = millis() - tone_start_time;
        if (elapsed >= tone_duration_ms) {
            sineWave.amplitude(0.0);
            tone_playing = false;
        }
    }
}

/* ============================================================================
 * SD CARD AUDIO - Voice File Playback for Accessibility
 * ========================================================================== */

/**
 * Play a WAV file from SD card
 * Used for accessibility: spoken string names, directions, cent values
 * 
 * Example files on SD card:
 *   /AUDIO/STRING_E.wav - "String E"
 *   /AUDIO/STRING_A.wav - "String A"
 *   /AUDIO/TUNE_UP.wav  - "Tune up"
 *   /AUDIO/TUNE_DOWN.wav - "Tune down"
 *   /AUDIO/IN_TUNE.wav   - "In tune"
 *   /AUDIO/10_CENTS.wav  - "Ten cents"
 */
teensy_audio_error_t play_audio_file_from_sd(const char* filename) {
#ifdef NO_SD_CARD
    Serial.println("ERROR: SD card support disabled (NO_SD_CARD)");
    return TEENSY_AUDIO_NO_SD;
#else
    if (!audio_initialized) {
        Serial.println("ERROR: Audio not initialized");
        return TEENSY_AUDIO_ERROR;
    }
    
    if (!sd_card_available) {
        Serial.print("WARNING: SD card not available, cannot play: ");
        Serial.println(filename);
        return TEENSY_AUDIO_NO_SD;
    }
    
    /* Stop any currently playing audio */
    if (playWav.isPlaying()) {
        playWav.stop();
        delay(10);
    }
    
    /* Start playing new file */
    if (!playWav.play(filename)) {
        Serial.print("ERROR: Could not play file: ");
        Serial.println(filename);
        return TEENSY_AUDIO_FILE_ERROR;
    }
    
    Serial.print("Playing SD audio: ");
    Serial.println(filename);
    
    return TEENSY_AUDIO_OK;
#endif
}

/**
 * Check if SD card audio is currently playing
 */
bool is_sd_audio_playing(void) {
#ifdef NO_SD_CARD
    return false;
#else
    return sd_card_available && playWav.isPlaying();
#endif
}

/**
 * Stop SD card audio playback
 */
void stop_sd_audio(void) {
#ifndef NO_SD_CARD
    if (playWav.isPlaying()) {
        playWav.stop();
        Serial.println("SD audio playback stopped");
    }
#endif
}

/* ============================================================================
 * UNIFIED PLAYBACK CONTROL
 * ========================================================================== */

/**
 * Stop all audio playback (both synthesized and SD)
 */
void stop_audio_playback(void) {
    sineWave.amplitude(0.0);
    tone_playing = false;
    
#ifndef NO_SD_CARD
    if (playWav.isPlaying()) {
        playWav.stop();
    }
#endif
    
    Serial.println("All audio playback stopped");
}

/**
 * Check if any audio is currently playing
 */
bool is_audio_playing(void) {
#ifdef NO_SD_CARD
    return tone_playing;
#else
    return tone_playing || (sd_card_available && playWav.isPlaying());
#endif
}

/* ============================================================================
 * VOLUME CONTROL
 * ========================================================================== */

void set_volume(float vol) {
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    
    current_volume = vol;
    
    if (audio_initialized) {
        audioShield.volume(vol);
        Serial.print("Volume set to: ");
        Serial.println(vol, 2);
    }
}

float get_volume(void) {
    return current_volume;
}

/* ============================================================================
 * LEGACY COMPATIBILITY FUNCTIONS
 * ========================================================================== */

teensy_audio_error_t open_audio_file(teensy_audio_stream_t* stream, const char* filename) {
    /* Not used in this implementation - files are played directly */
    return TEENSY_AUDIO_ERROR;
}

teensy_audio_error_t read_audio_block(teensy_audio_stream_t* stream, float* output) {
    return TEENSY_AUDIO_ERROR;
}

void close_audio_file(teensy_audio_stream_t* stream) {
    /* Not used */
}

void get_fft_data(float* fft_output, int num_bins) {
    if (!fft_output || !audio_initialized) {
        return;
    }
    
    if (fft256.available()) {
        int max_bins = (num_bins < 128) ? num_bins : 128;
        for (int i = 0; i < max_bins; i++) {
            fft_output[i] = fft256.read(i);
        }
        for (int i = max_bins; i < num_bins; i++) {
            fft_output[i] = 0.0f;
        }
    }
}

void process_audio_realtime(void) {
    update_tone_playback();
}

void list_audio_files(void) {
#ifdef NO_SD_CARD
    Serial.println("SD card support disabled (NO_SD_CARD)");
#else
    if (!sd_card_available) {
        Serial.println("SD card not available");
        return;
    }
    
    Serial.println("\nAudio files on SD card:");
    
    File root = SD.open("/AUDIO/");
    if (!root) {
        Serial.println("  Could not open /AUDIO/ directory");
        return;
    }
    
    while (true) {
        File entry = root.openNextFile();
        if (!entry) break;
        
        if (!entry.isDirectory()) {
            String filename = entry.name();
            if (filename.endsWith(".wav") || filename.endsWith(".WAV")) {
                Serial.print("  - ");
                Serial.print(filename);
                Serial.print(" (");
                Serial.print(entry.size());
                Serial.println(" bytes)");
            }
        }
        entry.close();
    }
    root.close();
    Serial.println();
#endif
}

/* ============================================================================
 * DIAGNOSTIC FUNCTIONS
 * ========================================================================== */

void print_audio_status(void) {
    Serial.println("\n=== Audio System Status ===");
    Serial.print("Initialized: ");
    Serial.println(audio_initialized ? "YES" : "NO");
    Serial.print("SD Card: ");
    Serial.println(sd_card_available ? "AVAILABLE" : "NOT AVAILABLE");
    Serial.print("Volume: ");
    Serial.println(current_volume, 2);
    Serial.print("Tone playing: ");
    Serial.println(tone_playing ? "YES" : "NO");
    Serial.print("SD audio playing: ");
    Serial.println(is_sd_audio_playing() ? "YES" : "NO");
    Serial.print("FFT available: ");
    Serial.println(fft256.available() ? "YES" : "NO");
    Serial.print("CPU Usage: ");
    Serial.print(AudioProcessorUsage(), 1);
    Serial.println("%");
    Serial.print("Memory Usage: ");
    Serial.print(AudioMemoryUsage());
    Serial.println(" blocks");
    Serial.println("==========================\n");
}

/* ============================================================================
 * SD CARD STATUS
 * ========================================================================== */

bool is_sd_card_available(void) {
    return sd_card_available;
}