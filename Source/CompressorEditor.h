#pragma once
#include <JuceHeader.h>
#include "Compressor.h"

// Petite fenetre de controle pour notre compresseur interne.
class CompressorEditor : public juce::Component
{
public:
    explicit CompressorEditor (Compressor& compressorToControl)
        : compressor (compressorToControl)
    {
        thresholdLabel.setText ("Threshold (dB)", juce::dontSendNotification);
        addAndMakeVisible (thresholdLabel);

        thresholdSlider.setRange (-60.0, 0.0, 0.1);
        thresholdSlider.setValue (compressor.getThresholdDb());
        thresholdSlider.onValueChange = [this]
        {
            compressor.setThresholdDb ((float) thresholdSlider.getValue());
        };
        addAndMakeVisible (thresholdSlider);

        ratioLabel.setText ("Ratio", juce::dontSendNotification);
        addAndMakeVisible (ratioLabel);

        ratioSlider.setRange (1.0, 20.0, 0.1);
        ratioSlider.setValue (compressor.getRatio());
        ratioSlider.onValueChange = [this]
        {
            compressor.setRatio ((float) ratioSlider.getValue());
        };
        addAndMakeVisible (ratioSlider);

        attackLabel.setText ("Attack (ms)", juce::dontSendNotification);
        addAndMakeVisible (attackLabel);

        attackSlider.setRange (1.0, 200.0, 0.1);
        attackSlider.setValue (compressor.getAttackMs());
        attackSlider.onValueChange = [this]
        {
            compressor.setAttackMs ((float) attackSlider.getValue());
        };
        addAndMakeVisible (attackSlider);

        releaseLabel.setText ("Release (ms)", juce::dontSendNotification);
        addAndMakeVisible (releaseLabel);

        releaseSlider.setRange (10.0, 1000.0, 1.0);
        releaseSlider.setValue (compressor.getReleaseMs());
        releaseSlider.onValueChange = [this]
        {
            compressor.setReleaseMs ((float) releaseSlider.getValue());
        };
        addAndMakeVisible (releaseSlider);

        makeupLabel.setText ("Makeup Gain (dB)", juce::dontSendNotification);
        addAndMakeVisible (makeupLabel);

        makeupSlider.setRange (-12.0, 24.0, 0.1);
        makeupSlider.setValue (compressor.getMakeupGainDb());
        makeupSlider.onValueChange = [this]
        {
            compressor.setMakeupGainDb ((float) makeupSlider.getValue());
        };
        addAndMakeVisible (makeupSlider);

        setSize (300, 320);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (12);

        auto row = [&area] (int height)
        {
            return area.removeFromTop (height);
        };

        thresholdLabel.setBounds (row (18));
        thresholdSlider.setBounds (row (36));
        area.removeFromTop (8);

        ratioLabel.setBounds (row (18));
        ratioSlider.setBounds (row (36));
        area.removeFromTop (8);

        attackLabel.setBounds (row (18));
        attackSlider.setBounds (row (36));
        area.removeFromTop (8);

        releaseLabel.setBounds (row (18));
        releaseSlider.setBounds (row (36));
        area.removeFromTop (8);

        makeupLabel.setBounds (row (18));
        makeupSlider.setBounds (row (36));
    }

private:
    Compressor& compressor;

    juce::Label thresholdLabel, ratioLabel, attackLabel, releaseLabel, makeupLabel;
    juce::Slider thresholdSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Slider ratioSlider     { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Slider attackSlider    { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Slider releaseSlider   { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Slider makeupSlider    { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
};
