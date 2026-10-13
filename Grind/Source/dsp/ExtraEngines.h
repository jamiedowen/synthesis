// Engines that the Grind adds on top of the Mutable Instruments Plaits set:
// FM Drum, Cowbell, Tom, TD-3 Bassline, Wave Generator and Vox, plus the
// "Dust" view of the Plaits particle engine.
//
// They implement the Plaits Engine interface, so the macro oscillator runs
// them at the Plaits rate (48 kHz) in Plaits-sized blocks, with the same
// HARMONICS / TIMBRE / MORPH / trigger / accent conventions.

#pragma once

#include <algorithm>
#include <cmath>

#include "plaits/dsp/engine/engine.h"
#include "plaits/dsp/engine/particle_engine.h"
#include "plaits/dsp/drums/analog_bass_drum.h"
#include "plaits/dsp/drums/synthetic_bass_drum.h"
#include "plaits/dsp/oscillator/sine_oscillator.h"
#include "stmlib/dsp/filter.h"
#include "stmlib/dsp/polyblep.h"
#include "stmlib/utils/random.h"

namespace grind::dsp
{
using plaits::EngineParameters;

inline float sinTwoPi (float phase) noexcept { return plaits::Sine (phase - std::floor (phase)); }

// Time constant (seconds) -> one-pole coefficient at the Plaits rate
inline float decayCoef (float seconds) noexcept
{
    return 1.0f - std::exp (-1.0f / (std::max (seconds, 1.0e-4f) * plaits::kSampleRate));
}

inline float wrap01 (float x) noexcept { return x - std::floor (x); }

// Polyblep sawtooth / pulse pair sharing one phase (one sample of latency,
// the same scheme the Plaits oscillators use)
struct BlepOscillator
{
    float phase = 0.0f, nextSaw = 0.0f, nextSquare = 0.0f;

    void reset() noexcept { phase = 0.0f; nextSaw = nextSquare = 0.0f; }

    // f: cycles per sample, pw: pulse width (0..1)
    void next (float f, float pw, float& saw, float& square) noexcept
    {
        using namespace stmlib;
        f = std::clamp (f, 1.0e-6f, 0.45f);
        pw = std::clamp (pw, 2.0f * f, 1.0f - 2.0f * f);

        float thisSaw = nextSaw, thisSquare = nextSquare;
        nextSaw = nextSquare = 0.0f;

        const float previous = phase;
        phase += f;
        const bool wrapped = phase >= 1.0f;
        if (wrapped)
        {
            phase -= 1.0f;
            const float t = phase / f;
            thisSaw -= ThisBlepSample (t);
            nextSaw -= NextBlepSample (t);
            thisSquare += ThisBlepSample (t);
            nextSquare += NextBlepSample (t);
        }
        else if (previous < pw && phase >= pw)
        {
            const float t = (phase - pw) / f;
            thisSquare -= ThisBlepSample (t);
            nextSquare -= NextBlepSample (t);
        }

        nextSaw += phase;
        nextSquare += phase < pw ? 1.0f : 0.0f;

        saw = 2.0f * thisSaw - 1.0f;
        square = 2.0f * thisSquare - 1.0f;
    }
};

//==============================================================================
// Plaits' particle engine with OUT and AUX swapped: raw dust on the main out.
class DustEngine : public plaits::Engine
{
public:
    explicit DustEngine (plaits::ParticleEngine& p) : particle (p) {}
    void Init (stmlib::BufferAllocator* a) override { particle.Init (a); }
    void Reset() override { particle.Reset(); }
    void LoadUserData (const uint8_t* d) override { particle.LoadUserData (d); }
    void Render (const EngineParameters& p, float* out, float* aux, size_t size, bool* env) override
    {
        particle.Render (p, aux, out, size, env);
    }

private:
    plaits::ParticleEngine& particle;
};

//==============================================================================
// Two-operator FM percussion: pitch-swept carrier, decaying modulation index.
class FMDrumEngine : public plaits::Engine
{
public:
    void Init (stmlib::BufferAllocator*) override { Reset(); }
    void Reset() override { carrier = modulator = 0.0f; amp = modEnv = pitchEnv = 0.0f; fb = 0.0f; hp.Init(); }
    void LoadUserData (const uint8_t*) override {}

    void Render (const EngineParameters& p, float* out, float* aux, size_t size, bool*) override
    {
        static constexpr float ratios[] = { 0.5f, 1.0f, 1.41f, 1.5f, 2.0f, 2.76f, 3.5f, 4.2f, 5.4f, 7.13f };
        const float rIndex = p.harmonics * 9.0f;
        const int r0 = std::min ((int) rIndex, 8);
        const float ratio = ratios[r0] + (ratios[r0 + 1] - ratios[r0]) * (rIndex - (float) r0);

        if (p.trigger & plaits::TRIGGER_RISING_EDGE)
        {
            amp = 1.0f;
            modEnv = 1.0f;
            pitchEnv = 1.0f;
            carrier = modulator = 0.0f;
            accent = p.accent;
        }

        const float decay = 0.03f * std::pow (60.0f, p.morph);       // 30 ms .. 1.8 s
        const float ampCoef = decayCoef (decay);
        const float modCoef = decayCoef (decay * (0.15f + 0.5f * (1.0f - p.timbre)));
        const float pitchCoef = decayCoef (0.012f + 0.02f * p.morph);
        const float index = 7.0f * p.timbre * p.timbre * (0.5f + 0.5f * accent);
        const float f0 = plaits::NoteToFrequency (p.note);
        const float sweep = 1.5f + 2.5f * accent;                    // octaves at the start of the hit
        hp.set_f_q<stmlib::FREQUENCY_FAST> (0.08f, 0.7f);

        for (size_t i = 0; i < size; ++i)
        {
            const float f = std::min (f0 * std::exp2 (sweep * pitchEnv * pitchEnv), 0.25f);
            modulator = wrap01 (modulator + f * ratio);
            carrier = wrap01 (carrier + f);

            const float m = sinTwoPi (modulator + 0.35f * fb);
            fb = m;
            const float c = sinTwoPi (carrier + index * modEnv * m * 0.25f);
            const float noise = stmlib::Random::GetFloat() * 2.0f - 1.0f;

            out[i] = c * amp * (0.6f + 0.4f * accent);
            aux[i] = (0.6f * c + 0.4f * hp.Process<stmlib::FILTER_MODE_BAND_PASS> (noise * modEnv)) * amp;

            amp -= amp * ampCoef;
            modEnv -= modEnv * modCoef;
            pitchEnv -= pitchEnv * pitchCoef;
        }
    }

private:
    float carrier = 0, modulator = 0, amp = 0, modEnv = 0, pitchEnv = 0, fb = 0, accent = 0.8f;
    stmlib::Svf hp;
};

//==============================================================================
// TR-808 style cowbell: two detuned pulse waves through a band-pass, with the
// classic fast-then-slow decay. AUX is a two-sine "agogo" variant.
class CowbellEngine : public plaits::Engine
{
public:
    void Init (stmlib::BufferAllocator*) override { Reset(); }
    void Reset() override
    {
        a.reset(); b.reset();
        bp.Init(); bp2.Init();
        envFast = envSlow = 0.0f;
        sineA = sineB = 0.0f;
    }
    void LoadUserData (const uint8_t*) override {}

    void Render (const EngineParameters& p, float* out, float* aux, size_t size, bool*) override
    {
        if (p.trigger & plaits::TRIGGER_RISING_EDGE)
        {
            envFast = envSlow = 1.0f;
            accent = p.accent;
        }

        // C4 -> ~540 Hz, the 808 lower oscillator
        const float f1 = std::min (plaits::NoteToFrequency (p.note) * 2.07f, 0.2f);
        const float detune = 1.48f + p.harmonics * p.harmonics * 0.9f;
        const float f2 = std::min (f1 * detune, 0.4f);
        const float centre = std::min (f1 * (2.2f + 6.0f * p.timbre), 0.45f);
        bp.set_f_q<stmlib::FREQUENCY_FAST> (centre, 1.5f + 2.0f * p.timbre);
        bp2.set_f_q<stmlib::FREQUENCY_FAST> (std::min (f1 * 1.2f, 0.45f), 2.0f);

        const float decay = 0.08f * std::pow (25.0f, p.morph);       // 80 ms .. 2 s
        const float slowCoef = decayCoef (decay);
        const float fastCoef = decayCoef (0.012f);

        for (size_t i = 0; i < size; ++i)
        {
            float sawA, sqA, sawB, sqB;
            a.next (f1, 0.5f, sawA, sqA);
            b.next (f2, 0.5f, sawB, sqB);
            const float metal = 0.5f * (sqA + sqB);
            const float env = 0.55f * envFast + 0.45f * envSlow;
            const float body = bp.Process<stmlib::FILTER_MODE_BAND_PASS> (metal) * 1.8f;
            out[i] = std::tanh (body * env * (0.7f + 0.6f * accent));

            sineA = wrap01 (sineA + f1 * 1.5f);
            sineB = wrap01 (sineB + f2 * 1.5f);
            const float agogo = 0.5f * (sinTwoPi (sineA) + 0.7f * sinTwoPi (sineB));
            aux[i] = (agogo + 0.3f * bp2.Process<stmlib::FILTER_MODE_BAND_PASS> (metal)) * env;

            envFast -= envFast * fastCoef;
            envSlow -= envSlow * slowCoef;
        }
    }

private:
    BlepOscillator a, b;
    stmlib::Svf bp, bp2;
    float envFast = 0, envSlow = 0, sineA = 0, sineB = 0, accent = 0.8f;
};

//==============================================================================
// Analog tom: the Plaits bridged-T resonator tuned up, with a pitch drop and a
// stick transient. AUX is a Simmons-style synthetic tom.
class TomEngine : public plaits::Engine
{
public:
    void Init (stmlib::BufferAllocator*) override { analog.Init(); synthetic.Init(); stick.Init(); Reset(); }
    void Reset() override { stickEnv = 0.0f; }
    void LoadUserData (const uint8_t*) override {}

    void Render (const EngineParameters& p, float* out, float* aux, size_t size, bool*) override
    {
        const bool trig = (p.trigger & plaits::TRIGGER_RISING_EDGE) != 0;
        if (trig)
            stickEnv = 1.0f;

        // One octave above the bass drum engine at the same note
        const float f0 = std::min (plaits::NoteToFrequency (p.note + 12.0f), 0.05f);
        analog.Render (false, trig, p.accent, f0, 0.3f + 0.5f * p.timbre, p.morph,
                       0.25f + 0.35f * p.harmonics, p.harmonics, out, size);
        synthetic.Render (false, trig, p.accent, f0, 0.3f + 0.6f * p.timbre, p.morph,
                          p.harmonics, 0.3f, 0.6f, aux, size);

        stick.set_f_q<stmlib::FREQUENCY_FAST> (0.08f + 0.1f * p.timbre, 1.0f);
        const float stickCoef = decayCoef (0.004f);
        for (size_t i = 0; i < size; ++i)
        {
            const float n = stmlib::Random::GetFloat() * 2.0f - 1.0f;
            const float s = stick.Process<stmlib::FILTER_MODE_BAND_PASS> (n) * stickEnv * p.timbre;
            out[i] += s;
            aux[i] += s;
            stickEnv -= stickEnv * stickCoef;
        }
    }

private:
    plaits::AnalogBassDrum analog;
    plaits::SyntheticBassDrum synthetic;
    stmlib::Svf stick;
    float stickEnv = 0;
};

//==============================================================================
// TB-303 / TD-3 style bassline voice: saw (OUT) and square (AUX) through a
// resonant 4-pole diode-ish filter with its own decaying envelope and accent.
//   HARMONICS = resonance, TIMBRE = cutoff, MORPH = envelope mod + decay.
class TD3Engine : public plaits::Engine
{
public:
    void Init (stmlib::BufferAllocator*) override { Reset(); }
    void Reset() override
    {
        osc.reset();
        sawFilter = {};
        squareFilter = {};
        env = 0.0f;
        accentEnv = 0.0f;
    }
    void LoadUserData (const uint8_t*) override {}

    void Render (const EngineParameters& p, float* out, float* aux, size_t size, bool*) override
    {
        if (p.trigger & plaits::TRIGGER_RISING_EDGE)
        {
            env = 1.0f;
            accented = p.accent > 0.9f;
            if (accented)
                accentEnv = std::min (1.0f, accentEnv + 0.6f);
        }

        const float f = plaits::NoteToFrequency (p.note);
        const float decay = accented ? 0.2f : 0.2f + 1.8f * p.morph * p.morph;
        const float envCoef = decayCoef (decay);
        const float accentCoef = decayCoef (0.18f);
        const float envMod = 0.5f + 4.0f * p.morph;                  // octaves
        const float baseHz = 60.0f * std::pow (80.0f, p.timbre);     // 60 Hz .. 4.8 kHz
        const float reso = p.harmonics;
        const float k = 3.95f * reso * (accented ? 0.95f : 1.0f);

        for (size_t i = 0; i < size; ++i)
        {
            float saw, square;
            osc.next (f, 0.5f, saw, square);

            const float sweep = env * envMod + accentEnv * 2.0f;
            const float hz = std::min (baseHz * std::exp2 (sweep), 18000.0f);
            const float g = std::tan (3.14159265f * hz / plaits::kSampleRate);

            const float level = 0.7f + 0.5f * accentEnv;
            out[i] = sawFilter.process (saw * 0.6f, g, k) * level * 2.2f;
            aux[i] = squareFilter.process (square * 0.5f, g, k) * level * 2.0f;

            env -= env * envCoef;
            accentEnv -= accentEnv * accentCoef;
        }
    }

private:
    // Four cascaded TPT one-poles with tanh feedback and a feedback high-pass
    // (the 303's resonance keeps more bass than a Moog ladder).
    struct Filter
    {
        float s[4] {}, hpState = 0.0f;
        float process (float x, float g, float k) noexcept
        {
            const float G = g / (1.0f + g);
            const float hp = s[3] - hpState;
            hpState += 0.004f * hp;
            float u = std::tanh (x - k * std::tanh (hp));
            for (auto& st : s)
            {
                const float v = (u - st) * G;
                const float y = v + st;
                st = y + v;
                u = y;
            }
            return u;
        }
    };

    BlepOscillator osc;
    Filter sawFilter, squareFilter;
    float env = 0, accentEnv = 0;
    bool accented = false;
};

//==============================================================================
// Classic function generator: MORPH scans sine -> triangle -> saw -> square,
// TIMBRE sets symmetry / pulse width, HARMONICS adds a detuned or interval
// second oscillator. AUX is a sine plus sub-octave square.
class WaveGeneratorEngine : public plaits::Engine
{
public:
    void Init (stmlib::BufferAllocator*) override { Reset(); }
    void Reset() override { a.reset(); b.reset(); sub.reset(); phaseA = phaseB = 0.0f; }
    void LoadUserData (const uint8_t*) override {}

    void Render (const EngineParameters& p, float* out, float* aux, size_t size, bool*) override
    {
        const float f = plaits::NoteToFrequency (p.note);

        // Second oscillator: unison detune below the middle, musical intervals above
        float ratio, mix2;
        if (p.harmonics < 0.5f)
        {
            const float d = p.harmonics * 2.0f;
            ratio = std::exp2 (0.35f * d * d / 12.0f);
            mix2 = std::min (1.0f, d * 4.0f) * 0.8f;
        }
        else
        {
            static constexpr float intervals[] = { 7.0f, 12.0f, 19.0f, 24.0f, 31.0f };
            const int idx = std::min (4, (int) ((p.harmonics - 0.5f) * 10.0f));
            ratio = std::exp2 (intervals[idx] / 12.0f);
            mix2 = 0.6f;
        }

        const float shape = p.morph * 3.0f;
        const float pw = 0.5f - 0.45f * p.timbre;
        const float skew = 0.5f + 0.49f * p.timbre;

        for (size_t i = 0; i < size; ++i)
        {
            const float w1 = wave (a, phaseA, f, shape, pw, skew);
            const float w2 = mix2 > 0.0f ? wave (b, phaseB, std::min (f * ratio, 0.45f), shape, pw, skew) : 0.0f;
            out[i] = (w1 + mix2 * w2) / (1.0f + 0.5f * mix2);

            float s, q;
            sub.next (f * 0.5f, 0.5f, s, q);
            aux[i] = 0.6f * sinTwoPi (phaseA) + 0.5f * q;
        }
    }

private:
    static float wave (BlepOscillator& o, float& phase, float f, float shape, float pw, float skew) noexcept
    {
        float saw, square;
        o.next (f, pw, saw, square);
        phase = o.phase;
        const float sine = sinTwoPi (phase);
        const float tri = phase < skew ? (2.0f * phase / skew - 1.0f)
                                       : (1.0f - 2.0f * (phase - skew) / (1.0f - skew));
        if (shape < 1.0f) return sine + (tri - sine) * shape;
        if (shape < 2.0f) return tri + (saw - tri) * (shape - 1.0f);
        return saw + (square - saw) * std::min (1.0f, shape - 2.0f);
    }

    BlepOscillator a, b, sub;
    float phaseA = 0, phaseB = 0;
};

//==============================================================================
// Formant voice / choir. A glottal pulse (and a breath noise) through four
// parallel formant band-passes.
//   HARMONICS = voice type (bass .. soprano), TIMBRE = breath + choir spread,
//   MORPH = vowel (A E I O U). AUX = whispered version.
class VoxEngine : public plaits::Engine
{
public:
    void Init (stmlib::BufferAllocator*) override
    {
        for (auto& f : formants) f.Init();
        for (auto& f : whisper) f.Init();
        Reset();
    }
    void Reset() override
    {
        for (auto& p : phases) p = 0.0f;
        vibratoPhase = 0.0f;
        glottalLp = 0.0f;
    }
    void LoadUserData (const uint8_t*) override {}

    void Render (const EngineParameters& p, float* out, float* aux, size_t size, bool*) override
    {
        // Formant frequencies / gains (dB) for bass, tenor, alto, soprano x A E I O U
        static constexpr float freq[4][5][4] = {
            { { 600, 1040, 2250, 2450 }, { 400, 1620, 2400, 2800 }, { 250, 1750, 2600, 3050 }, { 400, 750, 2400, 2600 }, { 350, 600, 2400, 2675 } },
            { { 650, 1080, 2650, 2900 }, { 400, 1700, 2600, 3200 }, { 290, 1870, 2800, 3250 }, { 400, 800, 2600, 2800 }, { 350, 600, 2700, 2900 } },
            { { 800, 1150, 2800, 3500 }, { 400, 1600, 2700, 3300 }, { 350, 1700, 2700, 3700 }, { 450, 800, 2830, 3500 }, { 325, 700, 2530, 3500 } },
            { { 800, 1150, 2900, 3900 }, { 350, 2000, 2800, 3600 }, { 270, 2140, 2950, 3900 }, { 450, 800, 2830, 3800 }, { 325, 700, 2700, 3800 } },
        };
        static constexpr float gainDb[4][5][4] = {
            { { 0, -7, -9, -9 }, { 0, -12, -9, -12 }, { 0, -30, -16, -22 }, { 0, -11, -21, -20 }, { 0, -20, -32, -28 } },
            { { 0, -6, -7, -8 }, { 0, -14, -12, -14 }, { 0, -15, -18, -20 }, { 0, -10, -12, -12 }, { 0, -20, -17, -14 } },
            { { 0, -4, -20, -36 }, { 0, -24, -30, -35 }, { 0, -20, -30, -36 }, { 0, -9, -16, -28 }, { 0, -12, -30, -40 } },
            { { 0, -6, -32, -20 }, { 0, -20, -15, -40 }, { 0, -12, -26, -26 }, { 0, -11, -22, -22 }, { 0, -16, -35, -40 } },
        };

        const float typeIndex = p.harmonics * 2.999f;
        const int t0 = (int) typeIndex;
        const float tf = typeIndex - (float) t0;
        const float vowelIndex = p.morph * 3.999f;
        const int v0 = (int) vowelIndex;
        const float vf = vowelIndex - (float) v0;

        float fHz[4], gain[4];
        for (int k = 0; k < 4; ++k)
        {
            auto lerp2 = [&] (const float (&tab)[4][5][4])
            {
                const float a = tab[t0][v0][k] + (tab[t0][v0 + 1][k] - tab[t0][v0][k]) * vf;
                const float b = tab[t0 + 1][v0][k] + (tab[t0 + 1][v0 + 1][k] - tab[t0 + 1][v0][k]) * vf;
                return a + (b - a) * tf;
            };
            fHz[k] = lerp2 (freq);
            gain[k] = std::pow (10.0f, lerp2 (gainDb) / 20.0f);
            const float fn = std::min (fHz[k] / plaits::kSampleRate, 0.45f);
            const float q = 6.0f + 4.0f * (float) k;
            formants[k].set_f_q<stmlib::FREQUENCY_FAST> (fn, q);
            whisper[k].set_f_q<stmlib::FREQUENCY_FAST> (fn, q * 0.6f);
        }

        const float f0 = plaits::NoteToFrequency (p.note);
        const float spread = p.timbre * p.timbre * 0.012f;           // choir detune
        const float breath = 0.05f + 0.6f * p.timbre;
        const float vibFreq = 5.3f / plaits::kSampleRate;

        for (size_t i = 0; i < size; ++i)
        {
            vibratoPhase = wrap01 (vibratoPhase + vibFreq);
            const float vib = 1.0f + 0.004f * sinTwoPi (vibratoPhase);

            // Three slightly detuned glottal pulses form the choir
            float source = 0.0f;
            for (int v = 0; v < 3; ++v)
            {
                const float detune = 1.0f + spread * (float) (v - 1);
                phases[v] = wrap01 (phases[v] + std::min (f0 * vib * detune, 0.45f));
                const float ph = phases[v];
                // Rosenberg-like pulse: smooth opening, sharp closure
                const float g = ph < 0.6f ? 0.5f * (1.0f - std::cos (3.14159265f * ph / 0.6f))
                                          : std::cos (3.14159265f * (ph - 0.6f) / 0.8f);
                source += std::max (g, 0.0f) * (v == 1 ? 1.0f : 0.6f * std::min (1.0f, p.timbre * 3.0f));
            }
            // Differentiate (lip radiation) and add aspiration
            const float diff = source - glottalLp;
            glottalLp = source;
            const float noise = stmlib::Random::GetFloat() * 2.0f - 1.0f;
            const float excitation = diff * 4.0f + noise * breath * 0.3f;

            float voiced = 0.0f, whispered = 0.0f;
            for (int k = 0; k < 4; ++k)
            {
                voiced += formants[k].Process<stmlib::FILTER_MODE_BAND_PASS> (excitation) * gain[k];
                whispered += whisper[k].Process<stmlib::FILTER_MODE_BAND_PASS> (noise) * gain[k];
            }
            out[i] = std::tanh (voiced * 5.0f);
            aux[i] = whispered * 0.8f;
        }
    }

private:
    stmlib::Svf formants[4], whisper[4];
    float phases[3] {}, vibratoPhase = 0, glottalLp = 0;
};

} // namespace grind::dsp
