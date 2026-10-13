#include "Synth.h"

#include <algorithm>
#include <cmath>

namespace grind::dsp
{
namespace
{
// Plaits computes pitch for its real codec rate (47872 Hz); we run at 48 kHz.
const float pitchCorrection = 12.0f * std::log2 (plaits::kCorrectedSampleRate / plaits::kSampleRate);

inline float clamp01 (float x) noexcept { return std::clamp (x, 0.0f, 1.0f); }
inline float onePoleCoef (float seconds, float samples) noexcept
{
    return seconds <= 0.0f ? 1.0f : 1.0f - std::exp (-samples / (seconds * rate));
}

inline float attackSeconds (float knob) noexcept { return 0.001f * std::pow (10000.0f, clamp01 (knob)); }
inline float decaySeconds (float knob) noexcept { return 0.003f * std::pow (3333.0f, clamp01 (knob)); }
inline float lfoHz (float knob) noexcept { return 0.05f * std::pow (1600.0f, clamp01 (knob)); } // 0.05 .. 80 Hz
} // namespace

//==============================================================================
float LadderFilter::tick (float x, float G, float k, float drive, bool highPass) noexcept
{
    const float oneMinusG = 1.0f - G;
    const float G2 = G * G, G3 = G2 * G, G4 = G2 * G2;
    const float S = oneMinusG * (G3 * state[0] + G2 * state[1] + G * state[2] + state[3]);

    // Bass loss with resonance is a ladder trait; put a little of it back
    const float in = x * drive * (1.0f + 0.35f * k);
    const float estimate = (G4 * in + S) / (1.0f + k * G4);
    float u = std::tanh (in - k * estimate);

    const float input = u;
    float y[4];
    for (int i = 0; i < 4; ++i)
    {
        const float v = (u - state[i]) * G;
        y[i] = v + state[i];
        state[i] = y[i] + v;
        u = y[i];
    }

    if (highPass)
        return input - 4.0f * y[0] + 6.0f * y[1] - 4.0f * y[2] + y[3];
    return y[3];
}

float LadderFilter::process (float x, float g, float k, float drive, bool highPass) noexcept
{
    const float G = g / (1.0f + g);
    const float mid = 0.5f * (previousInput + x);
    previousInput = x;
    const float a = tick (mid, G, k, drive, highPass);
    const float b = tick (x, G, k, drive, highPass);
    return 0.5f * (a + b);
}

//==============================================================================
void Envelope::process (int samples, float attackSecs, float decaySecs, bool sustain) noexcept
{
    switch (stage)
    {
        case attack:
            // RC charge towards an overshoot target, like the analog EG
            value += (1.3f - value) * onePoleCoef (attackSecs * 0.75f, (float) samples);
            if (value >= 1.0f)
            {
                value = 1.0f;
                stage = sustain ? hold : decay;
            }
            break;

        case hold:
            value = 1.0f;
            if (! sustain)
                stage = decay;
            break;

        case decay:
            value += (-0.01f - value) * onePoleCoef (decaySecs * 0.25f, (float) samples);
            if (value <= 0.0f)
            {
                value = 0.0f;
                stage = idle;
            }
            break;

        case idle:
            value = 0.0f;
            break;
    }
}

//==============================================================================
void Voice::init()
{
    osc.init();
    reset();
}

void Voice::reset()
{
    osc.reset();
    filter.reset();
    envelope.reset();
    gate = false;
    note = -1;
    retriggerPending = oscGatePrev = egGatePrev = false;
    started = false;
    previousGain = 0.0f;
    silentBlocks = 1000;
    for (auto& s : sources) s = 0.0f;
}

void Voice::noteOn (int newNote, float vel, bool acc, bool sl, bool retrigger)
{
    note = newNote;
    targetPitch = (float) newNote;
    if (! started)
    {
        pitch = targetPitch;
        started = true;
    }
    velocity = vel;
    accent = acc;
    slide = sl;
    if (retrigger || ! gate)
        retriggerPending = true;
    gate = true;
    silentBlocks = 0;
}

void Voice::noteOff()
{
    gate = false;
}

bool Voice::isSilent() const noexcept
{
    return silentBlocks > 8;
}

void Voice::render (const SynthParams& p, const GlobalModulation& g, float* out, int numSamples)
{
    const float n = (float) numSamples;

    //== Patch bay outputs
    float* src = sources;
    src[srcLfoTri] = g.lfoTri;
    src[srcLfoSquare] = g.lfoSquare;
    src[srcEnvelope] = envelope.value;
    src[srcGate] = gate ? 1.0f : 0.0f;
    src[srcKeyboard] = (pitch - 60.0f) / 60.0f;
    src[srcVelocity] = velocity;
    src[srcModWheel] = g.modWheel;
    src[srcAftertouch] = g.aftertouch;
    src[srcSampleHold] = g.sampleHold;
    src[srcNoise] = g.noise;
    src[srcAccent] = accent ? 1.0f : 0.0f;
    src[srcOscEnvelope] = osc.decayEnvelope();

    //== Patch bay inputs
    float in[numDestinations];
    bool patched[numDestinations];
    for (int d = 0; d < numDestinations; ++d)
    {
        float sum = 0.0f;
        bool any = false;
        for (int s = 0; s < numSources; ++s)
        {
            const float amount = p.patch[(size_t) patchIndex (d, s)];
            if (isPatched (amount))
            {
                sum += amount * src[s];
                any = true;
            }
        }
        in[d] = sum;
        patched[d] = any;
    }

    // A Poly Chain voice that has never been played stays quiet, even with the VCA set to ON.
    // The first voice always runs, like the hardware (drones, self-patched triggers).
    if (! started && ! primary)
        return;

    //== Gates (the trigger inputs are OR'd with the keyboard gate)
    const bool oscGate = gate || (patched[dstOscTrig] && in[dstOscTrig] > 0.5f);
    const bool egGate = gate || (patched[dstEgTrig] && in[dstEgTrig] > 0.5f);
    const bool retrigger = retriggerPending;
    retriggerPending = false;

    if (retrigger || (egGate && ! egGatePrev))
        envelope.gateOn();
    else if (! egGate && egGatePrev)
        envelope.gateOff();
    egGatePrev = egGate;

    const bool oscRetrigger = retrigger || (oscGate && ! oscGatePrev);
    oscGatePrev = oscGate;

    // Idle voices cost nothing
    const bool vcaPatched = patched[dstVca];
    if (! p.vcaOn && ! vcaPatched && envelope.stage == Envelope::idle && ! egGate && ! oscGate)
    {
        if (++silentBlocks > 8)
        {
            previousGain = 0.0f;
            return;
        }
    }
    else
    {
        silentBlocks = 0;
    }

    //== Envelope
    const float egStart = envelope.value;
    envelope.process (numSamples,
                      attackSeconds (p.attack + in[dstAttack]),
                      decaySeconds (p.decay + in[dstDecay]),
                      p.sustain);

    //== Glide
    float glideTime = 3.0f * std::pow (clamp01 (p.glide + in[dstGlide]), 3.0f);
    if (slide)
        glideTime = std::max (glideTime, 0.05f);
    pitch += (targetPitch - pitch) * onePoleCoef (glideTime, n);

    //== VCO / VCF mod busses (source switch: EG or LFO)
    const float lfoOut = p.lfoSquare ? g.lfoSquare : g.lfoTri;
    const float vcoSrc = p.vcoModSource == modFromEnvelope ? egStart : lfoOut;
    const float vcoAmt = clamp01 (p.vcoModAmount + in[dstVcoMod]);
    const float vcfSrc = p.vcfModSource == modFromEnvelope ? egStart : lfoOut;
    const float vcfAmt = clamp01 (p.vcfModAmount + in[dstVcfMod]) * (p.vcfModInvert ? -1.0f : 1.0f);

    //== Oscillator
    MacroOscillator::Params op;
    op.engine = std::clamp (p.engine + (int) std::lround (in[dstModel] * (float) numEngines), 0, numEngines - 1);
    op.dx7Bank = p.dx7Bank;
    op.note = pitch + p.frequency + p.fine * 0.01f + g.bend * p.bendRange + in[dstPitch] * 60.0f + pitchCorrection;
    op.harmonics = clamp01 (p.harmonics + in[dstHarmonics]);
    op.timbre = p.timbre;
    op.morph = p.morph;

    switch (p.vcoModDest)
    {
        case vcoToFrequency: op.note += vcoAmt * vcoAmt * 48.0f * vcoSrc; break;
        case vcoToHarmonics: op.harmonics = clamp01 (op.harmonics + vcoAmt * vcoSrc); break;
        case vcoToTimbre:    op.timbre = clamp01 (op.timbre + vcoAmt * vcoSrc); break;
        case vcoToMorph:     op.morph = clamp01 (op.morph + vcoAmt * vcoSrc); break;
        default: break;
    }

    op.fmAmount = p.fmAmount;
    op.timbreAmount = p.timbreAmount;
    op.morphAmount = p.morphAmount;
    op.fmPatched = patched[dstFm];
    op.timbrePatched = patched[dstTimbre];
    op.morphPatched = patched[dstMorph];
    op.fmCv = in[dstFm] * 60.0f;
    op.timbreCv = in[dstTimbre] * 0.8f;
    op.morphCv = in[dstMorph] * 0.8f;
    op.decay = clamp01 (p.oscDecay + in[dstOscDecay]);
    op.lpgColour = clamp01 (p.lpgColour + in[dstLpgColour]);
    op.lpgOn = p.lpgOn;
    op.accent = accent ? 1.0f : 0.35f + 0.55f * velocity;

    osc.render (op, oscGate, oscRetrigger, outBuffer, auxBuffer, (size_t) numSamples);

    const float mix = clamp01 (p.mix + in[dstMix]);
    const float oscLevel = std::clamp (1.0f + in[dstOscLevel], 0.0f, 2.0f);

    //== VCF
    const float octaves = vcfAmt * 7.0f * vcfSrc + in[dstCutoff] * 5.0f + p.keyTrack * (pitch - 60.0f) / 12.0f;
    const float cutoff = std::clamp (p.cutoffHz * std::exp2 (octaves), 8.0f, 20000.0f);
    const float targetG = std::tan (3.14159265f * std::min (cutoff / (2.0f * rate), 0.24f));
    const float k = 4.3f * clamp01 (p.resonance + in[dstResonance]);
    const float drive = 0.5f + 3.0f * clamp01 (p.drive + in[dstDrive]);
    const float makeup = 1.0f / std::min (1.0f, 0.4f + 0.35f * drive);

    //== VCA
    float gain = p.vcaOn ? 1.0f : envelope.value;
    gain = clamp01 (gain + in[dstVca]);
    gain *= 1.0f - p.velocity + p.velocity * velocity;

    const float gStep = (targetG - previousG) / n;
    const float gainStep = (gain - previousGain) / n;
    float gCoef = previousG, vca = previousGain;

    for (int i = 0; i < numSamples; ++i)
    {
        gCoef += gStep;
        vca += gainStep;
        const float x = (outBuffer[i] + (auxBuffer[i] - outBuffer[i]) * mix) * oscLevel;
        out[i] += filter.process (x, gCoef, k, drive, p.highPass) * makeup * vca;
    }

    previousG = targetG;
    previousGain = gain;
}

//==============================================================================
Synth::Synth()
{
    for (auto& v : voices)
        v = std::make_unique<Voice>();
    voices[0]->primary = true;
}

void Synth::init()
{
    for (auto& v : voices)
        v->init();
    reset();
}

void Synth::reset()
{
    for (auto& v : voices)
        v->reset();
    numHeld = 0;
    lfoPhase = 0.0f;
    lfoSquareSmooth = 0.0f;
}

int Synth::pickMonoNote() const noexcept
{
    if (numHeld == 0)
        return -1;
    int chosen = heldNotes[(size_t) numHeld - 1];
    for (int i = 0; i < numHeld; ++i)
    {
        const int nn = heldNotes[(size_t) i];
        if (params.priority == priorityLow && nn < chosen) chosen = nn;
        if (params.priority == priorityHigh && nn > chosen) chosen = nn;
    }
    return chosen;
}

Voice& Synth::allocateVoice (int note)
{
    // Same note: reuse its voice
    for (int i = 0; i < activeVoices; ++i)
        if (voices[(size_t) i]->note == note)
            return *voices[(size_t) i];

    // Oldest released voice
    Voice* best = nullptr;
    for (int i = 0; i < activeVoices; ++i)
    {
        auto* v = voices[(size_t) i].get();
        if (! v->gate && (best == nullptr || v->age < best->age))
            best = v;
    }
    if (best != nullptr)
        return *best;

    // Steal: oldest (last priority), highest (low priority) or lowest (high priority)
    best = voices[0].get();
    for (int i = 1; i < activeVoices; ++i)
    {
        auto* v = voices[(size_t) i].get();
        if (params.priority == priorityLow ? v->note > best->note
            : params.priority == priorityHigh ? v->note < best->note
            : v->age < best->age)
            best = v;
    }
    return *best;
}

void Synth::noteOn (int note, float velocity, bool accent, bool slide)
{
    if (activeVoices == 1)
    {
        // Mono: note stack with priority, legato returns
        for (int i = 0; i < numHeld; ++i)
            if (heldNotes[(size_t) i] == note)
            {
                for (int j = i; j < numHeld - 1; ++j)
                {
                    heldNotes[(size_t) j] = heldNotes[(size_t) j + 1];
                    heldVelocities[(size_t) j] = heldVelocities[(size_t) j + 1];
                }
                --numHeld;
                break;
            }
        if (numHeld == (int) heldNotes.size())
        {
            std::copy (heldNotes.begin() + 1, heldNotes.end(), heldNotes.begin());
            std::copy (heldVelocities.begin() + 1, heldVelocities.end(), heldVelocities.begin());
            --numHeld;
        }
        heldNotes[(size_t) numHeld] = note;
        heldVelocities[(size_t) numHeld] = velocity;
        ++numHeld;

        auto& v = *voices[0];
        if (pickMonoNote() == note)
        {
            const bool legato = slide && v.gate;
            v.noteOn (note, velocity, accent, slide, ! legato);
            v.age = ++ageCounter;
        }
        return;
    }

    auto& v = allocateVoice (note);
    v.noteOn (note, velocity, accent, false, true);
    v.age = ++ageCounter;
}

void Synth::noteOff (int note)
{
    if (activeVoices == 1)
    {
        for (int i = 0; i < numHeld; ++i)
            if (heldNotes[(size_t) i] == note)
            {
                for (int j = i; j < numHeld - 1; ++j)
                {
                    heldNotes[(size_t) j] = heldNotes[(size_t) j + 1];
                    heldVelocities[(size_t) j] = heldVelocities[(size_t) j + 1];
                }
                --numHeld;
                break;
            }

        auto& v = *voices[0];
        const int next = pickMonoNote();
        if (next < 0)
        {
            if (v.note == note)
                v.noteOff();
        }
        else if (next != v.note)
        {
            float vel = 0.8f;
            for (int i = 0; i < numHeld; ++i)
                if (heldNotes[(size_t) i] == next)
                    vel = heldVelocities[(size_t) i];
            v.noteOn (next, vel, false, false, false);
        }
        return;
    }

    for (int i = 0; i < maxVoices; ++i)
        if (voices[(size_t) i]->note == note && voices[(size_t) i]->gate)
            voices[(size_t) i]->noteOff();
}

void Synth::allNotesOff()
{
    numHeld = 0;
    for (auto& v : voices)
        v->noteOff();
}

float Synth::getEnvelopeLevel() const noexcept
{
    float level = 0.0f;
    for (int i = 0; i < activeVoices; ++i)
        level = std::max (level, voices[(size_t) i]->sources[srcEnvelope]);
    return level;
}

void Synth::render (float* out, int numSamples)
{
    for (int i = 0; i < numSamples; i += blockSize)
        renderBlock (out + i);
}

void Synth::setParams (const SynthParams& p) noexcept
{
    params = p;

    // Voice count changes (Poly Chain size) release the voices that drop out
    const int wanted = std::clamp (params.voices, 1, maxVoices);
    if (wanted != activeVoices)
    {
        for (int i = wanted; i < maxVoices; ++i)
            voices[(size_t) i]->reset();
        if (wanted == 1 || activeVoices == 1)
        {
            numHeld = 0;
            voices[0]->noteOff();
        }
        activeVoices = wanted;
    }
}

void Synth::renderBlock (float* out)
{

    //== LFO (shared by every voice), rate patchable from the first voice's bay
    const auto* src0 = voices[0]->sources;
    float rateMod = 0.0f;
    for (int s = 0; s < numSources; ++s)
        rateMod += params.patch[(size_t) patchIndex (dstLfoRate, s)] * src0[s];

    const float hz = lfoHz (params.lfoRate) * std::exp2 (4.0f * rateMod);
    lfoPhase += hz * (float) blockSize / rate;
    if (lfoPhase >= 1.0f)
    {
        lfoPhase -= std::floor (lfoPhase);
        noiseState = noiseState * 1664525u + 1013904223u;
        sampleHold = (float) (noiseState >> 8) / 8388608.0f - 1.0f;
    }
    const float tri = 4.0f * std::abs (lfoPhase - 0.5f) - 1.0f;
    const float square = lfoPhase < 0.5f ? 1.0f : -1.0f;
    lfoSquareSmooth += (square - lfoSquareSmooth) * onePoleCoef (0.0007f, (float) blockSize);
    lfoOutput = params.lfoSquare ? lfoSquareSmooth : -tri;

    noiseState = noiseState * 1664525u + 1013904223u;

    GlobalModulation g;
    g.lfoTri = -tri;
    g.lfoSquare = lfoSquareSmooth;
    g.sampleHold = sampleHold;
    g.noise = (float) (noiseState >> 8) / 8388608.0f - 1.0f;
    g.modWheel = modWheel;
    g.aftertouch = aftertouch;
    g.bend = bend;

    std::fill (out, out + blockSize, 0.0f);
    for (int i = 0; i < activeVoices; ++i)
        voices[(size_t) i]->render (params, g, out, blockSize);

    // Keep a full Poly Chain chord in the same range as one voice
    const float targetGain = 1.0f / std::sqrt ((float) activeVoices);
    const float step = (targetGain - polyGain) / (float) blockSize;
    for (int i = 0; i < blockSize; ++i)
    {
        polyGain += step;
        out[i] *= polyGain;
    }
}

} // namespace grind::dsp
