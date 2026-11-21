#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"

// Panneau "OUTPUT" complet : titre, selecteur de sortie, vumetre, bouton
// rotatif de gain et bouton IN, dans un cadre distinct, toujours ajoute en
// dernier a la fenetre principale pour rester au premier plan.
class MeterAndGainOverlay : public juce::Component
{
public:
    MeterAndGainOverlay()
    {
        titleLabel.setText ("OUTPUT", juce::dontSendNotification);
        titleLabel.setFont (juce::Font (12.0f, juce::Font::bold));
        titleLabel.setJustificationType (juce::Justification::centred);
        titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a92a0));
        addAndMakeVisible (titleLabel);

        for (int i = 1; i <= 16; ++i)
            outputCombo.addItem ("Out " + juce::String (i), i);
        addAndMakeVisible (outputCombo);

        gainKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        gainKnob.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                      juce::MathConstants<float>::pi * 2.8f, true);
        gainKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        gainKnob.setRange (0.0, 2.0, 0.01);
        addAndMakeVisible (gainKnob);

        inButton.setButtonText ("IN");
        inButton.setClickingTogglesState (true);
        inButton.setToggleState (true, juce::dontSendNotification);
        inButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff3fae54));
        inButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        addAndMakeVisible (inButton);

        for (auto db : { 0, -10, -20, -30, -40, -50, -60 })
        {
            auto lbl = std::make_unique<juce::Label>("", juce::String (db));
            lbl->setFont (9.0f);
            lbl->setJustificationType (juce::Justification::centredRight);
            lbl->setColour (juce::Label::textColourId, juce::Colour (0xff6a7280));
            addAndMakeVisible (*lbl);
            meterScaleLabels.push_back (std::move (lbl));
        }
    }

    void setChannel (AudioChannel* newChannel)
    {
        channel = newChannel;

        gainKnob.onValueChange = nullptr;
        inButton.onClick = nullptr;
        outputCombo.onChange = nullptr;

        if (channel != nullptr)
        {
            gainKnob.setValue (channel->getGain(), juce::dontSendNotification);
            inButton.setToggleState (! channel->isBypassed(), juce::dontSendNotification);
            outputCombo.setSelectedId (channel->getOutputChannelIndex() + 1, juce::dontSendNotification);

            gainKnob.onValueChange = [this]
            {
                if (channel != nullptr)
                    channel->setGain ((float) gainKnob.getValue());
            };
            inButton.onClick = [this]
            {
                if (channel != nullptr)
                    channel->setBypassed (! inButton.getToggleState());
            };
            outputCombo.onChange = [this]
            {
                if (channel != nullptr)
                    channel->setOutputChannelIndex (outputCombo.getSelectedId() - 1);
            };
        }

        setVisible (channel != nullptr);
    }

    void updateMeter()
    {
        if (channel == nullptr)
            return;

        auto target = channel->getLevel();

        if (target > displayedLevel)
            displayedLevel = target;
        else
            displayedLevel *= 0.85f;

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto panelBounds = getLocalBounds().toFloat();

        g.setColour (juce::Colour (0xff1e2227));
        g.fillRoundedRectangle (panelBounds, 6.0f);
        g.setColour (juce::Colour (0xff3d434d));
        g.drawRoundedRectangle (panelBounds.reduced (0.5f), 6.0f, 1.0f);

        auto meterArea = meterBounds.toFloat();

        g.setColour (juce::Colour (0xff0d0f11));
        g.fillRoundedRectangle (meterArea, 4.0f);
        g.setColour (juce::Colour (0xff3d434d));
        g.drawRoundedRectangle (meterArea, 4.0f, 1.5f);

        auto level = juce::jlimit (0.0f, 1.0f, displayedLevel);
        auto filledHeight = level * meterArea.getHeight();
        auto filledArea = meterArea.withTop (meterArea.getBottom() - filledHeight);

        g.setColour (level > 0.9f ? juce::Colours::red
                                   : (level > 0.7f ? juce::Colours::orange : juce::Colour (0xff4ea1ff)));
        g.fillRoundedRectangle (filledArea.reduced (2.0f), 2.0f);

        for (int i = 0; i < 6; ++i)
        {
            auto y = meterArea.getY() + meterArea.getHeight() * (float) i / 6.0f;
            g.setColour (juce::Colour (0xff2c313a));
            g.drawHorizontalLine ((int) y, meterArea.getX(), meterArea.getRight());
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8);

        titleLabel.setBounds (area.removeFromTop (18));
        area.removeFromTop (6);

        outputCombo.setBounds (area.removeFromTop (26));
        area.removeFromTop (10);

        auto scaleColumn = area.removeFromLeft (28);
        area.removeFromLeft (4);
        auto meterColumn = area.removeFromLeft (34);

        auto meterHeight = area.getHeight() - 90;
        meterBounds = meterColumn.withHeight (meterHeight).withY (area.getY());

        for (size_t i = 0; i < meterScaleLabels.size(); ++i)
        {
            auto y = meterBounds.getY() + (int) ((float) meterBounds.getHeight() * (float) i / (float) (meterScaleLabels.size() - 1)) - 7;
            meterScaleLabels[i]->setBounds (scaleColumn.getX(), y, scaleColumn.getWidth(), 14);
        }

        area.removeFromLeft (4);
        gainKnob.setBounds (area.withHeight (80).withY (area.getY()));
        inButton.setBounds (area.withY (area.getY() + 90).withHeight (30));
    }

private:
    AudioChannel* channel = nullptr;
    float displayedLevel = 0.0f;
    juce::Rectangle<int> meterBounds;

    juce::Label titleLabel;
    juce::ComboBox outputCombo;
    juce::Slider gainKnob;
    juce::TextButton inButton;
    std::vector<std::unique_ptr<juce::Label>> meterScaleLabels;
};
