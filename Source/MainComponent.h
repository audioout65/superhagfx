#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"
#include "PluginManager.h"
#include "ChannelStripComponent.h"

static constexpr int numChannels = 32;
static constexpr int channelsPerBank = 16;

class ChannelsHolder : public juce::Component
{
public:
    void resized() override
    {
        auto area = getLocalBounds();
        auto stripWidth = area.getWidth() / juce::jmax (1, (int) strips.size());

        for (auto* strip : strips)
            strip->setBounds (area.removeFromLeft (stripWidth));
    }

    std::vector<ChannelStripComponent*> strips;
};

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
    void showSlotMenu (int channelIndex, int slotIndex);
    void handleSlotClick (int channelIndex, int slotIndex);
    void openSlotEditor (int channelIndex, int slotIndex);
    void refreshSlotButtonText (int channelIndex, int slotIndex);
    void loadVst3IntoSlot (int channelIndex, int slotIndex, int pluginListIndex);
    void showBank (int bankIndex);

    std::array<AudioChannel, numChannels> channels;

    juce::TextButton deviceSettingsButton { "Audio Device Settings..." };
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;

    juce::TextButton bankAButton { "1-16" };
    juce::TextButton bankBButton { "17-32" };
    int currentBank = 0;

    PluginManager pluginManager;
    juce::TextButton scanPluginsButton { "Scanner les plugins VST3" };

    ChannelsHolder channelsHolder;
    std::vector<std::unique_ptr<ChannelStripComponent>> channelStrips;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
