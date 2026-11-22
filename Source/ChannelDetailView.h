#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"
#include "RackSlotComponent.h"
#include "CompressorEditor.h"

// Vue detaillee d'un seul canal : entree/sortie en haut, pile de slots
// composites a gauche (nom+selecteur, IN, mini-vumetre, miniature),
// editeur du plugin selectionne encastre a droite, a sa taille reelle
// (pas de mise a l'echelle). getRequiredWidth()/getRequiredHeight()
// permettent a MainComponent d'agrandir toute la fenetre du programme
// quand le plugin a besoin de plus de place.
class ChannelDetailView : public juce::Component,
                           private juce::ComponentListener
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

        addAndMakeVisible (pluginDisplayArea);
    }

    ~ChannelDetailView() override
    {
        clearPluginDisplay();
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

    int getRequiredWidth() const
    {
        // 20 (marge gauche) + 170 (IN) + 20 (gap) + 280 (slots) + 20 (gap)
        // + 170 (OUT) + 20 (gap) + 20 (marge droite) = 720, plus la largeur
        // reelle du plugin actuellement affiche (ou la largeur par defaut).
        constexpr int fixedWidth = 720;
        return fixedWidth + pluginDisplayArea.getWidth();
    }

    int getRequiredHeight() const
    {
        // 20+20 (marges haut/bas) + 40 (en-tete) + 16 (gap) = 96, plus le
        // plus grand entre la hauteur de la colonne de slots et celle du plugin.
        constexpr int verticalChrome = 96;
        return verticalChrome + juce::jmax (lastSlotsColumnHeight, pluginDisplayArea.getHeight());
    }

    void prepareForSlotReplace (int slotIndex)
    {
        if (selectedSlot == slotIndex)
            clearPluginDisplay();
    }

    void refreshSlots (int slotIndexThatChanged = -1)
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

        if (selectedSlot >= 0 && (slotIndexThatChanged < 0 || slotIndexThatChanged == selectedSlot))
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
            outputPanelBounds = defaultOutputPanelBounds;
            if (onPanelBoundsChanged != nullptr)
                onPanelBoundsChanged();
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
        editorComponent->setTopLeftPosition (0, 0);
        editorComponent->addComponentListener (this);

        layoutPluginEditor (*editorComponent);

        juce::Component::SafePointer<juce::Component> safeEditor (editorComponent);
        juce::Timer::callAfterDelay (300, [this, safeEditor]
        {
            if (safeEditor != nullptr)
                layoutPluginEditor (*safeEditor);
        });

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff1a1d21));

        g.setColour (juce::Colour (0xff22262b));
        g.fillRoundedRectangle (pluginDisplayArea.getBounds().toFloat(), 6.0f);

        if (selectedSlot < 0 || channel == nullptr || channel->getSlot (selectedSlot).isEmpty())
        {
            g.setColour (juce::Colour (0xff5a6472));
            g.setFont (14.0f);
            g.drawText ("Selectionnez un emplacement pour voir ses reglages",
                        pluginDisplayArea.getBounds(), juce::Justification::centred);
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

        defaultOutputPanelBounds = area.removeFromRight (170);
        area.removeFromRight (20);

        constexpr int slotHeight = 150;
        constexpr int slotGap = 10;

        auto slotsColumn = area.removeFromLeft (280);
        lastSlotsColumnHeight = 0;

        for (auto& slotComp : slotComponents)
        {
            slotComp->setBounds (slotsColumn.removeFromTop (slotHeight));
            slotsColumn.removeFromTop (slotGap);
            lastSlotsColumnHeight += slotHeight + slotGap;
        }

        area.removeFromLeft (20);
        pluginAreaTopLeft = area.getPosition();
        pluginAreaMaxBounds = area;

        if (currentEditor != nullptr)
            layoutPluginEditor (*currentEditor);
        else if (currentInternalEditor != nullptr)
            layoutPluginEditor (*currentInternalEditor);
        else
        {
            layoutEmptyDisplay();
            outputPanelBounds = defaultOutputPanelBounds;
        }

        if (onPanelBoundsChanged != nullptr)
            onPanelBoundsChanged();
    }

    std::function<void()> onPrevious;
    std::function<void()> onNext;
    std::function<void (int channelIndex, int slotIndex)> onSlotRightClicked;
    std::function<void()> onPanelBoundsChanged;

private:
    void componentMovedOrResized (juce::Component& component, bool /*wasMoved*/, bool wasResized) override
    {
        if (! wasResized)
            return;

        if (&component == currentEditor.get() || &component == currentInternalEditor.get())
            layoutPluginEditor (component);
    }

    void layoutEmptyDisplay()
    {
        pluginDisplayArea.setBounds (pluginAreaTopLeft.x, pluginAreaTopLeft.y,
                                     pluginAreaMaxBounds.getWidth(), pluginAreaMaxBounds.getHeight());
    }

    void layoutPluginEditor (juce::Component& editor)
    {
        editor.setTopLeftPosition (0, 0);
        auto editorWidth = editor.getWidth();
        auto editorHeight = editor.getHeight();

        if (editorWidth <= 0 || editorHeight <= 0)
        {
            layoutEmptyDisplay();
            return;
        }

        pluginDisplayArea.setBounds (pluginAreaTopLeft.x, pluginAreaTopLeft.y, editorWidth, editorHeight);

        constexpr int outputGap = 20;
        constexpr int outputWidth = 170;

        outputPanelBounds = juce::Rectangle<int> (pluginDisplayArea.getRight() + outputGap,
                                                   defaultOutputPanelBounds.getY(),
                                                   outputWidth,
                                                   defaultOutputPanelBounds.getHeight());

        if (onPanelBoundsChanged != nullptr)
            onPanelBoundsChanged();
    }

    void clearPluginDisplay()
    {
        if (currentEditor != nullptr)
            currentEditor->removeComponentListener (this);
        if (currentInternalEditor != nullptr)
            currentInternalEditor->removeComponentListener (this);

        currentEditor.reset();
        currentInternalEditor.reset();
    }

    int channelIndex = 0;
    AudioChannel* channel = nullptr;
    int selectedSlot = -1;
    juce::Rectangle<int> inputPanelBounds, outputPanelBounds, defaultOutputPanelBounds;
    juce::Point<int> pluginAreaTopLeft;
    juce::Rectangle<int> pluginAreaMaxBounds;
    int lastSlotsColumnHeight = 0;

    juce::TextButton prevButton, nextButton;
    juce::Label channelNameLabel;
    std::vector<std::unique_ptr<RackSlotComponent>> slotComponents;

    juce::Component pluginDisplayArea;
    std::unique_ptr<juce::AudioProcessorEditor> currentEditor;
    std::unique_ptr<CompressorEditor> currentInternalEditor;
};
