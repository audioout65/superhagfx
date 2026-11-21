#pragma once
#include <JuceHeader.h>

// Barre laterale gauche de la vue OVERVIEW, inspiree de SuperRack : un bloc
// MODE en haut (un seul mode pour nous : RACKS), puis les banques de
// canaux empilees verticalement en dessous (LAYERS).
class LeftSidebarComponent : public juce::Component
{
public:
    LeftSidebarComponent()
    {
        modeSectionLabel.setText ("MODE", juce::dontSendNotification);
        modeSectionLabel.setFont (10.0f);
        modeSectionLabel.setJustificationType (juce::Justification::centred);
        modeSectionLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a92a0));
        addAndMakeVisible (modeSectionLabel);

        racksModeButton.setButtonText ("RACKS");
        racksModeButton.setClickingTogglesState (true);
        racksModeButton.setToggleState (true, juce::dontSendNotification);
        racksModeButton.setRadioGroupId (3001);
        addAndMakeVisible (racksModeButton);

        layersSectionLabel.setText ("LAYERS", juce::dontSendNotification);
        layersSectionLabel.setFont (10.0f);
        layersSectionLabel.setJustificationType (juce::Justification::centred);
        layersSectionLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a92a0));
        addAndMakeVisible (layersSectionLabel);

        bankAButton.setButtonText ("1-16");
        bankBButton.setButtonText ("17-32");
        bankAButton.setClickingTogglesState (true);
        bankBButton.setClickingTogglesState (true);
        bankAButton.setRadioGroupId (3002);
        bankBButton.setRadioGroupId (3002);
        bankAButton.setToggleState (true, juce::dontSendNotification);

        bankAButton.onClick = [this] { if (onBankSelected != nullptr) onBankSelected (0); };
        bankBButton.onClick = [this] { if (onBankSelected != nullptr) onBankSelected (1); };

        addAndMakeVisible (bankAButton);
        addAndMakeVisible (bankBButton);
    }

    void setSelectedBank (int bankIndex)
    {
        bankAButton.setToggleState (bankIndex == 0, juce::dontSendNotification);
        bankBButton.setToggleState (bankIndex == 1, juce::dontSendNotification);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff14171a));
        g.fillRect (getLocalBounds());

        g.setColour (juce::Colour (0xff2c313a));
        g.drawLine ((float) getWidth() - 1, 0, (float) getWidth() - 1, (float) getHeight(), 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10);

        modeSectionLabel.setBounds (area.removeFromTop (16));
        area.removeFromTop (6);
        racksModeButton.setBounds (area.removeFromTop (34));

        area.removeFromTop (24);

        layersSectionLabel.setBounds (area.removeFromTop (16));
        area.removeFromTop (6);
        bankAButton.setBounds (area.removeFromTop (34));
        area.removeFromTop (6);
        bankBButton.setBounds (area.removeFromTop (34));
    }

    std::function<void (int bankIndex)> onBankSelected;

private:
    juce::Label modeSectionLabel;
    juce::TextButton racksModeButton;

    juce::Label layersSectionLabel;
    juce::TextButton bankAButton, bankBButton;
};
