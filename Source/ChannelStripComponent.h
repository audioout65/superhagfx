#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"
#include "SlotButton.h"

// L'interface complete d'un seul canal : entree/sortie, vumetre, gain,
// bypass, et la pile d'emplacements de plugins.
class ChannelStripComponent : public juce::Component
{
public:
    ChannelStripComponent (int channelIndexIn, AudioChannel& channelIn)
        : channelIndex (channelIndexIn), channel (channelIn)
    {
        channelLabel.setButtonText ("Ch " + juce::String (channelIndex + 1));
        channelLabel.onClick = [this]
        {
            if (onChannelNameClicked != nullptr)
                onChannelNameClicked (channelIndex);
        };
        addAndMakeVisible (channelLabel);

        for (int i = 1; i <= 32; ++i)
            inputCombo.addItem ("In " + juce::String (i), i);

        for (int i = 1; i <= 16; ++i)
            outputCombo.addItem ("Out " + juce::String (i), i);

        inputCombo.setSelectedId (channel.getInputChannelIndex() + 1, juce::dontSendNotification);
        outputCombo.setSelectedId (channel.getOutputChannelIndex() + 1, juce::dontSendNotification);

        inputCombo.onChange = [this]
        {
            channel.setInputChannelIndex (inputCombo.getSelectedId() - 1);
        };
        outputCombo.onChange = [this]
        {
            channel.setOutputChannelIndex (outputCombo.getSelectedId() - 1);
        };

        addAndMakeVisible (inputCombo);
        addAndMakeVisible (outputCombo);

        bypassButton.setButtonText ("Bypass");
        bypassButton.onClick = [this]
        {
            channel.setBypassed (bypassButton.getToggleState());
        };
        addAndMakeVisible (bypassButton);

        gainSlider.setSliderStyle (juce::Slider::LinearVertical);
        gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 18);
        gainSlider.setRange (0.0, 2.0, 0.01);
        gainSlider.setValue (1.0);
        gainSlider.onValueChange = [this]
        {
            channel.setGain ((float) gainSlider.getValue());
        };
        addAndMakeVisible (gainSlider);

        for (int slot = 0; slot < numSlotsPerChannel; ++slot)
        {
            auto slotBtn = std::make_unique<SlotButton>("+");
            auto* ptr = slotBtn.get();

            slotBtn->onClick = [this, slot]
            {
                if (onSlotClicked != nullptr)
                    onSlotClicked (channelIndex, slot);
            };
            slotBtn->onRightClick = [this, slot]
            {
                if (onSlotRightClicked != nullptr)
                    onSlotRightClicked (channelIndex, slot);
            };

            addAndMakeVisible (*ptr);
            slotButtons.push_back (std::move (slotBtn));
        }
    }

    void refreshSlotButtonText (int slotIndex, const juce::String& text)
    {
        if (slotIndex >= 0 && slotIndex < (int) slotButtons.size())
            slotButtons[(size_t) slotIndex]->setButtonText (text);
    }

    void refreshFromChannel()
    {
        inputCombo.setSelectedId (channel.getInputChannelIndex() + 1, juce::dontSendNotification);
        outputCombo.setSelectedId (channel.getOutputChannelIndex() + 1, juce::dontSendNotification);
        gainSlider.setValue (channel.getGain(), juce::dontSendNotification);
        bypassButton.setToggleState (channel.isBypassed(), juce::dontSendNotification);

        for (int slot = 0; slot < numSlotsPerChannel; ++slot)
            refreshSlotButtonText (slot, channel.getSlot (slot).getDisplayName());
    }

    void updateMeter()
    {
        auto target = channel.getLevel();

        if (target > displayedLevel)
            displayedLevel = target;
        else
            displayedLevel *= 0.85f;

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::transparentBlack);

        auto meterArea = meterBounds;

        g.setColour (juce::Colours::darkgrey);
        g.fillRect (meterArea);

        auto level = juce::jlimit (0.0f, 1.0f, displayedLevel);
        auto filledHeight = (int) (level * (float) meterArea.getHeight());
        auto filledArea = meterArea.withTop (meterArea.getBottom() - filledHeight);

        g.setColour (level > 0.9f ? juce::Colours::red
                                   : (level > 0.7f ? juce::Colours::orange : juce::Colours::green));
        g.fillRect (filledArea);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (6);

        channelLabel.setBounds (area.removeFromTop (22));
        area.removeFromTop (4);

        auto ioArea = area.removeFromTop (44);
        inputCombo.setBounds (ioArea.removeFromTop (20));
        ioArea.removeFromTop (4);
        outputCombo.setBounds (ioArea);
        area.removeFromTop (6);

        auto meterAndGainArea = area.removeFromTop (160);
        meterBounds = meterAndGainArea.removeFromLeft (24).withSizeKeepingCentre (18, 150);
        gainSlider.setBounds (meterAndGainArea);

        area.removeFromTop (4);
        bypassButton.setBounds (area.removeFromTop (24));
        area.removeFromTop (6);

        for (auto& btn : slotButtons)
        {
            btn->setBounds (area.removeFromTop (30));
            area.removeFromTop (4);
        }
    }

    std::function<void (int channelIndex)> onChannelNameClicked;
    std::function<void (int channelIndex, int slotIndex)> onSlotClicked;
    std::function<void (int channelIndex, int slotIndex)> onSlotRightClicked;

private:
    int channelIndex;
    AudioChannel& channel;
    float displayedLevel = 0.0f;
    juce::Rectangle<int> meterBounds;

    juce::TextButton channelLabel;
    juce::ComboBox inputCombo, outputCombo;
    juce::ToggleButton bypassButton;
    juce::Slider gainSlider;
    std::vector<std::unique_ptr<SlotButton>> slotButtons;
};
