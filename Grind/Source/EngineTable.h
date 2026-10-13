#pragma once

#include <array>

namespace grind
{
constexpr int numEngines = 24;

enum class EngineBank { synth, percussion, extended };

struct EngineInfo
{
    const char* name;      // as printed on the panel
    const char* shortName; // for the model buttons
    EngineBank bank;
    const char* harmonics; // what each macro control does on this engine
    const char* timbre;
    const char* morph;
    const char* aux;       // what the AUX output carries
};

// Engine order follows the Grind's model list: 10 synth engines,
// 10 percussion / noise engines, 4 extended engines.
inline constexpr std::array<EngineInfo, numEngines> engineInfo {{
    { "Virtual Analog", "VA",      EngineBank::synth, "Detune",        "Square shape",   "Saw shape",      "Hard-synced pair" },
    { "Waveshaper",     "WSHAPE",  EngineBank::synth, "Shaper curve",  "Fold amount",    "Asymmetry",      "Alternate curve" },
    { "FM",             "FM",      EngineBank::synth, "Ratio",         "Mod index",      "Feedback",       "Sub oscillator" },
    { "Grains",         "GRAINS",  EngineBank::synth, "Formant ratio", "Formant freq",   "Formant width",  "Windowed sines" },
    { "Additive",       "ADDITV",  EngineBank::synth, "Bumps",         "Main harmonic",  "Bump shape",     "Drawbar organ" },
    { "Chords",         "CHORDS",  EngineBank::synth, "Chord type",    "Inversion",      "Waveform",       "Root note" },
    { "Speech",         "SPEECH",  EngineBank::synth, "Synth type",    "Species",        "Phoneme / word", "Raw vocal cords" },
    { "Karplus",        "KARPLS",  EngineBank::synth, "Inharmonicity", "Brightness",     "Decay",          "Raw exciter" },
    { "Hypersaw",       "HYPSAW",  EngineBank::synth, "Pitch spread",  "Grain density",  "Grain overlap",  "Sine swarm" },
    { "Wavetable",      "WTABLE",  EngineBank::synth, "Bank",          "Row",            "Column",         "Lo-fi 5-bit" },

    { "Rain",           "RAIN",    EngineBank::percussion, "Randomness",   "Density",      "Filter type",  "Raw dust" },
    { "Noise",          "NOISE",   EngineBank::percussion, "LP / BP / HP", "Clock freq",   "Resonance",    "Dual band-pass" },
    { "Dust",           "DUST",    EngineBank::percussion, "Randomness",   "Density",      "Filter type",  "Filtered rain" },
    { "Modal Strings",  "MODAL",   EngineBank::percussion, "Material",     "Brightness",   "Decay",        "Raw exciter" },
    { "FM Drum",        "FM DRM",  EngineBank::percussion, "Ratio",        "FM amount",    "Decay",        "Noise layer" },
    { "Bass",           "BASS",    EngineBank::percussion, "Punch / drive", "Tone",        "Decay",        "Synthetic kick" },
    { "Snare",          "SNARE",   EngineBank::percussion, "Tone / noise", "Mode balance", "Decay",        "Synthetic snare" },
    { "Hi Hat",         "HI HAT",  EngineBank::percussion, "Metal / noise", "High-pass",   "Decay",        "Ring-mod squares" },
    { "Cowbell",        "COWBEL",  EngineBank::percussion, "Clank",        "Brightness",   "Decay",        "Agogo" },
    { "Tom",            "TOM",     EngineBank::percussion, "Pitch drop",   "Stick / tone", "Decay",        "Synthetic tom" },

    { "DX7",            "DX7",     EngineBank::extended, "Preset",       "Mod level",    "Env stretch",  "Same as OUT" },
    { "TD-3 Bassline",  "TD-3",    EngineBank::extended, "Resonance",    "Cutoff",       "Env mod / decay", "Square wave" },
    { "Wave Generator", "WAVGEN",  EngineBank::extended, "2nd osc",      "Symmetry / PW", "Waveform",    "Sine + sub" },
    { "Vox",            "VOX",     EngineBank::extended, "Voice type",   "Breath / choir", "Vowel",      "Whisper" },
}};

// Engines that envelope themselves when triggered (percussion, plucked models)
inline constexpr bool engineIsPercussive (int e)
{
    return e == 7 || (e >= 13 && e <= 20);
}

} // namespace grind
