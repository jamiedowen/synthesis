#pragma once

#include "PluginProcessor.h"

namespace grind::ui
{
inline juce::Font uiFont (float height, bool bold = true)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

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

// On phones and tablets every popup (menus, combo box lists) is drawn inside the app's own
// window: separate native popup windows are fragile on Android.
inline juce::PopupMenu::Options inAppMenuOptions (juce::PopupMenu::Options options, juce::Component* anchor)
{
   #if JUCE_ANDROID || JUCE_IOS
    if (anchor != nullptr)
        if (auto* top = anchor->getTopLevelComponent())
            return options.withParentComponent (top);
   #else
    juce::ignoreUnused (anchor);
   #endif
    return options;
}

class GrindLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GrindLookAndFeel();
    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu (juce::ComboBox& box, juce::Label& label) override
    {
        return inAppMenuOptions (LookAndFeel_V4::getOptionsForComboBoxPopupMenu (box, label), &box);
    }
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
    void parentHierarchyChanged() override { slider.setPopupDisplayEnabled (true, true, getTopLevelComponent()); }

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
// Step edit menu (gate / accent / slide / tie / ratchet / note), shared by the
// sequencer grid and the panel's STEP pads
void showStepMenu (MidnightGrindProcessor&, int pattern, int step, juce::Component& target,
                   juce::Rectangle<int> screenArea, std::function<void()> onDone);

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
class SettingsView : public juce::Component
{
public:
    explicit SettingsView (MidnightGrindProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MidnightGrindProcessor& processor;
    juce::Slider voicesSlider, bendSlider;
    juce::ComboBox priorityBox, channelBox, bankBox, assignBox;
    juce::TextButton lpgButton { "PLAITS LPG" }, clearCables { "REMOVE ALL CABLES" };
    Knob oscDecay, lpgColour, fine, drive, keyTrack, velocity, seqGate, swing;
    Switch playMode, vcoSrc, vcoDest;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> voicesAttach, bendAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> priorityAttach, channelAttach, bankAttach, assignAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lpgAttach;
};

} // namespace grind::ui
