#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"
#include "SlotButton.h"
#include "CompressorEditor.h"

// Vue detaillee d'un seul canal : entree/sortie en haut, pile de slots a
// gauche, editeur du plugin selectionne a droite. Le vumetre/fader/IN sont
// geres a part par MeterAndGainOverlay, toujours au premier plan.
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
        addAndMakeVisible (channelNameLabel);

        inputLabel.setText ("Input", juce::dontSendNotification);
        inputLabel.setJustificationType (juce::Justification::centredLeft);
        inputLabel.setFont (12.0f);
        addAndMakeVisible (inputLabel);

        outputLabel.setText ("Output", juce::dontSendNotification);
        outputLabel.setJustificationType (juce::Justification::centredLeft);
        outputLabel.setFont (12.0f);
        addAndMakeVisible (outputLabel);

        for (int i = 1; i <= 32; ++i)
            inputCombo.addItem ("In " + juce::String (i), i);

        for (int i = 1; i <= 16; ++i)
            outputCombo.addItem ("Out " + juce::String (i), i);

        addAndMakeVisible (inputCombo);
        addAndMakeVisible (outputCombo);

        for (int slot = 0; slot < numSlotsPerChannel; ++slot)
        {
            auto slotBtn = std::make_unique<SlotButton>("ADD PLUGIN");
            auto slotIndexCopy = slot;

            slotBtn->onClick = [this, slotIndexCopy]
            {
                selectSlotForDisplay (slotIndexCopy);

                if (onSlotClicked != nullptr)
                    onSlotClicked (channelIndex, slotIndexCopy);
            };
            slotBtn->onRightClick = [this, slotIndexCopy]
            {
                if (onSlotRightClicked != nullptr)
                    onSlotRightClicked (channelIndex, slotIndexCopy);
            };

            addAndMakeVisible (*slotBtn);
            slotButtons.push_back (std::move (slotBtn));
        }

        pluginViewport.setViewedComponent (&pluginDisplayArea, false);
        pluginViewport.setScrollBarsShown (true, true);
        addAndMakeVisible (pluginViewport);
    }

    void setChannel (int newChannelIndex, AudioChannel& newChannel)
    {
        channelIndex = newChannelIndex;
        channel = &newChannel;

        channelNameLabel.setText ("Ch " + juce::String (channelIndex + 1), juce::dontSendNotification);

        inputCombo.onChange = nullptr;
        outputCombo.onChange = nullptr;

        inputCombo.setSelectedId (channel->getInputChannelIndex() + 1, juce::dontSendNotification);
        outputCombo.setSelectedId (channel->getOutputChannelIndex() + 1, juce::dontSendNotification);

        inputCombo.onChange = [this]
        {
            if (channel != nullptr)
                channel->setInputChannelIndex (inputCombo.getSelectedId() - 1);
        };
        outputCombo.onChange = [this]
        {
            if (channel != nullptr)
                channel->setOutputChannelIndex (outputCombo.getSelectedId() - 1);
        };

        selectedSlot = -1;
        clearPluginDisplay();
        refreshSlots();
    }

    // La zone reservee au vumetre/fader/IN, a droite, pour que MainComponent
    // puisse y placer l'overlay toujours au premier plan.
    juce::Rectangle<int> getOverlayBounds() const { return overlayBounds; }

    void refreshSlots()
    {
        if (channel == nullptr)
            return;

        for (int slot = 0; slot < numSlotsPerChannel; ++slot)
        {
            auto& pluginSlot = channel->getSlot (slot);
            auto name = pluginSlot.getDisplayName();
            auto isLoaded = ! pluginSlot.isEmpty();

            slotButtons[(size_t) slot]->setButtonText (isLoaded ? name : "ADD PLUGIN");
            slotButtons[(size_t) slot]->setColour (juce::TextButton::buttonColourId,
                isLoaded ? juce::Colour (0xff2d6fc4) : juce::Colour (0xff2c313a));
        }

        if (selectedSlot >= 0)
            selectSlotForDisplay (selectedSlot);
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

        auto ioArea = area.removeFromTop (70);
        auto inputArea = ioArea.removeFromLeft (ioArea.getWidth() / 2);
        inputLabel.setBounds (inputArea.removeFromTop (18));
        inputCombo.setBounds (inputArea.removeFromTop (30));

        ioArea.removeFromLeft (20);
        outputLabel.setBounds (ioArea.removeFromTop (18));
        outputCombo.setBounds (ioArea.removeFromTop (30));

        area.removeFromTop (16);

        // Reserve toujours 160px pour l'overlay (vumetre/fader/IN), quelle
        // que soit la largeur de la zone restante pour le plugin.
        overlayBounds = area.removeFromRight (160);

        area.removeFromRight (20);

        auto slotsColumn = area.removeFromLeft (280);

        for (auto& btn : slotButtons)
        {
            btn->setBounds (slotsColumn.removeFromTop (44));
            slotsColumn.removeFromTop (8);
        }

        area.removeFromLeft (20);
        pluginViewport.setBounds (area);

        if (currentEditor != nullptr)
            layoutPluginEditor (*currentEditor);
        else if (currentInternalEditor != nullptr)
            layoutPluginEditor (*currentInternalEditor);
        else
            layoutEmptyDisplay();

        if (onOverlayBoundsChanged != nullptr)
            onOverlayBoundsChanged();
    }

    std::function<void()> onPrevious;
    std::function<void()> onNext;
    std::function<void (int channelIndex, int slotIndex)> onSlotClicked;
    std::function<void (int channelIndex, int slotIndex)> onSlotRightClicked;
    std::function<void()> onOverlayBoundsChanged;

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
    juce::Rectangle<int> overlayBounds;

    juce::TextButton prevButton, nextButton;
    juce::Label channelNameLabel;
    juce::Label inputLabel, outputLabel;
    juce::ComboBox inputCombo, outputCombo;
    std::vector<std::unique_ptr<SlotButton>> slotButtons;

    juce::Viewport pluginViewport;
    juce::Component pluginDisplayArea;
    std::unique_ptr<juce::AudioProcessorEditor> currentEditor;
    std::unique_ptr<CompressorEditor> currentInternalEditor;
};
