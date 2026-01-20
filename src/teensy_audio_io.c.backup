
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "arm_math.h"
#include "teensy_audio_io.h"

// teensy_audio_io.cpp converted to C using Claude AI

// --------- GLOBAL VARIABLES ---------

static audio_play_sd_wav_t playWav;
static audio_analyze_fft1024_t fft;
static audio_output_i2s_t i2s_out;
static audio_control_sgtl5000_t sgtl5000;

// Audio connections would need to be managed differently in C
// Depending on your audio library, you may need to implement these manually


// -------- Initializes hardware and software needed for audio --------
teensy_audio_error_t init_audio_system(void) 
{
    // Set up SPI for accessing SD card -- pins 11 and 13 on Teensy 4.1
    spi_set_mosi(11);  
    spi_set_sck(13);

    if (!sd_begin(SD_CHIP_SELECT)) 
    {
        serial_println("SD card initialization failed!");
        serial_println("Check that SD card is inserted.");
        return TEENSY_AUDIO_NO_SD;
    } 
    else 
    {
        serial_println("SD card initialized successfully.");
    }
   
    // Initialize audio memory blocks - allocated 20 blocks in RAM
    // Each block is 128 samples (~2.9ms at 44.1kHz). 
    // More blocks means code can handle more complex audio chains but uses more RAM
    audio_memory_init(20); 
    
    sgtl5000_enable(&sgtl5000);
    sgtl5000_set_volume(&sgtl5000, 0.5f); 
    sgtl5000_input_select(&sgtl5000, AUDIO_INPUT_LINEIN);
    
    serial_println("Audio codec enabled");
    serial_println("Audio system initialized");
    
    return TEENSY_AUDIO_OK;
}

// --------- Opens audio file (on SD card) --------------
teensy_audio_error_t open_audio_file(teensy_audio_stream_t* stream, const char* filename) 
{
    // Open file that's stored on SD card
    stream->audio_file = sd_open(filename);

    if (stream->audio_file == NULL) 
    {
        serial_print("ERROR: Cannot open file: ");
        serial_println(filename);
        return TEENSY_AUDIO_FILE_ERROR;
    }
    
    stream->file_size = sd_file_size(stream->audio_file);
    stream->bytes_read = 0;
    stream->is_playing = true;
    
    serial_print("Opened audio file: ");
    serial_print(filename);
    serial_print(", size: ");
    serial_print_uint32(stream->file_size);
    serial_println(" bytes");
    
    return TEENSY_AUDIO_OK;
}

// Reads block of audio samples from audio file
teensy_audio_error_t read_audio_block(teensy_audio_stream_t* stream, float* output) 
{
    if (!stream->is_playing || !sd_file_available(stream->audio_file)) 
    {
        stream->is_playing = false;
        return TEENSY_AUDIO_ERROR;
    }
    
    // Read raw audio data, this assumes 16-bit WAV but can be changed
    int bytes_to_read = AUDIO_BLOCK_SIZE * 2; // 2 bytes per 16-bit sample
    int bytes_read = sd_file_read(stream->audio_file, (uint8_t*)stream->buffer, bytes_to_read);
    
    if (bytes_read <= 0) 
    {
        stream->is_playing = false;
        return TEENSY_AUDIO_ERROR;
    }
    
    // Convert 16-bit samples (-32768 to +32767) to float (-1.0, 1.0)
    int samples_read = bytes_read / 2;
    for (int i = 0; i < samples_read; i++) 
    {
        output[i] = stream->buffer[i] / 32768.0f;
    }
    
    // If it gets to the end of the file and the block is less than 16-bits, fill end with 0s
    for (int i = samples_read; i < AUDIO_BLOCK_SIZE; i++) 
    {
        output[i] = 0.0f;
    }
    
    stream->bytes_read += bytes_read;
    
    // Print progress every 10% -- also an error check if it isn't loading
    static uint32_t last_progress = 0;
    uint32_t progress = (stream->bytes_read * 100) / stream->file_size;
    if (progress >= last_progress + 10) 
    {
        serial_print("Progress: ");
        serial_print_uint32(progress);
        serial_println("%");
        last_progress = progress;
    }
    
    return TEENSY_AUDIO_OK;
}

// --------- Closes audio file ----------
void close_audio_file(teensy_audio_stream_t* stream) 
{
    if (stream->audio_file != NULL) 
    {
        sd_file_close(stream->audio_file);
        stream->is_playing = false;
        serial_println("Audio file closed");
    }
}

// ------------- Gets data output from FFT ------------------
// Copies FFT results from the Teensy Audio Library into array
// FFT updates constantly, this prevents it from being overwritten
void get_fft_data(float* fft_output, int num_bins) 
{
    // Check if new data is available - FFT runs continuously but needs about 23 ms to finish processing
    if (fft_available(&fft)) 
    {
        // Teensy FFT provides 256 bins for FFT1024
        int max_bins = (num_bins < 256) ? num_bins : 256;
        
        for (int i = 0; i < max_bins; i++) 
        {
            fft_output[i] = fft_read(&fft, i);
        }
        
        // Zero remaining bins if requested more than available
        for (int i = max_bins; i < num_bins; i++) 
        {
            fft_output[i] = 0.0f;
        }
    }
}

// -------- Plays audio file ----------
teensy_audio_error_t play_audio_file(const char* filename) 
{
    // Stop any currently playing audio
    if (play_wav_is_playing(&playWav)) 
    {
        play_wav_stop(&playWav);
        delay_ms(50); 
    }
    
    // Start playing new file
    bool result = play_wav_play(&playWav, filename);
    if (!result) 
    {
        serial_print("ERROR: Could not play file: ");
        serial_println(filename);
        return TEENSY_AUDIO_FILE_ERROR;
    }
    
    serial_print("Now playing: ");
    serial_println(filename);
    
    delay_ms(10);
    
    return TEENSY_AUDIO_OK;
}

// ------ Simple audio playing check - for main function (prevents audio being cutoff) --------
bool is_audio_playing(void) 
{
    return play_wav_is_playing(&playWav);
}

void stop_audio_playback(void) 
{
    if (play_wav_is_playing(&playWav)) 
    {
        play_wav_stop(&playWav);
        serial_println("Playback stopped");
    }
}

void set_volume(float vol) 
{
    // Keep audio range 0.0-1.0
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    
    sgtl5000_set_volume(&sgtl5000, vol);
    serial_print("Volume set to: ");
    serial_print_float(vol);
    serial_println("");
}

// Real-time audio processing 
void process_audio_realtime(void) 
{
    static float fft_data[256];
    static uint32_t last_print = 0;
    
    if (fft_available(&fft)) 
    {
        get_fft_data(fft_data, 256);
        
        // Find dominant frequency
        float max_magnitude = 0.0f;
        int dominant_bin = 0;
        
        // Skip DC bin (0) and very low frequencies
        for (int i = 5; i < 128; i++) 
        {
            if (fft_data[i] > max_magnitude) 
            {
                max_magnitude = fft_data[i];
                dominant_bin = i;
            }
        }
        
        // Convert bin to frequency
        // Each bin represents: (sample_rate / FFT_size) Hz
        float bin_width = AUDIO_SAMPLE_RATE / (float)FFT_SIZE;
        float dominant_freq = dominant_bin * bin_width;
        
        // Print every 200ms to avoid flooding serial
        uint32_t current_time = millis();
        if (current_time - last_print > 200) 
        {
            serial_print("Dominant frequency: ");
            serial_print_float_precision(dominant_freq, 1);
            serial_print(" Hz, Magnitude: ");
            serial_print_float_precision(max_magnitude, 4);
            serial_println("");
            last_print = current_time;
        }
        
        // Add more stuff to improve audio processing
    }
}

// Function to list files on SD card
void list_audio_files(void) 
{
    serial_println("\nAudio files on SD card:");
    sd_file_t* root = sd_open("/");
    
    while (true) 
    {
        sd_file_t* entry = sd_open_next_file(root);
        if (entry == NULL) break;
        
        if (!sd_is_directory(entry)) 
        {
            char filename[256];
            sd_get_filename(entry, filename, sizeof(filename));
            
            // Check if file ends with .wav or .WAV
            size_t len = strlen(filename);
            if (len > 4) 
            {
                const char* ext = &filename[len - 4];
                if (strcmp(ext, ".wav") == 0 || strcmp(ext, ".WAV") == 0) 
                {
                    serial_print("  - ");
                    serial_print(filename);
                    serial_print(" (");
                    serial_print_uint32(sd_file_size(entry));
                    serial_println(" bytes)");
                }
            }
        }
        sd_file_close(entry);
    }
    sd_file_close(root);
    serial_println("");
}