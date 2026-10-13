#include "MacroOscillator.h"

#include "plaits/resources.h"

namespace grind::dsp
{
namespace
{
constexpr size_t ramSize = 32768;
constexpr int speechEngine = 6;
} // namespace

MacroOscillator::MacroOscillator() : ram (new char[ramSize]) {}
MacroOscillator::~MacroOscillator() = default;

void MacroOscillator::init()
{
    // Same per-engine gain staging as plaits::Voice::Init (a negative gain
    // means "run through the limiter").
    const Slot table[numEngines] = {
        { &virtualAnalog, false, 0.8f, 0.8f },
        { &waveshaping,   false, 0.7f, 0.6f },
        { &fm,            false, 0.6f, 0.6f },
        { &grain,         false, 0.7f, 0.6f },
        { &additive,      false, 0.8f, 0.8f },
        { &chords,        false, 0.8f, 0.8f },
        { &speech,        false, -0.7f, 0.8f },
        { &string,        true,  -1.0f, 0.8f },
        { &swarm,         false, -3.0f, 1.0f },
        { &wavetable,     false, 0.6f, 0.6f },

        { &particle,      false, -2.0f, 1.0f },
        { &noise,         false, -1.0f, -1.0f },
        { &dust,          false, 1.0f, -2.0f },
        { &modal,         true,  -1.0f, 0.8f },
        { &fmDrum,        true,  0.8f, 0.8f },
        { &bassDrum,      true,  0.8f, 0.8f },
        { &snareDrum,     true,  0.8f, 0.8f },
        { &hiHat,         true,  0.8f, 0.8f },
        { &cowbell,       true,  0.8f, 0.8f },
        { &tom,           true,  0.8f, 0.8f },

        { &sixOp,         true,  1.0f, 1.0f },
        { &td3,           false, 0.8f, 0.8f },
        { &waveGen,       false, 0.6f, 0.7f },
        { &vox,           false, -0.8f, 0.8f },
    };

    allocator.Init (ram.get(), ramSize);
    for (int i = 0; i < numEngines; ++i)
    {
        slots[(size_t) i] = table[i];
        auto& pp = slots[(size_t) i].engine->post_processing_settings;
        pp.already_enveloped = table[i].alreadyEnveloped;
        pp.out_gain = table[i].outGain;
        pp.aux_gain = table[i].auxGain;

        // All engines share the same RAM, exactly like on the module
        allocator.Free();
        slots[(size_t) i].engine->Init (&allocator);
    }

    decayEnv.Init();
    lpgEnv.Init();
    outPost.init();
    auxPost.init();
    previousEngine = -1;
    loadedBank = -1;
    triggerState = false;
}

void MacroOscillator::reset()
{
    // Force the active engine to reload and clear its state on the next block
    previousEngine = -1;
    triggerState = false;
    decayEnv.Init();
    lpgEnv.Init();
}

void MacroOscillator::PostProcessor::process (float gain, bool bypassLpg, float lpgGain, float lpgFreq,
                                              float lpgBleed, float* buf, size_t size)
{
    if (gain < 0.0f)
        limiter.Process (-gain, buf, size);
    const float post = gain < 0.0f ? 1.0f : gain;

    if (! bypassLpg)
    {
        lpg.Process (post * lpgGain, lpgFreq, lpgBleed, buf, size);
    }
    else
    {
        for (size_t i = 0; i < size; ++i)
            buf[i] *= post;
    }

    // The module's codec path saturates at 16-bit full scale
    for (size_t i = 0; i < size; ++i)
        buf[i] = std::clamp (buf[i], -1.0f, 1.0f);
}

void MacroOscillator::render (const Params& patch, bool gate, bool retrigger, float* out, float* aux, size_t size)
{
    using namespace stmlib;

    // Trigger edge detection (the MIDI gate is always "patched" into TRIG)
    const bool previousTrigger = triggerState;
    bool risingEdge = false;
    if (retrigger || (gate && ! previousTrigger))
    {
        risingEdge = true;
        lpgEnv.Trigger();
        decayEnv.Trigger();
    }
    triggerState = gate;

    const int engineIndex = std::clamp (patch.engine, 0, numEngines - 1);
    auto& slot = slots[(size_t) engineIndex];
    plaits::Engine* e = slot.engine;

    if (engineIndex != previousEngine || (engineIndex == 20 && patch.dx7Bank != loadedBank))
    {
        const uint8_t* data = nullptr;
        if (engineIndex == 20)
        {
            loadedBank = std::clamp (patch.dx7Bank, 0, 2);
            data = plaits::fm_patches_table[loadedBank];
        }
        e->LoadUserData (data);
        e->Reset();
        outPost.limiter.Init();
        previousEngine = engineIndex;
    }

    plaits::EngineParameters p;
    p.trigger = (risingEdge ? plaits::TRIGGER_RISING_EDGE : plaits::TRIGGER_LOW)
              | (triggerState || risingEdge ? plaits::TRIGGER_HIGH : plaits::TRIGGER_LOW);
    p.accent = patch.accent;

    const float note = (patch.note + previousNote) * 0.5f;
    previousNote = patch.note;

    const float shortDecay = (200.0f * (float) plaits::kBlockSize) / plaits::kSampleRate
                           * SemitonesToRatio (-96.0f * patch.decay);
    decayEnv.Process (shortDecay * 2.0f);

    p.harmonics = std::clamp (patch.harmonics, 0.0f, 1.0f);

    float envAmplitude = 1.0f;
    if (engineIndex == speechEngine)
    {
        envAmplitude = std::clamp (2.0f - p.harmonics * 6.0f, 0.0f, 1.0f);
        speech.set_prosody_amount (patch.fmPatched ? 0.0f : patch.fmAmount);
        speech.set_speed (patch.morphPatched ? 0.0f : patch.morphAmount);
    }

    const float env = decayEnv.value();
    auto modulate = [] (float base, float amount, bool patched, float cv, float internal, float lo, float hi)
    {
        amount *= std::max (std::abs (amount) - 0.05f, 0.05f);
        amount *= 1.05f;
        return std::clamp (base + amount * (patched ? cv : internal), lo, hi);
    };

    p.note = modulate (note, patch.fmAmount, patch.fmPatched, patch.fmCv, envAmplitude * env * env * 48.0f, -119.0f, 120.0f);
    p.timbre = modulate (patch.timbre, patch.timbreAmount, patch.timbrePatched, patch.timbreCv, env, 0.0f, 1.0f);
    p.morph = modulate (patch.morph, patch.morphAmount, patch.morphPatched, patch.morphCv, envAmplitude * env, 0.0f, 1.0f);

    bool alreadyEnveloped = e->post_processing_settings.already_enveloped;
    e->Render (p, outBuffer, auxBuffer, size, &alreadyEnveloped);

    const bool lpgBypass = alreadyEnveloped || ! patch.lpgOn;
    if (! lpgBypass)
    {
        const float hf = patch.lpgColour;
        const float decayTail = (20.0f * (float) plaits::kBlockSize) / plaits::kSampleRate
                              * SemitonesToRatio (-72.0f * patch.decay + 12.0f * hf) - shortDecay;
        const float attack = plaits::NoteToFrequency (p.note) * (float) plaits::kBlockSize * 2.0f;
        lpgEnv.ProcessPing (attack, shortDecay, decayTail, hf);
    }
    else
    {
        lpgEnv.Init();
    }

    const auto& pp = e->post_processing_settings;
    outPost.process (pp.out_gain, lpgBypass, lpgEnv.gain(), lpgEnv.frequency(), lpgEnv.hf_bleed(), outBuffer, size);
    auxPost.process (pp.aux_gain, lpgBypass, lpgEnv.gain(), lpgEnv.frequency(), lpgEnv.hf_bleed(), auxBuffer, size);

    std::copy (outBuffer, outBuffer + size, out);
    std::copy (auxBuffer, auxBuffer + size, aux);
}

} // namespace grind::dsp
