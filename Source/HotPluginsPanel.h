#pragma once
#include <JuceHeader.h>
#include "HotPluginSlot.h"

// Une case individuelle du panneau Hot Plugins : affiche le nom du plugin
// sauvegarde et sa miniature (reutilisee depuis le slot d'origine).
class HotPluginButton : public juce::Component
{
public:
    void setData (const juce::String& name, const juce::Image& thumb, bool hasData)
    {
        displayName = name;
        thumbnail = thumb;
        hasPlugin = hasData;
        repaint();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        {
            if (onRightClick != nullptr)
                onRightClick();
        }
        else
        {
            if (onClick != nullptr)
                onClick();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour (hasPlugin ? juce::Colour (0xff2c313a) : juce::Colour (0xff1c1f24));
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (juce::Colour (0xff101214));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

        if (! hasPlugin)
        {
            g.setColour (juce::Colour (0xff4a5260));
            g.setFont (juce::Font (juce::FontOptions (11.0f)));
            g.drawText ("vide", getLocalBounds(), juce::Justification::centred);
            return;
        }

        auto nameArea = getLocalBounds().removeFromTop (14);
        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.drawFittedText (displayName, nameArea.reduced (2, 0), juce::Justification::centred, 1);

        auto thumbArea = getLocalBounds().withTrimmedTop (16).toFloat().reduced (2.0f);

        if (thumbnail.isValid())
        {
            g.saveState();
            juce::Path clip;
            clip.addRoundedRectangle (thumbArea, 2.0f);
            g.reduceClipRegion (clip);

            g.setColour (juce::Colour (0xff0d0f11));
            g.fillRoundedRectangle (thumbArea, 2.0f);

            g.drawImage (thumbnail, thumbArea, juce::RectanglePlacement::centred);
            g.restoreState();
        }
        else
        {
            g.setColour (juce::Colour (0xff0d0f11));
            g.fillRoundedRectangle (thumbArea, 2.0f);
        }
    }

    std::function<void()> onClick;
    std::function<void()> onRightClick;

private:
    juce::String displayName;
    juce::Image thumbnail;
    bool hasPlugin = false;
};

// Panneau de raccourcis "Hot Plugins" (favoris), visible en permanence a
// droite de l'ecran (OVERVIEW et RACK). Le chargement d'un Hot Plugin dans
// un slot se fait via le menu habituel d'un emplacement (clic sur un
// emplacement vide, ou clic droit) ; clic droit ici vide ce Hot Plugin.
class HotPluginsPanel : public juce::Component
{
public:
    static constexpr int numSlots = 12;
    static constexpr int numColumns = 2;

    HotPluginsPanel()
    {
        for (int i = 0; i < numSlots; ++i)
        {
            auto btn = std::make_unique<HotPluginButton>();
            auto indexCopy = i;

            btn->onRightClick = [this, indexCopy]
            {
                if (onSlotRightClicked != nullptr)
                    onSlotRightClicked (indexCopy);
            };

            addAndMakeVisible (*btn);
            slotButtons.push_back (std::move (btn));
        }
    }

    void refreshSlot (int index, const HotPluginSlot& data)
    {
        if (index < 0 || index >= (int) slotButtons.size())
            return;

        slotButtons[(size_t) index]->setData (data.displayName, data.thumbnail, data.hasPlugin);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff15171a));

        g.setColour (juce::Colour (0xff8a93a1));
        g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        g.drawText ("HOT PLUGINS", getLocalBounds().removeFromTop (20), juce::Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (6);
        area.removeFromTop (20);

        constexpr int gap = 6;
        constexpr int rowHeight = 60;
        auto numRows = (numSlots + numColumns - 1) / numColumns;
        auto colWidth = (area.getWidth() - gap * (numColumns - 1)) / numColumns;

        for (int row = 0; row < numRows; ++row)
        {
            auto rowArea = area.removeFromTop (rowHeight);

            for (int col = 0; col < numColumns; ++col)
            {
                auto index = row * numColumns + col;
                if (index >= numSlots)
                    continue;

                auto slotArea = rowArea.removeFromLeft (colWidth);
                slotButtons[(size_t) index]->setBounds (slotArea.reduced (2));

                if (col < numColumns - 1)
                    rowArea.removeFromLeft (gap);
            }

            area.removeFromTop (gap);
        }
    }

    std::function<void (int index)> onSlotRightClicked;

private:
    std::vector<std::unique_ptr<HotPluginButton>> slotButtons;
};
