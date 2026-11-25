#pragma once
#include <JuceHeader.h>
#include "SlotButton.h"

// Une ligne de bus a gauche : ON/OFF, nom (double-clic pour renommer),
// bouton plugin (montre le nom du plugin charge, "+" si vide -- clic
// gauche ouvre son interface si un plugin est deja charge, clic droit
// ouvre le menu pour en choisir/changer/vider un, comme les slots d'une
// chaine), sortie physique (Out 1-16) et gain -- comme une mini-chaine.
// Meme principe de cadre que LinkGroupRowHeader (PatchView.h) pour
// l'alignement visuel.
class SendBusRowHeader : public juce::Component
{
public:
    SendBusRowHeader (int busIndex, const juce::String& initialName)
        : pluginButton ("+")
    {
        juce::ignoreUnused (busIndex);

        onButton.setButtonText ("ON");
        onButton.setClickingTogglesState (true);
        onButton.onClick = [this] { if (onToggled != nullptr) onToggled (onButton.getToggleState()); };
        addAndMakeVisible (onButton);

        nameLabel.setText (initialName, juce::dontSendNotification);
        nameLabel.setFont (juce::Font (13.0f));
        nameLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        nameLabel.setEditable (false, true, false);
        nameLabel.onTextChange = [this] { if (onRenamed != nullptr) onRenamed (nameLabel.getText()); };
        addAndMakeVisible (nameLabel);

        pluginButton.onClick = [this] { if (onPluginClicked != nullptr) onPluginClicked(); };
        pluginButton.onRightClick = [this] { if (onPluginRightClicked != nullptr) onPluginRightClicked(); };
        addAndMakeVisible (pluginButton);

        for (int i = 1; i <= 16; ++i)
            outputCombo.addItem ("Out " + juce::String (i), i);
        outputCombo.setSelectedId (1, juce::dontSendNotification);
        outputCombo.onChange = [this]
        {
            if (onOutputChanged != nullptr)
                onOutputChanged (outputCombo.getSelectedId() - 1);
        };
        addAndMakeVisible (outputCombo);

        gainKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        gainKnob.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                      juce::MathConstants<float>::pi * 2.76f, true);
        gainKnob.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        gainKnob.setRange (-60.0, 12.0, 0.1);
        gainKnob.setDoubleClickReturnValue (true, 0.0);
        gainKnob.onValueChange = [this]
        {
            if (onGainChanged != nullptr)
                onGainChanged ((float) gainKnob.getValue());
        };
        addAndMakeVisible (gainKnob);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff23272d));
        g.drawRect (getLocalBounds(), 0.5f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (4, 3);

        auto topRow = area.removeFromTop (20);
        onButton.setBounds (topRow.removeFromLeft (34));
        topRow.removeFromLeft (4);
        nameLabel.setBounds (topRow);

        area.removeFromTop (3);

        auto bottomRow = area;
        pluginButton.setBounds (bottomRow.removeFromLeft (56));
        bottomRow.removeFromLeft (4);
        outputCombo.setBounds (bottomRow.removeFromLeft (64));
        bottomRow.removeFromLeft (4);
        gainKnob.setBounds (bottomRow);
    }

    void setOn (bool isOn) { onButton.setToggleState (isOn, juce::dontSendNotification); }
    void setBusName (const juce::String& name) { nameLabel.setText (name, juce::dontSendNotification); }
    void setPluginName (const juce::String& name) { pluginButton.setButtonText (name); }
    void setOutputChannelIndex (int index) { outputCombo.setSelectedId (index + 1, juce::dontSendNotification); }
    void setGainDb (float db) { gainKnob.setValue (db, juce::dontSendNotification); }

    std::function<void (bool)> onToggled;
    std::function<void (juce::String)> onRenamed;
    std::function<void()> onPluginClicked;
    std::function<void()> onPluginRightClicked;
    std::function<void (int outputChannelIndex)> onOutputChanged;
    std::function<void (float gainDb)> onGainChanged;

private:
    juce::TextButton onButton;
    juce::Label nameLabel;
    SlotButton pluginButton;
    juce::ComboBox outputCombo;
    juce::Slider gainKnob;
};

// La grille : une case par (bus x chaine). Clic gauche = ajoute/retire la
// chaine du bus (devient membre en POST par defaut). Clic droit sur une
// case deja membre = bascule PRE/POST (bleu = post-effets, orange =
// pre-effets). Meme principe de dessin a la main que LinkGroupCellGrid
// (PatchView.h), avec les noms de chaine pivotes en en-tete.
class SendBusCellGrid : public juce::Component
{
public:
    static constexpr int numBuses = 8;
    static constexpr int numChannels = 32;
    static constexpr int rowHeight = 66;
    static constexpr int headerHeight = 95;
    static constexpr float numberAreaHeight = 20.0f;

    void setMember (int bus, int channel, bool isMember)
    {
        if (bus >= 0 && bus < numBuses && channel >= 0 && channel < numChannels)
        {
            members[(size_t) bus][(size_t) channel] = isMember;
            repaint();
        }
    }

    void setMemberIsPre (int bus, int channel, bool isPre)
    {
        if (bus >= 0 && bus < numBuses && channel >= 0 && channel < numChannels)
        {
            memberIsPre[(size_t) bus][(size_t) channel] = isPre;
            repaint();
        }
    }

    void setChannelLabel (int channel, const juce::String& label)
    {
        if (channel >= 0 && channel < numChannels)
        {
            channelLabels[(size_t) channel] = label;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto width = getWidth();
        auto cellWidth = (float) width / (float) numChannels;
        auto nameAreaHeight = (float) headerHeight - numberAreaHeight;

        g.setColour (juce::Colour (0xff1a1e23));
        g.fillRect (0, 0, width, headerHeight);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto x = (float) ch * cellWidth;

            g.setColour (juce::Colour (0xff23272d));
            g.drawRect (juce::Rectangle<float> (x, 0.0f, cellWidth, (float) headerHeight), 0.5f);

            g.setFont (juce::Font (12.0f, juce::Font::bold));
            g.setColour (juce::Colour (0xffb5bbc6));
            g.drawText (juce::String (ch + 1),
                       juce::Rectangle<float> (x, 2.0f, cellWidth, numberAreaHeight - 2.0f).toNearestInt(),
                       juce::Justification::centred);

            if (channelLabels[(size_t) ch].isNotEmpty())
            {
                g.saveState();

                auto anchorX = x + cellWidth * 0.5f;
                auto anchorY = (float) headerHeight;

                g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi)
                                                       .translated (anchorX, anchorY));

                g.setFont (juce::Font (11.0f));
                g.setColour (juce::Colour (0xff9aa2b0));
                g.drawText (channelLabels[(size_t) ch],
                           juce::Rectangle<float> (0.0f, -cellWidth * 0.5f, nameAreaHeight, cellWidth),
                           juce::Justification::centred);

                g.restoreState();
            }
        }

        for (int row = 0; row < numBuses; ++row)
        {
            auto y = headerHeight + row * rowHeight;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto x = (float) ch * cellWidth;
                juce::Rectangle<float> cell (x, (float) y, cellWidth, (float) rowHeight);

                g.setColour (juce::Colour (0xff23272d));
                g.drawRect (cell, 0.5f);

                if (members[(size_t) row][(size_t) ch])
                {
                    g.setColour (memberIsPre[(size_t) row][(size_t) ch] ? juce::Colour (0xffe8a33d)
                                                                         : juce::Colour (0xff4ea1ff));
                    g.fillRect (cell.reduced (1.5f));

                    g.setColour (juce::Colours::black.withAlpha (0.55f));
                    g.setFont (juce::Font (9.0f, juce::Font::bold));
                    g.drawText (memberIsPre[(size_t) row][(size_t) ch] ? "PRE" : "POST",
                               cell.toNearestInt(), juce::Justification::centred);
                }
            }
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.y < headerHeight)
            return;

        auto cellWidth = (float) getWidth() / (float) numChannels;

        auto col = (int) ((float) e.x / cellWidth);
        auto row = (e.y - headerHeight) / rowHeight;

        if (row < 0 || row >= numBuses || col < 0 || col >= numChannels)
            return;

        if (e.mods.isPopupMenu())
        {
            if (! members[(size_t) row][(size_t) col])
                return;

            auto newIsPre = ! memberIsPre[(size_t) row][(size_t) col];
            memberIsPre[(size_t) row][(size_t) col] = newIsPre;
            repaint();

            if (onCellPrePostToggled != nullptr)
                onCellPrePostToggled (row, col, newIsPre);

            return;
        }

        auto newState = ! members[(size_t) row][(size_t) col];
        members[(size_t) row][(size_t) col] = newState;

        if (newState)
            memberIsPre[(size_t) row][(size_t) col] = false;

        repaint();

        if (onCellToggled != nullptr)
            onCellToggled (row, col, newState);
    }

    std::function<void (int bus, int channel, bool nowMember)> onCellToggled;
    std::function<void (int bus, int channel, bool nowIsPre)> onCellPrePostToggled;

private:
    std::array<std::array<bool, numChannels>, numBuses> members {};
    std::array<std::array<bool, numChannels>, numBuses> memberIsPre {};
    std::array<juce::String, numChannels> channelLabels;
};

// Vue SEND : colonne des bus a gauche, grille a droite. Meme squelette que
// PatchView (SHOW/PATCH), adapte aux bus (plus de controles par ligne).
class SendView : public juce::Component
{
public:
    static constexpr int numSendBuses = SendBusCellGrid::numBuses;

    SendView()
    {
        titleLabel.setText ("SEND", juce::dontSendNotification);
        titleLabel.setFont (juce::Font (13.0f, juce::Font::bold));
        titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a92a0));
        addAndMakeVisible (titleLabel);

        for (int i = 0; i < numSendBuses; ++i)
        {
            auto row = std::make_unique<SendBusRowHeader> (i, "Bus " + juce::String (i + 1));

            row->onToggled = [this, i] (bool isOn) { if (onBusToggled != nullptr) onBusToggled (i, isOn); };
            row->onRenamed = [this, i] (juce::String newName) { if (onBusRenamed != nullptr) onBusRenamed (i, newName); };
            row->onPluginClicked = [this, i] { if (onBusPluginClicked != nullptr) onBusPluginClicked (i); };
            row->onPluginRightClicked = [this, i] { if (onBusPluginRightClicked != nullptr) onBusPluginRightClicked (i); };
            row->onOutputChanged = [this, i] (int outIndex) { if (onBusOutputChanged != nullptr) onBusOutputChanged (i, outIndex); };
            row->onGainChanged = [this, i] (float gainDb) { if (onBusGainChanged != nullptr) onBusGainChanged (i, gainDb); };

            addAndMakeVisible (*row);
            rowHeaders.push_back (std::move (row));
        }

        addAndMakeVisible (grid);
        grid.onCellToggled = [this] (int bus, int channel, bool nowMember)
        {
            if (onCellToggled != nullptr)
                onCellToggled (bus, channel, nowMember);
        };
        grid.onCellPrePostToggled = [this] (int bus, int channel, bool nowIsPre)
        {
            if (onCellPrePostToggled != nullptr)
                onCellPrePostToggled (bus, channel, nowIsPre);
        };
    }

    void setBusName (int busIndex, const juce::String& name)
    {
        if (busIndex >= 0 && busIndex < (int) rowHeaders.size())
            rowHeaders[(size_t) busIndex]->setBusName (name);
    }

    void setBusOn (int busIndex, bool isOn)
    {
        if (busIndex >= 0 && busIndex < (int) rowHeaders.size())
            rowHeaders[(size_t) busIndex]->setOn (isOn);
    }

    void setBusPluginName (int busIndex, const juce::String& name)
    {
        if (busIndex >= 0 && busIndex < (int) rowHeaders.size())
            rowHeaders[(size_t) busIndex]->setPluginName (name);
    }

    void setBusOutputChannelIndex (int busIndex, int outputChannelIndex)
    {
        if (busIndex >= 0 && busIndex < (int) rowHeaders.size())
            rowHeaders[(size_t) busIndex]->setOutputChannelIndex (outputChannelIndex);
    }

    void setBusGainDb (int busIndex, float gainDb)
    {
        if (busIndex >= 0 && busIndex < (int) rowHeaders.size())
            rowHeaders[(size_t) busIndex]->setGainDb (gainDb);
    }

    void setMember (int busIndex, int channel, bool isMember) { grid.setMember (busIndex, channel, isMember); }
    void setMemberIsPre (int busIndex, int channel, bool isPre) { grid.setMemberIsPre (busIndex, channel, isPre); }
    void setChannelLabel (int channel, const juce::String& label) { grid.setChannelLabel (channel, label); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff181b1f));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (16);

        auto titleArea = area.removeFromTop (22);
        titleLabel.setBounds (titleArea);

        area.removeFromTop (6);

        constexpr int rowHeaderWidth = 260;
        auto leftColumn = area.removeFromLeft (rowHeaderWidth);
        area.removeFromLeft (8);

        leftColumn.removeFromTop (SendBusCellGrid::headerHeight);

        for (auto& row : rowHeaders)
            row->setBounds (leftColumn.removeFromTop (SendBusCellGrid::rowHeight));

        grid.setBounds (area);
    }

    std::function<void (int busIndex, bool isOn)> onBusToggled;
    std::function<void (int busIndex, juce::String newName)> onBusRenamed;
    std::function<void (int busIndex)> onBusPluginClicked;
    std::function<void (int busIndex)> onBusPluginRightClicked;
    std::function<void (int busIndex, int outputChannelIndex)> onBusOutputChanged;
    std::function<void (int busIndex, float gainDb)> onBusGainChanged;
    std::function<void (int busIndex, int channel, bool nowMember)> onCellToggled;
    std::function<void (int busIndex, int channel, bool nowIsPre)> onCellPrePostToggled;

private:
    juce::Label titleLabel;
    std::vector<std::unique_ptr<SendBusRowHeader>> rowHeaders;
    SendBusCellGrid grid;
};
