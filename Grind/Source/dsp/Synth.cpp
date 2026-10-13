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
void Envelope::process (int samples, float attackSecs, float decaySecs, float sustainLevel) noexcept
{
    switch (stage)
    {
        case attack:
            // RC charge towards an overshoot target, like the analog EG
            value += (1.3f - value) * onePoleCoef (attackSecs * 0.75f, (float) samples);
            if (value >= 1.0f)
            {
                value = 1.0f;
                stage = decay;
            }
            break;

        case decay:
            // Fall to the SUSTAIN level and sit there while the gate is held
            value += (sustainLevel - 0.01f - value) * onePoleCoef (decaySecs * 0.25f, (float) samples);
            value = std::max (value, sustainLevel);
            if (sustainLevel <= 0.0f && value <= 0.0f)
            {
                value = 0.0f;
                stage = idle;
            }
            break;

        case release:
            // Release uses the DECAY time
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
    noiseState = 0x9e3779b9u ^ (uint32_t) reinterpret_cast<uintptr_t> (this);
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
    for (auto& x : outputs) x = 0.0f;
    for (auto& x : inputs) x = 0.0f;
    lastOsc1 = lastOsc2 = lastMix = lastVcf = lastVca = 0.0f;
    lpgState[0] = lpgState[1] = 0.0f;
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

float Voice::assignValue (const SynthParams& p, const GlobalModulation& g) const noexcept
{
    switch (p.assignSource)
    {
        case 0:  return velocity;
        case 1:  return g.modWheel;
        case 2:  return g.aftertouch;
        case 3:  return accent ? 1.0f : 0.0f;
        case 4:  return g.sampleHold;
        default: return osc.decayEnvelope();
    }
}

void Voice::render (const SynthParams& p, const GlobalModulation& g, float* out, int numSamples)
{
    const float n = (float) numSamples;

    //== Patch bay outputs (control-rate values; audio outputs give their latest sample)
    float* o = outputs;
    o[outLfoTri] = g.lfoTri;
    o[outLfoSquare] = g.lfoSquare;
    o[outEnv] = envelope.value;
    o[outGate] = gate ? 1.0f : 0.0f;
    o[outKbCv] = (pitch - 60.0f) / 60.0f;
    o[outAssign] = assignValue (p, g);
    o[outNoise] = g.noise;
    o[outOsc1] = lastOsc1;
    o[outOsc2] = lastOsc2;
    o[outVcMix] = lastMix;
    o[outVcf] = lastVcf;
    o[outVcaLine] = o[outPhones] = lastVca;

    //== Patch bay inputs (control rate)
    const auto& patch = p.patch;
    auto patched = [&] (int input) { return patch[(size_t) input] >= 0; };
    float in[numInputs];
    for (int i = 0; i < numInputs; ++i)
        in[i] = patched (i) ? o[patch[(size_t) i]] : 0.0f;
    for (int i = 0; i < numInputs; ++i)
        inputs[i] = in[i];

    // A Poly Chain voice that has never been played stays quiet, even with the VCA set to ON.
    // The first voice always runs, like the hardware (drones, self-patched triggers).
    if (! started && ! primary)
        return;

    //== Gates: a cable into TRIG / ENV GATE replaces the keyboard gate
    const bool oscGate = patched (inTrig) ? in[inTrig] > 0.25f : gate;
    const bool egGate = patched (inEnvGate) ? in[inEnvGate] > 0.25f : gate;
    const bool retrigger = retriggerPending;
    retriggerPending = false;

    if ((retrigger && ! patched (inEnvGate)) || (egGate && ! egGatePrev))
        envelope.gateOn();
    else if (! egGate && egGatePrev)
        envelope.gateOff();
    egGatePrev = egGate;

    const bool oscRetrigger = (retrigger && ! patched (inTrig)) || (oscGate && ! oscGatePrev);
    oscGatePrev = oscGate;

    // Idle voices cost nothing
    const bool vcaOn = p.vcaMode == vcaModeOn;
    if (! vcaOn && ! patched (inVcaCv) && envelope.stage == Envelope::idle && ! egGate && ! oscGate)
    {
        if (++silentBlocks > 8)
        {
            previousGain = 0.0f;
            lastVca = 0.0f;
            return;
        }
    }
    else
    {
        silentBlocks = 0;
    }

    //== Envelope
    const float egStart = envelope.value;
    envelope.process (numSamples, attackSeconds (p.attack), decaySeconds (p.decay), clamp01 (p.sustain));

    //== Glide
    float glideTime = 3.0f * std::pow (clamp01 (p.glide), 3.0f);
    if (slide)
        glideTime = std::max (glideTime, 0.05f);
    pitch += (targetPitch - pitch) * onePoleCoef (glideTime, n);

    //== VCO (vibrato) / VCF mod busses
    const float lfoOut = p.lfoSquare ? g.lfoSquare : g.lfoTri;
    const float vcoSrc = p.vcoModSource == modFromEnvelope ? egStart : lfoOut;
    const float vcoAmt = clamp01 (p.vcoModAmount);
    const float vcfSrc = p.vcfModSource == modFromEnvelope ? egStart : lfoOut;
    const float vcfAmt = clamp01 (p.vcfModAmount) * (p.vcfModInvert ? -1.0f : 1.0f);

    //== Oscillator
    MacroOscillator::Params op;
    op.engine = std::clamp (p.engine + (int) std::lround (in[inModel] * (float) numEngines), 0, numEngines - 1);
    op.dx7Bank = p.dx7Bank;
    op.note = pitch + p.frequency + p.fine * 0.01f + g.bend * p.bendRange + in[inVOct] * 60.0f + pitchCorrection;
    op.harmonics = clamp01 (p.harmonics + in[inHarmonics] * 0.5f);
    op.timbre = p.timbre;
    op.morph = p.morph;

    switch (p.vcoModDest)
    {
        case vcoToFrequency: op.note += vcoAmt * vcoAmt * 12.0f * vcoSrc; break;
        case vcoToHarmonics: op.harmonics = clamp01 (op.harmonics + vcoAmt * vcoSrc); break;
        case vcoToTimbre:    op.timbre = clamp01 (op.timbre + vcoAmt * vcoSrc); break;
        case vcoToMorph:     op.morph = clamp01 (op.morph + vcoAmt * vcoSrc); break;
        default: break;
    }

    op.fmAmount = p.fmAmount;
    op.timbreAmount = p.timbreAmount;
    op.morphAmount = p.morphAmount;
    op.fmPatched = patched (inFm);
    op.timbrePatched = patched (inTimbre);
    op.morphPatched = patched (inMorph);
    op.fmCv = in[inFm] * 60.0f;
    op.timbreCv = in[inTimbre] * 0.8f;
    op.morphCv = in[inMorph] * 0.8f;
    op.decay = clamp01 (p.oscDecay);
    op.lpgColour = clamp01 (p.lpgColour);
    op.lpgOn = p.lpgOn;
    op.accent = accent ? 1.0f : 0.35f + 0.55f * velocity;

    osc.render (op, oscGate, oscRetrigger, outBuffer, auxBuffer, (size_t) numSamples);

    // OSC LEVEL: a cable sets the oscillator level (0-5 V)
    const float oscLevel = patched (inLevel) ? clamp01 (in[inLevel]) : 1.0f;
    const float mix = clamp01 (p.mix + in[inVcMix]);

    //== VCF
    const float octaves = vcfAmt * 7.0f * vcfSrc + in[inCutoff] * 5.0f + p.keyTrack * (pitch - 60.0f) / 12.0f;
    const float cutoff = std::clamp (p.cutoffHz * std::exp2 (octaves), 8.0f, 20000.0f);
    const float targetG = std::tan (3.14159265f * std::min (cutoff / (2.0f * rate), 0.24f));
    const float k = 4.3f * clamp01 (p.resonance + in[inResonance]);
    const float drive = 0.5f + 3.0f * clamp01 (p.drive);
    const float makeup = 1.0f / std::min (1.0f, 0.4f + 0.35f * drive);

    //== VCA: ENV, LPG (envelope-driven vactrol low-pass gate) or ON; VCA CV replaces the envelope
    const float control = patched (inVcaCv) ? clamp01 (in[inVcaCv]) : envelope.value;
    float gain = vcaOn ? 1.0f : control;
    gain *= 1.0f - p.velocity + p.velocity * velocity;
    const bool lpgMode = p.vcaMode == vcaModeLpg;
    const float lpgCoef = 1.0f - std::exp (-6.2831853f * std::min (40.0f * std::exp2 (9.0f * control), 18000.0f) / rate);

    // Audio-rate cables into MIX 1 / MIX 2 / VCF IN
    const int mix1Src = patch[(size_t) inMix1], mix2Src = patch[(size_t) inMix2], vcfInSrc = patch[(size_t) inVcfIn];

    const float gStep = (targetG - previousG) / n;
    const float gainStep = (gain - previousGain) / n;
    float gCoef = previousG, vca = previousGain;

    for (int i = 0; i < numSamples; ++i)
    {
        gCoef += gStep;
        vca += gainStep;

        const float osc1 = outBuffer[i] * oscLevel, osc2 = auxBuffer[i] * oscLevel;
        noiseState = noiseState * 1664525u + 1013904223u;
        const float noise = (float) (noiseState >> 8) / 8388608.0f - 1.0f;

        auto audio = [&] (int src) -> float
        {
            switch (src)
            {
                case outOsc1:    return osc1;
                case outOsc2:    return osc2;
                case outNoise:   return noise;
                case outVcMix:   return lastMix;
                case outVcf:     return lastVcf;
                case outVcaLine:
                case outPhones:  return lastVca;
                default:         return o[src];
            }
        };

        const float m1 = mix1Src >= 0 ? audio (mix1Src) : osc1;
        const float m2 = mix2Src >= 0 ? audio (mix2Src) : osc2;
        const float mixed = m1 + (m2 - m1) * mix;
        lastMix = mixed;

        const float vcfIn = vcfInSrc >= 0 ? audio (vcfInSrc) : mixed;
        const float filtered = filter.process (vcfIn, gCoef, k, drive, p.highPass) * makeup;
        lastVcf = filtered;

        float y = filtered;
        if (lpgMode)
        {
            lpgState[0] += (y - lpgState[0]) * lpgCoef;
            lpgState[1] += (lpgState[0] - lpgState[1]) * lpgCoef;
            y = lpgState[1];
        }
        y *= vca;
        lastVca = y;
        lastOsc1 = osc1;
        lastOsc2 = osc2;
        out[i] += y;
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
        level = std::max (level, voices[(size_t) i]->outputs[outEnv]);
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

    //== LFO (shared by every voice); its LFO RATE input is read from the first voice's bay
    const float rateMod = params.patch[inLfoRate] >= 0 ? voices[0]->inputs[inLfoRate] : 0.0f;

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
