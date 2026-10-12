#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr float gateOffMs = 1000.0f;   // the gate knob's top position means "no gate"
constexpr float voiceHeadroom = 0.5f;  // -6 dB per voice so a full kit sits below 0 dBFS
const juce::StringArray rateNames { "1/8", "1/16", "1/16T", "1/32" };
constexpr double rateLengths[] { 0.5, 0.25, 1.0 / 6.0, 0.125 }; // in quarter notes

inline float driveShape (float x, float amount) noexcept
{
    if (amount <= 0.0f)
        return x;
    const float g = 1.0f + 14.0f * amount * amount;
    const float shaped = std::tanh (g * x) / std::tanh (g);
    return x + amount * (shaped - x);
}

// Transparent below -0.9 dBFS, then rounds off smoothly into a 0 dBFS ceiling
inline float softClip (float x) noexcept
{
    constexpr float knee = 0.9f;
    const float ax = std::abs (x);
    if (ax <= knee)
        return x;
    return std::copysign (knee + (1.0f - knee) * std::tanh ((ax - knee) / (1.0f - knee)), x);
}

// 8-bit mu-law round trip, the companding the LinnDrum's sample ROMs used
inline float muLaw8 (float x) noexcept
{
    x = juce::jlimit (-1.0f, 1.0f, x);
    const float c = std::copysign (std::log1p (255.0f * std::abs (x)) / std::log (256.0f), x);
    const float q = std::round (c * 127.0f) / 127.0f;
    return std::copysign ((std::pow (256.0f, std::abs (q)) - 1.0f) / 255.0f, q);
}
} // namespace

//==============================================================================
juce::String MidnightEdgeProcessor::voiceParamId (int voice, const char* suffix)
{
    return juce::String (me::voiceInfo[(size_t) voice].id) + "_" + suffix;
}

juce::AudioProcessorValueTreeState::ParameterLayout MidnightEdgeProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    const auto& kit = me::factoryKits()[0];

    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; });

    for (int v = 0; v < me::numVoices; ++v)
    {
        const auto& info = me::voiceInfo[(size_t) v];
        const auto& d = kit.voices[(size_t) v];
        auto group = std::make_unique<AudioProcessorParameterGroup> (info.id, info.name, " | ");
        const String n (info.name);

        auto addFloat = [&] (const char* suffix, const String& label, NormalisableRange<float> range,
                             float def, AudioParameterFloatAttributes attr)
        {
            group->addChild (std::make_unique<AudioParameterFloat> (
                ParameterID { voiceParamId (v, suffix), 1 }, n + " " + label, range, def, attr));
        };

        addFloat ("tune", "Tune", { -24.0f, 24.0f, 0.01f }, d.tune,
                  AudioParameterFloatAttributes().withLabel ("st").withStringFromValueFunction (
                      [] (float x, int) { return String (x, 1) + " st"; }));
        addFloat ("decay", "Decay", { 0.0f, 1.0f }, d.decay, pct);
        addFloat ("tone", info.knobNames[2], { 0.0f, 1.0f }, d.tone, pct);
        addFloat ("snap", info.knobNames[3], { 0.0f, 1.0f }, d.snap, pct);
        addFloat ("drive", "Drive", { 0.0f, 1.0f }, d.drive, pct);
        addFloat ("verb", "Gated Verb", { 0.0f, 1.0f }, d.verb, pct);
        addFloat ("blend", "Sample Blend", { 0.0f, 1.0f }, d.blend,
                  AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int)
                  {
                      if (x < 0.005f) return String ("Synth");
                      if (x > 0.995f) return String ("Sample");
                      return String (roundToInt (x * 100.0f)) + "% smp";
                  }));
        addFloat ("pan", "Pan", { -1.0f, 1.0f }, d.pan,
                  AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int)
                  {
                      if (std::abs (x) < 0.01f) return String ("C");
                      return String (roundToInt (std::abs (x) * 100.0f)) + (x < 0 ? "L" : "R");
                  }));
        addFloat ("level", "Level", NormalisableRange<float> (-48.0f, 6.0f, 0.1f, 2.0f), d.level,
                  AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (
                      [] (float x, int) { return x <= -47.9f ? String ("-inf") : String (x, 1) + " dB"; }));

        layout.add (std::move (group));
    }

    auto addMaster = [&] (const char* id, const char* name, NormalisableRange<float> range, float def,
                          AudioParameterFloatAttributes attr)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, def, attr));
    };

    addMaster ("vintage", "Vintage", { 0.0f, 1.0f }, kit.vintage, pct);
    addMaster ("heat", "Heat", { 0.0f, 1.0f }, kit.heat, pct);
    addMaster ("punch", "Punch", { 0.0f, 1.0f }, kit.punch, pct);
    addMaster ("verbSize", "Verb Size", { 0.0f, 1.0f }, kit.verbSize, pct);
    addMaster ("verbGate", "Verb Gate", NormalisableRange<float> (40.0f, gateOffMs, 1.0f, 0.6f), kit.verbGate,
               AudioParameterFloatAttributes().withLabel ("ms").withStringFromValueFunction (
                   [] (float x, int) { return x >= gateOffMs - 0.5f ? String ("OFF") : String (roundToInt (x)) + " ms"; }));
    addMaster ("verbTone", "Verb Tone", { 0.0f, 1.0f }, kit.verbTone, pct);
    addMaster ("verbMix", "Verb Level", { 0.0f, 1.0f }, kit.verbMix, pct);
    addMaster ("swing", "Swing", { 0.0f, 1.0f }, 0.0f,
               AudioParameterFloatAttributes().withStringFromValueFunction (
                   [] (float x, int) { return String (roundToInt (50.0f + 25.0f * x)) + "%"; }));
    addMaster ("accent", "Accent", { 0.0f, 1.0f }, 0.5f, pct);
    addMaster ("output", "Output", NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f,
               AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (
                   [] (float x, int) { return String (x, 1) + " dB"; }));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "seqOn", 1 }, "Sequencer On", true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "pattern", 1 }, "Pattern",
                                                        StringArray { "A", "B", "C", "D", "E", "F", "G", "H" }, 0));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "length", 1 }, "Pattern Length", 1, me::numSteps, 16));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "rate", 1 }, "Step Rate", rateNames, 1));

    return layout;
}

//==============================================================================
MidnightEdgeProcessor::MidnightEdgeProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MidnightEdge", createLayout())
{
    voices[me::kick]      = std::make_unique<me::KickVoice>();
    voices[me::snare]     = std::make_unique<me::SnareVoice>();
    voices[me::clap]      = std::make_unique<me::ClapVoice>();
    voices[me::closedHat] = std::make_unique<me::HatVoice> (false);
    voices[me::openHat]   = std::make_unique<me::HatVoice> (true);
    voices[me::lowTom]    = std::make_unique<me::TomVoice> (98.0f);
    voices[me::highTom]   = std::make_unique<me::TomVoice> (147.0f);
    voices[me::edgePerc]  = std::make_unique<me::EdgePercVoice>();

    for (int v = 0; v < me::numVoices; ++v)
        for (size_t i = 0; i < me::voiceParamSuffixes.size(); ++i)
            voiceParams[(size_t) v].p[i] = apvts.getRawParameterValue (voiceParamId (v, me::voiceParamSuffixes[i]));

    pVintage  = apvts.getRawParameterValue ("vintage");
    pHeat     = apvts.getRawParameterValue ("heat");
    pPunch    = apvts.getRawParameterValue ("punch");
    pVerbSize = apvts.getRawParameterValue ("verbSize");
    pVerbGate = apvts.getRawParameterValue ("verbGate");
    pVerbTone = apvts.getRawParameterValue ("verbTone");
    pVerbMix  = apvts.getRawParameterValue ("verbMix");
    pSwing    = apvts.getRawParameterValue ("swing");
    pAccent   = apvts.getRawParameterValue ("accent");
    pOutput   = apvts.getRawParameterValue ("output");
    pSeqOn    = apvts.getRawParameterValue ("seqOn");
    pPattern  = apvts.getRawParameterValue ("pattern");
    pLength   = apvts.getRawParameterValue ("length");
    pRate     = apvts.getRawParameterValue ("rate");

    for (auto& s : steps)
        s.store (stepOff);
    loadDefaultPatterns();

    formatManager.registerBasicFormats();
    triggers.reserve (1024);
}

bool MidnightEdgeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MidnightEdgeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    for (auto& v : voices)
        v->prepare ((float) sampleRate);
    for (auto& sp : samplePlayers)
        sp = {};

    sendBuffer.setSize (2, samplesPerBlock);
    gateBuffer.setSize (1, samplesPerBlock);
    reverb.setSampleRate (sampleRate);
    reverb.reset();

    sendHpL.setCutoff (180.0f, (float) sr);
    sendHpR.setCutoff (180.0f, (float) sr);
    sendHpL.reset();
    sendHpR.reset();
    vintageLpL.setCutoff (11000.0f, (float) sr);
    vintageLpR.setCutoff (11000.0f, (float) sr);
    vintageLpL.reset();
    vintageLpR.reset();

    gateEnv = 0.0f;
    gateCounter = 0;
    compEnvDb = 0.0f;
    vintagePhase = 0.0f;
    vintageHeldL = vintageHeldR = 0.0f;
}

//==============================================================================
uint8_t MidnightEdgeProcessor::getStep (int pattern, int voice, int step) const noexcept
{
    return steps[(size_t) ((pattern * me::numVoices + voice) * me::numSteps + step)].load (std::memory_order_relaxed);
}

void MidnightEdgeProcessor::setStep (int pattern, int voice, int step, uint8_t value) noexcept
{
    steps[(size_t) ((pattern * me::numVoices + voice) * me::numSteps + step)].store (value, std::memory_order_relaxed);
}

void MidnightEdgeProcessor::clearPattern (int pattern) noexcept
{
    for (int v = 0; v < me::numVoices; ++v)
        for (int s = 0; s < me::numSteps; ++s)
            setStep (pattern, v, s, stepOff);
}

int MidnightEdgeProcessor::getCurrentPattern() const noexcept
{
    return juce::jlimit (0, me::numPatterns - 1, (int) pPattern->load());
}

void MidnightEdgeProcessor::loadDefaultPatterns()
{
    const auto& defaults = me::defaultPatterns();
    for (size_t p = 0; p < defaults.size(); ++p)
        for (int v = 0; v < me::numVoices; ++v)
            for (int s = 0; s < me::numSteps; ++s)
            {
                const char c = defaults[p].rows[(size_t) v][s];
                setStep ((int) p, v, s, c == 'X' ? stepAccent : (c == 'x' ? stepOn : stepOff));
            }
}

//==============================================================================
void MidnightEdgeProcessor::addTrigger (int offset, int voice, float velocity) noexcept
{
    if (triggers.size() < triggers.capacity())
        triggers.push_back ({ offset, voice, velocity });
}

void MidnightEdgeProcessor::runSequencer (int numSamples)
{
    auto* hostPlayHead = getPlayHead();
    if (hostPlayHead == nullptr || pSeqOn->load() < 0.5f)
    {
        currentStep.store (-1);
        return;
    }

    const auto pos = hostPlayHead->getPosition();
    if (! pos.hasValue() || ! pos->getIsPlaying() || ! pos->getPpqPosition().hasValue())
    {
        currentStep.store (-1);
        return;
    }

    const double bpm = pos->getBpm().orFallback (120.0);
    const double ppq0 = *pos->getPpqPosition();
    const double ppqPerSample = bpm / (60.0 * sr);
    const double ppq1 = ppq0 + numSamples * ppqPerSample;

    const int rateIndex = juce::jlimit (0, 3, (int) pRate->load());
    const double stepLen = rateLengths[rateIndex];
    const double swingOffset = pSwing->load() * 0.5 * stepLen;
    const int length = juce::jlimit (1, me::numSteps, (int) pLength->load());
    const int pattern = getCurrentPattern();
    const float accentAmt = pAccent->load();
    const float normalVel = 1.0f - 0.45f * accentAmt;

    // Start one step early so a swung (delayed) odd step from the previous block isn't missed
    for (auto k = (int64_t) std::floor (ppq0 / stepLen) - 1;; ++k)
    {
        const double straight = (double) k * stepLen;
        if (straight >= ppq1)
            break;

        const double t = straight + ((k & 1) != 0 ? swingOffset : 0.0);
        if (t < ppq0 || t >= ppq1)
            continue;

        const int offset = juce::jlimit (0, numSamples - 1, (int) ((t - ppq0) / ppqPerSample));
        const int stepIndex = (int) (((k % length) + length) % length);
        currentStep.store (stepIndex, std::memory_order_relaxed);

        for (int v = 0; v < me::numVoices; ++v)
        {
            const auto st = getStep (pattern, v, stepIndex);
            if (st != stepOff)
                addTrigger (offset, v, st == stepAccent ? 1.0f : normalVel);
        }
    }
}

void MidnightEdgeProcessor::triggerVoice (int v, float velocity)
{
    const auto& vp = voiceParams[(size_t) v];
    me::VoiceParams p { vp.get (0), vp.get (1), vp.get (2), vp.get (3) };
    voices[(size_t) v]->trigger (velocity, p);

    auto& player = samplePlayers[(size_t) v];
    auto* data = sampleLockHeld[(size_t) v] ? sampleSlots[(size_t) v].data.get() : nullptr;
    if (data != nullptr)
    {
        player.pos = 0.0;
        player.rate = data->sourceRate / sr * me::semitonesToRatio (p.tune);
        player.gain = velocity;
        player.chokeGain = 1.0f;
        player.chokeCoef = 1.0f;
        player.playing = true;
    }

    if (v == me::closedHat)
    {
        voices[me::openHat]->choke();
        samplePlayers[me::openHat].chokeCoef = me::tauCoef (0.006f, (float) sr);
    }

    // Open the reverb gate for any voice that feeds it
    if (vp.get (5) > 0.02f)
    {
        const float gateMs = pVerbGate->load();
        gateCounter = std::max (gateCounter, (int) (gateMs * 0.001f * (float) sr));
    }

    triggeredMask.fetch_or (1u << v, std::memory_order_relaxed);
}

//==============================================================================
void MidnightEdgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();
    if (numSamples == 0 || buffer.getNumChannels() < 2)
        return;

    if (sendBuffer.getNumSamples() < numSamples)
    {
        sendBuffer.setSize (2, numSamples, false, false, true);
        gateBuffer.setSize (1, numSamples, false, false, true);
    }

    //== Gather triggers: MIDI, UI auditions and the step sequencer
    triggers.clear();
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            const int v = me::noteToVoice (msg.getNoteNumber());
            if (v >= 0)
                addTrigger (juce::jlimit (0, numSamples - 1, meta.samplePosition), v, msg.getFloatVelocity());
        }
    }

    if (const auto mask = auditionMask.exchange (0); mask != 0)
        for (int v = 0; v < me::numVoices; ++v)
            if ((mask & (1u << v)) != 0)
                addTrigger (0, v, 0.9f);

    runSequencer (numSamples);

    // Insertion sort by offset (allocation free, keeps original order for ties)
    for (size_t i = 1; i < triggers.size(); ++i)
        for (size_t j = i; j > 0 && triggers[j].offset < triggers[j - 1].offset; --j)
            std::swap (triggers[j], triggers[j - 1]);

    //== Snapshot per-voice mix settings
    struct Mix { float synthGain, sampleGain, drive, verb, gainL, gainR; };
    std::array<Mix, me::numVoices> mix {};
    for (int v = 0; v < me::numVoices; ++v)
    {
        const auto& vp = voiceParams[(size_t) v];
        const float blend = vp.get (6);
        const float pan = vp.get (7);
        const float level = juce::Decibels::decibelsToGain (vp.get (8), -47.9f);
        const float angle = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        mix[(size_t) v] = { std::min (1.0f, 2.0f * (1.0f - blend)), std::min (1.0f, 2.0f * blend),
                            vp.get (4), vp.get (5),
                            level * std::cos (angle) * juce::MathConstants<float>::sqrt2,
                            level * std::sin (angle) * juce::MathConstants<float>::sqrt2 };
        sampleLockHeld[(size_t) v] = sampleSlots[(size_t) v].lock.tryEnter();
    }

    const float gateMs = pVerbGate->load();
    const bool gateOff = gateMs >= gateOffMs - 0.5f;
    const float gateAttack = 1.0f - me::tauCoef (0.0008f, (float) sr);
    const float gateRelease = 1.0f - me::tauCoef (0.012f, (float) sr);

    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getWritePointer (1);
    auto* sendL = sendBuffer.getWritePointer (0);
    auto* sendR = sendBuffer.getWritePointer (1);
    auto* gate = gateBuffer.getWritePointer (0);

    //== Render voices
    size_t nextTrigger = 0;
    for (int s = 0; s < numSamples; ++s)
    {
        while (nextTrigger < triggers.size() && triggers[nextTrigger].offset <= s)
        {
            const auto& t = triggers[nextTrigger++];
            triggerVoice (t.voice, t.velocity);
        }

        float l = 0.0f, r = 0.0f, sl = 0.0f, sr_ = 0.0f;
        for (int v = 0; v < me::numVoices; ++v)
        {
            const auto& m = mix[(size_t) v];
            float x = 0.0f;
            auto& voice = *voices[(size_t) v];
            if (voice.isActive())
                x = voice.process() * m.synthGain;

            auto& player = samplePlayers[(size_t) v];
            if (player.playing && sampleLockHeld[(size_t) v])
            {
                if (auto* data = sampleSlots[(size_t) v].data.get())
                {
                    const int len = data->audio.getNumSamples();
                    const int i0 = (int) player.pos;
                    if (i0 + 1 < len && player.chokeGain > 1.0e-4f)
                    {
                        const float frac = (float) (player.pos - i0);
                        const float* d = data->audio.getReadPointer (0);
                        x += (d[i0] + frac * (d[i0 + 1] - d[i0])) * player.gain * player.chokeGain * m.sampleGain;
                        player.pos += player.rate;
                        player.chokeGain *= player.chokeCoef;
                    }
                    else
                    {
                        player.playing = false;
                    }
                }
                else
                {
                    player.playing = false;
                }
            }

            if (std::abs (x) < 1.0e-9f)
                continue;

            x = driveShape (x, m.drive) * voiceHeadroom;
            const float xl = x * m.gainL, xr = x * m.gainR;
            l += xl;
            r += xr;
            sl += xl * m.verb;
            sr_ += xr * m.verb;
        }

        outL[s] = l;
        outR[s] = r;
        sendL[s] = sendHpL.highpass (sl);
        sendR[s] = sendHpR.highpass (sr_);

        if (gateOff)
        {
            gateEnv = 1.0f;
        }
        else
        {
            const float target = gateCounter > 0 ? 1.0f : 0.0f;
            gateEnv += (target - gateEnv) * (target > gateEnv ? gateAttack : gateRelease);
            if (gateCounter > 0)
                --gateCounter;
        }
        gate[s] = gateEnv;
    }

    for (int v = 0; v < me::numVoices; ++v)
        if (std::exchange (sampleLockHeld[(size_t) v], false))
            sampleSlots[(size_t) v].lock.exit();

    //== Gated reverb
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.55f + 0.44f * pVerbSize->load();
        rp.damping = 0.9f - 0.8f * pVerbTone->load();
        rp.wetLevel = 1.0f;
        rp.dryLevel = 0.0f;
        rp.width = 1.0f;
        reverb.setParameters (rp);
        reverb.processStereo (sendL, sendR, numSamples);
    }

    //== Master bus: vintage -> + reverb -> punch compressor -> heat -> output
    const float vintage = pVintage->load();
    const float vintageInc = 28000.0f / (float) sr;
    const float verbGain = pVerbMix->load() * 1.6f;

    const float punch = pPunch->load();
    const float threshDb = -4.0f - 22.0f * punch;
    const float ratio = 2.0f + 6.0f * punch;
    const float makeup = juce::Decibels::decibelsToGain (5.0f * punch);
    const float compAttack = 1.0f - me::tauCoef (0.012f, (float) sr);   // slow enough to let the transient through
    const float compRelease = 1.0f - me::tauCoef (0.14f, (float) sr);

    const float heat = pHeat->load();
    const float heatGain = 1.0f + 7.0f * heat;
    const float heatNorm = 1.0f / std::tanh (heatGain);
    const float outGain = juce::Decibels::decibelsToGain (pOutput->load());

    for (int s = 0; s < numSamples; ++s)
    {
        float l = outL[s], r = outR[s];

        if (vintage > 0.0f)
        {
            vintagePhase += vintageInc;
            if (vintagePhase >= 1.0f)
            {
                vintagePhase -= std::floor (vintagePhase);
                vintageHeldL = muLaw8 (l);
                vintageHeldR = muLaw8 (r);
            }
            l += vintage * (vintageLpL.lowpass (vintageHeldL) - l);
            r += vintage * (vintageLpR.lowpass (vintageHeldR) - r);
        }

        l += sendL[s] * gate[s] * verbGain;
        r += sendR[s] * gate[s] * verbGain;

        if (punch > 0.0f)
        {
            const float peak = std::max (std::abs (l), std::abs (r));
            const float levelDb = juce::Decibels::gainToDecibels (peak, -100.0f);
            const float overDb = std::max (0.0f, levelDb - threshDb);
            const float targetDb = overDb * (1.0f - 1.0f / ratio);
            compEnvDb += (targetDb - compEnvDb) * (targetDb > compEnvDb ? compAttack : compRelease);
            const float g = juce::Decibels::decibelsToGain (-compEnvDb) * makeup;
            l *= g;
            r *= g;
        }

        if (heat > 0.0f)
        {
            l += heat * (std::tanh (heatGain * l) * heatNorm - l);
            r += heat * (std::tanh (heatGain * r) * heatNorm - r);
        }

        outL[s] = softClip (l * outGain);
        outR[s] = softClip (r * outGain);
    }

    midi.clear();
}

//==============================================================================
bool MidnightEdgeProcessor::loadSample (int voice, const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr || reader->sampleRate <= 0.0)
        return false;

    const auto maxLength = (juce::int64) (reader->sampleRate * 10.0);
    const int length = (int) std::min (reader->lengthInSamples, maxLength);
    if (length <= 1)
        return false;

    juce::AudioBuffer<float> temp ((int) reader->numChannels, length);
    reader->read (&temp, 0, length, 0, true, true);

    auto data = std::make_unique<SampleData>();
    data->audio.setSize (1, length);
    data->audio.clear();
    for (int ch = 0; ch < temp.getNumChannels(); ++ch)
        data->audio.addFrom (0, 0, temp, ch, 0, length, 1.0f / (float) temp.getNumChannels());
    data->sourceRate = reader->sampleRate;
    data->path = file.getFullPathName();
    data->name = file.getFileNameWithoutExtension();

    auto& slot = sampleSlots[(size_t) voice];
    {
        const juce::SpinLock::ScopedLockType sl (slot.lock);
        std::swap (slot.data, data);
        samplePlayers[(size_t) voice].playing = false;
    }

    // A freshly loaded sample should be audible straight away
    if (auto* blend = apvts.getParameter (voiceParamId (voice, "blend")); blend != nullptr && blend->getValue() < 0.01f)
        blend->setValueNotifyingHost (0.5f);

    return true; // the old sample (now in `data`) is freed here, off the audio thread
}

void MidnightEdgeProcessor::clearSample (int voice)
{
    std::unique_ptr<SampleData> old;
    auto& slot = sampleSlots[(size_t) voice];
    {
        const juce::SpinLock::ScopedLockType sl (slot.lock);
        std::swap (slot.data, old);
        samplePlayers[(size_t) voice].playing = false;
    }
}

juce::String MidnightEdgeProcessor::getSampleName (int voice) const
{
    auto& slot = sampleSlots[(size_t) voice];
    const juce::SpinLock::ScopedLockType sl (slot.lock);
    return slot.data != nullptr ? slot.data->name : juce::String();
}

//==============================================================================
int MidnightEdgeProcessor::getNumPrograms()
{
    return (int) me::factoryKits().size();
}

const juce::String MidnightEdgeProcessor::getProgramName (int index)
{
    const auto& kits = me::factoryKits();
    return juce::isPositiveAndBelow (index, (int) kits.size()) ? juce::String (kits[(size_t) index].name) : juce::String();
}

void MidnightEdgeProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms()))
        return;
    currentKit = index;
    applyKit (index);
}

void MidnightEdgeProcessor::applyKit (int index)
{
    const auto& kit = me::factoryKits()[(size_t) index];

    auto set = [this] (const juce::String& id, float value)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    };

    for (int v = 0; v < me::numVoices; ++v)
    {
        const auto& vs = kit.voices[(size_t) v];
        const float values[] { vs.tune, vs.decay, vs.tone, vs.snap, vs.drive, vs.verb, vs.blend, vs.pan, vs.level };
        for (size_t i = 0; i < me::voiceParamSuffixes.size(); ++i)
        {
            // Keep the sample blend if the user has a sample loaded on this voice
            if (i == 6 && getSampleName (v).isNotEmpty())
                continue;
            set (voiceParamId (v, me::voiceParamSuffixes[i]), values[i]);
        }
    }

    set ("vintage", kit.vintage);
    set ("heat", kit.heat);
    set ("punch", kit.punch);
    set ("verbSize", kit.verbSize);
    set ("verbGate", kit.verbGate);
    set ("verbTone", kit.verbTone);
    set ("verbMix", kit.verbMix);
}

//==============================================================================
juce::ValueTree MidnightEdgeProcessor::patternsToTree() const
{
    juce::ValueTree tree ("Patterns");
    for (int p = 0; p < me::numPatterns; ++p)
    {
        juce::String text;
        for (int v = 0; v < me::numVoices; ++v)
            for (int s = 0; s < me::numSteps; ++s)
                text << juce::String::charToString ((juce::juce_wchar) ('0' + getStep (p, v, s)));
        tree.setProperty ("p" + juce::String (p), text, nullptr);
    }
    return tree;
}

void MidnightEdgeProcessor::patternsFromTree (const juce::ValueTree& tree)
{
    for (int p = 0; p < me::numPatterns; ++p)
    {
        const auto text = tree.getProperty ("p" + juce::String (p)).toString();
        if (text.length() != me::numVoices * me::numSteps)
            continue;
        for (int i = 0; i < text.length(); ++i)
            setStep (p, i / me::numSteps, i % me::numSteps, (uint8_t) juce::jlimit (0, 2, (int) (text[i] - '0')));
    }
}

void MidnightEdgeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("Patterns"), nullptr);
    state.removeChild (state.getChildWithName ("Samples"), nullptr);
    state.appendChild (patternsToTree(), nullptr);

    juce::ValueTree samples ("Samples");
    for (int v = 0; v < me::numVoices; ++v)
    {
        auto& slot = sampleSlots[(size_t) v];
        const juce::SpinLock::ScopedLockType sl (slot.lock);
        if (slot.data != nullptr)
            samples.setProperty ("s" + juce::String (v), slot.data->path, nullptr);
    }
    state.appendChild (samples, nullptr);
    state.setProperty ("kit", currentKit, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MidnightEdgeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    currentKit = juce::jlimit (0, getNumPrograms() - 1, (int) state.getProperty ("kit", 0));

    if (auto patterns = state.getChildWithName ("Patterns"); patterns.isValid())
        patternsFromTree (patterns);

    // Restore samples before the parameters so the saved blend values win
    const auto samples = state.getChildWithName ("Samples");
    for (int v = 0; v < me::numVoices; ++v)
    {
        const auto path = samples.isValid() ? samples.getProperty ("s" + juce::String (v)).toString() : juce::String();
        if (path.isNotEmpty() && juce::File (path).existsAsFile())
            loadSample (v, juce::File (path));
        else
            clearSample (v);
    }

    apvts.replaceState (state);
}

//==============================================================================
juce::AudioProcessorEditor* MidnightEdgeProcessor::createEditor()
{
    return new MidnightEdgeEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidnightEdgeProcessor();
}
