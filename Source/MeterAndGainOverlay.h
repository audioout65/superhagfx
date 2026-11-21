#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"

// Le vumetre, le fader de gain et le bouton IN d'un canal, concus pour etre
// ajoutes en dernier a la fenetre principale : ainsi ils restent toujours
// au premier plan, par-dessus n'importe quel editeur de plugin, meme si
// celui-ci deborde visuellement de son emplacement prevu.
class MeterAndGainOverlay : public juce::Component
{
public:
    MeterAndGainOverlay()
    {
        gainSlider.setSliderStyle (juce::Slider::LinearVertical);
        gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        gainSlider.setRange (0.0, 2.0, 0.01);
        addAndMakeVisible (gainSlider);

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

        setInterceptsMouseClicks (false, true);
    }

    void setChannel (AudioChannel* newChannel)
    {
        channel = newChannel;

        gainSlider.onValueChange = nullptr;
        inButton.onClick = nullptr;

        if (channel != nullptr)
        {
            gainSlider.setValue (channel->getGain(), juce::dontSendNotification);
            inButton.setToggleState (! channel->isBypassed(), juce::dontSendNotification);

            gainSlider.onValueChange = [this]
            {
                if (channel != nullptr)
                    channel->setGain ((float) gainSlider.getValue());
            };
            inButton.onClick = [this]
            {
                if (channel != nullptr)
                    channel->setBypassed (! inButton.getToggleState());
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
        auto area = getLocalBounds();

        auto scaleColumn = area.removeFromLeft (30);
        area.removeFromLeft (6);
        auto meterColumn = area.removeFromLeft (36);

        auto meterHeight = area.getHeight() - 70;
        meterBounds = meterColumn.withHeight (meterHeight).withY (area.getY());

        for (size_t i = 0; i < meterScaleLabels.size(); ++i)
        {
            auto y = meterBounds.getY() + (int) ((float) meterBounds.getHeight() * (float) i / (float) (meterScaleLabels.size() - 1)) - 7;
            meterScaleLabels[i]->setBounds (scaleColumn.getX(), y, scaleColumn.getWidth(), 14);
        }

        area.removeFromLeft (10);
        gainSlider.setBounds (area.withHeight (meterHeight));
        inButton.setBounds (area.withY (area.getY() + meterHeight + 10).withHeight (30));
    }

private:
    AudioChannel* channel = nullptr;
    float displayedLevel = 0.0f;
    juce::Rectangle<int> meterBounds;

    juce::Slider gainSlider;
    juce::TextButton inButton;
    std::vector<std::unique_ptr<juce::Label>> meterScaleLabels;
};
