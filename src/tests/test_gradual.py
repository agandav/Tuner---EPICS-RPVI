import numpy as np
import sounddevice as sd
import time
import math
"""Test gradual guiding beeps with pitch glides and speed variation based on proximity to target note It will play a series of beeps that glide in pitch from a higher frequency down toward the target note as the user gets closer to being in tune. The beep speed will also increase when farther from the target and slow down as they get closer, providing intuitive feedback on how to adjust their tuning."""


SAMPLE_RATE = 44100
A4_FREQUENCY = 440.0
MIN_AUDIBLE_FREQUENCY = 700.0
MAX_AUDIBLE_FREQUENCY = 4000.0
RUN_AUDIO_DIAGNOSTIC = False


def to_audible_frequency(frequency):
    audible = float(frequency)
    while audible < MIN_AUDIBLE_FREQUENCY:
        audible *= 2.0
    while audible > MAX_AUDIBLE_FREQUENCY:
        audible /= 2.0
    return audible


def comparison_frequencies(detected_freq, target_freq):
    audible_target = to_audible_frequency(target_freq)
    ratio = detected_freq / target_freq if target_freq > 0 else 1.0
    base_detected = to_audible_frequency(audible_target * ratio)
    is_sharp = detected_freq > target_freq
    min_gap_hz = 80.0

    candidates = []
    for shift in range(-4, 5):
        candidate = base_detected * (2.0 ** shift)
        if MIN_AUDIBLE_FREQUENCY <= candidate <= MAX_AUDIBLE_FREQUENCY:
            candidates.append(candidate)

    if is_sharp:
        directional = [candidate for candidate in candidates if candidate > audible_target]
    else:
        directional = [candidate for candidate in candidates if candidate < audible_target]

    if directional:
        separated = [
            candidate for candidate in directional
            if abs(candidate - audible_target) >= min_gap_hz
        ]
        if separated:
            audible_detected = min(separated, key=lambda candidate: abs(candidate - audible_target))
        else:
            audible_detected = max(directional, key=lambda candidate: abs(candidate - audible_target))
    else:
        if is_sharp:
            audible_detected = audible_target * 2.0 if audible_target * 2.0 <= MAX_AUDIBLE_FREQUENCY else audible_target
        else:
            audible_detected = audible_target / 2.0 if audible_target / 2.0 >= MIN_AUDIBLE_FREQUENCY else audible_target

    return audible_detected, audible_target

def audio_device_diagnostic():
    """Test that audio output is working"""
    print("\n=== AUDIO DEVICE TEST ===")
    print(f"Default output device: {sd.default.device}")
    print(f"Available devices:")
    print(sd.query_devices())
    print("\nPlaying test tone (440 Hz for 1 second)...")
    duration = 1.0
    t = np.linspace(0, duration, int(SAMPLE_RATE * duration), False)
    wave = 0.9 * np.sin(2 * np.pi * 440 * t).astype(np.float32)
    sd.play(wave, SAMPLE_RATE)
    sd.wait()
    print("Did you hear the test tone? If not, check your speakers/volume.")
    time.sleep(0.5)

def play_beep(frequency, duration_ms, amplitude=0.9, preserve_frequency=False):
    """Play audible beep with visual confirmation"""
    if not preserve_frequency:
        frequency = to_audible_frequency(frequency)
    print(f"    [PLAYING BEEP: {frequency:.0f} Hz for {duration_ms} ms]")
    duration_sec = duration_ms / 1000.0
    samples = int(SAMPLE_RATE * duration_sec)
    t = np.linspace(0, duration_sec, samples, False)
    wave = amplitude * np.sin(2 * np.pi * frequency * t).astype(np.float32)
    sd.play(wave, SAMPLE_RATE)
    sd.wait()
    time.sleep(0.1)
    print(f"    [BEEP FINISHED]")

def play_in_tune_jingle():
    """Play C-E-G success chime"""
    print("\n  *** IN TUNE! PLAYING JINGLE ***")
    print("    C5 (523 Hz)...")
    play_beep(523.25, 250, preserve_frequency=True)
    time.sleep(0.15)
    print("    E5 (659 Hz)...")
    play_beep(659.25, 250, preserve_frequency=True)
    time.sleep(0.15)
    print("    G5 (784 Hz)...")
    play_beep(783.99, 500, preserve_frequency=True)
    time.sleep(0.3)
    print("  *** JINGLE COMPLETE ***\n")

def get_semitone_offset(note_letter):
    mapping = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
    return mapping.get(note_letter.upper(), -1)

def parse_note(note):
    if not note:
        return 0.0
    letter = note[0].upper()
    semitone = get_semitone_offset(letter)
    if semitone < 0:
        return 0.0
    idx = 1
    sharp = False
    if idx < len(note) and note[idx] == "#":
        sharp = True
        idx += 1
    if idx >= len(note) or not note[idx].isdigit():
        return 0.0
    octave = int(note[idx])
    if sharp:
        semitone += 1
    midi = (octave + 1) * 12 + semitone
    frequency = A4_FREQUENCY * (2 ** ((midi - 69) / 12.0))
    return frequency

def map_value(x, in_min, in_max, out_min, out_max):
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

def constrain(x, min_val, max_val):
    return max(min_val, min(max_val, x))

def gradual_guiding_beeps(target_note, detected_ratio, steps):
    target_freq = parse_note(target_note)
    start_freq = target_freq * detected_ratio
    freq_step = (target_freq - start_freq) / steps
    
    # Determine direction
    tuning_up = (start_freq < target_freq)
    
    # Calculate GLIDING beep range
    # Tuning UP (too low): ascending guidance (low -> high)
    # Tuning DOWN (too high): descending guidance (high -> low)
    if tuning_up:
        beep_start = target_freq * 2.0  # 1 octave higher than target
        beep_end = target_freq * 4.0    # 2 octaves higher than target
    else:
        beep_start = target_freq * 8.0  # 3 octaves higher than target
        beep_end = target_freq * 4.0    # 2 octaves higher than target

    while min(beep_start, beep_end) < MIN_AUDIBLE_FREQUENCY:
        beep_start *= 2.0
        beep_end *= 2.0

    while max(beep_start, beep_end) > MAX_AUDIBLE_FREQUENCY:
        beep_start /= 2.0
        beep_end /= 2.0
    
    beep_freq_step = (beep_end - beep_start) / steps
    
    print("\n=================================")
    print(f"Target: {target_note} ({target_freq:.2f} Hz)")
    print(f"Starting: {start_freq:.2f} Hz")
    print(f"Beep glide: {beep_start:.0f} Hz -> {beep_end:.0f} Hz")
    print("=================================")
    
    # INITIAL COMPARISON - ONCE AT START
    compare_detected, compare_target = comparison_frequencies(start_freq, target_freq)
    print("\n  -> Initial Comparison: Your note vs Target")
    print(f"     Playing YOUR note: {start_freq:.2f} Hz (audible {compare_detected:.0f} Hz)...")
    play_beep(compare_detected, 1000, 0.8, preserve_frequency=True)
    time.sleep(0.3)
    print(f"     Playing TARGET note: {target_freq:.2f} Hz (audible {compare_target:.0f} Hz)...")
    play_beep(compare_target, 1000, 0.8, preserve_frequency=True)
    time.sleep(0.5)
    
    print("\n  -> Starting gradual beeping (pitch glides)...\n")
    
    # GRADUAL BEEPING - PITCH GLIDES TOWARD TARGET
    for i in range(steps + 1):
        current_freq = start_freq + (freq_step * i)
        cents = 1200.0 * math.log2(current_freq / target_freq)
        abs_cents = abs(cents)
        
        # Check if in tune
        if abs_cents < 5.0:
            print(f"\nStep {i+1}/{steps+1} - {current_freq:.2f} Hz -> IN TUNE!")
            play_in_tune_jingle()
            time.sleep(1.0)
            return
        
        # GRADUAL BEEPING: Pitch glides from beep_start toward beep_end
        beep_freq = beep_start + (beep_freq_step * i)
        
        # Speed: FAST when FAR, SLOW when CLOSE
        gap_ms = int(map_value(constrain(abs_cents, 5, 100), 5, 100, 800, 100))
        
        # Beep duration varies
        beep_duration = int(map_value(constrain(abs_cents, 5, 100), 5, 100, 400, 150))
        
        print(f"Step {i+1}/{steps+1} - String: {current_freq:.2f} Hz ({cents:.1f} cents)")
        print(f"  Direction: {'TUNE DOWN' if cents > 0 else 'TUNE UP'}")
        print(f"  Beep: {beep_freq:.0f} Hz (gliding), Duration: {beep_duration}ms, Gap: {gap_ms}ms")
        
        # PLAY THE GLIDING BEEP
        play_beep(beep_freq, beep_duration, preserve_frequency=True)
        
        # WAIT THE GAP
        time.sleep(gap_ms / 1000.0)

def main():
    print("=" * 50)
    print("GRADUAL GUIDING BEEPS TEST")
    print("Beep PITCH GLIDES from high to lower as you approach target")
    print("Speed: FAST beeps when FAR, SLOW beeps when CLOSE")
    print("=" * 50)
    
    # Run audio diagnostic first (optional)
    if RUN_AUDIO_DIAGNOSTIC:
        audio_device_diagnostic()
    input("\nPress ENTER to continue with tests (or Ctrl+C to abort)...")
    
    time.sleep(2)
    
    # TEST 1
    print("\n" + "=" * 50)
    print(">>> TEST 1: E4 String - 80 cents FLAT <<<") # Needs to tune up
    print("=" * 50)
    gradual_guiding_beeps("E4", 0.954, 15)
    sd.wait()
    time.sleep(4)
    
    # TEST 2
    print("\n" + "=" * 50)
    print(">>> TEST 2: A2 String - 60 cents SHARP <<<") # Needs to tune down
    print("=" * 50)
    gradual_guiding_beeps("A2", 1.035, 15)
    sd.wait()
    time.sleep(4)
    
    
    # FINAL JINGLE
    print("\n" + "=" * 50)
    print("ALL TESTS COMPLETE!")
    print("=" * 50)
    play_in_tune_jingle()

if __name__ == "__main__":
    main()