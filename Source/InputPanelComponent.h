#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"

// Panneau "INPUT" : titre, selecteur, vumetre centre sur le meme axe que le
// knob de gain d'entree en dessous, et le bouton IN (vert quand actif) tout
// en bas, qui active ou coupe la reception du signal.
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

        gainKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        gainKnob.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                      juce::MathConstants<float>::pi * 2.76f, true);
        gainKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        gainKnob.setRange (-60.0, 12.0, 0.1);
        gainKnob.setDoubleClickReturnValue (true, 0.0);
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
            lbl->setJustificationType (juce::Justification::centredLeft);
            lbl->setColour (juce::Label::textColourId, juce::Colour (0xff6a7280));
            addAndMakeVisible (*lbl);
            meterScaleLabels.push_back (std::move (lbl));
        }
    }

    void setChannel (AudioChannel* newChannel)
    {
        channel = newChannel;

        inputCombo.onChange = nullptr;
        gainKnob.onValueChange = nullptr;
        inButton.onClick = nullptr;

        if (channel != nullptr)
        {
            inputCombo.setSelectedId (channel->getInputChannelIndex() + 1, juce::dontSendNotification);
            gainKnob.setValue (channel->getInputGainDb(), juce::dontSendNotification);
            inButton.setToggleState (channel->isInputEnabled(), juce::dontSendNotification);

            inputCombo.onChange = [this]
            {
                if (channel != nullptr)
                    channel->setInputChannelIndex (inputCombo.getSelectedId() - 1);
            };
            gainKnob.onValueChange = [this]
            {
                if (channel != nullptr)
                {
                    channel->setInputGainDb ((float) gainKnob.getValue());

                    if (onInputGainChanged != nullptr)
                        onInputGainChanged();
                }
            };
            inButton.onClick = [this]
            {
                if (channel != nullptr)
                {
                    channel->setInputEnabled (inButton.getToggleState());

                    if (onInputEnabledChanged != nullptr)
                        onInputEnabledChanged();
                }
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
        auto panelCentreX = area.getCentreX();

        titleLabel.setBounds (area.removeFromTop (18));
        area.removeFromTop (6);

        inputCombo.setBounds (area.removeFromTop (26));
        area.removeFromTop (10);

        auto bottomArea = area.removeFromBottom (126);

        constexpr int meterWidth = 34;
        constexpr int scaleGap = 4;
        constexpr int scaleWidth = 28;

        auto meterX = panelCentreX - meterWidth / 2;
        meterBounds = juce::Rectangle<int> (meterX, area.getY(), meterWidth, area.getHeight());

        auto scaleColumn = juce::Rectangle<int> (meterX + meterWidth + scaleGap, area.getY(), scaleWidth, area.getHeight());

        for (size_t i = 0; i < meterScaleLabels.size(); ++i)
        {
            auto y = meterBounds.getY() + (int) ((float) meterBounds.getHeight() * (float) i / (float) (meterScaleLabels.size() - 1)) - 7;
            meterScaleLabels[i]->setBounds (scaleColumn.getX(), y, scaleColumn.getWidth(), 14);
        }

        auto knobArea = bottomArea.removeFromTop (80);
        auto knobSize = juce::jmin (knobArea.getWidth(), knobArea.getHeight());
        gainKnob.setBounds (knobArea.withSizeKeepingCentre (knobSize, knobSize));

        bottomArea.removeFromTop (16);
        inButton.setBounds (bottomArea.removeFromTop (30));
    }

    // Appeles apres un changement de gain d'entree ou d'activation IN, pour
    // que MainComponent puisse propager la valeur aux chaines liees (Phase B).
    // Le routing (selecteur In 1-32) n'est volontairement PAS concerne : c'est
    // un reglage materiel propre a chaque chaine, qui ne doit jamais etre
    // partage entre chaines liees.
    std::function<void()> onInputGainChanged;
    std::function<void()> onInputEnabledChanged;

private:
    AudioChannel* channel = nullptr;
    float displayedLevel = 0.0f;
    juce::Rectangle<int> meterBounds;

    juce::Label titleLabel;
    juce::ComboBox inputCombo;
    juce::Slider gainKnob;
    juce::TextButton inButton;
    std::vector<std::unique_ptr<juce::Label>> meterScaleLabels;
};
