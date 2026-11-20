#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"
#include "PluginManager.h"

static constexpr int numChannels = 4;

class MainComponent : public juce::AudioAppComponent,
                       private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    std::array<AudioChannel, numChannels> channels;
    std::array<float, numChannels> displayedLevels {};

    juce::TextButton deviceSettingsButton { "Audio Device Settings..." };
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;

    std::array<std::unique_ptr<juce::ToggleButton>, numChannels> bypassButtons;
    std::array<std::unique_ptr<juce::Slider>, numChannels> gainSliders;
    std::array<std::unique_ptr<juce::Slider>, numChannels> thresholdSliders;
    std::array<std::unique_ptr<juce::Slider>, numChannels> ratioSliders;
    std::array<std::unique_ptr<juce::Label>, numChannels> thresholdLabels;
    std::array<std::unique_ptr<juce::Label>, numChannels> ratioLabels;

    PluginManager pluginManager;
    juce::TextButton scanPluginsButton { "Scanner les plugins VST3" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
