#include "MainComponent.h"
#include "MainMenu.h"

MainComponent::MainComponent()
{
    setAudioChannels (32, 32);

    addAndMakeVisible (topBar);
    topBar.onOverviewTabClicked = [this] { closeChannelDetail(); };
    topBar.onRackTabClicked = [this] { openChannelDetail (detailChannelIndex); };
    topBar.onShowTabClicked = [this] { showShowTab(); };
    topBar.onPatchTabClicked = [this] { showPatchTab(); };
    topBar.onSendTabClicked = [this] { showSendTab(); };
    topBar.onLockToggled = [this] (bool locked) { setInterfaceLocked (locked); };
    topBar.onLatencyChanged = [this] (int newBufferSize) { changeBufferSize (newBufferSize); };
    topBar.onPreviousChannel = [this] { navigateSelectedChannel (-1); };
    topBar.onNextChannel = [this] { navigateSelectedChannel (1); };
    topBar.onChannelPickerRequested = [this] { showChannelPicker(); };

    if (pluginManager.loadCacheIfAvailable())
        DBG ("Plugins charges depuis le cache, pas besoin de rescanner.");

    addAndMakeVisible (leftSidebar);
    leftSidebar.onBankSelected = [this] (int bankIndex) { showBank (bankIndex); };
    leftSidebar.onCustomLayerSelected = [this] (const std::array<bool, 32>& selection) { showCustomLayer (selection); };

    addAndMakeVisible (hotPluginsPanel);

    loadHotPlugins (hotPlugins);
    for (int i = 0; i < (int) hotPlugins.size(); ++i)
        hotPluginsPanel.refreshSlot (i, hotPlugins[(size_t) i]);

    hotPluginsPanel.onSlotRightClicked = [this] (int index) { clearHotPlugin (index); };
    hotPluginsPanel.onSlotClicked = [this] (int index) { showHotPluginPicker (index); };
    hotPluginsPanel.onSlotRenameRequested = [this] (int index) { renameHotPlugin (index); };

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
        strip->onStripSelected = [this, ch]
        {
            updateSelectedChannelDisplay (ch);
        };
        strip->onInputGainChanged = [this] (int channelIndex)
        {
            auto value = channels[(size_t) channelIndex].getInputGainDb();
            propagateLinkedChange (channelIndex, [value] (AudioChannel& c) { c.setInputGainDb (value); });
        };
        strip->onInputEnabledChanged = [this] (int channelIndex)
        {
            auto value = channels[(size_t) channelIndex].isInputEnabled();
            propagateLinkedChange (channelIndex, [value] (AudioChannel& c) { c.setInputEnabled (value); });
        };
        strip->onOutputGainChanged = [this] (int channelIndex)
        {
            auto value = channels[(size_t) channelIndex].getOutputGainDb();
            propagateLinkedChange (channelIndex, [value] (AudioChannel& c) { c.setOutputGainDb (value); });
        };
        strip->onMuteChanged = [this] (int channelIndex)
        {
            auto value = channels[(size_t) channelIndex].isOutputMuted();
            propagateLinkedChange (channelIndex, [value] (AudioChannel& c) { c.setOutputMuted (value); });
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
    channelDetailView.onSlotRightClicked = [this] (int channelIndex, int slotIndex)
    {
        showSlotMenu (channelIndex, slotIndex);
    };
    channelDetailView.onPanelBoundsChanged = [this]
    {
        updatePanelPositions();
    };
    addChildComponent (channelDetailView);

    addChildComponent (inputPanel);
    addChildComponent (outputPanel);

    inputPanel.onInputGainChanged = [this]
    {
        auto value = channels[(size_t) detailChannelIndex].getInputGainDb();
        propagateLinkedChange (detailChannelIndex, [value] (AudioChannel& c) { c.setInputGainDb (value); });
    };
    inputPanel.onInputEnabledChanged = [this]
    {
        auto value = channels[(size_t) detailChannelIndex].isInputEnabled();
        propagateLinkedChange (detailChannelIndex, [value] (AudioChannel& c) { c.setInputEnabled (value); });
    };

    outputPanel.onChannelChanged = [this]
    {
        if (detailViewOpen)
            channelStrips[(size_t) detailChannelIndex]->refreshFromChannel();
    };
    outputPanel.onOutputGainChanged = [this]
    {
        auto value = channels[(size_t) detailChannelIndex].getOutputGainDb();
        propagateLinkedChange (detailChannelIndex, [value] (AudioChannel& c) { c.setOutputGainDb (value); });
    };
    outputPanel.onMuteChanged = [this]
    {
        auto value = channels[(size_t) detailChannelIndex].isOutputMuted();
        propagateLinkedChange (detailChannelIndex, [value] (AudioChannel& c) { c.setOutputMuted (value); });
    };

    addChildComponent (showView);
    addChildComponent (patchView);
    addChildComponent (sendView);

    showView.onSaveSessionRequested = [this]
    {
        promptForNameAndSave ("Enregistrer la session",
                              juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                              [this] (juce::File f) { writeSessionXmlTo (f); },
                              [this] { refreshShowView(); });
    };
    showView.onOverwriteSessionRequested = [this] (juce::File file)
    {
        confirmAndOverwriteFile (file, "Ecraser cette session avec l'etat actuel ?",
                                 [this] (juce::File f) { writeSessionXmlTo (f); });
    };
    showView.onLoadSessionRequested = [this] (juce::File file)
    {
        loadSessionXmlFrom (file);
    };
    showView.onDeleteSessionRequested = [this] (juce::File file)
    {
        confirmAndDeleteFile (file, "Supprimer cette session ?");
    };

    showView.onSaveSnapshotRequested = [this]
    {
        promptForNameAndSave ("Enregistrer un snapshot",
                              getSnapshotsFolder(),
                              [this] (juce::File f) { writeSessionXmlTo (f); },
                              [this] { refreshShowView(); });
    };
    showView.onOverwriteSnapshotRequested = [this] (juce::File file)
    {
        confirmAndOverwriteFile (file, "Ecraser ce snapshot avec l'etat actuel ?",
                                 [this] (juce::File f) { writeSessionXmlTo (f); });
    };
    showView.onRecallSnapshotRequested = [this] (juce::File file)
    {
        recallSnapshotFast (file);
    };
    showView.onDeleteSnapshotRequested = [this] (juce::File file)
    {
        confirmAndDeleteFile (file, "Supprimer ce snapshot ?");
    };

    showView.onSavePatchSnapshotRequested = [this]
    {
        promptForNameAndSave ("Enregistrer un Patch Snapshot",
                              getPatchSnapshotsFolder(),
                              [this] (juce::File f) { writePatchSnapshotXmlTo (f); },
                              [this] { refreshShowView(); });
    };
    showView.onOverwritePatchSnapshotRequested = [this] (juce::File file)
    {
        confirmAndOverwriteFile (file, "Ecraser ce Patch Snapshot avec la config actuelle ?",
                                 [this] (juce::File f) { writePatchSnapshotXmlTo (f); });
    };
    showView.onRecallPatchSnapshotRequested = [this] (juce::File file)
    {
        recallPatchSnapshot (file);
    };
    showView.onDeletePatchSnapshotRequested = [this] (juce::File file)
    {
        confirmAndDeleteFile (file, "Supprimer ce Patch Snapshot ?");
    };

    for (int i = 0; i < PatchView::numLinkGroups; ++i)
        linkGroups[(size_t) i].name = "Link " + juce::String (i + 1);

    patchView.onGroupToggled = [this] (int groupIndex, bool isOn)
    {
        linkGroups[(size_t) groupIndex].isOn = isOn;
    };
    patchView.onGroupRenamed = [this] (int groupIndex, juce::String newName)
    {
        linkGroups[(size_t) groupIndex].name = newName;
    };
    patchView.onCellToggled = [this] (int groupIndex, int channel, bool nowMember)
    {
        linkGroups[(size_t) groupIndex].members[(size_t) channel] = nowMember;
    };
    patchView.onPatchSnapshotRequested = [this]
    {
        showPatchSnapshotPicker();
    };

    for (int i = 0; i < (int) sendBuses.size(); ++i)
        sendBuses[(size_t) i].name = "Bus " + juce::String (i + 1);

    sendView.onBusToggled = [this] (int busIndex, bool isOn)
    {
        sendBuses[(size_t) busIndex].isOn = isOn;
    };
    sendView.onBusRenamed = [this] (int busIndex, juce::String newName)
    {
        sendBuses[(size_t) busIndex].name = newName;
    };
    sendView.onBusPluginClicked = [this] (int busIndex)
    {
        if (busIndex < 0 || busIndex >= (int) sendBuses.size())
            return;

        if (sendBuses[(size_t) busIndex].slot.isEmpty())
            showBusSlotMenu (busIndex);
        else
            openBusSlotEditor (busIndex);
    };
    sendView.onBusPluginRightClicked = [this] (int busIndex)
    {
        showBusSlotMenu (busIndex);
    };
    sendView.onBusOutputChanged = [this] (int busIndex, int outIndex)
    {
        sendBuses[(size_t) busIndex].outputChannelIndex = outIndex;
    };
    sendView.onBusGainChanged = [this] (int busIndex, float gainDb)
    {
        sendBuses[(size_t) busIndex].outputGainDb = gainDb;
    };
    sendView.onCellToggled = [this] (int busIndex, int channel, bool nowMember)
    {
        sendBuses[(size_t) busIndex].members[(size_t) channel] = nowMember;

        if (nowMember)
            sendBuses[(size_t) busIndex].memberIsPre[(size_t) channel] = false;

        channelStrips[(size_t) channel]->setSendBusMembership (busIndex, nowMember,
                                                               sendBuses[(size_t) busIndex].memberIsPre[(size_t) channel]);
    };
    sendView.onCellPrePostToggled = [this] (int busIndex, int channel, bool nowIsPre)
    {
        sendBuses[(size_t) busIndex].memberIsPre[(size_t) channel] = nowIsPre;

        channelStrips[(size_t) channel]->setSendBusMembership (busIndex, true, nowIsPre);
    };

    refreshChannelLabels();

    showBank (0);
    updateSelectedChannelDisplay (0);

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

void MainComponent::setInterfaceLocked (bool shouldBeLocked)
{
    interfaceLocked = shouldBeLocked;

    leftSidebar.setEnabled (! interfaceLocked);
    channelsHolder.setEnabled (! interfaceLocked);
    channelDetailView.setEnabled (! interfaceLocked);
    inputPanel.setEnabled (! interfaceLocked);
    outputPanel.setEnabled (! interfaceLocked);

    repaint();
}

void MainComponent::changeBufferSize (int newBufferSize)
{
    auto* device = deviceManager.getCurrentAudioDevice();

    if (device == nullptr)
        return;

    auto setup = deviceManager.getAudioDeviceSetup();
    setup.bufferSize = newBufferSize;

    auto error = deviceManager.setAudioDeviceSetup (setup, true);

    if (error.isNotEmpty())
        DBG ("Erreur lors du changement de buffer : " << error);
    else
        DBG ("Buffer change a " << newBufferSize << " samples.");
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

void MainComponent::updatePanelPositions()
{
    if (! detailViewOpen)
        return;

    auto inputBoundsInView = channelDetailView.getInputPanelBounds();
    auto inputTopLeft = channelDetailView.getPosition() + inputBoundsInView.getPosition();
    inputPanel.setBounds (inputTopLeft.x, inputTopLeft.y,
                          inputBoundsInView.getWidth(), inputBoundsInView.getHeight());

    auto outputBoundsInView = channelDetailView.getOutputPanelBounds();
    auto outputTopLeft = channelDetailView.getPosition() + outputBoundsInView.getPosition();
    outputPanel.setBounds (outputTopLeft.x, outputTopLeft.y,
                           outputBoundsInView.getWidth(), outputBoundsInView.getHeight());

    growWindowToFitIfNeeded();
}

void MainComponent::growWindowToFitIfNeeded()
{
    if (! detailViewOpen)
        return;

    constexpr int leftSidebarWidth = 100;
    constexpr int hotPluginsWidth = 220;
    constexpr int topBarHeight = 128;

    auto requiredWidth = leftSidebarWidth + channelDetailView.getRequiredWidth() + hotPluginsWidth;
    auto requiredHeight = topBarHeight + channelDetailView.getRequiredHeight();

    auto newWidth = juce::jmax (getWidth(), requiredWidth);
    auto newHeight = juce::jmax (getHeight(), requiredHeight);

    if (newWidth != getWidth() || newHeight != getHeight())
        setSize (newWidth, newHeight);
}

void MainComponent::updateSelectedChannelDisplay (int channelIndex)
{
    if (channelIndex < 0 || channelIndex >= numChannels)
        return;

    selectedChannelIndex = channelIndex;

    for (int ch = 0; ch < numChannels; ++ch)
        channelStrips[(size_t) ch]->setSelected (ch == channelIndex);

    auto displayName = channels[(size_t) channelIndex].getCustomName();
    if (displayName.isEmpty())
        displayName = "Ch " + juce::String (channelIndex + 1);

    topBar.setSelectedChannelName (displayName);
}

void MainComponent::navigateSelectedChannel (int direction)
{
    auto newIndex = juce::jlimit (0, numChannels - 1, selectedChannelIndex + direction);

    if (detailViewOpen)
        openChannelDetail (newIndex);
    else
        updateSelectedChannelDisplay (newIndex);
}

void MainComponent::showChannelPicker()
{
    juce::PopupMenu menu;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto name = channels[(size_t) ch].getCustomName();
        if (name.isEmpty())
            name = "Ch " + juce::String (ch + 1);

        menu.addItem (ch + 1, name, true, ch == selectedChannelIndex);
    }

    menu.showMenuAsync (juce::PopupMenu::Options(), [this] (int result)
    {
        if (result <= 0)
            return;

        auto index = result - 1;

        if (detailViewOpen)
            openChannelDetail (index);
        else
            updateSelectedChannelDisplay (index);
    });
}

void MainComponent::openChannelDetail (int channelIndex)
{
    updateSelectedChannelDisplay (channelIndex);

    detailChannelIndex = channelIndex;
    detailViewOpen = true;

    channelDetailView.setChannel (channelIndex, channels[(size_t) channelIndex]);
    channelDetailView.setVisible (true);
    channelsHolder.setVisible (false);
    leftSidebar.setVisible (false);
    showView.setVisible (false);
    patchView.setVisible (false);
    sendView.setVisible (false);

    inputPanel.setChannel (&channels[(size_t) channelIndex]);
    inputPanel.setVisible (true);

    outputPanel.setChannel (&channels[(size_t) channelIndex]);
    outputPanel.setVisible (true);

    topBar.setActiveTab (1);

    resized();
}

void MainComponent::closeChannelDetail()
{
    detailViewOpen = false;
    channelDetailView.setVisible (false);
    channelsHolder.setVisible (true);
    leftSidebar.setVisible (true);
    showView.setVisible (false);
    patchView.setVisible (false);
    sendView.setVisible (false);

    inputPanel.setVisible (false);
    inputPanel.setChannel (nullptr);

    outputPanel.setVisible (false);
    outputPanel.setChannel (nullptr);

    topBar.setActiveTab (0);

    resized();
}

void MainComponent::showShowTab()
{
    detailViewOpen = false;
    channelDetailView.setVisible (false);
    channelsHolder.setVisible (false);
    leftSidebar.setVisible (false);

    inputPanel.setVisible (false);
    inputPanel.setChannel (nullptr);

    outputPanel.setVisible (false);
    outputPanel.setChannel (nullptr);

    patchView.setVisible (false);
    sendView.setVisible (false);
    showView.setVisible (true);
    refreshShowView();

    topBar.setActiveTab (2);

    resized();
}

void MainComponent::showPatchTab()
{
    detailViewOpen = false;
    channelDetailView.setVisible (false);
    channelsHolder.setVisible (false);
    leftSidebar.setVisible (false);

    inputPanel.setVisible (false);
    inputPanel.setChannel (nullptr);

    outputPanel.setVisible (false);
    outputPanel.setChannel (nullptr);

    showView.setVisible (false);
    sendView.setVisible (false);
    patchView.setVisible (true);
    refreshChannelLabels();

    topBar.setActiveTab (3);

    resized();
}

void MainComponent::showSendTab()
{
    detailViewOpen = false;
    channelDetailView.setVisible (false);
    channelsHolder.setVisible (false);
    leftSidebar.setVisible (false);

    inputPanel.setVisible (false);
    inputPanel.setChannel (nullptr);

    outputPanel.setVisible (false);
    outputPanel.setChannel (nullptr);

    showView.setVisible (false);
    patchView.setVisible (false);
    sendView.setVisible (true);

    topBar.setActiveTab (4);

    resized();
}

void MainComponent::refreshChannelLabels()
{
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto name = channels[(size_t) ch].getCustomName();

        if (name.isEmpty())
            name = "Ch " + juce::String (ch + 1);

        patchView.setChannelLabel (ch, name);
        sendView.setChannelLabel (ch, name);
    }
}

void MainComponent::propagateLinkedChange (int sourceChannel, std::function<void (AudioChannel&)> apply)
{
    for (auto& group : linkGroups)
    {
        if (! group.isOn || ! group.members[(size_t) sourceChannel])
            continue;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (ch == sourceChannel || ! group.members[(size_t) ch])
                continue;

            apply (channels[(size_t) ch]);
            channelStrips[(size_t) ch]->refreshFromChannel();

            if (detailViewOpen && detailChannelIndex == ch)
            {
                inputPanel.setChannel (&channels[(size_t) ch]);
                outputPanel.setChannel (&channels[(size_t) ch]);
            }
        }
    }
}

void MainComponent::showBusSlotMenu (int busIndex)
{
    if (busIndex < 0 || busIndex >= (int) sendBuses.size())
        return;

    juce::PopupMenu menu;

    auto& currentSlot = sendBuses[(size_t) busIndex].slot;
    auto slotHasPlugin = ! currentSlot.isEmpty();

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

    bool anyHotPlugin = false;
    for (auto& hp : hotPlugins)
        if (hp.hasPlugin)
            anyHotPlugin = true;

    if (anyHotPlugin)
    {
        menu.addSeparator();
        menu.addSectionHeader ("Hot Plugins");

        for (int i = 0; i < (int) hotPlugins.size(); ++i)
            if (hotPlugins[(size_t) i].hasPlugin)
                menu.addItem (2000 + i, hotPlugins[(size_t) i].displayName);
    }

    if (slotHasPlugin)
    {
        menu.addSeparator();
        menu.addItem (2, "Vider cet emplacement");
    }

    menu.showMenuAsync (juce::PopupMenu::Options(), [this, busIndex] (int result)
    {
        if (result == 0)
            return;

        if (busIndex < 0 || busIndex >= (int) sendBuses.size())
            return;

        auto& slot = sendBuses[(size_t) busIndex].slot;

        if (result == 1)
        {
            slot.loadInternalCompressor();
            refreshBusPluginButtonText (busIndex);
        }
        else if (result == 2)
        {
            slot.clear();
            refreshBusPluginButtonText (busIndex);
        }
        else if (result >= 2000)
        {
            loadHotPluginIntoBusSlot (busIndex, result - 2000);
        }
        else if (result >= 1000)
        {
            loadVst3IntoBusSlot (busIndex, result - 1000);
        }
    });
}

void MainComponent::loadVst3IntoBusSlot (int busIndex, int pluginListIndex)
{
    const auto& plugins = pluginManager.getScannedPlugins();

    if (pluginListIndex < 0 || pluginListIndex >= (int) plugins.size())
        return;

    if (busIndex < 0 || busIndex >= (int) sendBuses.size())
        return;

    const auto& desc = plugins[(size_t) pluginListIndex];
    auto& slot = sendBuses[(size_t) busIndex].slot;

    sendView.setBusPluginName (busIndex, "Chargement...");

    slot.loadVst3Async (desc, pluginManager.getFormatManager(),
                        [this, busIndex] (bool success, juce::String error)
    {
        if (! success)
        {
            DBG ("Echec du chargement VST3 (bus) : " << error);
            sendView.setBusPluginName (busIndex, "+");
            return;
        }

        refreshBusPluginButtonText (busIndex);
    });
}

void MainComponent::loadHotPluginIntoBusSlot (int busIndex, int hotIndex)
{
    if (hotIndex < 0 || hotIndex >= (int) hotPlugins.size())
        return;

    if (busIndex < 0 || busIndex >= (int) sendBuses.size())
        return;

    auto& hotPlugin = hotPlugins[(size_t) hotIndex];

    if (! hotPlugin.hasPlugin)
        return;

    auto& slot = sendBuses[(size_t) busIndex].slot;

    sendView.setBusPluginName (busIndex, "Chargement...");

    auto savedState = hotPlugin.state;

    slot.loadVst3Async (hotPlugin.description, pluginManager.getFormatManager(),
                        [this, busIndex, savedState] (bool success, juce::String error)
    {
        if (! success)
        {
            DBG ("Echec du chargement du Hot Plugin (bus) : " << error);
            sendView.setBusPluginName (busIndex, "+");
            return;
        }

        auto& slotRef = sendBuses[(size_t) busIndex].slot;
        slotRef.applyStateAndRecapture (savedState);

        refreshBusPluginButtonText (busIndex);
    });
}

void MainComponent::refreshBusPluginButtonText (int busIndex)
{
    if (busIndex < 0 || busIndex >= (int) sendBuses.size())
        return;

    sendView.setBusPluginName (busIndex, sendBuses[(size_t) busIndex].slot.getDisplayName());
}

void MainComponent::openFloatingEditor (const juce::String& title, juce::Component* editorToOwn)
{
    // Fenetre non modale (voir PluginEditorWindow.h) : contrairement au
    // DialogWindow::LaunchOptions::launchAsync() utilise auparavant, elle
    // ne bloque pas le reste de l'application. On la garde en vie dans
    // openPluginEditorWindows tant qu'elle n'est pas fermee par
    // l'utilisateur (closeButtonPressed -> onClosed la retire de la liste).
    auto window = std::make_unique<PluginEditorWindow> (title, editorToOwn);
    auto* rawPtr = window.get();

    window->onClosed = [this, rawPtr]
    {
        openPluginEditorWindows.erase (
            std::remove_if (openPluginEditorWindows.begin(), openPluginEditorWindows.end(),
                            [rawPtr] (const std::unique_ptr<PluginEditorWindow>& p) { return p.get() == rawPtr; }),
            openPluginEditorWindows.end());
    };

    openPluginEditorWindows.push_back (std::move (window));
}

void MainComponent::openBusSlotEditor (int busIndex)
{
    if (busIndex < 0 || busIndex >= (int) sendBuses.size())
        return;

    auto& slot = sendBuses[(size_t) busIndex].slot;

    if (slot.isEmpty())
        return;

    auto title = sendBuses[(size_t) busIndex].name + " - " + slot.getDisplayName();

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

        openFloatingEditor (title, editor);
        return;
    }

    auto editor = std::make_unique<CompressorEditor>(slot.getCompressor());
    openFloatingEditor (title, editor.release());
}

void MainComponent::loadLinkGroupsFromXml (const juce::XmlElement& rootXml)
{
    auto* linkGroupsXml = rootXml.getChildByName ("LinkGroups");

    if (linkGroupsXml == nullptr)
        return;

    for (auto* groupXml : linkGroupsXml->getChildWithTagNameIterator ("Group"))
    {
        auto index = groupXml->getIntAttribute ("index", -1);

        if (index < 0 || index >= PatchView::numLinkGroups)
            continue;

        auto& group = linkGroups[(size_t) index];
        group.name = groupXml->getStringAttribute ("name", group.name);
        group.isOn = groupXml->getBoolAttribute ("isOn", group.isOn);

        auto membersStr = groupXml->getStringAttribute ("members", "");

        for (int ch = 0; ch < numChannels && ch < membersStr.length(); ++ch)
            group.members[(size_t) ch] = (membersStr[ch] == '1');

        patchView.setGroupName (index, group.name);
        patchView.setGroupOn (index, group.isOn);

        for (int ch = 0; ch < numChannels; ++ch)
            patchView.setMember (index, ch, group.members[(size_t) ch]);
    }
}

juce::File MainComponent::getPatchSnapshotsFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("LiveRack Patch Snapshots");
}

void MainComponent::writePatchSnapshotXmlTo (const juce::File& file)
{
    juce::XmlElement root ("LiveRackPatchSnapshot");

    auto* linkGroupsXml = root.createNewChildElement ("LinkGroups");

    for (int i = 0; i < PatchView::numLinkGroups; ++i)
    {
        auto* groupXml = linkGroupsXml->createNewChildElement ("Group");
        groupXml->setAttribute ("index", i);
        groupXml->setAttribute ("name", linkGroups[(size_t) i].name);
        groupXml->setAttribute ("isOn", linkGroups[(size_t) i].isOn);

        juce::String membersStr;
        for (int ch = 0; ch < numChannels; ++ch)
            membersStr += linkGroups[(size_t) i].members[(size_t) ch] ? "1" : "0";

        groupXml->setAttribute ("members", membersStr);
    }

    if (root.writeTo (file))
        DBG ("Patch Snapshot ecrit : " << file.getFullPathName());
    else
        DBG ("Echec de l'ecriture du Patch Snapshot : " << file.getFullPathName());
}

void MainComponent::recallPatchSnapshot (const juce::File& file)
{
    auto xml = juce::parseXML (file);

    if (xml == nullptr || ! xml->hasTagName ("LiveRackPatchSnapshot"))
    {
        DBG ("Fichier Patch Snapshot invalide : " << file.getFullPathName());
        return;
    }

    loadLinkGroupsFromXml (*xml);

    DBG ("Patch Snapshot rappele depuis : " << file.getFullPathName());
}

void MainComponent::showPatchSnapshotPicker()
{
    auto folder = getPatchSnapshotsFolder();
    auto files = folder.findChildFiles (juce::File::findFiles, false, "*.liverack");

    if (files.isEmpty())
    {
        juce::NativeMessageBox::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
            "Patch Snapshots",
            "Aucun Patch Snapshot enregistre pour le moment.",
            nullptr);
        return;
    }

    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
    {
        return a.getFileNameWithoutExtension().compareIgnoreCase (b.getFileNameWithoutExtension()) < 0;
    });

    juce::PopupMenu menu;

    for (int i = 0; i < files.size(); ++i)
        menu.addItem (i + 1, files[i].getFileNameWithoutExtension());

    menu.showMenuAsync (juce::PopupMenu::Options(), [this, files] (int result)
    {
        if (result <= 0)
            return;

        auto index = result - 1;

        if (index >= 0 && index < files.size())
            recallPatchSnapshot (files[index]);
    });
}

void MainComponent::writeSessionXmlTo (const juce::File& file)
{
    juce::XmlElement root ("LiveRackSession");

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* channelXml = root.createNewChildElement ("Channel");
        channelXml->setAttribute ("index", ch);
        channels[(size_t) ch].saveToXml (*channelXml);
    }

    auto* linkGroupsXml = root.createNewChildElement ("LinkGroups");

    for (int i = 0; i < PatchView::numLinkGroups; ++i)
    {
        auto* groupXml = linkGroupsXml->createNewChildElement ("Group");
        groupXml->setAttribute ("index", i);
        groupXml->setAttribute ("name", linkGroups[(size_t) i].name);
        groupXml->setAttribute ("isOn", linkGroups[(size_t) i].isOn);

        juce::String membersStr;
        for (int ch = 0; ch < numChannels; ++ch)
            membersStr += linkGroups[(size_t) i].members[(size_t) ch] ? "1" : "0";

        groupXml->setAttribute ("members", membersStr);
    }

    if (root.writeTo (file))
        DBG ("Sauvegarde ecrite : " << file.getFullPathName());
    else
        DBG ("Echec de l'ecriture : " << file.getFullPathName());
}

void MainComponent::loadSessionXmlFrom (const juce::File& file)
{
    auto xml = juce::parseXML (file);

    if (xml == nullptr || ! xml->hasTagName ("LiveRackSession"))
    {
        DBG ("Fichier invalide : " << file.getFullPathName());
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

            juce::Timer::callAfterDelay (900, [this, index]
            {
                if (detailViewOpen && detailChannelIndex == index)
                    channelDetailView.refreshSlots();
            });
        });
    }

    loadLinkGroupsFromXml (*xml);

    DBG ("Chargement en cours depuis : " << file.getFullPathName());
}

void MainComponent::recallSnapshotFast (const juce::File& file)
{
    auto xml = juce::parseXML (file);

    if (xml == nullptr || ! xml->hasTagName ("LiveRackSession"))
    {
        DBG ("Fichier invalide : " << file.getFullPathName());
        return;
    }

    for (auto* channelXml : xml->getChildWithTagNameIterator ("Channel"))
    {
        auto index = channelXml->getIntAttribute ("index", -1);

        if (index < 0 || index >= numChannels)
            continue;

        channels[(size_t) index].loadFromXmlFast (*channelXml, pluginManager.getFormatManager(),
            [this, index]
        {
            channelStrips[(size_t) index]->refreshFromChannel();

            if (detailViewOpen && detailChannelIndex == index)
                channelDetailView.refreshSlots();
        });
    }

    loadLinkGroupsFromXml (*xml);

    DBG ("Rappel a chaud depuis : " << file.getFullPathName());
}

void MainComponent::promptForNameAndSave (const juce::String& dialogTitle, const juce::File& targetFolder,
                                           std::function<void (juce::File)> saveFunction, std::function<void()> afterSave)
{
    targetFolder.createDirectory();

    auto* nameWindow = new juce::AlertWindow (dialogTitle, "Nom :", juce::AlertWindow::NoIcon);
    nameWindow->addTextEditor ("name", "", "Nom");
    nameWindow->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    nameWindow->addButton ("Annuler", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    nameWindow->enterModalState (true, juce::ModalCallbackFunction::create (
        [this, nameWindow, targetFolder, saveFunction, afterSave] (int result)
    {
        std::unique_ptr<juce::AlertWindow> windowDeleter (nameWindow);

        if (result == 0)
            return;

        auto chosenName = nameWindow->getTextEditorContents ("name").trim();

        if (chosenName.isEmpty())
            chosenName = "Sans titre " + juce::Time::getCurrentTime().toString (true, true, false, true);

        auto file = targetFolder.getChildFile (chosenName + ".liverack");

        if (saveFunction != nullptr)
            saveFunction (file);

        if (afterSave != nullptr)
            afterSave();
    }), true);
}

void MainComponent::confirmAndOverwriteFile (const juce::File& file, const juce::String& message, std::function<void (juce::File)> saveFunction)
{
    auto fullMessage = message + "\n\"" + file.getFileNameWithoutExtension() + "\"";

    juce::NativeMessageBox::showOkCancelBox (juce::AlertWindow::QuestionIcon,
        "Confirmation",
        fullMessage,
        nullptr,
        juce::ModalCallbackFunction::create ([this, file, saveFunction] (int result)
        {
            if (result != 1)
                return;

            if (saveFunction != nullptr)
                saveFunction (file);

            refreshShowView();
        }));
}

void MainComponent::confirmAndDeleteFile (const juce::File& file, const juce::String& message)
{
    auto fullMessage = message + "\n\"" + file.getFileNameWithoutExtension() + "\"\n\nCette action est definitive.";

    juce::NativeMessageBox::showOkCancelBox (juce::AlertWindow::WarningIcon,
        "Confirmation",
        fullMessage,
        nullptr,
        juce::ModalCallbackFunction::create ([this, file] (int result)
        {
            if (result != 1)
                return;

            file.deleteFile();
            refreshShowView();
        }));
}

juce::File MainComponent::getSnapshotsFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("LiveRack Snapshots");
}

void MainComponent::refreshShowView()
{
    std::vector<std::pair<juce::String, juce::File>> sessions;

    auto docsFolder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    for (auto& file : docsFolder.findChildFiles (juce::File::findFiles, false, "*.liverack"))
        sessions.push_back ({ file.getFileNameWithoutExtension(), file });

    std::sort (sessions.begin(), sessions.end(), [] (auto& a, auto& b)
    {
        return a.first.compareIgnoreCase (b.first) < 0;
    });

    std::vector<std::pair<juce::String, juce::File>> snapshots;

    auto snapshotsFolder = getSnapshotsFolder();
    for (auto& file : snapshotsFolder.findChildFiles (juce::File::findFiles, false, "*.liverack"))
        snapshots.push_back ({ file.getFileNameWithoutExtension(), file });

    std::sort (snapshots.begin(), snapshots.end(), [] (auto& a, auto& b)
    {
        return a.first.compareIgnoreCase (b.first) < 0;
    });

    std::vector<std::pair<juce::String, juce::File>> patchSnapshots;

    auto patchSnapshotsFolder = getPatchSnapshotsFolder();
    for (auto& file : patchSnapshotsFolder.findChildFiles (juce::File::findFiles, false, "*.liverack"))
        patchSnapshots.push_back ({ file.getFileNameWithoutExtension(), file });

    std::sort (patchSnapshots.begin(), patchSnapshots.end(), [] (auto& a, auto& b)
    {
        return a.first.compareIgnoreCase (b.first) < 0;
    });

    showView.setSessions (sessions);
    showView.setSnapshots (snapshots);
    showView.setPatchSnapshots (patchSnapshots);
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

        writeSessionXmlTo (file);
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

        loadSessionXmlFrom (file);
    });
}

void MainComponent::showBank (int bankIndex)
{
    currentBank = bankIndex;
    leftSidebar.setSelectedBank (bankIndex);

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

void MainComponent::showCustomLayer (const std::array<bool, numChannels>& selection)
{
    channelsHolder.strips.clear();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto isVisible = selection[(size_t) ch];
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

    auto& currentSlot = channels[(size_t) channelIndex].getSlot (slotIndex);
    auto slotHasPlugin = ! currentSlot.isEmpty();

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

    bool anyHotPlugin = false;
    for (auto& hp : hotPlugins)
        if (hp.hasPlugin)
            anyHotPlugin = true;

    if (anyHotPlugin)
    {
        menu.addSeparator();
        menu.addSectionHeader ("Hot Plugins");

        for (int i = 0; i < (int) hotPlugins.size(); ++i)
            if (hotPlugins[(size_t) i].hasPlugin)
                menu.addItem (2000 + i, hotPlugins[(size_t) i].displayName);
    }

    if (slotHasPlugin)
    {
        menu.addSeparator();
        menu.addItem (3, "Ajouter aux Hot Plugins");
        menu.addItem (2, "Vider cet emplacement");
    }

    menu.showMenuAsync (juce::PopupMenu::Options(), [this, channelIndex, slotIndex] (int result)
    {
        if (result == 0)
            return;

        auto& slot = channels[(size_t) channelIndex].getSlot (slotIndex);

        if (result == 3)
        {
            addCurrentSlotToHotPlugins (channelIndex, slotIndex);
            return;
        }

        if (detailViewOpen && detailChannelIndex == channelIndex)
            channelDetailView.prepareForSlotReplace (slotIndex);

        if (result == 1)
        {
            slot.loadInternalCompressor();
            refreshSlotButtonText (channelIndex, slotIndex);

            if (detailViewOpen && detailChannelIndex == channelIndex)
                channelDetailView.refreshSlots (slotIndex);
        }
        else if (result == 2)
        {
            slot.clear();
            refreshSlotButtonText (channelIndex, slotIndex);

            if (detailViewOpen && detailChannelIndex == channelIndex)
                channelDetailView.refreshSlots (slotIndex);
        }
        else if (result >= 2000)
        {
            loadHotPluginIntoSlot (channelIndex, slotIndex, result - 2000);
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

        juce::Timer::callAfterDelay (900, [this, channelIndex, slotIndex]
        {
            if (detailViewOpen && detailChannelIndex == channelIndex)
                channelDetailView.refreshSlots (slotIndex);
        });
    });
}

void MainComponent::addCurrentSlotToHotPlugins (int channelIndex, int slotIndex)
{
    auto& slot = channels[(size_t) channelIndex].getSlot (slotIndex);

    if (slot.isEmpty() || ! slot.isVst3())
        return;

    auto* instance = slot.getVstInstance();
    if (instance == nullptr)
        return;

    auto* nameWindow = new juce::AlertWindow ("Ajouter aux Hot Plugins",
                                              "Nom pour ce Hot Plugin :",
                                              juce::AlertWindow::NoIcon);
    nameWindow->addTextEditor ("name", slot.getDisplayName(), "Nom");
    nameWindow->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    nameWindow->addButton ("Annuler", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    nameWindow->enterModalState (true, juce::ModalCallbackFunction::create (
        [this, channelIndex, slotIndex, nameWindow] (int result)
    {
        std::unique_ptr<juce::AlertWindow> windowDeleter (nameWindow);

        if (result == 0)
            return;

        auto chosenName = nameWindow->getTextEditorContents ("name").trim();

        auto& slotRef = channels[(size_t) channelIndex].getSlot (slotIndex);

        if (slotRef.isEmpty() || ! slotRef.isVst3())
            return;

        auto* inst = slotRef.getVstInstance();
        if (inst == nullptr)
            return;

        if (chosenName.isEmpty())
            chosenName = slotRef.getDisplayName();

        juce::MemoryBlock state;
        inst->getStateInformation (state);

        int targetIndex = 0;
        for (int i = 0; i < (int) hotPlugins.size(); ++i)
        {
            if (! hotPlugins[(size_t) i].hasPlugin)
            {
                targetIndex = i;
                break;
            }
        }

        hotPlugins[(size_t) targetIndex].setFromPlugin (chosenName, slotRef.getPluginDescription(),
                                                         state, slotRef.getThumbnail());

        hotPluginsPanel.refreshSlot (targetIndex, hotPlugins[(size_t) targetIndex]);
        saveHotPlugins (hotPlugins);
    }), true);
}

void MainComponent::loadHotPluginIntoSlot (int channelIndex, int slotIndex, int hotIndex)
{
    if (hotIndex < 0 || hotIndex >= (int) hotPlugins.size())
        return;

    auto& hotPlugin = hotPlugins[(size_t) hotIndex];

    if (! hotPlugin.hasPlugin)
        return;

    auto& slot = channels[(size_t) channelIndex].getSlot (slotIndex);

    channelStrips[(size_t) channelIndex]->refreshSlotButtonText (slotIndex, "Chargement...");

    auto savedState = hotPlugin.state;

    slot.loadVst3Async (hotPlugin.description, pluginManager.getFormatManager(),
                        [this, channelIndex, slotIndex, savedState] (bool success, juce::String error)
    {
        if (! success)
        {
            DBG ("Echec du chargement du Hot Plugin : " << error);
            channelStrips[(size_t) channelIndex]->refreshSlotButtonText (slotIndex, "+");
            return;
        }

        auto& slotRef = channels[(size_t) channelIndex].getSlot (slotIndex);
        slotRef.applyStateAndRecapture (savedState);

        refreshSlotButtonText (channelIndex, slotIndex);

        juce::Timer::callAfterDelay (900, [this, channelIndex, slotIndex]
        {
            if (detailViewOpen && detailChannelIndex == channelIndex)
                channelDetailView.refreshSlots (slotIndex);
        });
    });
}

void MainComponent::showHotPluginPicker (int hotIndex)
{
    if (hotIndex < 0 || hotIndex >= (int) hotPlugins.size())
        return;

    juce::PopupMenu menu;
    bool hasAnyOther = false;

    for (int i = 0; i < (int) hotPlugins.size(); ++i)
    {
        if (i == hotIndex)
            continue;

        auto& other = hotPlugins[(size_t) i];

        if (! other.hasPlugin)
            continue;

        hasAnyOther = true;
        menu.addItem (i + 1, other.displayName);
    }

    if (! hasAnyOther)
        return;

    auto targetBounds = hotPluginsPanel.getSlotScreenBounds (hotIndex);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (targetBounds),
                        [this, hotIndex] (int result)
    {
        if (result <= 0)
            return;

        auto sourceIndex = result - 1;

        if (sourceIndex < 0 || sourceIndex >= (int) hotPlugins.size())
            return;

        auto& source = hotPlugins[(size_t) sourceIndex];

        if (! source.hasPlugin)
            return;

        auto& dest = hotPlugins[(size_t) hotIndex];
        dest.setFromPlugin (source.displayName, source.description, source.state, source.thumbnail);

        hotPluginsPanel.refreshSlot (hotIndex, dest);
        saveHotPlugins (hotPlugins);
    });
}

void MainComponent::renameHotPlugin (int hotIndex)
{
    if (hotIndex < 0 || hotIndex >= (int) hotPlugins.size())
        return;

    auto& hotPlugin = hotPlugins[(size_t) hotIndex];

    if (! hotPlugin.hasPlugin)
        return;

    auto* nameWindow = new juce::AlertWindow ("Renommer le Hot Plugin",
                                              "Nouveau nom :",
                                              juce::AlertWindow::NoIcon);
    nameWindow->addTextEditor ("name", hotPlugin.displayName, "Nom");
    nameWindow->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    nameWindow->addButton ("Annuler", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    nameWindow->enterModalState (true, juce::ModalCallbackFunction::create (
        [this, hotIndex, nameWindow] (int result)
    {
        std::unique_ptr<juce::AlertWindow> windowDeleter (nameWindow);

        if (result == 0)
            return;

        auto chosenName = nameWindow->getTextEditorContents ("name").trim();

        if (hotIndex < 0 || hotIndex >= (int) hotPlugins.size())
            return;

        auto& hp = hotPlugins[(size_t) hotIndex];

        if (! hp.hasPlugin)
            return;

        if (chosenName.isNotEmpty())
            hp.displayName = chosenName;

        hotPluginsPanel.refreshSlot (hotIndex, hp);
        saveHotPlugins (hotPlugins);
    }), true);
}

void MainComponent::clearHotPlugin (int hotIndex)
{
    if (hotIndex < 0 || hotIndex >= (int) hotPlugins.size())
        return;

    hotPlugins[(size_t) hotIndex].clear();
    hotPluginsPanel.refreshSlot (hotIndex, hotPlugins[(size_t) hotIndex]);
    saveHotPlugins (hotPlugins);
}

void MainComponent::openSlotEditor (int channelIndex, int slotIndex)
{
    auto& slot = channels[(size_t) channelIndex].getSlot (slotIndex);

    if (slot.isEmpty())
        return;

    auto title = "Ch " + juce::String (channelIndex + 1) + " - " + slot.getDisplayName();

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

        openFloatingEditor (title, editor);
        return;
    }

    auto editor = std::make_unique<CompressorEditor>(slot.getCompressor());
    openFloatingEditor (title, editor.release());
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

    for (auto& bus : sendBuses)
    {
        bus.busBuffer.setSize (1, juce::jmax (samplesPerBlockExpected, 32), false, false, true);
        bus.slot.prepare (sampleRate, samplesPerBlockExpected);
    }
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

    // Bus SEND (Phase 1) : chaque bus actif somme le signal (pre ou post
    // effets, selon la case) de ses chaines membres, le fait passer dans son
    // propre plugin partage, puis ADDITIONNE (jamais ne remplace) le
    // resultat sur son canal de sortie -- pour pouvoir se melanger au son
    // sec des chaines sur la meme sortie physique, sans l'effacer.
    for (auto& bus : sendBuses)
    {
        if (! bus.isOn)
            continue;

        if (bus.busBuffer.getNumSamples() < numSamples)
            bus.busBuffer.setSize (1, numSamples, false, false, true);

        bus.busBuffer.clear (0, 0, numSamples);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (! bus.members[(size_t) ch])
                continue;

            if (bus.memberIsPre[(size_t) ch])
                channels[(size_t) ch].addPreFxSendTo (bus.busBuffer, 0, numSamples);
            else
                channels[(size_t) ch].addPostFxSendTo (bus.busBuffer, 0, numSamples);
        }

        auto* busSamples = bus.busBuffer.getWritePointer (0);
        bus.slot.process (busSamples, numSamples);

        auto busGainLinear = juce::Decibels::decibelsToGain (bus.outputGainDb, -60.0f);
        for (int i = 0; i < numSamples; ++i)
            busSamples[i] *= busGainLinear;

        if (bus.outputChannelIndex >= 0 && bus.outputChannelIndex < buffer.getNumChannels())
            buffer.addFrom (bus.outputChannelIndex, startSample, bus.busBuffer, 0, 0, numSamples);
    }
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
    {
        inputPanel.updateMeter();
        outputPanel.updateMeter();
        channelDetailView.updateSlotMeters();
    }

    topBar.setCpuUsage (deviceManager.getCpuUsage());
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    if (interfaceLocked)
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        auto overlayArea = getLocalBounds().withTrimmedTop (128);
        g.fillRect (overlayArea);
    }
}

void MainComponent::resized()
{
    auto fullArea = getLocalBounds();

    auto topBarArea = fullArea.removeFromTop (120);
    topBar.setBounds (topBarArea);

    fullArea.removeFromTop (8);

    leftSidebar.setBounds (fullArea.removeFromLeft (100));

    hotPluginsPanel.setBounds (fullArea.removeFromRight (220));

    channelsHolder.setBounds (fullArea);
    channelsHolder.resized();

    channelDetailView.setBounds (fullArea);
    showView.setBounds (fullArea);
    patchView.setBounds (fullArea);
    sendView.setBounds (fullArea);

    updatePanelPositions();
}
