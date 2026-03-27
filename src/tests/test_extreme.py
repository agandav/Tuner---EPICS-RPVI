import numpy as np
import sounddevice as sd
import time
import math

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
        audible_detected = base_detected

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
    # Use louder amplitude and ensure float32 format
    wave = amplitude * np.sin(2 * np.pi * frequency * t).astype(np.float32)
    sd.play(wave, SAMPLE_RATE)
    sd.wait()  # Explicitly wait for playback to finish
    time.sleep(0.1)  # Longer buffer after playback
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
    time.sleep(0.3)  # Wait after jingle completes
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

def extreme_guiding_beeps(target_note, detected_ratio, steps):
    target_freq = parse_note(target_note)
    start_freq = target_freq * detected_ratio
    freq_step = (target_freq - start_freq) / steps
    
    print("\n=================================")
    print(f"Target: {target_note} ({target_freq:.2f} Hz)")
    print(f"Starting: {start_freq:.2f} Hz")
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
    
    print("\n  -> Starting proximity beeping...\n")
    
    # PROXIMITY BEEPING FOR EACH STEP
    for i in range(steps + 1):
        current_freq = start_freq + (freq_step * i)
        cents = 1200.0 * math.log2(current_freq / target_freq)
        abs_cents = abs(cents)
        
        # Check if in tune
        if abs_cents < 5.0:
            print(f"\nStep {i+1}/{steps+1} - {current_freq:.2f} Hz -> IN TUNE!")
            play_in_tune_jingle()
            time.sleep(1.0)  # Extra pause after jingle before returning
            return
        
        # Fixed beep frequencies for direction
        # Too low (tune DOWN) -> 1000 Hz (lower pitch beep)
        # Too high (tune UP) -> 600 Hz (higher  pitch beep)
        too_high = (cents > 0)
        if too_high:
            beep_freq = 600.0  # low beep = tune down
        else:
            beep_freq = 1100.0   # High beep = tune up
        
        # Speed: FAST (short gap) when FAR, SLOW (long gap) when CLOSE
        # Far away (100 cents) = 100ms gap (fast beeps)
        # Close (5 cents) = 800ms gap (slow beeps)
        gap_ms = int(map_value(constrain(abs_cents, 5, 100), 5, 100, 800, 100))
        
        # Beep duration: shorter when far (quick), longer when close
        beep_duration = int(map_value(constrain(abs_cents, 5, 100), 5, 100, 400, 150))
        
        # Print status FIRST
        print(f"Step {i+1}/{steps+1} - String: {current_freq:.2f} Hz ({cents:.1f} cents)")
        print(f"  Direction: {'TUNE DOWN' if too_high else 'TUNE UP'}")
        print(f"  Beep: {beep_freq:.0f} Hz ({'+2 oct' if too_high else '-2 oct'}), Duration: {beep_duration}ms, Gap: {gap_ms}ms")
        
        # PLAY THE BEEP
        play_beep(beep_freq, beep_duration)
        
        # WAIT THE GAP
        time.sleep(gap_ms / 1000.0)

def main():
    print("=" * 50)
    print("EXTREME GUIDING BEEPS TEST")
    print("Tune DOWN: 1000 Hz beep (higher pitch)")
    print("Tune UP: 500 Hz beep (lower pitch)")
    print("Speed: FAST beeps when FAR, SLOW beeps when CLOSE")
    print("=" * 50)
    
    # Run audio diagnostic first (optional)
    if RUN_AUDIO_DIAGNOSTIC:
        audio_device_diagnostic()
    input("\nPress ENTER to continue with tests (or Ctrl+C to abort)...")
    
    time.sleep(2)
    
    # TEST 1
   # print("\n" + "=" * 50)
   # print(">>> TEST 1: E4 String - 80 cents FLAT <<<")
   # print("=" * 50)
    #extreme_guiding_beeps("E4", 0.954, 15)
    #sd.wait()  # Ensure all audio finished
    #time.sleep(4)
    
    # TEST 2
    print("\n" + "=" * 50)
    print(">>> TEST 2: A2 String - 60 cents SHARP <<<") # Needs to tune down, so should get 1000 Hz beeps
    print("=" * 50)
    extreme_guiding_beeps("A2", 1.035, 15)
    sd.wait()  # Ensure all audio finished
    time.sleep(4)
    
    # TEST 3
    print("\n" + "=" * 50)
    print(">>> TEST 3: G3 String - 100 cents FLAT <<<") # Needs to tune up, so should get 600 Hz beeps
    print("=" * 50)
    extreme_guiding_beeps("G3", 0.943, 20)
    sd.wait()  # Ensure all audio finished
    
    # FINAL JINGLE
    print("\n" + "=" * 50)
    print("ALL TESTS COMPLETE!")
    print("=" * 50)
    play_in_tune_jingle()

if __name__ == "__main__":
    main()