#pragma once

#include <initializer_list>
#include <utility>
#include <vector>

#include "PatchMatrix.h"

namespace grind
{
struct PatchCable { int destination, source; float amount; };

struct Preset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values; // parameter id -> real value (unlisted = default)
    std::vector<PatchCable> cables;
};

// Engine indices, for readability
enum EngineId
{
    eVA, eWaveshaper, eFM, eGrains, eAdditive, eChords, eSpeech, eKarplus, eHypersaw, eWavetable,
    eRain, eNoise, eDust, eModal, eFMDrum, eBass, eSnare, eHiHat, eCowbell, eTom,
    eDX7, eTD3, eWaveGen, eVox
};

inline const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> presets {
        { "Init", {}, {} },

        { "Grind Bass",
          { { "model", eVA }, { "harmonics", 0.25f }, { "timbre", 0.6f }, { "morph", 0.8f },
            { "cutoff", 380.0f }, { "resonance", 0.35f }, { "vcfModAmt", 0.55f }, { "drive", 0.55f },
            { "attack", 0.0f }, { "decay", 0.35f }, { "glide", 0.15f }, { "frequency", -12.0f } }, {} },

        { "Acid Line",
          { { "model", eTD3 }, { "harmonics", 0.75f }, { "timbre", 0.25f }, { "morph", 0.45f },
            { "cutoff", 6000.0f }, { "resonance", 0.1f }, { "vcfModAmt", 0.1f }, { "drive", 0.6f },
            { "decay", 0.4f }, { "playMode", 1 }, { "seqRun", 1 }, { "pattern", 0 }, { "frequency", -12.0f } }, {} },

        { "Hypersaw Lead",
          { { "model", eHypersaw }, { "harmonics", 0.35f }, { "timbre", 0.7f }, { "morph", 0.75f },
            { "cutoff", 4200.0f }, { "resonance", 0.2f }, { "vcfModAmt", 0.3f }, { "attack", 0.15f },
            { "decay", 0.5f }, { "glide", 0.2f } },
          { { dstPitch, srcLfoTri, 0.004f } } },

        { "DX Keys",
          { { "model", eDX7 }, { "harmonics", 0.12f }, { "timbre", 0.55f }, { "morph", 0.5f },
            { "cutoff", 12000.0f }, { "vcaMode", 1 }, { "voices", 8 }, { "dx7Bank", 0 } }, {} },

        { "Chord Stabs",
          { { "model", eChords }, { "harmonics", 0.4f }, { "timbre", 0.45f }, { "morph", 0.35f },
            { "cutoff", 900.0f }, { "resonance", 0.4f }, { "vcfModAmt", 0.5f }, { "decay", 0.55f },
            { "sustain", 0 } }, {} },

        { "Plaits Pluck",
          { { "model", eWaveshaper }, { "harmonics", 0.55f }, { "timbre", 0.4f }, { "morph", 0.5f },
            { "lpgOn", 1 }, { "oscDecay", 0.45f }, { "lpgColour", 0.6f }, { "timbreAmt", 0.5f },
            { "vcaMode", 1 }, { "cutoff", 8000.0f }, { "voices", 6 } }, {} },

        { "Karplus Harp",
          { { "model", eKarplus }, { "harmonics", 0.3f }, { "timbre", 0.55f }, { "morph", 0.65f },
            { "vcaMode", 1 }, { "cutoff", 9000.0f }, { "voices", 8 }, { "playMode", 2 }, { "arpPattern", 2 },
            { "arpOctaves", 2 } }, {} },

        { "Modal Bells",
          { { "model", eModal }, { "harmonics", 0.7f }, { "timbre", 0.65f }, { "morph", 0.7f },
            { "vcaMode", 1 }, { "cutoff", 12000.0f }, { "voices", 8 } }, {} },

        { "Robot Speech",
          { { "model", eSpeech }, { "harmonics", 0.6f }, { "timbre", 0.5f }, { "morph", 0.3f },
            { "cutoff", 6000.0f }, { "resonance", 0.25f } },
          { { dstMorph, srcLfoTri, 0.8f } } },

        { "Vox Choir",
          { { "model", eVox }, { "harmonics", 0.45f }, { "timbre", 0.75f }, { "morph", 0.1f },
            { "attack", 0.55f }, { "decay", 0.6f }, { "cutoff", 5000.0f }, { "voices", 8 },
            { "lfoRate", 0.25f } },
          { { dstMorph, srcLfoTri, 0.5f } } },

        { "Wavetable Sweep",
          { { "model", eWavetable }, { "harmonics", 0.3f }, { "timbre", 0.2f }, { "morph", 0.2f },
            { "cutoff", 3000.0f }, { "resonance", 0.3f }, { "lfoRate", 0.2f } },
          { { dstMorph, srcLfoTri, 0.7f }, { dstTimbre, srcEnvelope, 0.6f } } },

        { "Rain Drone",
          { { "model", eRain }, { "harmonics", 0.6f }, { "timbre", 0.55f }, { "morph", 0.7f },
            { "vcaMode", 1 }, { "cutoff", 2500.0f }, { "resonance", 0.55f }, { "vcfModSrc", 1 },
            { "vcfModAmt", 0.3f }, { "lfoRate", 0.15f } }, {} },

        { "Grind Kick",
          { { "model", eBass }, { "harmonics", 0.6f }, { "timbre", 0.4f }, { "morph", 0.5f },
            { "vcaMode", 1 }, { "cutoff", 16000.0f }, { "frequency", -24.0f } }, {} },

        { "Snare",
          { { "model", eSnare }, { "harmonics", 0.5f }, { "timbre", 0.5f }, { "morph", 0.45f },
            { "vcaMode", 1 }, { "cutoff", 16000.0f }, { "frequency", -12.0f } }, {} },

        { "Hats",
          { { "model", eHiHat }, { "harmonics", 0.5f }, { "timbre", 0.6f }, { "morph", 0.2f },
            { "vcaMode", 1 }, { "cutoff", 16000.0f } }, {} },

        { "FM Drum Perc",
          { { "model", eFMDrum }, { "harmonics", 0.55f }, { "timbre", 0.55f }, { "morph", 0.35f },
            { "vcaMode", 1 }, { "cutoff", 9000.0f }, { "resonance", 0.3f }, { "playMode", 1 },
            { "seqRun", 1 }, { "pattern", 3 } }, {} },

        { "Cowbell",
          { { "model", eCowbell }, { "harmonics", 0.0f }, { "timbre", 0.4f }, { "morph", 0.3f },
            { "vcaMode", 1 }, { "cutoff", 16000.0f } }, {} },

        { "Tom Fill",
          { { "model", eTom }, { "harmonics", 0.45f }, { "timbre", 0.3f }, { "morph", 0.5f },
            { "vcaMode", 1 }, { "cutoff", 16000.0f }, { "frequency", -12.0f } }, {} },

        { "Wave Gen Bass",
          { { "model", eWaveGen }, { "harmonics", 0.55f }, { "timbre", 0.3f }, { "morph", 0.7f },
            { "cutoff", 600.0f }, { "resonance", 0.45f }, { "vcfModAmt", 0.45f }, { "decay", 0.5f },
            { "frequency", -12.0f }, { "sustain", 0 } }, {} },

        { "FM Growl",
          { { "model", eFM }, { "harmonics", 0.3f }, { "timbre", 0.45f }, { "morph", 0.4f },
            { "cutoff", 1200.0f }, { "resonance", 0.5f }, { "lfoRate", 0.45f }, { "frequency", -12.0f } },
          { { dstTimbre, srcLfoTri, 0.6f }, { dstCutoff, srcLfoTri, 0.15f } } },

        { "Grain Pad",
          { { "model", eGrains }, { "harmonics", 0.45f }, { "timbre", 0.5f }, { "morph", 0.6f },
            { "attack", 0.6f }, { "decay", 0.65f }, { "cutoff", 2400.0f }, { "voices", 8 }, { "lfoRate", 0.3f } },
          { { dstTimbre, srcLfoTri, 0.4f }, { dstHarmonics, srcSampleHold, 0.1f } } },

        { "Noise Sweep",
          { { "model", eNoise }, { "harmonics", 0.5f }, { "timbre", 0.8f }, { "morph", 0.7f },
            { "cutoff", 400.0f }, { "resonance", 0.6f }, { "vcfModSrc", 1 }, { "vcfModAmt", 0.6f },
            { "lfoRate", 0.2f } }, {} },
    };
    return presets;
}

} // namespace grind
