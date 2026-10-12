#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <memory>
#include "DrumVoices.h"
#include "Params.h"

class MidnightEdgeProcessor : public juce::AudioProcessor
{
public:
    MidnightEdgeProcessor();
    ~MidnightEdgeProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Midnight Edge"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentKit; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Step sequencer data (shared with the editor)
    enum StepState : uint8_t { stepOff = 0, stepOn = 1, stepAccent = 2 };
    uint8_t getStep (int pattern, int voice, int step) const noexcept;
    void setStep (int pattern, int voice, int step, uint8_t value) noexcept;
    void clearPattern (int pattern) noexcept;
    int getCurrentStep() const noexcept { return currentStep.load (std::memory_order_relaxed); }
    int getCurrentPattern() const noexcept;

    // Trigger a voice from the UI (audition)
    void audition (int voice) noexcept { auditionMask.fetch_or (1u << voice); }
    // Per-voice activity, for UI lights (decays in the editor)
    uint32_t takeTriggeredMask() noexcept { return triggeredMask.exchange (0); }

    // Sample layers (message thread only)
    bool loadSample (int voice, const juce::File& file);
    void clearSample (int voice);
    juce::String getSampleName (int voice) const;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::String voiceParamId (int voice, const char* suffix);

private:
    //==============================================================================
    struct SampleData
    {
        juce::AudioBuffer<float> audio; // mono
        double sourceRate = 44100.0;
        juce::String path, name;
    };

    struct SampleSlot
    {
        juce::SpinLock lock;
        std::unique_ptr<SampleData> data;
    };

    struct SamplePlayer
    {
        double pos = 0, rate = 1;
        float gain = 0, chokeGain = 1, chokeCoef = 1;
        bool playing = false;
    };

    struct VoiceParamPtrs
    {
        std::atomic<float>* p[9] {};
        float get (int i) const noexcept { return p[i]->load (std::memory_order_relaxed); }
    };

    struct Trigger { int offset; int voice; float velocity; };

    void triggerVoice (int voice, float velocity);
    void runSequencer (int numSamples);
    void addTrigger (int offset, int voice, float velocity) noexcept;
    void applyKit (int index);
    void loadDefaultPatterns();
    juce::ValueTree patternsToTree() const;
    void patternsFromTree (const juce::ValueTree& tree);

    //==============================================================================
    std::array<std::unique_ptr<me::Voice>, me::numVoices> voices;
    std::array<VoiceParamPtrs, me::numVoices> voiceParams;
    std::array<SampleSlot, me::numVoices> sampleSlots;
    std::array<SamplePlayer, me::numVoices> samplePlayers;
    std::array<bool, me::numVoices> sampleLockHeld {}; // audio thread only
    std::array<std::atomic<uint8_t>, me::numPatterns * me::numVoices * me::numSteps> steps;

    std::atomic<float>* pVintage = nullptr;
    std::atomic<float>* pHeat = nullptr;
    std::atomic<float>* pPunch = nullptr;
    std::atomic<float>* pVerbSize = nullptr;
    std::atomic<float>* pVerbGate = nullptr;
    std::atomic<float>* pVerbTone = nullptr;
    std::atomic<float>* pVerbMix = nullptr;
    std::atomic<float>* pSwing = nullptr;
    std::atomic<float>* pAccent = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pSeqOn = nullptr;
    std::atomic<float>* pPattern = nullptr;
    std::atomic<float>* pLength = nullptr;
    std::atomic<float>* pRate = nullptr;

    std::vector<Trigger> triggers;
    juce::AudioBuffer<float> sendBuffer, gateBuffer;
    juce::Reverb reverb;
    me::OnePole sendHpL, sendHpR, vintageLpL, vintageLpR;

    double sr = 44100.0;
    float gateEnv = 0.0f;
    int gateCounter = 0;
    float compEnvDb = 0.0f;
    float vintagePhase = 0.0f, vintageHeldL = 0.0f, vintageHeldR = 0.0f;

    std::atomic<int> currentStep { -1 };
    std::atomic<uint32_t> auditionMask { 0 };
    std::atomic<uint32_t> triggeredMask { 0 };
    int currentKit = 0;

    juce::AudioFormatManager formatManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidnightEdgeProcessor)
};
