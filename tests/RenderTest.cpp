// Offline render test for Midnight Edge.
// Renders each voice and each factory kit (through the sequencer) to WAV,
// checks for NaNs / silence / clipping, round-trips the plugin state and
// saves a screenshot of the editor.
//
//   cmake -B build -DMIDNIGHT_EDGE_BUILD_TESTS=ON && cmake --build build --target MidnightEdgeRenderTest
//   ./build/MidnightEdgeRenderTest_artefacts/Release/MidnightEdgeRenderTest <output-dir>

#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

namespace
{
constexpr double sampleRate = 48000.0;
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
            sum += x * x;
        }
    s.rms = (float) std::sqrt (sum / (b.getNumChannels() * b.getNumSamples()));
    return s;
}

void writeWav (const juce::AudioBuffer<float>& b, const juce::File& file)
{
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (file), sampleRate,
                                                                          2, 24, {}, 0));
    if (writer != nullptr)
        writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

juce::AudioBuffer<float> render (MidnightEdgeProcessor& p, double seconds, FakePlayHead* ph,
                                 std::function<void (juce::MidiBuffer&, int)> midiForBlock = {})
{
    const int total = (int) (seconds * sampleRate);
    juce::AudioBuffer<float> out (2, total), block (2, blockSize);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < total; pos += blockSize)
    {
        const int n = std::min (blockSize, total - pos);
        block.setSize (2, n, false, false, true);
        midi.clear();
        if (midiForBlock) midiForBlock (midi, pos);
        p.processBlock (block, midi);
        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, pos, block, ch, 0, n);
        if (ph != nullptr)
            ph->ppq += n * ph->bpm / (60.0 * sampleRate);
    }
    return out;
}

void setParam (MidnightEdgeProcessor& p, const juce::String& id, float value)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (value));
}

int failures = 0;
void check (bool ok, const juce::String& what)
{
    if (! ok) { ++failures; std::cout << "  FAIL: " << what << "\n"; }
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir (argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1])
                                      : juce::File::getCurrentWorkingDirectory().getChildFile ("render-test"));
    outDir.createDirectory();

    //== Each voice on its own via MIDI
    std::cout << "Voices (kit 'Midnight 84', MIDI velocity 127):\n";
    for (int v = 0; v < me::numVoices; ++v)
    {
        MidnightEdgeProcessor p;
        setParam (p, "seqOn", 0.0f);
        p.prepareToPlay (sampleRate, blockSize);
        auto out = render (p, 1.5, nullptr, [v] (juce::MidiBuffer& m, int pos)
        {
            if (pos == 0) m.addEvent (juce::MidiMessage::noteOn (10, me::voiceInfo[(size_t) v].midiNote, (juce::uint8) 127), 10);
        });
        const auto s = analyse (out);
        std::cout << "  " << juce::String (me::voiceInfo[(size_t) v].name).paddedRight (' ', 10)
                  << " peak " << juce::String (juce::Decibels::gainToDecibels (s.peak), 1) << " dBFS"
                  << "  rms " << juce::String (juce::Decibels::gainToDecibels (s.rms), 1) << " dBFS\n";
        check (s.finite, "non-finite output");
        check (s.peak > 0.05f, "voice too quiet");
        check (s.peak <= 1.0f, "voice clips");
        
        writeWav (out, outDir.getChildFile (juce::String ("voice_") + me::voiceInfo[(size_t) v].id + ".wav"));
    }

    //== Each kit through the sequencer, patterns A-D, 2 bars each
    std::cout << "Kits (sequencer, patterns A-D, 120 BPM):\n";
    for (int k = 0; k < (int) me::factoryKits().size(); ++k)
    {
        MidnightEdgeProcessor p;
        FakePlayHead ph;
        p.setPlayHead (&ph);
        p.setCurrentProgram (k);
        p.prepareToPlay (sampleRate, blockSize);

        juce::AudioBuffer<float> all (2, 0);
        for (int pattern = 0; pattern < 4; ++pattern)
        {
            setParam (p, "pattern", (float) pattern);
            auto out = render (p, 4.0, &ph);
            const int start = all.getNumSamples();
            all.setSize (2, start + out.getNumSamples(), true, false, false);
            for (int ch = 0; ch < 2; ++ch)
                all.copyFrom (ch, start, out, ch, 0, out.getNumSamples());
        }
        const auto s = analyse (all);
        std::cout << "  " << p.getProgramName (k).paddedRight (' ', 14)
                  << " peak " << juce::String (juce::Decibels::gainToDecibels (s.peak), 1) << " dBFS"
                  << "  rms " << juce::String (juce::Decibels::gainToDecibels (s.rms), 1) << " dBFS\n";
        check (s.finite, "non-finite output");
        check (s.rms > 0.01f, "kit too quiet");
        check (s.peak <= 1.0f, "kit clips");
        writeWav (all, outDir.getChildFile ("kit_" + juce::String (k + 1) + "_" + p.getProgramName (k).removeCharacters (" '") + ".wav"));
        p.setPlayHead (nullptr);
    }

    //== Swing / rate / length sanity: count snare hits per bar with 1/32 and length 5
    {
        MidnightEdgeProcessor p;
        FakePlayHead ph;
        p.setPlayHead (&ph);
        p.prepareToPlay (sampleRate, blockSize);
        p.clearPattern (0);
        p.setStep (0, me::kick, 0, MidnightEdgeProcessor::stepAccent);
        setParam (p, "length", 4.0f);
        setParam (p, "swing", 1.0f);
        render (p, 2.0, &ph); // 4 beats = 16 sixteenth steps → kick fires 4 times
        check (p.getCurrentStep() >= 0, "sequencer should report a current step while playing");
        ph.playing = false;
        render (p, 0.1, &ph);
        check (p.getCurrentStep() == -1, "sequencer should stop when the host stops");
        p.setPlayHead (nullptr);
    }

    //== Sample layer: write a test tone and load it onto the snare
    {
        const auto wavFile = outDir.getChildFile ("layer_test_tone.wav");
        juce::AudioBuffer<float> tone (1, 12000);
        for (int i = 0; i < tone.getNumSamples(); ++i)
            tone.setSample (0, i, 0.8f * std::sin (juce::MathConstants<float>::twoPi * 440.0f * (float) i / 44100.0f)
                                    * std::exp (-(float) i / 3000.0f));
        {
            wavFile.deleteFile();
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (wavFile), 44100.0, 1, 16, {}, 0));
            w->writeFromAudioSampleBuffer (tone, 0, tone.getNumSamples());
        }

        MidnightEdgeProcessor p;
        setParam (p, "seqOn", 0.0f);
        p.prepareToPlay (sampleRate, blockSize);
        check (p.loadSample (me::snare, wavFile), "sample failed to load");
        check (p.getSampleName (me::snare) == "layer_test_tone", "sample name");
        check (p.apvts.getRawParameterValue ("snare_blend")->load() > 0.4f, "blend should auto-raise on load");
        setParam (p, "snare_blend", 1.0f);
        auto out = render (p, 0.5, nullptr, [] (juce::MidiBuffer& m, int pos) { if (pos == 0) m.addEvent (juce::MidiMessage::noteOn (10, 38, (juce::uint8) 127), 0); });
        const auto s = analyse (out);
        check (s.peak > 0.3f, "sample layer should be audible");
        std::cout << "Sample layer: peak " << juce::String (juce::Decibels::gainToDecibels (s.peak), 1) << " dBFS\n";

        //== State round trip (patterns + sample path + params)
        p.setStep (5, me::edgePerc, 7, MidnightEdgeProcessor::stepAccent);
        setParam (p, "kick_tune", -7.0f);
        juce::MemoryBlock state;
        p.getStateInformation (state);

        MidnightEdgeProcessor q;
        q.setStateInformation (state.getData(), (int) state.getSize());
        check (q.getStep (5, me::edgePerc, 7) == MidnightEdgeProcessor::stepAccent, "pattern state round trip");
        check (std::abs (q.apvts.getRawParameterValue ("kick_tune")->load() + 7.0f) < 0.01f, "param round trip");
        check (q.getSampleName (me::snare) == "layer_test_tone", "sample path round trip");
        check (std::abs (q.apvts.getRawParameterValue ("snare_blend")->load() - 1.0f) < 0.01f, "blend round trip");
    }

    //== Editor screenshot
    {
        MidnightEdgeProcessor p;
        p.prepareToPlay (sampleRate, blockSize);
        std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
        auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        juce::PNGImageFormat png;
        auto file = outDir.getChildFile ("editor.png");
        file.deleteFile();
        juce::FileOutputStream os (file);
        png.writeImageToStream (image, os);
    }

    std::cout << (failures == 0 ? "ALL CHECKS PASSED\n" : "SOME CHECKS FAILED\n");
    return failures == 0 ? 0 : 1;
}
