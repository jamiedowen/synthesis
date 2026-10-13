#pragma once

#include "PluginProcessor.h"

namespace grind::ui
{
namespace colours
{
    const juce::Colour background { 0xff0d0f14 };
    const juce::Colour panel      { 0xff171a22 };
    const juce::Colour panelLight { 0xff20242f };
    const juce::Colour edge       { 0xff2e3442 };
    const juce::Colour text       { 0xffe6e8ee };
    const juce::Colour dimText    { 0xff8a91a3 };
    const juce::Colour amber      { 0xffffa21f };
    const juce::Colour orange     { 0xffff5a1f };
    const juce::Colour teal       { 0xff27d3c3 };
    const juce::Colour red        { 0xffff3344 };
    const juce::Colour lcd        { 0xff0a1512 };
    const juce::Colour lcdText    { 0xff6dffcf };
}

class GrindLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GrindLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getPopupMenuFont() override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

//==============================================================================
class Knob : public juce::Component
{
public:
    enum Size { small, medium, large };
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& label,
          juce::Colour colour, Size size = medium);
    void resized() override;
    void paint (juce::Graphics&) override;
    void setSubLabel (const juce::String& s) { if (s != subLabel) { subLabel = s; repaint(); } }

    juce::Slider slider;

private:
    juce::String label, subLabel;
    Size size;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
// Hardware-style switch: a row (or column) of labelled positions bound to a
// choice or bool parameter.
class Switch : public juce::Component, public juce::SettableTooltipClient
{
public:
    Switch (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& title,
            const juce::StringArray& labels, juce::Colour colour, bool vertical = true);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::String title;
    juce::StringArray labels;
    juce::Colour colour;
    bool vertical;
    int current = 0;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

//==============================================================================
// The 24 model buttons in their three banks
class ModelSelector : public juce::Component
{
public:
    explicit ModelSelector (juce::AudioProcessorValueTreeState&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void resized() override;
    int getModel() const noexcept { return current; }

private:
    juce::Rectangle<int> buttonBounds (int engine) const;
    int current = 0;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

//==============================================================================
class Display : public juce::Component
{
public:
    explicit Display (MidnightGrindProcessor& p) : processor (p) {}
    void paint (juce::Graphics&) override;
    void tick();

private:
    MidnightGrindProcessor& processor;
    int model = -1, voices = 0;
    float env = 0;
};

//==============================================================================
class SequencerView : public juce::Component
{
public:
    explicit SequencerView (MidnightGrindProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void tick();

private:
    int stepAt (juce::Point<int>) const;
    juce::Rectangle<int> stepBounds (int step) const;
    void showStepMenu (int step);
    int pattern() const { return processor.getCurrentPattern(); }

    MidnightGrindProcessor& processor;
    juce::ComboBox patternBox, rateBox;
    juce::Slider lengthSlider;
    juce::TextButton runButton { "RUN" }, recButton { "REC" }, clearButton { "CLEAR" }, copyButton { "COPY" },
                     pasteButton { "PASTE" }, randomButton { "RANDOM" };
    Knob gate, swing, tempo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> patternAttach, rateAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> runAttach, recAttach;

    juce::Rectangle<int> gridArea;
    int dragStep = -1, dragStartNote = 0, dragStartY = 0;
    bool dragged = false;
    int playing = -1, cursor = -1, shownPattern = -1;
    std::array<uint32_t, numSteps> clipboard {};
    int clipboardLength = 0;
};

//==============================================================================
class ArpView : public juce::Component
{
public:
    explicit ArpView (MidnightGrindProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void tick();

private:
    juce::Rectangle<int> patternBounds (int i) const;
    juce::Rectangle<int> octaveBounds (int i) const;

    MidnightGrindProcessor& processor;
    juce::ComboBox rateBox;
    juce::TextButton holdButton { "HOLD" };
    Knob gate, swing, tempo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> holdAttach;
    juce::Rectangle<int> patternArea, octaveArea;
    int shownPattern = -1, shownOctaves = -1, playing = -1;
};

//==============================================================================
class PatchBayView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit PatchBayView (MidnightGrindProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void tick();

private:
    bool cellAt (juce::Point<int>, int& destination, int& source) const;
    juce::Rectangle<float> cellBounds (int destination, int source) const;

    MidnightGrindProcessor& processor;
    juce::TextButton clearButton { "CLEAR ALL" };
    juce::Rectangle<int> grid;
    int dragDest = -1, dragSource = -1, hoverDest = -1, hoverSource = -1;
    float dragStartAmount = 0.0f;
    bool dragged = false;
    int shownCount = -1;
};

//==============================================================================
class SettingsView : public juce::Component
{
public:
    explicit SettingsView (MidnightGrindProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MidnightGrindProcessor& processor;
    juce::Slider voicesSlider, bendSlider;
    juce::ComboBox priorityBox, channelBox, bankBox;
    Knob velocity, fine;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> voicesAttach, bendAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> priorityAttach, channelAttach, bankAttach;
};

//==============================================================================
class MidnightGrindEditor : public juce::AudioProcessorEditor,
                            private juce::Timer,
                            private juce::MidiKeyboardState::Listener
{
public:
    explicit MidnightGrindEditor (MidnightGrindProcessor&);
    ~MidnightGrindEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void handleNoteOn (juce::MidiKeyboardState*, int, int note, float) override { processor.playNoteFromEditor (note, true); }
    void handleNoteOff (juce::MidiKeyboardState*, int, int note, float) override { processor.playNoteFromEditor (note, false); }
    void showTab (int index);
    void drawSection (juce::Graphics&, juce::Rectangle<int>, const juce::String& title, juce::Colour) const;

    MidnightGrindProcessor& processor;
    GrindLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    // Top bar
    juce::ComboBox presetBox;
    Switch playMode;

    // Oscillator
    ModelSelector models;
    Display display;
    Knob frequency, harmonics, timbre, morph, fmAmt, timbreAmt, morphAmt, oscDecay, lpgColour, mix;
    juce::TextButton lpgButton { "LPG" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lpgAttach;

    // VCO mod
    Knob vcoMod;
    Switch vcoModSrc, vcoModDest;

    // VCF
    Knob cutoff, resonance, vcfMod, keyTrack, drive;
    Switch vcfModSrc, vcfModPol, vcfMode;

    // EG / VCA
    Knob attack, decay;
    Switch sustain, vcaMode;

    // LFO / glide / out
    Knob lfoRate, glide, volume;
    Switch lfoWave;

    // Bottom
    struct Tabs : juce::TabbedButtonBar
    {
        Tabs() : TabbedButtonBar (TabsAtTop) {}
        std::function<void (int)> onTabChanged;
        void currentTabChanged (int index, const juce::String&) override { if (onTabChanged) onTabChanged (index); }
    } tabs;
    SequencerView sequencer;
    ArpView arp;
    PatchBayView patchBay;
    SettingsView settings;

    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboard { keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard };

    juce::Rectangle<int> oscArea, vcoArea, vcfArea, egArea, lfoArea, bottomArea, topBar;
    float lfoLed = 0.0f, meter = 0.0f;
    int shownProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidnightGrindEditor)
};

} // namespace grind::ui
