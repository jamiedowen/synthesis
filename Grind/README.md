# Midnight Grind

A hybrid semi-modular synth VST3 modelled on the **Behringer GRIND**. It has a 24-engine
digital oscillator, a 24 dB ladder filter, a VCA with an attack/decay envelope, an analog-style
LFO, a 32-step sequencer, an arpeggiator, a 22 × 12 patch bay and up to 16 voices of
"Poly Chain".

![Midnight Grind](docs/editor.png)

The GRIND's oscillator is built on Mutable Instruments **Plaits**, and so is this one. The Plaits
engines here are Emilie Gillet's original open-source DSP code (MIT licence), vendored in
`ThirdParty/mutable` and driven the way the Plaits voice drives them: the trigger, the internal
decay envelope, the low-pass gate and the per-engine gain staging all match. Those engines
are therefore the real thing, not an approximation.

## What's in it

### Digital oscillator: 24 engines

| Bank | Engines | Source |
|---|---|---|
| **Synth** | Virtual Analog, Waveshaper, FM, Grains, Additive, Chords, Speech, Karplus, Hypersaw, Wavetable | Plaits |
| **Percussion / noise** | Rain, Noise, Dust, Modal Strings, Bass, Snare, Hi Hat | Plaits |
| | FM Drum, Cowbell, Tom | new (Tom uses the Plaits analog drum resonator) |
| **Extended** | DX7 | Plaits 6-op FM, with the three DX7 patch banks |
| | TD-3 Bassline, Wave Generator, Vox | new |

- **HARMONICS / TIMBRE / MORPH** are the three macro controls. The display and the labels
  under the knobs show what each one does on the selected engine.
- **FM / TIMBRE CV / MORPH CV** are the Plaits attenuverters. With nothing patched into those
  inputs, they apply the oscillator's internal decay envelope, just as on Plaits.
- **DECAY**, **LPG COLOUR** and **LPG** control the Plaits low-pass gate, which can pluck the
  oscillator on every note.
- **OUT / AUX** blends the engine's main output with its AUX variant. Each engine's AUX is
  listed on the display.
- **FREQUENCY** sets ±3 octaves. Fine tune is on the Settings tab.

### Analog path
- **VCO MOD**: an amount knob, a source switch (EG / LFO) and a destination switch
  (Frequency / Harmonics / Timbre / Morph).
- **24 dB ladder VCF**: a zero-delay-feedback transistor-ladder model with tanh saturation,
  oversampled 2×. It self-oscillates at high resonance and has a switchable **LP / HP** mode.
  The panel also has **VCF MOD** (source EG / LFO, polarity + / −), **KB TRACK** and **DRIVE**.
- **EG / VCA**: an analog-style attack/decay envelope with a **SUSTAIN** switch, and a **VCA**
  mode switch: **EG**, or **ON** for drones and self-enveloped percussion.
- **LFO**: triangle or square, 0.05–80 Hz, with a rate LED.
- **GLIDE**, **VOLUME**, and an output meter.

### Patch bay (22 × 12)
On the **PATCH BAY** tab you can route 12 outputs into 22 inputs. Every input is normalled, so
the synth plays with nothing patched.

- **Outputs**: LFO TRI, LFO SQR, EG, GATE, KB CV, VELO, MOD WHL, AFTERT, S&H, NOISE, ACCENT,
  OSC ENV.
- **Inputs**: PITCH, FM, HARMONICS, TIMBRE, MORPH, MODEL, OSC LEVEL, OSC DECAY, LPG COLOUR,
  OUT/AUX MIX, CUTOFF, RESONANCE, VCF MOD, VCA, ATTACK, DECAY, LFO RATE, GLIDE, VCO MOD, DRIVE,
  OSC TRIG, EG TRIG.

To use it:
- Click a jack to patch it. Drag up or down to set the level (amber is positive, teal is
  negative). Double-click to unpatch.
- PITCH and CUTOFF follow a 1 V/oct-style scale, so KB CV → CUTOFF at 100% gives full keyboard
  tracking.
- Try LFO SQR → OSC TRIG + EG TRIG with a percussion engine, and the synth plays itself.

### Sequencer and arpeggiator
- **PLAY MODE** offers KEYS, SEQ or ARP.
- **Sequencer**:
  - 32 steps and 64 pattern slots (banks A–H × 8). Each step has a note, gate, accent, slide,
    tie and ratchet (×2–×4).
  - Pattern length is 1–32, the rate goes from 1/4 to 1/32 (triplets included), with gate
    length and swing.
  - **REC** enables step entry: play a note to write it at the cursor. While the pattern runs,
    the keys transpose it.
  - **COPY / PASTE / CLEAR / RANDOM** work on the current pattern.
- **Arpeggiator**: 12 patterns (Up, Down, Up/Down, Down/Up, Up+Down, Played, Random, Converge,
  Diverge, Pinky Up, Thumb Up, Chord), 1–4 octaves, rate, gate and **HOLD** (latch).
- **Timing**: both follow the host transport when it is playing. With the transport stopped,
  they run from their own clock at the TEMPO setting: the arpeggiator whenever keys are held,
  the sequencer when **RUN** is on.

### Voices and MIDI
- **Poly Chain**: 1–16 voices. Each voice is a full oscillator, filter, VCA and envelope.
  With 1 voice, the synth is the mono GRIND, with legato note returns.
- **Voice priority** (Last / Low / High), **MIDI channel** (Omni / 1–16) and **pitch bend
  range** are on the Settings tab. Mod wheel and aftertouch are available in the patch bay.

### Factory presets
There are 23 presets, chosen from the PRESET menu or from FL Studio's preset list:
- Init
- Grind Bass
- Acid Line (TD-3 + sequencer)
- Hypersaw Lead
- DX Keys
- Chord Stabs
- Plaits Pluck
- Karplus Harp
- Modal Bells
- Robot Speech
- Vox Choir
- Wavetable Sweep
- Rain Drone
- Grind Kick
- Snare
- Hats
- FM Drum Perc
- Cowbell
- Tom Fill
- Wave Gen Bass
- FM Growl
- Grain Pad
- Noise Sweep

## How close is it to the hardware?

- **Plaits engines**: bit-for-bit the same algorithms. They run at the module's 48 kHz
  internal rate and are resampled to your session rate. Plaits' 16-bit output clipping is
  kept too.
- **New engines** (FM Drum, Cowbell, Tom, TD-3 Bassline, Wave Generator, Vox): Behringer's
  firmware isn't public, so these are new implementations that follow the published
  description of each engine. They behave like the hardware, but they are not a copy of it.
- **Analog filter, VCA, EG and LFO**: these are models, tuned by ear and by measurement.
  They don't come from schematics.
- **Panel layout**: it follows the GRIND's Mother-32 / Crave-style architecture. The patch bay
  is a matrix rather than a row of jacks, and the Poly Chain is built in instead of needing
  extra units.

## Using it in FL Studio

1. Download the **MidnightGrind-Windows-VST3** artifact from the latest *Build* run in GitHub
   Actions, then unzip it.
2. Copy the `Midnight Grind.vst3` folder to `C:\Program Files\Common Files\VST3\`.
3. In FL Studio, open *Options → Manage plugins*, click *Find plugins*, then add
   **Midnight Grind** to the Channel Rack.

Tips:
- For drums, choose a percussion engine and set **VCA** to **ON** so each hit rings out fully.
- For acid lines, load *Acid Line*, press play in FL, and play a key to transpose the pattern.
- For pads, set Poly Chain to 8 voices and add some LFO into TIMBRE or MORPH in the patch bay.

## Building from source

The plugin is part of the repository's top-level CMake project, and also builds on its own:

```sh
cmake -S Grind -B build-grind -DCMAKE_BUILD_TYPE=Release
cmake --build build-grind --config Release --target MidnightGrind_VST3
```

A `MidnightGrind_Standalone` target is also available.

### Render test

An offline test renders every engine and preset to WAV and checks for NaNs, silence and
clipping. It also covers the following:
- pitch accuracy at host rates from 44.1 to 96 kHz
- the sequencer, step record, arpeggiator hold, the Poly Chain and the patch bay
- a state round trip
- a CPU measurement
- a screenshot of every tab

```sh
cmake -S Grind -B build-grind-test -DMIDNIGHT_GRIND_BUILD_TESTS=ON
cmake --build build-grind-test --target MidnightGrindRenderTest
./build-grind-test/MidnightGrindRenderTest_artefacts/Release/MidnightGrindRenderTest grind-out
```

On Linux, run it under `xvfb-run` for the screenshots.

## Credits

- Plaits DSP and stmlib © Emilie Gillet, MIT licence (`ThirdParty/mutable/LICENSE`). The code
  is unmodified apart from two portability fixes for MSVC: an always-inline macro and an empty
  lookup table that was removed.
- "Behringer" and "GRIND" are trademarks of their owners. This is an independent plugin
  inspired by that instrument, and it is not affiliated with Behringer or Mutable Instruments.
