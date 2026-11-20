#include "MainComponent.h"

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

    for (int i = 0; i < numChannels; ++i)
    {
        auto bypassBtn = std::make_unique<juce::ToggleButton>("Bypass");
        bypassBtn->onClick = [this, i, ptr = bypassBtn.get()]
        {
            channels[(size_t) i].setBypassed (ptr->getToggleState());
        };
        addAndMakeVisible (*bypassBtn);
        bypassButtons[(size_t) i] = std::move (bypassBtn);

        auto slider = std::make_unique<juce::Slider>(juce::Slider::LinearVertical, juce::Slider::TextBoxBelow);
        slider->setRange (0.0, 2.0, 0.01);
        slider->setValue (1.0);
        slider->onValueChange = [this, i, ptr = slider.get()]
        {
            channels[(size_t) i].setGain ((float) ptr->getValue());
        };
        addAndMakeVisible (*slider);
        gainSliders[(size_t) i] = std::move (slider);

        auto thresholdLabel = std::make_unique<juce::Label>("", "Threshold");
        thresholdLabel->setJustificationType (juce::Justification::centred);
        thresholdLabel->setFont (11.0f);
        addAndMakeVisible (*thresholdLabel);
        thresholdLabels[(size_t) i] = std::move (thresholdLabel);

        auto thresholdSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
        thresholdSlider->setRange (-60.0, 0.0, 0.1);
        thresholdSlider->setValue (-18.0);
        thresholdSlider->onValueChange = [this, i, ptr = thresholdSlider.get()]
        {
            channels[(size_t) i].getCompressor().setThresholdDb ((float) ptr->getValue());
        };
        addAndMakeVisible (*thresholdSlider);
        thresholdSliders[(size_t) i] = std::move (thresholdSlider);

        auto ratioLabel = std::make_unique<juce::Label>("", "Ratio");
        ratioLabel->setJustificationType (juce::Justification::centred);
        ratioLabel->setFont (11.0f);
        addAndMakeVisible (*ratioLabel);
        ratioLabels[(size_t) i] = std::move (ratioLabel);

        auto ratioSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
        ratioSlider->setRange (1.0, 20.0, 0.1);
        ratioSlider->setValue (4.0);
        ratioSlider->onValueChange = [this, i, ptr = ratioSlider.get()]
        {
            channels[(size_t) i].getCompressor().setRatio ((float) ptr->getValue());
        };
        addAndMakeVisible (*ratioSlider);
        ratioSliders[(size_t) i] = std::move (ratioSlider);
    }

    setSize (900, 600);
    startTimerHz (30);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
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

    for (int i = 0; i < numChannels; ++i)
    {
        auto channelArea = area.removeFromLeft (channelWidth).reduced (8);
        channelArea.removeFromTop (20);

        auto meterAndGainArea = channelArea.removeFromLeft (60);
        gainSliders[(size_t) i]->setBounds (meterAndGainArea.withTrimmedLeft (30).removeFromTop (180));
        bypassButtons[(size_t) i]->setBounds (meterAndGainArea.withTrimmedLeft (30).withY (meterAndGainArea.getY() + 185).withHeight (25));

        auto controlsArea = channelArea.reduced (4);
        controlsArea.removeFromTop (190);

        thresholdLabels[(size_t) i]->setBounds (controlsArea.removeFromTop (16));
        thresholdSliders[(size_t) i]->setBounds (controlsArea.removeFromTop (24));

        controlsArea.removeFromTop (8);

        ratioLabels[(size_t) i]->setBounds (controlsArea.removeFromTop (16));
        ratioSliders[(size_t) i]->setBounds (controlsArea.removeFromTop (24));
    }
}
