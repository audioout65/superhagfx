#pragma once
#include <JuceHeader.h>

// Vue PATCH : gestion des Link Groups (les chaines liees entre elles).
// Pour l'instant : squelette vide, le contenu arrive a la prochaine etape.
class PatchView : public juce::Component
{
public:
    PatchView()
    {
        titleLabel.setText ("PATCH", juce::dontSendNotification);
        titleLabel.setFont (juce::Font (20.0f, juce::Font::bold));
        titleLabel.setJustificationType (juce::Justification::centred);
        titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a92a0));
        addAndMakeVisible (titleLabel);

        subtitleLabel.setText ("Link Groups arrivent ici.", juce::dontSendNotification);
        subtitleLabel.setFont (juce::Font (14.0f));
        subtitleLabel.setJustificationType (juce::Justification::centred);
        subtitleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff5a6170));
        addAndMakeVisible (subtitleLabel);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff181b1f));
    }

    void resized() override
    {
        auto area = getLocalBounds();
        auto centreY = area.getHeight() / 2;
        titleLabel.setBounds (area.withY (centreY - 30).withHeight (30));
        subtitleLabel.setBounds (area.withY (centreY + 2).withHeight (24));
    }

private:
    juce::Label titleLabel;
    juce::Label subtitleLabel;
};
