// The Grind's digital oscillator: the Mutable Instruments Plaits engines plus
// the Grind's extra engines, driven the way the Plaits voice drives them
// (trigger, internal decay envelope, low-pass gate, output gain staging),
// but rendering floats instead of 16-bit codec frames.
//
// Runs at plaits::kSampleRate (48 kHz) in blocks of plaits::kBlockSize.

#pragma once

#include <memory>

#include "../EngineTable.h"
#include "ExtraEngines.h"

#include "plaits/dsp/engine/additive_engine.h"
#include "plaits/dsp/engine/bass_drum_engine.h"
#include "plaits/dsp/engine/chord_engine.h"
#include "plaits/dsp/engine/fm_engine.h"
#include "plaits/dsp/engine/grain_engine.h"
#include "plaits/dsp/engine/hi_hat_engine.h"
#include "plaits/dsp/engine/modal_engine.h"
#include "plaits/dsp/engine/noise_engine.h"
#include "plaits/dsp/engine/particle_engine.h"
#include "plaits/dsp/engine/snare_drum_engine.h"
#include "plaits/dsp/engine/speech_engine.h"
#include "plaits/dsp/engine/string_engine.h"
#include "plaits/dsp/engine/swarm_engine.h"
#include "plaits/dsp/engine/virtual_analog_engine.h"
#include "plaits/dsp/engine/waveshaping_engine.h"
#include "plaits/dsp/engine/wavetable_engine.h"
#include "plaits/dsp/engine2/six_op_engine.h"
#include "plaits/dsp/envelope.h"
#include "plaits/dsp/fx/low_pass_gate.h"
#include "stmlib/dsp/limiter.h"

namespace grind::dsp
{
class MacroOscillator
{
public:
    struct Params
    {
        int engine = 0;
        int dx7Bank = 0;
        float note = 60.0f;          // MIDI note, fractional
        float harmonics = 0.5f, timbre = 0.5f, morph = 0.5f;
        float fmAmount = 0.0f, timbreAmount = 0.0f, morphAmount = 0.0f; // attenuverters
        float fmCv = 0.0f, timbreCv = 0.0f, morphCv = 0.0f;               // patched CV (semitones / unit)
        bool fmPatched = false, timbrePatched = false, morphPatched = false;
        float decay = 0.5f, lpgColour = 0.5f;
        bool lpgOn = false;
        float accent = 0.8f;
    };

    MacroOscillator();
    ~MacroOscillator();

    void init();
    void reset();

    // gate: held trigger level; retrigger: force a rising edge this block
    void render (const Params&, bool gate, bool retrigger, float* out, float* aux, size_t size);

    float decayEnvelope() const noexcept { return decayEnv.value(); }
    int activeEngine() const noexcept { return previousEngine; }

private:
    struct Slot
    {
        plaits::Engine* engine = nullptr;
        bool alreadyEnveloped = false;
        float outGain = 1.0f, auxGain = 1.0f;
    };

    struct PostProcessor
    {
        stmlib::Limiter limiter;
        plaits::LowPassGate lpg;
        void init() { limiter.Init(); lpg.Init(); }
        void process (float gain, bool bypassLpg, float lpgGain, float lpgFreq, float lpgBleed, float* buf, size_t size);
    };

    // Plaits engines
    plaits::VirtualAnalogEngine virtualAnalog;
    plaits::WaveshapingEngine waveshaping;
    plaits::FMEngine fm;
    plaits::GrainEngine grain;
    plaits::AdditiveEngine additive;
    plaits::WavetableEngine wavetable;
    plaits::ChordEngine chords;
    plaits::SpeechEngine speech;
    plaits::SwarmEngine swarm;
    plaits::NoiseEngine noise;
    plaits::ParticleEngine particle;
    plaits::StringEngine string;
    plaits::ModalEngine modal;
    plaits::BassDrumEngine bassDrum;
    plaits::SnareDrumEngine snareDrum;
    plaits::HiHatEngine hiHat;
    plaits::SixOpEngine sixOp;

    // Grind engines
    DustEngine dust { particle };
    FMDrumEngine fmDrum;
    CowbellEngine cowbell;
    TomEngine tom;
    TD3Engine td3;
    WaveGeneratorEngine waveGen;
    VoxEngine vox;

    std::array<Slot, numEngines> slots;
    std::unique_ptr<char[]> ram;   // shared engine RAM, like the 16 kB on the Plaits MCU
    stmlib::BufferAllocator allocator;

    plaits::DecayEnvelope decayEnv;
    plaits::LPGEnvelope lpgEnv;
    PostProcessor outPost, auxPost;

    int previousEngine = -1, loadedBank = -1;
    bool triggerState = false;
    float previousNote = 60.0f;
    float outBuffer[plaits::kMaxBlockSize] {}, auxBuffer[plaits::kMaxBlockSize] {};
};

} // namespace grind::dsp
