#include "PluginEditor.h"
#include "Presets.h"

using namespace grind::ui;

namespace
{
juce::Font uiFont (float height, bool bold = true)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

juce::String noteName (int note)
{
    return juce::MidiMessage::getMidiNoteName (note, true, true, 4);
}

juce::Colour bankColour (grind::EngineBank b)
{
    switch (b)
    {
        case grind::EngineBank::synth:      return colours::amber;
        case grind::EngineBank::percussion: return colours::orange;
        case grind::EngineBank::extended:   return colours::teal;
    }
    return colours::amber;
}

constexpr int editorWidth = 1280;
constexpr int editorHeight = 828;
} // namespace

//==============================================================================
GrindLookAndFeel::GrindLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, colours::panelLight);
    setColour (juce::ComboBox::textColourId, colours::text);
    setColour (juce::ComboBox::outlineColourId, colours::edge);
    setColour (juce::ComboBox::arrowColourId, colours::amber);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff141720));
    setColour (juce::PopupMenu::textColourId, colours::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::amber.withAlpha (0.3f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, colours::panelLight);
    setColour (juce::TextButton::buttonOnColourId, colours::amber);
    setColour (juce::TextButton::textColourOffId, colours::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xf0141720));
    setColour (juce::BubbleComponent::outlineColourId, colours::amber);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xf0141720));
    setColour (juce::TooltipWindow::textColourId, colours::text);
    setColour (juce::TooltipWindow::outlineColourId, colours::amber);
    setColour (juce::Slider::textBoxTextColourId, colours::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, colours::panelLight);
    setColour (juce::TabbedButtonBar::tabTextColourId, colours::dimText);
    setColour (juce::TabbedButtonBar::frontTextColourId, colours::text);
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, colours::amber.withAlpha (0.7f));
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, colours::amber.withAlpha (0.25f));
    setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.4f));
    setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, colours::panelLight);
    setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, colours::amber);
}

void GrindLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider& slider)
{
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (2.0f);
    const float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float arcRadius = radius - 2.0f;
    const float thickness = std::max (2.0f, radius * 0.11f);

    // Scale ticks
    g.setColour (colours::edge.brighter (0.3f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
        g.drawLine (juce::Line<float> (centre.getPointOnCircumference (radius - 0.5f, a),
                                       centre.getPointOnCircumference (radius - 3.0f, a)), 1.0f);
    }

    // Value arc (bipolar parameters draw from the centre)
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius - 4.0f, arcRadius - 4.0f, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff0a0c10));
    g.strokePath (track, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    if (std::abs (angle - from) > 0.01f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcRadius - 4.0f, arcRadius - 4.0f, 0.0f,
                             std::min (from, angle), std::max (from, angle), true);
        g.setColour (accent);
        g.strokePath (value, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Knob cap
    const float bodyR = radius * 0.62f;
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR + 2.0f, bodyR * 2.0f, bodyR * 2.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3f4c), centre.x, centre.y - bodyR,
                                             juce::Colour (0xff15171d), centre.x, centre.y + bodyR, false));
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    // Pointer
    const auto tip = centre.getPointOnCircumference (bodyR * 0.92f, angle);
    const auto base = centre.getPointOnCircumference (bodyR * 0.3f, angle);
    g.setColour (juce::Colours::white);
    g.drawLine ({ base, tip }, std::max (1.6f, radius * 0.07f));
}

void GrindLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                         juce::Slider::SliderStyle, juce::Slider& slider)
{
    const auto r = juce::Rectangle<float> ((float) x, (float) y + (float) h * 0.5f - 3.0f, (float) w, 6.0f);
    g.setColour (juce::Colour (0xff0a0c10));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (slider.findColour (juce::Slider::trackColourId));
    g.fillRoundedRectangle (r.withRight (pos), 3.0f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (pos - 6.0f, r.getCentreY() - 6.0f, 12.0f, 12.0f);
}

void GrindLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                             bool over, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = button.getToggleState();
    const auto accent = button.findColour (juce::TextButton::buttonOnColourId);

    if (on)
    {
        g.setColour (accent.withAlpha (0.25f));
        g.fillRoundedRectangle (bounds.expanded (1.5f), 4.0f);
        g.setColour (accent.withAlpha (down ? 1.0f : 0.9f));
        g.fillRoundedRectangle (bounds, 3.0f);
    }
    else
    {
        g.setColour (colours::panelLight.brighter (over ? 0.2f : 0.0f).brighter (down ? 0.2f : 0.0f));
        g.fillRoundedRectangle (bounds, 3.0f);
        g.setColour (over ? accent.withAlpha (0.8f) : colours::edge);
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
    }
}

void GrindLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    g.setFont (getTextButtonFont (button, button.getHeight()));
    g.setColour (button.findColour (button.getToggleState() ? juce::TextButton::textColourOnId
                                                             : juce::TextButton::textColourOffId));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (3, 1), juce::Justification::centred, 1, 0.7f);
}

juce::Font GrindLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return uiFont (std::min (12.5f, (float) buttonHeight * 0.55f));
}

void GrindLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (box.isMouseOver (true) ? colours::amber.withAlpha (0.8f) : colours::edge);
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

    juce::Path arrow;
    const float ax = (float) w - 13.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (colours::amber);
    g.fillPath (arrow);
}

juce::Font GrindLookAndFeel::getComboBoxFont (juce::ComboBox&) { return uiFont (13.0f); }
juce::Font GrindLookAndFeel::getPopupMenuFont() { return uiFont (14.0f, false); }

juce::Label* GrindLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (slider);
    l->setFont (uiFont (12.0f));
    l->setJustificationType (juce::Justification::centred);
    return l;
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& text,
            juce::Colour colour, Size s)
    : label (text), size (s)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    slider.setDoubleClickReturnValue (true, state.getParameter (paramId)->convertFrom0to1 (
                                                state.getParameter (paramId)->getDefaultValue()));
    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);
    slider.setTooltip (state.getParameter (paramId)->getName (64));
}

void Knob::resized()
{
    auto b = getLocalBounds();
    b.removeFromTop (size == small ? 13 : 15);
    if (subLabel.isNotEmpty() || size == large)
        b.removeFromBottom (14);
    slider.setBounds (b.withSizeKeepingCentre (std::min (b.getWidth(), b.getHeight()), std::min (b.getWidth(), b.getHeight())));
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (colours::text);
    g.setFont (uiFont (size == small ? 10.5f : (size == large ? 13.0f : 11.5f)));
    g.drawFittedText (label, getLocalBounds().removeFromTop (size == small ? 13 : 15), juce::Justification::centred, 1, 0.7f);
    if (subLabel.isNotEmpty())
    {
        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        g.setFont (uiFont (11.0f, false));
        g.drawFittedText (subLabel, getLocalBounds().removeFromBottom (14), juce::Justification::centred, 1, 0.7f);
    }
}

//==============================================================================
Switch::Switch (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& t,
                const juce::StringArray& l, juce::Colour c, bool v)
    : title (t), labels (l), colour (c), vertical (v)
{
    attachment = std::make_unique<juce::ParameterAttachment> (*state.getParameter (paramId), [this] (float value)
    {
        current = juce::roundToInt (value);
        repaint();
    });
    attachment->sendInitialUpdate();
    setTooltip (state.getParameter (paramId)->getName (64));
}

void Switch::paint (juce::Graphics& g)
{
    auto b = getLocalBounds();
    if (title.isNotEmpty())
    {
        g.setColour (colours::text);
        g.setFont (uiFont (10.5f));
        g.drawFittedText (title, b.removeFromTop (14), juce::Justification::centred, 1, 0.7f);
    }

    const int n = labels.size();
    g.setColour (juce::Colour (0xff0a0c10));
    g.fillRoundedRectangle (b.toFloat().reduced (1.0f), 3.0f);

    for (int i = 0; i < n; ++i)
    {
        auto seg = vertical ? b.withHeight (b.getHeight() / n).withY (b.getY() + i * b.getHeight() / n)
                            : b.withWidth (b.getWidth() / n).withX (b.getX() + i * b.getWidth() / n);
        const bool on = i == current;
        if (on)
        {
            g.setColour (colour.withAlpha (0.9f));
            g.fillRoundedRectangle (seg.toFloat().reduced (2.0f), 2.5f);
        }
        g.setColour (on ? juce::Colours::black : colours::dimText);
        g.setFont (uiFont (10.5f));
        g.drawFittedText (labels[i], seg.reduced (2, 0), juce::Justification::centred, 1, 0.6f);
    }
}

void Switch::mouseDown (const juce::MouseEvent& e)
{
    auto b = getLocalBounds();
    if (title.isNotEmpty())
        b.removeFromTop (14);
    const int n = labels.size();
    if (n == 0 || ! b.contains (e.getPosition()))
        return;
    const int index = vertical ? (e.y - b.getY()) * n / std::max (1, b.getHeight())
                               : (e.x - b.getX()) * n / std::max (1, b.getWidth());
    attachment->setValueAsCompleteGesture ((float) juce::jlimit (0, n - 1, index));
}

//==============================================================================
ModelSelector::ModelSelector (juce::AudioProcessorValueTreeState& state)
{
    attachment = std::make_unique<juce::ParameterAttachment> (*state.getParameter ("model"), [this] (float v)
    {
        current = juce::roundToInt (v);
        repaint();
    });
    attachment->sendInitialUpdate();
}

void ModelSelector::resized() {}

juce::Rectangle<int> ModelSelector::buttonBounds (int e) const
{
    // Synth: 2 rows of 5, Percussion: 2 rows of 5, Extended: 1 row of 4
    const int w = getWidth() / 5;
    const int h = 22, gap = 3, labelH = 14;
    int group, index;
    if (e < 10)       { group = 0; index = e; }
    else if (e < 20)  { group = 1; index = e - 10; }
    else              { group = 2; index = e - 20; }

    const int groupTop = group == 0 ? 0 : group == 1 ? labelH + 2 * (h + gap) + 6 : 2 * (labelH + 2 * (h + gap) + 6);
    const int row = index / 5, col = index % 5;
    return { col * w + 1, groupTop + labelH + row * (h + gap), w - 2, h };
}

void ModelSelector::paint (juce::Graphics& g)
{
    const char* groupNames[] = { "SYNTH", "PERCUSSION / NOISE", "EXTENDED" };
    for (int group = 0; group < 3; ++group)
    {
        const auto first = buttonBounds (group * 10);
        g.setColour (bankColour ((grind::EngineBank) group));
        g.setFont (uiFont (10.0f));
        g.drawText (groupNames[group], 2, first.getY() - 14, getWidth(), 13, juce::Justification::centredLeft);
    }

    for (int e = 0; e < grind::numEngines; ++e)
    {
        const auto& info = grind::engineInfo[(size_t) e];
        const auto b = buttonBounds (e).toFloat();
        const auto c = bankColour (info.bank);
        const bool on = e == current;

        g.setColour (on ? c : colours::panelLight);
        g.fillRoundedRectangle (b, 3.0f);
        g.setColour (on ? c.brighter (0.3f) : colours::edge);
        g.drawRoundedRectangle (b.reduced (0.5f), 3.0f, 1.0f);
        g.setColour (on ? juce::Colours::black : colours::text);
        g.setFont (uiFont (10.0f));
        g.drawFittedText (info.shortName, b.toNearestInt().reduced (2, 0), juce::Justification::centred, 1, 0.6f);
    }
}

void ModelSelector::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < grind::numEngines; ++i)
        if (buttonBounds (i).contains (e.getPosition()))
        {
            attachment->setValueAsCompleteGesture ((float) i);
            return;
        }
}

//==============================================================================
void Display::tick()
{
    const int m = (int) std::lround (processor.apvts.getRawParameterValue ("model")->load());
    const int v = (int) std::lround (processor.apvts.getRawParameterValue ("voices")->load());
    const float e = processor.getEnvelopeLevel();
    if (m != model || v != voices || std::abs (e - env) > 0.01f)
    {
        model = m;
        voices = v;
        env = e;
        repaint();
    }
}

void Display::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (colours::lcd);
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (colours::edge);
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);

    if (! juce::isPositiveAndBelow (model, grind::numEngines))
        return;
    const auto& info = grind::engineInfo[(size_t) model];
    auto r = getLocalBounds().reduced (10, 6);

    g.setColour (colours::lcdText);
    g.setFont (uiFont (11.0f));
    const char* bankNames[] = { "SYNTH", "PERC/NOISE", "EXTENDED" };
    g.drawText (juce::String (model + 1).paddedLeft ('0', 2) + "  " + bankNames[(int) info.bank],
                r.removeFromTop (14), juce::Justification::centredLeft);
    g.setFont (uiFont (21.0f));
    g.drawText (juce::String (info.name).toUpperCase(), r.removeFromTop (26), juce::Justification::centredLeft);

    g.setFont (uiFont (10.5f, false));
    g.setColour (colours::lcdText.withAlpha (0.75f));
    g.drawText (juce::String ("AUX: ") + info.aux, r.removeFromTop (14), juce::Justification::centredLeft);
    g.drawText (voices > 1 ? "POLY CHAIN  " + juce::String (voices) + " VOICES" : juce::String ("MONO"),
                r.removeFromTop (14), juce::Justification::centredLeft);

    // Envelope bar
    auto bar = r.removeFromTop (8).toFloat().withTrimmedTop (2.0f);
    g.setColour (colours::lcdText.withAlpha (0.15f));
    g.fillRect (bar);
    g.setColour (colours::lcdText);
    g.fillRect (bar.withWidth (bar.getWidth() * juce::jlimit (0.0f, 1.0f, env)));
}

//==============================================================================
SequencerView::SequencerView (MidnightGrindProcessor& p)
    : processor (p),
      gate (p.apvts, "seqGate", "GATE", colours::amber, Knob::small),
      swing (p.apvts, "swing", "SWING", colours::amber, Knob::small),
      tempo (p.apvts, "tempo", "TEMPO", colours::amber, Knob::small)
{
    patternBox.addItemList (MidnightGrindProcessor::patternNames(), 1);
    patternAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "pattern", patternBox);
    patternBox.setTooltip ("Pattern slot: 8 banks (A-H) of 8 patterns");

    for (auto* r : grind::rateNames)
        rateBox.addItem (r, rateBox.getNumItems() + 1);
    rateAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "seqRate", rateBox);

    lengthSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    lengthSlider.setRange (1, grind::numSteps, 1);
    lengthSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 34, 20);
    lengthSlider.setColour (juce::Slider::trackColourId, colours::amber);
    lengthSlider.onValueChange = [this]
    {
        processor.getPerformance().setLength (pattern(), (int) lengthSlider.getValue());
        repaint();
    };
    lengthSlider.setTooltip ("Pattern length (steps)");

    runButton.setClickingTogglesState (true);
    recButton.setClickingTogglesState (true);
    recButton.setColour (juce::TextButton::buttonOnColourId, colours::red);
    runAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "seqRun", runButton);
    recAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "seqRec", recButton);
    runButton.setTooltip ("Run the sequencer from its own clock (it always follows the host transport when that is playing)");
    recButton.setTooltip ("Step record: play notes to write them at the cursor. Click a step to move the cursor.");

    clearButton.onClick = [this] { processor.getPerformance().clearPattern (pattern()); repaint(); };
    copyButton.onClick = [this]
    {
        for (int s = 0; s < grind::numSteps; ++s)
            clipboard[(size_t) s] = processor.getPerformance().getStep (pattern(), s).pack();
        clipboardLength = processor.getPerformance().getLength (pattern());
    };
    pasteButton.onClick = [this]
    {
        if (clipboardLength == 0) return;
        for (int s = 0; s < grind::numSteps; ++s)
            processor.getPerformance().setStep (pattern(), s, grind::Step::unpack (clipboard[(size_t) s]));
        processor.getPerformance().setLength (pattern(), clipboardLength);
        shownPattern = -1;
        repaint();
    };
    randomButton.onClick = [this]
    {
        // A random line in C minor pentatonic, with some accents, slides and rests
        static const int scale[] = { 0, 3, 5, 7, 10 };
        auto& rng = juce::Random::getSystemRandom();
        for (int s = 0; s < grind::numSteps; ++s)
        {
            grind::Step st;
            st.note = 36 + 12 * rng.nextInt (2) + scale[rng.nextInt (5)];
            st.gate = rng.nextFloat() > 0.18f;
            st.accent = rng.nextFloat() > 0.75f;
            st.slide = rng.nextFloat() > 0.8f;
            processor.getPerformance().setStep (pattern(), s, st);
        }
        repaint();
    };

    for (auto* c : std::initializer_list<juce::Component*> { &patternBox, &rateBox, &lengthSlider, &runButton, &recButton,
                                                             &clearButton, &copyButton, &pasteButton, &randomButton,
                                                             &gate, &swing, &tempo })
        addAndMakeVisible (c);
}

void SequencerView::resized()
{
    auto b = getLocalBounds().reduced (10, 6);
    auto controls = b.removeFromLeft (250);

    auto row = controls.removeFromTop (26);
    patternBox.setBounds (row.removeFromLeft (70));
    row.removeFromLeft (6);
    rateBox.setBounds (row.removeFromLeft (70));
    row.removeFromLeft (6);
    runButton.setBounds (row.removeFromLeft (46));
    row.removeFromLeft (4);
    recButton.setBounds (row.removeFromLeft (46));

    controls.removeFromTop (6);
    lengthSlider.setBounds (controls.removeFromTop (24));
    controls.removeFromTop (6);

    row = controls.removeFromTop (24);
    const int bw = (row.getWidth() - 12) / 4;
    for (auto* btn : { &clearButton, &copyButton, &pasteButton, &randomButton })
    {
        btn->setBounds (row.removeFromLeft (bw));
        row.removeFromLeft (4);
    }

    controls.removeFromTop (8);
    row = controls.removeFromTop (72);
    const int kw = row.getWidth() / 3;
    gate.setBounds (row.removeFromLeft (kw));
    swing.setBounds (row.removeFromLeft (kw));
    tempo.setBounds (row);

    b.removeFromLeft (14);
    gridArea = b.withTrimmedTop (16).withTrimmedBottom (34);
}

juce::Rectangle<int> SequencerView::stepBounds (int step) const
{
    const int row = step / 16, col = step % 16;
    const int w = gridArea.getWidth() / 16;
    const int h = (gridArea.getHeight() - 8) / 2;
    return { gridArea.getX() + col * w, gridArea.getY() + row * (h + 8), w - 3, h };
}

int SequencerView::stepAt (juce::Point<int> pos) const
{
    for (int s = 0; s < grind::numSteps; ++s)
        if (stepBounds (s).contains (pos))
            return s;
    return -1;
}

void SequencerView::paint (juce::Graphics& g)
{
    auto& perf = processor.getPerformance();
    const int pat = pattern();
    const int length = perf.getLength (pat);

    g.setColour (colours::dimText);
    g.setFont (uiFont (10.5f, false));
    g.drawText ("Click: gate on/off   Drag: note   Wheel: note   Shift-drag: octave   Right-click: accent / slide / tie / ratchet",
                gridArea.getX(), gridArea.getY() - 16, gridArea.getWidth(), 14, juce::Justification::centredLeft);

    for (int s = 0; s < grind::numSteps; ++s)
    {
        const auto st = perf.getStep (pat, s);
        const auto r = stepBounds (s).toFloat();
        const bool inPattern = s < length;
        const bool isPlaying = s == playing;

        g.setColour (isPlaying ? colours::amber.withAlpha (0.35f) : colours::panelLight.withAlpha (inPattern ? 1.0f : 0.35f));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (s == cursor && processor.apvts.getRawParameterValue ("seqRec")->load() > 0.5f ? colours::red
                     : (s % 4 == 0 ? colours::edge.brighter (0.4f) : colours::edge));
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, s == cursor ? 2.0f : 1.0f);

        const float alpha = inPattern ? 1.0f : 0.3f;
        auto inner = r.reduced (4.0f);

        // Gate LED
        auto led = inner.removeFromTop (8.0f).withSizeKeepingCentre (14.0f, 6.0f);
        g.setColour ((st.gate ? (st.accent ? colours::orange : colours::amber) : juce::Colour (0xff0a0c10)).withAlpha (alpha));
        g.fillRoundedRectangle (led, 2.0f);

        // Note
        g.setColour ((st.gate || st.tie ? colours::text : colours::dimText).withAlpha (alpha));
        g.setFont (uiFont (std::min (15.0f, inner.getHeight() * 0.32f)));
        g.drawFittedText (st.tie ? juce::String ("TIE") : st.gate ? noteName (st.note) : juce::String ("-"),
                          inner.removeFromTop (inner.getHeight() * 0.5f).toNearestInt(), juce::Justification::centred, 1, 0.6f);

        // Flags
        juce::String stepFlags;
        if (st.accent) stepFlags << "A ";
        if (st.slide) stepFlags << "S ";
        if (st.ratchet > 1) stepFlags << "x" << st.ratchet;
        g.setColour (colours::teal.withAlpha (alpha));
        g.setFont (uiFont (10.0f));
        g.drawFittedText (stepFlags.trim(), inner.toNearestInt(), juce::Justification::centred, 1, 0.6f);

        g.setColour (colours::dimText.withAlpha (0.6f * alpha));
        g.setFont (uiFont (9.0f, false));
        g.drawText (juce::String (s + 1), r.toNearestInt().reduced (3, 2), juce::Justification::topLeft);
    }

    auto info = getLocalBounds().reduced (10, 6).removeFromBottom (24).withTrimmedLeft (264);
    g.setColour (colours::dimText);
    g.setFont (uiFont (11.0f, false));
    const int tr = perf.getTranspose();
    g.drawText ("Pattern " + MidnightGrindProcessor::patternNames()[pat] + "  |  " + juce::String (length) + " steps"
                    + (tr != 0 ? "  |  transpose " + juce::String (tr > 0 ? "+" : "") + juce::String (tr) : juce::String())
                    + "  |  plays in SEQ mode; keys transpose while it runs",
                info, juce::Justification::centredLeft);
}

void SequencerView::mouseDown (const juce::MouseEvent& e)
{
    const int s = stepAt (e.getPosition());
    if (s < 0)
        return;

    if (e.mods.isPopupMenu())
    {
        showStepMenu (s);
        return;
    }

    dragStep = s;
    dragStartNote = processor.getPerformance().getStep (pattern(), s).note;
    dragStartY = e.y;
    dragged = false;
    processor.getPerformance().setRecordCursor (s);
}

void SequencerView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragStep < 0)
        return;
    const int dy = dragStartY - e.y;
    if (! dragged && std::abs (dy) < 4)
        return;
    dragged = true;

    auto st = processor.getPerformance().getStep (pattern(), dragStep);
    const int delta = e.mods.isShiftDown() ? 12 * (dy / 24) : dy / 6;
    st.note = juce::jlimit (0, 127, dragStartNote + delta);
    st.gate = true;
    processor.getPerformance().setStep (pattern(), dragStep, st);
    repaint();
}

void SequencerView::mouseUp (const juce::MouseEvent&)
{
    if (dragStep >= 0 && ! dragged)
    {
        auto st = processor.getPerformance().getStep (pattern(), dragStep);
        st.gate = ! st.gate;
        if (! st.gate) st.tie = false;
        processor.getPerformance().setStep (pattern(), dragStep, st);
        repaint();
    }
    dragStep = -1;
}

void SequencerView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int s = stepAt (e.getPosition());
    if (s < 0)
        return;
    auto st = processor.getPerformance().getStep (pattern(), s);
    st.note = juce::jlimit (0, 127, st.note + (w.deltaY > 0 ? 1 : -1) * (e.mods.isShiftDown() ? 12 : 1));
    processor.getPerformance().setStep (pattern(), s, st);
    repaint();
}

void SequencerView::showStepMenu (int s)
{
    auto st = processor.getPerformance().getStep (pattern(), s);
    juce::PopupMenu m;
    m.addSectionHeader ("Step " + juce::String (s + 1) + "  " + noteName (st.note));
    m.addItem (1, "Gate", true, st.gate);
    m.addItem (2, "Accent", true, st.accent);
    m.addItem (3, "Slide", true, st.slide);
    m.addItem (4, "Tie (hold previous note)", true, st.tie);
    juce::PopupMenu ratchet;
    for (int r = 1; r <= 4; ++r)
        ratchet.addItem (10 + r, r == 1 ? juce::String ("Off") : "x" + juce::String (r), true, st.ratchet == r);
    m.addSubMenu ("Ratchet", ratchet);
    m.addSeparator();
    m.addItem (20, "Octave up");
    m.addItem (21, "Octave down");
    m.addItem (22, "Set pattern length to here");

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (
                         localAreaToGlobal (stepBounds (s))),
                     [this, s, st] (int result) mutable
                     {
                         if (result == 0) return;
                         if (result == 1) st.gate = ! st.gate;
                         if (result == 2) st.accent = ! st.accent;
                         if (result == 3) st.slide = ! st.slide;
                         if (result == 4) { st.tie = ! st.tie; if (st.tie) st.gate = true; }
                         if (result >= 11 && result <= 14) st.ratchet = result - 10;
                         if (result == 20) st.note = juce::jmin (127, st.note + 12);
                         if (result == 21) st.note = juce::jmax (0, st.note - 12);
                         if (result == 22)
                         {
                             processor.getPerformance().setLength (pattern(), s + 1);
                             shownPattern = -1;
                         }
                         else
                         {
                             processor.getPerformance().setStep (pattern(), s, st);
                         }
                         repaint();
                     });
}

void SequencerView::tick()
{
    const int pat = pattern();
    if (pat != shownPattern)
    {
        shownPattern = pat;
        lengthSlider.setValue (processor.getPerformance().getLength (pat), juce::dontSendNotification);
        repaint();
    }
    const int pl = processor.getPerformance().getPlayingStep();
    const int cur = processor.getPerformance().getRecordCursor();
    const bool seqMode = (int) std::lround (processor.apvts.getRawParameterValue ("playMode")->load()) == grind::modeSeq;
    const int shownPlaying = seqMode ? pl : -1;
    if (shownPlaying != playing || cur != cursor)
    {
        playing = shownPlaying;
        cursor = cur;
        repaint();
    }
}

//==============================================================================
ArpView::ArpView (MidnightGrindProcessor& p)
    : processor (p),
      gate (p.apvts, "arpGate", "GATE", colours::teal, Knob::small),
      swing (p.apvts, "swing", "SWING", colours::teal, Knob::small),
      tempo (p.apvts, "tempo", "TEMPO", colours::teal, Knob::small)
{
    for (auto* r : grind::rateNames)
        rateBox.addItem (r, rateBox.getNumItems() + 1);
    rateAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "arpRate", rateBox);
    holdButton.setClickingTogglesState (true);
    holdButton.setColour (juce::TextButton::buttonOnColourId, colours::teal);
    holdAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "arpHold", holdButton);
    holdButton.setTooltip ("Latch: the arpeggio keeps playing after you let go");

    for (auto* c : std::initializer_list<juce::Component*> { &rateBox, &holdButton, &gate, &swing, &tempo })
        addAndMakeVisible (c);
}

void ArpView::resized()
{
    auto b = getLocalBounds().reduced (10, 6);
    auto controls = b.removeFromLeft (250);
    auto row = controls.removeFromTop (26);
    rateBox.setBounds (row.removeFromLeft (80));
    row.removeFromLeft (8);
    holdButton.setBounds (row.removeFromLeft (70));
    controls.removeFromTop (14);
    row = controls.removeFromTop (72);
    const int kw = row.getWidth() / 3;
    gate.setBounds (row.removeFromLeft (kw));
    swing.setBounds (row.removeFromLeft (kw));
    tempo.setBounds (row);

    b.removeFromLeft (14);
    patternArea = b.removeFromTop (b.getHeight() - 70).withTrimmedTop (18);
    octaveArea = b.withTrimmedTop (20).withHeight (34).withWidth (4 * 70);
}

juce::Rectangle<int> ArpView::patternBounds (int i) const
{
    const int cols = 6;
    const int w = patternArea.getWidth() / cols;
    const int h = (patternArea.getHeight() - 6) / 2;
    return { patternArea.getX() + (i % cols) * w, patternArea.getY() + (i / cols) * (h + 6), w - 6, h };
}

juce::Rectangle<int> ArpView::octaveBounds (int i) const
{
    return { octaveArea.getX() + i * 70, octaveArea.getY(), 64, octaveArea.getHeight() };
}

void ArpView::paint (juce::Graphics& g)
{
    const int pattern = (int) std::lround (processor.apvts.getRawParameterValue ("arpPattern")->load());
    const int octaves = (int) std::lround (processor.apvts.getRawParameterValue ("arpOctaves")->load());

    g.setColour (colours::dimText);
    g.setFont (uiFont (10.5f, false));
    g.drawText ("PATTERN  (switch PLAY MODE to ARP, then hold some keys)", patternArea.getX(), patternArea.getY() - 16,
                patternArea.getWidth(), 14, juce::Justification::centredLeft);

    for (int i = 0; i < (int) grind::arpPatternNames.size(); ++i)
    {
        const auto r = patternBounds (i).toFloat();
        const bool on = i == pattern;
        g.setColour (on ? colours::teal : colours::panelLight);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (on ? colours::teal.brighter (0.3f) : colours::edge);
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
        g.setColour (on ? juce::Colours::black : colours::text);
        g.setFont (uiFont (13.0f));
        g.drawFittedText (grind::arpPatternNames[(size_t) i], r.toNearestInt(), juce::Justification::centred, 1, 0.7f);
    }

    g.setColour (colours::dimText);
    g.setFont (uiFont (10.5f, false));
    g.drawText ("OCTAVES", octaveArea.getX(), octaveArea.getY() - 16, 200, 14, juce::Justification::centredLeft);
    for (int i = 0; i < 4; ++i)
    {
        const auto r = octaveBounds (i).toFloat();
        const bool on = i + 1 == octaves;
        g.setColour (on ? colours::teal : colours::panelLight);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (on ? juce::Colours::black : colours::text);
        g.setFont (uiFont (14.0f));
        g.drawText (juce::String (i + 1), r, juce::Justification::centred);
    }

    // Playing position
    if (playing >= 0)
    {
        g.setColour (colours::teal);
        g.setFont (uiFont (12.0f));
        g.drawText ("STEP " + juce::String (playing + 1), octaveArea.getRight() + 20, octaveArea.getY(), 120,
                    octaveArea.getHeight(), juce::Justification::centredLeft);
    }
}

void ArpView::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < (int) grind::arpPatternNames.size(); ++i)
        if (patternBounds (i).contains (e.getPosition()))
        {
            auto* param = processor.apvts.getParameter ("arpPattern");
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 ((float) i));
            param->endChangeGesture();
            repaint();
            return;
        }
    for (int i = 0; i < 4; ++i)
        if (octaveBounds (i).contains (e.getPosition()))
        {
            auto* param = processor.apvts.getParameter ("arpOctaves");
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 ((float) (i + 1)));
            param->endChangeGesture();
            repaint();
            return;
        }
}

void ArpView::tick()
{
    const int pattern = (int) std::lround (processor.apvts.getRawParameterValue ("arpPattern")->load());
    const int octaves = (int) std::lround (processor.apvts.getRawParameterValue ("arpOctaves")->load());
    const bool arpMode = (int) std::lround (processor.apvts.getRawParameterValue ("playMode")->load()) == grind::modeArp;
    const int pl = arpMode ? processor.getPerformance().getPlayingStep() : -1;
    if (pattern != shownPattern || octaves != shownOctaves || pl != playing)
    {
        shownPattern = pattern;
        shownOctaves = octaves;
        playing = pl;
        repaint();
    }
}

//==============================================================================
PatchBayView::PatchBayView (MidnightGrindProcessor& p) : processor (p)
{
    clearButton.onClick = [this] { processor.clearPatches(); repaint(); };
    addAndMakeVisible (clearButton);
}

void PatchBayView::resized()
{
    auto b = getLocalBounds().reduced (10, 6);
    clearButton.setBounds (b.getRight() - 90, b.getBottom() - 22, 90, 22);
    grid = b.withTrimmedLeft (70).withTrimmedTop (34).withTrimmedBottom (26);
}

juce::Rectangle<float> PatchBayView::cellBounds (int d, int s) const
{
    const float w = (float) grid.getWidth() / (float) grind::numDestinations;
    const float h = (float) grid.getHeight() / (float) grind::numSources;
    return { (float) grid.getX() + (float) d * w, (float) grid.getY() + (float) s * h, w, h };
}

bool PatchBayView::cellAt (juce::Point<int> pos, int& d, int& s) const
{
    if (! grid.contains (pos))
        return false;
    d = juce::jlimit (0, grind::numDestinations - 1, (pos.x - grid.getX()) * grind::numDestinations / grid.getWidth());
    s = juce::jlimit (0, grind::numSources - 1, (pos.y - grid.getY()) * grind::numSources / grid.getHeight());
    return true;
}

void PatchBayView::paint (juce::Graphics& g)
{
    // Column headers (inputs)
    g.setFont (uiFont (9.5f));
    for (int d = 0; d < grind::numDestinations; ++d)
    {
        const auto c = cellBounds (d, 0);
        g.setColour (d == hoverDest ? colours::amber : colours::text);
        g.saveState();
        g.addTransform (juce::AffineTransform::rotation (-0.6f, c.getCentreX(), (float) grid.getY() - 4.0f));
        g.drawText (grind::destinationNames[(size_t) d], (int) c.getCentreX() - 2, grid.getY() - 12, 80, 12,
                    juce::Justification::centredLeft);
        g.restoreState();
    }

    // Row headers (outputs)
    g.setFont (uiFont (10.0f));
    for (int s = 0; s < grind::numSources; ++s)
    {
        const auto c = cellBounds (0, s);
        g.setColour (s == hoverSource ? colours::amber : colours::text);
        g.drawText (grind::sourceNames[(size_t) s], grid.getX() - 68, (int) c.getY(), 64, (int) c.getHeight(),
                    juce::Justification::centredRight);
    }

    for (int d = 0; d < grind::numDestinations; ++d)
        for (int s = 0; s < grind::numSources; ++s)
        {
            const auto c = cellBounds (d, s);
            const float amount = processor.getPatch (d, s);
            const bool hover = d == hoverDest || s == hoverSource;
            g.setColour (hover ? colours::panelLight.brighter (0.1f) : ((d + s) % 2 == 0 ? colours::panelLight : colours::panel));
            g.fillRect (c.reduced (0.5f));

            const float jackR = std::min (c.getWidth(), c.getHeight()) * 0.36f;
            const auto centre = c.getCentre();
            g.setColour (juce::Colour (0xff07080b));
            g.fillEllipse (centre.x - jackR, centre.y - jackR, jackR * 2, jackR * 2);

            if (grind::isPatched (amount))
            {
                const auto col = amount > 0.0f ? colours::amber : colours::teal;
                juce::Path arc;
                arc.addCentredArc (centre.x, centre.y, jackR, jackR, 0.0f, 0.0f,
                                   juce::MathConstants<float>::twoPi * std::abs (amount), true);
                g.setColour (col);
                g.strokePath (arc, juce::PathStrokeType (2.2f));
                g.fillEllipse (centre.x - jackR * 0.45f, centre.y - jackR * 0.45f, jackR * 0.9f, jackR * 0.9f);
            }
            else
            {
                g.setColour (colours::edge);
                g.drawEllipse (centre.x - jackR, centre.y - jackR, jackR * 2, jackR * 2, 1.0f);
            }
        }

    auto footer = getLocalBounds().reduced (10, 6).removeFromBottom (22);
    g.setColour (colours::dimText);
    g.setFont (uiFont (11.0f, false));
    juce::String hint = "Click a jack to patch an output (row) into an input (column). Drag up/down to set the level "
                        "(amber +, teal -). Double-click to unpatch.";
    if (hoverDest >= 0 && hoverSource >= 0)
    {
        const float a = processor.getPatch (hoverDest, hoverSource);
        hint = juce::String (grind::sourceNames[(size_t) hoverSource]) + "  >  " + grind::destinationNames[(size_t) hoverDest]
             + (grind::isPatched (a) ? "   " + juce::String (juce::roundToInt (a * 100.0f)) + "%" : juce::String ("   (not patched)"));
    }
    g.drawText (hint, footer.withTrimmedRight (100), juce::Justification::centredLeft);
}

void PatchBayView::mouseDown (const juce::MouseEvent& e)
{
    int d, s;
    if (! cellAt (e.getPosition(), d, s))
        return;
    if (e.mods.isPopupMenu())
    {
        processor.setPatch (d, s, 0.0f);
        repaint();
        return;
    }
    dragDest = d;
    dragSource = s;
    dragStartAmount = processor.getPatch (d, s);
    dragged = false;
}

void PatchBayView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragDest < 0)
        return;
    const int dy = -e.getDistanceFromDragStartY();
    if (! dragged && std::abs (dy) < 3)
        return;
    dragged = true;
    float amount = juce::jlimit (-1.0f, 1.0f, dragStartAmount + (float) dy / (e.mods.isShiftDown() ? 600.0f : 150.0f));
    if (std::abs (amount) < 0.01f)
        amount = 0.0f;
    processor.setPatch (dragDest, dragSource, amount);
    repaint();
}

void PatchBayView::mouseUp (const juce::MouseEvent&)
{
    if (dragDest >= 0 && ! dragged)
    {
        // Click: patch at full level, or unpatch
        processor.setPatch (dragDest, dragSource, grind::isPatched (dragStartAmount) ? 0.0f : 1.0f);
        repaint();
    }
    dragDest = dragSource = -1;
}

void PatchBayView::mouseDoubleClick (const juce::MouseEvent& e)
{
    int d, s;
    if (cellAt (e.getPosition(), d, s))
    {
        processor.setPatch (d, s, 0.0f);
        repaint();
    }
}

void PatchBayView::mouseMove (const juce::MouseEvent& e)
{
    int d = -1, s = -1;
    if (! cellAt (e.getPosition(), d, s))
        d = s = -1;
    if (d != hoverDest || s != hoverSource)
    {
        hoverDest = d;
        hoverSource = s;
        repaint();
    }
}

void PatchBayView::mouseExit (const juce::MouseEvent&)
{
    hoverDest = hoverSource = -1;
    repaint();
}

void PatchBayView::tick()
{
    const int n = processor.countPatches();
    if (n != shownCount)
    {
        shownCount = n;
        repaint();
    }
}

//==============================================================================
SettingsView::SettingsView (MidnightGrindProcessor& p)
    : processor (p),
      velocity (p.apvts, "velocity", "VELOCITY > VCA", colours::amber, Knob::small),
      fine (p.apvts, "fine", "FINE TUNE", colours::amber, Knob::small)
{
    for (auto* s : { &voicesSlider, &bendSlider })
    {
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 40, 20);
        s->setColour (juce::Slider::trackColourId, colours::amber);
        addAndMakeVisible (s);
    }
    voicesAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "voices", voicesSlider);
    bendAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "bendRange", bendSlider);

    priorityBox.addItemList ({ "Last note", "Lowest note", "Highest note" }, 1);
    juce::StringArray channels { "Omni" };
    for (int c = 1; c <= 16; ++c)
        channels.add ("Channel " + juce::String (c));
    channelBox.addItemList (channels, 1);
    bankBox.addItemList ({ "DX7 Bank 1", "DX7 Bank 2", "DX7 Bank 3" }, 1);
    priorityAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "priority", priorityBox);
    channelAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "midiChannel", channelBox);
    bankAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "dx7Bank", bankBox);

    for (auto* c : std::initializer_list<juce::Component*> { &priorityBox, &channelBox, &bankBox, &velocity, &fine })
        addAndMakeVisible (c);
}

void SettingsView::resized()
{
    auto b = getLocalBounds().reduced (20, 14);
    auto left = b.removeFromLeft (440);
    auto row = [&] (juce::Component& c)
    {
        auto r = left.removeFromTop (28);
        r.removeFromLeft (150);
        c.setBounds (r);
        left.removeFromTop (12);
    };
    row (voicesSlider);
    row (priorityBox);
    row (channelBox);
    row (bendSlider);
    row (bankBox);

    b.removeFromLeft (40);
    auto knobs = b.removeFromTop (80);
    velocity.setBounds (knobs.removeFromLeft (100));
    fine.setBounds (knobs.removeFromLeft (100));
}

void SettingsView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().reduced (20, 14);
    auto left = b.removeFromLeft (440);
    g.setColour (colours::text);
    g.setFont (uiFont (12.0f));
    for (auto* label : { "POLY CHAIN VOICES", "VOICE PRIORITY", "MIDI CHANNEL", "PITCH BEND (st)", "DX7 PATCH BANK" })
    {
        g.drawText (label, left.removeFromTop (28).removeFromLeft (145), juce::Justification::centredLeft);
        left.removeFromTop (12);
    }

    b.removeFromLeft (40);
    b.removeFromTop (90);
    g.setColour (colours::dimText);
    g.setFont (uiFont (11.5f, false));
    g.drawFittedText ("POLY CHAIN: the hardware links up to 16 units for polyphony. Here each voice is a complete "
                      "oscillator, ladder filter, VCA and envelope. Set voices to 1 for the mono synth with legato "
                      "and note priority.\n\n"
                      "The digital oscillator uses the Mutable Instruments Plaits engines (MIT licence, by Emilie "
                      "Gillet). The extended engines and the analog-modelled path are new.",
                      b.removeFromTop (110), juce::Justification::topLeft, 8);
}

//==============================================================================
MidnightGrindEditor::MidnightGrindEditor (MidnightGrindProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      playMode (p.apvts, "playMode", "", { "KEYS", "SEQ", "ARP" }, colours::amber, false),
      models (p.apvts), display (p),
      frequency (p.apvts, "frequency", "FREQUENCY", colours::amber, Knob::medium),
      harmonics (p.apvts, "harmonics", "HARMONICS", colours::amber, Knob::large),
      timbre (p.apvts, "timbre", "TIMBRE", colours::amber, Knob::large),
      morph (p.apvts, "morph", "MORPH", colours::amber, Knob::large),
      fmAmt (p.apvts, "fmAmt", "FM", colours::teal, Knob::small),
      timbreAmt (p.apvts, "timbreAmt", "TIMBRE CV", colours::teal, Knob::small),
      morphAmt (p.apvts, "morphAmt", "MORPH CV", colours::teal, Knob::small),
      oscDecay (p.apvts, "oscDecay", "DECAY", colours::amber, Knob::small),
      lpgColour (p.apvts, "lpgColour", "LPG COLOUR", colours::amber, Knob::small),
      mix (p.apvts, "mix", "OUT / AUX", colours::amber, Knob::small),
      vcoMod (p.apvts, "vcoModAmt", "VCO MOD", colours::orange, Knob::medium),
      vcoModSrc (p.apvts, "vcoModSrc", "SOURCE", { "EG", "LFO" }, colours::orange),
      vcoModDest (p.apvts, "vcoModDest", "DEST", { "FREQ", "HARM", "TIMB", "MORPH" }, colours::orange),
      cutoff (p.apvts, "cutoff", "CUTOFF", colours::orange, Knob::large),
      resonance (p.apvts, "resonance", "RESONANCE", colours::orange, Knob::medium),
      vcfMod (p.apvts, "vcfModAmt", "VCF MOD", colours::orange, Knob::medium),
      keyTrack (p.apvts, "keyTrack", "KB TRACK", colours::orange, Knob::small),
      drive (p.apvts, "drive", "DRIVE", colours::orange, Knob::small),
      vcfModSrc (p.apvts, "vcfModSrc", "SOURCE", { "EG", "LFO" }, colours::orange),
      vcfModPol (p.apvts, "vcfModPol", "POL", { "+", "-" }, colours::orange),
      vcfMode (p.apvts, "vcfMode", "MODE", { "LP", "HP" }, colours::orange),
      attack (p.apvts, "attack", "ATTACK", colours::teal, Knob::medium),
      decay (p.apvts, "decay", "DECAY", colours::teal, Knob::medium),
      sustain (p.apvts, "sustain", "SUSTAIN", { "OFF", "ON" }, colours::teal),
      vcaMode (p.apvts, "vcaMode", "VCA", { "EG", "ON" }, colours::teal),
      lfoRate (p.apvts, "lfoRate", "LFO RATE", colours::teal, Knob::medium),
      glide (p.apvts, "glide", "GLIDE", colours::amber, Knob::medium),
      volume (p.apvts, "volume", "VOLUME", colours::amber, Knob::medium),
      lfoWave (p.apvts, "lfoWave", "WAVE", { "TRI", "SQR" }, colours::teal),
      sequencer (p), arp (p), patchBay (p), settings (p)
{
    setLookAndFeel (&lnf);

    for (int i = 0; i < processor.getNumPrograms(); ++i)
        presetBox.addItem (processor.getProgramName (i), i + 1);
    presetBox.setTextWhenNothingSelected ("PRESET");
    presetBox.onChange = [this]
    {
        const int i = presetBox.getSelectedItemIndex();
        if (i >= 0 && i != processor.getCurrentProgram())
            processor.setCurrentProgram (i);
        shownProgram = processor.getCurrentProgram();
    };
    addAndMakeVisible (presetBox);
    addAndMakeVisible (playMode);

    lpgButton.setClickingTogglesState (true);
    lpgAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "lpgOn", lpgButton);
    lpgButton.setTooltip ("Plaits low-pass gate: plucks the oscillator on every note, shaped by DECAY and LPG COLOUR");

    for (auto* c : std::initializer_list<juce::Component*> {
             &models, &display, &frequency, &harmonics, &timbre, &morph, &fmAmt, &timbreAmt, &morphAmt,
             &oscDecay, &lpgColour, &mix, &lpgButton, &vcoMod, &vcoModSrc, &vcoModDest,
             &cutoff, &resonance, &vcfMod, &keyTrack, &drive, &vcfModSrc, &vcfModPol, &vcfMode,
             &attack, &decay, &sustain, &vcaMode, &lfoRate, &glide, &volume, &lfoWave,
             &tabs, &keyboard })
        addAndMakeVisible (c);

    tabs.addTab ("SEQUENCER", colours::panel, 0);
    tabs.addTab ("ARPEGGIATOR", colours::panel, 1);
    tabs.addTab ("PATCH BAY  22 x 12", colours::panel, 2);
    tabs.addTab ("SETTINGS", colours::panel, 3);
    addChildComponent (sequencer);
    addChildComponent (arp);
    addChildComponent (patchBay);
    addChildComponent (settings);
    tabs.onTabChanged = [this] (int i) { showTab (i); };
    tabs.setCurrentTabIndex (0);
    showTab (0);

    keyboard.setAvailableRange (24, 108);
    keyboard.setScrollButtonsVisible (false);
    keyboardState.addListener (this);

    setSize (editorWidth, editorHeight);
    timerCallback();
    startTimerHz (30);
}

MidnightGrindEditor::~MidnightGrindEditor()
{
    keyboardState.removeListener (this);
    setLookAndFeel (nullptr);
}

void MidnightGrindEditor::showTab (int index)
{
    sequencer.setVisible (index == 0);
    arp.setVisible (index == 1);
    patchBay.setVisible (index == 2);
    settings.setVisible (index == 3);
}

void MidnightGrindEditor::drawSection (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title,
                                       juce::Colour c) const
{
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r.toFloat(), 6.0f);
    g.setColour (colours::edge);
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 6.0f, 1.0f);
    g.setColour (c);
    g.fillRect (r.getX() + 10, r.getY() + 8, 3, 12);
    g.setColour (colours::text);
    g.setFont (uiFont (12.5f));
    g.drawText (title, r.getX() + 18, r.getY() + 5, r.getWidth() - 24, 18, juce::Justification::centredLeft);
}

void MidnightGrindEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    // Top bar
    g.setColour (colours::panel);
    g.fillRect (topBar);
    g.setColour (colours::edge);
    g.drawHorizontalLine (topBar.getBottom(), 0.0f, (float) getWidth());
    g.setColour (colours::amber);
    g.setFont (uiFont (24.0f));
    g.drawText ("MIDNIGHT", 16, topBar.getY(), 130, topBar.getHeight(), juce::Justification::centredLeft);
    g.setColour (colours::text);
    g.drawText ("GRIND", 136, topBar.getY(), 100, topBar.getHeight(), juce::Justification::centredLeft);
    g.setColour (colours::dimText);
    g.setFont (uiFont (11.0f, false));
    g.drawText ("hybrid semi-modular  |  24 engines  |  24 dB ladder", 222, topBar.getY(), 330, topBar.getHeight(),
                juce::Justification::centredLeft);
    g.setColour (colours::text);
    g.setFont (uiFont (11.0f));
    g.drawText ("PLAY MODE", playMode.getX() - 72, topBar.getY(), 68, topBar.getHeight(), juce::Justification::centredRight);

    drawSection (g, oscArea, "DIGITAL OSCILLATOR", colours::amber);
    drawSection (g, vcoArea, "VCO MOD", colours::orange);
    drawSection (g, vcfArea, "24 dB LADDER VCF", colours::orange);
    drawSection (g, egArea, "EG / VCA", colours::teal);
    drawSection (g, lfoArea, "LFO / OUT", colours::teal);

    // LFO LED
    const auto ledPos = juce::Point<float> ((float) lfoRate.getRight() - 8.0f, (float) lfoRate.getBottom() - 10.0f);
    const float level = 0.5f + 0.5f * lfoLed;
    g.setColour (colours::red.withAlpha (0.15f + 0.85f * level));
    g.fillEllipse (ledPos.x - 5.0f, ledPos.y - 5.0f, 10.0f, 10.0f);
    g.setColour (colours::red.withAlpha (0.25f * level));
    g.fillEllipse (ledPos.x - 9.0f, ledPos.y - 9.0f, 18.0f, 18.0f);

    // Output meter
    auto m = juce::Rectangle<float> ((float) lfoArea.getRight() - 16.0f, (float) volume.getY() + 18.0f, 6.0f,
                                     (float) volume.getHeight() - 22.0f);
    g.setColour (juce::Colour (0xff07080b));
    g.fillRoundedRectangle (m, 2.0f);
    const float mh = m.getHeight() * juce::jlimit (0.0f, 1.0f, meter);
    g.setColour (meter > 0.95f ? colours::red : colours::teal);
    g.fillRoundedRectangle (m.withTop (m.getBottom() - mh), 2.0f);

    // Bottom panel
    g.setColour (colours::panel);
    g.fillRoundedRectangle (bottomArea.toFloat(), 6.0f);
    g.setColour (colours::edge);
    g.drawRoundedRectangle (bottomArea.toFloat().reduced (0.5f), 6.0f, 1.0f);
}

void MidnightGrindEditor::resized()
{
    auto b = getLocalBounds();
    topBar = b.removeFromTop (48);
    {
        auto t = topBar.reduced (12, 9);
        presetBox.setBounds (t.removeFromRight (220));
        t.removeFromRight (20);
        playMode.setBounds (t.removeFromRight (180));
    }

    b.reduce (10, 8);
    keyboard.setBounds (b.removeFromBottom (58));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 50.0f); // C1..C8: 50 white keys
    b.removeFromBottom (8);

    auto main = b.removeFromTop (400);
    b.removeFromTop (8);
    bottomArea = b;
    tabs.setBounds (bottomArea.getX() + 6, bottomArea.getY() + 4, 560, 26);
    const auto content = bottomArea.withTrimmedTop (30);
    for (auto* c : std::initializer_list<juce::Component*> { &sequencer, &arp, &patchBay, &settings })
        c->setBounds (content);

    oscArea = main.removeFromLeft (590);
    main.removeFromLeft (8);
    vcoArea = main.removeFromLeft (110);
    main.removeFromLeft (8);
    vcfArea = main.removeFromLeft (248);
    main.removeFromLeft (8);
    egArea = main.removeFromLeft (152);
    main.removeFromLeft (8);
    lfoArea = main;

    //== Oscillator
    {
        auto r = oscArea.reduced (10).withTrimmedTop (20);
        auto left = r.removeFromLeft (250);
        display.setBounds (left.removeFromTop (100));
        left.removeFromTop (22);
        models.setBounds (left.removeFromTop (190));

        r.removeFromLeft (14);
        auto big = r.removeFromTop (128);
        const int bw = big.getWidth() / 3;
        harmonics.setBounds (big.removeFromLeft (bw));
        timbre.setBounds (big.removeFromLeft (bw));
        morph.setBounds (big);

        r.removeFromTop (6);
        auto row2 = r.removeFromTop (100);
        frequency.setBounds (row2.removeFromLeft (84));
        const int sw = row2.getWidth() / 3;
        fmAmt.setBounds (row2.removeFromLeft (sw).withSizeKeepingCentre (70, 76));
        timbreAmt.setBounds (row2.removeFromLeft (sw).withSizeKeepingCentre (70, 76));
        morphAmt.setBounds (row2.withSizeKeepingCentre (70, 76));

        r.removeFromTop (6);
        auto row3 = r.removeFromTop (80);
        const int w3 = row3.getWidth() / 4;
        oscDecay.setBounds (row3.removeFromLeft (w3).withSizeKeepingCentre (70, 76));
        lpgColour.setBounds (row3.removeFromLeft (w3).withSizeKeepingCentre (70, 76));
        lpgButton.setBounds (row3.removeFromLeft (w3).withSizeKeepingCentre (56, 26));
        mix.setBounds (row3.withSizeKeepingCentre (70, 76));
    }

    //== VCO mod
    {
        auto r = vcoArea.reduced (10).withTrimmedTop (22);
        vcoMod.setBounds (r.removeFromTop (92));
        r.removeFromTop (10);
        vcoModSrc.setBounds (r.removeFromTop (70).withSizeKeepingCentre (64, 70));
        r.removeFromTop (10);
        vcoModDest.setBounds (r.removeFromTop (120).withSizeKeepingCentre (64, 120));
    }

    //== VCF
    {
        auto r = vcfArea.reduced (10).withTrimmedTop (22);
        auto top = r.removeFromTop (128);
        cutoff.setBounds (top.removeFromLeft (130));
        auto sw = top.reduced (4, 4);
        const int w = sw.getWidth() / 2;
        vcfMode.setBounds (sw.removeFromLeft (w).reduced (4, 6));
        vcfModPol.setBounds (sw.reduced (4, 6));

        r.removeFromTop (6);
        auto mid = r.removeFromTop (100);
        resonance.setBounds (mid.removeFromLeft (mid.getWidth() / 3));
        vcfMod.setBounds (mid.removeFromLeft (mid.getWidth() / 2));
        vcfModSrc.setBounds (mid.reduced (8, 8));

        r.removeFromTop (6);
        auto low = r.removeFromTop (80);
        keyTrack.setBounds (low.removeFromLeft (low.getWidth() / 2).withSizeKeepingCentre (70, 76));
        drive.setBounds (low.withSizeKeepingCentre (70, 76));
    }

    //== EG / VCA
    {
        auto r = egArea.reduced (10).withTrimmedTop (22);
        auto knobs = r.removeFromTop (96);
        attack.setBounds (knobs.removeFromLeft (knobs.getWidth() / 2));
        decay.setBounds (knobs);
        r.removeFromTop (12);
        auto sw = r.removeFromTop (80);
        sustain.setBounds (sw.removeFromLeft (sw.getWidth() / 2).reduced (6, 0));
        vcaMode.setBounds (sw.reduced (6, 0));
    }

    //== LFO / glide / volume
    {
        auto r = lfoArea.reduced (10).withTrimmedTop (22);
        auto top = r.removeFromTop (92);
        lfoRate.setBounds (top.removeFromLeft (top.getWidth() - 44));
        lfoWave.setBounds (top.withSizeKeepingCentre (42, 64));
        r.removeFromTop (6);
        glide.setBounds (r.removeFromTop (92).withTrimmedRight (30));
        r.removeFromTop (6);
        volume.setBounds (r.removeFromTop (92).withTrimmedRight (30));
    }
}

void MidnightGrindEditor::timerCallback()
{
    display.tick();
    if (sequencer.isVisible()) sequencer.tick();
    if (arp.isVisible()) arp.tick();
    if (patchBay.isVisible()) patchBay.tick();

    // Engine-specific meaning of the macro knobs
    const int m = models.getModel();
    if (juce::isPositiveAndBelow (m, grind::numEngines))
    {
        const auto& info = grind::engineInfo[(size_t) m];
        harmonics.setSubLabel (info.harmonics);
        timbre.setSubLabel (info.timbre);
        morph.setSubLabel (info.morph);
    }

    if (processor.getCurrentProgram() != shownProgram)
    {
        shownProgram = processor.getCurrentProgram();
        presetBox.setSelectedItemIndex (shownProgram, juce::dontSendNotification);
    }

    const float newLed = processor.getLfoLevel();
    const float newMeter = processor.getOutputLevel();
    if (std::abs (newLed - lfoLed) > 0.02f || std::abs (newMeter - meter) > 0.01f)
    {
        lfoLed = newLed;
        meter = newMeter;
        repaint (lfoArea);
    }
}
