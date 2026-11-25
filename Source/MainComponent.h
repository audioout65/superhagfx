#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"
#include "PluginManager.h"
#include "ChannelStripComponent.h"
#include "ChannelDetailView.h"
#include "TopBarComponent.h"
#include "MeterAndGainOverlay.h"
#include "InputPanelComponent.h"
#include "LeftSidebarComponent.h"
#include "HotPluginsPanel.h"
#include "HotPluginSlot.h"
#include "ShowView.h"
#include "PatchView.h"

static constexpr int numChannels = 32;
static constexpr int channelsPerBank = 16;

class ChannelsHolder : public juce::Component
{
public:
    void resized() override
    {
        auto area = getLocalBounds();
        if (strips.empty())
            return;
        auto width = area.getWidth() / (int) strips.size();
        for (auto* strip : strips)
            strip->setBounds (area.removeFromLeft (width));
    }
    std::vector<ChannelStripComponent*> strips;
};

struct LinkGroup
{
    juce::String name;
    bool isOn = false;
    std::array<bool, numChannels> members {};
};

class MainComponent : public juce::AudioAppComponent, private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void handleMenuCommand (int menuItemID);

private:
    void timerCallback() override;
    void showSlotMenu (int channelIndex, int slotIndex);
    void handleSlotClick (int channelIndex, int slotIndex);
    void openSlotEditor (int channelIndex, int slotIndex);
    void refreshSlotButtonText (int channelIndex, int slotIndex);
    void loadVst3IntoSlot (int channelIndex, int slotIndex, int pluginListIndex);
    void addCurrentSlotToHotPlugins (int channelIndex, int slotIndex);
    void loadHotPluginIntoSlot (int channelIndex, int slotIndex, int hotIndex);
    void showHotPluginPicker (int hotIndex);
    void renameHotPlugin (int hotIndex);
    void clearHotPlugin (int hotIndex);
    void showBank (int bankIndex);
    void showCustomLayer (const std::array<bool, numChannels>& selection);
    void saveSession();
    void loadSession();
    void openAudioSettings();
    void openChannelDetail (int channelIndex);
    void closeChannelDetail();
    void showShowTab();
    void showPatchTab();
    void updatePanelPositions();
    void growWindowToFitIfNeeded();
    void setInterfaceLocked (bool shouldBeLocked);
    void changeBufferSize (int newBufferSize);
    void updateSelectedChannelDisplay (int channelIndex);
    void navigateSelectedChannel (int direction);
    void showChannelPicker();

    void writeSessionXmlTo (const juce::File& file);
    void loadSessionXmlFrom (const juce::File& file);
    void recallSnapshotFast (const juce::File& file);
    void promptForNameAndSave (const juce::String& dialogTitle, const juce::File& targetFolder,
                               std::function<void (juce::File)> saveFunction, std::function<void()> afterSave);
    void confirmAndOverwriteFile (const juce::File& file, const juce::String& message, std::function<void (juce::File)> saveFunction);
    juce::File getSnapshotsFolder();
    void refreshShowView();
    void confirmAndDeleteFile (const juce::File& file, const juce::String& message);
    void refreshChannelLabels();
    void loadLinkGroupsFromXml (const juce::XmlElement& rootXml);

    juce::File getPatchSnapshotsFolder();
    void writePatchSnapshotXmlTo (const juce::File& file);
    void recallPatchSnapshot (const juce::File& file);
    void showPatchSnapshotPicker();

    std::array<AudioChannel, numChannels> channels;
    TopBarComponent topBar;
    LeftSidebarComponent leftSidebar;
    HotPluginsPanel hotPluginsPanel;
    std::array<HotPluginSlot, HotPluginsPanel::numSlots> hotPlugins;
    int currentBank = 0;
    bool interfaceLocked = false;
    int selectedChannelIndex = 0;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;
    std::unique_ptr<juce::FileChooser> fileChooser;
    PluginManager pluginManager;
    ChannelsHolder channelsHolder;
    std::vector<std::unique_ptr<ChannelStripComponent>> channelStrips;
    ChannelDetailView channelDetailView;
    InputPanelComponent inputPanel;
    MeterAndGainOverlay outputPanel;
    bool detailViewOpen = false;
    int detailChannelIndex = 0;
    ShowView showView;
    PatchView patchView;
    std::array<LinkGroup, PatchView::numLinkGroups> linkGroups;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
