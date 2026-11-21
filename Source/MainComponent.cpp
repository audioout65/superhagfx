#include "MainComponent.h"
#include "CompressorEditor.h"
#include "MainMenu.h"

MainComponent::MainComponent()
{
    setAudioChannels (32, 32);

    addAndMakeVisible (topBar);
    topBar.onOverviewTabClicked = [this] { closeChannelDetail(); };
    topBar.onRackTabClicked = [this] { openChannelDetail (detailChannelIndex); };

    if (pluginManager.loadCacheIfAvailable())
        DBG ("Plugins charges depuis le cache, pas besoin de rescanner.");

    addAndMakeVisible (bankAButton);
    addAndMakeVisible (bankBButton);
    bankAButton.setClickingTogglesState (true);
    bankBButton.setClickingTogglesState (true);
    bankAButton.setRadioGroupId (1001);
    bankBButton.setRadioGroupId (1001);
    bankAButton.setToggleState (true, juce::dontSendNotification);

    bankAButton.onClick = [this] { showBank (0); };
    bankBButton.onClick = [this] { showBank (1); };

    for (int ch = 0; ch < numChannels; ++ch)
    {
        channels[(size_t) ch].setInputChannelIndex (ch);
        channels[(size_t) ch].setOutputChannelIndex (ch);

        auto strip = std::make_unique<ChannelStripComponent>(ch, channels[(size_t) ch]);

        strip->onChannelNameClicked = [this] (int channelIndex)
        {
            openChannelDetail (channelIndex);
        };
        strip->onSlotClicked = [this] (int channelIndex, int slotIndex)
        {
            auto& slot = channels[(size_t) channelIndex].getSlot (slotIndex);

            if (slot.isEmpty())
            {
                showSlotMenu (channelIndex, slotIndex);
            }
            else
            {
                openChannelDetail (channelIndex);
                channelDetailView.selectSlotForDisplay (slotIndex);
            }
        };
        strip->onSlotRightClicked = [this] (int channelIndex, int slotIndex)
        {
            showSlotMenu (channelIndex, slotIndex);
        };

        channelsHolder.addChildComponent (*strip);
        channelsHolder.strips.push_back (strip.get());
        channelStrips.push_back (std::move (strip));
    }

    addAndMakeVisible (channelsHolder);

    channelDetailView.onPrevious = [this]
    {
        if (detailChannelIndex > 0)
            openChannelDetail (detailChannelIndex - 1);
    };
    channelDetailView.onNext = [this]
    {
        if (detailChannelIndex < numChannels - 1)
            openChannelDetail (detailChannelIndex + 1);
    };
    channelDetailView.onSlotClicked = [this] (int channelIndex, int slotIndex)
    {
        auto& slot = channels[(size_t) channelIndex].getSlot (slotIndex);

        if (slot.isEmpty())
            showSlotMenu (channelIndex, slotIndex);
    };
    channelDetailView.onSlotRightClicked = [this] (int channelIndex, int slotIndex)
    {
        showSlotMenu (channelIndex, slotIndex);
    };
    channelDetailView.onOverlayBoundsChanged = [this]
    {
        updateOverlayPosition();
    };
    addChildComponent (channelDetailView);

    // Ajoute en tout dernier : toujours dessine par-dessus tout le reste,
    // y compris l'interface d'un plugin qui deborderait de son emplacement.
    addChildComponent (meterOverlay);

    showBank (0);

    setSize (1100, 680);
    startTimerHz (30);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::handleMenuCommand (int menuItemID)
{
    switch (menuItemID)
    {
        case MainMenu::scanPlugins:    pluginManager.scanForPlugins(); break;
        case MainMenu::saveSession:    saveSession(); break;
        case MainMenu::loadSession:    loadSession(); break;
        case MainMenu::audioSettings:  openAudioSettings(); break;
        default: break;
    }
}

void MainComponent::openAudioSettings()
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
}

void MainComponent::updateOverlayPosition()
{
    if (! detailViewOpen)
        return;

    // La zone que ChannelDetailView a reservee est exprimee dans ses propres
    // coordonnees ; on la convertit vers celles de MainComponent, puisque
    // meterOverlay est un enfant direct de MainComponent, pas de la vue.
    auto boundsInDetailView = channelDetailView.getOverlayBounds();
    auto topLeftInMain = channelDetailView.getPosition() + boundsInDetailView.getPosition();

    meterOverlay.setBounds (topLeftInMain.x, topLeftInMain.y,
                            boundsInDetailView.getWidth(), boundsInDetailView.getHeight());
}

void MainComponent::openChannelDetail (int channelIndex)
{
    detailChannelIndex = channelIndex;
    detailViewOpen = true;

    channelDetailView.setChannel (channelIndex, channels[(size_t) channelIndex]);
    channelDetailView.setVisible (true);
    channelsHolder.setVisible (false);
    bankAButton.setVisible (false);
    bankBButton.setVisible (false);

    meterOverlay.setChannel (&channels[(size_t) channelIndex]);
    meterOverlay.setVisible (true);

    topBar.setActiveTab (true);

    resized();
}

void MainComponent::closeChannelDetail()
{
    detailViewOpen = false;
    channelDetailView.setVisible (false);
    channelsHolder.setVisible (true);
    bankAButton.setVisible (true);
    bankBButton.setVisible (true);

    meterOverlay.setVisible (false);
    meterOverlay.setChannel (nullptr);

    topBar.setActiveTab (false);

    resized();
}

void MainComponent::saveSession()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Sauvegarder la session",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.liverack");

    auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();

        if (file == juce::File())
            return;

        if (! file.hasFileExtension (".liverack"))
            file = file.withFileExtension (".liverack");

        juce::XmlElement root ("LiveRackSession");

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* channelXml = root.createNewChildElement ("Channel");
            channelXml->setAttribute ("index", ch);
            channels[(size_t) ch].saveToXml (*channelXml);
        }

        if (root.writeTo (file))
            DBG ("Session sauvegardee : " << file.getFullPathName());
        else
            DBG ("Echec de la sauvegarde de session.");
    });
}

void MainComponent::loadSession()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Charger une session",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.liverack");

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();

        if (file == juce::File())
            return;

        auto xml = juce::parseXML (file);

        if (xml == nullptr || ! xml->hasTagName ("LiveRackSession"))
        {
            DBG ("Fichier de session invalide.");
            return;
        }

        for (auto* channelXml : xml->getChildWithTagNameIterator ("Channel"))
        {
            auto index = channelXml->getIntAttribute ("index", -1);

            if (index < 0 || index >= numChannels)
                continue;

            channels[(size_t) index].loadFromXmlAsync (*channelXml, pluginManager.getFormatManager(),
                [this, index]
            {
                channelStrips[(size_t) index]->refreshFromChannel();

                if (detailViewOpen && detailChannelIndex == index)
                    channelDetailView.refreshSlots();
            });
        }

        DBG ("Session en cours de chargement...");
    });
}

void MainComponent::showBank (int bankIndex)
{
    currentBank = bankIndex;
    auto firstVisible = bankIndex * channelsPerBank;
    auto lastVisible = firstVisible + channelsPerBank;

    channelsHolder.strips.clear();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto isVisible = (ch >= firstVisible && ch < lastVisible);
        channelStrips[(size_t) ch]->setVisible (isVisible);

        if (isVisible)
            channelsHolder.strips.push_back (channelStrips[(size_t) ch].get());
    }

    channelsHolder.resized();
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

    const auto& plugins = pluginManager.getScannedPlugins();

    if (! plugins.empty())
    {
        menu.addSeparator();
        menu.addSectionHeader ("Plugins VST3 (" + juce::String ((int) plugins.size()) + ")");

        std::map<juce::String, std::vector<int>> byManufacturer;

        for (int i = 0; i < (int) plugins.size(); ++i)
        {
            auto manufacturer = plugins[(size_t) i].manufacturerName.isNotEmpty()
                                    ? plugins[(size_t) i].manufacturerName
                                    : juce::String ("Autre");
            byManufacturer[manufacturer].push_back (i);
        }

        for (auto& entry : byManufacturer)
        {
            juce::PopupMenu subMenu;

            for (auto pluginIndex : entry.second)
                subMenu.addItem (1000 + pluginIndex, plugins[(size_t) pluginIndex].name);

            menu.addSubMenu (entry.first, subMenu);
        }
    }

    menu.addSeparator();
    menu.addItem (2, "Vider cet emplacement");

    menu.showMenuAsync (juce::PopupMenu::Options(), [this, channelIndex, slotIndex] (int result)
    {
        auto& slot = channels[(size_t) channelIndex].getSlot (slotIndex);

        if (result == 1)
        {
            slot.loadInternalCompressor();
            refreshSlotButtonText (channelIndex, slotIndex);
        }
        else if (result == 2)
        {
            slot.clear();
            refreshSlotButtonText (channelIndex, slotIndex);
        }
        else if (result >= 1000)
        {
            loadVst3IntoSlot (channelIndex, slotIndex, result - 1000);
        }
    });
}

void MainComponent::loadVst3IntoSlot (int channelIndex, int slotIndex, int pluginListIndex)
{
    const auto& plugins = pluginManager.getScannedPlugins();

    if (pluginListIndex < 0 || pluginListIndex >= (int) plugins.size())
        return;

    const auto& desc = plugins[(size_t) pluginListIndex];
    auto& slot = channels[(size_t) channelIndex].getSlot (slotIndex);

    channelStrips[(size_t) channelIndex]->refreshSlotButtonText (slotIndex, "Chargement...");

    slot.loadVst3Async (desc, pluginManager.getFormatManager(),
                        [this, channelIndex, slotIndex] (bool success, juce::String error)
    {
        if (! success)
        {
            DBG ("Echec du chargement VST3 : " << error);
            channelStrips[(size_t) channelIndex]->refreshSlotButtonText (slotIndex, "+");
            return;
        }

        refreshSlotButtonText (channelIndex, slotIndex);

        if (detailViewOpen && detailChannelIndex == channelIndex)
            channelDetailView.refreshSlots();
    });
}

void MainComponent::openSlotEditor (int channelIndex, int slotIndex)
{
    auto& slot = channels[(size_t) channelIndex].getSlot (slotIndex);

    if (slot.isEmpty())
        return;

    if (slot.isVst3())
    {
        auto* instance = slot.getVstInstance();

        if (instance == nullptr)
            return;

        auto* editor = instance->createEditorIfNeeded();

        if (editor == nullptr)
        {
            DBG ("Ce plugin n'a pas d'interface graphique.");
            return;
        }

        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned (editor);
        options.dialogTitle = "Ch " + juce::String (channelIndex + 1) + " - " + slot.getDisplayName();
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = true;
        options.launchAsync();

        return;
    }

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
    channelStrips[(size_t) channelIndex]->refreshSlotButtonText (slotIndex, slot.getDisplayName());
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    for (auto& channel : channels)
        channel.prepare (sampleRate, samplesPerBlockExpected);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto& buffer = *bufferToFill.buffer;
    auto startSample = bufferToFill.startSample;
    auto numSamples = bufferToFill.numSamples;

    for (auto& channel : channels)
        channel.readInput (buffer, startSample, numSamples);

    for (auto& channel : channels)
        channel.processAndWriteOutput (buffer, startSample, numSamples);
}

void MainComponent::releaseResources()
{
}

void MainComponent::timerCallback()
{
    for (auto& strip : channelStrips)
        if (strip->isVisible())
            strip->updateMeter();

    if (detailViewOpen)
        meterOverlay.updateMeter();

    topBar.setCpuUsage (deviceManager.getCpuUsage());
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto fullArea = getLocalBounds();

    auto topBarArea = fullArea.removeFromTop (44);
    topBar.setBounds (topBarArea);

    auto bankBarArea = fullArea.removeFromTop (44);
    auto bankButtonsArea = bankBarArea.removeFromRight (160).reduced (10, 7);
    bankAButton.setBounds (bankButtonsArea.removeFromLeft (70));
    bankButtonsArea.removeFromLeft (10);
    bankBButton.setBounds (bankButtonsArea);

    channelsHolder.setBounds (fullArea);
    channelsHolder.resized();

    channelDetailView.setBounds (fullArea);

    updateOverlayPosition();
}
