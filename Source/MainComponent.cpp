#include "MainComponent.h"

MainComponent::MainComponent()
{
    setAudioChannels (2, 0); // 2 entrées pour commencer, 0 sortie (pas besoin de sortie pour ce test)

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

    setSize (600, 400);
    startTimerHz (30);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay (int /*samplesPerBlockExpected*/, double /*sampleRate*/)
{
    auto numInputs = deviceManager.getCurrentAudioDevice() != nullptr
                        ? deviceManager.getCurrentAudioDevice()->getActiveInputChannels().countNumberOfSetBits()
                        : 2;

    currentLevels.assign ((size_t) juce::jmax (2, numInputs), 0.0f);
    displayedLevels.assign (currentLevels.size(), 0.0f);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto numChannels = bufferToFill.buffer->getNumChannels();

    if ((int) currentLevels.size() != numChannels)
        currentLevels.assign ((size_t) numChannels, 0.0f);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto magnitude = bufferToFill.buffer->getMagnitude (ch, bufferToFill.startSample, bufferToFill.numSamples);
        currentLevels[(size_t) ch] = magnitude;
    }
}

void MainComponent::releaseResources()
{
}

void MainComponent::timerCallback()
{
    bool needsRepaint = false;

    if (displayedLevels.size() != currentLevels.size())
        displayedLevels.assign (currentLevels.size(), 0.0f);

    for (size_t i = 0; i < currentLevels.size(); ++i)
    {
        auto target = currentLevels[i];
        auto& display = displayedLevels[i];

        if (target > display)
            display = target;
        else
            display *= 0.85f; // chute douce du vumètre

        needsRepaint = true;
    }

    if (needsRepaint)
        repaint();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (50); // place pour le bouton

    g.setColour (juce::Colours::white);
    g.setFont (16.0f);

    if (displayedLevels.empty())
    {
        g.drawText ("Aucune entree audio detectee. Verifie la SQ-7 dans les reglages.",
                    area, juce::Justification::centred);
        return;
    }

    auto meterWidth = juce::jmax (20, area.getWidth() / (int) displayedLevels.size() - 10);

    for (size_t i = 0; i < displayedLevels.size(); ++i)
    {
        auto x = area.getX() + (int) i * (meterWidth + 10);
        auto meterArea = juce::Rectangle<int> (x, area.getY(), meterWidth, area.getHeight());

        g.setColour (juce::Colours::darkgrey);
        g.fillRect (meterArea);

        auto level = juce::jlimit (0.0f, 1.0f, displayedLevels[i]);
        auto filledHeight = (int) (level * (float) meterArea.getHeight());

        auto filledArea = meterArea.withTop (meterArea.getBottom() - filledHeight);

        g.setColour (level > 0.9f ? juce::Colours::red
                                   : (level > 0.7f ? juce::Colours::orange : juce::Colours::green));
        g.fillRect (filledArea);

        g.setColour (juce::Colours::white);
        g.drawText ("Ch " + juce::String ((int) i + 1),
                    meterArea.withTop (meterArea.getBottom() - 20),
                    juce::Justification::centred);
    }
}

void MainComponent::resized()
{
    deviceSettingsButton.setBounds (20, 10, 220, 30);
}
