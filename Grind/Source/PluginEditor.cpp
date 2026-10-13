#include "PluginEditor.h"

using namespace grind::ui;

namespace
{
constexpr float jackX0 = 436.0f, jackStep = 87.25f, jackRow1 = 140.0f, jackRow2 = 225.0f;
constexpr float bigKnob = 132.0f, smallKnob = 46.0f;

const juce::Colour cableColours[] = {
    juce::Colour (0xffe8343a), juce::Colour (0xfff2c12e), juce::Colour (0xff2f8fe8), juce::Colour (0xff38c95a),
    juce::Colour (0xffff7a1a), juce::Colour (0xffa45cf0), juce::Colour (0xffe0e0e0)
};

// Small glyphs for the model icons
void drawModelGlyph (juce::Graphics& g, juce::Rectangle<float> r, int engine)
{
    juce::Path p;
    const float x0 = r.getX() + 3.0f, x1 = r.getRight() - 3.0f, y0 = r.getY() + 4.0f, y1 = r.getBottom() - 4.0f;
    const float w = x1 - x0, h = y1 - y0, cy = (y0 + y1) * 0.5f;
    auto wave = [&] (std::function<float (float)> f, int points = 24)
    {
        for (int i = 0; i <= points; ++i)
        {
            const float t = (float) i / (float) points;
            const float y = cy - f (t) * h * 0.5f;
            if (i == 0) p.startNewSubPath (x0 + t * w, y); else p.lineTo (x0 + t * w, y);
        }
    };
    const float tau = juce::MathConstants<float>::twoPi;
    const float pi = juce::MathConstants<float>::pi;

    switch (engine % 24)
    {
        case 0:  wave ([] (float t) { return 1.0f - 2.0f * std::fmod (t * 2.0f, 1.0f); }); break;
        case 1:  wave ([tau] (float t) { return std::sin (tau * t) * std::sin (tau * t * 3.0f); }); break;
        case 2:  wave ([tau] (float t) { return std::sin (tau * t * 2.0f + 2.0f * std::sin (tau * t * 4.0f)); }, 40); break;
        case 3:  wave ([tau, pi] (float t) { return std::sin (tau * t * 6.0f) * std::sin (pi * t); }, 40); break;
        case 4:  for (int i = 0; i < 6; ++i) { const float x = x0 + w * (0.1f + 0.16f * (float) i); p.startNewSubPath (x, y1); p.lineTo (x, y1 - h * (1.0f - 0.14f * (float) i)); } break;
        case 5:  for (int i = 0; i < 3; ++i) { const float y = y0 + h * (0.2f + 0.3f * (float) i); p.startNewSubPath (x0, y); p.lineTo (x1, y); } break;
        case 6:  p.addRoundedRectangle (x0, y0, w, h * 0.7f, 3.0f); p.startNewSubPath (x0 + 3.0f, y0 + h * 0.7f); p.lineTo (x0 + 2.0f, y1); p.lineTo (x0 + 8.0f, y0 + h * 0.7f); break;
        case 7:  wave ([tau] (float t) { return std::sin (tau * t * 5.0f) * std::exp (-3.0f * t); }, 40); break;
        case 8:  for (int k = 0; k < 3; ++k) wave ([k] (float t) { return 0.8f - 1.6f * std::fmod (t * (2.0f + 0.3f * (float) k), 1.0f); }); break;
        case 9:  for (int i = 0; i < 3; ++i) { p.startNewSubPath (x0 + w * (float) i / 3.0f, y0); p.lineTo (x0 + w * (float) i / 3.0f, y1); p.startNewSubPath (x0, y0 + h * (float) i / 3.0f); p.lineTo (x1, y0 + h * (float) i / 3.0f); } break;
        case 10: for (int i = 0; i < 6; ++i) p.addEllipse (x0 + w * (0.1f + 0.15f * (float) i), y0 + h * (float) ((i * 37) % 5) / 5.0f, 2.2f, 3.5f); break;
        case 11: wave ([] (float t) { return (float) (((int) (t * 997.0f) * 7919) % 200) / 100.0f - 1.0f; }, 28); break;
        case 12: for (int i = 0; i < 5; ++i) p.addEllipse (x0 + w * (float) ((i * 53) % 10) / 10.0f, y0 + h * (float) ((i * 29) % 10) / 10.0f, 2.0f, 2.0f); break;
        case 13: p.addEllipse (x0 + w * 0.2f, y0, h, h); p.addEllipse (x0 + w * 0.2f + h * 0.25f, y0 + h * 0.25f, h * 0.5f, h * 0.5f); break;
        case 14: wave ([tau] (float t) { return std::sin (tau * t * 4.0f * (1.0f - 0.6f * t)) * (1.0f - t); }, 40); break;
        case 15: wave ([tau] (float t) { return std::sin (tau * t * 2.0f * (1.0f - 0.7f * t)) * (1.0f - t); }, 40); break;
        case 16: wave ([] (float t) { return (float) (((int) (t * 911.0f) * 104729) % 200) / 100.0f * (1.0f - t) - (1.0f - t); }, 28); break;
        case 17: for (int i = 0; i < 4; ++i) { const float x = x0 + w * (0.15f + 0.22f * (float) i); p.startNewSubPath (x, y0); p.lineTo (x + 3.0f, y1); } break;
        case 18: p.startNewSubPath (x0 + w * 0.3f, y0); p.lineTo (x0 + w * 0.7f, y0); p.lineTo (x1, y1); p.lineTo (x0, y1); p.closeSubPath(); break;
        case 19: p.addEllipse (x0, cy - h * 0.3f, w, h * 0.6f); p.startNewSubPath (x0, cy); p.lineTo (x0, y1); p.startNewSubPath (x1, cy); p.lineTo (x1, y1); break;
        case 20: for (int i = 0; i < 6; ++i) p.addRectangle (x0 + w * (float) (i % 3) / 3.0f + 1.0f, y0 + h * (float) (i / 3) / 2.0f + 1.0f, w / 3.0f - 2.0f, h / 2.0f - 2.0f); break;
        case 21: wave ([] (float t) { const float f = std::fmod (t * 3.0f, 1.0f); return f < 0.5f ? 0.8f - 3.2f * f : -0.8f; }); break;
        case 22: wave ([tau] (float t) { return t < 0.5f ? std::sin (tau * t * 2.0f) : (std::fmod (t * 4.0f, 1.0f) < 0.5f ? 0.8f : -0.8f); }, 40); break;
        default: p.addEllipse (x0 + w * 0.25f, y0, w * 0.5f, h * 0.6f); p.startNewSubPath (x0 + w * 0.5f, y0 + h * 0.6f); p.lineTo (x0 + w * 0.5f, y1); break;
    }
    g.strokePath (p, juce::PathStrokeType (1.4f));
}
} // namespace

//==============================================================================
void HardwareLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                            float startAngle, float endAngle, juce::Slider& slider)
{
    const bool small = slider.getProperties()["small"];
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat();
    const auto centre = bounds.getCentre();
    const float size = std::min (bounds.getWidth(), bounds.getHeight());
    const float body = small ? size * 0.42f : size * 0.31f;

    if (! small)
    {
        // Scale ticks, like the printed panel
        g.setColour (hw::label);
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
            const float r0 = size * 0.38f, r1 = size * (i == 0 || i == 5 || i == 10 ? 0.46f : 0.44f);
            g.drawLine (juce::Line<float> (centre.getPointOnCircumference (r0, a), centre.getPointOnCircumference (r1, a)), 2.6f);
        }
    }

    // Knob: shadow, skirt, cap
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (centre.x - body * 1.04f, centre.y - body * 0.98f, body * 2.08f, body * 2.12f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b3b3e), centre.x - body * 0.5f, centre.y - body,
                                             juce::Colour (0xff0b0b0c), centre.x + body * 0.4f, centre.y + body, true));
    g.fillEllipse (centre.x - body, centre.y - body, body * 2.0f, body * 2.0f);
    g.setColour (juce::Colour (0xff2c2c2f));
    g.drawEllipse (centre.x - body * 0.78f, centre.y - body * 0.78f, body * 1.56f, body * 1.56f, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawEllipse (centre.x - body, centre.y - body, body * 2.0f, body * 2.0f, 1.5f);

    // Pointer
    const float angle = startAngle + pos * (endAngle - startAngle);
    g.setColour (juce::Colours::white);
    g.drawLine (juce::Line<float> (centre.getPointOnCircumference (body * 0.35f, angle),
                                   centre.getPointOnCircumference (body * 0.98f, angle)), small ? 2.5f : 4.0f);
}

//==============================================================================
PanelKnob::PanelKnob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, bool small)
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    setPopupDisplayEnabled (true, true, nullptr);
    setMouseDragSensitivity (220);
    getProperties().set ("small", small);
    auto* param = state.getParameter (paramId);
    setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, *this);
    setTooltip (param->getName (64));
}

//==============================================================================
ToggleSwitch::ToggleSwitch (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, std::vector<int> v)
    : values (std::move (v))
{
    attachment = std::make_unique<juce::ParameterAttachment> (*state.getParameter (paramId), [this] (float value)
    {
        const int iv = juce::roundToInt (value);
        for (size_t i = 0; i < values.size(); ++i)
            if (values[i] == iv)
                position = (int) i;
        repaint();
    });
    attachment->sendInitialUpdate();
    setTooltip (state.getParameter (paramId)->getName (64));
}

void ToggleSwitch::paint (juce::Graphics& g)
{
    const auto c = getLocalBounds().toFloat().getCentre();
    // Hex nut and bushing
    g.setColour (juce::Colour (0xff9a9aa0));
    g.fillEllipse (c.x - 19.0f, c.y - 19.0f, 38.0f, 38.0f);
    g.setColour (juce::Colour (0xff5d5d63));
    g.drawEllipse (c.x - 19.0f, c.y - 19.0f, 38.0f, 38.0f, 2.0f);
    g.setColour (juce::Colour (0xff2b2b2e));
    g.fillEllipse (c.x - 10.0f, c.y - 10.0f, 20.0f, 20.0f);

    // Lever: tilted left / up / right
    const int n = (int) values.size();
    const float tilt = n == 3 ? (position == 0 ? -1.0f : position == 1 ? 0.0f : 1.0f)
                              : (position == 0 ? -1.0f : 1.0f);
    const auto tip = c + juce::Point<float> (tilt * 17.0f, std::abs (tilt) < 0.5f ? -16.0f : -4.0f);
    g.setColour (juce::Colour (0xffd8d8dc));
    g.drawLine (juce::Line<float> (c, tip), 7.0f);
    g.fillEllipse (tip.x - 6.5f, tip.y - 6.5f, 13.0f, 13.0f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (tip.x - 3.0f, tip.y - 4.0f, 5.0f, 5.0f);
}

void ToggleSwitch::mouseDown (const juce::MouseEvent& e)
{
    const int n = (int) values.size();
    const int newPos = n == 2 ? 1 - position : juce::jlimit (0, n - 1, e.x * n / std::max (1, getWidth()));
    attachment->setValueAsCompleteGesture ((float) values[(size_t) newPos]);
}

//==============================================================================
void PanelButton::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.5f);
    const float corner = style == small ? 3.0f : 5.0f;

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), corner);

    juce::Colour base = style == pad ? juce::Colour (0xff3a1010) : juce::Colour (0xff1b1b1d);
    if (lit > 0.0f)
        base = base.interpolatedWith (litColour, juce::jlimit (0.0f, 1.0f, lit));
    if (down)
        base = base.brighter (0.25f);

    if (lit > 0.3f)
    {
        g.setColour (litColour.withAlpha (0.25f * lit));
        g.fillRoundedRectangle (r.expanded (4.0f), corner + 3.0f);
    }
    g.setGradientFill (juce::ColourGradient (base.brighter (0.12f), r.getX(), r.getY(), base.darker (0.25f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (r, corner, 1.2f);

    if (text.isNotEmpty())
    {
        g.setColour (lit > 0.5f ? juce::Colours::black : hw::label.withAlpha (0.85f));
        g.setFont (uiFont (style == small ? 18.0f : 17.0f));
        g.drawFittedText (text, getLocalBounds().reduced (3), juce::Justification::centred, 2, 0.7f);
    }
}

void PanelButton::mouseDown (const juce::MouseEvent&)
{
    down = true;
    repaint();
    if (onPress) onPress();
}

void PanelButton::mouseUp (const juce::MouseEvent& e)
{
    down = false;
    repaint();
    if (onRelease) onRelease();
    if (! getLocalBounds().contains (e.getPosition()))
        return;
    if (onLongPress && (e.getLengthOfMousePress() > 450 || e.mods.isPopupMenu()))
        onLongPress();
    else if (onClick)
        onClick();
}

//==============================================================================
ModelDisplay::ModelDisplay (MidnightGrindProcessor& p) : processor (p) {}

juce::Rectangle<float> ModelDisplay::iconBounds (int row, int index) const
{
    const float w = (float) getWidth() / 10.0f;
    const float y = row == 0 ? 0.0f : (float) getHeight() - 24.0f;
    return { (float) index * w + 1.5f, y, w - 3.0f, 23.0f };
}

void ModelDisplay::paint (juce::Graphics& g)
{
    if (model < 0)
        return;
    const int bank = model / 10, index = model % 10;

    for (int row = 0; row < 2; ++row)
        for (int i = 0; i < 10; ++i)
        {
            const auto r = iconBounds (row, i);
            g.setColour (hw::label.withAlpha (row == bank ? 1.0f : 0.5f));
            g.drawRoundedRectangle (r, 2.5f, 1.5f);
            drawModelGlyph (g, r, row * 10 + i);
        }

    // LED row between the icon rows (green when bank C, the extended engines, is selected)
    const float ly = (float) getHeight() * 0.5f;
    for (int i = 0; i < 10; ++i)
    {
        const float cx = iconBounds (0, i).getCentreX();
        const bool on = i == index;
        const auto c = bank == 2 ? hw::ledGreen : hw::ledRed;
        if (on)
        {
            g.setColour (c.withAlpha (0.3f));
            g.fillEllipse (cx - 11.0f, ly - 11.0f, 22.0f, 22.0f);
        }
        g.setColour (on ? c : juce::Colour (0xff4a4a4f));
        g.fillEllipse (cx - 6.5f, ly - 6.5f, 13.0f, 13.0f);
    }
}

void ModelDisplay::mouseDown (const juce::MouseEvent& e)
{
    for (int row = 0; row < 2; ++row)
        for (int i = 0; i < 10; ++i)
            if (iconBounds (row, i).expanded (1.0f, 4.0f).contains (e.position))
            {
                auto* param = processor.apvts.getParameter ("model");
                param->beginChangeGesture();
                param->setValueNotifyingHost (param->convertTo0to1 ((float) (row * 10 + i)));
                param->endChangeGesture();
                return;
            }
}

void ModelDisplay::tick()
{
    const int m = (int) std::lround (processor.apvts.getRawParameterValue ("model")->load());
    if (m != model)
    {
        model = m;
        repaint();
    }
}

//==============================================================================
CableLayer::CableLayer (MidnightGrindProcessor& p) : processor (p) {}

juce::Point<float> CableLayer::jackCentre (int slot)
{
    const int row = slot / grind::jacksPerRow, col = slot % grind::jacksPerRow;
    return { jackX0 + (float) col * jackStep, row == 0 ? jackRow1 : jackRow2 };
}

int CableLayer::jackAt (juce::Point<float> p) const
{
    for (int s = 0; s < (int) grind::panelJacks.size(); ++s)
        if (jackCentre (s).getDistanceFrom (p) < 36.0f)
            return s;
    return -1;
}

bool CableLayer::hitTest (int x, int y)
{
    return dragging || jackAt ({ (float) x, (float) y }) >= 0;
}

void CableLayer::drawCable (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b, juce::Colour c) const
{
    const float sag = 30.0f + 0.2f * a.getDistanceFrom (b);
    juce::Path p;
    p.startNewSubPath (a);
    p.cubicTo (a + juce::Point<float> (0.0f, sag), b + juce::Point<float> (0.0f, sag), b);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.strokePath (p, juce::PathStrokeType (12.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                  juce::AffineTransform::translation (2.0f, 4.0f));
    g.setColour (c.darker (0.35f).withAlpha (0.92f));
    g.strokePath (p, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c.withAlpha (0.92f));
    g.strokePath (p, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    for (auto end : { a, b })
    {
        g.setColour (c.darker (0.2f));
        g.fillEllipse (end.x - 15.0f, end.y - 15.0f, 30.0f, 30.0f);
        g.setColour (juce::Colour (0xffcfcfd4));
        g.fillEllipse (end.x - 8.0f, end.y - 8.0f, 16.0f, 16.0f);
    }
}

namespace
{
int slotOf (bool isInput, int index)
{
    for (int s = 0; s < (int) grind::panelJacks.size(); ++s)
        if (grind::panelJacks[(size_t) s].isInput == isInput && grind::panelJacks[(size_t) s].index == index)
            return s;
    return 0;
}
} // namespace

void CableLayer::paint (juce::Graphics& g)
{
    // Jacks
    for (int s = 0; s < (int) grind::panelJacks.size(); ++s)
    {
        const auto c = jackCentre (s);
        juce::Path nut;
        nut.addPolygon (c, 6, 24.0f, juce::MathConstants<float>::pi / 6.0f);
        g.setColour (juce::Colour (0xffb9b9bf));
        g.fillPath (nut);
        g.setColour (juce::Colour (0xff6e6e75));
        g.strokePath (nut, juce::PathStrokeType (1.5f));
        g.setColour (juce::Colour (0xffe6e6ea));
        g.fillEllipse (c.x - 15.0f, c.y - 15.0f, 30.0f, 30.0f);
        g.setColour (juce::Colour (0xff09090a));
        g.fillEllipse (c.x - 10.0f, c.y - 10.0f, 20.0f, 20.0f);
    }

    // Cables, output -> input
    for (int i = 0; i < grind::numInputs; ++i)
    {
        const int o = processor.getCable (i);
        if (o < 0 || i == detachedInput)
            continue;
        drawCable (g, jackCentre (slotOf (false, o)), jackCentre (slotOf (true, i)), cableColours[(i * 3) % 7]);
    }

    if (dragging && moved)
        drawCable (g, jackCentre (dragFromSlot), dragPos,
                   cableColours[((detachedInput >= 0 ? detachedInput : dragFromSlot) * 3) % 7]);
}

void CableLayer::mouseDown (const juce::MouseEvent& e)
{
    const int s = jackAt (e.position);
    if (s < 0)
        return;
    dragging = true;
    moved = false;
    dragPos = e.position;
    detachedInput = -1;
    dragFromSlot = s;

    const auto& jack = grind::panelJacks[(size_t) s];
    if (jack.isInput && processor.getCable (jack.index) >= 0)
    {
        // Pick up the plug: the cable now hangs from its output
        detachedInput = jack.index;
        dragFromSlot = slotOf (false, processor.getCable (jack.index));
    }
}

void CableLayer::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;
    dragPos = e.position;
    if (e.getDistanceFromDragStart() > 6)
        moved = true;
    repaint();
}

void CableLayer::mouseUp (const juce::MouseEvent& e)
{
    if (! dragging)
        return;
    dragging = false;

    const int target = jackAt (e.position);
    const auto from = grind::panelJacks[(size_t) dragFromSlot];

    if (detachedInput >= 0)
    {
        // Moving an existing plug: drop it on another input, or anywhere else (or a tap) to unplug
        processor.setCable (detachedInput, -1);
        if (moved && target >= 0 && grind::panelJacks[(size_t) target].isInput)
            processor.setCable (grind::panelJacks[(size_t) target].index, from.index);
    }
    else if (moved && target >= 0 && target != dragFromSlot)
    {
        const auto to = grind::panelJacks[(size_t) target];
        if (from.isInput != to.isInput)
        {
            const int input = from.isInput ? from.index : to.index;
            const int output = from.isInput ? to.index : from.index;
            processor.setCable (input, output);
        }
    }

    detachedInput = -1;
    moved = false;
    repaint();
}

void CableLayer::tick()
{
    const auto v = processor.getCableVersion();
    if (v != shownVersion)
    {
        shownVersion = v;
        repaint();
    }
}

//==============================================================================
MenuOverlay::MenuOverlay (MidnightGrindProcessor& p) : sequencer (p), arp (p), settings (p)
{
    tabs.addTab ("SEQUENCER", colours::panel, 0);
    tabs.addTab ("ARPEGGIATOR", colours::panel, 1);
    tabs.addTab ("SETTINGS", colours::panel, 2);
    tabs.onTabChanged = [this] (int i) { showTab (i); };
    closeButton.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (tabs);
    addAndMakeVisible (closeButton);
    addChildComponent (sequencer);
    addChildComponent (arp);
    addChildComponent (settings);
    setSize ((int) logicalWidth, (int) logicalHeight);
    showTab (0);
}

void MenuOverlay::showTab (int index)
{
    if (tabs.getCurrentTabIndex() != index)
        tabs.setCurrentTabIndex (index, false);
    sequencer.setVisible (index == 0);
    arp.setVisible (index == 1);
    settings.setVisible (index == 2);
}

void MenuOverlay::resized()
{
    auto b = getLocalBounds().reduced (8);
    auto top = b.removeFromTop (30);
    closeButton.setBounds (top.removeFromRight (90));
    tabs.setBounds (top.withWidth (420));
    b.removeFromTop (4);
    for (auto* c : std::initializer_list<juce::Component*> { &sequencer, &arp, &settings })
        c->setBounds (b);
}

void MenuOverlay::paint (juce::Graphics& g)
{
    g.setColour (colours::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);
    g.setColour (hw::orange);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 8.0f, 2.0f);
}

void MenuOverlay::tick()
{
    if (sequencer.isVisible()) sequencer.tick();
    if (arp.isVisible()) arp.tick();
}

//==============================================================================
GrindPanel::GrindPanel (MidnightGrindProcessor& p)
    : processor (p),
      timbre (p.apvts, "timbre", false), timbreAmt (p.apvts, "timbreAmt", true),
      harmonics (p.apvts, "harmonics", false), fm (p.apvts, "fmAmt", true),
      frequency (p.apvts, "frequency", false), morph (p.apvts, "morph", false), morphAmt (p.apvts, "morphAmt", true),
      cutoff (p.apvts, "cutoff", false), resonance (p.apvts, "resonance", false), vcfMod (p.apvts, "vcfModAmt", false),
      volume (p.apvts, "volume", false),
      attack (p.apvts, "attack", false), decay (p.apvts, "decay", false), sustain (p.apvts, "sustain", false),
      oscMod (p.apvts, "vcoModAmt", false), lfoRate (p.apvts, "lfoRate", false), glide (p.apvts, "glide", false),
      vcMix (p.apvts, "mix", false), tempo (p.apvts, "tempo", false), swing (p.apvts, "swing", false),
      vcfMode (p.apvts, "vcfMode", { 0, 1 }), vcfModSrc (p.apvts, "vcfModSrc", { 0, 1 }),
      vcfModPol (p.apvts, "vcfModPol", { 0, 1 }), vcaMode (p.apvts, "vcaMode", { 0, 1, 2 }),
      lfoShape (p.apvts, "lfoWave", { 1, 0 }),
      modelDisplay (p), cables (p), menu (p)
{
    setLookAndFeel (&lnf);
    setSize ((int) panelWidth, (int) panelHeight);

    //== Top strip
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
    presetBox.setBounds (420, 14, 420, 44);
    addAndMakeVisible (presetBox);
    menuButton.setBounds (1700, 14, 180, 44);
    menuButton.onClick = [this] { showMenu (menu.isVisible() ? -1 : 0); };
    menuButton.setTooltip ("Sequencer grid, arpeggiator patterns and settings");
    addAndMakeVisible (menuButton);

    //== Knobs
    placeKnob (timbre, 190, 380, false);    placeKnob (timbreAmt, 190, 492, true);
    placeKnob (harmonics, 420, 380, false); placeKnob (fm, 535, 384, true);
    placeKnob (frequency, 650, 380, false);
    placeKnob (morph, 880, 380, false);     placeKnob (morphAmt, 880, 492, true);
    placeKnob (cutoff, 1118, 380, false);   placeKnob (resonance, 1348, 380, false);
    placeKnob (vcfMod, 1578, 380, false);   placeKnob (volume, 1808, 380, false);
    placeKnob (attack, 190, 640, false);    placeKnob (decay, 420, 640, false);
    placeKnob (sustain, 650, 640, false);   placeKnob (oscMod, 880, 640, false);
    placeKnob (lfoRate, 1118, 640, false);  placeKnob (glide, 1578, 640, false);
    placeKnob (vcMix, 1808, 640, false);
    placeKnob (tempo, 190, 856, false);     placeKnob (swing, 190, 856, false);
    swing.setVisible (false);

    //== Switches
    placeSwitch (vcfMode, 1118, 511);
    placeSwitch (vcfModSrc, 1348, 511);
    placeSwitch (vcfModPol, 1578, 511);
    placeSwitch (vcaMode, 1808, 511);
    placeSwitch (lfoShape, 1348, 643);

    //== Model section
    bankButton.setBounds (265, 455, 80, 72);
    bankButton.setLit (1.0f);
    bankButton.onClick = [this]
    {
        // A -> B -> C (extended) -> A, keeping the position in the row where possible
        const int m = (int) getParam ("model");
        const int bank = (m / 10 + 1) % 3, index = m % 10;
        setParam ("model", (float) std::min (bank * 10 + index, grind::numEngines - 1));
    };
    modelButton.setBounds (725, 455, 80, 72);
    modelButton.onClick = [this]
    {
        const int m = (int) getParam ("model");
        const int bank = m / 10, size = bank == 2 ? 4 : 10;
        setParam ("model", (float) (bank * 10 + (m % 10 + 1) % size));
    };
    modelDisplay.setBounds (412, 456, 252, 72);
    for (auto* c : std::initializer_list<juce::Component*> { &bankButton, &modelButton, &modelDisplay })
        addAndMakeVisible (c);

    //== Sequencer buttons
    auto button = [this] (PanelButton& b, int x, int y, int w = 76, int h = 34)
    {
        b.setBounds (x, y, w, h);
        addAndMakeVisible (b);
    };
    button (holdButton, 338, 790);  button (resetButton, 426, 790);
    button (arpButton, 513, 790);   button (patternButton, 600, 790);
    button (shiftButton, 338, 885); button (pageButton, 426, 885);
    button (playButton, 513, 885);  button (recButton, 600, 885);
    button (kybdButton, 782, 885);  button (stepButton, 870, 885);
    button (leftArrow, 732, 885, 44, 34); button (rightArrow, 952, 885, 44, 34);
    leftArrow.setText ("<");
    rightArrow.setText (">");

    holdButton.onClick = [this]
    {
        if (shift || getParam ("seqRec") > 0.5f)
        {
            // REST: write a rest at the record cursor
            auto& perf = processor.getPerformance();
            const int pat = processor.getCurrentPattern();
            const int cur = perf.getRecordCursor();
            grind::Step st = perf.getStep (pat, cur);
            st.gate = false;
            st.tie = false;
            perf.setStep (pat, cur, st);
            perf.setRecordCursor ((cur + 1) % perf.getLength (pat));
        }
        else
        {
            setParam ("arpHold", getParam ("arpHold") > 0.5f ? 0.0f : 1.0f);
        }
    };
    resetButton.onClick = [this]
    {
        if (shift || getParam ("seqRec") > 0.5f)
        {
            // ACCENT: toggle the accent on the last recorded step
            auto& perf = processor.getPerformance();
            const int pat = processor.getCurrentPattern();
            const int len = perf.getLength (pat);
            const int s = (perf.getRecordCursor() + len - 1) % len;
            auto st = perf.getStep (pat, s);
            st.accent = ! st.accent;
            perf.setStep (pat, s, st);
        }
        else
        {
            processor.getPerformance().restart();
        }
    };
    arpButton.onClick = [this]
    {
        if (shift)
        {
            // SET END: the pattern ends at the record cursor
            auto& perf = processor.getPerformance();
            perf.setLength (processor.getCurrentPattern(), perf.getRecordCursor() + 1);
            shift = false;
            return;
        }
        const bool arpOn = (int) getParam ("playMode") == grind::modeArp;
        setParam ("playMode", arpOn ? (float) grind::modeKeys : (float) grind::modeArp);
    };
    patternButton.onClick = [this] { showPatternMenu(); };
    shiftButton.onClick = [this] { shift = ! shift; };
    pageButton.onClick = [this] { page = (page + 1) % 4; stepMode = true; };
    playButton.onClick = [this]
    {
        const bool running = getParam ("seqRun") > 0.5f;
        if (! running && (int) getParam ("playMode") != grind::modeSeq)
            setParam ("playMode", (float) grind::modeSeq);
        setParam ("seqRun", running ? 0.0f : 1.0f);
    };
    recButton.onClick = [this]
    {
        const bool rec = getParam ("seqRec") > 0.5f;
        if (! rec)
        {
            setParam ("playMode", (float) grind::modeSeq);
            processor.getPerformance().setRecordCursor (0);
        }
        setParam ("seqRec", rec ? 0.0f : 1.0f);
    };
    kybdButton.onClick = [this] { stepMode = false; };
    stepButton.onClick = [this] { stepMode = true; };
    leftArrow.onClick = [this] { if (stepMode) page = (page + 3) % 4; else octave = std::max (0, octave - 1); };
    rightArrow.onClick = [this] { if (stepMode) page = (page + 1) % 4; else octave = std::min (7, octave + 1); };

    //== Pads (white keys / steps) and black keys
    static const int whiteOffsets[] = { 0, 2, 4, 5, 7, 9, 11, 12 };
    static const int blackOffsets[] = { 1, 3, 6, 8, 10 };
    static const int blackX[] = { 1095, 1183, 1357, 1445, 1533 };
    for (int i = 0; i < 8; ++i)
    {
        auto& pad = pads[(size_t) i];
        pad.setBounds (1052 + juce::roundToInt ((float) i * 87.3f), 877, 77, 70);
        pad.onPress = [this, i] { padPressed (i, false, true); };
        pad.onRelease = [this, i] { padPressed (i, false, false); };
        pad.onLongPress = [this, i] { if (stepMode) stepPadLongPress (i); };
        padNote[(size_t) i] = whiteOffsets[i];
        addAndMakeVisible (pad);
    }
    for (int i = 0; i < 5; ++i)
    {
        auto& key = blackKeys[(size_t) i];
        key.setBounds (blackX[i], 757, 78, 70);
        key.onPress = [this, i] { padPressed (i, true, true); };
        key.onRelease = [this, i] { padPressed (i, true, false); };
        keyNote[(size_t) i] = blackOffsets[i];
        addAndMakeVisible (key);
    }

    addAndMakeVisible (cables);
    cables.setBounds (getLocalBounds());

    addChildComponent (menu);
    menu.setTransform (juce::AffineTransform::scale (1.8f).translated (55.0f, 70.0f));
    menu.onClose = [this] { showMenu (-1); };

    tick();
}

GrindPanel::~GrindPanel()
{
    setLookAndFeel (nullptr);
}

void GrindPanel::placeKnob (juce::Slider& s, float cx, float cy, bool small)
{
    const float size = small ? smallKnob : bigKnob;
    s.setBounds (juce::Rectangle<float> (size, size).withCentre ({ cx, cy }).toNearestInt());
    addAndMakeVisible (s);
}

void GrindPanel::placeSwitch (ToggleSwitch& s, float cx, float cy)
{
    s.setBounds (juce::Rectangle<float> (64.0f, 48.0f).withCentre ({ cx, cy }).toNearestInt());
    addAndMakeVisible (s);
}

void GrindPanel::setParam (const juce::String& id, float value)
{
    if (auto* param = processor.apvts.getParameter (id))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (value));
        param->endChangeGesture();
    }
}

float GrindPanel::getParam (const juce::String& id) const
{
    return processor.apvts.getRawParameterValue (id)->load();
}

void GrindPanel::showMenu (int tab)
{
    if (tab < 0)
    {
        menu.setVisible (false);
        return;
    }
    menu.showTab (tab);
    menu.setVisible (true);
    menu.toFront (false);
    menu.tick();
}

void GrindPanel::padPressed (int index, bool black, bool down)
{
    if (stepMode && ! black)
    {
        if (! down)
            return;
        // STEP: the pads are the 8 steps of the current page; a tap toggles the gate
        auto& perf = processor.getPerformance();
        const int pat = processor.getCurrentPattern();
        const int s = page * 8 + index;
        auto st = perf.getStep (pat, s);
        st.gate = ! st.gate;
        if (! st.gate) st.tie = false;
        perf.setStep (pat, s, st);
        perf.setRecordCursor (s);
        return;
    }

    const int note = 24 + 12 * octave + (black ? keyNote[(size_t) index] : padNote[(size_t) index]);
    processor.playNoteFromEditor (note, down);
}

void GrindPanel::stepPadLongPress (int index)
{
    const int s = page * 8 + index;
    // Undo the toggle the press made, then open the step menu
    auto& perf = processor.getPerformance();
    const int pat = processor.getCurrentPattern();
    auto st = perf.getStep (pat, s);
    st.gate = ! st.gate;
    perf.setStep (pat, s, st);
    showStepMenu (processor, pat, s, pads[(size_t) index], pads[(size_t) index].getScreenBounds(), {});
}

void GrindPanel::showPatternMenu()
{
    juce::PopupMenu m;
    const auto names = MidnightGrindProcessor::patternNames();
    const int current = processor.getCurrentPattern();
    for (int bank = 0; bank < 8; ++bank)
    {
        juce::PopupMenu sub;
        for (int slot = 0; slot < 8; ++slot)
        {
            const int idx = bank * 8 + slot;
            sub.addItem (idx + 1, names[idx], true, idx == current);
        }
        m.addSubMenu ("Bank " + juce::String::charToString ((juce::juce_wchar) ('A' + bank)), sub, true,
                      nullptr, current / 8 == bank);
    }
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&patternButton), [this] (int r)
    {
        if (r > 0)
            setParam ("pattern", (float) (r - 1));
    });
}

void GrindPanel::updateSequencerLights()
{
    auto& perf = processor.getPerformance();
    const int mode = (int) getParam ("playMode");
    const bool running = getParam ("seqRun") > 0.5f;
    const bool rec = getParam ("seqRec") > 0.5f;
    const int playing = perf.getPlayingStep();

    holdButton.setLit (getParam ("arpHold") > 0.5f ? 1.0f : 0.0f);
    arpButton.setLit (mode == grind::modeArp ? 1.0f : 0.0f);
    shiftButton.setLit (shift ? 1.0f : 0.0f, hw::label);
    playButton.setLit (running && mode == grind::modeSeq ? 1.0f : 0.0f, hw::ledGreen);
    recButton.setLit (rec ? 1.0f : 0.0f);
    kybdButton.setLit (stepMode ? 0.0f : 1.0f, hw::label);
    stepButton.setLit (stepMode ? 1.0f : 0.0f, hw::label);

    const int pat = processor.getCurrentPattern();
    for (int i = 0; i < 8; ++i)
    {
        if (stepMode)
        {
            const int s = page * 8 + i;
            const auto st = perf.getStep (pat, s);
            const bool inPattern = s < perf.getLength (pat);
            float level = st.gate && inPattern ? (st.accent ? 1.0f : 0.75f) : 0.0f;
            auto c = hw::padRed;
            if (s == playing && mode == grind::modeSeq)
            {
                level = 1.0f;
                c = juce::Colour (0xffffd0c0);
            }
            pads[(size_t) i].setLit (level, c);
            pads[(size_t) i].setText (inPattern && st.gate ? juce::MidiMessage::getMidiNoteName (st.note, true, true, 4)
                                                           : juce::String());
        }
        else
        {
            pads[(size_t) i].setLit (0.8f);
            pads[(size_t) i].setText ({});
        }
    }

    swing.setVisible (shift);
    tempo.setVisible (! shift);
}

void GrindPanel::tick()
{
    modelDisplay.tick();
    cables.tick();
    if (menu.isVisible())
        menu.tick();
    updateSequencerLights();

    if (processor.getCurrentProgram() != shownProgram)
    {
        shownProgram = processor.getCurrentProgram();
        presetBox.setSelectedItemIndex (shownProgram, juce::dontSendNotification);
    }

    const float led = processor.getLfoLevel();
    if (std::abs (led - lfoLed) > 0.03f)
    {
        lfoLed = led;
        repaint (1170, 576, 40, 40);
    }
    repaint (735, 785, 260, 70);   // octave / location LEDs
    repaint (420, 425, 300, 28);   // model name
    repaint (150, 915, 80, 24);    // SWING legend (lit with SHIFT)
}

//==============================================================================
void GrindPanel::drawSection (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title) const
{
    g.setColour (hw::orange);
    g.drawRect (r, 3.0f);
    auto header = r.removeFromTop (25.0f);
    g.fillRect (header);
    g.setColour (juce::Colour (0xfffff1e8));
    g.setFont (uiFont (24.0f));
    g.drawText (title, header.withTrimmedLeft (12.0f).toNearestInt(), juce::Justification::centredLeft);
}

void GrindPanel::drawLabel (juce::Graphics& g, const juce::String& text, float cx, float cy, float size) const
{
    g.setColour (hw::label);
    g.setFont (uiFont (size));
    g.drawText (text, juce::Rectangle<float> (300.0f, size + 6.0f).withCentre ({ cx, cy }).toNearestInt(),
                juce::Justification::centred);
}

void GrindPanel::drawBoxedLabel (juce::Graphics& g, const juce::String& text, float cx, float cy, bool filled, float size) const
{
    g.setFont (uiFont (size));
    const float w = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), text) + 10.0f;
    const auto r = juce::Rectangle<float> (w, size + 3.0f).withCentre ({ cx, cy });
    if (filled)
    {
        g.setColour (hw::label);
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (hw::faceDark);
    }
    else
    {
        g.setColour (hw::label);
        g.drawRoundedRectangle (r, 2.0f, 1.6f);
    }
    g.drawText (text, r.toNearestInt(), juce::Justification::centred);
}

void GrindPanel::paint (juce::Graphics& g)
{
    // Wooden end cheeks and the panel face
    for (auto x : { 0.0f, panelWidth - 50.0f })
    {
        g.setGradientFill (juce::ColourGradient (hw::wood.brighter (0.15f), x, 0.0f, hw::wood.darker (0.35f), x + 50.0f, 0.0f, false));
        g.fillRect (x, 0.0f, 50.0f, panelHeight);
    }
    g.setGradientFill (juce::ColourGradient (hw::face.brighter (0.06f), 0.0f, 0.0f, hw::face.darker (0.25f), 0.0f, panelHeight, false));
    g.fillRect (50.0f, 0.0f, panelWidth - 100.0f, panelHeight);

    //== Top strip
    g.setColour (hw::label.withAlpha (0.6f));
    g.setFont (uiFont (20.0f));
    g.drawText ("PRESET", 300, 14, 110, 44, juce::Justification::centredRight);

    //== MIDI sockets
    for (auto [x, name] : { std::pair<float, const char*> { 190.0f, "MIDI IN" }, { 320.0f, "MIDI OUT/THRU" } })
    {
        drawLabel (g, name, x, 97.0f, 18.0f);
        const juce::Point<float> c (x, 183.0f);
        g.setColour (juce::Colour (0xffc9c9ce));
        g.fillEllipse (c.x - 50.0f, c.y - 50.0f, 100.0f, 100.0f);
        g.setColour (juce::Colour (0xff0d0d0e));
        g.fillEllipse (c.x - 40.0f, c.y - 40.0f, 80.0f, 80.0f);
        g.setColour (juce::Colour (0xff3a3a3e));
        for (int i = 0; i < 5; ++i)
        {
            const float a = juce::MathConstants<float>::pi * (0.75f + 0.25f * (float) i) + juce::MathConstants<float>::halfPi;
            const auto pin = c.getPointOnCircumference (22.0f, a);
            g.fillEllipse (pin.x - 5.0f, pin.y - 5.0f, 10.0f, 10.0f);
        }
    }

    //== Jack labels (boxed = input, dark plate = output)
    for (int s = 0; s < (int) grind::panelJacks.size(); ++s)
    {
        const auto& jack = grind::panelJacks[(size_t) s];
        const auto c = CableLayer::jackCentre (s);
        const juce::String name = jack.isInput ? grind::inputNames[(size_t) jack.index] : grind::outputNames[(size_t) jack.index];
        const float ly = s < grind::jacksPerRow ? 97.0f : 183.0f;
        if (jack.isInput)
        {
            drawBoxedLabel (g, name.replace ("MIX ", "MIX"), c.x, ly, false, 15.0f);
        }
        else
        {
            g.setColour (juce::Colour (0xff0e0e10));
            g.fillRect (juce::Rectangle<float> (84.0f, 20.0f).withCentre ({ c.x, ly }));
            drawLabel (g, name, c.x, ly, 15.5f);
        }
    }
    drawBoxedLabel (g, "IN", 1908.0f, 158.0f, false, 20.0f);
    drawBoxedLabel (g, "OUT", 1908.0f, 182.0f, true, 20.0f);

    //== Sections
    drawSection (g, { 78.0f, 272.0f, 921.0f, 264.0f }, "OSCILLATOR");
    drawSection (g, { 1003.0f, 272.0f, 689.0f, 264.0f }, "FILTER (VCF)");
    drawSection (g, { 1694.0f, 272.0f, 229.0f, 264.0f }, "OUTPUT (VCA)");
    drawSection (g, { 78.0f, 537.0f, 686.0f, 182.0f }, "ENVELOPE");
    drawSection (g, { 766.0f, 537.0f, 233.0f, 182.0f }, "VIBRATO");
    drawSection (g, { 1003.0f, 537.0f, 459.0f, 182.0f }, "MODULATION");
    drawSection (g, { 1464.0f, 537.0f, 459.0f, 182.0f }, "UTILITY");
    drawSection (g, { 78.0f, 724.0f, 1845.0f, 246.0f }, "SEQUENCER");

    //== Oscillator
    drawLabel (g, "TIMBRE", 190, 310);
    drawLabel (g, "HARMONICS", 420, 310);
    drawLabel (g, "FM", 535, 347, 19.0f);
    drawLabel (g, "FREQUENCY", 650, 310);
    drawLabel (g, "MORPH", 880, 310);
    for (float x : { 190.0f, 880.0f })
    {
        drawLabel (g, "-", x - 15.0f, 521, 20.0f);
        drawLabel (g, "+", x + 15.0f, 521, 20.0f);
        g.setColour (hw::label);
        g.drawLine (x, 440.0f, x, 466.0f, 2.0f);
    }
    drawLabel (g, "-", 518, 414, 18.0f);
    drawLabel (g, "+", 552, 414, 18.0f);
    drawLabel (g, "BANK", 305, 440, 19.0f);
    drawLabel (g, "MODEL", 765, 440, 19.0f);
    drawLabel (g, "A", 400, 467, 19.0f);
    drawLabel (g, "B", 400, 516, 19.0f);
    {
        const int m = (int) getParam ("model");
        if (juce::isPositiveAndBelow (m, grind::numEngines))
        {
            const auto bank = (juce::juce_wchar) ('A' + m / 10);
            g.setColour (hw::orange.brighter (0.3f));
            g.setFont (uiFont (18.0f));
            g.drawText (juce::String::charToString (bank) + juce::String (m % 10 + 1) + "  "
                            + juce::String (grind::engineInfo[(size_t) m].name).toUpperCase(),
                        420, 427, 300, 22, juce::Justification::centred);
        }
    }

    //== Filter / output
    drawLabel (g, "CUTOFF", 1118, 310);
    drawLabel (g, "RESONANCE", 1348, 310);
    drawLabel (g, "VCF MOD", 1578, 310);
    drawLabel (g, "VOLUME", 1808, 310);
    drawLabel (g, "MODE", 1118, 463, 19.0f);
    drawLabel (g, "MOD SOURCE", 1348, 463, 19.0f);
    drawLabel (g, "MOD POLARITY", 1578, 463, 19.0f);
    drawLabel (g, "VCA MODE", 1808, 463, 19.0f);
    drawLabel (g, "LPG", 1808, 481, 15.0f);
    for (auto [x, l, r] : { std::tuple<float, const char*, const char*> { 1118.0f, "LO", "HI" }, { 1348.0f, "ENV", "LFO" },
                            { 1578.0f, "POS", "NEG" }, { 1808.0f, "ENV", "ON" } })
    {
        drawLabel (g, l, x - 60.0f, 511, 18.0f);
        drawLabel (g, r, x + 58.0f, 511, 18.0f);
    }

    //== Envelope / vibrato / modulation / utility
    drawLabel (g, "ATTACK", 190, 571);
    drawLabel (g, "DECAY", 420, 571);
    drawLabel (g, "SUSTAIN", 650, 571);
    drawLabel (g, "OSC MOD", 880, 571);
    drawLabel (g, "LFO RATE", 1118, 571);
    drawLabel (g, "SHAPE", 1348, 611, 19.0f);
    {
        // Square and triangle symbols either side of the SHAPE switch
        g.setColour (hw::label);
        juce::Path sq, tri;
        sq.startNewSubPath (1292, 652); sq.lineTo (1298, 652); sq.lineTo (1298, 636); sq.lineTo (1308, 636); sq.lineTo (1308, 652); sq.lineTo (1314, 652);
        tri.startNewSubPath (1380, 652); tri.lineTo (1388, 636); tri.lineTo (1396, 652);
        g.strokePath (sq, juce::PathStrokeType (2.2f));
        g.strokePath (tri, juce::PathStrokeType (2.2f));
    }
    drawLabel (g, "GLIDE", 1578, 571);
    drawLabel (g, "VC MIX", 1808, 571);
    drawLabel (g, "LO/MIX 1", 1752, 706, 15.0f);
    drawLabel (g, "HI/MIX 2", 1864, 706, 15.0f);

    // LFO rate LED
    {
        const float level = 0.5f + 0.5f * lfoLed;
        g.setColour (hw::ledRed.withAlpha (0.25f * level));
        g.fillEllipse (1176.0f, 582.0f, 28.0f, 28.0f);
        g.setColour (hw::ledRed.withAlpha (0.2f + 0.8f * level));
        g.fillEllipse (1181.0f, 587.0f, 18.0f, 18.0f);
    }

    //== Sequencer
    drawLabel (g, "TEMPO/GATE LENGTH", 190, 784, 19.0f);
    drawBoxedLabel (g, "SWING", 190, 926, shift, 16.0f);
    const char* row1[] = { "HOLD/", "RESET/", "ARP", "PATTERN" };
    const char* row1b[] = { "REST", "ACCENT", "SET END", "BANK" };
    const char* row2[] = { "SHIFT", "PAGE", "PLAY/STOP", "REC" };
    for (int i = 0; i < 4; ++i)
    {
        const float cx = 376.0f + 87.0f * (float) i;
        drawLabel (g, row1[i], cx, 840, 16.0f);
        if (i >= 2) drawBoxedLabel (g, row1b[i], cx, 857, true, 15.0f);
        else drawLabel (g, row1b[i], cx, 857, 16.0f);
        if (i == 0) drawBoxedLabel (g, row2[i], cx, 936, true, 15.0f);
        else drawLabel (g, row2[i], cx, 936, 16.0f);
    }
    drawBoxedLabel (g, "KYBD", 820, 936, true, 15.0f);
    drawBoxedLabel (g, "STEP", 908, 936, true, 15.0f);
    drawLabel (g, "OCTAVE/LOCATION", 864, 775, 17.0f);

    {
        const int playing = processor.getPerformance().getPlayingStep();
        for (int i = 0; i < 8; ++i)
        {
            const float cx = 754.0f + 31.7f * (float) i;
            const bool red = stepMode ? i == page : i == octave;
            const bool green = playing >= 0 && (stepMode ? playing / 8 == i : playing % 8 == i);
            const auto c = red ? hw::ledRed : green ? hw::ledGreen : juce::Colour (0xff4a4a4f);
            if (red || green)
            {
                g.setColour (c.withAlpha (0.3f));
                g.fillEllipse (cx - 13.0f, 793.0f, 26.0f, 26.0f);
            }
            g.setColour (c);
            g.fillEllipse (cx - 8.5f, 797.5f, 17.0f, 17.0f);
            drawLabel (g, juce::String (i + 1), cx, 840, 16.0f);
        }
    }
    for (int i = 0; i < 8; ++i)
        drawLabel (g, juce::String (i + 1), 1090.0f + 87.3f * (float) i, 961, 17.0f);

    // Name plate and power LED
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (uiFont (66.0f), "GRIND", 1640.0f, 830.0f);
        juce::Path text;
        ga.createPath (text);
        g.setColour (hw::orange);
        g.strokePath (text, juce::PathStrokeType (3.0f));
        g.setColour (hw::label.withAlpha (0.8f));
        g.setFont (uiFont (17.0f));
        g.drawText ("MIDNIGHT", 1642, 768, 200, 20, juce::Justification::centredLeft);
        drawLabel (g, "POWER", 1832, 775, 16.0f);
        g.setColour (juce::Colour (0xffffb020).withAlpha (0.35f));
        g.fillEllipse (1820.0f, 794.0f, 24.0f, 24.0f);
        g.setColour (juce::Colour (0xffffb020));
        g.fillEllipse (1825.0f, 799.0f, 14.0f, 14.0f);
        g.setColour (hw::label.withAlpha (0.6f));
        g.setFont (uiFont (22.0f, false));
        g.drawText ("midnight audio", 1760, 905, 160, 30, juce::Justification::centredRight);
    }
}

//==============================================================================
MidnightGrindEditor::MidnightGrindEditor (MidnightGrindProcessor& p)
    : AudioProcessorEditor (&p), processor (p), panel (p)
{
    setLookAndFeel (&lnf);
    panel.getMenu().setLookAndFeel (&lnf);
    addAndMakeVisible (panel);

    setResizable (true, true);
   #if ! (JUCE_ANDROID || JUCE_IOS)
    getConstrainer()->setFixedAspectRatio ((double) panelWidth / (double) panelHeight);
    setResizeLimits (700, 360, 3000, 1545);
   #endif
    setSize (1400, juce::roundToInt (1400.0f * panelHeight / panelWidth));
    startTimerHz (30);
}

MidnightGrindEditor::~MidnightGrindEditor()
{
    panel.getMenu().setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void MidnightGrindEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void MidnightGrindEditor::resized()
{
    const float scale = std::min ((float) getWidth() / panelWidth, (float) getHeight() / panelHeight);
    const float x = ((float) getWidth() - panelWidth * scale) * 0.5f;
    const float y = ((float) getHeight() - panelHeight * scale) * 0.5f;
    panel.setTransform (juce::AffineTransform::scale (scale).translated (x, y));
}
