#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"

// Panneau "OUTPUT" complet : titre, selecteur, vumetre centre sur le meme
// axe que le knob de gain en dessous, graduations a gauche du vumetre,
// MUTE tout en bas (coupe la sortie du canal).
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
                                      juce::MathConstants<float>::pi * 2.76f, true);
        gainKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        gainKnob.setRange (-60.0, 12.0, 0.1);
        gainKnob.setDoubleClickReturnValue (true, 0.0);
        addAndMakeVisible (gainKnob);

        muteButton.setButtonText ("MUTE");
        muteButton.setClickingTogglesState (true);
        muteButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::red);
        muteButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        addAndMakeVisible (muteButton);

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
        muteButton.onClick = nullptr;
        outputCombo.onChange = nullptr;

        if (channel != nullptr)
        {
            gainKnob.setValue (channel->getOutputGainDb(), juce::dontSendNotification);
            muteButton.setToggleState (channel->isOutputMuted(), juce::dontSendNotification);
            outputCombo.setSelectedId (channel->getOutputChannelIndex() + 1, juce::dontSendNotification);

            gainKnob.onValueChange = [this]
            {
                if (channel != nullptr)
                {
                    channel->setOutputGainDb ((float) gainKnob.getValue());

                    if (onChannelChanged != nullptr)
                        onChannelChanged();

                    if (onOutputGainChanged != nullptr)
                        onOutputGainChanged();
                }
            };
            muteButton.onClick = [this]
            {
                if (channel != nullptr)
                {
                    channel->setOutputMuted (muteButton.getToggleState());

                    if (onChannelChanged != nullptr)
                        onChannelChanged();

                    if (onMuteChanged != nullptr)
                        onMuteChanged();
                }
            };
            outputCombo.onChange = [this]
            {
                if (channel != nullptr)
                {
                    channel->setOutputChannelIndex (outputCombo.getSelectedId() - 1);

                    if (onChannelChanged != nullptr)
                        onChannelChanged();
                }
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
        auto panelCentreX = area.getCentreX();

        titleLabel.setBounds (area.removeFromTop (18));
        area.removeFromTop (6);

        outputCombo.setBounds (area.removeFromTop (26));
        area.removeFromTop (10);

        auto bottomArea = area.removeFromBottom (126);

        constexpr int meterWidth = 34;
        constexpr int scaleGap = 4;
        constexpr int scaleWidth = 28;

        auto meterX = panelCentreX - meterWidth / 2;
        meterBounds = juce::Rectangle<int> (meterX, area.getY(), meterWidth, area.getHeight());

        auto scaleColumn = juce::Rectangle<int> (meterX - scaleGap - scaleWidth, area.getY(), scaleWidth, area.getHeight());

        for (size_t i = 0; i < meterScaleLabels.size(); ++i)
        {
            auto y = meterBounds.getY() + (int) ((float) meterBounds.getHeight() * (float) i / (float) (meterScaleLabels.size() - 1)) - 7;
            meterScaleLabels[i]->setBounds (scaleColumn.getX(), y, scaleColumn.getWidth(), 14);
        }

        auto knobArea = bottomArea.removeFromTop (80);
        auto knobSize = juce::jmin (knobArea.getWidth(), knobArea.getHeight());
        gainKnob.setBounds (knobArea.withSizeKeepingCentre (knobSize, knobSize));

        bottomArea.removeFromTop (16);
        muteButton.setBounds (bottomArea.removeFromTop (30));
    }

    // Appele apres chaque changement (mute, gain, routing de sortie), pour
    // que MainComponent puisse resynchroniser la vignette OVERVIEW.
    std::function<void()> onChannelChanged;

    // Appeles specifiquement apres un changement de gain de sortie ou de
    // MUTE (pas le routing), pour que MainComponent puisse propager la
    // valeur aux chaines liees (Phase B).
    std::function<void()> onOutputGainChanged;
    std::function<void()> onMuteChanged;

private:
    AudioChannel* channel = nullptr;
    float displayedLevel = 0.0f;
    juce::Rectangle<int> meterBounds;

    juce::Label titleLabel;
    juce::ComboBox outputCombo;
    juce::Slider gainKnob;
    juce::TextButton muteButton;
    std::vector<std::unique_ptr<juce::Label>> meterScaleLabels;
};
