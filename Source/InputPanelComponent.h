#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"

// Panneau "INPUT" : titre, selecteur d'entree et vumetre d'entree, dans un
// cadre distinct, symetrique au panneau OUTPUT.
class InputPanelComponent : public juce::Component
{
public:
    InputPanelComponent()
    {
        titleLabel.setText ("INPUT", juce::dontSendNotification);
        titleLabel.setFont (juce::Font (12.0f, juce::Font::bold));
        titleLabel.setJustificationType (juce::Justification::centred);
        titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a92a0));
        addAndMakeVisible (titleLabel);

        for (int i = 1; i <= 32; ++i)
            inputCombo.addItem ("In " + juce::String (i), i);
        addAndMakeVisible (inputCombo);
    }

    void setChannel (AudioChannel* newChannel)
    {
        channel = newChannel;

        inputCombo.onChange = nullptr;

        if (channel != nullptr)
        {
            inputCombo.setSelectedId (channel->getInputChannelIndex() + 1, juce::dontSendNotification);

            inputCombo.onChange = [this]
            {
                if (channel != nullptr)
                    channel->setInputChannelIndex (inputCombo.getSelectedId() - 1);
            };
        }
    }

    void updateMeter()
    {
        if (channel == nullptr)
            return;

        auto target = channel->getInputLevel();

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
        area.removeFromTop (8);

        inputCombo.setBounds (area.removeFromTop (28));
        area.removeFromTop (12);

        meterBounds = area.withSizeKeepingCentre (34, area.getHeight()).withY (area.getY());
    }

private:
    AudioChannel* channel = nullptr;
    float displayedLevel = 0.0f;
    juce::Rectangle<int> meterBounds;

    juce::Label titleLabel;
    juce::ComboBox inputCombo;
};
