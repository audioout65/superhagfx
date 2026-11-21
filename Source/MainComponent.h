#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"
#include "PluginManager.h"
#include "ChannelStripComponent.h"
#include "ChannelDetailView.h"
#include "TopBarComponent.h"

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

    // Appele depuis le menu natif de la barre de menu (voir MainMenu).
    void handleMenuCommand (int menuItemID);

private:
    void timerCallback() override;
    void showSlotMenu (int channelIndex, int slotIndex);
    void handleSlotClick (int channelIndex, int slotIndex);
    void openSlotEditor (int channelIndex, int slotIndex);
    void refreshSlotButtonText (int channelIndex, int slotIndex);
    void loadVst3IntoSlot (int channelIndex, int slotIndex, int pluginListIndex);
    void showBank (int bankIndex);
    void saveSession();
    void loadSession();
    void openAudioSettings();
    void openChannelDetail (int channelIndex);
    void closeChannelDetail();

    std::array<AudioChannel, numChannels> channels;

    TopBarComponent topBar;

    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;

    juce::TextButton bankAButton { "1-16" };
    juce::TextButton bankBButton { "17-32" };
    int currentBank = 0;

    std::unique_ptr<juce::FileChooser> fileChooser;

    PluginManager pluginManager;

    ChannelsHolder channelsHolder;
    std::vector<std::unique_ptr<ChannelStripComponent>> channelStrips;

    ChannelDetailView channelDetailView;
    bool detailViewOpen = false;
    int detailChannelIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
