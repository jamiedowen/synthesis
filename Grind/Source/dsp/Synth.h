// The Grind's analog path and voice architecture around the digital
// oscillator: 24 dB ladder VCF (LP / HP), VCA, attack/decay envelope with
// sustain switch, triangle/square LFO, glide, the patch matrix, and up to 16
// voices (the Poly Chain).
//
// Everything here runs at the oscillator rate (48 kHz) in blocks of
// plaits::kBlockSize samples.

#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "../PatchMatrix.h"
#include "MacroOscillator.h"

namespace grind::dsp
{
constexpr float rate = 48000.0f; // plaits::kSampleRate
constexpr int blockSize = (int) plaits::kBlockSize;
constexpr int maxVoices = 16;

enum ModSource { modFromEnvelope, modFromLfo };
enum VcoModDest { vcoToFrequency, vcoToHarmonics, vcoToTimbre, vcoToMorph };
enum Priority { priorityLast, priorityLow, priorityHigh };

struct SynthParams
{
    // Oscillator
    int engine = 0, dx7Bank = 0;
    float frequency = 0.0f, fine = 0.0f;              // semitones, cents
    float harmonics = 0.5f, timbre = 0.5f, morph = 0.5f;
    float fmAmount = 0.0f, timbreAmount = 0.0f, morphAmount = 0.0f;
    float oscDecay = 0.5f, lpgColour = 0.5f, mix = 0.0f;
    bool lpgOn = false;

    // VCO mod
    float vcoModAmount = 0.0f;
    int vcoModSource = modFromLfo, vcoModDest = vcoToFrequency;

    // VCF
    float cutoffHz = 2000.0f, resonance = 0.0f, vcfModAmount = 0.0f, keyTrack = 0.0f, drive = 0.3f;
    int vcfModSource = modFromEnvelope;
    bool vcfModInvert = false, highPass = false;

    // VCA / EG
    bool vcaOn = false, sustain = true;
    float attack = 0.0f, decay = 0.4f;                // 0..1 knob positions
    float velocity = 0.0f;                            // velocity -> VCA amount

    // LFO
    float lfoRate = 0.5f;                             // knob position
    bool lfoSquare = false;

    float glide = 0.0f;
    float bendRange = 2.0f;
    int voices = 1, priority = priorityLast;

    std::array<float, numPatchPoints> patch {};
};

//==============================================================================
// Zero-delay-feedback 4-pole transistor ladder (2x oversampled), with the
// classic multimode tap mix for the high-pass setting.
class LadderFilter
{
public:
    void reset() noexcept { for (auto& s : state) s = 0.0f; previousInput = 0.0f; }

    // g = tan(pi * fc / (2 * rate)) for the oversampled rate, k = 0..4.3
    float process (float x, float g, float k, float drive, bool highPass) noexcept;

private:
    float tick (float x, float G, float k, float drive, bool highPass) noexcept;
    float state[4] {};
    float previousInput = 0.0f;
};

//==============================================================================
// Attack / decay envelope with the sustain switch: with SUSTAIN on it holds
// at full level while the gate is high, and decays when the gate is released.
class Envelope
{
public:
    enum Stage { idle, attack, hold, decay };

    void reset() noexcept { stage = idle; value = 0.0f; }
    void gateOn() noexcept { stage = attack; }
    void gateOff() noexcept { if (stage == attack || stage == hold) stage = decay; }
    void process (int samples, float attackSeconds, float decaySeconds, bool sustain) noexcept;

    float value = 0.0f;
    Stage stage = idle;
};

//==============================================================================
struct GlobalModulation
{
    float lfoTri = 0, lfoSquare = 0, sampleHold = 0, noise = 0;
    float modWheel = 0, aftertouch = 0, bend = 0;
};

class Voice
{
public:
    void init();
    void reset();

    void noteOn (int note, float velocity, bool accent, bool slide, bool retrigger);
    void noteOff();
    void render (const SynthParams&, const GlobalModulation&, float* out, int numSamples);

    bool isSilent() const noexcept;
    int note = -1;
    bool gate = false, primary = false;
    uint32_t age = 0;
    float sources[numSources] {};          // last block's patch-bay outputs (for the editor)

private:
    MacroOscillator osc;
    LadderFilter filter;
    Envelope envelope;

    float pitch = 60.0f, targetPitch = 60.0f;
    float velocity = 0.8f;
    bool accent = false, slide = false;
    bool retriggerPending = false, oscGatePrev = false, egGatePrev = false;
    bool started = false;
    float previousGain = 0.0f, previousG = 0.1f;
    int silentBlocks = 0;

    float outBuffer[plaits::kMaxBlockSize] {}, auxBuffer[plaits::kMaxBlockSize] {};
};

//==============================================================================
class Synth
{
public:
    Synth();
    void init();
    void reset();

    void setParams (const SynthParams& p) noexcept;
    const SynthParams& getParams() const noexcept { return params; }

    void noteOn (int note, float velocity, bool accent = false, bool slide = false);
    void noteOff (int note);
    void allNotesOff();
    void setPitchBend (float normalised) noexcept { bend = normalised; }
    void setModWheel (float v) noexcept { modWheel = v; }
    void setAftertouch (float v) noexcept { aftertouch = v; }

    // Renders numSamples (a multiple of blockSize) mono samples into out
    void render (float* out, int numSamples);

    float getLfoValue() const noexcept { return lfoOutput; }
    float getEnvelopeLevel() const noexcept;
    int getActiveEngine() const noexcept { return params.engine; }

private:
    void renderBlock (float* out);
    int pickMonoNote() const noexcept;
    Voice& allocateVoice (int note);

    SynthParams params;
    std::array<std::unique_ptr<Voice>, maxVoices> voices;
    int activeVoices = 1;

    // Mono note stack
    std::array<int, 32> heldNotes {};
    std::array<float, 32> heldVelocities {};
    int numHeld = 0;

    // LFO
    float lfoPhase = 0.0f, lfoSquareSmooth = 0.0f, lfoOutput = 0.0f, sampleHold = 0.0f;
    uint32_t noiseState = 22222;

    float bend = 0.0f, modWheel = 0.0f, aftertouch = 0.0f;
    float polyGain = 1.0f;
    uint32_t ageCounter = 0;
};

} // namespace grind::dsp
