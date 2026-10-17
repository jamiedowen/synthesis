// Offline render test for CJ Autoworx RevHead.
// Drives every engine preset through a start-up, blip and full pull, plays the
// musical presets, checks pitch tracking, the rev limiter, the gearbox, pass-by
// and the quiz, round-trips the state and saves screenshots of the editor.
//
//   cmake -B build -DCJ_AUTOWORX_BUILD_TESTS=ON && cmake --build build --target CJAutoworxRenderTest
//   ./build/CJAutoworx/CJAutoworxRenderTest_artefacts/Release/CJAutoworxRenderTest <output-dir>

#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 256;

struct Stats { float peak = 0, rms = 0; bool finite = true; };

Stats analyse (const juce::AudioBuffer<float>& b, int start = 0)
{
    Stats s;
    double sum = 0;
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = start; i < b.getNumSamples(); ++i)
        {
            const float x = b.getSample (ch, i);
            if (! std::isfinite (x)) s.finite = false;
            s.peak = std::max (s.peak, std::abs (x));
            sum += x * x;
        }
    s.rms = (float) std::sqrt (sum / (b.getNumChannels() * std::max (1, b.getNumSamples() - start)));
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

struct Note { double start, length; int note; float velocity; };

// Render with a list of notes. onBlock runs after each block (for readouts).
juce::AudioBuffer<float> render (CJAutoworxProcessor& p, double seconds, const std::vector<Note>& notes,
                                 std::function<void (double)> onBlock = {})
{
    const int total = (int) (seconds * sampleRate);
    juce::AudioBuffer<float> out (2, total), block (2, blockSize);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < total; pos += blockSize)
    {
        const int n = std::min (blockSize, total - pos);
        block.setSize (2, n, false, false, true);
        midi.clear();
        for (const auto& note : notes)
        {
            const int on = (int) (note.start * sampleRate), off = (int) ((note.start + note.length) * sampleRate);
            if (on >= pos && on < pos + n)
                midi.addEvent (juce::MidiMessage::noteOn (1, note.note, note.velocity), on - pos);
            if (off >= pos && off < pos + n)
                midi.addEvent (juce::MidiMessage::noteOff (1, note.note), off - pos);
        }
        p.processBlock (block, midi);
        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, pos, block, ch, 0, n);
        if (onBlock)
            onBlock ((double) (pos + n) / sampleRate);
    }
    return out;
}

void setParam (CJAutoworxProcessor& p, const juce::String& id, float value)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (value));
}

// Autocorrelation pitch estimate of the mono mix, in Hz
float estimatePitch (const juce::AudioBuffer<float>& b, int start, int length, float minHz, float maxHz)
{
    std::vector<float> x ((size_t) length);
    for (int i = 0; i < length; ++i)
        x[(size_t) i] = b.getSample (0, start + i) + b.getSample (1, start + i);

    const int minLag = (int) (sampleRate / maxHz), maxLag = (int) (sampleRate / minHz);
    int bestLag = minLag;
    double best = -1.0e30;
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double sum = 0;
        for (int i = 0; i + lag < length; ++i)
            sum += x[(size_t) i] * x[(size_t) (i + lag)];
        sum /= (double) (length - lag);
        if (sum > best)
        {
            best = sum;
            bestLag = lag;
        }
    }
    return (float) (sampleRate / bestLag);
}

void snapshot (juce::Component& c, const juce::File& file)
{
    auto image = c.createComponentSnapshot (c.getLocalBounds(), true, 1.0f);
    juce::PNGImageFormat png;
    file.deleteFile();
    juce::FileOutputStream os (file);
    png.writeImageToStream (image, os);
}

int failures = 0;
void check (bool ok, const juce::String& what)
{
    if (! ok) { ++failures; std::cout << "  FAIL: " << what << "\n"; }
}

juce::String dB (float x) { return juce::String (juce::Decibels::gainToDecibels (x), 1) + " dBFS"; }
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir (argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1])
                                      : juce::File::getCurrentWorkingDirectory().getChildFile ("render-test"));
    outDir.createDirectory();
    const auto& presets = cj::factoryPresets();

    //== Engine presets: start-up, idle, a blip, a full pull into the limiter / gears, lift off
    std::cout << "Engine presets (ENGINE mode: start, blip, full pull, lift off):\n";
    const std::vector<Note> drive { { 1.6, 0.35, 84, 1.0f }, { 3.0, 2.6, 84, 1.0f } };
    for (int i = 0; i < cj::numEnginePresets; ++i)
    {
        CJAutoworxProcessor p;
        p.setCurrentProgram (i);
        p.prepareToPlay (sampleRate, blockSize);
        float maxRpm = 0.0f;
        int maxGear = -1;
        auto out = render (p, 8.0, drive, [&] (double) { maxRpm = std::max (maxRpm, p.getRpm()); maxGear = std::max (maxGear, p.getGear()); });
        const auto s = analyse (out);
        const auto idle = analyse (out, (int) (1.0 * sampleRate));
        std::cout << "  " << juce::String (presets[(size_t) i].name).paddedRight (' ', 24)
                  << " peak " << dB (s.peak) << "  rms " << dB (s.rms)
                  << "  max " << juce::roundToInt (maxRpm) << " rpm"
                  << (maxGear >= 0 ? "  top gear " + juce::String (maxGear + 1) : juce::String()) << "\n";
        check (s.finite, "non-finite output");
        check (s.peak <= 1.0f, "output above 0 dBFS");
        check (idle.rms > 0.01f, "engine too quiet");
        check (maxRpm <= presets[(size_t) i].redline * 1.06f, "rev limiter should hold the redline");
        check (maxRpm >= presets[(size_t) i].redline * 0.85f, "full throttle should reach the redline");
        writeWav (out, outDir.getChildFile ("engine_" + juce::String (i + 1).paddedLeft ('0', 2) + "_"
                                            + juce::String (presets[(size_t) i].name).removeCharacters (" -'") + ".wav"));
    }

    //== Gearbox: hold the throttle and it should change up
    {
        CJAutoworxProcessor p;
        p.setCurrentProgram (0);
        setParam (p, "gearbox", 1.0f);
        p.prepareToPlay (sampleRate, blockSize);
        int maxGear = -1;
        render (p, 9.0, { { 1.2, 7.5, 84, 1.0f } }, [&] (double) { maxGear = std::max (maxGear, p.getGear()); });
        std::cout << "Gearbox: reached gear " << maxGear + 1 << "\n";
        check (maxGear >= 2, "auto shift should reach at least third gear");
    }

    //== Ignition off: the engine stops and goes quiet
    {
        CJAutoworxProcessor p;
        p.prepareToPlay (sampleRate, blockSize);
        render (p, 2.0, {});
        setParam (p, "ignition", 0.0f);
        render (p, 3.0, {});
        const auto tail = analyse (render (p, 0.5, {}));
        std::cout << "Ignition off: tail rms " << dB (tail.rms) << ", rpm " << juce::roundToInt (p.getRpm()) << "\n";
        check (tail.rms < 0.003f, "engine should be quiet after the ignition is switched off");
    }

    //== Musical presets: a bass line and some chords
    std::cout << "Musical presets:\n";
    const std::vector<Note> tune {
        { 0.0, 0.45, 36, 0.9f }, { 0.5, 0.45, 43, 0.7f }, { 1.0, 0.45, 48, 0.8f }, { 1.5, 0.45, 46, 0.7f },
        { 2.0, 1.4, 48, 0.8f }, { 2.0, 1.4, 52, 0.8f }, { 2.0, 1.4, 55, 0.8f },
        { 3.5, 1.4, 45, 0.8f }, { 3.5, 1.4, 48, 0.8f }, { 3.5, 1.4, 52, 0.8f }, { 3.5, 1.4, 57, 1.0f },
    };
    for (int i = cj::numEnginePresets; i < (int) presets.size(); ++i)
    {
        CJAutoworxProcessor p;
        p.setCurrentProgram (i);
        p.prepareToPlay (sampleRate, blockSize);
        auto out = render (p, 7.0, tune);
        const auto s = analyse (out);
        std::cout << "  " << juce::String (presets[(size_t) i].name).paddedRight (' ', 24)
                  << " peak " << dB (s.peak) << "  rms " << dB (s.rms) << "\n";
        check (s.finite, "non-finite output");
        check (s.peak <= 1.0f, "output above 0 dBFS");
        check (s.rms > 0.01f, "preset too quiet");
        writeWav (out, outDir.getChildFile ("musical_" + juce::String (i + 1).paddedLeft ('0', 2) + "_"
                                            + juce::String (presets[(size_t) i].name).removeCharacters (" -'") + ".wav"));
    }

    //== Pitch tracking: A3 on an even-firing engine should sound at 220 Hz
    for (const int layout : { (int) cj::inline4, (int) cj::v12 })
    {
        CJAutoworxProcessor p;
        p.setCurrentProgram (cj::numEnginePresets); // Burble Bass
        setParam (p, "layout", (float) layout);
        setParam (p, "rough", 0.0f);
        setParam (p, "revAmt", 0.0f);
        setParam (p, "glide", 0.0f);
        p.prepareToPlay (sampleRate, blockSize);
        auto out = render (p, 1.5, { { 0.0, 1.4, 57, 0.9f } });
        const float hz = estimatePitch (out, (int) (0.5 * sampleRate), 8192, 80.0f, 500.0f);
        std::cout << "Pitch (" << cj::layouts()[(size_t) layout].name << ", A3): " << juce::String (hz, 1) << " Hz\n";
        check (std::abs (hz - 220.0f) < 4.0f, "A3 should sound at 220 Hz");
    }

    //== Pass-by
    {
        CJAutoworxProcessor p;
        p.setCurrentProgram (2);
        p.prepareToPlay (sampleRate, blockSize);
        render (p, 1.5, {});
        p.startPassBy();
        bool seen = false;
        auto out = render (p, 6.5, {}, [&] (double) { seen = seen || p.isPassingBy(); });
        const auto s = analyse (out);
        std::cout << "Pass-by: peak " << dB (s.peak) << "\n";
        check (seen && ! p.isPassingBy(), "pass-by should run and finish");
        check (s.finite && s.peak <= 1.0f, "pass-by output");
        writeWav (out, outDir.getChildFile ("passby.wav"));
    }

    //== Quiz round
    {
        CJAutoworxProcessor p;
        p.prepareToPlay (sampleRate, blockSize);
        render (p, 1.0, {});
        p.startQuiz (6);
        bool seen = false;
        auto out = render (p, 8.5, {}, [&] (double) { seen = seen || p.isQuizPlaying(); });
        const auto s = analyse (out);
        std::cout << "Quiz (Rotary): peak " << dB (s.peak) << "  rms " << dB (s.rms) << "\n";
        check (seen && ! p.isQuizPlaying(), "quiz round should play and finish");
        check (s.rms > 0.01f && s.peak <= 1.0f, "quiz output");
        writeWav (out, outDir.getChildFile ("quiz_round.wav"));
    }

    //== State round trip
    {
        CJAutoworxProcessor p;
        p.setCurrentProgram (15);
        setParam (p, "pipe", 2.4f);
        juce::MemoryBlock state;
        p.getStateInformation (state);

        CJAutoworxProcessor q;
        q.setStateInformation (state.getData(), (int) state.getSize());
        check (q.getCurrentProgram() == 15, "preset round trip");
        check (std::abs (q.apvts.getRawParameterValue ("pipe")->load() - 2.4f) < 0.01f, "param round trip");
        check ((int) q.apvts.getRawParameterValue ("mode")->load() == cj::musicalMode, "mode round trip");
    }

    //== Editor screenshots: on the limiter, and the quiz
    {
        CJAutoworxProcessor p;
        p.setCurrentProgram (0);
        p.prepareToPlay (sampleRate, blockSize);
        render (p, 4.0, { { 1.0, 3.0, 66, 1.0f } });

        std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
        auto* e = dynamic_cast<CJAutoworxEditor*> (editor.get());
        for (int i = 0; i < 8; ++i)
            e->refresh();
        render (p, 0.03, { { 0.0, 0.03, 66, 1.0f } }); // light up the cylinders
        e->refresh();
        snapshot (*editor, outDir.getChildFile ("editor.png"));

        e->showQuiz();
        e->refresh();
        snapshot (*editor, outDir.getChildFile ("editor_quiz.png"));
    }

    std::cout << (failures == 0 ? "ALL CHECKS PASSED\n" : "SOME CHECKS FAILED\n");
    return failures == 0 ? 0 : 1;
}
