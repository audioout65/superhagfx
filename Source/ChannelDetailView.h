#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"
#include "SlotButton.h"

// Vue detaillee d'un seul canal, inspiree de l'ecran "Rack" de SuperRack :
// entree/sortie en haut, pile de slots large, vumetre et fader a droite.
class ChannelDetailView : public juce::Component
{
public:
    ChannelDetailView()
    {
        prevButton.setButtonText ("<");
        prevButton.onClick = [this] { if (onPrevious != nullptr) onPrevious(); };
        addAndMakeVisible (prevButton);

        nextButton.setButtonText (">");
        nextButton.onClick = [this] { if (onNext != nullptr) onNext(); };
        addAndMakeVisible (nextButton);

        channelNameLabel.setJustificationType (juce::Justification::centred);
        channelNameLabel.setFont (juce::Font (20.0f, juce::Font::bold));
        addAndMakeVisible (channelNameLabel);

        inputLabel.setText ("Input", juce::dontSendNotification);
        inputLabel.setJustificationType (juce::Justification::centredLeft);
        inputLabel.setFont (12.0f);
        addAndMakeVisible (inputLabel);

        outputLabel.setText ("Output", juce::dontSendNotification);
        outputLabel.setJustificationType (juce::Justification::centredLeft);
        outputLabel.setFont (12.0f);
        addAndMakeVisible (outputLabel);

        for (int i = 1; i <= 32; ++i)
            inputCombo.addItem ("In " + juce::String (i), i);

        for (int i = 1; i <= 16; ++i)
            outputCombo.addItem ("Out " + juce::String (i), i);

        addAndMakeVisible (inputCombo);
        addAndMakeVisible (outputCombo);

        bypassButton.setButtonText ("Bypass");
        addAndMakeVisible (bypassButton);

        gainSlider.setSliderStyle (juce::Slider::LinearVertical);
        gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        gainSlider.setRange (0.0, 2.0, 0.01);
        addAndMakeVisible (gainSlider);

        for (int slot = 0; slot < numSlotsPerChannel; ++slot)
        {
            auto slotBtn = std::make_unique<SlotButton>("ADD PLUGIN");
            slotBtn->setButtonText ("ADD PLUGIN");

            auto* ptr = slotBtn.get();
            auto slotIndexCopy = slot;

            slotBtn->onClick = [this, slotIndexCopy]
            {
                if (onSlotClicked != nullptr)
                    onSlotClicked (channelIndex, slotIndexCopy);
            };
            slotBtn->onRightClick = [this, slotIndexCopy]
            {
                if (onSlotRightClicked != nullptr)
                    onSlotRightClicked (channelIndex, slotIndexCopy);
            };

            addAndMakeVisible (*ptr);
            slotButtons.push_back (std::move (slotBtn));
            juce::ignoreUnused (ptr);
        }
    }

    void setChannel (int newChannelIndex, AudioChannel& newChannel)
    {
        channelIndex = newChannelIndex;
        channel = &newChannel;

        channelNameLabel.setText ("Ch " + juce::String (channelIndex + 1), juce::dontSendNotification);

        inputCombo.onChange = nullptr;
        outputCombo.onChange = nullptr;
        bypassButton.onClick = nullptr;
        gainSlider.onValueChange = nullptr;

        inputCombo.setSelectedId (channel->getInputChannelIndex() + 1, juce::dontSendNotification);
        outputCombo.setSelectedId (channel->getOutputChannelIndex() + 1, juce::dontSendNotification);
        bypassButton.setToggleState (channel->isBypassed(), juce::dontSendNotification);
        gainSlider.setValue (channel->getGain(), juce::dontSendNotification);

        inputCombo.onChange = [this]
        {
            if (channel != nullptr)
                channel->setInputChannelIndex (inputCombo.getSelectedId() - 1);
        };
        outputCombo.onChange = [this]
        {
            if (channel != nullptr)
                channel->setOutputChannelIndex (outputCombo.getSelectedId() - 1);
        };
        bypassButton.onClick = [this]
        {
            if (channel != nullptr)
                channel->setBypassed (bypassButton.getToggleState());
        };
        gainSlider.onValueChange = [this]
        {
            if (channel != nullptr)
                channel->setGain ((float) gainSlider.getValue());
        };

        refreshSlots();
    }

    void refreshSlots()
    {
        if (channel == nullptr)
            return;

        for (int slot = 0; slot < numSlotsPerChannel; ++slot)
        {
            auto name = channel->getSlot (slot).getDisplayName();
            slotButtons[(size_t) slot]->setButtonText (name == "+" ? "ADD PLUGIN" : name);
        }
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
        g.fillAll (juce::Colour (0xff1a1d21));

        auto meterArea = meterBounds;

        g.setColour (juce::Colour (0xff2c313a));
        g.fillRoundedRectangle (meterArea.toFloat(), 4.0f);

        auto level = juce::jlimit (0.0f, 1.0f, displayedLevel);
        auto filledHeight = (int) (level * (float) meterArea.getHeight());
        auto filledArea = meterArea.withTop (meterArea.getBottom() - filledHeight);

        g.setColour (level > 0.9f ? juce::Colours::red
                                   : (level > 0.7f ? juce::Colours::orange : juce::Colour (0xff4ea1ff)));
        g.fillRoundedRectangle (filledArea.toFloat(), 4.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (20);

        auto headerArea = area.removeFromTop (40);
        prevButton.setBounds (headerArea.removeFromLeft (40));
        nextButton.setBounds (headerArea.removeFromRight (40));
        channelNameLabel.setBounds (headerArea);

        area.removeFromTop (16);

        auto ioArea = area.removeFromTop (70);
        auto inputArea = ioArea.removeFromLeft (ioArea.getWidth() / 2).reduced (0, 0);
        inputLabel.setBounds (inputArea.removeFromTop (18));
        inputCombo.setBounds (inputArea.removeFromTop (30));

        ioArea.removeFromLeft (20);
        outputLabel.setBounds (ioArea.removeFromTop (18));
        outputCombo.setBounds (ioArea.removeFromTop (30));

        area.removeFromTop (16);

        auto rightColumn = area.removeFromRight (120);
        meterBounds = rightColumn.removeFromLeft (40).withSizeKeepingCentre (28, rightColumn.getHeight() - 60).withY (rightColumn.getY());
        rightColumn.removeFromLeft (10);
        gainSlider.setBounds (rightColumn.withHeight (rightColumn.getHeight() - 40));
        bypassButton.setBounds (rightColumn.withY (rightColumn.getBottom() - 30).withHeight (30));

        area.removeFromRight (20);

        for (auto& btn : slotButtons)
        {
            btn->setBounds (area.removeFromTop (44));
            area.removeFromTop (8);
        }
    }

    std::function<void()> onPrevious;
    std::function<void()> onNext;
    std::function<void (int channelIndex, int slotIndex)> onSlotClicked;
    std::function<void (int channelIndex, int slotIndex)> onSlotRightClicked;

private:
    int channelIndex = 0;
    AudioChannel* channel = nullptr;
    float displayedLevel = 0.0f;
    juce::Rectangle<int> meterBounds;

    juce::TextButton prevButton, nextButton;
    juce::Label channelNameLabel;
    juce::Label inputLabel, outputLabel;
    juce::ComboBox inputCombo, outputCombo;
    juce::ToggleButton bypassButton;
    juce::Slider gainSlider;
    std::vector<std::unique_ptr<SlotButton>> slotButtons;
};
