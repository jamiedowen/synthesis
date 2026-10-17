#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "Engine.h"
#include "Presets.h"

class CJAutoworxProcessor : public juce::AudioProcessor
{
public:
    CJAutoworxProcessor();
    ~CJAutoworxProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CJ Autoworx RevHead"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // UI hooks (message thread)
    void setRevButton (bool down) noexcept { revButton.store (down); }
    void startPassBy() noexcept { passByRequest.store (true); }
    bool isPassingBy() const noexcept { return passByActive.load(); }
    void startQuiz (int presetIndex) noexcept { quizRequest.store (presetIndex); }
    bool isQuizPlaying() const noexcept { return quizPlaying.load(); }

    // Readouts for the dashboard
    float getRpm() const noexcept { return displayRpm.load (std::memory_order_relaxed); }
    float getRedline() const noexcept { return displayRedline.load (std::memory_order_relaxed); }
    float getBoost() const noexcept { return displayBoost.load (std::memory_order_relaxed); }
    float getThrottle() const noexcept { return displayThrottle.load (std::memory_order_relaxed); }
    int getGear() const noexcept { return displayGear.load (std::memory_order_relaxed); }
    uint32_t takeFiredMask() noexcept { return firedMask.exchange (0); }

    juce::MidiKeyboardState keyboardState;
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    //==============================================================================
    struct MusicalVoice
    {
        cj::EngineVoice engine;
        juce::ADSR env;
        int note = -1;
        float velocity = 0, freq = 110, targetFreq = 110, revOffset = 0;
        bool held = false;
        uint64_t age = 0;
    };

    void handleMidi (const juce::MidiMessage&);
    void noteOn (int note, float velocity);
    void noteOff (int note);
    void allNotesOff();
    void render (float* outL, float* outR, int start, int num);
    void updateBlockSettings();
    void applyPreset (int index);
    float engineModeThrottle() const noexcept;

    //==============================================================================
    std::array<MusicalVoice, 8> voices;
    cj::EngineVoice mainEngine, quizEngine;
    cj::Crank mainCrank, quizCrank;
    cj::EngineSettings settings, quizSettings;
    juce::ADSR::Parameters adsr;

    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pLayout = nullptr;
    std::atomic<float>* pIdle = nullptr;
    std::atomic<float>* pRedline = nullptr;
    std::atomic<float>* pFlywheel = nullptr;
    std::atomic<float>* pThrottle = nullptr;
    std::atomic<float>* pGearbox = nullptr;
    std::atomic<float>* pIgnition = nullptr;
    std::atomic<float>* pPulse = nullptr;
    std::atomic<float>* pRough = nullptr;
    std::atomic<float>* pRasp = nullptr;
    std::atomic<float>* pMech = nullptr;
    std::atomic<float>* pPipe = nullptr;
    std::atomic<float>* pReso = nullptr;
    std::atomic<float>* pMuffler = nullptr;
    std::atomic<float>* pCrackle = nullptr;
    std::atomic<float>* pWidth = nullptr;
    std::atomic<float>* pIntake = nullptr;
    std::atomic<float>* pTurbo = nullptr;
    std::atomic<float>* pBlower = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pGlide = nullptr;
    std::atomic<float>* pOctave = nullptr;
    std::atomic<float>* pRevAmt = nullptr;
    std::atomic<float>* pRevTime = nullptr;
    std::atomic<float>* pMono = nullptr;
    std::atomic<float>* pGarage = nullptr;
    std::atomic<float>* pOutput = nullptr;

    double sr = 44100.0;
    int mode = cj::engineMode;
    int lastLayout = -1;
    uint64_t noteCounter = 0;

    // Engine mode key state: any held key is the accelerator pedal
    std::array<bool, 128> keysDown {};
    int keysHeld = 0;
    float keyThrottle = 0.0f, modWheel = 0.0f, pitchBend = 0.0f;
    float throttleSmoothed = 0.0f;

    // Starter motor / shut down
    bool engineRunning = false;
    int starterSamples = 0;
    float starterPhase = 0.0f, engineGain = 0.0f;

    // Musical mode
    std::vector<int> monoStack;
    float lastFreq = 0.0f;
    float glideCoef = 0.0f, revCoef = 0.0f;

    // Pass-by: Doppler pitch, distance and pan
    float passT = -1.0f, passPitch = 1.0f, passGain = 1.0f, passPan = 0.0f;
    cj::OnePole passLpL, passLpR;

    // Quiz
    int quizPreset = -1;
    float quizT = -1.0f;

    juce::Reverb garage;
    juce::AudioBuffer<float> wetBuffer;

    std::atomic<bool> revButton { false }, passByRequest { false }, passByActive { false }, quizPlaying { false };
    std::atomic<int> quizRequest { -1 };
    std::atomic<float> displayRpm { 0 }, displayRedline { 6500 }, displayBoost { 0 }, displayThrottle { 0 };
    std::atomic<int> displayGear { -1 };
    std::atomic<uint32_t> firedMask { 0 };
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CJAutoworxProcessor)
};
