#pragma once

#include <array>

namespace grind
{
// The GRIND's patch bay: 21 inputs and 13 outputs, laid out in two rows of
// 17 jacks exactly as on the panel. Every input is normalled, so the synth
// plays with nothing patched; a cable replaces the normalled connection.
//
// Signals use "volts / 5": a +-5 V LFO is +-1, a 0-5 V gate is 0..1, 1 V/oct is
// 0.2 per octave.

enum Output
{
    outVcMix, outLfoTri, outOsc1, outOsc2, outEnv, outVcaLine, outPhones,
    outLfoSquare, outNoise, outAssign, outKbCv, outGate, outVcf,
    numOutputs
};

enum Input
{
    inTimbre, inHarmonics, inFm, inMorph, inVcfIn, inCutoff, inResonance, inMix1, inMix2, inVcMix,
    inModel, inVOct, inLevel, inTrig, inTempo, inPlayStop, inReset, inHold, inEnvGate, inVcaCv, inLfoRate,
    numInputs
};

// Outputs carrying audio (patched per sample into audio inputs)
inline constexpr bool isAudioOutput (int o)
{
    return o == outVcMix || o == outOsc1 || o == outOsc2 || o == outVcaLine || o == outPhones
        || o == outNoise || o == outVcf;
}

inline constexpr std::array<const char*, numOutputs> outputNames {
    "VC MIX", "LFO TRI", "OSC OUT1", "OSC OUT2", "ENV", "VCA/LINE", "PHONES",
    "LFO SQU", "NOISE", "ASSIGN", "KB CV", "GATE", "VCF"
};

inline constexpr std::array<const char*, numInputs> inputNames {
    "OSC TIMBRE", "OSC HAR", "OSC FM", "OSC MORPH", "VCF IN", "VCF CUTOFF", "VCF RES", "MIX 1", "MIX 2", "VC MIX",
    "OSC MODEL", "OSC V/OCT", "OSC LEVEL", "OSC TRIG", "TEMPO", "PLAY/STOP", "RESET", "HOLD", "ENV GATE", "VCA CV", "LFO RATE"
};

// Panel order: each row is 17 jacks, inputs first (boxed labels), then outputs
struct JackSlot { bool isInput; int index; };
constexpr int jacksPerRow = 17;

inline constexpr std::array<JackSlot, 2 * jacksPerRow> panelJacks {{
    { true, inTimbre }, { true, inHarmonics }, { true, inFm }, { true, inMorph }, { true, inVcfIn },
    { true, inCutoff }, { true, inResonance }, { true, inMix1 }, { true, inMix2 }, { true, inVcMix },
    { false, outVcMix }, { false, outLfoTri }, { false, outOsc1 }, { false, outOsc2 }, { false, outEnv },
    { false, outVcaLine }, { false, outPhones },

    { true, inModel }, { true, inVOct }, { true, inLevel }, { true, inTrig }, { true, inTempo },
    { true, inPlayStop }, { true, inReset }, { true, inHold }, { true, inEnvGate }, { true, inVcaCv },
    { true, inLfoRate },
    { false, outLfoSquare }, { false, outNoise }, { false, outAssign }, { false, outKbCv }, { false, outGate },
    { false, outVcf },
}};

// What the ASSIGN output carries
inline constexpr std::array<const char*, 6> assignNames { "Velocity", "Mod Wheel", "Aftertouch", "Accent", "S&H", "Osc Envelope" };

using PatchState = std::array<int, numInputs>; // source output per input, -1 = unpatched (normalled)

} // namespace grind
