#pragma once

#include <array>

namespace grind
{
// The patch bay, modelled as a 22 x 12 matrix: 12 outputs (sources) can be
// patched into 22 inputs (destinations), each patch with its own level.
// Everything is normalled, so the synth plays with no patches at all.

enum Source
{
    srcLfoTri, srcLfoSquare, srcEnvelope, srcGate, srcKeyboard, srcVelocity,
    srcModWheel, srcAftertouch, srcSampleHold, srcNoise, srcAccent, srcOscEnvelope,
    numSources
};

enum Destination
{
    dstPitch, dstFm, dstHarmonics, dstTimbre, dstMorph, dstModel,
    dstOscLevel, dstOscDecay, dstLpgColour, dstMix, dstCutoff, dstResonance,
    dstVcfMod, dstVca, dstAttack, dstDecay, dstLfoRate, dstGlide,
    dstVcoMod, dstDrive, dstOscTrig, dstEgTrig,
    numDestinations
};

constexpr int numPatchPoints = (int) numSources * (int) numDestinations;

inline constexpr std::array<const char*, numSources> sourceNames {
    "LFO TRI", "LFO SQR", "EG", "GATE", "KB CV", "VELO",
    "MOD WHL", "AFTERT", "S&H", "NOISE", "ACCENT", "OSC ENV"
};

inline constexpr std::array<const char*, numDestinations> destinationNames {
    "PITCH", "FM", "HARMONICS", "TIMBRE", "MORPH", "MODEL",
    "OSC LEVEL", "OSC DECAY", "LPG COLOUR", "OUT/AUX MIX", "CUTOFF", "RESONANCE",
    "VCF MOD", "VCA", "ATTACK", "DECAY", "LFO RATE", "GLIDE",
    "VCO MOD", "DRIVE", "OSC TRIG", "EG TRIG"
};

inline constexpr int patchIndex (int destination, int source) { return destination * (int) numSources + source; }

// A patch point with a level this small counts as unpatched
inline bool isPatched (float amount) noexcept { return amount > 1.0e-4f || amount < -1.0e-4f; }

} // namespace grind
