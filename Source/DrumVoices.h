#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Synthesised drum voices for Midnight Edge.
//
// The voices are modelled loosely on the LinnDrum kit (tight, tuned, slightly
// gritty) but built from analogue-style synthesis so they can be pushed much
// harder: pitch-swept kicks with a click layer, two-oscillator snares with a
// noise "snappy" layer, multi-burst claps, metallic hats, tom drops and a
// Behringer Edge style FM percussion voice.

namespace me
{
constexpr float pi    = 3.14159265358979f;
constexpr float twoPi = 6.28318530717959f;

inline float semitonesToRatio (float st) { return std::exp2 (st / 12.0f); }

// Coefficient for a one-pole exponential decay with time constant tau (seconds)
inline float tauCoef (float tauSeconds, float sr) { return std::exp (-1.0f / (std::max (tauSeconds, 1.0e-5f) * sr)); }

struct Noise
{
    uint32_t state = 0x9e3779b9u;
    float next() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return (float) (int32_t) state * (1.0f / 2147483648.0f);
    }
};

// Zavalishin TPT state variable filter (lp / bp / hp outputs)
struct SVF
{
    float a1 = 0, a2 = 0, a3 = 0, k = 1, ic1 = 0, ic2 = 0;
    float lp = 0, bp = 0, hp = 0;

    void set (float cutoff, float q, float sr) noexcept
    {
        cutoff = std::clamp (cutoff, 10.0f, sr * 0.45f);
        const float g = std::tan (pi * cutoff / sr);
        k  = 1.0f / std::max (q, 0.05f);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    void process (float x) noexcept
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2;
        bp = v1;
        hp = x - k * v1 - v2;
    }

    void reset() noexcept { ic1 = ic2 = lp = bp = hp = 0; }
};

struct OnePole
{
    float a = 0, z = 0;
    void setCutoff (float hz, float sr) noexcept { a = std::exp (-twoPi * std::min (hz, sr * 0.45f) / sr); }
    float lowpass (float x) noexcept  { z = x + a * (z - x); return z; }
    float highpass (float x) noexcept { return x - lowpass (x); }
    void reset() noexcept { z = 0; }
};

// Exponential decay envelope
struct Env
{
    float value = 0, coef = 0;
    void trigger (float level, float tau, float sr) noexcept { value = level; coef = tauCoef (tau, sr); }
    void setTau (float tau, float sr) noexcept { coef = tauCoef (tau, sr); }
    float next() noexcept { const float v = value; value *= coef; return v; }
};

// Parameters sampled at trigger time (like a hardware drum machine)
struct VoiceParams
{
    float tune  = 0.0f; // semitones
    float decay = 0.5f; // 0..1
    float tone  = 0.5f; // 0..1
    float snap  = 0.5f; // 0..1, voice specific character
};

class Voice
{
public:
    virtual ~Voice() = default;
    virtual void prepare (float sampleRate) { sr = sampleRate; active = false; }
    virtual void trigger (float velocity, const VoiceParams& p) = 0;
    virtual float process() noexcept = 0;
    virtual void choke() noexcept {}
    bool isActive() const noexcept { return active; }

protected:
    // Shared click-free attack ramp (~0.4 ms)
    float attackStep() const noexcept { return 1.0f / (0.0004f * sr); }
    float sr = 44100.0f;
    bool active = false;
};

//==============================================================================
class KickVoice : public Voice
{
public:
    void trigger (float vel, const VoiceParams& p) override
    {
        base     = 47.0f * semitonesToRatio (p.tune);
        depth    = 1.5f + 7.0f * p.snap;                  // punch: how far the pitch sweeps
        pitchEnv.trigger (1.0f, 0.006f + 0.022f * (1.0f - 0.5f * p.snap), sr);
        ampEnv.trigger (1.0f, 0.07f + 0.75f * p.decay * p.decay, sr);
        clickEnv.trigger (1.0f, 0.0025f + 0.002f * p.tone, sr);
        clickLevel = 0.15f + 0.85f * p.tone;
        clickFilter.set (1800.0f + 3000.0f * p.tone, 0.9f, sr);
        gain = vel;
        phase = 0.0f;
        attack = 0.0f;
        active = true;
    }

    float process() noexcept override
    {
        const float pe = pitchEnv.next();
        const float f = base * (1.0f + depth * pe);
        phase += f / sr;
        if (phase >= 1.0f) phase -= 1.0f;

        attack = std::min (1.0f, attack + attackStep());
        const float amp = ampEnv.next() * attack;
        float body = std::sin (twoPi * phase) * amp;
        body = std::tanh (1.6f * body) * 1.08f; // fatten the fundamental a little

        clickFilter.process (noise.next());
        const float click = clickFilter.bp * clickEnv.next() * clickLevel * 2.5f;

        if (amp < 1.0e-4f) active = false;
        return (body + click) * gain;
    }

private:
    float base = 47, depth = 4, phase = 0, gain = 1, clickLevel = 0.5f, attack = 0;
    Env pitchEnv, ampEnv, clickEnv;
    SVF clickFilter;
    Noise noise;
};

//==============================================================================
class SnareVoice : public Voice
{
public:
    void trigger (float vel, const VoiceParams& p) override
    {
        const float r = semitonesToRatio (p.tune);
        f1 = 182.0f * r;
        f2 = 331.0f * r;
        bodyEnv.trigger (1.0f, 0.045f + 0.09f * p.decay, sr);
        noiseEnv.trigger (1.0f, 0.06f + 0.32f * p.decay, sr);
        pitchEnv.trigger (1.0f, 0.012f, sr);
        hp.set (700.0f + 2000.0f * p.tone, 0.7f, sr);
        lp.set (4500.0f + 8000.0f * p.tone, 0.6f, sr);
        bodyLevel  = 1.0f - 0.55f * p.snap;
        noiseLevel = 0.35f + 0.95f * p.snap;
        gain = vel;
        ph1 = ph2 = 0.0f;
        attack = 0.0f;
        active = true;
    }

    float process() noexcept override
    {
        const float bend = 1.0f + 0.5f * pitchEnv.next();
        ph1 += f1 * bend / sr; if (ph1 >= 1.0f) ph1 -= 1.0f;
        ph2 += f2 * bend / sr; if (ph2 >= 1.0f) ph2 -= 1.0f;

        attack = std::min (1.0f, attack + attackStep());
        const float be = bodyEnv.next() * attack;
        const float body = (std::sin (twoPi * ph1) + 0.6f * std::sin (twoPi * ph2)) * 0.7f * be;

        hp.process (noise.next());
        lp.process (hp.hp);
        const float ne = noiseEnv.next() * attack;
        const float n = lp.lp * ne * 1.6f;

        if (be < 1.0e-4f && ne < 1.0e-4f) active = false;
        return std::tanh (1.3f * (body * bodyLevel + n * noiseLevel)) * gain;
    }

private:
    float f1 = 180, f2 = 330, ph1 = 0, ph2 = 0, bodyLevel = 1, noiseLevel = 1, gain = 1, attack = 0;
    Env bodyEnv, noiseEnv, pitchEnv;
    SVF hp, lp;
    Noise noise;
};

//==============================================================================
class ClapVoice : public Voice
{
public:
    void trigger (float vel, const VoiceParams& p) override
    {
        const float r = semitonesToRatio (p.tune);
        bp.set ((900.0f + 1600.0f * p.tone) * r, 1.4f, sr);
        hp.setCutoff (500.0f, sr);
        spacing     = (0.006f + 0.010f * p.snap) * sr;
        tailCoef    = tauCoef (0.05f + 0.38f * p.decay, sr);
        burstCoef   = tauCoef (0.0032f, sr);
        t = 0.0f;
        env = 1.0f;
        burst = 0;
        gain = vel;
        active = true;
    }

    float process() noexcept override
    {
        // Three short bursts followed by the tail, like the classic analogue clap circuit
        t += 1.0f;
        if (burst < 3 && t >= spacing)
        {
            t = 0.0f;
            ++burst;
            env = 1.0f;
        }
        env *= (burst < 3 ? burstCoef : tailCoef);

        bp.process (noise.next());
        const float x = hp.highpass (bp.bp) * env * 2.4f;
        if (burst >= 3 && env < 1.0e-4f) active = false;
        return std::tanh (x) * gain;
    }

private:
    float spacing = 400, tailCoef = 0.99f, burstCoef = 0.9f, t = 0, env = 0, gain = 1;
    int burst = 0;
    SVF bp;
    OnePole hp;
    Noise noise;
};

//==============================================================================
// Metallic hats: six detuned square oscillators mixed with noise, band-passed
class HatVoice : public Voice
{
public:
    explicit HatVoice (bool isOpen) : open (isOpen) {}

    void trigger (float vel, const VoiceParams& p) override
    {
        static constexpr std::array<float, 6> ratios { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };
        const float r = semitonesToRatio (p.tune) * 1.6f;
        for (size_t i = 0; i < ratios.size(); ++i)
            inc[i] = ratios[i] * r / sr;

        const float tau = open ? (0.07f + 0.75f * p.decay * p.decay) : (0.008f + 0.07f * p.decay);
        ampEnv.trigger (1.0f, tau, sr);
        bp.set (7000.0f + 4500.0f * p.tone, 0.9f, sr);
        hp.set (5200.0f + 3500.0f * p.tone, 0.7f, sr);
        metal = 0.25f + 0.75f * p.snap;
        gain = vel;
        attack = 0.0f;
        active = true;
    }

    void choke() noexcept override
    {
        if (active)
            ampEnv.setTau (0.006f, sr);
    }

    float process() noexcept override
    {
        float sq = 0.0f;
        for (size_t i = 0; i < inc.size(); ++i)
        {
            phase[i] += inc[i];
            if (phase[i] >= 1.0f) phase[i] -= 1.0f;
            sq += phase[i] < 0.5f ? 1.0f : -1.0f;
        }

        const float src = sq * (1.0f / 6.0f) * metal + noise.next() * (1.0f - metal * 0.7f);
        bp.process (src);
        hp.process (bp.bp);

        attack = std::min (1.0f, attack + attackStep());
        const float amp = ampEnv.next() * attack;
        if (amp < 1.0e-4f) active = false;
        return hp.hp * amp * 2.2f * gain;
    }

private:
    bool open;
    std::array<float, 6> phase {}, inc {};
    float metal = 0.5f, gain = 1, attack = 0;
    Env ampEnv;
    SVF bp, hp;
    Noise noise;
};

//==============================================================================
class TomVoice : public Voice
{
public:
    explicit TomVoice (float baseFreq) : baseHz (baseFreq) {}

    void trigger (float vel, const VoiceParams& p) override
    {
        base  = baseHz * semitonesToRatio (p.tune);
        depth = 0.25f + 1.3f * p.snap;
        pitchEnv.trigger (1.0f, 0.03f + 0.05f * p.snap, sr);
        ampEnv.trigger (1.0f, 0.1f + 0.55f * p.decay, sr);
        stickEnv.trigger (1.0f, 0.006f, sr);
        stickLevel = 0.1f + 0.6f * p.tone;
        stick.set (2500.0f + 2500.0f * p.tone, 0.8f, sr);
        gain = vel;
        phase = 0.0f;
        attack = 0.0f;
        active = true;
    }

    float process() noexcept override
    {
        const float f = base * (1.0f + depth * pitchEnv.next());
        phase += f / sr;
        if (phase >= 1.0f) phase -= 1.0f;

        attack = std::min (1.0f, attack + attackStep());
        const float amp = ampEnv.next() * attack;
        const float body = (std::sin (twoPi * phase) + 0.15f * std::sin (2.0f * twoPi * phase)) * amp;

        stick.process (noise.next());
        const float s = stick.bp * stickEnv.next() * stickLevel * 2.0f;

        if (amp < 1.0e-4f) active = false;
        return std::tanh (1.4f * (body + s)) * gain;
    }

private:
    float baseHz, base = 100, depth = 0.5f, phase = 0, stickLevel = 0.3f, gain = 1, attack = 0;
    Env pitchEnv, ampEnv, stickEnv;
    SVF stick;
    Noise noise;
};

//==============================================================================
// Behringer Edge flavoured percussion: two oscillators with decaying FM,
// pitch sweep and a resonant low-pass with envelope.
class EdgePercVoice : public Voice
{
public:
    void trigger (float vel, const VoiceParams& p) override
    {
        f1 = 210.0f * semitonesToRatio (p.tune);
        f2 = f1 * 1.47f;
        fmIndex = 7.0f * p.snap;
        fmEnv.trigger (1.0f, 0.02f + 0.1f * p.decay, sr);
        pitchEnv.trigger (1.0f, 0.015f, sr);
        ampEnv.trigger (1.0f, 0.04f + 0.5f * p.decay * p.decay, sr);
        cutoff = 250.0f + 9000.0f * p.tone * p.tone;
        gain = vel;
        ph1 = ph2 = 0.0f;
        attack = 0.0f;
        counter = 0;
        active = true;
    }

    float process() noexcept override
    {
        const float pe = pitchEnv.next();
        ph2 += f2 / sr; if (ph2 >= 1.0f) ph2 -= 1.0f;
        ph1 += f1 * (1.0f + 1.5f * pe) / sr; if (ph1 >= 1.0f) ph1 -= 1.0f;

        const float fe = fmEnv.next();
        const float osc = std::sin (twoPi * ph1 + fmIndex * fe * std::sin (twoPi * ph2));

        attack = std::min (1.0f, attack + attackStep());
        const float amp = ampEnv.next() * attack;

        // Update the swept filter every 16 samples to keep the tan() cost down
        if ((counter++ & 15) == 0)
            filter.set (cutoff * (0.35f + 0.65f * amp) + 80.0f, 2.8f, sr);
        filter.process (osc);

        if (amp < 1.0e-4f) active = false;
        return std::tanh (1.5f * filter.lp) * amp * gain;
    }

private:
    float f1 = 200, f2 = 300, ph1 = 0, ph2 = 0, fmIndex = 2, cutoff = 2000, gain = 1, attack = 0;
    uint32_t counter = 0;
    Env fmEnv, pitchEnv, ampEnv;
    SVF filter;
};

} // namespace me
