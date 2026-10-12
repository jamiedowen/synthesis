#pragma once

#include "PluginProcessor.h"

namespace me::ui
{
namespace colours
{
    const juce::Colour background   { 0xff0b0118 };
    const juce::Colour panel        { 0xff170932 };
    const juce::Colour panelEdge    { 0xff3a1d6e };
    const juce::Colour text         { 0xffe8d9ff };
    const juce::Colour dimText      { 0xff8f7bb5 };
    const juce::Colour pink         { 0xffff2a6d };
    const juce::Colour cyan         { 0xff05d9e8 };
    const juce::Colour orange       { 0xffff6c11 };
    const juce::Colour yellow       { 0xfff9c80e };
}

class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
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
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& label,
          juce::Colour colour, juce::Component* popupParent);
    void resized() override;
    void paint (juce::Graphics&) override;

    juce::Slider slider;

private:
    juce::String label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
// TextButton that remembers whether the last click was a right-click
class SampleButton : public juce::TextButton
{
public:
    using TextButton::clicked;
    bool wasPopupClick = false;
    void clicked (const juce::ModifierKeys& mods) override { wasPopupClick = mods.isPopupMenu(); }
};

//==============================================================================
class VoiceStrip : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    VoiceStrip (MidnightEdgeProcessor&, int voice, juce::Component* popupParent);
    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }
    void filesDropped (const juce::StringArray&, int, int) override;

    void flash() { activity = 1.0f; }
    void tick();

private:
    void chooseSample();

    MidnightEdgeProcessor& processor;
    int voice;
    juce::Colour colour;
    juce::TextButton nameButton;
    SampleButton loadButton;
    juce::OwnedArray<Knob> knobs;
    std::unique_ptr<juce::FileChooser> chooser;
    float activity = 0.0f;
    bool dragOver = false;
    juce::String shownSample { "\x01" };
};

//==============================================================================
class StepGrid : public juce::Component
{
public:
    explicit StepGrid (MidnightEdgeProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

    static constexpr int labelWidth = 104;

private:
    bool cellAt (juce::Point<int>, int& voice, int& step) const;
    juce::Rectangle<float> cellBounds (int voice, int step) const;

    MidnightEdgeProcessor& processor;
    uint8_t dragValue = 0;
    int lastVoice = -1, lastStep = -1;
};

} // namespace me::ui

//==============================================================================
class MidnightEdgeEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MidnightEdgeEditor (MidnightEdgeProcessor&);
    ~MidnightEdgeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int designWidth = 1200;
    static constexpr int designHeight = 720;

private:
    class Content : public juce::Component
    {
    public:
        void paint (juce::Graphics&) override;
    };

    void timerCallback() override;
    void layout();
    void selectPattern (int index);

    MidnightEdgeProcessor& processor;
    me::ui::NeonLookAndFeel lnf;
    Content content;

    juce::ComboBox kitBox;
    juce::OwnedArray<me::ui::Knob> masterKnobs;
    juce::OwnedArray<me::ui::VoiceStrip> strips;

    juce::TextButton seqButton { "SEQ" };
    juce::OwnedArray<juce::TextButton> patternButtons;
    juce::TextButton copyButton { "COPY" }, pasteButton { "PASTE" }, clearButton { "CLEAR" };
    juce::ComboBox lengthBox, rateBox;
    juce::OwnedArray<me::ui::Knob> seqKnobs;
    me::ui::StepGrid grid;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lengthAttachment, rateAttachment;

    std::array<uint8_t, me::numVoices * me::numSteps> clipboard {};
    bool hasClipboard = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidnightEdgeEditor)
};
