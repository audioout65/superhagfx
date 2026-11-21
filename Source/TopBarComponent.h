#pragma once
#include <JuceHeader.h>

// La barre superieure de l'application : logo, onglets de navigation,
// charge CPU, BPM, horloge.
class TopBarComponent : public juce::Component,
                        private juce::Timer
{
public:
    TopBarComponent()
    {
        logoLabel.setText ("SHFX", juce::dontSendNotification);
        logoLabel.setFont (juce::Font (20.0f, juce::Font::bold));
        logoLabel.setColour (juce::Label::textColourId, juce::Colour (0xff4ea1ff));
        logoLabel.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (logoLabel);

        overviewTabButton.setButtonText ("OVERVIEW");
        overviewTabButton.setClickingTogglesState (true);
        overviewTabButton.setRadioGroupId (2001);
        overviewTabButton.setToggleState (true, juce::dontSendNotification);
        overviewTabButton.onClick = [this] { if (onOverviewTabClicked != nullptr) onOverviewTabClicked(); };
        addAndMakeVisible (overviewTabButton);

        rackTabButton.setButtonText ("RACK");
        rackTabButton.setClickingTogglesState (true);
        rackTabButton.setRadioGroupId (2001);
        rackTabButton.onClick = [this] { if (onRackTabClicked != nullptr) onRackTabClicked(); };
        addAndMakeVisible (rackTabButton);

        cpuLabel.setJustificationType (juce::Justification::centred);
        cpuLabel.setFont (12.0f);
        addAndMakeVisible (cpuLabel);

        bpmLabel.setText ("BPM", juce::dontSendNotification);
        bpmLabel.setJustificationType (juce::Justification::centredRight);
        bpmLabel.setFont (12.0f);
        addAndMakeVisible (bpmLabel);

        bpmEditor.setText ("120", juce::dontSendNotification);
        bpmEditor.setJustification (juce::Justification::centred);
        bpmEditor.setInputRestrictions (3, "0123456789");
        addAndMakeVisible (bpmEditor);

        clockLabel.setJustificationType (juce::Justification::centredRight);
        clockLabel.setFont (12.0f);
        addAndMakeVisible (clockLabel);

        startTimerHz (2);
        updateClock();
    }

    void setCpuUsage (double usageRatio)
    {
        auto percent = juce::roundToInt (usageRatio * 100.0);
        cpuLabel.setText ("CPU " + juce::String (percent) + "%", juce::dontSendNotification);

        auto colour = percent > 80 ? juce::Colours::red
                                   : (percent > 50 ? juce::Colours::orange : juce::Colour (0xffe0e0e0));
        cpuLabel.setColour (juce::Label::textColourId, colour);
    }

    double getBpm() const
    {
        return bpmEditor.getText().getDoubleValue();
    }

    void setActiveTab (bool rackTabActive)
    {
        overviewTabButton.setToggleState (! rackTabActive, juce::dontSendNotification);
        rackTabButton.setToggleState (rackTabActive, juce::dontSendNotification);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff14171a));
        g.fillRect (getLocalBounds());

        g.setColour (juce::Colour (0xff2c313a));
        g.drawLine (0, (float) getHeight() - 1, (float) getWidth(), (float) getHeight() - 1, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8, 6);

        logoLabel.setBounds (area.removeFromLeft (60));

        clockLabel.setBounds (area.removeFromRight (70));
        area.removeFromRight (10);

        bpmEditor.setBounds (area.removeFromRight (44));
        area.removeFromRight (4);
        bpmLabel.setBounds (area.removeFromRight (32));
        area.removeFromRight (10);

        cpuLabel.setBounds (area.removeFromRight (64));
        area.removeFromRight (10);

        auto tabsArea = area.withSizeKeepingCentre (juce::jmin (180, area.getWidth()), area.getHeight());
        overviewTabButton.setBounds (tabsArea.removeFromLeft (tabsArea.getWidth() / 2));
        rackTabButton.setBounds (tabsArea);
    }

    std::function<void()> onOverviewTabClicked;
    std::function<void()> onRackTabClicked;

private:
    void timerCallback() override
    {
        updateClock();
    }

    void updateClock()
    {
        auto now = juce::Time::getCurrentTime();
        clockLabel.setText (now.toString (false, true, false, true), juce::dontSendNotification);
    }

    juce::Label logoLabel, cpuLabel, bpmLabel, clockLabel;
    juce::TextEditor bpmEditor;
    juce::TextButton overviewTabButton, rackTabButton;
};
