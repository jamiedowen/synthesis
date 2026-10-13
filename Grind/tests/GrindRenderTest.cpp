// Offline render test for Midnight Grind.
// Renders every oscillator engine and every factory preset to WAV, checks for
// NaNs / silence / clipping, checks pitch at several host sample rates, runs
// the sequencer, arpeggiator, Poly Chain and patch bay, round-trips the
// plugin state, measures CPU and saves a screenshot of the editor.
//
//   cmake -S Grind -B build-grind-test -DMIDNIGHT_GRIND_BUILD_TESTS=ON
//   cmake --build build-grind-test --target MidnightGrindRenderTest
//   ./build-grind-test/MidnightGrindRenderTest_artefacts/Release/MidnightGrindRenderTest <output-dir>

#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/Presets.h"

#include <iostream>

namespace
{
constexpr int blockSize = 256;

struct FakePlayHead : juce::AudioPlayHead
{
    double bpm = 120.0, ppq = 0.0;
    bool playing = true;

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setIsPlaying (playing);
        info.setBpm (bpm);
        info.setPpqPosition (ppq);
        return info;
    }
};

struct Stats { float peak = 0, rms = 0; bool finite = true; };

Stats analyse (const juce::AudioBuffer<float>& b)
{
    Stats s;
    double sum = 0;
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float x = b.getSample (ch, i);
            if (! std::isfinite (x)) s.finite = false;
            s.peak = std::max (s.peak, std::abs (x));
            sum += (double) x * x;
        }
    s.rms = (float) std::sqrt (sum / std::max (1, b.getNumChannels() * b.getNumSamples()));
    return s;
}

void writeWav (const juce::AudioBuffer<float>& b, double rate, const juce::File& file)
{
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> stream (new juce::FileOutputStream (file));
    auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (rate)
                                                   .withNumChannels (2).withBitsPerSample (24));
    if (writer != nullptr)
        writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

using MidiFn = std::function<void (juce::MidiBuffer&, int pos, int n)>;

juce::AudioBuffer<float> render (MidnightGrindProcessor& p, double rate, double seconds, FakePlayHead* ph,
                                 MidiFn midiForBlock = {})
{
    const int total = (int) (seconds * rate);
    juce::AudioBuffer<float> out (2, total), block (2, blockSize);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < total; pos += blockSize)
    {
        const int n = std::min (blockSize, total - pos);
        block.setSize (2, n, false, false, true);
        midi.clear();
        if (midiForBlock) midiForBlock (midi, pos, n);
        p.processBlock (block, midi);
        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, pos, block, ch, 0, n);
        if (ph != nullptr && ph->playing)
            ph->ppq += n * ph->bpm / (60.0 * rate);
    }
    return out;
}

// Note on at `onAt` seconds, off at `offAt` seconds
MidiFn noteAt (double rate, int note, double onAt, double offAt, int velocity = 100)
{
    return [=] (juce::MidiBuffer& m, int pos, int n)
    {
        const int on = (int) (onAt * rate), off = (int) (offAt * rate);
        if (on >= pos && on < pos + n) m.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) velocity), on - pos);
        if (off >= pos && off < pos + n) m.addEvent (juce::MidiMessage::noteOff (1, note), off - pos);
    };
}

MidiFn chordAt (double rate, std::vector<int> notes, double onAt, double offAt)
{
    return [=] (juce::MidiBuffer& m, int pos, int n)
    {
        for (int note : notes)
            noteAt (rate, note, onAt, offAt) (m, pos, n);
    };
}

void setParam (MidnightGrindProcessor& p, const juce::String& id, float value)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (value));
}

// Fundamental from positive-going zero crossings (for clean, periodic signals)
double measurePitch (const juce::AudioBuffer<float>& b, double rate, int start)
{
    int crossings = 0, first = -1, last = -1;
    for (int i = start + 1; i < b.getNumSamples(); ++i)
        if (b.getSample (0, i - 1) <= 0.0f && b.getSample (0, i) > 0.0f)
        {
            if (first < 0) first = i;
            last = i;
            ++crossings;
        }
    return crossings > 1 ? (crossings - 1) * rate / (double) (last - first) : 0.0;
}

int failures = 0;
void check (bool ok, const juce::String& what)
{
    if (! ok) { ++failures; std::cout << "  FAIL: " << what << "\n"; }
}

juce::String dB (float g) { return juce::String (juce::Decibels::gainToDecibels (g, -120.0f), 1); }
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir (argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1])
                                      : juce::File::getCurrentWorkingDirectory().getChildFile ("grind-render-test"));
    outDir.createDirectory();
    constexpr double rate = 48000.0;

    //== Every engine, one note
    std::cout << "Engines (C3, 1.5 s):\n";
    for (int e = 0; e < grind::numEngines; ++e)
    {
        MidnightGrindProcessor p;
        setParam (p, "model", (float) e);
        setParam (p, "cutoff", 12000.0f);
        setParam (p, "vcaMode", grind::engineIsPercussive (e) || e == 20 ? 1.0f : 0.0f);
        p.prepareToPlay (rate, blockSize);
        auto out = render (p, rate, 1.5, nullptr, noteAt (rate, 48, 0.01, 1.0));
        const auto s = analyse (out);
        std::cout << "  " << juce::String (e + 1).paddedLeft (' ', 2) << " "
                  << juce::String (grind::engineInfo[(size_t) e].name).paddedRight (' ', 16)
                  << " peak " << dB (s.peak) << " dBFS  rms " << dB (s.rms) << " dBFS\n";
        check (s.finite, "non-finite output");
        check (s.peak > 0.02f, "engine too quiet");
        check (s.peak <= 1.0f, "engine clips");
        writeWav (out, rate, outDir.getChildFile ("engine_" + juce::String (e + 1).paddedLeft ('0', 2) + "_"
                                                  + juce::String (grind::engineInfo[(size_t) e].name).removeCharacters (" -") + ".wav"));
    }

    //== Every factory preset: a phrase (or its own sequencer pattern)
    std::cout << "Presets:\n";
    const auto& presets = grind::factoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
    {
        MidnightGrindProcessor p;
        FakePlayHead ph;
        p.setPlayHead (&ph);
        p.setCurrentProgram (i);
        p.prepareToPlay (rate, blockSize);
        const bool seq = (int) p.apvts.getRawParameterValue ("playMode")->load() == grind::modeSeq;
        const bool arpMode = (int) p.apvts.getRawParameterValue ("playMode")->load() == grind::modeArp;
        MidiFn phrase = [&] (juce::MidiBuffer& m, int pos, int n)
        {
            if (seq) return;
            if (arpMode) { chordAt (rate, { 60, 63, 67 }, 0.05, 3.5) (m, pos, n); return; }
            static const int notes[] = { 48, 51, 55, 60, 58, 55, 53, 51 };
            for (int k = 0; k < 8; ++k)
                noteAt (rate, notes[k], 0.05 + k * 0.45, 0.05 + k * 0.45 + 0.3) (m, pos, n);
        };
        const juce::String name (presets[(size_t) i].name);
        if (name == "DX Keys" || name == "Chord Stabs" || name == "Vox Choir" || name == "Grain Pad")
            phrase = chordAt (rate, { 48, 55, 60, 63, 67 }, 0.05, 3.0);

        auto out = render (p, rate, 4.0, &ph, phrase);
        const auto s = analyse (out);
        std::cout << "  " << juce::String (presets[(size_t) i].name).paddedRight (' ', 16)
                  << " peak " << dB (s.peak) << " dBFS  rms " << dB (s.rms) << " dBFS\n";
        check (s.finite, "non-finite output");
        check (s.rms > 0.005f, juce::String ("preset too quiet: ") + presets[(size_t) i].name);
        check (s.peak <= 1.0f, "preset clips");
        writeWav (out, rate, outDir.getChildFile ("preset_" + juce::String (i + 1).paddedLeft ('0', 2) + "_"
                                                  + juce::String (presets[(size_t) i].name).removeCharacters (" -") + ".wav"));
        p.setPlayHead (nullptr);
    }

    //== Pitch accuracy across host sample rates (Wave Generator sine, A4)
    std::cout << "Pitch (A4 = 440 Hz):\n";
    for (double r : { 44100.0, 48000.0, 88200.0, 96000.0 })
    {
        MidnightGrindProcessor p;
        setParam (p, "model", 22.0f);
        setParam (p, "morph", 0.0f);
        setParam (p, "harmonics", 0.0f);
        setParam (p, "cutoff", 20000.0f);
        setParam (p, "resonance", 0.0f);
        setParam (p, "vcfModAmt", 0.0f);
        setParam (p, "drive", 0.0f);
        setParam (p, "vcaMode", 1.0f);
        p.prepareToPlay (r, blockSize);
        auto out = render (p, r, 1.2, nullptr, noteAt (r, 69, 0.0, 1.2));
        const double hz = measurePitch (out, r, (int) (0.2 * r));
        std::cout << "  " << juce::String (r, 0) << " Hz host: " << juce::String (hz, 2) << " Hz\n";
        check (std::abs (hz - 440.0) < 1.0, "pitch off at " + juce::String (r, 0) + " Hz");
    }

    //== Sequencer follows the host, then free-runs with RUN
    {
        MidnightGrindProcessor p;
        FakePlayHead ph;
        p.setPlayHead (&ph);
        setParam (p, "playMode", (float) grind::modeSeq);
        setParam (p, "model", 21.0f);
        p.prepareToPlay (rate, blockSize);
        auto out = render (p, rate, 2.0, &ph);
        check (analyse (out).rms > 0.01f, "sequencer should play with the host transport");
        check (p.getPerformance().getPlayingStep() >= 0, "sequencer should report a step while playing");
        ph.playing = false;
        out = render (p, rate, 0.5, &ph);
        check (p.getPerformance().getPlayingStep() == -1, "sequencer should stop with the host");
        setParam (p, "seqRun", 1.0f);
        out = render (p, rate, 1.0, &ph);
        check (analyse (out).rms > 0.01f, "sequencer RUN should free-run");
        writeWav (out, rate, outDir.getChildFile ("sequencer_freerun.wav"));

        // Step record writes at the cursor
        setParam (p, "seqRun", 0.0f);
        setParam (p, "seqRec", 1.0f);
        render (p, rate, 0.1, &ph);
        p.getPerformance().setRecordCursor (0);
        render (p, rate, 0.2, &ph, noteAt (rate, 62, 0.0, 0.05));
        check (p.getPerformance().getStep (0, 0).note == 62, "step record should write the note");
        check (p.getPerformance().getRecordCursor() == 1, "step record should advance the cursor");
        p.setPlayHead (nullptr);
    }

    //== Arpeggiator with hold
    {
        MidnightGrindProcessor p;
        setParam (p, "playMode", (float) grind::modeArp);
        setParam (p, "arpHold", 1.0f);
        setParam (p, "arpOctaves", 2.0f);
        setParam (p, "arpPattern", 2.0f);
        p.prepareToPlay (rate, blockSize);
        auto out = render (p, rate, 3.0, nullptr, chordAt (rate, { 48, 52, 55 }, 0.0, 0.3));
        const auto s = analyse (out);
        juce::AudioBuffer<float> tail (2, (int) rate);
        for (int ch = 0; ch < 2; ++ch)
            tail.copyFrom (ch, 0, out, ch, out.getNumSamples() - (int) rate, (int) rate);
        check (analyse (tail).rms > 0.01f, "arp HOLD should keep playing after the keys are released");
        check (s.peak <= 1.0f, "arp clips");
        writeWav (out, rate, outDir.getChildFile ("arpeggiator_hold.wav"));
    }

    //== Poly Chain: 8 voices, chord
    {
        MidnightGrindProcessor p;
        setParam (p, "voices", 8.0f);
        setParam (p, "model", 5.0f);
        p.prepareToPlay (rate, blockSize);
        auto out = render (p, rate, 2.0, nullptr, chordAt (rate, { 48, 52, 55, 59, 62, 64, 67, 71 }, 0.0, 1.5));
        const auto s = analyse (out);
        check (s.finite && s.peak <= 1.0f && s.rms > 0.01f, "poly chord");
        writeWav (out, rate, outDir.getChildFile ("poly_chain_8.wav"));
    }

    //== Patch bay: LFO > cutoff, S&H > timbre, LFO square > EG trig (self-playing)
    {
        MidnightGrindProcessor p;
        p.setPatch (grind::dstCutoff, grind::srcLfoTri, 0.4f);
        p.setPatch (grind::dstTimbre, grind::srcSampleHold, 0.8f);
        p.setPatch (grind::dstEgTrig, grind::srcLfoSquare, 1.0f);
        p.setPatch (grind::dstOscTrig, grind::srcLfoSquare, 1.0f);
        setParam (p, "lfoRate", 0.55f);
        setParam (p, "model", 2.0f);
        setParam (p, "timbreAmt", 0.8f);
        p.prepareToPlay (rate, blockSize);
        auto out = render (p, rate, 3.0, nullptr);
        check (analyse (out).rms > 0.01f, "LFO square into EG TRIG should play without keys");
        writeWav (out, rate, outDir.getChildFile ("patchbay_selfplaying.wav"));
    }

    //== State round trip
    {
        MidnightGrindProcessor p;
        p.prepareToPlay (rate, blockSize);
        setParam (p, "model", 13.0f);
        setParam (p, "cutoff", 777.0f);
        p.setPatch (grind::dstMorph, grind::srcLfoTri, -0.35f);
        grind::Step st;
        st.note = 70; st.accent = true; st.slide = true; st.ratchet = 3;
        p.getPerformance().setStep (17, 9, st);
        p.getPerformance().setLength (17, 23);
        juce::MemoryBlock state;
        p.getStateInformation (state);

        MidnightGrindProcessor q;
        q.setStateInformation (state.getData(), (int) state.getSize());
        check ((int) q.apvts.getRawParameterValue ("model")->load() == 13, "param round trip (model)");
        check (std::abs (q.apvts.getRawParameterValue ("cutoff")->load() - 777.0f) < 1.0f, "param round trip (cutoff)");
        check (std::abs (q.getPatch (grind::dstMorph, grind::srcLfoTri) + 0.35f) < 0.001f, "patch round trip");
        const auto r = q.getPerformance().getStep (17, 9);
        check (r.note == 70 && r.accent && r.slide && r.ratchet == 3, "step round trip");
        check (q.getPerformance().getLength (17) == 23, "pattern length round trip");
    }

    //== CPU: 16-voice Poly Chain, one 4 s render
    {
        MidnightGrindProcessor p;
        setParam (p, "voices", 16.0f);
        setParam (p, "model", 8.0f);
        setParam (p, "vcaMode", 1.0f);
        p.prepareToPlay (44100.0, blockSize);
        std::vector<int> notes;
        for (int n = 0; n < 16; ++n) notes.push_back (40 + n * 2);
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        auto out = render (p, 44100.0, 4.0, nullptr, chordAt (44100.0, notes, 0.0, 4.0));
        const auto ms = juce::Time::getMillisecondCounterHiRes() - t0;
        const auto s = analyse (out);
        check (s.finite && s.peak <= 1.0f && s.rms > 0.02f, "16-voice chord");
        std::cout << "CPU: 16 voices (Hypersaw), 4 s of audio rendered in " << juce::String (ms, 0) << " ms ("
                  << juce::String (4000.0 / ms, 1) << "x realtime)\n";
    }

    //== Editor screenshots (each bottom tab)
    {
        MidnightGrindProcessor p;
        p.prepareToPlay (rate, blockSize);
        p.setPatch (grind::dstCutoff, grind::srcLfoTri, 0.5f);
        p.setPatch (grind::dstTimbre, grind::srcEnvelope, -0.6f);
        p.setPatch (grind::dstPitch, grind::srcKeyboard, 1.0f);
        std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
        auto* tabs = editor->getChildren().size() > 0 ? dynamic_cast<juce::TabbedButtonBar*> (
                         *std::find_if (editor->getChildren().begin(), editor->getChildren().end(),
                                        [] (auto* c) { return dynamic_cast<juce::TabbedButtonBar*> (c) != nullptr; }))
                                                      : nullptr;
        const char* names[] = { "editor.png", "editor_arp.png", "editor_patchbay.png", "editor_settings.png" };
        for (int t = 0; t < 4; ++t)
        {
            if (tabs != nullptr)
                tabs->setCurrentTabIndex (t);
            auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
            juce::PNGImageFormat png;
            auto file = outDir.getChildFile (names[t]);
            file.deleteFile();
            juce::FileOutputStream os (file);
            png.writeImageToStream (image, os);
        }
    }

    std::cout << (failures == 0 ? "ALL CHECKS PASSED\n" : "SOME CHECKS FAILED\n");
    return failures == 0 ? 0 : 1;
}
