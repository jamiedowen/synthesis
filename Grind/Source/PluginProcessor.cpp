#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

namespace
{
using namespace juce;

const StringArray onOff { "Off", "On" };

String percent (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; }
String bipolarPercent (float v, int) { return (v > 0.0005f ? "+" : "") + String (roundToInt (v * 100.0f)) + "%"; }

String timeString (float seconds)
{
    return seconds < 1.0f ? String (roundToInt (seconds * 1000.0f)) + " ms" : String (seconds, 2) + " s";
}

// Soft ceiling just under 0 dBFS
inline float softClip (float x) noexcept
{
    constexpr float knee = 0.9f;
    const float ax = std::abs (x);
    if (ax <= knee)
        return x;
    return std::copysign (knee + (1.0f - knee) * std::tanh ((ax - knee) / (1.0f - knee)), x);
}
} // namespace

//==============================================================================
StringArray MidnightGrindProcessor::patternNames()
{
    StringArray names;
    for (int bank = 0; bank < 8; ++bank)
        for (int slot = 0; slot < 8; ++slot)
            names.add (String::charToString ((juce_wchar) ('A' + bank)) + String (slot + 1));
    return names;
}

AudioProcessorValueTreeState::ParameterLayout MidnightGrindProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;
    using AF = AudioParameterFloatAttributes;
    const auto pct = AF().withStringFromValueFunction (percent);
    const auto bip = AF().withStringFromValueFunction (bipolarPercent);

    auto addFloat = [&] (const char* id, const char* name, NormalisableRange<float> range, float def, AF attr)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, def, attr));
    };
    auto addChoice = [&] (const char* id, const char* name, const StringArray& choices, int def)
    {
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, choices, def));
    };
    auto addBool = [&] (const char* id, const char* name, bool def)
    {
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };
    auto addInt = [&] (const char* id, const char* name, int lo, int hi, int def)
    {
        layout.add (std::make_unique<AudioParameterInt> (ParameterID { id, 1 }, name, lo, hi, def));
    };

    //== Oscillator
    StringArray models;
    for (const auto& e : grind::engineInfo)
        models.add (e.name);
    addChoice ("model", "Model", models, 0);
    addFloat ("frequency", "Frequency", { -36.0f, 36.0f, 0.01f }, 0.0f,
              AF().withLabel ("st").withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " st"; }));
    addFloat ("fine", "Fine Tune", { -50.0f, 50.0f, 0.1f }, 0.0f,
              AF().withLabel ("ct").withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " ct"; }));
    addFloat ("harmonics", "Harmonics", { 0.0f, 1.0f }, 0.5f, pct);
    addFloat ("timbre", "Timbre", { 0.0f, 1.0f }, 0.5f, pct);
    addFloat ("morph", "Morph", { 0.0f, 1.0f }, 0.5f, pct);
    addFloat ("fmAmt", "FM Attenuverter", { -1.0f, 1.0f }, 0.0f, bip);
    addFloat ("timbreAmt", "Timbre Attenuverter", { -1.0f, 1.0f }, 0.0f, bip);
    addFloat ("morphAmt", "Morph Attenuverter", { -1.0f, 1.0f }, 0.0f, bip);
    addFloat ("oscDecay", "Osc Decay", { 0.0f, 1.0f }, 0.5f, pct);
    addFloat ("lpgColour", "LPG Colour", { 0.0f, 1.0f }, 0.5f, pct);
    addBool ("lpgOn", "LPG", false);
    addFloat ("mix", "Out/Aux Mix", { 0.0f, 1.0f }, 0.0f,
              AF().withStringFromValueFunction ([] (float v, int)
              {
                  if (v < 0.005f) return String ("OUT");
                  if (v > 0.995f) return String ("AUX");
                  return String (roundToInt (v * 100.0f)) + "% AUX";
              }));
    addChoice ("dx7Bank", "DX7 Bank", { "Bank 1", "Bank 2", "Bank 3" }, 0);

    //== VCO mod
    addFloat ("vcoModAmt", "VCO Mod", { 0.0f, 1.0f }, 0.0f, pct);
    addChoice ("vcoModSrc", "VCO Mod Source", { "EG", "LFO" }, 1);
    addChoice ("vcoModDest", "VCO Mod Dest", { "Frequency", "Harmonics", "Timbre", "Morph" }, 0);

    //== VCF
    addFloat ("cutoff", "Cutoff", NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.25f), 2400.0f,
              AF().withLabel ("Hz").withStringFromValueFunction ([] (float v, int)
              {
                  return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz" : String (roundToInt (v)) + " Hz";
              }));
    addFloat ("resonance", "Resonance", { 0.0f, 1.0f }, 0.15f, pct);
    addFloat ("vcfModAmt", "VCF Mod", { 0.0f, 1.0f }, 0.35f, pct);
    addChoice ("vcfModSrc", "VCF Mod Source", { "EG", "LFO" }, 0);
    addChoice ("vcfModPol", "VCF Mod Polarity", { "+", "-" }, 0);
    addChoice ("vcfMode", "VCF Mode", { "Low Pass", "High Pass" }, 0);
    addFloat ("keyTrack", "Key Track", { 0.0f, 1.0f }, 0.0f, pct);
    addFloat ("drive", "Drive", { 0.0f, 1.0f }, 0.3f, pct);

    //== VCA / EG
    addChoice ("vcaMode", "VCA Mode", { "Env", "LPG", "On" }, 0);
    addFloat ("attack", "Attack", { 0.0f, 1.0f }, 0.0f,
              AF().withStringFromValueFunction ([] (float v, int) { return timeString (0.001f * std::pow (10000.0f, v)); }));
    addFloat ("decay", "Decay", { 0.0f, 1.0f }, 0.45f,
              AF().withStringFromValueFunction ([] (float v, int) { return timeString (0.003f * std::pow (3333.0f, v)); }));
    addFloat ("sustain", "Sustain", { 0.0f, 1.0f }, 0.7f, pct);
    addFloat ("velocity", "Velocity > VCA", { 0.0f, 1.0f }, 0.0f, pct);

    //== LFO / glide / out
    addFloat ("lfoRate", "LFO Rate", { 0.0f, 1.0f }, 0.45f,
              AF().withStringFromValueFunction ([] (float v, int)
              {
                  const float hz = 0.05f * std::pow (1600.0f, v);
                  return hz < 10.0f ? String (hz, 2) + " Hz" : String (hz, 1) + " Hz";
              }));
    addChoice ("lfoWave", "LFO Wave", { "Triangle", "Square" }, 0);
    addFloat ("glide", "Glide", { 0.0f, 1.0f }, 0.0f,
              AF().withStringFromValueFunction ([] (float v, int) { return v < 0.005f ? String ("Off") : timeString (3.0f * v * v * v); }));
    addFloat ("volume", "Volume", NormalisableRange<float> (-60.0f, 6.0f, 0.1f, 2.0f), -6.0f,
              AF().withLabel ("dB").withStringFromValueFunction ([] (float v, int)
              {
                  return v <= -59.9f ? String ("-inf") : String (v, 1) + " dB";
              }));

    //== Sequencer / arpeggiator
    addChoice ("playMode", "Play Mode", { "Keys", "Sequencer", "Arpeggiator" }, 0);
    addFloat ("tempo", "Tempo", NormalisableRange<float> (40.0f, 240.0f, 0.1f), 120.0f,
              AF().withLabel ("BPM").withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " BPM"; }));
    addBool ("seqRun", "Sequencer Run", false);
    addBool ("seqRec", "Sequencer Record", false);
    addChoice ("pattern", "Pattern", patternNames(), 0);

    StringArray rates;
    for (auto* r : grind::rateNames)
        rates.add (r);
    addChoice ("seqRate", "Sequencer Rate", rates, 3);
    addFloat ("seqGate", "Sequencer Gate", { 0.05f, 1.0f }, 0.5f, pct);
    addFloat ("swing", "Swing", { 0.0f, 1.0f }, 0.0f,
              AF().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (50.0f + 25.0f * v)) + "%"; }));

    StringArray arpPatterns;
    for (auto* a : grind::arpPatternNames)
        arpPatterns.add (a);
    addChoice ("arpPattern", "Arp Pattern", arpPatterns, 0);
    addInt ("arpOctaves", "Arp Octaves", 1, 4, 1);
    addChoice ("arpRate", "Arp Rate", rates, 3);
    addFloat ("arpGate", "Arp Gate", { 0.05f, 1.0f }, 0.5f, pct);
    addBool ("arpHold", "Arp Hold", false);

    //== Voicing / MIDI
    addInt ("voices", "Poly Chain Voices", 1, grind::dsp::maxVoices, 1);
    addChoice ("priority", "Voice Priority", { "Last", "Low", "High" }, 0);
    StringArray channels { "Omni" };
    for (int c = 1; c <= 16; ++c)
        channels.add (String (c));
    addChoice ("midiChannel", "MIDI Channel", channels, 0);
    addInt ("bendRange", "Pitch Bend Range", 0, 24, 2);

    StringArray assigns;
    for (auto* a : grind::assignNames)
        assigns.add (a);
    addChoice ("assignSrc", "Assign Output", assigns, 0);

    return layout;
}

//==============================================================================
MidnightGrindProcessor::MidnightGrindProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MidnightGrind", createLayout())
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    p = { get ("model"), get ("frequency"), get ("fine"), get ("harmonics"), get ("timbre"), get ("morph"),
          get ("fmAmt"), get ("timbreAmt"), get ("morphAmt"), get ("oscDecay"), get ("lpgColour"), get ("lpgOn"),
          get ("mix"), get ("dx7Bank"),
          get ("vcoModAmt"), get ("vcoModSrc"), get ("vcoModDest"),
          get ("cutoff"), get ("resonance"), get ("vcfModAmt"), get ("vcfModSrc"), get ("vcfModPol"), get ("vcfMode"),
          get ("keyTrack"), get ("drive"),
          get ("vcaMode"), get ("attack"), get ("decay"), get ("assignSrc"), get ("sustain"), get ("velocity"),
          get ("lfoRate"), get ("lfoWave"), get ("glide"), get ("volume"),
          get ("playMode"), get ("tempo"), get ("seqRun"), get ("seqRec"), get ("pattern"), get ("seqRate"),
          get ("seqGate"), get ("swing"),
          get ("arpPattern"), get ("arpOctaves"), get ("arpRate"), get ("arpGate"), get ("arpHold"),
          get ("voices"), get ("priority"), get ("midiChannel"), get ("bendRange") };

    for (auto& c : cables)
        c.store (-1);
    for (int i = 0; i < 4; ++i)
    {
        uiNoteOn[i].store (0);
        uiNoteOff[i].store (0);
    }

    events.reserve (1024);
    synth.init();
}

bool MidnightGrindProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == AudioChannelSet::stereo() || out == AudioChannelSet::mono();
}

void MidnightGrindProcessor::Biquad::setLowPass (double cutoff, double sampleRate, double q)
{
    const double w = MathConstants<double>::twoPi * cutoff / sampleRate;
    const double alpha = std::sin (w) / (2.0 * q);
    const double c = std::cos (w);
    const double a0 = 1.0 + alpha;
    b0 = (float) ((1.0 - c) * 0.5 / a0);
    b1 = (float) ((1.0 - c) / a0);
    b2 = b0;
    a1 = (float) (-2.0 * c / a0);
    a2 = (float) ((1.0 - alpha) / a0);
    z1 = z2 = 0.0f;
}

void MidnightGrindProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    hostRate = sampleRate;
    ratio = grind::dsp::rate / sampleRate;
    phase = 0.0;
    std::fill (std::begin (history), std::end (history), 0.0f);
    internalPos = grind::dsp::blockSize;

    // Below 48 kHz the engine output is band-limited before decimation
    useAntiAlias = sampleRate < grind::dsp::rate - 1.0;
    if (useAntiAlias)
    {
        const double cutoff = 0.45 * sampleRate;
        antiAlias[0].setLowPass (cutoff, grind::dsp::rate, 0.54);
        antiAlias[1].setLowPass (cutoff, grind::dsp::rate, 1.31);
    }

    dcState = dcPrev = 0.0f;
    mono.assign ((size_t) std::max (samplesPerBlock, 1), 0.0f);
    performance.prepare (sampleRate);
    updateParams();
    synth.reset();
}

//==============================================================================
void MidnightGrindProcessor::updateParams()
{
    using namespace grind::dsp;
    auto& s = synthParams;
    auto ld = [] (std::atomic<float>* a) { return a->load (std::memory_order_relaxed); };
    auto idx = [&] (std::atomic<float>* a) { return (int) std::lround (ld (a)); };

    s.engine = idx (p.model);
    s.dx7Bank = idx (p.dx7Bank);
    s.frequency = ld (p.frequency);
    s.fine = ld (p.fine);
    s.harmonics = ld (p.harmonics);
    s.timbre = ld (p.timbre);
    s.morph = ld (p.morph);
    s.fmAmount = ld (p.fmAmt);
    s.timbreAmount = ld (p.timbreAmt);
    s.morphAmount = ld (p.morphAmt);
    s.oscDecay = ld (p.oscDecay);
    s.lpgColour = ld (p.lpgColour);
    s.lpgOn = ld (p.lpgOn) > 0.5f;
    s.mix = ld (p.mix);
    s.vcoModAmount = ld (p.vcoModAmt);
    s.vcoModSource = idx (p.vcoModSrc);
    s.vcoModDest = idx (p.vcoModDest);
    s.cutoffHz = ld (p.cutoff);
    s.resonance = ld (p.resonance);
    s.vcfModAmount = ld (p.vcfModAmt);
    s.vcfModSource = idx (p.vcfModSrc);
    s.vcfModInvert = idx (p.vcfModPol) == 1;
    s.highPass = idx (p.vcfMode) == 1;
    s.keyTrack = ld (p.keyTrack);
    s.drive = ld (p.drive);
    s.vcaMode = idx (p.vcaMode);
    s.assignSource = idx (p.assignSrc);
    s.attack = ld (p.attack);
    s.decay = ld (p.decay);
    s.sustain = ld (p.sustain);
    s.velocity = ld (p.velocity);
    s.lfoRate = ld (p.lfoRate);
    s.lfoSquare = idx (p.lfoWave) == 1;
    s.glide = ld (p.glide);
    s.bendRange = ld (p.bendRange);
    s.voices = idx (p.voices);
    s.priority = idx (p.priority);
    for (size_t i = 0; i < cables.size(); ++i)
        s.patch[i] = cables[i].load (std::memory_order_relaxed);

    synth.setParams (s);
}

grind::Performance::Settings MidnightGrindProcessor::performanceSettings() const
{
    auto ld = [] (std::atomic<float>* a) { return a->load (std::memory_order_relaxed); };
    auto idx = [&] (std::atomic<float>* a) { return (int) std::lround (ld (a)); };

    grind::Performance::Settings s;
    s.mode = idx (p.playMode);
    s.pattern = idx (p.pattern);
    s.seqRate = idx (p.seqRate);
    s.arpRate = idx (p.arpRate);
    s.swing = ld (p.swing);
    s.seqGate = ld (p.seqGate);
    s.arpGate = ld (p.arpGate);
    s.arpPattern = idx (p.arpPattern);
    s.arpOctaves = idx (p.arpOctaves);
    s.arpHold = ld (p.arpHold) > 0.5f;
    s.seqRun = ld (p.seqRun) > 0.5f;
    s.record = ld (p.seqRec) > 0.5f;
    s.tempo = ld (p.tempo);
    return s;
}

int MidnightGrindProcessor::getCurrentPattern() const noexcept
{
    return (int) std::lround (p.pattern->load (std::memory_order_relaxed));
}

void MidnightGrindProcessor::applyEvent (const grind::NoteEvent& e)
{
    using T = grind::NoteEvent;
    switch (e.type)
    {
        case T::noteOn:     synth.noteOn (e.note, e.velocity, e.accent, e.slide); break;
        case T::noteOff:    synth.noteOff (e.note); break;
        case T::allOff:     synth.allNotesOff(); break;
        case T::pitchBend:  synth.setPitchBend (e.velocity); break;
        case T::modWheel:   synth.setModWheel (e.velocity); break;
        case T::aftertouch: synth.setAftertouch (e.velocity); break;
    }
}

float MidnightGrindProcessor::pullInternalSample()
{
    if (internalPos >= grind::dsp::blockSize)
    {
        synth.render (internalBlock, grind::dsp::blockSize);
        if (useAntiAlias)
            for (auto& x : internalBlock)
                x = antiAlias[1].process (antiAlias[0].process (x));
        internalPos = 0;
    }
    return internalBlock[internalPos++];
}

void MidnightGrindProcessor::playNoteFromEditor (int note, bool on) noexcept
{
    if (note < 0 || note > 127)
        return;
    auto& target = on ? uiNoteOn[note >> 5] : uiNoteOff[note >> 5];
    target.fetch_or (1u << (note & 31));
}

//==============================================================================
void MidnightGrindProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    if ((int) mono.size() < numSamples)
        mono.resize ((size_t) numSamples);

    updateParams();
    auto settings = performanceSettings();

    // Sequencer jacks (read from the first voice's patch bay)
    if (getCable (grind::inTempo) >= 0)
        settings.tempo *= std::exp2 (2.0 * (double) synth.getInput (grind::inTempo));
    if (getCable (grind::inHold) >= 0 && synth.getInput (grind::inHold) > 0.25f)
        settings.arpHold = true;
    {
        const float playStop = getCable (grind::inPlayStop) >= 0 ? synth.getInput (grind::inPlayStop) : 0.0f;
        if (playStop > 0.25f && lastPlayStop <= 0.25f)
        {
            settings.seqRun = ! settings.seqRun;
            if (auto* run = apvts.getParameter ("seqRun"))
                run->setValueNotifyingHost (settings.seqRun ? 1.0f : 0.0f);
        }
        lastPlayStop = playStop;

        const float reset = getCable (grind::inReset) >= 0 ? synth.getInput (grind::inReset) : 0.0f;
        if (reset > 0.25f && lastReset <= 0.25f)
            performance.restart();
        lastReset = reset;
    }
    const int channel = (int) std::lround (p.midiChannel->load());

    //== Collect events
    events.clear();
    for (int w = 0; w < 4; ++w)
    {
        const uint32_t offs = uiNoteOff[w].exchange (0);
        const uint32_t ons = uiNoteOn[w].exchange (0);
        for (int b = 0; b < 32; ++b)
        {
            if (offs & (1u << b)) performance.keyOff (settings, 0, w * 32 + b, events);
            if (ons & (1u << b))  performance.keyOn (settings, 0, w * 32 + b, 0.8f, events);
        }
    }

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (channel > 0 && msg.getChannel() != channel)
            continue;
        const int offset = juce::jlimit (0, numSamples - 1, meta.samplePosition);

        if (msg.isNoteOn())
            performance.keyOn (settings, offset, msg.getNoteNumber(), msg.getFloatVelocity(), events);
        else if (msg.isNoteOff())
            performance.keyOff (settings, offset, msg.getNoteNumber(), events);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            events.push_back ({ offset, grind::NoteEvent::allOff });
        else if (msg.isPitchWheel())
            events.push_back ({ offset, grind::NoteEvent::pitchBend, 0, (msg.getPitchWheelValue() - 8192) / 8192.0f });
        else if (msg.isControllerOfType (1))
            events.push_back ({ offset, grind::NoteEvent::modWheel, 0, msg.getControllerValue() / 127.0f });
        else if (msg.isChannelPressure())
            events.push_back ({ offset, grind::NoteEvent::aftertouch, 0, msg.getChannelPressureValue() / 127.0f });
        else if (msg.isAftertouch())
            events.push_back ({ offset, grind::NoteEvent::aftertouch, 0, msg.getAfterTouchValue() / 127.0f });
    }

    //== Clock
    grind::Performance::Transport transport;
    if (auto* ph = getPlayHead())
    {
        if (const auto pos = ph->getPosition())
        {
            transport.playing = pos->getIsPlaying();
            transport.hasPosition = pos->getPpqPosition().hasValue();
            transport.ppq = pos->getPpqPosition().orFallback (0.0);
            transport.bpm = pos->getBpm().orFallback ((double) p.tempo->load());
        }
    }
    performance.process (settings, transport, numSamples, events);

    //== Render (48 kHz engine, resampled to the host rate)
    size_t nextEvent = 0;
    float* out = mono.data();
    for (int i = 0; i < numSamples; ++i)
    {
        while (nextEvent < events.size() && events[nextEvent].offset <= i)
            applyEvent (events[nextEvent++]);

        while (phase >= 1.0)
        {
            phase -= 1.0;
            history[0] = history[1];
            history[1] = history[2];
            history[2] = history[3];
            history[3] = pullInternalSample();
        }

        // 4-point Hermite between history[1] and history[2]
        const float t = (float) phase;
        const float xm1 = history[0], x0 = history[1], x1 = history[2], x2 = history[3];
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        out[i] = ((c3 * t + c2) * t + c1) * t + x0;
        phase += ratio;
    }
    while (nextEvent < events.size())
        applyEvent (events[nextEvent++]);

    //== Output: DC blocker, volume, soft ceiling
    const float volDb = p.volume->load();
    const float gain = volDb <= -59.9f ? 0.0f : Decibels::decibelsToGain (volDb);
    const float dcCoef = 1.0f - (float) (MathConstants<double>::twoPi * 8.0 / hostRate);
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float x = out[i];
        dcState = x - dcPrev + dcCoef * dcState;
        dcPrev = x;
        out[i] = softClip (dcState * gain);
        peak = std::max (peak, std::abs (out[i]));
    }

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, out, numSamples);

    lfoLevel.store (synth.getLfoValue(), std::memory_order_relaxed);
    envLevel.store (synth.getEnvelopeLevel(), std::memory_order_relaxed);
    outLevel.store (std::max (peak, outLevel.load() * 0.9f), std::memory_order_relaxed);
    midi.clear();
}

//==============================================================================
int MidnightGrindProcessor::getCable (int input) const noexcept
{
    return isPositiveAndBelow (input, grind::numInputs) ? cables[(size_t) input].load (std::memory_order_relaxed) : -1;
}

void MidnightGrindProcessor::setCable (int input, int output) noexcept
{
    if (! isPositiveAndBelow (input, grind::numInputs))
        return;
    cables[(size_t) input].store (isPositiveAndBelow (output, grind::numOutputs) ? output : -1, std::memory_order_relaxed);
    cableVersion.fetch_add (1);
}

void MidnightGrindProcessor::clearCables() noexcept
{
    for (auto& c : cables)
        c.store (-1, std::memory_order_relaxed);
    cableVersion.fetch_add (1);
}

int MidnightGrindProcessor::countCables() const noexcept
{
    int n = 0;
    for (auto& c : cables)
        n += c.load (std::memory_order_relaxed) >= 0 ? 1 : 0;
    return n;
}

//==============================================================================
int MidnightGrindProcessor::getNumPrograms()
{
    return (int) grind::factoryPresets().size();
}

const String MidnightGrindProcessor::getProgramName (int index)
{
    const auto& presets = grind::factoryPresets();
    return isPositiveAndBelow (index, (int) presets.size()) ? String (presets[(size_t) index].name) : String();
}

void MidnightGrindProcessor::setCurrentProgram (int index)
{
    if (! isPositiveAndBelow (index, getNumPrograms()))
        return;
    currentProgram = index;
    applyProgram (index);
}

void MidnightGrindProcessor::applyProgram (int index)
{
    const auto& preset = grind::factoryPresets()[(size_t) index];

    // Everything back to default, then the preset's own settings
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<RangedAudioParameter*> (param))
        {
            if (ranged->getParameterID() == "midiChannel")
                continue;
            ranged->setValueNotifyingHost (ranged->getDefaultValue());
        }

    for (const auto& [id, value] : preset.values)
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));

    clearCables();
    for (const auto& c : preset.cables)
        setCable (c.input, c.output);
}

juce::ValueTree MidnightGrindProcessor::extraStateToTree() const
{
    ValueTree extra ("EXTRA");
    extra.setProperty ("program", currentProgram, nullptr);

    String cableText;
    for (int i = 0; i < grind::numInputs; ++i)
        if (const int o = getCable (i); o >= 0)
            cableText << i << ":" << o << ";";
    extra.setProperty ("cables", cableText, nullptr);

    ValueTree patterns ("PATTERNS");
    for (int pat = 0; pat < grind::numPatterns; ++pat)
    {
        String stepsText;
        for (int st = 0; st < grind::numSteps; ++st)
            stepsText << String::toHexString ((int) performance.getStep (pat, st).pack()) << (st < grind::numSteps - 1 ? "," : "");
        ValueTree pt ("P");
        pt.setProperty ("i", pat, nullptr);
        pt.setProperty ("len", performance.getLength (pat), nullptr);
        pt.setProperty ("steps", stepsText, nullptr);
        patterns.appendChild (pt, nullptr);
    }
    extra.appendChild (patterns, nullptr);
    return extra;
}

void MidnightGrindProcessor::extraStateFromTree (const ValueTree& extra)
{
    if (! extra.isValid())
        return;
    currentProgram = extra.getProperty ("program", 0);

    clearCables();
    StringArray cableTokens;
    cableTokens.addTokens (extra.getProperty ("cables").toString(), ";", "");
    for (const auto& c : cableTokens)
    {
        StringArray parts;
        parts.addTokens (c, ":", "");
        if (parts.size() == 2)
            setCable (parts[0].getIntValue(), parts[1].getIntValue());
    }

    const auto patterns = extra.getChildWithName ("PATTERNS");
    for (const auto& pt : patterns)
    {
        const int pat = pt.getProperty ("i", -1);
        if (! isPositiveAndBelow (pat, grind::numPatterns))
            continue;
        performance.setLength (pat, pt.getProperty ("len", 16));
        StringArray stepTokens;
        stepTokens.addTokens (pt.getProperty ("steps").toString(), ",", "");
        for (int st = 0; st < std::min (stepTokens.size(), grind::numSteps); ++st)
            performance.setStep (pat, st, grind::Step::unpack ((uint32_t) stepTokens[st].getHexValue32()));
    }
}

void MidnightGrindProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("EXTRA"), nullptr);
    state.appendChild (extraStateToTree(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MidnightGrindProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = ValueTree::fromXml (*xml);
        if (! state.hasType (apvts.state.getType()))
            return;
        const auto extra = state.getChildWithName ("EXTRA").createCopy();
        state.removeChild (state.getChildWithName ("EXTRA"), nullptr);
        apvts.replaceState (state);
        extraStateFromTree (extra);
    }
}

AudioProcessorEditor* MidnightGrindProcessor::createEditor()
{
    return new grind::ui::MidnightGrindEditor (*this);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidnightGrindProcessor();
}
