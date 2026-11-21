#pragma once
#include <JuceHeader.h>

// Style visuel sombre et moderne pour toute l'application, inspire des
// consoles et racks d'effets professionnels.
class LiveRackLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LiveRackLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff1a1d21));

        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2c313a));
        setColour (juce::TextButton::buttonOnColourId, accentColour);
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xffe0e0e0));
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);

        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2c313a));
        setColour (juce::ComboBox::textColourId, juce::Colour (0xffe0e0e0));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3d434d));
        setColour (juce::ComboBox::arrowColourId, accentColour);

        setColour (juce::Slider::trackColourId, juce::Colour (0xff3d434d));
        setColour (juce::Slider::thumbColourId, accentColour);
        setColour (juce::Slider::rotarySliderFillColourId, accentColour);
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3d434d));
        setColour (juce::Slider::backgroundColourId, juce::Colour (0xff2c313a));
        setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe0e0e0));
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff2c313a));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff3d434d));

        setColour (juce::ToggleButton::textColourId, juce::Colour (0xffe0e0e0));
        setColour (juce::ToggleButton::tickColourId, accentColour);
        setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff3d434d));

        setColour (juce::Label::textColourId, juce::Colour (0xffe0e0e0));

        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff22262b));
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xffe0e0e0));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accentColour);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        setColour (juce::DocumentWindow::backgroundColourId, juce::Colour (0xff1a1d21));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        auto cornerSize = 6.0f;

        auto baseColour = backgroundColour;

        if (shouldDrawButtonAsDown)
            baseColour = baseColour.darker (0.2f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter (0.1f);

        g.setColour (baseColour);
        g.fillRoundedRectangle (bounds, cornerSize);

        g.setColour (baseColour.brighter (0.15f));
        g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
        auto cornerSize = 4.0f;

        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (bounds, cornerSize);

        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (bounds, cornerSize, 1.0f);

        juce::Rectangle<int> arrowZone (width - 20, 0, 16, height);
        juce::Path path;
        path.startNewSubPath ((float) arrowZone.getX() + 3.0f, (float) arrowZone.getCentreY() - 2.0f);
        path.lineTo ((float) arrowZone.getCentreX(), (float) arrowZone.getCentreY() + 3.0f);
        path.lineTo ((float) arrowZone.getRight() - 3.0f, (float) arrowZone.getCentreY() - 2.0f);

        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.strokePath (path, juce::PathStrokeType (2.0f));
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            auto trackWidth = 6.0f;
            auto trackBounds = juce::Rectangle<float> ((float) x + (float) width * 0.5f - trackWidth * 0.5f,
                                                       (float) y, trackWidth, (float) height);

            g.setColour (slider.findColour (juce::Slider::backgroundColourId));
            g.fillRoundedRectangle (trackBounds, trackWidth * 0.5f);

            auto filledBounds = trackBounds.withTop (sliderPos);
            g.setColour (slider.findColour (juce::Slider::trackColourId));
            g.fillRoundedRectangle (filledBounds, trackWidth * 0.5f);

            auto thumbRadius = 7.0f;
            g.setColour (slider.findColour (juce::Slider::thumbColourId));
            g.fillEllipse (trackBounds.getCentreX() - thumbRadius, sliderPos - thumbRadius,
                           thumbRadius * 2.0f, thumbRadius * 2.0f);
        }
        else
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }

    // Knob metallique avec un anneau de petites LED autour (bleues jusqu'a la
    // valeur actuelle, sombres au-dela), et le repere "0" (0 dB) marque.
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        auto fullBounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (2.0f);
        auto overallRadius = juce::jmin (fullBounds.getWidth(), fullBounds.getHeight()) * 0.5f;
        auto centre = fullBounds.getCentre();

        auto knobRadius = overallRadius * 0.62f;
        auto ledInnerRadius = overallRadius * 0.86f;
        auto ledOuterRadius = overallRadius;

        constexpr int numLeds = 24;
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        for (int i = 0; i < numLeds; ++i)
        {
            auto ledProportion = (float) i / (float) (numLeds - 1);
            auto ledAngle = rotaryStartAngle + ledProportion * (rotaryEndAngle - rotaryStartAngle);

            juce::Point<float> p1 (centre.x + ledInnerRadius * std::sin (ledAngle), centre.y - ledInnerRadius * std::cos (ledAngle));
            juce::Point<float> p2 (centre.x + ledOuterRadius * std::sin (ledAngle), centre.y - ledOuterRadius * std::cos (ledAngle));

            auto isLit = ledAngle <= angle + 0.001f;
            g.setColour (isLit ? slider.findColour (juce::Slider::rotarySliderFillColourId)
                               : juce::Colour (0xff23303f));
            g.drawLine ({ p1, p2 }, 2.6f);
        }

        // Repere "0" (0 dB), marque en blanc par-dessus les LED a sa position.
        auto zeroProportion = juce::jlimit (0.0f, 1.0f,
            (float) ((0.0 - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum())));
        auto zeroAngle = rotaryStartAngle + zeroProportion * (rotaryEndAngle - rotaryStartAngle);
        juce::Point<float> zeroInner (centre.x + ledInnerRadius * std::sin (zeroAngle), centre.y - ledInnerRadius * std::cos (zeroAngle));
        juce::Point<float> zeroOuter (centre.x + ledOuterRadius * std::sin (zeroAngle), centre.y - ledOuterRadius * std::cos (zeroAngle));
        g.setColour (juce::Colours::white);
        g.drawLine ({ zeroInner, zeroOuter }, 2.2f);

        juce::Point<float> labelPos (centre.x + (ledOuterRadius + 10.0f) * std::sin (zeroAngle),
                                     centre.y - (ledOuterRadius + 10.0f) * std::cos (zeroAngle));
        g.setFont (juce::Font (9.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xff8a92a0));
        g.drawText ("0", juce::Rectangle<float> (labelPos.x - 8.0f, labelPos.y - 6.0f, 16.0f, 12.0f),
                    juce::Justification::centred);

        // Corps du knob : degrade metallique circulaire.
        juce::ColourGradient metalGradient (juce::Colour (0xffd8dadd), centre.x - knobRadius * 0.4f, centre.y - knobRadius * 0.4f,
                                            juce::Colour (0xff3a3e44), centre.x + knobRadius * 0.6f, centre.y + knobRadius * 0.6f, true);
        g.setGradientFill (metalGradient);
        g.fillEllipse (centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

        g.setColour (juce::Colour (0xff15171a));
        g.drawEllipse (centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 1.5f);

        // Petit point indicateur, sur le bord du knob, qui tourne avec la valeur.
        auto dotRadius = knobRadius * 0.9f;
        juce::Point<float> dotCentre (centre.x + dotRadius * std::sin (angle), centre.y - dotRadius * std::cos (angle));
        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        g.fillEllipse (dotCentre.x - 3.0f, dotCentre.y - 3.0f, 6.0f, 6.0f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto boxSize = juce::jmin (18.0f, bounds.getHeight() - 4.0f);
        auto boxBounds = bounds.withWidth (boxSize).withHeight (boxSize).withCentre ({ bounds.getX() + boxSize * 0.5f + 2.0f, bounds.getCentreY() });

        g.setColour (button.findColour (button.getToggleState() ? juce::ToggleButton::tickColourId
                                                                 : juce::ToggleButton::tickDisabledColourId));
        g.drawRoundedRectangle (boxBounds, 3.0f, 1.5f);

        if (button.getToggleState())
            g.fillRoundedRectangle (boxBounds.reduced (3.0f), 2.0f);

        g.setColour (button.findColour (juce::ToggleButton::textColourId));
        g.setFont (juce::jmin (15.0f, bounds.getHeight() * 0.6f));
        g.drawText (button.getButtonText(),
                    bounds.withTrimmedLeft (boxSize + 8.0f),
                    juce::Justification::centredLeft);

        juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    }

    static const juce::Colour accentColour;
};

inline const juce::Colour LiveRackLookAndFeel::accentColour = juce::Colour (0xff4ea1ff);
