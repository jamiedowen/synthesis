#include "EditorViews.h"

using namespace grind::ui;

namespace
{

juce::String noteName (int note)
{
    return juce::MidiMessage::getMidiNoteName (note, true, true, 4);
}

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
    g.drawText ("Tap: gate on/off   Drag: note   Wheel: note   Shift-drag: octave   Long-press / right-click: step menu",
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

void SequencerView::mouseUp (const juce::MouseEvent& e)
{
    if (dragStep >= 0 && ! dragged && e.getLengthOfMousePress() > 450)
    {
        // Long-press (touch screens): the step menu
        showStepMenu (dragStep);
    }
    else if (dragStep >= 0 && ! dragged)
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
    grind::ui::showStepMenu (processor, pattern(), s, *this, localAreaToGlobal (stepBounds (s)), [this]
    {
        shownPattern = -1;
        repaint();
    });
}

void grind::ui::showStepMenu (MidnightGrindProcessor& processor, int pattern, int s, juce::Component& target,
                              juce::Rectangle<int> screenArea, std::function<void()> onDone)
{
    auto st = processor.getPerformance().getStep (pattern, s);
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
    m.addItem (23, "Note up");
    m.addItem (24, "Note down");
    m.addItem (20, "Octave up");
    m.addItem (21, "Octave down");
    m.addItem (22, "Set pattern end here");

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&target).withTargetScreenArea (screenArea),
                     [&processor, pattern, s, st, onDone] (int result) mutable
                     {
                         if (result == 0) return;
                         if (result == 1) st.gate = ! st.gate;
                         if (result == 2) st.accent = ! st.accent;
                         if (result == 3) st.slide = ! st.slide;
                         if (result == 4) { st.tie = ! st.tie; if (st.tie) st.gate = true; }
                         if (result >= 11 && result <= 14) st.ratchet = result - 10;
                         if (result == 20) st.note = juce::jmin (127, st.note + 12);
                         if (result == 21) st.note = juce::jmax (0, st.note - 12);
                         if (result == 23) st.note = juce::jmin (127, st.note + 1);
                         if (result == 24) st.note = juce::jmax (0, st.note - 1);
                         if (result == 22)
                             processor.getPerformance().setLength (pattern, s + 1);
                         else
                             processor.getPerformance().setStep (pattern, s, st);
                         if (onDone) onDone();
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
SettingsView::SettingsView (MidnightGrindProcessor& p)
    : processor (p),
      oscDecay (p.apvts, "oscDecay", "OSC DECAY", colours::amber, Knob::small),
      lpgColour (p.apvts, "lpgColour", "LPG COLOUR", colours::amber, Knob::small),
      fine (p.apvts, "fine", "FINE TUNE", colours::amber, Knob::small),
      drive (p.apvts, "drive", "VCF DRIVE", colours::orange, Knob::small),
      keyTrack (p.apvts, "keyTrack", "KB TRACK", colours::orange, Knob::small),
      velocity (p.apvts, "velocity", "VELO > VCA", colours::teal, Knob::small),
      seqGate (p.apvts, "seqGate", "SEQ GATE", colours::teal, Knob::small),
      swing (p.apvts, "swing", "SWING", colours::teal, Knob::small),
      playMode (p.apvts, "playMode", "PLAY MODE", { "KEYS", "SEQ", "ARP" }, colours::amber, false),
      vcoSrc (p.apvts, "vcoModSrc", "OSC MOD SOURCE", { "ENV", "LFO" }, colours::orange, false),
      vcoDest (p.apvts, "vcoModDest", "OSC MOD DEST", { "FREQ", "HARM", "TIMB", "MORPH" }, colours::orange, false)
{
    for (auto* sl : { &voicesSlider, &bendSlider })
    {
        sl->setSliderStyle (juce::Slider::LinearHorizontal);
        sl->setTextBoxStyle (juce::Slider::TextBoxRight, false, 40, 20);
        sl->setColour (juce::Slider::trackColourId, colours::amber);
        addAndMakeVisible (sl);
    }
    voicesAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "voices", voicesSlider);
    bendAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "bendRange", bendSlider);

    priorityBox.addItemList ({ "Last note", "Lowest note", "Highest note" }, 1);
    juce::StringArray channels { "Omni" };
    for (int c = 1; c <= 16; ++c)
        channels.add ("Channel " + juce::String (c));
    channelBox.addItemList (channels, 1);
    bankBox.addItemList ({ "DX7 Bank 1", "DX7 Bank 2", "DX7 Bank 3" }, 1);
    for (auto* a : grind::assignNames)
        assignBox.addItem (a, assignBox.getNumItems() + 1);
    priorityAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "priority", priorityBox);
    channelAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "midiChannel", channelBox);
    bankAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "dx7Bank", bankBox);
    assignAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "assignSrc", assignBox);

    lpgButton.setClickingTogglesState (true);
    lpgAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "lpgOn", lpgButton);
    clearCables.onClick = [this] { processor.clearCables(); };

    for (auto* c : std::initializer_list<juce::Component*> { &priorityBox, &channelBox, &bankBox, &assignBox, &lpgButton,
                                                             &clearCables, &oscDecay, &lpgColour, &fine, &drive, &keyTrack,
                                                             &velocity, &seqGate, &swing, &playMode, &vcoSrc, &vcoDest })
        addAndMakeVisible (c);
}

void SettingsView::resized()
{
    auto b = getLocalBounds().reduced (20, 14);
    auto left = b.removeFromLeft (420);
    auto row = [&] (juce::Component& c)
    {
        auto r = left.removeFromTop (30);
        r.removeFromLeft (160);
        c.setBounds (r);
        left.removeFromTop (10);
    };
    row (voicesSlider);
    row (priorityBox);
    row (channelBox);
    row (bendSlider);
    row (bankBox);
    row (assignBox);
    left.removeFromTop (6);
    auto btns = left.removeFromTop (32);
    lpgButton.setBounds (btns.removeFromLeft (200));
    btns.removeFromLeft (12);
    clearCables.setBounds (btns);

    b.removeFromLeft (30);
    auto knobs1 = b.removeFromTop (90);
    for (auto* k : { &oscDecay, &lpgColour, &fine, &drive })
        k->setBounds (knobs1.removeFromLeft (knobs1.getWidth() / 4 + 1).withSizeKeepingCentre (90, 86));
    b.removeFromTop (10);
    auto knobs2 = b.removeFromTop (90);
    for (auto* k : { &keyTrack, &velocity, &seqGate, &swing })
        k->setBounds (knobs2.removeFromLeft (knobs2.getWidth() / 4 + 1).withSizeKeepingCentre (90, 86));
    b.removeFromTop (14);
    playMode.setBounds (b.removeFromTop (48).withWidth (300));
    b.removeFromTop (10);
    auto sw = b.removeFromTop (48);
    vcoSrc.setBounds (sw.removeFromLeft (180));
    sw.removeFromLeft (20);
    vcoDest.setBounds (sw.removeFromLeft (320));
}

void SettingsView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().reduced (20, 14);
    auto left = b.removeFromLeft (420);
    g.setColour (colours::text);
    g.setFont (uiFont (13.0f));
    for (auto* label : { "POLY CHAIN VOICES", "VOICE PRIORITY", "MIDI CHANNEL", "PITCH BEND (st)", "DX7 PATCH BANK",
                         "ASSIGN OUTPUT" })
    {
        g.drawText (label, left.removeFromTop (30).removeFromLeft (155), juce::Justification::centredLeft);
        left.removeFromTop (10);
    }
    left.removeFromTop (48);
    g.setColour (colours::dimText);
    g.setFont (uiFont (11.5f, false));
    g.drawFittedText ("PLAITS LPG plucks the oscillator on every note (shaped by OSC DECAY and LPG COLOUR). "
                      "POLY CHAIN: each voice is a full oscillator, filter, VCA and envelope. "
                      "Oscillator: Mutable Instruments Plaits engines (MIT licence, Emilie Gillet).",
                      left.removeFromTop (80), juce::Justification::topLeft, 5);
}
