#pragma once
#include <JuceHeader.h>

// Une ligne a gauche : bouton ON/OFF + nom du groupe (double-clic pour renommer).
// Un cadre est dessine autour de la ligne pour s'aligner visuellement avec
// les cases de la grille a droite (effet tableau).
class LinkGroupRowHeader : public juce::Component
{
public:
    LinkGroupRowHeader (int groupIndex, const juce::String& initialName)
    {
        juce::ignoreUnused (groupIndex);

        onButton.setButtonText ("ON");
        onButton.setClickingTogglesState (true);
        onButton.setToggleState (true, juce::dontSendNotification);
        onButton.onClick = [this] { if (onToggled != nullptr) onToggled (onButton.getToggleState()); };
        addAndMakeVisible (onButton);

        nameLabel.setText (initialName, juce::dontSendNotification);
        nameLabel.setFont (juce::Font (13.0f));
        nameLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        nameLabel.setEditable (false, true, false);
        nameLabel.onTextChange = [this] { if (onRenamed != nullptr) onRenamed (nameLabel.getText()); };
        addAndMakeVisible (nameLabel);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff23272d));
        g.drawRect (getLocalBounds(), 0.5f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (3, 2);
        onButton.setBounds (area.removeFromLeft (34));
        area.removeFromLeft (4);
        nameLabel.setBounds (area);
    }

    void setOn (bool isOn) { onButton.setToggleState (isOn, juce::dontSendNotification); }
    void setGroupName (const juce::String& name) { nameLabel.setText (name, juce::dontSendNotification); }

    juce::TextButton onButton;
    juce::Label nameLabel;
    std::function<void (bool)> onToggled;
    std::function<void (juce::String)> onRenamed;
};

// La grille elle-meme : une case par (groupe x chaine), cliquable. Dessinee
// a la main (pas 512 boutons) pour rester leger. Hauteur de ligne fixe,
// alignee avec les lignes de LinkGroupRowHeader a gauche. L'en-tete affiche
// le numero de chaine, et en dessous le nom pivote a 90 degres (comme sur
// une console), centre verticalement pour rester lisible.
class LinkGroupCellGrid : public juce::Component
{
public:
    static constexpr int numGroups = 16;
    static constexpr int numChannels = 32;
    static constexpr int rowHeight = 26;
    static constexpr int headerHeight = 95;
    static constexpr float numberAreaHeight = 20.0f;

    void setMember (int group, int channel, bool isMember)
    {
        if (group >= 0 && group < numGroups && channel >= 0 && channel < numChannels)
        {
            members[(size_t) group][(size_t) channel] = isMember;
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

        for (int row = 0; row < numGroups; ++row)
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
                    g.setColour (juce::Colour (0xff4ea1ff));
                    g.fillRect (cell.reduced (1.5f));
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

        if (row < 0 || row >= numGroups || col < 0 || col >= numChannels)
            return;

        auto newState = ! members[(size_t) row][(size_t) col];
        members[(size_t) row][(size_t) col] = newState;
        repaint();

        if (onCellToggled != nullptr)
            onCellToggled (row, col, newState);
    }

    std::function<void (int group, int channel, bool nowMember)> onCellToggled;

private:
    std::array<std::array<bool, numChannels>, numGroups> members {};
    std::array<juce::String, numChannels> channelLabels;
};

// Vue PATCH : colonne des groupes a gauche, grille a droite.
class PatchView : public juce::Component
{
public:
    static constexpr int numLinkGroups = LinkGroupCellGrid::numGroups;

    PatchView()
    {
        titleLabel.setText ("LINK GROUPS", juce::dontSendNotification);
        titleLabel.setFont (juce::Font (13.0f, juce::Font::bold));
        titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a92a0));
        addAndMakeVisible (titleLabel);

        for (int i = 0; i < numLinkGroups; ++i)
        {
            auto row = std::make_unique<LinkGroupRowHeader> (i, "Link " + juce::String (i + 1));

            row->onToggled = [this, i] (bool isOn) { if (onGroupToggled != nullptr) onGroupToggled (i, isOn); };
            row->onRenamed = [this, i] (juce::String newName) { if (onGroupRenamed != nullptr) onGroupRenamed (i, newName); };

            addAndMakeVisible (*row);
            rowHeaders.push_back (std::move (row));
        }

        addAndMakeVisible (grid);
        grid.onCellToggled = [this] (int group, int channel, bool nowMember)
        {
            if (onCellToggled != nullptr)
                onCellToggled (group, channel, nowMember);
        };
    }

    void setGroupName (int groupIndex, const juce::String& name)
    {
        if (groupIndex >= 0 && groupIndex < (int) rowHeaders.size())
            rowHeaders[(size_t) groupIndex]->setGroupName (name);
    }

    void setGroupOn (int groupIndex, bool isOn)
    {
        if (groupIndex >= 0 && groupIndex < (int) rowHeaders.size())
            rowHeaders[(size_t) groupIndex]->setOn (isOn);
    }

    void setMember (int groupIndex, int channel, bool isMember) { grid.setMember (groupIndex, channel, isMember); }
    void setChannelLabel (int channel, const juce::String& label) { grid.setChannelLabel (channel, label); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff181b1f));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (16);
        titleLabel.setBounds (area.removeFromTop (20));
        area.removeFromTop (6);

        constexpr int rowHeaderWidth = 150;
        auto leftColumn = area.removeFromLeft (rowHeaderWidth);
        area.removeFromLeft (8);

        leftColumn.removeFromTop (LinkGroupCellGrid::headerHeight);

        for (auto& row : rowHeaders)
            row->setBounds (leftColumn.removeFromTop (LinkGroupCellGrid::rowHeight));

        grid.setBounds (area);
    }

    std::function<void (int groupIndex, bool isOn)> onGroupToggled;
    std::function<void (int groupIndex, juce::String newName)> onGroupRenamed;
    std::function<void (int groupIndex, int channel, bool nowMember)> onCellToggled;

private:
    juce::Label titleLabel;
    std::vector<std::unique_ptr<LinkGroupRowHeader>> rowHeaders;
    LinkGroupCellGrid grid;
};
