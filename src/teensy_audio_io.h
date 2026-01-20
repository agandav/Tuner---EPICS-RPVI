/**
 * teensy_audio_io.h - Audio I/O Interface for Teensy 4.1
 * 
 * Provides hardware abstraction for:
 * - Microphone input and frequency detection
 * - Tone and beep generation
 * - Volume control
 * - Audio codec management
 */

#ifndef TEENSY_AUDIO_IO_H
#define TEENSY_AUDIO_IO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONFIGURATION CONSTANTS
 * ========================================================================== */

#define AUDIO_BLOCK_SIZE 128        /* Audio block size in samples (~2.9ms at 44.1kHz) */
#define FFT_SIZE 256                /* FFT size for frequency analysis */
#define AUDIO_SAMPLE_RATE 44100     /* Sample rate in Hz */
#define MAX_FILENAME_LENGTH 256     /* Max filename length for SD card files */

/* ============================================================================
 * ERROR CODES
 * ========================================================================== */

typedef enum {
    TEENSY_AUDIO_OK = 0,
    TEENSY_AUDIO_ERROR = -1,
    TEENSY_AUDIO_NO_SD = -2,
    TEENSY_AUDIO_FILE_ERROR = -3
} teensy_audio_error_t;

/* ============================================================================
 * DATA STRUCTURES
 * ========================================================================== */

/* Opaque pointer for file handle (platform-dependent) */
typedef void* audio_file_handle_t;

/* Audio streaming structure for SD card playback (legacy) */
typedef struct {
    audio_file_handle_t audio_file;
    bool is_playing;
    uint32_t file_size;
    uint32_t bytes_read;
    int16_t buffer[AUDIO_BLOCK_SIZE];
    float fft_buffer[FFT_SIZE];
} teensy_audio_stream_t;

/* ============================================================================
 * CORE FUNCTIONS - Used by State Machine
 * ========================================================================== */

/**
 * Initialize audio system
 * Sets up audio codec, allocates memory, configures I/O
 * @return TEENSY_AUDIO_OK on success, error code otherwise
 */
teensy_audio_error_t init_audio_system(void);

/**
 * Capture audio samples from microphone
 * Reads real-time audio data from I2S input
 * 
 * @param buffer - Output buffer for audio samples (int16_t PCM format)
 * @param max_samples - Maximum number of samples to capture
 * @return Number of samples actually captured
 */
int capture_audio_samples(int16_t* buffer, int max_samples);

/**
 * Read frequency from microphone using FFT
 * This is the main frequency detection function used by the tuner
 * 
 * @param samples - Audio samples (can be NULL if using FFT directly)
 * @param num_samples - Number of samples
 * @return Detected frequency in Hz, or 0.0 if no signal
 */
double read_frequency_from_microphone(const int16_t* samples, int num_samples);

/**
 * Play a tone at specified frequency
 * Used for "play tone" mode - lets user hear target frequency
 * 
 * @param frequency - Frequency in Hz (e.g., 110.0 for A2)
 * @param duration_ms - How long to play tone (milliseconds)
 */
void play_tone(double frequency, uint32_t duration_ms);

/**
 * Play a short "ready" beep
 * Signals to user that device is ready to listen
 */
void play_ready_beep(void);

/**
 * Generate a beep for feedback
 * Used by dynamic beep feedback system
 * 
 * @param duration_ms - Beep duration in milliseconds (typically 50ms)
 */
void play_beep(uint32_t duration_ms);

/**
 * Stop all audio playback
 */
void stop_audio_playback(void);

/**
 * Update tone playback (call in main loop)
 * Automatically stops tone after duration expires
 */
void update_tone_playback(void);

/**
 * Check if tone is currently playing
 * @return true if playing, false otherwise
 */
bool is_audio_playing(void);

/**
 * Set audio output volume
 * @param vol - Volume level (0.0 to 1.0)
 */
void set_volume(float vol);

/**
 * Get current volume level
 * @return Volume (0.0 to 1.0)
 */
float get_volume(void);

/* ============================================================================
 * DIAGNOSTIC FUNCTIONS
 * ========================================================================== */

/**
 * Play a WAV file from SD card
 * Used for accessibility - spoken feedback
 * 
 * @param filename - Path to WAV file (e.g., "/AUDIO/STRING_E.wav")
 * @return TEENSY_AUDIO_OK on success, error code otherwise
 */
teensy_audio_error_t play_audio_file_from_sd(const char* filename);

/**
 * Check if SD card audio is playing
 * @return true if SD audio is playing
 */
bool is_sd_audio_playing(void);

/**
 * Stop SD card audio playback
 */
void stop_sd_audio(void);

/**
 * Check if SD card is available
 * @return true if SD card is mounted and accessible
 */
bool is_sd_card_available(void);

/**
 * Print audio system status for debugging
 */
void print_audio_status(void);

/* ============================================================================
 * LEGACY FUNCTIONS - SD Card Playback (Not Used by State Machine)
 * ========================================================================== */

teensy_audio_error_t open_audio_file(teensy_audio_stream_t* stream, const char* filename);
teensy_audio_error_t read_audio_block(teensy_audio_stream_t* stream, float* output);
void close_audio_file(teensy_audio_stream_t* stream);
void get_fft_data(float* fft_output, int num_bins);
void process_audio_realtime(void);
void list_audio_files(void);
/* play_audio_file() is declared in audio_sequencer.h */

#ifdef __cplusplus
}
#endif

#endif /* TEENSY_AUDIO_IO_H */