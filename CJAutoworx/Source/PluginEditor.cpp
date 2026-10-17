#include "PluginEditor.h"
#include "BinaryData.h"

using namespace cj::ui;

namespace
{
juce::Font uiFont (float height, bool bold = true)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

juce::String withThousands (int value)
{
    const auto s = juce::String (value);
    return value >= 1000 ? s.dropLastCharacters (3) + "," + s.getLastCharacters (3) : s;
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, juce::Colour accent)
{
    g.setGradientFill (juce::ColourGradient (colours::panel.brighter (0.06f), 0, r.getY(), colours::panel.darker (0.35f),
                                             0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (colours::panelEdge);
    g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);

    // Racing stripe on the title
    g.setColour (accent);
    g.fillRoundedRectangle (r.getX() + 10.0f, r.getY() + 9.0f, 4.0f, 13.0f, 1.5f);
    g.setColour (accent.withAlpha (0.45f));
    g.fillRoundedRectangle (r.getX() + 16.0f, r.getY() + 9.0f, 2.0f, 13.0f, 1.0f);
    g.setColour (colours::text);
    g.setFont (uiFont (12.5f).withExtraKerningFactor (0.18f));
    g.drawText (title, r.withTrimmedLeft (24.0f).removeFromTop (31.0f), juce::Justification::centredLeft);
}
} // namespace

//==============================================================================
const juce::Image& cj::ui::jarradImage()
{
    static const juce::Image image = juce::ImageCache::getFromMemory (CJBinary::jarrad_png, CJBinary::jarrad_pngSize);
    return image;
}

void cj::ui::drawJarrad (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour ring, float glow)
{
    if (glow > 0.01f)
    {
        g.setColour (ring.withAlpha (0.35f * glow));
        g.fillEllipse (area.expanded (area.getWidth() * 0.09f));
    }

    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path clip;
        clip.addEllipse (area);
        g.reduceClipRegion (clip);
        g.setOpacity (1.0f);
        g.drawImage (jarradImage(), area, juce::RectanglePlacement::fillDestination);
    }

    const float t = std::max (2.0f, area.getWidth() * 0.045f);
    g.setGradientFill (juce::ColourGradient (colours::chrome, area.getX(), area.getY(), juce::Colour (0xff5d636e),
                                             area.getRight(), area.getBottom(), false));
    g.drawEllipse (area.expanded (t * 0.5f), t);
    g.setColour (ring);
    g.drawEllipse (area.expanded (t * 1.3f), std::max (1.5f, t * 0.6f));
}

//==============================================================================
GarageLookAndFeel::GarageLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, colours::panel.brighter (0.05f));
    setColour (juce::ComboBox::textColourId, colours::text);
    setColour (juce::ComboBox::outlineColourId, colours::panelEdge);
    setColour (juce::ComboBox::arrowColourId, colours::orange);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff17181c));
    setColour (juce::PopupMenu::textColourId, colours::text);
    setColour (juce::PopupMenu::headerTextColourId, colours::orange);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::orange.withAlpha (0.4f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, colours::panel);
    setColour (juce::TextButton::buttonOnColourId, colours::orange);
    setColour (juce::TextButton::textColourOffId, colours::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xee17181c));
    setColour (juce::BubbleComponent::outlineColourId, colours::orange);
    setColour (juce::TooltipWindow::textColourId, colours::text);
}

void GarageLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float startAngle, float endAngle, juce::Slider& slider)
{
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3.0f);
    const float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float arcRadius = radius - 2.5f;
    const float thickness = std::max (2.0f, radius * 0.12f);

    // Track and value arc (bipolar parameters draw from the centre)
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2a2c33));
    g.strokePath (track, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    if (std::abs (angle - from) > 0.01f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, std::min (from, angle), std::max (from, angle), true);
        g.setColour (accent.withAlpha (0.2f));
        g.strokePath (value, juce::PathStrokeType (thickness * 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (value, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Knurled machined-aluminium knob
    const float bodyR = radius * 0.7f;
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR + 2.0f, bodyR * 2.0f, bodyR * 2.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6b717c), centre.x, centre.y - bodyR,
                                             juce::Colour (0xff22252b), centre.x, centre.y + bodyR, false));
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (int i = 0; i < 28; ++i)
    {
        const float a = (float) i / 28.0f * cj::twoPi;
        g.drawLine ({ centre.getPointOnCircumference (bodyR * 0.86f, a), centre.getPointOnCircumference (bodyR, a) }, 1.0f);
    }
    const float capR = bodyR * 0.74f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4f58), centre.x - capR, centre.y - capR,
                                             juce::Colour (0xff1c1e23), centre.x + capR, centre.y + capR, false));
    g.fillEllipse (centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);

    // Pointer
    const auto tip = centre.getPointOnCircumference (bodyR * 0.9f, angle);
    const auto base = centre.getPointOnCircumference (bodyR * 0.3f, angle);
    g.setColour (accent.brighter (0.4f));
    g.drawLine ({ base, tip }, std::max (2.0f, radius * 0.09f));
}

void GarageLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                              bool over, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = button.getToggleState();
    const auto accent = button.findColour (juce::TextButton::buttonOnColourId);

    if (on)
    {
        g.setColour (accent.withAlpha (0.25f));
        g.fillRoundedRectangle (bounds.expanded (1.5f), 5.0f);
        g.setColour (accent.withAlpha (down ? 1.0f : 0.9f));
        g.fillRoundedRectangle (bounds, 4.0f);
    }
    else
    {
        g.setGradientFill (juce::ColourGradient (colours::panel.brighter (over ? 0.35f : 0.15f), 0, 0,
                                                 colours::panel.darker (down ? 0.0f : 0.3f), 0, bounds.getBottom(), false));
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (over ? accent.withAlpha (0.8f) : colours::panelEdge);
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
    }
}

void GarageLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    g.setFont (getTextButtonFont (button, button.getHeight()));
    g.setColour (button.findColour (button.getToggleState() ? juce::TextButton::textColourOnId
                                                             : juce::TextButton::textColourOffId)
                     .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.4f));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (4, 2), juce::Justification::centred, 2, 0.8f);
}

juce::Font GarageLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return uiFont (std::min (13.0f, (float) buttonHeight * 0.5f)).withExtraKerningFactor (0.06f);
}

void GarageLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (box.isMouseOver (true) ? colours::orange.withAlpha (0.8f) : colours::panelEdge);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    juce::Path arrow;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

juce::Font GarageLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return uiFont (std::min (14.0f, (float) box.getHeight() * 0.52f));
}

juce::Font GarageLookAndFeel::getPopupMenuFont()
{
    return uiFont (14.0f, false);
}

juce::Label* GarageLookAndFeel::createSliderTextBox (juce::Slider& slider)
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
    {
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        slider.setTooltip (param->getName (64));
    }
}

void Knob::resized()
{
    slider.setBounds (getLocalBounds().withTrimmedBottom (15));
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (colours::dimText);
    g.setFont (uiFont (10.5f).withExtraKerningFactor (0.08f));
    g.drawText (label, getLocalBounds().removeFromBottom (15), juce::Justification::centred);
}

//==============================================================================
Tachometer::Tachometer (CJAutoworxProcessor& p) : processor (p)
{
    setInterceptsMouseClicks (false, false);
}

void Tachometer::tick()
{
    const float target = processor.getRpm();
    rpm += (target - rpm) * 0.55f;
    redline = processor.getRedline();
    boost += (processor.getBoost() - boost) * 0.4f;
    throttle = processor.getThrottle();
    gear = processor.getGear();

    // The whole dash shakes a little with the engine
    const float amount = juce::jlimit (0.0f, 1.0f, rpm / std::max (1000.0f, redline));
    shake = { (random.nextFloat() - 0.5f) * 2.6f * amount, (random.nextFloat() - 0.5f) * 2.6f * amount };
    repaint();
}

void Tachometer::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float size = std::min (bounds.getWidth(), bounds.getHeight());
    const auto area = bounds.withSizeKeepingCentre (size, size).reduced (6.0f);
    const auto c = area.getCentre();
    const float R = area.getWidth() * 0.5f;
    const bool musical = processor.apvts.getRawParameterValue ("mode")->load() > 0.5f;

    // Bezel and face
    g.setGradientFill (juce::ColourGradient (colours::chrome, area.getX(), area.getY(), juce::Colour (0xff3a3e46),
                                             area.getRight(), area.getBottom(), false));
    g.fillEllipse (area);
    const auto faceArea = area.reduced (R * 0.06f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff23262d), c.x, c.y - R * 0.3f,
                                             juce::Colour (0xff07080a), c.x, c.y + R, true));
    g.fillEllipse (faceArea);

    const float maxRpm = std::ceil ((redline + 1000.0f) / 1000.0f) * 1000.0f;
    const float startA = -cj::pi * 0.75f, endA = cj::pi * 0.75f;
    auto angleFor = [&] (float r) { return startA + juce::jlimit (0.0f, 1.0f, r / maxRpm) * (endA - startA); };
    const float tickR = R * 0.84f;

    // Red zone
    {
        juce::Path red;
        red.addCentredArc (c.x, c.y, tickR - 6.0f, tickR - 6.0f, 0.0f, angleFor (redline), endA, true);
        g.setColour (colours::red.withAlpha (0.85f));
        g.strokePath (red, juce::PathStrokeType (R * 0.07f));
    }

    // Ticks and numbers (x1000)
    const int steps = (int) (maxRpm / 250.0f);
    for (int i = 0; i <= steps; ++i)
    {
        const float r = (float) i * 250.0f;
        const float a = angleFor (r);
        const bool major = i % 4 == 0;
        const bool half = i % 2 == 0;
        const float len = major ? R * 0.12f : (half ? R * 0.07f : R * 0.04f);
        g.setColour (r >= redline ? colours::red : colours::text.withAlpha (major ? 0.95f : 0.6f));
        g.drawLine ({ c.getPointOnCircumference (tickR, a), c.getPointOnCircumference (tickR - len, a) }, major ? 2.6f : 1.3f);

        if (major)
        {
            const auto p = c.getPointOnCircumference (tickR - R * 0.22f, a);
            g.setFont (uiFont (R * 0.11f));
            g.drawText (juce::String ((int) (r / 1000.0f)), juce::Rectangle<float> (R * 0.3f, R * 0.14f).withCentre (p),
                        juce::Justification::centred);
        }
    }

    g.setColour (colours::dimText);
    g.setFont (uiFont (R * 0.06f).withExtraKerningFactor (0.15f));
    g.drawText ("x1000 r/min", juce::Rectangle<float> (R, R * 0.1f).withCentre ({ c.x, c.y - R * 0.36f }), juce::Justification::centred);
    g.setColour (colours::orange);
    g.setFont (uiFont (R * 0.065f).withExtraKerningFactor (0.2f).italicised());
    g.drawText ("CJ AUTOWORX", juce::Rectangle<float> (R, R * 0.1f).withCentre ({ c.x, c.y - R * 0.47f }), juce::Justification::centred);

    // Shift lights
    {
        const float frac = rpm / std::max (1.0f, redline);
        const std::array<juce::Colour, 5> ledColours { colours::green, colours::green, colours::yellow, colours::yellow, colours::red };
        const bool flash = frac > 0.97f && (juce::Time::getMillisecondCounter() / 70) % 2 == 0;
        for (int i = 0; i < 5; ++i)
        {
            const bool lit = frac > 0.72f + 0.05f * (float) i;
            const auto led = juce::Rectangle<float> (R * 0.075f, R * 0.075f).withCentre ({ c.x + ((float) i - 2.0f) * R * 0.11f, c.y - R * 0.6f });
            const auto col = flash ? colours::red : ledColours[(size_t) i];
            if (lit || flash)
            {
                g.setColour (col.withAlpha (0.35f));
                g.fillEllipse (led.expanded (R * 0.025f));
            }
            g.setColour (lit || flash ? col : col.withAlpha (0.15f));
            g.fillEllipse (led);
        }
    }

    // Digital readouts: RPM, gear, boost
    {
        const auto rpmBox = juce::Rectangle<float> (R * 0.62f, R * 0.17f).withCentre ({ c.x, c.y + R * 0.42f });
        g.setColour (juce::Colour (0xff050607));
        g.fillRoundedRectangle (rpmBox, 3.0f);
        g.setColour (colours::orange);
        g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), R * 0.13f, juce::Font::bold)));
        g.drawText (withThousands ((int) (std::round (rpm / 10.0f) * 10.0f)), rpmBox, juce::Justification::centred);
        g.setColour (colours::dimText);
        g.setFont (uiFont (R * 0.055f).withExtraKerningFactor (0.15f));
        g.drawText ("RPM", rpmBox.translated (0, R * 0.13f), juce::Justification::centred);

        const auto gearBox = juce::Rectangle<float> (R * 0.2f, R * 0.22f).withCentre ({ c.x + R * 0.4f, c.y + R * 0.17f });
        g.setColour (juce::Colour (0xff050607));
        g.fillRoundedRectangle (gearBox, 3.0f);
        g.setColour (colours::yellow);
        g.setFont (uiFont (R * 0.17f));
        g.drawText (gear >= 0 ? juce::String (gear + 1) : (musical ? juce::String ("M") : juce::String ("N")),
                    gearBox, juce::Justification::centred);
        g.setColour (colours::dimText);
        g.setFont (uiFont (R * 0.05f).withExtraKerningFactor (0.15f));
        g.drawText ("GEAR", gearBox.translated (0, R * 0.16f), juce::Justification::centred);

        const auto boostBox = juce::Rectangle<float> (R * 0.2f, R * 0.22f).withCentre ({ c.x - R * 0.4f, c.y + R * 0.17f });
        g.setColour (juce::Colour (0xff050607));
        g.fillRoundedRectangle (boostBox, 3.0f);
        const auto fill = boostBox.reduced (3.0f);
        g.setColour (colours::teal.withAlpha (0.85f));
        g.fillRect (fill.withTrimmedTop (fill.getHeight() * (1.0f - juce::jlimit (0.0f, 1.0f, boost))));
        g.setColour (colours::text);
        g.setFont (uiFont (R * 0.075f));
        g.drawText (juce::String (juce::roundToInt (boost * 22.0f)), boostBox, juce::Justification::centred);
        g.setColour (colours::dimText);
        g.setFont (uiFont (R * 0.05f).withExtraKerningFactor (0.15f));
        g.drawText ("BOOST", boostBox.translated (0, R * 0.16f), juce::Justification::centred);
    }

    // Needle
    {
        const float a = angleFor (rpm);
        juce::Path needle;
        needle.startNewSubPath (-R * 0.025f, 0.0f);
        needle.lineTo (-R * 0.006f, -R * 0.8f);
        needle.lineTo (R * 0.006f, -R * 0.8f);
        needle.lineTo (R * 0.025f, 0.0f);
        needle.closeSubPath();
        needle.applyTransform (juce::AffineTransform::rotation (a).translated (c.x, c.y));
        g.setColour (colours::orange.withAlpha (0.25f));
        g.strokePath (needle, juce::PathStrokeType (R * 0.03f));
        g.setColour (colours::orange);
        g.fillPath (needle);
    }

    // Jarrad is the hub
    const float hubR = R * 0.2f;
    drawJarrad (g, juce::Rectangle<float> (hubR * 2.0f, hubR * 2.0f).withCentre (c + shake),
                rpm > redline * 0.97f ? colours::red : colours::orange, throttle);
}

//==============================================================================
CylinderLights::CylinderLights (CJAutoworxProcessor& p) : processor (p)
{
    setInterceptsMouseClicks (false, false);
}

void CylinderLights::tick()
{
    const auto mask = processor.takeFiredMask();
    for (int i = 0; i < cj::maxCylinders; ++i)
    {
        lights[(size_t) i] *= 0.6f;
        if ((mask & (1u << i)) != 0)
            lights[(size_t) i] = 1.0f;
    }
    bang = (mask & (1u << 31)) != 0 ? 1.0f : bang * 0.82f;
    repaint();
}

void CylinderLights::paint (juce::Graphics& g)
{
    const int layoutIndex = juce::jlimit (0, cj::numLayouts - 1, (int) processor.apvts.getRawParameterValue ("layout")->load());
    const auto& lay = cj::layouts()[(size_t) layoutIndex];
    const bool hidden = processor.isQuizPlaying();
    auto b = getLocalBounds().toFloat();

    g.setColour (colours::dimText);
    g.setFont (uiFont (11.0f).withExtraKerningFactor (0.15f));
    auto labelRow = b.removeFromTop (16.0f);
    g.drawText (hidden ? juce::String ("ENGINE: ???") : juce::String (lay.name).toUpperCase() + "  /  FIRING ORDER",
                labelRow, juce::Justification::centredLeft);
    if (bang > 0.02f && ! hidden)
    {
        g.setColour (colours::orange.withAlpha (bang));
        g.setFont (uiFont (14.0f).italicised());
        g.drawText ("BANG!", labelRow, juce::Justification::centredRight);
    }

    std::array<int, 2> perBank {};
    for (int i = 0; i < lay.numCylinders; ++i)
        ++perBank[lay.bank[(size_t) i]];
    const int rows = perBank[1] > 0 ? 2 : 1;
    const float rowH = b.getHeight() / (float) rows;
    const float slot = std::min (30.0f, b.getWidth() / (float) std::max (perBank[0], perBank[1]));

    std::array<int, 2> placed {};
    for (int i = 0; i < lay.numCylinders; ++i)
    {
        const int bank = lay.bank[(size_t) i];
        const float x = b.getX() + slot * (float) placed[(size_t) bank]++ + (bank == 1 ? slot * 0.5f : 0.0f);
        const auto cell = juce::Rectangle<float> (x, b.getY() + rowH * (float) bank, slot, rowH).reduced (3.0f, 2.0f);
        const float lit = hidden ? 0.0f : lights[(size_t) i];
        if (lit > 0.05f)
        {
            g.setColour (colours::orange.withAlpha (0.3f * lit));
            g.fillRoundedRectangle (cell.expanded (2.5f), 5.0f);
        }
        g.setColour (juce::Colour (0xff2a2c33).interpolatedWith (colours::orange, lit));
        g.fillRoundedRectangle (cell, 4.0f);
        g.setColour (lit > 0.5f ? juce::Colours::black : colours::dimText);
        g.setFont (uiFont (11.0f));
        g.drawText (hidden ? juce::String ("?") : juce::String (i + 1), cell, juce::Justification::centred);
    }
}

//==============================================================================
RevButton::RevButton (CJAutoworxProcessor& p) : processor (p)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip ("Press and hold Jarrad to rev the engine");
}

void RevButton::mouseDown (const juce::MouseEvent&)
{
    down = true;
    processor.setRevButton (true);
    repaint();
}

void RevButton::mouseUp (const juce::MouseEvent&)
{
    down = false;
    processor.setRevButton (false);
    repaint();
}

void RevButton::tick()
{
    const float target = processor.getThrottle();
    if (std::abs (target - glow) > 0.01f)
    {
        glow += (target - glow) * 0.5f;
        repaint();
    }
}

void RevButton::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (down ? colours::orange.withAlpha (0.18f) : colours::panel.brighter (0.05f));
    g.fillRoundedRectangle (b.reduced (0.5f), 8.0f);
    g.setColour (down ? colours::orange : colours::panelEdge);
    g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);

    const auto face = b.removeFromLeft (b.getHeight()).reduced (9.0f);
    drawJarrad (g, down ? face.reduced (2.0f) : face, colours::orange, glow);

    b.removeFromLeft (4.0f);
    g.setColour (colours::text);
    g.setFont (uiFont (15.0f).withExtraKerningFactor (0.08f));
    g.drawText (down ? "VROOOM!" : "HOLD TO REV", b.removeFromTop (b.getHeight() * 0.55f), juce::Justification::bottomLeft);
    g.setColour (colours::dimText);
    g.setFont (uiFont (11.0f, false));
    g.drawText ("press and hold Jarrad", b, juce::Justification::topLeft);
}

//==============================================================================
QuizPanel::QuizPanel (CJAutoworxProcessor& p) : processor (p)
{
    for (int i = 0; i < 4; ++i)
    {
        auto* b = options.add (new juce::TextButton());
        b->onClick = [this, i] { answer (i); };
        addAndMakeVisible (b);
    }

    replayButton.onClick = [this]
    {
        if (correct >= 0)
            processor.startQuiz (choices[(size_t) correct]);
    };
    nextButton.onClick = [this] { newRound(); };
    closeButton.onClick = [this] { if (onClose) onClose(); };
    nextButton.setColour (juce::TextButton::buttonOnColourId, colours::orange);
    for (auto* b : { &replayButton, &nextButton, &closeButton })
        addAndMakeVisible (b);
}

void QuizPanel::newRound()
{
    int engine;
    do
        engine = random.nextInt (cj::numEnginePresets);
    while (engine == lastEngine);
    lastEngine = engine;

    std::array<int, cj::numEnginePresets> pool {};
    for (int i = 0; i < cj::numEnginePresets; ++i)
        pool[(size_t) i] = i;
    for (int i = cj::numEnginePresets - 1; i > 0; --i)
        std::swap (pool[(size_t) i], pool[(size_t) random.nextInt (i + 1)]);

    choices[0] = engine;
    for (int i = 0, n = 1; n < 4; ++i)
        if (pool[(size_t) i] != engine)
            choices[(size_t) n++] = pool[(size_t) i];
    for (int i = 3; i > 0; --i)
        std::swap (choices[(size_t) i], choices[(size_t) random.nextInt (i + 1)]);
    for (int i = 0; i < 4; ++i)
        if (choices[(size_t) i] == engine)
            correct = i;

    for (int i = 0; i < 4; ++i)
    {
        auto* b = options[i];
        b->setButtonText (cj::factoryPresets()[(size_t) choices[(size_t) i]].name);
        b->setEnabled (true);
        b->setToggleState (false, juce::dontSendNotification);
        b->setColour (juce::TextButton::buttonOnColourId, colours::orange);
    }

    picked = -1;
    result = {};
    processor.startQuiz (engine);
    repaint();
}

void QuizPanel::answer (int choice)
{
    if (picked >= 0)
        return;
    picked = choice;
    ++rounds;

    const auto& p = cj::factoryPresets()[(size_t) choices[(size_t) correct]];
    if (choice == correct)
    {
        ++score;
        result = "Correct! It's the " + juce::String (p.name) + ". Jarrad would be proud.\n" + p.hint;
    }
    else
    {
        result = "Not quite. That was the " + juce::String (p.name) + ".\nJarrad says: " + p.hint;
    }

    for (int i = 0; i < 4; ++i)
    {
        auto* b = options[i];
        b->setColour (juce::TextButton::buttonOnColourId, i == correct ? colours::green : colours::red);
        b->setToggleState (i == correct || i == choice, juce::dontSendNotification);
    }
    repaint();
}

void QuizPanel::tick()
{
    replayButton.setEnabled (! processor.isQuizPlaying());
}

void QuizPanel::resized()
{
    auto b = getLocalBounds().reduced (24);
    b.removeFromLeft (250);
    b.removeFromTop (118);

    auto grid = b.removeFromTop (112);
    const int bw = (grid.getWidth() - 10) / 2;
    for (int i = 0; i < 4; ++i)
        options[i]->setBounds (grid.getX() + (i % 2) * (bw + 10), grid.getY() + (i / 2) * 56, bw, 46);

    auto bottom = getLocalBounds().reduced (24).removeFromBottom (38);
    bottom.removeFromLeft (250);
    replayButton.setBounds (bottom.removeFromLeft (150));
    bottom.removeFromLeft (10);
    nextButton.setBounds (bottom.removeFromLeft (150));
    closeButton.setBounds (bottom.removeFromRight (100));
}

void QuizPanel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xf20d0e11));
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (colours::orange);
    g.drawRoundedRectangle (bounds.reduced (1.0f), 8.0f, 2.0f);

    // Checkered flag strip
    for (int i = 0; i < (int) (bounds.getWidth() / 10.0f); ++i)
        for (int j = 0; j < 2; ++j)
            if ((i + j) % 2 == 0)
            {
                g.setColour (colours::text.withAlpha (0.12f));
                g.fillRect (bounds.getX() + (float) i * 10.0f, bounds.getBottom() - 22.0f + (float) j * 10.0f, 10.0f, 10.0f);
            }

    // Big Jarrad
    const auto inner = bounds.reduced (24.0f);
    const auto face = juce::Rectangle<float> (210.0f, 210.0f).withPosition (inner.getX() + 8.0f, inner.getY() + 30.0f);
    const bool playing = processor.isQuizPlaying();
    const float pulse = playing ? 0.5f + 0.5f * std::sin ((float) juce::Time::getMillisecondCounter() * 0.012f) : 0.0f;
    drawJarrad (g, face, picked < 0 ? colours::orange : (picked == correct ? colours::green : colours::yellow), pulse);

    g.setColour (colours::dimText);
    g.setFont (uiFont (12.0f).withExtraKerningFactor (0.12f));
    g.drawText ("JARRAD: " + juce::String (rounds) + " / " + juce::String (rounds), juce::Rectangle<float> (face.getX(), face.getBottom() + 18.0f, face.getWidth(), 16.0f),
                juce::Justification::centred);
    g.setColour (colours::yellow);
    g.drawText ("YOU: " + juce::String (score) + " / " + juce::String (rounds), juce::Rectangle<float> (face.getX(), face.getBottom() + 36.0f, face.getWidth(), 16.0f),
                juce::Justification::centred);

    auto text = inner.withTrimmedLeft (250.0f);
    g.setFont (uiFont (30.0f).withExtraKerningFactor (0.04f).italicised());
    g.setGradientFill (juce::ColourGradient (colours::yellow, 0, text.getY(), colours::orange, 0, text.getY() + 34.0f, false));
    g.drawText ("NAME THAT ENGINE", text.removeFromTop (38.0f), juce::Justification::centredLeft);
    g.setColour (colours::text);
    g.setFont (uiFont (13.5f, false));
    g.drawFittedText ("Jarrad can name any engine just by hearing it. Listen to it idle, blip and pull "
                      "to the limiter, then take your guess.",
                      text.removeFromTop (40.0f).toNearestInt(), juce::Justification::topLeft, 2);
    g.setColour (playing ? colours::orange : colours::dimText);
    g.setFont (uiFont (11.0f).withExtraKerningFactor (0.15f));
    g.drawText (playing ? "ENGINE RUNNING... LISTEN" : "WHAT WAS IT?", text.removeFromTop (30.0f), juce::Justification::centredLeft);

    text.removeFromTop (120.0f);
    if (result.isNotEmpty())
    {
        g.setColour (picked == correct ? colours::green : colours::yellow);
        g.setFont (uiFont (14.0f, false));
        g.drawFittedText (result, text.removeFromTop (70.0f).toNearestInt(), juce::Justification::topLeft, 4);
    }
}

//==============================================================================
void CJAutoworxEditor::Content::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();
    g.fillAll (colours::background);

    // Carbon fibre weave
    for (int y = 0; y < (int) h; y += 8)
        for (int x = ((y / 8) % 2) * 4; x < (int) w; x += 8)
        {
            g.setColour (juce::Colour (0xff15161a));
            g.fillRect ((float) x, (float) y, 4.0f, 4.0f);
        }

    // Header bar
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1f2127), 0, 0, juce::Colour (0xff101114), 0, 94.0f, false));
    g.fillRect (0.0f, 0.0f, w, 94.0f);

    // Logo: Jarrad in the CJ Autoworx badge
    drawJarrad (g, { 18.0f, 9.0f, 76.0f, 76.0f }, colours::orange, 0.4f);

    {
        const juce::Rectangle<float> titleArea (110.0f, 8.0f, 420.0f, 44.0f);
        g.setFont (uiFont (40.0f).withExtraKerningFactor (0.03f).italicised());
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawText ("CJ", titleArea.translated (2.0f, 2.0f), juce::Justification::left);
        g.setGradientFill (juce::ColourGradient (colours::yellow, 0, titleArea.getY(), colours::orange, 0, titleArea.getBottom(), false));
        g.drawText ("CJ", titleArea, juce::Justification::left);

        const auto rest = titleArea.withTrimmedLeft (60.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawText ("AUTOWORX", rest.translated (2.0f, 2.0f), juce::Justification::left);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white, 0, rest.getY() + 4.0f, juce::Colour (0xff7d838e), 0, rest.getBottom(), false));
        g.drawText ("AUTOWORX", rest, juce::Justification::left);

        g.setFont (uiFont (14.0f).withExtraKerningFactor (0.42f));
        g.setColour (colours::orange);
        g.drawText ("REVHEAD", juce::Rectangle<float> (112.0f, 54.0f, 120.0f, 16.0f), juce::Justification::left);
        g.setFont (uiFont (10.5f).withExtraKerningFactor (0.12f));
        g.setColour (colours::dimText);
        g.drawText ("ENGINE SYNTH  //  TUNED BY JARRAD'S EAR", juce::Rectangle<float> (222.0f, 55.0f, 300.0f, 16.0f), juce::Justification::left);
    }

    g.setColour (colours::dimText);
    g.setFont (uiFont (10.0f).withExtraKerningFactor (0.15f));
    g.drawText ("PRESET", juce::Rectangle<float> (538.0f, 2.0f, 100.0f, 14.0f), juce::Justification::left);
    g.drawText ("MODE", juce::Rectangle<float> (800.0f, 2.0f, 100.0f, 14.0f), juce::Justification::left);

    // Racing stripes under the header
    g.setColour (colours::orange);
    g.fillRect (0.0f, 94.0f, w, 3.0f);
    g.setColour (colours::yellow);
    g.fillRect (0.0f, 99.0f, w, 1.5f);

    // Panels
    drawPanel (g, { 14.0f, 110.0f, 388.0f, 414.0f }, "DASH", colours::orange);
    const bool musical = editor.processor.apvts.getRawParameterValue ("mode")->load() > 0.5f;
    drawPanel (g, { 412.0f, 110.0f, 430.0f, 160.0f }, "ENGINE", colours::orange);
    drawPanel (g, { 852.0f, 110.0f, 314.0f, 160.0f }, "COMBUSTION", colours::yellow);
    drawPanel (g, { 412.0f, 278.0f, 430.0f, 160.0f }, "EXHAUST", colours::red);
    drawPanel (g, { 852.0f, 278.0f, 314.0f, 160.0f }, "INDUCTION", colours::teal);
    drawPanel (g, { 412.0f, 446.0f, 754.0f, 160.0f }, musical ? "MUSICAL  //  NOTES SET THE FIRING FREQUENCY"
                                                               : "MUSICAL  //  SWITCH MODE TO MUSICAL TO PLAY TUNES",
               musical ? colours::teal : colours::dimText);

    g.setColour (colours::dimText);
    g.setFont (uiFont (10.5f, false));
    g.drawText (musical ? "Play notes: each note sets the engine's firing frequency."
                        : "Keys are the throttle: C2 is a gentle push, C5 and up is flat out.",
                juce::Rectangle<float> (14.0f, 610.0f, 600.0f, 14.0f), juce::Justification::left);
}

//==============================================================================
CJAutoworxEditor::CJAutoworxEditor (CJAutoworxProcessor& p)
    : AudioProcessorEditor (p), processor (p), tach (p), lights (p), revButton (p), quiz (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);

    // Presets, grouped into real engines and musical patches
    presetBox.addSectionHeading ("ENGINES");
    for (int i = 0; i < processor.getNumPrograms(); ++i)
    {
        if (i == cj::numEnginePresets)
            presetBox.addSectionHeading ("MUSICAL");
        presetBox.addItem (processor.getProgramName (i), i + 1);
    }
    presetBox.setSelectedId (processor.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        if (presetBox.getSelectedId() > 0)
            processor.setCurrentProgram (presetBox.getSelectedId() - 1);
    };
    content.addAndMakeVisible (presetBox);

    hintLabel.setFont (uiFont (11.5f, false).italicised());
    hintLabel.setColour (juce::Label::textColourId, colours::dimText);
    hintLabel.setJustificationType (juce::Justification::centredLeft);
    hintLabel.setMinimumHorizontalScale (0.8f);
    content.addAndMakeVisible (hintLabel);

    for (auto* b : { &engineModeButton, &musicalModeButton })
    {
        b->setRadioGroupId (1);
        content.addAndMakeVisible (b);
    }
    engineModeButton.setColour (juce::TextButton::buttonOnColourId, colours::orange);
    musicalModeButton.setColour (juce::TextButton::buttonOnColourId, colours::teal);
    engineModeButton.onClick = [this] { setMode (cj::engineMode); };
    musicalModeButton.onClick = [this] { setMode (cj::musicalMode); };

    ignitionButton.setClickingTogglesState (true);
    ignitionButton.setColour (juce::TextButton::buttonOnColourId, colours::red);
    ignitionButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    ignitionButton.setTooltip ("Start / stop the engine (ENGINE mode)");
    ignitionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "ignition", ignitionButton);
    content.addAndMakeVisible (ignitionButton);

    addKnob ("garage", "GARAGE", colours::teal);
    addKnob ("output", "OUTPUT", colours::chrome);

    // Engine
    layoutBox.addItemList ([]
    {
        juce::StringArray names;
        for (const auto& l : cj::layouts())
            names.add (l.name);
        return names;
    }(), 1);
    layoutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "layout", layoutBox);
    content.addAndMakeVisible (layoutBox);
    addKnob ("idle", "IDLE", colours::orange);
    addKnob ("redline", "REDLINE", colours::red);
    addKnob ("flywheel", "FLYWHEEL", colours::orange);
    addKnob ("throttle", "THROTTLE", colours::yellow);
    gearboxButton.setClickingTogglesState (true);
    gearboxButton.setTooltip ("Hold full throttle and it changes up through six gears");
    gearboxAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "gearbox", gearboxButton);
    content.addAndMakeVisible (gearboxButton);

    // Combustion, exhaust, induction
    addKnob ("pulse", "PULSE", colours::yellow);
    addKnob ("rough", "ROUGH", colours::yellow);
    addKnob ("rasp", "RASP", colours::yellow);
    addKnob ("mech", "MECH", colours::yellow);
    addKnob ("pipe", "PIPE", colours::red);
    addKnob ("reso", "RESO", colours::red);
    addKnob ("muffler", "MUFFLER", colours::red);
    addKnob ("crackle", "CRACKLE", colours::orange);
    addKnob ("width", "WIDTH", colours::red);
    addKnob ("intake", "INTAKE", colours::teal);
    addKnob ("turbo", "TURBO", colours::teal);
    addKnob ("blower", "BLOWER", colours::teal);

    // Musical
    addKnob ("attack", "ATTACK", colours::teal);
    addKnob ("decay", "DECAY", colours::teal);
    addKnob ("sustain", "SUSTAIN", colours::teal);
    addKnob ("release", "RELEASE", colours::teal);
    addKnob ("glide", "GLIDE", colours::teal);
    addKnob ("octave", "OCTAVE", colours::teal);
    addKnob ("revAmt", "REV-IN", colours::orange);
    addKnob ("revTime", "REV TIME", colours::orange);
    monoButton.setClickingTogglesState (true);
    monoButton.setColour (juce::TextButton::buttonOnColourId, colours::teal);
    monoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "mono", monoButton);
    content.addAndMakeVisible (monoButton);

    // Dash
    content.addAndMakeVisible (tach);
    content.addAndMakeVisible (lights);
    content.addAndMakeVisible (revButton);
    passByButton.setTooltip ("The car drives past you at 100 km/h");
    passByButton.onClick = [this] { processor.startPassBy(); };
    passByButton.setColour (juce::TextButton::buttonOnColourId, colours::yellow);
    quizButton.setTooltip ("Jarrad can name any engine by ear. Can you?");
    quizButton.setColour (juce::TextButton::buttonOnColourId, colours::orange);
    quizButton.onClick = [this] { showQuiz(); };
    content.addAndMakeVisible (passByButton);
    content.addAndMakeVisible (quizButton);

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffd9dce2));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff15161a));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff6b717c));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, colours::orange);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, colours::orange.withAlpha (0.3f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.4f));
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, colours::panel);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, colours::orange);
    keyboard.setAvailableRange (24, 96);
    keyboard.setOctaveForMiddleC (4);
    content.addAndMakeVisible (keyboard);

    quiz.onClose = [this] { quiz.setVisible (false); };
    content.addChildComponent (quiz);

    layout();
   #if JUCE_ANDROID || JUCE_IOS
    // Full screen on a phone or tablet: resized() fits the dashboard to whatever shape it gets
    setSize (designWidth, designHeight);
   #else
    setResizable (true, true);
    setResizeLimits (designWidth / 2, designHeight / 2, designWidth * 2, designHeight * 2);
    getConstrainer()->setFixedAspectRatio ((double) designWidth / designHeight);
    setSize (designWidth, designHeight);
   #endif

    startTimerHz (30);
}

CJAutoworxEditor::~CJAutoworxEditor()
{
    stopTimer();
    processor.setRevButton (false);
    setLookAndFeel (nullptr);
}

cj::ui::Knob* CJAutoworxEditor::addKnob (const char* id, const char* name, juce::Colour colour)
{
    auto* k = knobs.add (new Knob (processor.apvts, id, name, colour, &content));
    knobById[id] = k;
    content.addAndMakeVisible (k);
    return k;
}

void CJAutoworxEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);
}

void CJAutoworxEditor::resized()
{
    // Scale to fit and centre, so odd aspect ratios (phones) letterbox instead of cropping
    const float scale = std::min ((float) getWidth() / (float) designWidth, (float) getHeight() / (float) designHeight);
    content.setTransform (juce::AffineTransform::scale (scale).translated (((float) getWidth() - (float) designWidth * scale) * 0.5f,
                                                                          ((float) getHeight() - (float) designHeight * scale) * 0.5f));
}

void CJAutoworxEditor::layout()
{
    content.setBounds (0, 0, designWidth, designHeight);

    // Header
    presetBox.setBounds (536, 16, 250, 30);
    hintLabel.setBounds (532, 48, 258, 44);
    engineModeButton.setBounds (798, 16, 92, 30);
    musicalModeButton.setBounds (894, 16, 92, 30);
    ignitionButton.setBounds (798, 52, 188, 28);
    knobById["garage"]->setBounds (1004, 8, 76, 80);
    knobById["output"]->setBounds (1088, 8, 76, 80);

    // Dash
    tach.setBounds (34, 140, 348, 316);
    lights.setBounds (40, 460, 340, 58);
    revButton.setBounds (14, 532, 200, 74);
    passByButton.setBounds (222, 532, 180, 34);
    quizButton.setBounds (222, 572, 180, 34);

    auto row = [this] (std::initializer_list<const char*> ids, int x, int y, int w)
    {
        for (auto* id : ids)
        {
            knobById[id]->setBounds (x, y, w, 92);
            x += w;
        }
    };

    // Engine
    layoutBox.setBounds (568, 115, 170, 24);
    gearboxButton.setBounds (746, 115, 88, 24);
    row ({ "idle", "redline", "flywheel", "throttle" }, 430, 160, 98);
    row ({ "pulse", "rough", "rasp", "mech" }, 864, 160, 74);
    row ({ "pipe", "reso", "muffler", "crackle", "width" }, 422, 328, 82);
    row ({ "intake", "turbo", "blower" }, 872, 328, 96);
    row ({ "attack", "decay", "sustain", "release", "glide", "octave", "revAmt", "revTime" }, 424, 496, 82);
    monoButton.setBounds (1082, 115 + 336, 76, 24);

    keyboard.setBounds (14, 628, designWidth - 28, 82);
    keyboard.setKeyWidth ((float) (designWidth - 28) / 43.0f);
    keyboard.setLowestVisibleKey (24);

    quiz.setBounds (412, 110, 754, 496);
}

void CJAutoworxEditor::setMode (int newMode)
{
    if (auto* param = processor.apvts.getParameter ("mode"))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) newMode));
        param->endChangeGesture();
    }
}

void CJAutoworxEditor::showQuiz()
{
    quiz.setVisible (true);
    quiz.toFront (false);
    quiz.newRound();
}

void CJAutoworxEditor::timerCallback()
{
    refresh();
}

void CJAutoworxEditor::refresh()
{
    const int mode = (int) processor.apvts.getRawParameterValue ("mode")->load();
    if (mode != shownMode)
    {
        shownMode = mode;
        engineModeButton.setToggleState (mode == cj::engineMode, juce::dontSendNotification);
        musicalModeButton.setToggleState (mode == cj::musicalMode, juce::dontSendNotification);
        const float musicalAlpha = mode == cj::musicalMode ? 1.0f : 0.45f;
        for (auto* id : { "attack", "decay", "sustain", "release", "glide", "octave", "revAmt", "revTime" })
            knobById[id]->setAlpha (musicalAlpha);
        monoButton.setAlpha (musicalAlpha);
        for (auto* id : { "idle", "flywheel", "throttle" })
            knobById[id]->setAlpha (mode == cj::engineMode ? 1.0f : 0.45f);
        gearboxButton.setAlpha (mode == cj::engineMode ? 1.0f : 0.45f);
        ignitionButton.setAlpha (mode == cj::engineMode ? 1.0f : 0.45f);
        content.repaint();
    }

    const int preset = processor.getCurrentProgram();
    if (presetBox.getSelectedId() != preset + 1)
        presetBox.setSelectedId (preset + 1, juce::dontSendNotification);
    const juce::String hint ("\"" + juce::String (cj::factoryPresets()[(size_t) preset].hint) + "\"");
    if (hintLabel.getText() != hint)
        hintLabel.setText (hint, juce::dontSendNotification);

    passByButton.setToggleState (processor.isPassingBy(), juce::dontSendNotification);

    tach.tick();
    lights.tick();
    revButton.tick();
    if (quiz.isVisible())
    {
        quiz.tick();
        quiz.repaint();
    }
}
