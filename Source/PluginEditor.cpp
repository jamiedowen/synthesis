#include "PluginEditor.h"

using namespace me::ui;

namespace
{
juce::Font uiFont (float height, bool bold = true)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}
} // namespace

//==============================================================================
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, colours::panel);
    setColour (juce::ComboBox::textColourId, colours::text);
    setColour (juce::ComboBox::outlineColourId, colours::panelEdge);
    setColour (juce::ComboBox::arrowColourId, colours::cyan);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff12052a));
    setColour (juce::PopupMenu::textColourId, colours::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::pink.withAlpha (0.35f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, colours::panel);
    setColour (juce::TextButton::buttonOnColourId, colours::pink);
    setColour (juce::TextButton::textColourOffId, colours::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xee12052a));
    setColour (juce::BubbleComponent::outlineColourId, colours::cyan);
    setColour (juce::TooltipWindow::textColourId, colours::text);
}

void NeonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& slider)
{
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3.0f);
    const float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float arcRadius = radius - 2.5f;
    const float thickness = std::max (2.0f, radius * 0.14f);

    // Knob body
    const float bodyR = radius * 0.68f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c1752), centre.x, centre.y - bodyR,
                                             juce::Colour (0xff120626), centre.x, centre.y + bodyR, false));
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (accent.withAlpha (0.25f));
    g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    // Track
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2a1748));
    g.strokePath (track, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc (bipolar parameters draw from the centre)
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    if (std::abs (angle - from) > 0.01f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, std::min (from, angle), std::max (from, angle), true);
        g.setColour (accent.withAlpha (0.22f));
        g.strokePath (value, juce::PathStrokeType (thickness * 2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (value, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Pointer
    const auto tip = centre.getPointOnCircumference (bodyR * 0.85f, angle);
    const auto base = centre.getPointOnCircumference (bodyR * 0.25f, angle);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawLine ({ base, tip }, std::max (1.5f, radius * 0.08f));
}

void NeonLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                            bool over, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = button.getToggleState();
    const auto accent = button.findColour (juce::TextButton::buttonOnColourId);

    if (on)
    {
        g.setColour (accent.withAlpha (0.25f));
        g.fillRoundedRectangle (bounds.expanded (1.5f), 5.0f);
        g.setColour (accent.withAlpha (down ? 1.0f : 0.85f));
        g.fillRoundedRectangle (bounds, 4.0f);
    }
    else
    {
        g.setColour (colours::panel.brighter (over ? 0.25f : 0.0f).brighter (down ? 0.2f : 0.0f));
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (over ? accent.withAlpha (0.8f) : colours::panelEdge);
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
    }
}

void NeonLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    g.setFont (getTextButtonFont (button, button.getHeight()));
    g.setColour (button.findColour (button.getToggleState() ? juce::TextButton::textColourOnId
                                                             : juce::TextButton::textColourOffId));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (4, 2), juce::Justification::centred, 1, 0.8f);
}

juce::Font NeonLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return uiFont (std::min (13.0f, (float) buttonHeight * 0.55f));
}

void NeonLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (box.isMouseOver (true) ? colours::cyan.withAlpha (0.8f) : colours::panelEdge);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    juce::Path arrow;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

juce::Font NeonLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return uiFont (std::min (14.0f, (float) box.getHeight() * 0.55f));
}

juce::Font NeonLookAndFeel::getPopupMenuFont()
{
    return uiFont (14.0f, false);
}

juce::Label* NeonLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (uiFont (12.0f));
    return label;
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& labelText,
            juce::Colour colour, juce::Component* popupParent)
    : label (labelText)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    slider.setPopupDisplayEnabled (true, true, popupParent, 1500);
    slider.setMouseDragSensitivity (220);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);

    if (auto* param = state.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
}

void Knob::resized()
{
    auto b = getLocalBounds();
    if (b.getHeight() < 44) // compact: label on the left
        slider.setBounds (b.removeFromRight (b.getHeight()));
    else
        slider.setBounds (b.withTrimmedBottom (13));
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (colours::dimText);
    if (getHeight() < 44)
    {
        g.setFont (uiFont (11.0f));
        g.drawText (label, getLocalBounds().withTrimmedRight (getHeight() + 4), juce::Justification::centredRight);
    }
    else
    {
        g.setFont (uiFont (10.0f));
        g.drawText (label, getLocalBounds().removeFromBottom (13), juce::Justification::centred);
    }
}

//==============================================================================
VoiceStrip::VoiceStrip (MidnightEdgeProcessor& p, int v, juce::Component* popupParent)
    : processor (p), voice (v), colour (juce::Colour (me::voiceInfo[(size_t) v].colour))
{
    const auto& info = me::voiceInfo[(size_t) v];

    nameButton.setButtonText (info.name);
    nameButton.setTooltip ("Click to audition");
    nameButton.setColour (juce::TextButton::textColourOffId, colour);
    nameButton.setColour (juce::TextButton::buttonOnColourId, colour);
    nameButton.onClick = [this] { processor.audition (voice); };
    addAndMakeVisible (nameButton);

    loadButton.setButtonText ("+ SAMPLE");
    loadButton.setColour (juce::TextButton::buttonOnColourId, colour);
    loadButton.setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    loadButton.setTooltip ("Load a sample to layer (drag & drop works too). Right-click to clear.");
    loadButton.onClick = [this]
    {
        if (loadButton.wasPopupClick)
            processor.clearSample (voice);
        else
            chooseSample();
    };
    addAndMakeVisible (loadButton);

    const std::array<std::pair<const char*, juce::String>, 9> layout {{
        { "tune", info.knobNames[0] }, { "decay", info.knobNames[1] }, { "tone", info.knobNames[2] },
        { "snap", info.knobNames[3] }, { "drive", "DRIVE" },           { "verb", "G.VERB" },
        { "blend", "SAMPLE" },         { "pan", "PAN" },               { "level", "LEVEL" },
    }};

    for (const auto& [suffix, name] : layout)
        addAndMakeVisible (knobs.add (new Knob (processor.apvts, MidnightEdgeProcessor::voiceParamId (voice, suffix),
                                                name, colour, popupParent)));
}

void VoiceStrip::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    g.setGradientFill (juce::ColourGradient (colours::panel.brighter (0.08f), 0, 0, colours::panel.darker (0.3f),
                                             0, (float) getHeight(), false));
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (dragOver ? colour : colour.withAlpha (0.35f));
    g.drawRoundedRectangle (bounds, 6.0f, dragOver ? 2.0f : 1.0f);

    // Neon top edge + trigger light
    g.setColour (colour.withAlpha (0.5f + 0.5f * activity));
    g.fillRoundedRectangle (bounds.getX() + 8.0f, bounds.getY() + 1.0f, bounds.getWidth() - 16.0f, 2.0f, 1.0f);

    const auto led = juce::Rectangle<float> (bounds.getRight() - 15.0f, 12.0f, 7.0f, 7.0f);
    if (activity > 0.01f)
    {
        g.setColour (colour.withAlpha (0.35f * activity));
        g.fillEllipse (led.expanded (4.0f));
    }
    g.setColour (colour.withAlpha (0.2f + 0.8f * activity));
    g.fillEllipse (led);
}

void VoiceStrip::resized()
{
    auto b = getLocalBounds().reduced (6);
    nameButton.setBounds (b.removeFromTop (26).withTrimmedRight (16));
    b.removeFromTop (4);
    loadButton.setBounds (b.removeFromBottom (22));
    b.removeFromBottom (4);

    const int cellW = b.getWidth() / 3;
    const int cellH = b.getHeight() / 3;
    for (int i = 0; i < knobs.size(); ++i)
        knobs[i]->setBounds (b.getX() + (i % 3) * cellW, b.getY() + (i / 3) * cellH, cellW, cellH);
}

void VoiceStrip::tick()
{
    if (activity > 0.0f)
    {
        activity = std::max (0.0f, activity - 0.12f);
        repaint();
    }

    const auto name = processor.getSampleName (voice);
    if (name != shownSample)
    {
        shownSample = name;
        loadButton.setButtonText (name.isEmpty() ? "+ SAMPLE" : name);
        loadButton.setToggleState (name.isNotEmpty(), juce::dontSendNotification);
    }
}

void VoiceStrip::chooseSample()
{
    chooser = std::make_unique<juce::FileChooser> ("Load a sample for " + juce::String (me::voiceInfo[(size_t) voice].name),
                                                   juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe = juce::Component::SafePointer<VoiceStrip> (this)] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr)
                                  return;
                              const auto file = fc.getResult();
                              if (file.existsAsFile())
                                  safe->processor.loadSample (safe->voice, file);
                          });
}

bool VoiceStrip::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac;ogg"))
            return true;
    return false;
}

void VoiceStrip::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    for (const auto& f : files)
        if (processor.loadSample (voice, juce::File (f)))
            break;
    repaint();
}

//==============================================================================
StepGrid::StepGrid (MidnightEdgeProcessor& p) : processor (p) {}

juce::Rectangle<float> StepGrid::cellBounds (int voice, int step) const
{
    const float top = 14.0f;
    const float rowH = ((float) getHeight() - top) / (float) me::numVoices;
    const float cellW = ((float) getWidth() - (float) labelWidth) / (float) me::numSteps;
    return { (float) labelWidth + (float) step * cellW, top + (float) voice * rowH, cellW, rowH };
}

bool StepGrid::cellAt (juce::Point<int> pos, int& voice, int& step) const
{
    const float top = 14.0f;
    const float rowH = ((float) getHeight() - top) / (float) me::numVoices;
    const float cellW = ((float) getWidth() - (float) labelWidth) / (float) me::numSteps;
    voice = (int) std::floor (((float) pos.y - top) / rowH);
    step = pos.x < labelWidth ? -1 : (int) std::floor (((float) pos.x - (float) labelWidth) / cellW);
    return juce::isPositiveAndBelow (voice, me::numVoices) && step < me::numSteps;
}

void StepGrid::paint (juce::Graphics& g)
{
    const int pattern = processor.getCurrentPattern();
    const int current = processor.getCurrentStep();
    const int length = (int) processor.apvts.getRawParameterValue ("length")->load();

    // Step position LEDs
    for (int s = 0; s < me::numSteps; ++s)
    {
        const auto c = cellBounds (0, s);
        const auto led = juce::Rectangle<float> (c.getCentreX() - 6.0f, 2.0f, 12.0f, 5.0f);
        const bool lit = s == current;
        g.setColour (lit ? colours::pink : (s < length ? colours::panelEdge : colours::panelEdge.withAlpha (0.35f)));
        g.fillRoundedRectangle (led, 2.0f);
        if (lit)
        {
            g.setColour (colours::pink.withAlpha (0.3f));
            g.fillRoundedRectangle (led.expanded (3.0f), 4.0f);
        }
    }

    for (int v = 0; v < me::numVoices; ++v)
    {
        const auto colour = juce::Colour (me::voiceInfo[(size_t) v].colour);
        const auto row = cellBounds (v, 0);

        g.setColour (colour);
        g.setFont (uiFont (12.0f));
        g.drawText (me::voiceInfo[(size_t) v].name, juce::Rectangle<float> (4.0f, row.getY(), (float) labelWidth - 10.0f, row.getHeight()),
                    juce::Justification::centredLeft);

        for (int s = 0; s < me::numSteps; ++s)
        {
            const auto cell = cellBounds (v, s).reduced (2.5f, 3.0f);
            const auto state = processor.getStep (pattern, v, s);
            const bool inRange = s < length;
            const bool beatGroup = (s / 4) % 2 == 0;
            const float rangeAlpha = inRange ? 1.0f : 0.3f;

            if (state == MidnightEdgeProcessor::stepOff)
            {
                g.setColour ((beatGroup ? juce::Colour (0xff22113f) : juce::Colour (0xff1a0c32)).withMultipliedAlpha (rangeAlpha));
                g.fillRoundedRectangle (cell, 3.0f);
            }
            else
            {
                const bool accent = state == MidnightEdgeProcessor::stepAccent;
                if (accent && inRange)
                {
                    g.setColour (colour.withAlpha (0.3f));
                    g.fillRoundedRectangle (cell.expanded (2.0f), 4.0f);
                }
                g.setColour (colour.withAlpha ((accent ? 1.0f : 0.55f) * rangeAlpha));
                g.fillRoundedRectangle (cell, 3.0f);
                if (accent)
                {
                    g.setColour (juce::Colours::white.withAlpha (0.85f * rangeAlpha));
                    g.fillEllipse (cell.withSizeKeepingCentre (5.0f, 5.0f));
                }
            }

            if (s == current)
            {
                g.setColour (juce::Colours::white.withAlpha (0.14f));
                g.fillRoundedRectangle (cell, 3.0f);
            }
        }
    }
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    int v, s;
    lastVoice = lastStep = -1;
    if (! cellAt (e.getPosition(), v, s))
        return;

    if (s < 0)
    {
        processor.audition (v);
        return;
    }

    const int pattern = processor.getCurrentPattern();
    const auto current = processor.getStep (pattern, v, s);
    if (e.mods.isPopupMenu() || e.mods.isShiftDown())
        dragValue = current == MidnightEdgeProcessor::stepAccent ? MidnightEdgeProcessor::stepOn
                                                                 : MidnightEdgeProcessor::stepAccent;
    else
        dragValue = current == MidnightEdgeProcessor::stepOff ? MidnightEdgeProcessor::stepOn
                                                              : MidnightEdgeProcessor::stepOff;

    processor.setStep (pattern, v, s, dragValue);
    lastVoice = v;
    lastStep = s;
    repaint();
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    int v, s;
    if (lastVoice < 0 || ! cellAt (e.getPosition(), v, s) || s < 0 || (v == lastVoice && s == lastStep))
        return;

    processor.setStep (processor.getCurrentPattern(), v, s, dragValue);
    lastVoice = v;
    lastStep = s;
    repaint();
}

//==============================================================================
void MidnightEdgeEditor::Content::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff160333), 0, 0, juce::Colour (0xff05000d), 0, h, false));
    g.fillAll();

    // Retro perspective grid fading in behind the sequencer
    {
        const float horizon = 360.0f;
        g.setColour (colours::pink.withAlpha (0.07f));
        for (int i = -20; i <= 20; ++i)
        {
            const float xTop = w * 0.5f + (float) i * 40.0f;
            const float xBottom = w * 0.5f + (float) i * 160.0f;
            g.drawLine (xTop, horizon, xBottom, h, 1.0f);
        }
        for (int i = 1; i < 12; ++i)
        {
            const float y = horizon + (h - horizon) * std::pow ((float) i / 12.0f, 1.8f);
            g.drawHorizontalLine ((int) y, 0.0f, w);
        }
    }

    // Sun
    {
        const juce::Rectangle<float> sun (18.0f, 14.0f, 64.0f, 64.0f);
        g.setGradientFill (juce::ColourGradient (colours::yellow, sun.getCentreX(), sun.getY(),
                                                 colours::pink, sun.getCentreX(), sun.getBottom(), false));
        g.fillEllipse (sun);
        g.setColour (juce::Colour (0xff120330));
        for (int i = 0; i < 5; ++i)
        {
            const float y = sun.getCentreY() + 2.0f + (float) i * 6.5f;
            g.fillRect (sun.getX(), y, sun.getWidth(), 1.5f + (float) i * 0.7f);
        }
    }

    // Title
    {
        const juce::Rectangle<float> titleArea (94.0f, 16.0f, 260.0f, 36.0f);
        g.setFont (uiFont (31.0f).withExtraKerningFactor (0.06f));
        g.setColour (colours::pink.withAlpha (0.25f));
        for (auto [dx, dy] : { std::pair { -1.5f, 0.0f }, { 1.5f, 0.0f }, { 0.0f, -1.5f }, { 0.0f, 1.5f } })
            g.drawText ("MIDNIGHT", titleArea.translated (dx, dy), juce::Justification::left);
        g.setGradientFill (juce::ColourGradient (colours::yellow, 0, titleArea.getY(), colours::pink, 0, titleArea.getBottom(), false));
        g.drawText ("MIDNIGHT", titleArea, juce::Justification::left);

        g.setFont (uiFont (31.0f).withExtraKerningFactor (0.06f).italicised());
        g.setColour (colours::cyan);
        g.drawText ("EDGE", titleArea.translated (172.0f, 0.0f), juce::Justification::left);

        g.setFont (uiFont (10.5f).withExtraKerningFactor (0.25f));
        g.setColour (colours::dimText);
        g.drawText ("ANALOG DRUM SYNTHESIZER  //  1984", juce::Rectangle<float> (95.0f, 54.0f, 300.0f, 16.0f), juce::Justification::left);
    }

    // Header / sequencer separators
    g.setGradientFill (juce::ColourGradient (colours::pink.withAlpha (0.0f), 0, 0, colours::pink, w * 0.5f, 0, false));
    g.fillRect (0.0f, 93.0f, w * 0.5f, 1.5f);
    g.setGradientFill (juce::ColourGradient (colours::cyan, w * 0.5f, 0, colours::cyan.withAlpha (0.0f), w, 0, false));
    g.fillRect (w * 0.5f, 93.0f, w * 0.5f, 1.5f);

    const juce::Rectangle<float> seqPanel (12.0f, 362.0f, w - 24.0f, h - 374.0f);
    g.setColour (colours::panel.withAlpha (0.82f));
    g.fillRoundedRectangle (seqPanel, 6.0f);
    g.setColour (colours::panelEdge);
    g.drawRoundedRectangle (seqPanel, 6.0f, 1.0f);

    g.setColour (colours::dimText);
    g.setFont (uiFont (10.0f));
    g.drawText ("KIT", juce::Rectangle<float> (378.0f, 22.0f, 100.0f, 14.0f), juce::Justification::left);
    g.setFont (uiFont (11.0f));
    g.drawText ("LENGTH", juce::Rectangle<float> (566.0f, 372.0f, 54.0f, 26.0f), juce::Justification::centredRight);
    g.drawText ("RATE", juce::Rectangle<float> (690.0f, 372.0f, 44.0f, 26.0f), juce::Justification::centredRight);
}

//==============================================================================
MidnightEdgeEditor::MidnightEdgeEditor (MidnightEdgeProcessor& p)
    : AudioProcessorEditor (p), processor (p), grid (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);

    // Kit selector
    for (int i = 0; i < processor.getNumPrograms(); ++i)
        kitBox.addItem (processor.getProgramName (i), i + 1);
    kitBox.setSelectedId (processor.getCurrentProgram() + 1, juce::dontSendNotification);
    kitBox.onChange = [this]
    {
        if (kitBox.getSelectedId() > 0)
            processor.setCurrentProgram (kitBox.getSelectedId() - 1);
    };
    content.addAndMakeVisible (kitBox);

    // Master section
    const std::array<std::tuple<const char*, const char*, juce::Colour>, 8> masters {{
        { "vintage", "VINTAGE", colours::yellow }, { "heat", "HEAT", colours::orange },
        { "punch", "PUNCH", colours::pink },       { "verbSize", "V.SIZE", colours::cyan },
        { "verbGate", "V.GATE", colours::cyan },   { "verbTone", "V.TONE", colours::cyan },
        { "verbMix", "V.LEVEL", colours::cyan },   { "output", "OUTPUT", colours::text },
    }};
    for (const auto& [id, name, colour] : masters)
        content.addAndMakeVisible (masterKnobs.add (new Knob (processor.apvts, id, name, colour, &content)));

    for (int v = 0; v < me::numVoices; ++v)
        content.addAndMakeVisible (strips.add (new VoiceStrip (processor, v, &content)));

    // Sequencer controls
    seqButton.setClickingTogglesState (true);
    seqButton.setColour (juce::TextButton::buttonOnColourId, colours::pink);
    seqButton.setTooltip ("Run the step sequencer when the host is playing");
    seqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "seqOn", seqButton);
    content.addAndMakeVisible (seqButton);

    for (int i = 0; i < me::numPatterns; ++i)
    {
        auto* b = patternButtons.add (new juce::TextButton (juce::String::charToString ((juce::juce_wchar) ('A' + i))));
        b->setColour (juce::TextButton::buttonOnColourId, colours::cyan);
        b->setColour (juce::TextButton::textColourOnId, juce::Colours::black);
        b->onClick = [this, i] { selectPattern (i); };
        content.addAndMakeVisible (b);
    }

    copyButton.onClick = [this]
    {
        const int pattern = processor.getCurrentPattern();
        for (int v = 0; v < me::numVoices; ++v)
            for (int s = 0; s < me::numSteps; ++s)
                clipboard[(size_t) (v * me::numSteps + s)] = processor.getStep (pattern, v, s);
        hasClipboard = true;
    };
    pasteButton.onClick = [this]
    {
        if (! hasClipboard)
            return;
        const int pattern = processor.getCurrentPattern();
        for (int v = 0; v < me::numVoices; ++v)
            for (int s = 0; s < me::numSteps; ++s)
                processor.setStep (pattern, v, s, clipboard[(size_t) (v * me::numSteps + s)]);
        grid.repaint();
    };
    clearButton.onClick = [this]
    {
        processor.clearPattern (processor.getCurrentPattern());
        grid.repaint();
    };
    for (auto* b : { &copyButton, &pasteButton, &clearButton })
        content.addAndMakeVisible (b);

    for (int i = 1; i <= me::numSteps; ++i)
        lengthBox.addItem (juce::String (i), i);
    lengthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "length", lengthBox);
    content.addAndMakeVisible (lengthBox);

    rateBox.addItemList ({ "1/8", "1/16", "1/16T", "1/32" }, 1);
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "rate", rateBox);
    content.addAndMakeVisible (rateBox);

    seqKnobs.add (new Knob (processor.apvts, "swing", "SWING", colours::yellow, &content));
    seqKnobs.add (new Knob (processor.apvts, "accent", "ACCENT", colours::pink, &content));
    for (auto* k : seqKnobs)
        content.addAndMakeVisible (k);

    content.addAndMakeVisible (grid);

    layout();
    setResizable (true, true);
    setResizeLimits (designWidth / 2, designHeight / 2, designWidth * 2, designHeight * 2);
    getConstrainer()->setFixedAspectRatio ((double) designWidth / designHeight);
    setSize (designWidth, designHeight);

    startTimerHz (30);
}

MidnightEdgeEditor::~MidnightEdgeEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void MidnightEdgeEditor::paint (juce::Graphics& g)
{
    g.fillAll (me::ui::colours::background);
}

void MidnightEdgeEditor::resized()
{
    const float scale = (float) getWidth() / (float) designWidth;
    content.setTransform (juce::AffineTransform::scale (scale));
}

void MidnightEdgeEditor::layout()
{
    content.setBounds (0, 0, designWidth, designHeight);

    kitBox.setBounds (376, 40, 170, 28);

    for (int i = 0; i < masterKnobs.size(); ++i)
        masterKnobs[i]->setBounds (566 + i * 78, 10, 72, 76);

    for (int i = 0; i < strips.size(); ++i)
        strips[i]->setBounds (12 + i * 147, 102, 141, 252);

    const int barY = 372, barH = 26;
    seqButton.setBounds (26, barY, 54, barH);
    for (int i = 0; i < patternButtons.size(); ++i)
        patternButtons[i]->setBounds (92 + i * 32, barY, 28, barH);
    copyButton.setBounds (358, barY, 58, barH);
    pasteButton.setBounds (420, barY, 58, barH);
    clearButton.setBounds (482, barY, 58, barH);
    lengthBox.setBounds (626, barY, 60, barH);
    rateBox.setBounds (740, barY, 78, barH);
    seqKnobs[0]->setBounds (846, barY - 4, 120, 34);
    seqKnobs[1]->setBounds (980, barY - 4, 120, 34);

    grid.setBounds (24, 406, designWidth - 48, designHeight - 406 - 22);
}

void MidnightEdgeEditor::selectPattern (int index)
{
    if (auto* param = processor.apvts.getParameter ("pattern"))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) index));
        param->endChangeGesture();
    }
}

void MidnightEdgeEditor::timerCallback()
{
    const int pattern = processor.getCurrentPattern();
    for (int i = 0; i < patternButtons.size(); ++i)
        patternButtons[i]->setToggleState (i == pattern, juce::dontSendNotification);

    const auto mask = processor.takeTriggeredMask();
    for (int v = 0; v < strips.size(); ++v)
    {
        if ((mask & (1u << v)) != 0)
            strips[v]->flash();
        strips[v]->tick();
    }

    if (kitBox.getSelectedId() != processor.getCurrentProgram() + 1)
        kitBox.setSelectedId (processor.getCurrentProgram() + 1, juce::dontSendNotification);

    grid.repaint();
}
