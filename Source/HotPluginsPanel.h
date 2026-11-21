#pragma once
#include <JuceHeader.h>

// Panneau de raccourcis "Hot Plugins" (favoris), visible en permanence a
// droite de l'ecran (OVERVIEW et RACK). Pour l'instant : juste la structure
// visuelle (grille de cases numerotees) ; la sauvegarde/chargement de
// presets avec leurs reglages viendra dans une prochaine etape.
class HotPluginsPanel : public juce::Component
{
public:
    static constexpr int numSlots = 12;
    static constexpr int numColumns = 2;

    HotPluginsPanel()
    {
        for (int i = 0; i < numSlots; ++i)
        {
            auto btn = std::make_unique<juce::TextButton>();
            btn->setButtonText (juce::String (i + 1));
            btn->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2c313a));
            btn->setColour (juce::TextButton::textColourOffId, juce::Colours::white);

            auto indexCopy = i;
            btn->onClick = [this, indexCopy]
            {
                if (onSlotClicked != nullptr)
                    onSlotClicked (indexCopy);
            };

            addAndMakeVisible (*btn);
            slotButtons.push_back (std::move (btn));
        }
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff15171a));

        g.setColour (juce::Colour (0xff8a93a1));
        g.setFont (juce::Font (13.0f, juce::Font::bold));
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

    std::function<void (int index)> onSlotClicked;

private:
    std::vector<std::unique_ptr<juce::TextButton>> slotButtons;
};
