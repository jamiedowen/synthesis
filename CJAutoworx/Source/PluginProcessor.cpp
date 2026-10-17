#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr float quizLength = 8.0f;      // seconds of scripted driving per quiz round
constexpr float passByLength = 6.0f;
constexpr float starterSeconds = 0.85f;

// Transparent below -0.9 dBFS, then rounds off smoothly into a 0 dBFS ceiling
inline float softClip (float x) noexcept
{
    constexpr float knee = 0.9f;
    const float ax = std::abs (x);
    if (ax <= knee)
        return x;
    return std::copysign (knee + (1.0f - knee) * std::tanh ((ax - knee) / (1.0f - knee)), x);
}

inline float noteToHz (float note) noexcept { return 440.0f * std::exp2 ((note - 69.0f) / 12.0f); }

// ENGINE mode: the keyboard is the accelerator. C2 is a gentle push, C5 and up is flat out.
inline float noteToThrottle (int note) noexcept { return juce::jlimit (0.15f, 1.0f, 0.15f + 0.85f * (float) (note - 36) / 36.0f); }

// The quiz drives each engine the same way: idle, a blip, idle, a full pull, then lift off for the pops
inline float quizThrottle (float t) noexcept
{
    if (t < 1.4f) return 0.0f;
    if (t < 1.75f) return 1.0f;
    if (t < 3.0f) return 0.0f;
    if (t < 5.6f) return 1.0f;
    return 0.0f;
}

const juce::StringArray& layoutNames()
{
    static const juce::StringArray names = []
    {
        juce::StringArray n;
        for (const auto& l : cj::layouts())
            n.add (l.name);
        return n;
    }();
    return names;
}
} // namespace

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout CJAutoworxProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    const auto& d = cj::factoryPresets()[0];

    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; });
    auto rpm = AudioParameterFloatAttributes().withLabel ("rpm").withStringFromValueFunction (
        [] (float v, int) { return String (roundToInt (v)) + " rpm"; });
    auto ms = AudioParameterFloatAttributes().withLabel ("ms").withStringFromValueFunction (
        [] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 2) + " s" : String (roundToInt (v)) + " ms"; });

    auto addFloat = [&] (const char* id, const char* name, NormalisableRange<float> range, float def,
                         AudioParameterFloatAttributes attr)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, def, attr));
    };

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode", StringArray { "Engine", "Musical" }, d.mode));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "ignition", 1 }, "Ignition", true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "layout", 1 }, "Engine Layout", layoutNames(), d.layout));

    addFloat ("idle", "Idle", NormalisableRange<float> (500.0f, 1500.0f, 10.0f), d.idle, rpm);
    addFloat ("redline", "Redline", NormalisableRange<float> (3000.0f, 12000.0f, 50.0f), d.redline, rpm);
    addFloat ("flywheel", "Flywheel", { 0.0f, 1.0f }, d.flywheel, pct);
    addFloat ("throttle", "Throttle", { 0.0f, 1.0f }, 0.0f, pct);
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "gearbox", 1 }, "Auto Shift", d.gearbox));

    addFloat ("pulse", "Pulse Width", { 0.0f, 1.0f }, d.pulse, pct);
    addFloat ("rough", "Roughness", { 0.0f, 1.0f }, d.rough, pct);
    addFloat ("rasp", "Rasp", { 0.0f, 1.0f }, d.rasp, pct);
    addFloat ("mech", "Mechanical", { 0.0f, 1.0f }, d.mech, pct);

    addFloat ("pipe", "Pipe Length", NormalisableRange<float> (0.3f, 3.0f, 0.01f, 0.6f), d.pipe,
              AudioParameterFloatAttributes().withLabel ("m").withStringFromValueFunction (
                  [] (float v, int) { return String (v, 2) + " m"; }));
    addFloat ("reso", "Pipe Resonance", { 0.0f, 1.0f }, d.reso, pct);
    addFloat ("muffler", "Muffler", { 0.0f, 1.0f }, d.muffler,
              AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
              {
                  if (v < 0.02f) return String ("Straight");
                  return String (roundToInt (v * 100.0f)) + "%";
              }));
    addFloat ("crackle", "Crackle", { 0.0f, 1.0f }, d.crackle, pct);
    addFloat ("width", "Bank Width", { 0.0f, 1.0f }, d.width, pct);

    addFloat ("intake", "Intake", { 0.0f, 1.0f }, d.intake, pct);
    addFloat ("turbo", "Turbo", { 0.0f, 1.0f }, d.turbo, pct);
    addFloat ("blower", "Supercharger", { 0.0f, 1.0f }, d.blower, pct);

    addFloat ("attack", "Attack", NormalisableRange<float> (1.0f, 3000.0f, 0.1f, 0.3f), d.attack, ms);
    addFloat ("decay", "Decay", NormalisableRange<float> (5.0f, 5000.0f, 0.1f, 0.3f), d.decay, ms);
    addFloat ("sustain", "Sustain", { 0.0f, 1.0f }, d.sustain, pct);
    addFloat ("release", "Release", NormalisableRange<float> (5.0f, 6000.0f, 0.1f, 0.3f), d.release, ms);
    addFloat ("glide", "Glide", NormalisableRange<float> (0.0f, 1000.0f, 0.1f, 0.4f), d.glide, ms);
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "octave", 1 }, "Octave", -3, 2, d.octave));
    addFloat ("revAmt", "Rev-In", { 0.0f, 1.0f }, d.revAmt,
              AudioParameterFloatAttributes().withStringFromValueFunction (
                  [] (float v, int) { return v < 0.005f ? String ("Off") : "-" + String (v * 24.0f, 1) + " st"; }));
    addFloat ("revTime", "Rev-In Time", NormalisableRange<float> (10.0f, 2000.0f, 0.1f, 0.4f), d.revTime, ms);
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "mono", 1 }, "Mono", d.mono));

    addFloat ("garage", "Garage", { 0.0f, 1.0f }, d.garage, pct);
    addFloat ("output", "Output", NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f,
              AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (
                  [] (float x, int) { return String (x, 1) + " dB"; }));

    return layout;
}

//==============================================================================
CJAutoworxProcessor::CJAutoworxProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CJAutoworx", createLayout())
{
    pMode     = apvts.getRawParameterValue ("mode");
    pIgnition = apvts.getRawParameterValue ("ignition");
    pLayout   = apvts.getRawParameterValue ("layout");
    pIdle     = apvts.getRawParameterValue ("idle");
    pRedline  = apvts.getRawParameterValue ("redline");
    pFlywheel = apvts.getRawParameterValue ("flywheel");
    pThrottle = apvts.getRawParameterValue ("throttle");
    pGearbox  = apvts.getRawParameterValue ("gearbox");
    pPulse    = apvts.getRawParameterValue ("pulse");
    pRough    = apvts.getRawParameterValue ("rough");
    pRasp     = apvts.getRawParameterValue ("rasp");
    pMech     = apvts.getRawParameterValue ("mech");
    pPipe     = apvts.getRawParameterValue ("pipe");
    pReso     = apvts.getRawParameterValue ("reso");
    pMuffler  = apvts.getRawParameterValue ("muffler");
    pCrackle  = apvts.getRawParameterValue ("crackle");
    pWidth    = apvts.getRawParameterValue ("width");
    pIntake   = apvts.getRawParameterValue ("intake");
    pTurbo    = apvts.getRawParameterValue ("turbo");
    pBlower   = apvts.getRawParameterValue ("blower");
    pAttack   = apvts.getRawParameterValue ("attack");
    pDecay    = apvts.getRawParameterValue ("decay");
    pSustain  = apvts.getRawParameterValue ("sustain");
    pRelease  = apvts.getRawParameterValue ("release");
    pGlide    = apvts.getRawParameterValue ("glide");
    pOctave   = apvts.getRawParameterValue ("octave");
    pRevAmt   = apvts.getRawParameterValue ("revAmt");
    pRevTime  = apvts.getRawParameterValue ("revTime");
    pMono     = apvts.getRawParameterValue ("mono");
    pGarage   = apvts.getRawParameterValue ("garage");
    pOutput   = apvts.getRawParameterValue ("output");

    monoStack.reserve (128);
}

bool CJAutoworxProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void CJAutoworxProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    const auto fs = (float) sampleRate;

    for (auto& v : voices)
    {
        v.engine.prepare (fs);
        v.env.setSampleRate (sampleRate);
        v.env.reset();
        v.note = -1;
        v.held = false;
    }
    mainEngine.prepare (fs);
    quizEngine.prepare (fs);
    mainCrank.prepare (fs);
    quizCrank.prepare (fs);
    mainCrank.reset (0.0f);

    engineRunning = false;
    starterSamples = 0;
    engineGain = 0.0f;
    throttleSmoothed = 0.0f;
    monoStack.clear();
    keysDown.fill (false);
    keysHeld = 0;
    keyThrottle = 0.0f;
    lastFreq = 0.0f;

    passLpL.setCutoff (18000.0f, fs);
    passLpR.setCutoff (18000.0f, fs);
    passLpL.reset();
    passLpR.reset();
    passT = -1.0f;
    quizT = -1.0f;
    quizPlaying.store (false);
    passByActive.store (false);

    garage.setSampleRate (sampleRate);
    garage.reset();
    wetBuffer.setSize (2, samplesPerBlock);
    lastLayout = -1;
    updateBlockSettings();
}

//==============================================================================
void CJAutoworxProcessor::updateBlockSettings()
{
    const int newMode = juce::jlimit (0, 1, (int) pMode->load());
    if (newMode != mode)
    {
        allNotesOff();
        mainEngine.reset();
        mode = newMode;
    }

    settings.layout = juce::jlimit (0, cj::numLayouts - 1, (int) pLayout->load());
    settings.pulse = pPulse->load();
    settings.rough = pRough->load();
    settings.rasp = pRasp->load();
    settings.mech = pMech->load();
    settings.pipe = pPipe->load();
    settings.reso = pReso->load();
    settings.muffler = pMuffler->load();
    settings.crackle = pCrackle->load();
    settings.width = pWidth->load();
    settings.intake = pIntake->load();
    settings.turbo = pTurbo->load();
    settings.blower = pBlower->load();

    if (settings.layout != lastLayout)
    {
        lastLayout = settings.layout;
        mainEngine.reset();
    }

    mainCrank.setParams (pIdle->load(), pRedline->load(), pFlywheel->load(), pGearbox->load() > 0.5f);

    adsr.attack = pAttack->load() * 0.001f;
    adsr.decay = pDecay->load() * 0.001f;
    adsr.sustain = pSustain->load();
    adsr.release = pRelease->load() * 0.001f;
    for (auto& v : voices)
        v.env.setParameters (adsr);

    const float glideMs = pGlide->load();
    glideCoef = glideMs < 0.5f ? 1.0f : 1.0f - cj::tauCoef (glideMs * 0.001f / 3.0f, (float) sr);
    revCoef = cj::tauCoef (pRevTime->load() * 0.001f / 3.0f, (float) sr);
}

//==============================================================================
void CJAutoworxProcessor::handleMidi (const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
        noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
    else if (msg.isNoteOff())
        noteOff (msg.getNoteNumber());
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        allNotesOff();
    else if (msg.isPitchWheel())
        pitchBend = (float) (msg.getPitchWheelValue() - 8192) / 8192.0f;
    else if (msg.isControllerOfType (1))
        modWheel = (float) msg.getControllerValue() / 127.0f;
}

void CJAutoworxProcessor::noteOn (int note, float velocity)
{
    if (mode == cj::engineMode)
    {
        if (! keysDown[(size_t) note])
            ++keysHeld;
        keysDown[(size_t) note] = true;
        keyThrottle = std::max (keyThrottle, noteToThrottle (note));
        return;
    }

    const float hz = noteToHz ((float) note);
    const float startRev = -24.0f * pRevAmt->load();

    if (pMono->load() > 0.5f)
    {
        monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
        monoStack.push_back (note);

        auto& v = voices[0];
        const bool legato = v.held && v.env.isActive();
        v.note = note;
        v.targetFreq = hz;
        v.velocity = velocity;
        v.held = true;
        if (! legato)
        {
            v.freq = lastFreq > 0.0f && glideCoef < 1.0f ? lastFreq : hz;
            v.revOffset = startRev;
            if (! v.env.isActive())
                v.engine.reset();
            v.env.noteOn();
        }
        lastFreq = hz;
        return;
    }

    // Poly: a free voice, else steal the oldest
    MusicalVoice* target = nullptr;
    for (auto& v : voices)
        if (! v.env.isActive() && (target == nullptr || v.age < target->age))
            target = &v;
    if (target == nullptr)
    {
        target = &voices[0];
        for (auto& v : voices)
            if (v.age < target->age)
                target = &v;
    }

    if (! target->env.isActive())
        target->engine.reset();
    target->note = note;
    target->targetFreq = hz;
    target->freq = lastFreq > 0.0f && glideCoef < 1.0f ? lastFreq : hz;
    target->velocity = velocity;
    target->revOffset = startRev;
    target->held = true;
    target->age = ++noteCounter;
    target->env.noteOn();
    lastFreq = hz;
}

void CJAutoworxProcessor::noteOff (int note)
{
    if (mode == cj::engineMode)
    {
        if (keysDown[(size_t) note])
            --keysHeld;
        keysDown[(size_t) note] = false;
        keyThrottle = 0.0f;
        for (int n = 127; n >= 0 && keysHeld > 0; --n)
            if (keysDown[(size_t) n])
            {
                keyThrottle = noteToThrottle (n);
                break;
            }
        return;
    }

    if (pMono->load() > 0.5f)
    {
        monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
        auto& v = voices[0];
        if (! monoStack.empty())
        {
            v.note = monoStack.back();
            v.targetFreq = noteToHz ((float) v.note);
            lastFreq = v.targetFreq;
        }
        else if (v.note == note)
        {
            v.held = false;
            v.env.noteOff();
        }
        return;
    }

    for (auto& v : voices)
        if (v.held && v.note == note)
        {
            v.held = false;
            v.env.noteOff();
        }
}

void CJAutoworxProcessor::allNotesOff()
{
    for (auto& v : voices)
    {
        v.held = false;
        v.env.noteOff();
    }
    monoStack.clear();
    keysDown.fill (false);
    keysHeld = 0;
    keyThrottle = 0.0f;
}

float CJAutoworxProcessor::engineModeThrottle() const noexcept
{
    float t = std::max ({ pThrottle->load(), keyThrottle, modWheel, revButton.load() ? 1.0f : 0.0f });
    if (passT >= 0.0f)
        t = std::max (t, 0.8f);
    return t;
}

//==============================================================================
void CJAutoworxProcessor::render (float* outL, float* outR, int start, int num)
{
    const auto& lay = cj::layouts()[(size_t) settings.layout];
    const auto fs = (float) sr;
    const float redline = pRedline->load();
    const float throttleK = 1.0f - cj::tauCoef (0.015f, fs);
    const float gainK = 1.0f - cj::tauCoef (0.05f, fs);
    const float octaveRatio = std::exp2 (pOctave->load());
    const float bend = pitchBend * 2.0f;
    uint32_t fired = 0;

    for (int s = start; s < start + num; ++s)
    {
        //== Pass-by: the car drives past 6 m away at 100 km/h
        if (passT >= 0.0f)
        {
            constexpr float speed = 28.0f, distance = 6.0f, soundSpeed = 343.0f;
            const float x = speed * (passT - passByLength * 0.5f);
            const float r = std::sqrt (x * x + distance * distance);
            const float edge = std::min ({ 1.0f, passT / 0.4f, (passByLength - passT) / 0.4f });
            passPitch = 1.0f + edge * (soundSpeed / (soundSpeed + x * speed / r) - 1.0f);
            passGain = 1.0f + edge * (std::min (1.0f, 1.4f * distance / r) - 1.0f);
            passPan = edge * x / r;
            if ((s & 31) == 0)
            {
                const float cutoff = 2500.0f + 15500.0f * (1.0f - edge * (1.0f - distance / r));
                passLpL.setCutoff (cutoff, fs);
                passLpR.setCutoff (cutoff, fs);
            }
            passT += 1.0f / fs;
            if (passT >= passByLength)
            {
                passT = -1.0f;
                passPitch = passGain = 1.0f;
                passPan = 0.0f;
                passByActive.store (false);
            }
        }

        float l = 0.0f, r = 0.0f;

        if (mode == cj::engineMode)
        {
            const bool ignitionOn = pIgnition->load() > 0.5f;
            if (ignitionOn && ! engineRunning)
            {
                engineRunning = true;
                starterSamples = (int) (starterSeconds * fs);
                mainCrank.setRpm (0.0f);
            }
            else if (! ignitionOn)
            {
                engineRunning = false;
                starterSamples = 0;
            }

            throttleSmoothed += (engineModeThrottle() - throttleSmoothed) * throttleK;
            float throttle = throttleSmoothed;
            bool ignition;
            float starter = 0.0f;

            if (starterSamples > 0)
            {
                // Starter motor: cranking compressions, no combustion yet, then it catches with a flare
                const float progress = 1.0f - (float) starterSamples / (starterSeconds * fs);
                --starterSamples;
                mainCrank.setRpm (240.0f * std::min (1.0f, progress * 5.0f));
                ignition = false;
                throttle = 0.0f;
                starterPhase += 410.0f / fs;
                starterPhase -= std::floor (starterPhase);
                starter = (std::sin (cj::twoPi * starterPhase) + 0.35f * std::sin (2.0f * cj::twoPi * starterPhase))
                          * 0.035f * std::min (1.0f, progress * 12.0f);
            }
            else
            {
                const float sinceCatch = (float) -starterSamples / fs;
                if (engineRunning && sinceCatch < 0.45f)
                {
                    --starterSamples;
                    throttle = std::max (throttle, 0.4f * (1.0f - sinceCatch / 0.45f));
                }
                ignition = mainCrank.step (throttle, engineRunning);
            }

            const float rpm = mainCrank.getRpm();
            const float audible = std::min (1.0f, rpm / 200.0f);
            engineGain += ((engineRunning || rpm > 20.0f ? 1.0f : 0.0f) - engineGain) * gainK;

            if (engineGain > 1.0e-4f || engineRunning)
            {
                float el, er;
                mainEngine.process (settings, cj::rpmToFiringHz (lay, rpm) * passPitch, throttle,
                                    rpm / redline, ignition, el, er);
                const float g = 0.75f * engineGain * audible;
                l += el * g + starter;
                r += er * g + starter;
            }
        }
        else
        {
            const float bendRatio = cj::semitonesToRatio (bend) * octaveRatio * passPitch;
            for (auto& v : voices)
            {
                if (! v.env.isActive())
                    continue;

                v.freq += (v.targetFreq - v.freq) * glideCoef;
                v.revOffset *= revCoef;
                const float hz = v.freq * cj::semitonesToRatio (v.revOffset) * bendRatio;
                const float throttle = v.held ? std::min (1.0f, 0.3f + 0.7f * v.velocity + modWheel) : 0.0f;
                const float rpmNorm = cj::firingHzToRpm (lay, hz) / redline;
                const float env = v.env.getNextSample();

                float el, er;
                v.engine.process (settings, hz, throttle, rpmNorm, true, el, er);
                const float g = env * (0.35f + 0.65f * v.velocity) * 0.7f;
                l += el * g;
                r += er * g;
            }
        }

        // Pass-by moves the whole car; the quiz ducks it out of the way
        if (passT >= 0.0f)
        {
            const float panL = std::sqrt (0.5f * (1.0f - passPan)) * juce::MathConstants<float>::sqrt2;
            const float panR = std::sqrt (0.5f * (1.0f + passPan)) * juce::MathConstants<float>::sqrt2;
            l = passLpL.lowpass (l) * passGain * panL;
            r = passLpR.lowpass (r) * passGain * panR;
        }

        if (quizT >= 0.0f)
        {
            const float duck = std::max (0.0f, 1.0f - quizT * 4.0f) + std::max (0.0f, (quizT - quizLength + 0.3f) / 0.3f);
            l *= std::min (1.0f, duck);
            r *= std::min (1.0f, duck);

            const auto& qp = cj::factoryPresets()[(size_t) quizPreset];
            const auto& qLay = cj::layouts()[(size_t) qp.layout];
            const float thr = quizThrottle (quizT);
            const bool ign = quizCrank.step (thr, true);
            const float rpm = quizCrank.getRpm();
            float ql, qr;
            quizEngine.process (quizSettings, cj::rpmToFiringHz (qLay, rpm), thr, rpm / qp.redline, ign, ql, qr);
            const float fade = std::min ({ 1.0f, quizT / 0.05f, (quizLength - quizT) / 0.3f });
            l += ql * 0.75f * fade;
            r += qr * 0.75f * fade;

            quizT += 1.0f / fs;
            if (quizT >= quizLength)
            {
                quizT = -1.0f;
                quizPlaying.store (false);
            }
        }

        outL[s] = l;
        outR[s] = r;
    }

    if (mode == cj::engineMode)
        fired |= mainEngine.takeFiredMask();
    else
        for (auto& v : voices)
            fired |= v.engine.takeFiredMask();
    quizEngine.takeFiredMask();
    if (quizT < 0.0f) // no peeking at the cylinder lights during the quiz
        firedMask.fetch_or (fired, std::memory_order_relaxed);
}

//==============================================================================
void CJAutoworxProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();
    if (numSamples == 0 || buffer.getNumChannels() < 2)
        return;

    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);
    updateBlockSettings();

    if (passByRequest.exchange (false))
    {
        passT = 0.0f;
        passByActive.store (true);
    }

    if (const int q = quizRequest.exchange (-1); juce::isPositiveAndBelow (q, cj::numEnginePresets))
    {
        const auto& p = cj::factoryPresets()[(size_t) q];
        quizPreset = q;
        quizSettings = cj::settingsFromPreset (p);
        quizCrank.setParams (p.idle, p.redline, p.flywheel, p.gearbox);
        quizCrank.reset (p.idle);
        quizEngine.reset();
        quizT = 0.0f;
        quizPlaying.store (true);
    }

    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getWritePointer (1);

    // Render in slices between MIDI events so notes land sample-accurately
    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, numSamples, meta.samplePosition);
        if (t > pos)
        {
            render (outL, outR, pos, t - pos);
            pos = t;
        }
        handleMidi (meta.getMessage());
    }
    if (pos < numSamples)
        render (outL, outR, pos, numSamples - pos);

    //== Garage reverb, then output
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.42f;
        rp.damping = 0.45f;
        rp.width = 0.9f;
        rp.dryLevel = 0.5f;
        rp.wetLevel = pGarage->load() * 0.3f;
        garage.setParameters (rp);
        garage.processStereo (outL, outR, numSamples);
    }

    const float outGain = juce::Decibels::decibelsToGain (pOutput->load());
    for (int s = 0; s < numSamples; ++s)
    {
        outL[s] = softClip (outL[s] * outGain);
        outR[s] = softClip (outR[s] * outGain);
    }

    //== Dashboard readouts
    if (quizT >= 0.0f)
    {
        displayRpm.store (quizCrank.getRpm());
        displayRedline.store (cj::factoryPresets()[(size_t) quizPreset].redline);
        displayBoost.store (quizEngine.getBoost());
        displayGear.store (quizCrank.getGear());
        displayThrottle.store (quizThrottle (quizT));
    }
    else if (mode == cj::engineMode)
    {
        displayRpm.store (mainCrank.getRpm());
        displayRedline.store (pRedline->load());
        displayBoost.store (mainEngine.getBoost());
        displayGear.store (mainCrank.getGear());
        displayThrottle.store (throttleSmoothed);
    }
    else
    {
        // Show the most recently played voice as if it were the engine
        const MusicalVoice* newest = nullptr;
        for (const auto& v : voices)
            if (v.env.isActive() && (newest == nullptr || v.age > newest->age))
                newest = &v;
        const auto& lay = cj::layouts()[(size_t) settings.layout];
        const float hz = newest != nullptr ? newest->freq * cj::semitonesToRatio (newest->revOffset) * std::exp2 (pOctave->load()) : 0.0f;
        displayRpm.store (cj::firingHzToRpm (lay, hz));
        displayRedline.store (pRedline->load());
        displayBoost.store (newest != nullptr ? newest->engine.getBoost() : 0.0f);
        displayGear.store (-1);
        displayThrottle.store (newest != nullptr && newest->held ? newest->velocity : 0.0f);
    }

    midi.clear();
}

//==============================================================================
int CJAutoworxProcessor::getNumPrograms()
{
    return (int) cj::factoryPresets().size();
}

const juce::String CJAutoworxProcessor::getProgramName (int index)
{
    const auto& presets = cj::factoryPresets();
    return juce::isPositiveAndBelow (index, (int) presets.size()) ? juce::String (presets[(size_t) index].name) : juce::String();
}

void CJAutoworxProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms()))
        return;
    currentPreset = index;
    applyPreset (index);
}

void CJAutoworxProcessor::applyPreset (int index)
{
    const auto& p = cj::factoryPresets()[(size_t) index];

    auto set = [this] (const juce::String& id, float value)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };

    set ("mode", (float) p.mode);
    set ("layout", (float) p.layout);
    set ("idle", p.idle);
    set ("redline", p.redline);
    set ("flywheel", p.flywheel);
    set ("gearbox", p.gearbox ? 1.0f : 0.0f);
    set ("pulse", p.pulse);
    set ("rough", p.rough);
    set ("rasp", p.rasp);
    set ("mech", p.mech);
    set ("pipe", p.pipe);
    set ("reso", p.reso);
    set ("muffler", p.muffler);
    set ("crackle", p.crackle);
    set ("width", p.width);
    set ("intake", p.intake);
    set ("turbo", p.turbo);
    set ("blower", p.blower);
    set ("attack", p.attack);
    set ("decay", p.decay);
    set ("sustain", p.sustain);
    set ("release", p.release);
    set ("glide", p.glide);
    set ("octave", (float) p.octave);
    set ("revAmt", p.revAmt);
    set ("revTime", p.revTime);
    set ("mono", p.mono ? 1.0f : 0.0f);
    set ("garage", p.garage);
}

//==============================================================================
void CJAutoworxProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void CJAutoworxProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    currentPreset = juce::jlimit (0, getNumPrograms() - 1, (int) state.getProperty ("preset", 0));
    apvts.replaceState (state);
}

//==============================================================================
juce::AudioProcessorEditor* CJAutoworxProcessor::createEditor()
{
    return new CJAutoworxEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CJAutoworxProcessor();
}
