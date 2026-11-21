#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"
#include "RackSlotComponent.h"
#include "CompressorEditor.h"

// Vue detaillee d'un seul canal : entree/sortie en haut, pile de slots
// composites a gauche (nom+selecteur, IN, mini-vumetre, miniature),
// editeur du plugin selectionne a droite.
class ChannelDetailView : public juce::Component
{
public:
    ChannelDetailView()
    {
        prevButton.setButtonText ("<");
        prevButton.onClick = [this] { if (onPrevious != nullptr) onPrevious(); };
        addAndMakeVisible (prevButton);

        nextButton.setButtonText (">");
        nextButton.onClick = [this] { if (onNext != nullptr) onNext(); };
        addAndMakeVisible (nextButton);

        channelNameLabel.setJustificationType (juce::Justification::centred);
        channelNameLabel.setFont (juce::Font (20.0f, juce::Font::bold));
        channelNameLabel.setEditable (false, true, false);
        channelNameLabel.onTextChange = [this]
        {
            if (channel != nullptr)
                channel->setCustomName (channelNameLabel.getText());
        };
        addAndMakeVisible (channelNameLabel);

        for (int slot = 0; slot < numSlotsPerChannel; ++slot)
        {
            auto slotComp = std::make_unique<RackSlotComponent>();
            auto slotIndexCopy = slot;

            slotComp->onMenuRequested = [this, slotIndexCopy]
            {
                if (onSlotRightClicked != nullptr)
                    onSlotRightClicked (channelIndex, slotIndexCopy);
            };
            slotComp->onThumbnailClicked = [this, slotIndexCopy]
            {
                selectSlotForDisplay (slotIndexCopy);
            };
            slotComp->onInToggled = [this, slotIndexCopy] (bool isIn)
            {
                if (channel != nullptr)
                    channel->getSlot (slotIndexCopy).setBypassed (! isIn);
            };

            addAndMakeVisible (*slotComp);
            slotComponents.push_back (std::move (slotComp));
        }

        pluginViewport.setViewedComponent (&pluginDisplayArea, false);
        pluginViewport.setScrollBarsShown (true, true);
        addAndMakeVisible (pluginViewport);
    }

    void setChannel (int newChannelIndex, AudioChannel& newChannel)
    {
        channelIndex = newChannelIndex;
        channel = &newChannel;

        auto displayName = channel->getCustomName();
        if (displayName.isEmpty())
            displayName = "Ch " + juce::String (channelIndex + 1);
        channelNameLabel.setText (displayName, juce::dontSendNotification);

        selectedSlot = -1;
        clearPluginDisplay();
        refreshSlots();
    }

    juce::Rectangle<int> getInputPanelBounds() const  { return inputPanelBounds; }
    juce::Rectangle<int> getOutputPanelBounds() const { return outputPanelBounds; }

    void refreshSlots()
    {
        if (channel == nullptr)
            return;

        for (int slot = 0; slot < numSlotsPerChannel; ++slot)
        {
            auto& pluginSlot = channel->getSlot (slot);
            auto name = pluginSlot.getDisplayName();
            auto isLoaded = ! pluginSlot.isEmpty();

            slotComponents[(size_t) slot]->setPluginName (isLoaded ? name : "");
            slotComponents[(size_t) slot]->setInState (! pluginSlot.isBypassed());

            if (isLoaded && pluginSlot.getThumbnail().isValid())
                slotComponents[(size_t) slot]->setThumbnail (pluginSlot.getThumbnail());
            else
                slotComponents[(size_t) slot]->clearThumbnail();
        }

        if (selectedSlot >= 0)
            selectSlotForDisplay (selectedSlot);
    }

    void refreshSlotThumbnail (int slotIndex, const juce::Image& image)
    {
        if (slotIndex >= 0 && slotIndex < (int) slotComponents.size())
        {
            if (image.isValid())
                slotComponents[(size_t) slotIndex]->setThumbnail (image);
            else
                slotComponents[(size_t) slotIndex]->clearThumbnail();
        }
    }

    void updateSlotMeters()
    {
        // Pas de vumetre par plugin individuel pour l'instant : on reutilise
        // le niveau global du canal comme approximation visuelle simple.
        if (channel == nullptr)
            return;

        auto level = channel->getLevel();

        for (auto& slotComp : slotComponents)
            slotComp->setMeterLevel (level);
    }

    void selectSlotForDisplay (int slotIndex)
    {
        selectedSlot = slotIndex;
        clearPluginDisplay();

        if (channel == nullptr)
            return;

        auto& slot = channel->getSlot (slotIndex);

        if (slot.isEmpty())
        {
            layoutEmptyDisplay();
            repaint();
            return;
        }

        juce::Component* editorComponent = nullptr;

        if (slot.isVst3())
        {
            auto* instance = slot.getVstInstance();

            if (instance == nullptr)
                return;

            currentEditor.reset (instance->createEditorIfNeeded());
            editorComponent = currentEditor.get();
        }
        else
        {
            currentInternalEditor = std::make_unique<CompressorEditor>(slot.getCompressor());
            editorComponent = currentInternalEditor.get();
        }

        if (editorComponent == nullptr)
            return;

        pluginDisplayArea.addAndMakeVisible (*editorComponent);
        layoutPluginEditor (*editorComponent);

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff1a1d21));

        g.setColour (juce::Colour (0xff22262b));
        g.fillRoundedRectangle (pluginViewport.getBounds().toFloat(), 6.0f);

        if (selectedSlot < 0 || channel == nullptr || channel->getSlot (selectedSlot).isEmpty())
        {
            g.setColour (juce::Colour (0xff5a6472));
            g.setFont (14.0f);
            g.drawText ("Selectionnez un emplacement pour voir ses reglages",
                        pluginViewport.getBounds(), juce::Justification::centred);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (20);

        auto headerArea = area.removeFromTop (40);
        prevButton.setBounds (headerArea.removeFromLeft (40));
        nextButton.setBounds (headerArea.removeFromRight (40));
        channelNameLabel.setBounds (headerArea);

        area.removeFromTop (16);

        inputPanelBounds = area.removeFromLeft (170);
        area.removeFromLeft (20);

        outputPanelBounds = area.removeFromRight (170);
        area.removeFromRight (20);

        // Colonne de slots composites : large et assez haute pour que la
        // miniature du plugin soit bien lisible.
        constexpr int slotHeight = 150;
        constexpr int slotGap = 10;

        auto slotsColumn = area.removeFromLeft (280);

        for (auto& slotComp : slotComponents)
        {
            slotComp->setBounds (slotsColumn.removeFromTop (slotHeight));
            slotsColumn.removeFromTop (slotGap);
        }

        area.removeFromLeft (20);
        pluginViewport.setBounds (area);

        if (currentEditor != nullptr)
            layoutPluginEditor (*currentEditor);
        else if (currentInternalEditor != nullptr)
            layoutPluginEditor (*currentInternalEditor);
        else
            layoutEmptyDisplay();
    }

    std::function<void()> onPrevious;
    std::function<void()> onNext;
    std::function<void (int channelIndex, int slotIndex)> onSlotRightClicked;
    std::function<void()> onPanelBoundsChanged;

private:
    void layoutEmptyDisplay()
    {
        pluginDisplayArea.setBounds (0, 0, pluginViewport.getWidth(), pluginViewport.getHeight());
    }

    void layoutPluginEditor (juce::Component& editor)
    {
        auto viewportWidth = pluginViewport.getWidth();
        auto viewportHeight = pluginViewport.getHeight();

        auto editorWidth = editor.getWidth();
        auto editorHeight = editor.getHeight();

        auto areaWidth = juce::jmax (editorWidth, viewportWidth);
        auto areaHeight = juce::jmax (editorHeight, viewportHeight);

        pluginDisplayArea.setBounds (0, 0, areaWidth, areaHeight);

        auto x = (areaWidth - editorWidth) / 2;
        auto y = (areaHeight - editorHeight) / 2;
        editor.setTopLeftPosition (x, y);
    }

    void clearPluginDisplay()
    {
        currentEditor.reset();
        currentInternalEditor.reset();
    }

    int channelIndex = 0;
    AudioChannel* channel = nullptr;
    int selectedSlot = -1;
    juce::Rectangle<int> inputPanelBounds, outputPanelBounds;

    juce::TextButton prevButton, nextButton;
    juce::Label channelNameLabel;
    std::vector<std::unique_ptr<RackSlotComponent>> slotComponents;

    juce::Viewport pluginViewport;
    juce::Component pluginDisplayArea;
    std::unique_ptr<juce::AudioProcessorEditor> currentEditor;
    std::unique_ptr<CompressorEditor> currentInternalEditor;
};
