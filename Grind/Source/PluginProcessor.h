#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <vector>

#include "EngineTable.h"
#include "PatchBay.h"
#include "Performance.h"
#include "dsp/Synth.h"

class MidnightGrindProcessor : public juce::AudioProcessor
{
public:
    MidnightGrindProcessor();
    ~MidnightGrindProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Midnight Grind"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Patch cables: which output feeds each input (-1 = unpatched), shared with the editor
    int getCable (int input) const noexcept;
    void setCable (int input, int output) noexcept;
    void clearCables() noexcept;
    int countCables() const noexcept;
    uint32_t getCableVersion() const noexcept { return cableVersion.load (std::memory_order_relaxed); }

    grind::Performance& getPerformance() noexcept { return performance; }

    // Play a note from the editor (on-screen keys / audition)
    void playNoteFromEditor (int note, bool on) noexcept;

    // Live readouts for the editor
    float getLfoLevel() const noexcept { return lfoLevel.load (std::memory_order_relaxed); }
    float getEnvelopeLevel() const noexcept { return envLevel.load (std::memory_order_relaxed); }
    float getOutputLevel() const noexcept { return outLevel.load (std::memory_order_relaxed); }
    int getCurrentPattern() const noexcept;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::StringArray patternNames();

private:
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        void setLowPass (double cutoff, double sampleRate, double q);
        float process (float x) noexcept
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };

    void updateParams();
    grind::Performance::Settings performanceSettings() const;
    void applyEvent (const grind::NoteEvent&);
    float pullInternalSample();
    void applyProgram (int index);
    juce::ValueTree extraStateToTree() const;
    void extraStateFromTree (const juce::ValueTree&);

    grind::dsp::Synth synth;
    grind::dsp::SynthParams synthParams;
    grind::Performance performance;
    std::array<std::atomic<int>, grind::numInputs> cables;
    std::atomic<uint32_t> cableVersion { 0 };
    float lastPlayStop = 0.0f, lastReset = 0.0f;

    std::vector<grind::NoteEvent> events;
    std::vector<float> mono;

    // 48 kHz engine -> host rate
    double hostRate = 48000.0, ratio = 1.0, phase = 0.0;
    float history[4] {};
    float internalBlock[grind::dsp::blockSize] {};
    int internalPos = grind::dsp::blockSize;
    Biquad antiAlias[2];
    bool useAntiAlias = false;
    float dcState = 0.0f, dcPrev = 0.0f;

    // Editor note input (lock-free)
    std::atomic<uint32_t> uiNoteOn[4], uiNoteOff[4];

    std::atomic<float> lfoLevel { 0 }, envLevel { 0 }, outLevel { 0 };
    int currentProgram = 0;

    // Cached parameter pointers
    struct P
    {
        std::atomic<float>* model; std::atomic<float>* frequency; std::atomic<float>* fine;
        std::atomic<float>* harmonics; std::atomic<float>* timbre; std::atomic<float>* morph;
        std::atomic<float>* fmAmt; std::atomic<float>* timbreAmt; std::atomic<float>* morphAmt;
        std::atomic<float>* oscDecay; std::atomic<float>* lpgColour; std::atomic<float>* lpgOn;
        std::atomic<float>* mix; std::atomic<float>* dx7Bank;
        std::atomic<float>* vcoModAmt; std::atomic<float>* vcoModSrc; std::atomic<float>* vcoModDest;
        std::atomic<float>* cutoff; std::atomic<float>* resonance; std::atomic<float>* vcfModAmt;
        std::atomic<float>* vcfModSrc; std::atomic<float>* vcfModPol; std::atomic<float>* vcfMode;
        std::atomic<float>* keyTrack; std::atomic<float>* drive;
        std::atomic<float>* vcaMode; std::atomic<float>* attack; std::atomic<float>* decay; std::atomic<float>* assignSrc;
        std::atomic<float>* sustain; std::atomic<float>* velocity;
        std::atomic<float>* lfoRate; std::atomic<float>* lfoWave; std::atomic<float>* glide;
        std::atomic<float>* volume;
        std::atomic<float>* playMode; std::atomic<float>* tempo; std::atomic<float>* seqRun;
        std::atomic<float>* seqRec; std::atomic<float>* pattern; std::atomic<float>* seqRate;
        std::atomic<float>* seqGate; std::atomic<float>* swing;
        std::atomic<float>* arpPattern; std::atomic<float>* arpOctaves; std::atomic<float>* arpRate;
        std::atomic<float>* arpGate; std::atomic<float>* arpHold;
        std::atomic<float>* voices; std::atomic<float>* priority; std::atomic<float>* midiChannel;
        std::atomic<float>* bendRange;
    } p {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidnightGrindProcessor)
};
