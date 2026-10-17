#pragma once

#include "PluginProcessor.h"
#include <map>

namespace cj::ui
{
namespace colours
{
    const juce::Colour background   { 0xff0d0e11 };
    const juce::Colour panel        { 0xff1a1c21 };
    const juce::Colour panelEdge    { 0xff34373f };
    const juce::Colour text         { 0xffeef0f4 };
    const juce::Colour dimText      { 0xff8b919c };
    const juce::Colour orange       { 0xffff5a1f };
    const juce::Colour yellow       { 0xffffc21a };
    const juce::Colour red          { 0xffe8262d };
    const juce::Colour green        { 0xff3ddc6a };
    const juce::Colour chrome       { 0xffd3d8e0 };
    const juce::Colour teal         { 0xff2ad4c4 };
}

// Jarrad, the CJ Autoworx logo
const juce::Image& jarradImage();
void drawJarrad (juce::Graphics&, juce::Rectangle<float> area, juce::Colour ring, float glow = 0.0f);

class GarageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GarageLookAndFeel();
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
// The big rev counter. Jarrad sits in the middle as the needle hub.
class Tachometer : public juce::Component
{
public:
    explicit Tachometer (CJAutoworxProcessor&);
    void paint (juce::Graphics&) override;
    void tick();

private:
    CJAutoworxProcessor& processor;
    float rpm = 0, redline = 6500, boost = 0, throttle = 0;
    int gear = -1;
    juce::Random random;
    juce::Point<float> shake;
};

//==============================================================================
// Cylinder lights in firing order, one row per exhaust bank
class CylinderLights : public juce::Component
{
public:
    explicit CylinderLights (CJAutoworxProcessor&);
    void paint (juce::Graphics&) override;
    void tick();

private:
    CJAutoworxProcessor& processor;
    std::array<float, cj::maxCylinders> lights {};
    float bang = 0.0f;
};

//==============================================================================
// Press and hold Jarrad's face to rev the engine
class RevButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit RevButton (CJAutoworxProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void tick();

private:
    CJAutoworxProcessor& processor;
    bool down = false;
    float glow = 0.0f;
};

//==============================================================================
// "Name That Engine": Jarrad can name any engine by ear. Can you?
class QuizPanel : public juce::Component
{
public:
    explicit QuizPanel (CJAutoworxProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void newRound();
    void tick();

    std::function<void()> onClose;

private:
    void answer (int choice);

    CJAutoworxProcessor& processor;
    juce::OwnedArray<juce::TextButton> options;
    juce::TextButton replayButton { "LISTEN AGAIN" }, nextButton { "NEXT ENGINE" }, closeButton { "CLOSE" };
    std::array<int, 4> choices {};
    int correct = -1, lastEngine = -1, picked = -1;
    int rounds = 0, score = 0;
    juce::String result;
    juce::Random random;
};

} // namespace cj::ui

//==============================================================================
class CJAutoworxEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit CJAutoworxEditor (CJAutoworxProcessor&);
    ~CJAutoworxEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int designWidth = 1180;
    static constexpr int designHeight = 720;

    void showQuiz();
    void refresh(); // pull the latest readouts from the processor

private:
    class Content : public juce::Component
    {
    public:
        explicit Content (CJAutoworxEditor& e) : editor (e) {}
        void paint (juce::Graphics&) override;
        CJAutoworxEditor& editor;
    };

    void timerCallback() override;
    void layout();
    void setMode (int newMode);
    cj::ui::Knob* addKnob (const char* id, const char* name, juce::Colour);

    CJAutoworxProcessor& processor;
    cj::ui::GarageLookAndFeel lnf;
    Content content { *this };

    juce::ComboBox presetBox, layoutBox;
    juce::TextButton engineModeButton { "ENGINE" }, musicalModeButton { "MUSICAL" };
    juce::TextButton ignitionButton { "IGNITION" }, gearboxButton { "AUTO SHIFT" }, monoButton { "MONO" };
    juce::TextButton passByButton { "PASS-BY" }, quizButton { "NAME THAT ENGINE" };
    juce::Label hintLabel;
    juce::OwnedArray<cj::ui::Knob> knobs;
    std::map<juce::String, cj::ui::Knob*> knobById;

    cj::ui::Tachometer tach;
    cj::ui::CylinderLights lights;
    cj::ui::RevButton revButton;
    cj::ui::QuizPanel quiz;
    juce::MidiKeyboardComponent keyboard;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> layoutAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ignitionAttachment, gearboxAttachment, monoAttachment;

    int shownMode = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CJAutoworxEditor)
};
