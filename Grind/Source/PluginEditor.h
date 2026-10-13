#pragma once

#include "EditorViews.h"

namespace grind::ui
{
// The panel is drawn in the coordinates of the hardware's front panel
// (2000 x 1030) and scaled to whatever size the window is.
constexpr float panelWidth = 2000.0f, panelHeight = 1030.0f;

namespace hw
{
    const juce::Colour face     { 0xff232326 };
    const juce::Colour faceDark { 0xff141416 };
    const juce::Colour orange   { 0xffff5a1f };
    const juce::Colour label    { 0xffeeeeee };
    const juce::Colour padRed   { 0xffff2b2b };
    const juce::Colour ledRed   { 0xffff2020 };
    const juce::Colour ledGreen { 0xff3dff4a };
    const juce::Colour wood     { 0xff8c4524 };
}

class HardwareLookAndFeel : public GrindLookAndFeel
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    // The panel is drawn at hardware scale, so text sizes are larger than usual
    juce::Font getComboBoxFont (juce::ComboBox&) override { return uiFont (24.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return uiFont (22.0f); }
    juce::Font getPopupMenuFont() override { return uiFont (30.0f, false); }
};

//==============================================================================
class PanelKnob : public juce::Slider
{
public:
    PanelKnob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, bool small);

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
// Bat-handle toggle switch (2 or 3 positions) bound to a choice / bool parameter.
// valueForPosition maps the lever position (left .. right) to the parameter value.
class ToggleSwitch : public juce::Component, public juce::SettableTooltipClient
{
public:
    ToggleSwitch (juce::AudioProcessorValueTreeState&, const juce::String& paramId, std::vector<int> valueForPosition);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    std::vector<int> values;
    int position = 0;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

//==============================================================================
// Rubber button / pad. Lit state is set by the panel; supports press, release,
// click and long-press callbacks (long-press stands in for right-click on touch screens).
class PanelButton : public juce::Component
{
public:
    enum Style { square, pad, key, small };
    PanelButton (Style s) : style (s) {} // NOLINT: implicit so the pad arrays can be brace-initialised

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    void setLit (float level, juce::Colour c = hw::padRed) { if (std::abs (level - lit) > 1.0e-3f || c != litColour) { lit = level; litColour = c; repaint(); } }
    void setText (const juce::String& t) { if (t != text) { text = t; repaint(); } }

    std::function<void()> onPress, onRelease, onClick, onLongPress;

private:
    Style style;
    float lit = 0.0f;
    juce::Colour litColour = hw::padRed;
    juce::String text;
    bool down = false;
};

//==============================================================================
// The A / B model rows with their LEDs. The BANK button also reaches bank C
// (the four extended engines).
class ModelDisplay : public juce::Component
{
public:
    explicit ModelDisplay (MidnightGrindProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void tick();

private:
    juce::Rectangle<float> iconBounds (int row, int index) const;
    MidnightGrindProcessor& processor;
    int model = -1;
};

//==============================================================================
// Jacks and patch cables. Drag from any jack to another to patch; drag a cable's
// plug off an input to move or remove it; tap a patched input to unpatch it.
class CableLayer : public juce::Component
{
public:
    explicit CableLayer (MidnightGrindProcessor&);
    void paint (juce::Graphics&) override;
    bool hitTest (int x, int y) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void tick();

    static juce::Point<float> jackCentre (int slot);

private:
    int jackAt (juce::Point<float>) const;
    void drawCable (juce::Graphics&, juce::Point<float> a, juce::Point<float> b, juce::Colour) const;

    MidnightGrindProcessor& processor;
    int dragFromSlot = -1, detachedInput = -1;
    juce::Point<float> dragPos;
    bool dragging = false, moved = false;
    uint32_t shownVersion = 0;
};

//==============================================================================
class MenuOverlay : public juce::Component
{
public:
    explicit MenuOverlay (MidnightGrindProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void showTab (int index);
    void tick();

    std::function<void()> onClose;
    static constexpr float logicalWidth = 1050.0f, logicalHeight = 520.0f;

private:
    struct Tabs : juce::TabbedButtonBar
    {
        Tabs() : TabbedButtonBar (TabsAtTop) {}
        std::function<void (int)> onTabChanged;
        void currentTabChanged (int index, const juce::String&) override { if (onTabChanged) onTabChanged (index); }
    } tabs;

    juce::TextButton closeButton { "CLOSE" };
    SequencerView sequencer;
    ArpView arp;
    SettingsView settings;
};

//==============================================================================
class GrindPanel : public juce::Component
{
public:
    explicit GrindPanel (MidnightGrindProcessor&);
    ~GrindPanel() override;
    void paint (juce::Graphics&) override;
    void tick();

    MenuOverlay& getMenu() noexcept { return menu; }
    void showMenu (int tab);

private:
    void placeKnob (juce::Slider&, float cx, float cy, bool small);
    void placeSwitch (ToggleSwitch&, float cx, float cy);
    void drawSection (juce::Graphics&, juce::Rectangle<float>, const juce::String& title) const;
    void drawLabel (juce::Graphics&, const juce::String&, float cx, float cy, float size = 21.0f) const;
    void drawBoxedLabel (juce::Graphics&, const juce::String&, float cx, float cy, bool filled, float size = 17.0f) const;

    void setParam (const juce::String& id, float value);
    float getParam (const juce::String& id) const;
    void padPressed (int pad, bool black, bool down);
    void stepPadLongPress (int pad);
    void showPatternMenu();
    void updateSequencerLights();

    MidnightGrindProcessor& processor;
    HardwareLookAndFeel lnf;

    juce::ComboBox presetBox;
    juce::TextButton menuButton { "MENU" };

    PanelKnob timbre, timbreAmt, harmonics, fm, frequency, morph, morphAmt;
    PanelKnob cutoff, resonance, vcfMod, volume;
    PanelKnob attack, decay, sustain, oscMod, lfoRate, glide, vcMix;
    PanelKnob tempo, swing;
    ToggleSwitch vcfMode, vcfModSrc, vcfModPol, vcaMode, lfoShape;

    PanelButton bankButton { PanelButton::square }, modelButton { PanelButton::square };
    ModelDisplay modelDisplay;

    PanelButton holdButton { PanelButton::small }, resetButton { PanelButton::small }, arpButton { PanelButton::small },
                patternButton { PanelButton::small }, shiftButton { PanelButton::small }, pageButton { PanelButton::small },
                playButton { PanelButton::small }, recButton { PanelButton::small }, kybdButton { PanelButton::small },
                stepButton { PanelButton::small }, leftArrow { PanelButton::small }, rightArrow { PanelButton::small };
    std::array<PanelButton, 8> pads { PanelButton::pad, PanelButton::pad, PanelButton::pad, PanelButton::pad,
                                      PanelButton::pad, PanelButton::pad, PanelButton::pad, PanelButton::pad };
    std::array<PanelButton, 5> blackKeys { PanelButton::key, PanelButton::key, PanelButton::key, PanelButton::key, PanelButton::key };

    CableLayer cables;
    MenuOverlay menu;

    bool shift = false, stepMode = false;
    int octave = 3, page = 0;
    std::array<int, 8> padNote {};
    std::array<int, 5> keyNote {};
    float lfoLed = 0.0f;
    int shownProgram = -1;
};

//==============================================================================
class MidnightGrindEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MidnightGrindEditor (MidnightGrindProcessor&);
    ~MidnightGrindEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // -1 closes the menu, 0..2 opens it on that tab (also used by the render test)
    void showMenu (int tab) { panel.showMenu (tab); }

private:
    void timerCallback() override { panel.tick(); }

    MidnightGrindProcessor& processor;
    GrindLookAndFeel lnf;
    GrindPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidnightGrindEditor)
};

} // namespace grind::ui
