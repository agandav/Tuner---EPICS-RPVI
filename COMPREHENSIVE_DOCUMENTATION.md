# EPICS RPVI Guitar Tuner - Complete Documentation

## A Comprehensive Guide for Anyone (No Programming Experience Needed!)

**Last Updated:** January 2026  
**Project:** EPICS RPVI (Resources for People with Visual Impairments)  
**Team:** Purdue University Engineering Projects in Community Service

---

# What is This Project?

This is a **guitar tuner** designed specifically for **blind and visually impaired students** at the Indiana School for the Blind and Visually Impaired (ISBVI). Unlike visual tuners that show a needle or lights, this tuner provides **audio feedback** to tell the user:

1. **Which string** they're playing (E, A, D, G, B, E)
2. **How far off** they are from perfect tuning (in "cents" - a musical measurement)
3. **Which direction** to tune (tighten the string UP or loosen it DOWN)

### Example of How It Works:
```
User plays their guitar's A string...

Tuner detects: 108 Hz (slightly flat)
Target frequency: 110 Hz (perfect A2)
Offset: -31 cents (31 cents flat)

Audio Output: "A string... 30 cents... tune UP"
```

---

# Why Does This Project Exist?

Traditional guitar tuners are **visual** - they show:
- A moving needle (analog tuners)
- LED lights (clip-on tuners)
- A screen display (phone apps)

**None of these work for blind musicians!**

This project creates an **audio-based tuner** that speaks the tuning information out loud, making guitar tuning accessible to everyone.

---

# Understanding This Documentation

This guide is written for people who have **never programmed before**. We will explain:
- What each file does in plain English
- Why each piece exists
- How the pieces work together
- What you might want to change and how

Think of this like a car manual - even if you've never built an engine, you can still understand what the carburetor does and when you might need to adjust it.

---

# How Does a Guitar Tuner Work? (The Science Explained Simply)

## What is Sound?

Imagine you're sitting by a still pond. When you toss a pebble in, ripples spread out in waves. **Sound works exactly the same way!**

When you pluck a guitar string:
1. The string vibrates back and forth
2. This pushes air molecules, creating waves
3. These waves travel through the air to your ear
4. Your eardrum vibrates at the same speed
5. Your brain interprets this as sound

## Understanding Frequency (Why Notes Sound Different)

**Frequency** is just how fast something vibrates.

Think of a child on a swing:
- If they swing back and forth slowly (1 time per second), that's a **low frequency**
- If they swing very fast (10 times per second), that's a **high frequency**

Guitar strings work the same way:
- **Thick, loose strings** vibrate slowly → LOW frequency → LOW pitch (bass notes)
- **Thin, tight strings** vibrate quickly → HIGH frequency → HIGH pitch (treble notes)

We measure frequency in **Hertz (Hz)**, which just means "vibrations per second."

### Standard Guitar String Frequencies:

| String | Note | Frequency (Hz) | In Simple Terms |
|--------|------|----------------|-----------------|
| 6 (thickest) | E2 | 82.41 Hz | Vibrates 82 times per second - very low rumble |
| 5 | A2 | 110.00 Hz | Vibrates 110 times per second |
| 4 | D3 | 146.83 Hz | Vibrates 147 times per second |
| 3 | G3 | 196.00 Hz | Vibrates 196 times per second |
| 2 | B3 | 246.94 Hz | Vibrates 247 times per second |
| 1 (thinnest) | E4 | 329.63 Hz | Vibrates 330 times per second - high pitch |

**When you tune a guitar**, you're adjusting how tight each string is to make it vibrate at exactly the right frequency!

## How the Tuner Detects Frequency (The Magic of FFT)

When you pluck a string near a microphone, the computer "hears" a series of numbers representing the loudness of the sound at each moment in time. This is called a **waveform**.

**Problem:** Looking at these numbers, it's almost impossible to tell what frequency they represent. It's like trying to figure out what song is playing by looking at the grooves on a vinyl record!

**Solution:** We use something called an **FFT (Fast Fourier Transform)**.

### What the FFT Does (Explained Like You're Five):

Imagine you have a smoothie with strawberries, bananas, and blueberries all blended together. The FFT is like a magical filter that separates the smoothie back into individual fruits so you can see exactly what's in it.

For sound:
- **INPUT**: A complicated mix of vibrations (the smoothie)
- **FFT**: The magical separator
- **OUTPUT**: A list showing which frequencies are present and how strong each one is (the individual fruits)

```
BEFORE FFT (time-domain):
Time:    0ms   1ms   2ms   3ms   4ms   5ms ...
Sound:  [+50, -30, +80, -70, +60, -40, ...]
"It's just numbers changing over time - hard to understand!"

AFTER FFT (frequency-domain):
Frequency:  82Hz   110Hz   147Hz   196Hz ...
Strength:   [10,    250,    15,     8,    ...]
"Aha! There's a strong signal at 110 Hz - that's the A string!"
```

### Why This Matters:

Without FFT, we'd have to count how many times the waveform goes up and down per second - like counting individual heartbeats for an hour straight. FFT does this calculation instantly (in about 1/10,000th of a second!).

## Understanding "Cents" (The Tuner's Measurement System)

Musicians don't say "you're 3.2 Hz off" because that number doesn't mean much to human ears. Instead, they use **cents**.

Think of it like temperature:
- We could measure temperature in "electron kinetic energy" (scientifically accurate but confusing)
- OR we could use "degrees Fahrenheit" (much more useful!)

**Cents** are the musical equivalent of degrees:

### The Cent System:
- **1 semitone** = the distance from one piano key to the next = **100 cents**
- **1 cent** = 1/100th of a semitone (very tiny difference)
- **0 cents** = PERFECT tuning (you're exactly on target!)
- **+50 cents** = You're halfway to the next note (very sharp!)
- **-50 cents** = You're halfway down to the previous note (very flat!)

### What You Can Actually Hear:
- **±2 cents**: Most people can't hear the difference - sounds "in tune"
- **±5 cents**: Trained musicians start to notice
- **±10 cents**: Obviously out of tune
- **±20 cents**: Sounds bad to everyone
- **±50 cents**: Painfully out of tune

**This tuner's goal**: Get you within ±2 cents so even professional musicians would say "that's perfectly in tune!"

---

# The Overall System (How Everything Works Together)

Think of this tuner like a restaurant kitchen with different stations:

```
1. MICROPHONE (The Ears)
   ↓
   Captures vibrations from guitar string
   ↓
2. AUDIO PROCESSING (The Analyzer)  
   ↓
   Uses FFT to determine "this is vibrating at 108.5 Hz"
   ↓
3. STRING DETECTION (The Identifier)
   ↓
   Figures out "that's the A string, and you're 31 cents flat"
   ↓
4. AUDIO SEQUENCER (The Announcer)
   ↓
   Generates synthesized audio feedback using tone patterns
   ↓
5. SPEAKER (The Voice)
   ↓
   User hears synthesized tones indicating tuning status!
```

Each piece does ONE job really well, and they all work together in sequence.

---

# Understanding the Project Folder Structure

When you open this project, you'll see lots of folders and files. Let's understand what each one does!

Think of this like organizing a house:
- **Living room** = main source code (src/)
- **Workshop** = testing tools (Guitar Unit Testing Files/)
- **Library** = reference books (CMSIS-DSP-Tests/)
- **Photo album** = documentation images (img/)
- **Instruction manual** = README and this file
- **Blueprint** = build configuration (platformio.ini)

```
Tuner---EPICS-RPVI/
│
├── src/                             # LIVING ROOM - Main code that runs on device
│   ├── audio_processing.c           # Listens and detects frequencies
│   ├── string_detection.c           # Identifies which string and how off-tune
│   ├── audio_sequencer.c            # Plays spoken feedback
│   ├── button_input.c               # Handles button presses
│   ├── hardware_interface.c         # Controls physical components
│   ├── teensy_audio_io.cpp          # Manages microphone and speaker
│   ├── main.cpp                     # The "start here" file
│   └── config.h                     # Settings and configuration
│
├── Guitar Unit Testing Files/       # WORKSHOP - Test the code on your PC
│   ├── tuner_tests.c                # Automated quality checks
│   └── mingw32/                     # Tools for running tests on Windows
│
├── CMSIS-DSP-Tests/                 # LIBRARY - Advanced math functions
│   └── CMSIS-DSP/                   # Professional signal processing library
│
├── references_for_building/         # EXAMPLE CODE - Reference implementations
│   └── scipy_audio_ref.py           # Python example of FFT
│
├── img/                             # PHOTO ALBUM - Diagrams and pictures
│   └── Software Flowchart.drawio.png
│
├── platformio.ini                   # BLUEPRINT - Build instructions
├── README.md                        # QUICK START GUIDE
└── COMPREHENSIVE_DOCUMENTATION.md   # YOU ARE HERE!
```

---

# Complete File-by-File Guide (Every File Explained in Detail)

This section explains EVERY file in the project as if you've never seen code before. We'll use lots of analogies and real-world examples.

---

## The src/ Directory - The Main Program (What Runs on the Device)

This folder contains the actual program that runs on the physical tuner device (called a "Teensy 4.1" - it's a tiny, powerful computer the size of a stick of gum).

---

### src/main.cpp - The Starting Point
**What it is**: Think of this as the "power button" file  
**Size**: Tiny (about 30 lines)  
**Programming Language**: C++ (a common programming language)

**What this file does in plain English:**

When you turn on the tuner, this file runs first. It's like a morning checklist:

1. ☑️ Wake up the microphone system
2. ☑️ Turn on the frequency detector
3. ☑️ Get the buttons ready
4. ☑️ Prepare the speaker
5. ☑️ Start listening for guitar strings!

**The two most important parts:**

1. **setup()** - Runs ONCE when you turn on the device
   - This is like getting dressed in the morning - you only do it once
   
2. **loop()** - Runs OVER and OVER forever
   - This is like breathing - it keeps happening automatically
   - Checks "Is someone playing a string?"
   - If yes → detect frequency → announce result
   - If no → keep waiting

**Why you might change this file:**
- You want to add a new feature that needs to start when the tuner powers on
- You want to change the order things happen
- You need to add a new sensor or button

**You probably WON'T need to change this file** - it's already set up correctly!

---

### src/audio_processing.c - The Ear and Brain
**What it is**: The frequency detection system (the most complex part!)  
**Size**: About 405 lines of code  
**Programming Language**: C

**What this file does in plain English:**

This is the tuner's "ear" and "brain." It does three jobs:

**JOB 1: LISTENING**
- Constantly records sound from the microphone
- Takes 256 "snapshots" of the sound per measurement (called "samples")
- Each snapshot is just a number representing "how loud right now"

**JOB 2: ANALYZING (The FFT Magic)**
- Takes those 256 numbers and performs complex mathematics
- Uses an algorithm called "FFT" (Fast Fourier Transform)
- Think of it like a chef tasting a soup and saying "I taste tomatoes, basil, and garlic" - the FFT "tastes" the sound and says "I hear 110 Hz, 220 Hz, and 330 Hz"

**JOB 3: IDENTIFYING**
- Looks at all the frequencies it detected
- Finds the LOUDEST one (that's probably the guitar string)
- Announces "The guitar is producing 108.5 Hz"

**The key parts inside this file:**

1. **audio_processing_init()**
   - Sets everything up when the tuner turns on
   - Clears memory, prepares data storage
   - Like setting up a workstation before starting work

2. **apply_fft()**
   - THE BIG ONE - this does the frequency detection
   - Takes sound → returns frequency
   - Uses very complex mathematics (don't worry, you don't need to understand HOW it works, just WHAT it does!)

3. **remove_dc_offset()**
   - Fixes a common problem with microphones
   - Microphones sometimes add a constant "bias" to all measurements
   - This function removes that bias (like zeroing a scale before weighing something)

4. **apply_hann_window()**
   - Prepares the data for FFT by "smoothing the edges"
   - Imagine trimming rough edges off a piece of wood before sanding
   - Prevents errors in the FFT calculation

5. **find_peak_frequency()**
   - After FFT, we have 256 "bins" of frequency data
   - This finds which bin has the most energy
   - Like finding the tallest person in a crowd

**Important numbers in this file:**

- **FFT_SIZE = 256**: How many sound samples we analyze at once
  - Bigger = more accurate but slower
  - 256 is a good balance
  
- **SAMPLE_RATE = 10,000**: How many times per second we measure sound
  - 10,000 Hz is perfect for guitar (guitar range is 82-330 Hz)
  - Higher sample rates waste processing power on frequencies we don't care about

- **MIN_AMPLITUDE = 50**: How quiet is too quiet to analyze
  - If the sound is quieter than this, we ignore it (it's probably just room noise)
  - Prevents the tuner from trying to tune to a refrigerator hum!

**Why you might change this file:**
- The tuner isn't detecting very quiet playing → lower MIN_AMPLITUDE
- You want more accuracy → increase FFT_SIZE (to 512 or 1024)
- You're adapting this for a different instrument → change SAMPLE_RATE

**What happens if this file breaks:**
- The tuner can't tell what frequency it's hearing
- It might report random frequencies or say "0 Hz" for everything

---

### src/string_detection.c - The Identifier
**What it is**: Figures out which string is playing and how out-of-tune it is  
**Size**: About 160 lines  
**Programming Language**: C

**What this file does in plain English:**

The audio_processing.c file tells us "I hear 108.5 Hz." This file answers:
- "That's the A string!"
- "It should be 110 Hz"
- "You're 31 cents flat"
- "Tune UP!"

It's like a translator - it converts raw frequency numbers into musical information.

**How it works (step by step):**

**STEP 1: The Comparison**
- Has a table of all 6 guitar string frequencies
- Compares what it heard (108.5 Hz) to each string
- Finds the closest match (A string = 110 Hz is only 1.5 Hz away)

**STEP 2: The Math**
- Calculates HOW FAR OFF using the "cents" formula
- Formula: `cents = 1200 × log₂(detected ÷ target)`
- Don't worry about the math - the computer does it instantly!
- Result: -31 cents (negative means "flat" or "too low")

**STEP 3: The Direction**
- If cents are negative (flat) → "Tune UP" (tighten the string)
- If cents are positive (sharp) → "Tune DOWN" (loosen the string)
- If cents are very close to zero (±2) → "In tune!"

**The key parts inside this file:**

1. **guitar_notes[] array**
   - A big table listing every note the guitar can play
   - Each entry has: frequency, note name, string number, octave
   - Like a dictionary: "110 Hz = A, string 5, octave 2"

2. **calculate_cents_offset()**
   - Does the mathematical formula to get cents
   - Takes two inputs: detected frequency and target frequency
   - Returns: how many cents off you are

3. **get_tuning_direction()**
   - Simple decision maker
   - Looks at cents
   - Returns "UP", "DOWN", or "IN_TUNE"
   - Like a GPS: "Turn left" vs "Turn right" vs "You've arrived!"

4. **find_closest_string()**
   - Compares detected frequency to all 6 strings
   - Finds the nearest one
   - Returns which string it thinks you're playing

5. **analyze_tuning()**
   - THE MAIN FUNCTION - puts it all together
   - Input: detected frequency
   - Output: Complete TuningResult with all the info
   
**The TuningResult structure** (the "report card"):
```
┌─────────────────────────────────┐
│     TUNING RESULT               │
├─────────────────────────────────┤
│ Detected String: 5 (A string)   │
│ Detected Frequency: 108.5 Hz    │
│ Target Frequency: 110.0 Hz      │
│ Cents Offset: -31 cents         │
│ Direction: "Tune UP"            │
│ Note Name: "A"                  │
│ Octave: 2                       │
└─────────────────────────────────┘
```

**Important numbers in this file:**

- **TUNING_TOLERANCE = 2.0 cents**: How close is "close enough"
  - If you're within ±2 cents, we say "in tune!"
  - Professional musicians can barely hear a 2-cent difference
  - You could change this to 1.0 for stricter tuning or 5.0 for more forgiving

**Why you might change this file:**
- Add alternative tunings (Drop D, DADGAD, etc.) → modify the guitar_notes[] table
- Change how strict "in tune" is → modify TUNING_TOLERANCE
- Make it work for bass guitar or ukulele → add new note tables

**What happens if this file breaks:**
- The tuner might identify strings incorrectly
- It might say "tune up" when you should tune down
- Cents calculations might be wrong

---

### src/string_detection.h - The Definitions File
**What it is**: A "header file" that defines what the string_detection.c file can do  
**Size**: About 55 lines  
**Programming Language**: C

**What this file does in plain English:**

Think of a restaurant menu vs. a kitchen. The menu (header file) tells you what dishes are available. The kitchen (the .c file) is where the actual cooking happens.

This file is the "menu" - it lists:
- What functions are available (like "analyze_tuning")
- What data structures exist (like "TuningResult")
- What constants are defined (like "GUITAR_STRING_1_FREQ")

**Why header files exist:**
Other parts of the program need to know "what can string_detection.c do for me?" without needing to read all 160 lines of code. The header file is like a table of contents.

**You probably won't need to change this file** unless you're adding brand new features to string_detection.c.

---

### src/audio_sequencer.c - The Announcer
**What it is**: Generates synthesized audio feedback to the user  
**Size**: About 238 lines  
**Programming Language**: C

**What this file does in plain English:**

After the tuner knows "A string, 30 cents, tune UP," this file's job is to TELL the user using real-time synthesized audio tones.

**How it works:**

The tuner generates audio feedback using the Teensy Audio Library's AudioSynthWaveformSine:
- No SD card needed
- No WAV files stored
- All audio is mathematically generated in real-time

This file uses tone patterns and beeps:
1. Play reference tone for the detected string (e.g., 110 Hz for A string)
2. Use beeping patterns to indicate how far off (fast beeps = very off, slow beeps = close)
3. Different beep rates indicate tuning direction
4. Solid tone = perfectly in tune!

**Operating mode:**

**Dynamic Tone Feedback with Real-time Synthesis**
- Plays the reference frequency for the detected string
- Uses beeping patterns that change rate based on tuning accuracy
- Far from in-tune → fast beeping (rapid pulses)
- Close to in-tune → slow beeping (gentle pulses)
- Perfectly in-tune → solid continuous tone
- Similar to parking sensors in cars!
- All audio generated mathematically - no files needed

**The key parts inside this file:**

1. **playNoteTone()**
   - Plays a synthesized tone using note notation (e.g., "E2", "A4")
   - Uses the Teensy Audio Library's sine wave generator
   - Duration specified in milliseconds

2. **playFrequencyTone()**
   - Plays a tone at a specific frequency in Hz
   - Used for reference pitch playback
   - All audio generated in real-time

3. **playGuitarString()**
   - Plays the reference tone for a specific guitar string (1-6)
   - String 1 → plays E4 (329.63 Hz)
   - String 6 → plays E2 (82.41 Hz)

4. **playStringIdentifier()**
   - Uses distinct tone patterns to identify each string
   - String 1: single high beep
   - String 2: two medium beeps
   - Each string has unique audio signature

5. **playBeep()**
   - Generates short feedback beeps
   - Frequency and duration configurable
   - Used for tuning status indication

6. **stopAllTones()**
   - Immediately stops all audio output
   - Cleans up synthesis state

**Important numbers:**

- **Beep rate table**: Maps cents to milliseconds between beeps
  - >100 cents: 100ms interval (10 beeps/second - frantic!)
  - 75-100 cents: 150ms (6.7 beeps/second)
  - 50-75 cents: 200ms (5 beeps/second)
  - ... all the way down to ...
  - <5 cents: No beeps (solid tone - you're there!)

**Why you might change this file:**
- Add new audio files for more detailed feedback
- Change beep rates to be faster/slower
- Add multi-language support (spanish.wav files)
- Change feedback style completely

**What happens if this file breaks:**
- The tuner can still detect frequencies but won't provide audio feedback
- No tones or beeps will be generated
- Silent tuner - defeats the accessibility purpose!

---

### src/button_input.c - The Button Handler
**What it is**: Lets users select which string they want to tune  
**Size**: About 175 lines  
**Programming Language**: C

**What this file does in plain English:**

The tuner has buttons labeled E, A, D, G, B (the 5 main notes). When you press a button, this file:
1. Detects the button press
2. Listens to what frequency the guitar is currently making
3. Figures out which octave you're playing in
4. Calculates what the TARGET frequency should be

**Example:**
- User presses the "A" button
- Tuner hears 220 Hz (from the guitar)
- This file thinks: "220 Hz is in the octave 3 range, so they want A3"
- A3 = 220 Hz exactly
- Reports back: "Target is 220 Hz"

**Why the octave detection matters:**

"A" can mean different frequencies:
- A2 = 110 Hz (5th guitar string)
- A3 = 220 Hz (5th string, 12th fret)
- A4 = 440 Hz (international tuning standard)
- A5 = 880 Hz (very high)

By listening to what they're currently playing, the tuner knows which A they want!

**The key parts inside this file:**

1. **note_to_semitone_offset()**
   - Converts note letters to numbers
   - A=0, B=2, C=3, D=5, E=7, F=8, G=10
   - Why not 0,1,2,3? Because music has whole steps and half steps!
   - (Don't worry about the details - just know it converts letters to numbers)

2. **detect_octave_from_frequency()**
   - Hears current frequency
   - Determines which octave range it's in
   - 82-164 Hz → octave 2
   - 164-328 Hz → octave 3
   - 328-656 Hz → octave 4
   - Like sorting mail by zip code!

3. **button_to_frequency()**
   - THE MAIN FUNCTION
   - Input: button pressed + current frequency
   - Output: target frequency
   - Uses musical mathematics: `frequency = 440 × 2^(semitones_from_A4 / 12)`
   - (Again, don't worry about the formula - computer does it!)

**The musical math (simplified):**

Music is based on a ratio: 2^(1/12) ≈ 1.059463

- One semitone up = multiply by 1.059463
- One semitone down = divide by 1.059463
- 12 semitones up = multiply by 2 (one octave = double the frequency!)

This is called "equal temperament" and is why pianos sound good in any key.

**Why you might change this file:**
- Add more buttons for chromatic tuning (all 12 notes)
- Change the octave detection ranges for a different instrument
- Remove automatic octave detection and just use fixed frequencies

**What happens if this file breaks:**
- Buttons might not respond
- Wrong target frequencies might be selected
- Octave detection might fail (thinks you want A2 when you want A4)

---

### src/button_input.h - Button Definitions
**What it is**: Header file for button functions  
**Size**: About 85 lines  
**Programming Language**: C

**What this file does:** Lists what button_input.c can do, just like string_detection.h does for string_detection.c. It's the "menu" for the button system.

Contains:
- Enum of button types (NOTE_A, NOTE_B, NOTE_C, etc.)
- Function declarations
- Documentation

---

### src/hardware_interface.c - The Physical Controls Manager
**What it is**: Controls all the physical components (buttons, knobs, amplifier)  
**Size**: About 383 lines  
**Programming Language**: C

**What this file does in plain English:**

This file is the bridge between the software (code) and hardware (physical parts). It manages:
- **6 string buttons** (E, A, D, G, B, high E)
- **Volume knob** (potentiometer)
- **Audio amplifier** (the chip that makes sound loud enough to hear)
- **Optional components** (rotary encoders, additional buttons)

Think of it as the "remote control" system - it takes user actions and translates them into something the computer can understand.

**The major challenges this file solves:**

**CHALLENGE 1: Button Bouncing**

When you press a physical button, the metal contacts actually bounce together several times in about 20 milliseconds:
```
                                        ▔▔▔▔▔▔▔▔▔▔▔▔ (pressed)
          (not pressed) ▁▁▁▁▁▁▁▁▁|‾|_|‾|_|‾ 
                                    ↑
                                Bouncing!
```

Without "debouncing," one button press looks like 5-10 rapid presses!

**The solution:** When a button state changes, wait 20ms and check again. If it's still in the new state, it's a real press.

**CHALLENGE 2: Analog Reading (Volume Knob)**

The volume knob is a "potentiometer" (variable resistor). As you turn it, the resistance changes, which changes the voltage:
- Fully left: 0 volts
- Middle: 1.65 volts
- Fully right: 3.3 volts

The computer reads this voltage and converts it to a number 0-1023. This file converts that number to a user-friendly 0.0-1.0 scale.

**CHALLENGE 3: Amplifier Power Management**

The audio amplifier chip draws power. This file can turn it on/off to save battery:
- When tuning: Amplifier ON
- When idle for 5 minutes: Amplifier OFF (sleep mode)
- User presses button: Amplifier ON (wake up!)

**The key parts inside this file:**

1. **hardware_interface_init()**
   - Runs once at startup
   - Configures all the pins (tells the computer "pin 2 is a button input")
   - Sets initial states (volume = 50%, amplifier = ON)

2. **button_poll()**
   - Checks all buttons continuously
   - Returns TRUE if any button changed state
   - Like a security guard checking all the doors

3. **button_get_event()**
   - When button_poll() finds a change, this gives details:
     - Which button?
     - Pressed or released?
     - How long was it held?
   - Like a security report: "Door #3 opened at 10:23 AM"

4. **button_debounce()**
   - Implements the 20ms wait-and-check logic
   - Prevents false triggers
   - Essential for reliable button reading

5. **volume_read_analog()**
   - Reads the potentiometer
   - Converts 0-1023 to 0.0-1.0
   - Filters out jitter (tiny fluctuations)

6. **audio_amplifier_enable() / disable()**
   - Turns amplifier on/off
   - Saves battery power
   - Like a light switch for the speaker system

7. **mode_switch_is_play_tone() / mode_switch_is_listen_only()**
   - Reads the physical mode switch state
   - Play Tone mode: tuner plays reference pitch, then listens
   - Listen Only mode: tuner only listens and provides feedback
   - Returns true/false based on switch position

8. **tactile_feedback_click/confirm/warning()**
   - Plays short sounds to confirm button presses
   - Click = single short beep (acknowledgment)
   - Confirm = double beep (success)
   - Warning = triple beep (error)
   - Gives blind users non-visual confirmation

**Data structures:**

**button_state_t**: Tracks each button's current state
```
┌─────────────────────────────┐
│ BUTTON #2 (A String)        │
├─────────────────────────────┤
│ Current State: PRESSED      │
│ Last State: RELEASED        │
│ Debounce Counter: 25ms      │
│ Press Duration: 150ms       │
└─────────────────────────────┘
```

**volume_control_t**: Tracks volume state
```
┌─────────────────────────────┐
│ VOLUME CONTROL              │
├─────────────────────────────┤
│ Current Level: 0.65 (65%)   │
│ Raw ADC Value: 665          │
│ Last Update: 1234ms ago     │
└─────────────────────────────┘
```

**Important numbers:**

- **DEBOUNCE_TIME_MS = 20**: Milliseconds to wait for button stability
  - Too low → false triggers from bouncing
  - Too high → feels sluggish to user
  - 20ms is the sweet spot

**Why you might change this file:**
- Add new buttons or sensors
- Change debounce timing for different button types
- Add new tactile feedback patterns
- Implement long-press vs short-press functionality

**What happens if this file breaks:**
- Buttons might not respond or respond erratically
- Volume knob might not work
- Amplifier might not turn on
- No tactile feedback

---

### src/hardware_interface.h - Hardware Definitions
**What it is**: Header file defining hardware interface  
**Size**: About 205 lines  
**Programming Language**: C

**What this file does:** Defines enumerations and structures for hardware components:

- **button_id_t**: List of all button IDs
  - STRING_1_BUTTON, STRING_2_BUTTON, etc.
  - VOLUME_KNOB_CLICK (if volume knob can be pressed)
  
- **button_state_t**: PRESSED or RELEASED

- **button_event_t**: Complete information about a button event
  - Which button
  - What happened (pressed/released)
  - When it happened (timestamp)
  - How long it was held

**Analogy:** This is like a "dictionary" that defines terms used in hardware_interface.c.

---

### src/config.h - The Settings File
**What it is**: Central configuration file with all settings and pin assignments  
**Size**: About 155 lines  
**Programming Language**: C

**What this file does in plain English:**

This is the "control panel" for the entire project. Instead of having settings scattered across dozens of files, they're all collected here.

Think of it like your phone's Settings app - one place to adjust everything!

**What's in this file:**

**SECTION 1: Pin Assignments**

The Teensy 4.1 has 55 pins (tiny metal connection points). Each pin can be used for different purposes. This file assigns each pin a job:

```
Pin 0 → String 1 Button (high E)
Pin 1 → String 2 Button (B)
Pin 2 → String 3 Button (G)
Pin 3 → String 4 Button (D)
Pin 4 → String 5 Button (A)
Pin 5 → String 6 Button (low E)
Pin 6 → Mode Switch (Play Tone I / Listen Only O)
Pin 7 → I2S Data Out (to amplifier)
Pin 8 → I2S Data In (from microphone)
Pin 9-11 → Rotary Encoder (optional volume control)
Pin 14 (A0) → Microphone ADC Input
Pin 15 (A1) → Volume Potentiometer
Pin 20 → I2S LRCLK (Left/Right Clock)
Pin 21 → I2S BCLK (Bit Clock)
Pin 23 → Amplifier Enable (PAM8302A shutdown control)
```

**Why this matters:** If you build the physical hardware differently (button on pin 8 instead of pin 2), you just change the number here instead of hunting through all the code files!

**SECTION 2: Audio Settings**

```c
#define AUDIO_SAMPLE_RATE 44100     // CD-quality sound
#define AUDIO_BLOCK_SIZE 128        // Process 128 samples at a time
#define AUDIO_BIT_DEPTH 16          // 16-bit audio (standard)
```

**What these mean:**
- **Sample Rate**: How many times per second we measure sound
  - 44,100 Hz is "CD quality" - the standard for music
  - Higher = better quality but more data to process
  
- **Block Size**: How many samples we process together
  - Smaller blocks = lower latency (faster response) but more CPU work
  - Larger blocks = more efficient but slower response
  - 128 is a good middle ground

- **Bit Depth**: How many levels of loudness we can represent
  - 16-bit = 65,536 different loudness levels
  - More than enough for human ears!

**SECTION 3: FFT Settings**

```c
#define FFT_SIZE 256                 // Number of samples in FFT
#define FFT_INPUT_SAMPLE_RATE 10000  // Sample rate for FFT input
#define FFT_HZ_PER_BIN 39.06         // Frequency resolution
```

**What these mean:**
- **FFT_SIZE**: How many samples we analyze together
  - Must be a power of 2 (128, 256, 512, 1024...)
  - Larger = more accurate but slower
  - 256 is perfect for guitar

- **FFT_INPUT_SAMPLE_RATE**: Different from audio playback!
  - For detecting guitar frequencies, we don't need 44,100 Hz
  - 10,000 Hz is perfect (guitar range is 82-330 Hz)
  - Lower sample rate = less data = faster processing

- **FFT_HZ_PER_BIN**: Frequency resolution (calculated automatically)
  - Formula: Sample_Rate ÷ FFT_Size = 10,000 ÷ 256 = 39.06 Hz
  - Each "bin" in the FFT represents 39 Hz
  - Bin 0 = 0 Hz, Bin 1 = 39 Hz, Bin 2 = 78 Hz, Bin 3 = 117 Hz, etc.

**SECTION 4: Tuning Parameters**

```c
#define TUNING_TOLERANCE_CENTS 2.0     // "In tune" threshold
#define CENTS_THRESHOLD_WARN 10.0      // Show warning above this
#define CENTS_THRESHOLD_CRITICAL 50.0  // Very out of tune!
```

**What these control:**
- **Tolerance**: How close is "close enough"?
  - 2.0 cents = professional standard
  - 5.0 cents = more forgiving for beginners
  - 1.0 cents = very strict (studio recording quality)

- **Warning Threshold**: When to give warnings
  - If you're more than 10 cents off, maybe add a warning tone
  - Customizable based on user preference

- **Critical**: When something seems wrong
  - If you're 50+ cents off, you might be tuning to the wrong note!
  - Could trigger a "are you sure?" message

**SECTION 5: SD Card (NOT USED)**

```c
// #define USE_SD_CARD  // Commented out - not used in this implementation
```

**Important:** This tuner does NOT use an SD card!
- All audio is synthesized in real-time
- No WAV files stored or needed
- Simpler hardware, lower cost
- The SD card definitions remain in config.h only as reference for future expansion

**SECTION 6: Debug Settings**

```c
#define ENABLE_DEBUG_PRINTS 1   // 1 = ON, 0 = OFF
#define DEBUG_FFT_OUTPUT 0      // Print FFT data?
#define DEBUG_TUNING_RESULTS 1  // Print tuning info?
```

**What these do:**
- Control what information gets printed to the computer screen
- During development: Turn ON to see what's happening
- Final product: Turn OFF for speed (printing is slow)
- Like developer mode on your phone!

**Why you might change this file:**
- Hardware is wired differently → update pin numbers
- Want stricter/looser tuning → change TUNING_TOLERANCE_CENTS
- Different instrument → change FFT settings
- Debugging problems → enable DEBUG flags
- Saving battery → lower sample rates, smaller blocks

**What happens if this file has errors:**
- Code might try to use the wrong pins (amplifier gets button signal!)
- FFT might have wrong resolution
- Tuning tolerance might be too strict or too loose
- Debug information might be missing

**This is the FIRST file you should check when something seems wrong!**

### src/teensy_audio_io.cpp - The Audio Driver
**What it is**: The software that controls the microphone and speaker  
**Size**: About 295 lines  
**Programming Language**: C++ (slightly different from C, but similar)

**What this file does in plain English:**

This file is the "driver" for the audio hardware - think of it like the driver software you install to make a printer work with your computer.

It handles:
1. **Real-time Audio Synthesis**: Generating sine wave tones mathematically
2. **Microphone Input**: Getting sound data from the microphone via I2S
3. **Speaker Output**: Sending synthesized audio to the speaker via I2S
4. **Audio System Management**: Initializing and controlling the Teensy Audio Library

**NO SD Card - Pure Synthesis:**

This tuner uses ZERO pre-recorded audio files. Everything is generated in real-time:
- AudioSynthWaveformSine generates pure tones
- Frequencies calculated mathematically (A4 = 440 Hz, etc.)
- No storage needed - all audio created on-demand
- Simpler, more reliable, lower power consumption

**I2S Audio Interface:**

The Teensy 4.1 uses I2S (Inter-IC Sound) protocol for digital audio:
- AudioInputI2S captures microphone data directly
- AudioOutputI2S sends audio to external amplifier (PAM8302A)
- No separate codec chip needed - Teensy handles it internally
- Digital audio path reduces noise and improves quality

This file configures the I2S system and manages audio objects!

**The key parts inside this file:**

1. **init_audio_system()**
   - Initializes the Teensy Audio Library
   - Allocates audio memory blocks (20 blocks)
   - Sets up sine wave generators (sine1, beep_sine)
   - Configures I2S audio connections
   - Like connecting all the cables before a concert!

2. **play_tone()**
   - Main tone playback function
   - Sets frequency and amplitude on sine wave generator
   - Example: play_tone(329.63, 2000) plays E4 for 2 seconds
   - Non-blocking: tracks duration with millis()

3. **play_beep()**
   - Plays short feedback beeps
   - Uses separate beep_sine generator
   - Typical range: 400-1200 Hz, 50-200 ms duration

4. **play_ready_beep()**
   - Signals user that tuner is ready
   - Plays 1000 Hz tone for 200 ms
   - Audio confirmation for blind users

5. **stop_all_audio()**
   - Immediately silences all audio output
   - Sets all sine amplitudes to 0.0
   - Emergency stop function

6. **update_tone_playback()**
   - Called in main loop
   - Manages non-blocking tone duration
   - Automatically stops tones when duration expires

7. **audio_amplifier_enable() / disable()**
   - Controls external PAM8302A amplifier chip
   - Saves power by shutting off when idle
   - GPIO pin control (AUDIO_AMP_ENABLE_PIN)

8. **read_frequency_from_microphone()**
   - Captures audio from AudioInputI2S
   - Interfaces with FFT processing module
   - Returns detected frequency in Hz

9. **print_audio_status()**
   - Diagnostic function showing CPU and memory usage
   - AudioProcessorUsage() - how much CPU the audio system uses
   - AudioMemoryUsage() - how many audio blocks allocated

**Important concepts:**

**I2S Protocol:**
- Stands for "Inter-IC Sound" (IC = Integrated Circuit)
- A digital audio standard used between chips
- Like HDMI for audio
- Sends audio data at very high quality with no distortion

**Block-Based Processing:**
- Audio isn't processed one sample at a time (too slow!)
- Instead, we process 128 samples together in a "block"
- Like reading a sentence instead of one letter at a time
- More efficient!

**WAV File Format:**
- Stores audio as raw numbers (PCM = Pulse Code Modulation)
- 44,100 numbers per second (44.1kHz sample rate)
- Each number is 16 bits (can be -32768 to +32767)
- Simple, uncompressed, high quality

**Why you might change this file:**
- Support different audio formats (MP3, OGG)
- Add streaming from the internet instead of SD card
- Implement audio recording (save tuning sessions)
- Add audio effects (reverb, echo)
- Support different sample rates

**What happens if this file breaks:**
- No audio synthesis (no tones generated)
- Microphone input won't work
- Amplifier control might fail
- Audio memory allocation errors
- Complete silence - tuner becomes useless!

---

### src/teensy_audio_io.h - Audio Driver Definitions
**What it is**: Header file for the audio driver  
**Size**: About 50 lines  
**Programming Language**: C++

**What this file does:** Defines constants and structures for audio I/O:

- **Audio configuration**: Sample rates, block sizes, buffer counts
- **teensy_audio_stream_t**: Structure containing file handle and playback state
- **teensy_audio_error_t**: Error codes (OK, NO_SD_CARD, FILE_NOT_FOUND, etc.)
- **Function prototypes**: Lists all available audio functions

---

### src/tuner_main.c - Empty Placeholder
**What it is**: An intentionally empty file  
**Size**: About 3 lines (just comments)  
**Programming Language**: C

**Why this exists:**

PlatformIO (the build system) automatically compiles EVERY .c file it finds in the src/ folder. Some other files contain their own `main()` function for testing.

To prevent the compiler from getting confused ("wait, there are TWO main() functions!"), this file exists as a placeholder but doesn't actually contain any code.

**You should never need to change this file!**

---

### src/native_test_main.c - The Testing Program
**What it is**: A comprehensive test program that runs on your PC  
**Size**: About 662 lines  
**Programming Language**: C

**What this file does in plain English:**

This is quality assurance! Before deploying the tuner to actual hardware, this program tests every part to make sure it works correctly.

It simulates guitar frequencies and checks if the tuner detects them correctly.

**The three main test categories:**

**TEST 1: Open String Test**
- Simulates all 6 open strings (E2, A2, D3, G3, B3, E4)
- Verifies the tuner correctly identifies each one
- Checks that cents calculations are accurate
- Pass criteria: Must identify correct string and be within ±5 Hz

**TEST 2: Chromatic Note Test**
- Tests all 36 chromatic notes in guitar range (E2 through B4)
- That's every note including sharps and flats
- Ensures the tuner can handle any note, not just open strings
- Pass criteria: Must detect frequency within ±10 Hz

**TEST 3: Full Fretboard Test**
- Tests all 78 playable notes on a guitar (6 strings × 13 frets)
- Verifies the complete range of the instrument
- Most comprehensive test
- Pass criteria: 95%+ accuracy

**Why this is important:**

Without testing, you might upload code to the device and discover problems only when someone tries to use it. These tests catch bugs BEFORE the code ever touches the hardware.

It's like a practice exam before the real test!

**The test data arrays:**

The file contains large tables of frequencies:
```c
// Every note from E2 (82.41 Hz) to E5 (659.25 Hz)
// Tests the entire range a guitar can produce
```

**You probably won't need to change this file** unless you're adding support for a different instrument or tuning system.

---

### Other src/ Files (Stubs and Helpers)

Several small files exist for build compatibility:

**src/wrapper.c, src/buffer_manager.cpp, src/fft_processor.cpp**
- Size: 0-10 lines each
- Purpose: Empty stub files
- Why they exist: PlatformIO build system compatibility
- **You won't need to change these!**

**src/noise_filtering.c**
- Size: About 55 lines
- Purpose: Noise reduction algorithms
- Contains median filter implementation
- Used to remove random spikes and electrical noise from audio

**src/signal_processing.c**
- Size: About 15 lines (mostly placeholders)
- Purpose: Advanced signal processing (future expansion)
- Planned features:
  - Harmonic product spectrum (prevent detecting overtones)
  - Parabolic interpolation (improve frequency accuracy)
  - Pitch tracking over time
- **Currently not used - future development!**

---

## Guitar Unit Testing Files/ Directory - The Development Workshop

**What this entire directory is:** A self-contained testing environment for Windows PCs

**Why it exists:**

Imagine you're working on a car engine. Would you rather:
- **Option A**: Make a change, install the engine in the car, drive it, see if it works, remove the engine, make another change, repeat...
- **Option B**: Test the engine on a workbench where you can easily see everything and make quick changes?

This directory is "Option B" for the tuner software!

**What's inside:**

This directory contains COPIES of some of the main source files, modified to run on a regular Windows computer instead of on the Teensy microcontroller. This lets developers:
- Test code changes instantly (no waiting to upload to hardware)
- Run automated tests
- Debug problems more easily
- Verify math is correct before deploying

**The files:**

All the files here are modified versions of files from src/:
- **tuner_tests.c**: The main test program
- **audio_processing.c/h**: Simulated FFT (uses fake data instead of real microphone)
- **string_detection.c/h**: Real string detection code (exact copy from src/)
- **audio_sequencer.c/h**: Real audio sequencer code (exact copy from src/)

**mingw32/ folder**:
- Contains MinGW (Minimalist GNU for Windows)
- This is a free compiler that can build C programs on Windows
- Like having a factory that can manufacture the software

**How it works:**

```
1. Developer makes a change to string_detection.c
2. Copy the file to "Guitar Unit Testing Files/"
3. Run test.bat
4. Tests run on Windows PC in 2 seconds
5. See results immediately
6. If tests pass → upload to Teensy
7. If tests fail → fix the bug and try again
```

**You might use this if:**
- You're making changes to the tuning algorithms
- You want to verify your changes work correctly
- You're trying to understand how the code works
- You're adding support for a new feature

**You probably WON'T use this if:**
- You're just using the tuner (not developing it)
- You're only changing audio files on the SD card
- You're only adjusting config.h settings

---

## CMSIS-DSP-Tests/ Directory - The Math Library Testing Zone

**What this entire directory is:** Tests for professional-grade signal processing functions

**Why this exists (The Simple Explanation):**

The tuner uses very complex mathematical operations (FFT, windowing, filtering). These operations are SO common in engineering that ARM (the company that makes the processor in the Teensy) created a library of pre-written, super-optimized functions.

This library is called **CMSIS-DSP** (Cortex Microcontroller Software Interface Standard - Digital Signal Processing).

This directory tests that library to make sure it works correctly!

**The BIG Picture:**

Imagine you're building a house:
- You could cut every board yourself (slow, error-prone)
- OR you could buy pre-cut boards from a professional lumber yard (fast, accurate)

CMSIS-DSP is the "lumber yard" - it provides professionally-made mathematical functions that are:
- ✓ Correct (thoroughly tested)
- ✓ Fast (optimized for ARM processors)
- ✓ Free (provided by ARM)

**Why We Test It:**

Even though ARM provides these functions, we still need to test them in OUR specific use case to make sure:
1. We're using them correctly
2. They work on our specific hardware
3. They give expected results with guitar frequencies
4. Our integration with them is bug-free

**What's in this directory:**

---

### CMSIS-DSP-Tests/cmsis_dsp_tests.c - The Main Test File
**What it is**: Comprehensive test program for DSP functions  
**Size**: About 841 lines  
**Programming Language**: C

**What this file does:**

Tests dozens of mathematical functions organized into categories:

**CATEGORY 1: Vector Operations (Basic Math)**

Vector = fancy word for "array of numbers"

Tests:
- **Addition**: [1,2,3] + [4,5,6] = [5,7,9]
- **Subtraction**: [10,8,6] - [1,2,3] = [9,6,3]
- **Multiplication**: [2,3,4] × [5,6,7] = [10,18,28]
- **Dot Product**: Multiply and sum: [1,2,3] · [4,5,6] = (1×4) + (2×5) + (3×6) = 32
- **Scaling**: [1,2,3] × 2 = [2,4,6]
- **Absolute Value**: [-1, -2, 3] → [1, 2, 3]

Why test these? The tuner uses these operations constantly for audio processing!

**CATEGORY 2: FFT Operations (Frequency Detection)**

Tests the Fast Fourier Transform:
- **Real FFT**: Converts real audio signal to frequency spectrum
- **Complex FFT**: More general version (for advanced uses)
- **Inverse FFT**: Converts frequency back to time (used in filters)

Example test:
```
Input: Pure 440 Hz sine wave (A4 note)
FFT Process: Transform to frequency domain
Expected Output: Strong peak at 440 Hz bin
Verification: Check that peak is at right frequency ±1%
```

**CATEGORY 3: Frequency Analysis**

Tests:
- **Magnitude Calculation**: Converts complex numbers to real strengths
- **Power Spectrum**: Shows energy at each frequency
- **Peak Finding**: Locates the strongest frequency

These are critical for finding which guitar string is playing!

**CATEGORY 4: Statistics**

Tests mathematical statistics functions:
- **Mean** (average): [1,2,3,4,5] → 3
- **Variance**: How spread out the numbers are
- **Standard Deviation**: Square root of variance
- **Min/Max**: Find smallest and largest values

Used for noise detection and signal quality assessment.

**CATEGORY 5: Windowing Functions**

Tests:
- **Hann Window**: Smooths data edges (reduces FFT artifacts)
- **Hamming Window**: Alternative smoothing function

Essential for accurate FFT!

**CATEGORY 6: Integration Tests**

Tests the complete pipeline:
```
Sine Wave → FFT → Peak Finding → Frequency Detection
```

Simulates the entire tuning process to verify everything works together!

**How the tests work:**

Each test follows this pattern:
```c
1. Create test data (known input)
2. Run the CMSIS-DSP function
3. Compare output to expected result
4. If match within tolerance → PASS ✓
5. If different → FAIL ✗ (print details)
```

**Example test:**

```c
Test: arm_add_f32 (vector addition)

Input A: [1.0, 2.0, 3.0]
Input B: [4.0, 5.0, 6.0]
Expected: [5.0, 7.0, 9.0]

Run: arm_add_f32(A, B, result, 3)
Result: [5.0, 7.0, 9.0]

Compare: result == expected? YES!
Status: PASS ✓
```

---

### CMSIS-DSP-Tests/cmsis_dsp_tests.h - Test Framework Definitions
**What it is**: Header file with test infrastructure  
**Size**: About 436 lines  
**Programming Language**: C

**What this file provides:**

**1. Test Assertion Macros**

These are helper tools for testing:

```c
ASSERT_FLOAT_EQ(a, b, tolerance)
// Checks if two floating-point numbers are equal
// Within a tolerance (because computer math isn't perfect)

ASSERT_INT_EQ(a, b)
// Checks if two integers are exactly equal

ASSERT_NOT_NULL(pointer)
// Checks that a pointer isn't null (isn't broken)
```

**2. Test Result Tracking**

```c
typedef struct {
    int tests_run;      // How many tests executed
    int tests_passed;   // How many passed
    int tests_failed;   // How many failed
    char last_error[256]; // Description of last error
} cmsis_dsp_test_results_t;
```

Like a report card for the tests!

**3. Test Organization Macros**

```c
TEST_SECTION("Vector Operations")
// Prints a header to organize test output
// Makes results readable
```

**4. Floating-Point Tolerance**

```c
#define FLOAT_TOLERANCE 0.0001
```

Computers can't represent all decimal numbers perfectly. For example:
- You calculate: 1.0 ÷ 3.0 = 0.333333...
- Computer stores: 0.33333301544189453

So instead of requiring EXACT equality, we allow tiny differences (within 0.0001).

---

### CMSIS-DSP-Tests/arm_math_mock.h - The PC Simulator
**What it is**: PC versions of ARM functions  
**Size**: About 454 lines  
**Programming Language**: C

**What this file does (The Critical Concept):**

CMSIS-DSP functions are optimized for ARM processors. They use special ARM-only instructions (called NEON and SIMD) that don't exist on regular Intel/AMD computers.

**The Problem:**
```
ARM Processor (Teensy):     Intel Processor (Your PC):
✓ Has NEON instructions     ✗ No NEON instructions
✓ Can run CMSIS-DSP         ✗ Can't run CMSIS-DSP
```

**The Solution:**

This file RECREATES all the CMSIS-DSP functions using standard C code that works on ANY computer!

**Example:**

**Real ARM Version** (uses NEON, super fast):
```c
// Uses special ARM assembly instructions
// Processes 4 numbers at once (SIMD)
// Runs in 10 CPU cycles
```

**Mock Version for PC** (uses standard C, slower but compatible):
```c
void arm_add_f32(float* a, float* b, float* result, int length) {
    for (int i = 0; i < length; i++) {
        result[i] = a[i] + b[i];  // Standard C loop
    }
}
// Processes 1 number at a time
// Runs in 40 CPU cycles
// SAME OUTPUT, just slower!
```

**Why this matters:**

With this mock library:
- ✓ Tests run on your Windows PC (no hardware needed!)
- ✓ Same functions, same behavior
- ✓ Slower, but that's okay for testing
- ✓ Makes development much faster

**What functions are mocked:**

All the DSP functions:
- `arm_add_f32`, `arm_sub_f32`, `arm_mult_f32` - Vector arithmetic
- `arm_dot_prod_f32` - Dot product
- `arm_scale_f32` - Multiply by constant
- `arm_abs_f32` - Absolute values
- `arm_rfft_f32` - Real FFT
- `arm_cfft_f32` - Complex FFT
- `arm_cmplx_mag_f32` - Complex magnitude
- `arm_mean_f32` - Average
- `arm_var_f32` - Variance
- `arm_std_f32` - Standard deviation
- `arm_max_f32`, `arm_min_f32` - Min/max finding
- And many more!

---

### CMSIS-DSP-Tests/cmsis_dsp_test_utilities.c - Helper Functions
**What it is**: Utility functions for testing  
**Size**: About 100-150 lines (estimated)  
**Programming Language**: C

**What this file provides:**

**1. Signal Generation Functions**

```c
generate_sine_wave(frequency, amplitude, phase, sample_rate, num_samples)
```
- Creates perfect sine waves for testing
- Used to simulate guitar strings
- Example: Generate 110 Hz sine wave (A2 string)

```c
generate_noise(amplitude, num_samples)
```
- Creates random noise
- Tests noise rejection
- Example: Add noise to sine wave, verify FFT still finds signal

**2. Array Comparison Functions**

```c
arrays_equal(array1, array2, length, tolerance)
```
- Compares two arrays element by element
- Returns TRUE if all elements match within tolerance
- Used in almost every test!

**3. Data Printing Functions**

```c
print_array(array, length, name)
```
- Prints array contents in readable format
- Helpful for debugging failed tests
- Example output: "FFT_Output: [0.1, 0.3, 25.7, 0.2, ...]"

**4. File I/O Helpers**

```c
load_test_data(filename, array, max_length)
```
- Loads test data from files
- Useful for complex test cases
- Example: Load a recorded guitar strum to test

---

### CMSIS-DSP-Tests/cmsis_dsp_native_main.c - PC Test Runner
**What it is**: Main program to run DSP tests on PC  
**Size**: About 50-100 lines  
**Programming Language**: C

**What this file does:**

This is the "entry point" - the `main()` function that runs when you execute the test program on Windows.

**The structure:**

```c
int main() {
    1. Print "Starting CMSIS-DSP Tests..."
    
    2. Initialize test framework
       - Reset counters
       - Set up result tracking
    
    3. Run all test categories
       - Vector operations tests
       - FFT tests
       - Statistics tests
       - Windowing tests
       - Integration tests
    
    4. Print results summary
       - "Tests Run: 127"
       - "Tests Passed: 125"
       - "Tests Failed: 2"
       - "Success Rate: 98.4%"
    
    5. Return 0 if all passed, 1 if any failed
}
```

**Why it's separate:**

Different main files exist for different platforms:
- `cmsis_dsp_native_main.c` - For Windows/Mac/Linux PCs
- `cmsis_dsp_teensy_bare_metal.c` - For running on actual Teensy
- `cmsis_dsp_teensy_test.cpp` - For Teensy with Arduino framework

Same tests, different execution environments!

---

### CMSIS-DSP-Tests/cmsis_dsp_teensy_bare_metal.c - Hardware Test Runner
**What it is**: Main program to run DSP tests directly on Teensy  
**Size**: About 100-150 lines  
**Programming Language**: C

**What makes this different:**

When running on the Teensy:
- Uses real ARM NEON instructions (not mocks!)
- Tests actual hardware performance
- Verifies timing and speed
- Uses serial port for output instead of console

**Why run tests on hardware:**

Even though PC tests pass, hardware might have issues:
- Timing differences
- Memory constraints (Teensy has less RAM than PC)
- Compiler optimization differences
- Actual processor quirks

Hardware testing is the FINAL verification before deployment!

---

### CMSIS-DSP-Tests/cmsis_dsp_teensy_test.cpp - Arduino-Framework Version
**What it is**: Test runner using Arduino framework  
**Size**: About 50-75 lines  
**Programming Language**: C++

**Difference from bare_metal version:**

- **Bare metal**: Direct hardware access, no Arduino libraries
- **Arduino framework**: Uses Arduino's Serial, pinMode, etc.

The Arduino version is easier to work with but slightly slower. Good for general testing.

---

### CMSIS-DSP-Tests/CMSIS-DSP/ - The Actual Library
**What it is**: The complete ARM CMSIS-DSP library source code  
**Size**: Thousands of files, hundreds of thousands of lines  
**Programming Language**: Mostly C, some assembly

**What's inside:**

This is the OFFICIAL ARM library. It contains:

**Major subdirectories:**

1. **Include/** - Header files
   - arm_math.h (main header)
   - arm_const_structs.h (pre-computed tables)
   - Function declarations

2. **Source/** - Implementation files
   - BasicMathFunctions/ (add, sub, mult, etc.)
   - FastMathFunctions/ (sin, cos, sqrt, etc.)
   - ComplexMathFunctions/ (complex number operations)
   - FilteringFunctions/ (FIR, IIR filters)
   - MatrixFunctions/ (matrix math)
   - StatisticsFunctions/ (mean, variance, etc.)
   - SupportFunctions/ (conversions, copy, etc.)
   - TransformFunctions/ (FFT, DCT, etc.)
   - Each has multiple versions: C, NEON, Helium (different ARM optimizations)

3. **Examples/** - Sample programs
   - Shows how to use various functions
   - Learning resource

4. **Documentation/** - Technical docs
   - Function references
   - Performance benchmarks
   - Usage guides

5. **Testing/** - ARM's own test suite
   - Exhaustive tests
   - Validates correctness

6. **Scripts/** - Build and generation scripts
   - Automates library compilation
   - Generates tables and constants

**Why it's included:**

Instead of downloading the library separately, we include it in the project so:
- Everything needed is in one place
- Version is locked (won't break if ARM updates)
- Can build offline
- Easier for new developers

**You almost certainly won't need to modify anything in CMSIS-DSP/** - it's professional, tested, production code!

---

### CMSIS-DSP-Tests/CMSIS-DSP-TESTS-README.md - Documentation
**What it is**: Explanation of how to run the DSP tests  
**Size**: About 50-100 lines  
**Programming Language**: Markdown (documentation format)

**What it explains:**

- How to build the tests
- How to run them on PC vs. Teensy
- What each test does
- How to interpret results
- Troubleshooting common issues

**Quick reference for developers!**

---

### CMSIS-DSP-Tests/run_cmsis_dsp_tests_native.bat - PC Test Script
**What it is**: Windows batch file to compile and run PC tests  
**Size**: About 10-20 lines  
**Programming Language**: Batch script

**What it does:**

```batch
1. Clean previous builds
2. Compile all test files with GCC
3. Link with mock ARM math library
4. Run the resulting executable
5. Display results in terminal
```

**How to use:**

Just double-click the file in Windows Explorer! Tests run automatically.

---

### CMSIS-DSP-Tests/run_cmsis_dsp_tests_teensy.bat - Hardware Test Script
**What it is**: Windows batch file to compile and upload to Teensy  
**Size**: About 15-25 lines  
**Programming Language**: Batch script

**What it does:**

```batch
1. Clean previous builds
2. Compile with ARM compiler (not GCC!)
3. Link with REAL CMSIS-DSP (not mock)
4. Upload to connected Teensy via USB
5. Open serial monitor to see results
```

**How to use:**

1. Connect Teensy to USB
2. Double-click the file
3. Watch serial monitor for test results

---

---

### CMSIS-DSP-Tests/cmsis_dsp_teensy_bare_metal.c
**Purpose:** Entry point for running DSP tests directly on hardware

---

### CMSIS-DSP-Tests/CMSIS-DSP/
The complete ARM CMSIS-DSP library source code.

---

## references_for_building/ Directory

Contains reference implementations and test files used during development.

### references_for_building/scipy_audio_ref.py
**Lines of Code:** ~122  
**Purpose:** Python reference implementation for FFT and filtering

**What this file demonstrates:**
- Sine wave generation with noise
- Butterworth lowpass filter design
- FFT computation and visualization
- Audio file filtering example

---

## Root Directory Files

Files in the root of the repository that control the build process and provide documentation.

### platformio.ini
**Purpose:** Build system configuration

**Environments:**
- `[env:native]` - Build for PC testing
- `[env:teensy41]` - Build for microcontroller

---

### README.md
**Purpose:** Quick start guide and project overview

---

### Various .bat files
- `run_pio_tests.bat` - Run tests on PC
- `run_pio_tests.ps1` - PowerShell version
- Test result log files (*.txt)

---

# Software Configuration (PlatformIO)

## What is PlatformIO?

PlatformIO is a tool that makes it easy to write code for microcontrollers. It handles:
- Compiling your code
- Uploading to the Teensy
- Managing libraries
- Running tests

## platformio.ini Explained

```ini
; ============================================
; NATIVE ENVIRONMENT (Testing on your PC)
; ============================================
[env:native]
platform = native              ; Build for Windows/Mac/Linux
build_flags = 
    -I"${PROJECT_DIR}/Guitar Unit Testing Files"   ; Include test headers
    -I"${PROJECT_DIR}/CMSIS-DSP-Tests"            ; Include DSP headers
    -I"${PROJECT_DIR}/src"                        ; Include source headers
    -lm                        ; Link math library
    -std=c99                   ; Use C99 standard
    -Ofast                     ; Maximum optimization

; ============================================
; TEENSY 4.1 ENVIRONMENT (Actual hardware)
; ============================================
[env:teensy41]
platform = teensy              ; Teensy platform
board = teensy41               ; Teensy 4.1 board
framework = arduino            ; Use Arduino framework
monitor_speed = 115200         ; Serial monitor baud rate
build_flags = 
    -DARM_MATH_CM4             ; Enable ARM math optimizations
```

## Build Commands

```bash
# Build for PC testing
pio run -e native

# Build for Teensy 4.1
pio run -e teensy41

# Upload to Teensy
pio run -e teensy41 --target upload

# Open serial monitor
pio device monitor
```

---

# Testing System

## Why Testing Matters

Before uploading code to the Teensy, we test everything on a regular PC. This lets us:
- Find bugs quickly (no waiting for upload)
- Test without hardware
- Verify math is correct
- Ensure changes don't break existing features

## Test Files Overview

### 1. Guitar Unit Testing Files/tuner_tests.c

Tests the core tuning algorithms:

```c
void test_cents_calculation() {
    // Test perfect tuning = 0 cents
    double cents = calculate_cents_offset(440.0, 440.0);
    // Should be 0.0
    
    // Test sharp note
    cents = calculate_cents_offset(445.0, 440.0);
    // Should be +19.56 cents
    
    // Test flat note
    cents = calculate_cents_offset(435.0, 440.0);
    // Should be -19.56 cents
}

void test_string_detection() {
    // Test each guitar string frequency
    TuningResult result = analyze_tuning_auto(110.0);
    // Should detect string 5 (A string)
}

void test_tuning_direction() {
    // Test direction logic
    // 439 Hz vs 440 Hz target → "UP" (flat)
    // 441 Hz vs 440 Hz target → "DOWN" (sharp)
    // 440 Hz vs 440 Hz target → "IN_TUNE"
}
```

### 2. src/native_test_main.c

Comprehensive FFT testing with all guitar frequencies:

- Tests all 6 open strings (E2, A2, D3, G3, B3, E4)
- Tests all 36 chromatic notes (E2 through B4)
- Tests all 78 fretboard positions (6 strings × 13 frets)

### 3. CMSIS-DSP-Tests/cmsis_dsp_tests.c

Tests the signal processing library functions:

- Vector operations (add, subtract, multiply)
- FFT computation
- Filter operations
- Statistical functions

## Running Tests

### On PC (Native):
```bash
# Build
pio run -e native

# Run
.\.pio\build\native\program.exe
```

### On Teensy:
```bash
# Build and upload
pio run -e teensy41 --target upload

# Monitor output
pio device monitor
```

---

# CMSIS-DSP Library

## What is CMSIS-DSP?

**CMSIS-DSP** = Cortex Microcontroller Software Interface Standard - Digital Signal Processing

It's an **official ARM library** that provides super-fast implementations of:
- FFT (Fast Fourier Transform)
- Filtering (FIR, IIR)
- Vector math
- Matrix operations
- Statistical functions

## Why Use It?

ARM processors (like in the Teensy 4.1) have special instructions for math operations. CMSIS-DSP uses these instructions to make DSP operations **10-100x faster** than naive C code.

## Mock Implementation for PC Testing

Since CMSIS-DSP only works on ARM processors, we have a "mock" version (`arm_math_mock.h`) that provides the same functions but runs on regular Intel/AMD PCs.

```c
// Real CMSIS-DSP (on Teensy):
// Uses ARM NEON instructions, runs in ~150 microseconds

// Mock CMSIS-DSP (on PC):
// Uses regular C loops, runs in ~2 milliseconds
// Same output, just slower
```

---

# How to Build and Run

## Prerequisites

1. **Install PlatformIO CLI**
   - Download from: https://platformio.org/install/cli
   - Or install VS Code extension "PlatformIO IDE"

2. **Install USB Driver for Teensy** (if using hardware)
   - Download Teensyduino: https://www.pjrc.com/teensy/td_download.html

## Building for PC (Testing)

```bash
# Navigate to project folder
cd "C:\Users\User\OneDrive - purdue.edu\Desktop\EPCS 41200\Tuner---EPICS-RPVI"

# Build
pio run -e native

# Run tests
.\.pio\build\native\program.exe
```

## Building for Teensy 4.1

```bash
# Connect Teensy via USB

# Build and upload
pio run -e teensy41 --target upload

# Watch serial output
pio device monitor
```

## Using the Batch Scripts

Several `.bat` files are provided for convenience:

- `run_pio_tests.bat` - Build and run native tests
- `CMSIS-DSP-Tests/run_cmsis_dsp_tests_native.bat` - Run DSP tests on PC
- `CMSIS-DSP-Tests/run_cmsis_dsp_tests_teensy.bat` - Run DSP tests on Teensy

---

# Common Tasks for New Developers

## Task 1: Change the Tuning Tolerance

**File:** `src/config.h`

```c
// Current: ±2 cents considered "in tune"
#define TUNING_TOLERANCE_CENTS 2.0

// To make it stricter (±1 cent):
#define TUNING_TOLERANCE_CENTS 1.0

// To make it more forgiving (±5 cents):
#define TUNING_TOLERANCE_CENTS 5.0
```

Also update in `src/string_detection.c`:
```c
const double TUNING_TOLERANCE = 2.0;  // Change this too!
```

## Task 2: Add a New Audio File

1. Record the audio (WAV format, 44.1kHz, 16-bit)
2. Name it following the pattern: `NEWFILE.wav`
3. Copy to SD card's `/AUDIO/` folder
4. Add constant in `src/audio_sequencer.h`:
   ```c
   #define FILE_NEWFILE "NEWFILE.WAV"
   ```
5. Use in code:
   ```c
   play_audio_file(FILE_NEWFILE);
   ```

## Task 3: Add a New Button

1. Choose an unused GPIO pin
2. Add to `src/config.h`:
   ```c
   #define NEW_BUTTON_PIN 8
   ```
3. Initialize in `src/hardware_interface.c`:
   ```c
   pinMode(NEW_BUTTON_PIN, INPUT_PULLUP);
   ```
4. Add handling in `button_poll()` function

## Task 4: Improve FFT Accuracy

The FFT resolution is:
```
Resolution = Sample_Rate / FFT_Size
           = 10000 Hz / 256
           = 39.06 Hz per bin
```

To improve accuracy:
- **Option A:** Increase FFT size (256 → 512 → 1024)
  - Trade-off: Uses more memory and CPU
- **Option B:** Decrease sample rate (10000 → 5000 Hz)
  - Trade-off: Can't detect frequencies above 2500 Hz
- **Option C:** Implement parabolic interpolation (TODO in signal_processing.c)
  - Best option: Estimates frequency between bins

## Task 5: Add Alternative Tunings

Standard tuning is just one option. To add Drop D:

```c
// In string_detection.c, add alternative tuning arrays:
const double drop_d_frequencies[] = {
    329.63,  // String 1 - E4 (unchanged)
    246.94,  // String 2 - B3 (unchanged)
    196.00,  // String 3 - G3 (unchanged)
    146.83,  // String 4 - D3 (unchanged)
    110.00,  // String 5 - A2 (unchanged)
    73.42    // String 6 - D2 (dropped from E2!)
};
```

---

# Troubleshooting Guide

## Problem: FFT returns 0.0 Hz

**Possible causes:**
1. Signal too quiet → Check `MIN_AMPLITUDE` in config.h
2. No audio input → Check microphone wiring
3. DC offset too high → Verify `remove_dc_offset()` is called

## Problem: Wrong note detected

**Possible causes:**
1. Harmonics stronger than fundamental → Implement harmonic product spectrum
2. FFT resolution too low → Increase FFT_SIZE
3. Background noise → Add noise filtering

## Problem: Build fails on PC

**Solutions:**
```bash
# Clean and rebuild
pio run -e native --target clean
pio run -e native
```

Check that all include paths in `platformio.ini` are correct.

## Problem: Teensy not recognized

**Solutions:**
1. Try different USB cable (some are charge-only)
2. Press the button on Teensy to enter bootloader mode
3. Reinstall Teensyduino drivers

## Problem: No audio output

**Check:**
1. Is amplifier enabled? (`audio_amplifier_enable()`)
2. Is volume set? (`volume_set(0.7)`)
3. Are audio files on SD card?
4. Is SD card formatted as FAT32?

---

# Glossary of Terms

| Term | Definition |
|------|------------|
| **ADC** | Analog-to-Digital Converter - converts voltage to numbers |
| **Bin** | One frequency slot in FFT output |
| **Cents** | Musical unit: 100 cents = 1 semitone |
| **CMSIS-DSP** | ARM's optimized signal processing library |
| **DC Offset** | Constant bias in a signal |
| **FFT** | Fast Fourier Transform - converts time to frequency |
| **Fundamental** | The main frequency of a note (not harmonics) |
| **GPIO** | General Purpose Input/Output pins |
| **Harmonic** | Frequencies that are multiples of the fundamental |
| **Hann Window** | Smoothing function to reduce FFT edge effects |
| **Hz (Hertz)** | Cycles per second - unit of frequency |
| **I2S** | Inter-IC Sound - digital audio protocol |
| **Magnitude** | Strength of a frequency component |
| **PCM** | Pulse Code Modulation - raw digital audio format |
| **PlatformIO** | Build system for embedded development |
| **Sample Rate** | How many audio samples per second (e.g., 44100 Hz) |
| **Semitone** | Distance between adjacent piano keys |
| **Spectral Leakage** | FFT artifact from signal edges |
| **Teensy** | Fast microcontroller board from PJRC |

---

# Appendix A: Guitar Frequency Reference Chart

## Standard Tuning (A4 = 440 Hz)

### Open Strings
| String | Note | Frequency |
|--------|------|-----------|
| 6 | E2 | 82.41 Hz |
| 5 | A2 | 110.00 Hz |
| 4 | D3 | 146.83 Hz |
| 3 | G3 | 196.00 Hz |
| 2 | B3 | 246.94 Hz |
| 1 | E4 | 329.63 Hz |

### All Chromatic Notes (One Octave)
| Note | Octave 2 | Octave 3 | Octave 4 |
|------|----------|----------|----------|
| C | 65.41 | 130.81 | 261.63 |
| C# | 69.30 | 138.59 | 277.18 |
| D | 73.42 | 146.83 | 293.66 |
| D# | 77.78 | 155.56 | 311.13 |
| E | 82.41 | 164.81 | 329.63 |
| F | 87.31 | 174.61 | 349.23 |
| F# | 92.50 | 185.00 | 369.99 |
| G | 98.00 | 196.00 | 392.00 |
| G# | 103.83 | 207.65 | 415.30 |
| A | 110.00 | 220.00 | 440.00 |
| A# | 116.54 | 233.08 | 466.16 |
| B | 123.47 | 246.94 | 493.88 |

---

# Appendix B: Code Style Guide

## Naming Conventions

```c
// Functions: lowercase with underscores
void calculate_cents_offset();
void apply_fft();

// Constants: UPPERCASE with underscores
#define SAMPLE_RATE 10000
#define FFT_SIZE 256

// Variables: lowercase with underscores
int detected_string;
double target_frequency;

// Types: CamelCase with _t suffix
typedef struct { ... } TuningResult;
typedef enum { ... } TunerState;
```

## Comment Style

```c
/**
 * Function documentation (Doxygen style)
 * 
 * @param detected_freq: The frequency detected by FFT
 * @param target_freq: The target frequency for perfect tuning
 * @return: Cents offset (positive = sharp, negative = flat)
 */
double calculate_cents_offset(double detected_freq, double target_freq);

// Single-line comments for brief explanations
int pass_count = 0;  // Number of tests that passed

/* 
 * Multi-line comments for longer explanations
 * that span multiple lines
 */
```

---

# Additional File Details

## Understanding the _impl.c Files

Several files in src/ have `_impl` in their names:
- `audio_processing_impl.c`
- `audio_sequencer_impl.c`
- `string_detection_impl.c`
- `tuner_tests_impl.c`

**Why these exist:** PlatformIO compiles ALL .c files in the src/ directory. When running tests, we need different implementations than when running on hardware. The `_impl` files contain stub/placeholder implementations to prevent "duplicate symbol" linker errors.

**When to modify:** Generally, you should NOT modify these files. They exist purely for build system compatibility.

---

## The .native File Extension

Files ending in `.native` (like `tuner_tests.c.native`) are:
- Only compiled when building for PC (native environment)
- Excluded when building for Teensy hardware
- Contain test code that wouldn't work on the microcontroller

---

## tests-disabled/ Directory

This folder contains test files that are temporarily disabled:
- `FFT_Integration_Tests.c` - Integration tests for FFT pipeline
- `tuner_tests.c` - Alternative test implementations

**Why disabled:** These tests may have dependencies on code that's still in development, or they may conflict with other test files. Move files out of this directory when ready to re-enable them.

---

## Log Files in Root Directory

Several .txt files record test results and build logs:

| File | Contents |
|------|----------|
| `fretboard_test_results.txt` | Results from testing all 78 fretboard positions |
| `pio_audio_install_log.txt` | Log of audio library installation |
| `pio_build_log.txt` | Complete build output from PlatformIO |
| `pio_teensy_log.txt` | Teensy-specific build and upload logs |
| `test_output.txt` | Standard test output |
| `test_output_windowed.txt` | Test output with windowing enabled |
| `test_results_final.txt` | Final test results summary |

**When to check these:** If a build fails or tests produce unexpected results, check these logs for error messages and debugging information.

---

## The img/ Directory

Contains documentation images:
- `Software Flowchart.drawio.png` - Visual diagram of software architecture

This image can be edited using draw.io (free online tool) if you need to update the flowchart.

---

## Build Output Directory (.pio/)

After running `pio run`, PlatformIO creates a `.pio/` directory containing:
- `build/native/` - Compiled PC executable and object files
- `build/teensy41/` - Compiled Teensy firmware and object files
- `libdeps/` - Downloaded library dependencies

**Note:** This directory is in `.gitignore` and should NOT be committed to version control. It's regenerated automatically when you build.

---

## Key Relationships Between Files

### Audio Processing Pipeline
```
audio_processing.c  →  string_detection.c  →  audio_sequencer.c
     (FFT)              (Note ID)              (Feedback)
```

### Header Dependencies
```
config.h
   ↓
string_detection.h  ←  audio_processing.h
   ↓
audio_sequencer.h
   ↓
hardware_interface.h
```

### Test File Relationships
```
tuner_tests.c (main test runner)
   ├── audio_processing.c (simulated FFT)
   ├── string_detection.c (real implementation)
   └── audio_sequencer.c (real implementation)
```
