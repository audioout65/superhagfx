#include "MainComponent.h"
#include "CompressorEditor.h"

MainComponent::MainComponent()
{
    setAudioChannels (numChannels, 0);

    addAndMakeVisible (deviceSettingsButton);
    deviceSettingsButton.onClick = [this]
    {
        deviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(
            deviceManager, 0, 32, 0, 32, false, false, true, false);
        deviceSelector->setSize (500, 400);

        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned (deviceSelector.release());
        options.dialogTitle = "Audio Device Settings";
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = true;
        options.launchAsync();
    };

    addAndMakeVisible (scanPluginsButton);
    scanPluginsButton.onClick = [this]
    {
        pluginManager.scanForPlugins();
    };

    if (pluginManager.loadCacheIfAvailable())
        DBG ("Plugins charges depuis le cache, pas besoin de rescanner.");

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto bypassBtn = std::make_unique<juce::ToggleButton>("Bypass");
        bypassBtn->onClick = [this, ch, ptr = bypassBtn.get()]
        {
            channels[(size_t) ch].setBypassed (ptr->getToggleState());
        };
        addAndMakeVisible (*bypassBtn);
        bypassButtons[(size_t) ch] = std::move (bypassBtn);

        auto slider = std::make_unique<juce::Slider>(juce::Slider::LinearVertical, juce::Slider::TextBoxBelow);
        slider->setRange (0.0, 2.0, 0.01);
        slider->setValue (1.0);
        slider->onValueChange = [this, ch, ptr = slider.get()]
        {
            channels[(size_t) ch].setGain ((float) ptr->getValue());
        };
        addAndMakeVisible (*slider);
        gainSliders[(size_t) ch] = std::move (slider);

        for (int slot = 0; slot < numSlotsPerChannel; ++slot)
        {
            auto slotBtn = std::make_unique<SlotButton>("+");

            slotBtn->onClick = [this, ch, slot]
            {
                handleSlotClick (ch, slot);
            };

            slotBtn->onRightClick = [this, ch, slot]
            {
                showSlotMenu (ch, slot);
            };

            addAndMakeVisible (*slotBtn);
            slotButtons[(size_t) ch][(size_t) slot] = std::move (slotBtn);
        }
    }

    setSize (900, 650);
    startTimerHz (30);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::handleSlotClick (int channelIndex, int slotIndex)
{
    auto& slot = channels[(size_t) channelIndex].getSlot (slotIndex);

    if (slot.isEmpty())
        showSlotMenu (channelIndex, slotIndex);
    else
        openSlotEditor (channelIndex, slotIndex);
}

void MainComponent::showSlotMenu (int channelIndex, int slotIndex)
{
    juce::PopupMenu menu;

    menu.addSectionHeader ("Effets internes");
    menu.addItem (1, "Compressor");

    menu.addSeparator();
    menu.addItem (2, "Vider cet emplacement");

    menu.showMenuAsync (juce::PopupMenu::Options(), [this, channelIndex, slotIndex] (int result)
    {
        auto& slot = channels[(size_t) channelIndex].getSlot (slotIndex);

        if (result == 1)
        {
            slot.loadInternalCompressor();
        }
        else if (result == 2)
        {
            slot.clear();
        }

        refreshSlotButtonText (channelIndex, slotIndex);
    });
}

void MainComponent::openSlotEditor (int channelIndex, int slotIndex)
{
    auto& slot = channels[(size_t) channelIndex].getSlot (slotIndex);

    if (slot.isEmpty())
        return;

    auto editor = std::make_unique<CompressorEditor>(slot.getCompressor());

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (editor.release());
    options.dialogTitle = "Ch " + juce::String (channelIndex + 1) + " - " + slot.getDisplayName();
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}

void MainComponent::refreshSlotButtonText (int channelIndex, int slotIndex)
{
    auto& slot = channels[(size_t) channelIndex].getSlot (slotIndex);
    slotButtons[(size_t) channelIndex][(size_t) slotIndex]->setButtonText (slot.getDisplayName());
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    for (auto& channel : channels)
        channel.prepare (sampleRate, samplesPerBlockExpected);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto numAvailable = bufferToFill.buffer->getNumChannels();

    for (int ch = 0; ch < numChannels && ch < numAvailable; ++ch)
    {
        auto* samples = bufferToFill.buffer->getWritePointer (ch, bufferToFill.startSample);
        channels[(size_t) ch].process (samples, bufferToFill.numSamples);
    }
}

void MainComponent::releaseResources()
{
}

void MainComponent::timerCallback()
{
    bool needsRepaint = false;

    for (int i = 0; i < numChannels; ++i)
    {
        auto target = channels[(size_t) i].getLevel();
        auto& display = displayedLevels[(size_t) i];

        if (target > display)
            display = target;
        else
            display *= 0.85f;

        needsRepaint = true;
    }

    if (needsRepaint)
        repaint();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (90);

    auto channelWidth = area.getWidth() / numChannels;

    g.setColour (juce::Colours::white);
    g.setFont (14.0f);

    for (int i = 0; i < numChannels; ++i)
    {
        auto channelArea = area.removeFromLeft (channelWidth).reduced (8);

        auto labelArea = channelArea.removeFromTop (20);
        g.drawText ("Ch " + juce::String (i + 1), labelArea, juce::Justification::centred);

        auto meterArea = channelArea.removeFromLeft (30);
        meterArea = meterArea.withSizeKeepingCentre (20, 150);

        g.setColour (juce::Colours::darkgrey);
        g.fillRect (meterArea);

        auto level = juce::jlimit (0.0f, 1.0f, displayedLevels[(size_t) i]);
        auto filledHeight = (int) (level * (float) meterArea.getHeight());
        auto filledArea = meterArea.withTop (meterArea.getBottom() - filledHeight);

        g.setColour (level > 0.9f ? juce::Colours::red
                                   : (level > 0.7f ? juce::Colours::orange : juce::Colours::green));
        g.fillRect (filledArea);
    }
}

void MainComponent::resized()
{
    deviceSettingsButton.setBounds (20, 10, 220, 30);
    scanPluginsButton.setBounds (260, 10, 220, 30);

    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (90);

    auto channelWidth = area.getWidth() / numChannels;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto channelArea = area.removeFromLeft (channelWidth).reduced (8);
        channelArea.removeFromTop (20);

        auto meterAndGainArea = channelArea.removeFromLeft (60);
        gainSliders[(size_t) ch]->setBounds (meterAndGainArea.withTrimmedLeft (30).removeFromTop (180));
        bypassButtons[(size_t) ch]->setBounds (meterAndGainArea.withTrimmedLeft (30).withY (meterAndGainArea.getY() + 185).withHeight (25));

        auto slotsArea = channelArea.reduced (4);
        slotsArea.removeFromTop (190);

        for (int slot = 0; slot < numSlotsPerChannel; ++slot)
        {
            slotButtons[(size_t) ch][(size_t) slot]->setBounds (slotsArea.removeFromTop (34));
            slotsArea.removeFromTop (4);
        }
    }
}
