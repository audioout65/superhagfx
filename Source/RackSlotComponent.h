#pragma once
#include <JuceHeader.h>

class RackSlotComponent : public juce::Component
{
public:
    RackSlotComponent()
    {
        nameButton.setButtonText ("ADD PLUGIN");
        nameButton.onClick = [this] { if (onMenuRequested != nullptr) onMenuRequested(); };
        addAndMakeVisible (nameButton);

        inButton.setButtonText ("IN");
        inButton.setClickingTogglesState (true);
        inButton.setToggleState (true, juce::dontSendNotification);
        inButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff3fae54));
        inButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        inButton.onClick = [this] { if (onInToggled != nullptr) onInToggled (inButton.getToggleState()); };
        addAndMakeVisible (inButton);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        {
            if (onMenuRequested != nullptr)
                onMenuRequested();
            return;
        }

        if (hasPlugin)
        {
            if (onThumbnailClicked != nullptr)
                onThumbnailClicked();
        }
        else
        {
            if (onMenuRequested != nullptr)
                onMenuRequested();
        }
    }

    void setPluginName (const juce::String& name)
    {
        hasPlugin = name.isNotEmpty() && name != "+";
        nameButton.setButtonText (hasPlugin ? name : "ADD PLUGIN");
        repaint();
    }

    void setInState (bool isIn)
    {
        inButton.setToggleState (isIn, juce::dontSendNotification);
    }

    void setMeterLevel (float level)
    {
        displayedLevel = level;
        repaint();
    }

    void setThumbnail (const juce::Image& image)
    {
        thumbnailImage = image;
        repaint();
    }

    void clearThumbnail()
    {
        thumbnailImage = juce::Image();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Plus de fond bleu : meme couleur neutre, plugin charge ou non.
        g.setColour (juce::Colour (0xff2c313a));
        g.fillRoundedRectangle (bounds, 5.0f);
        g.setColour (juce::Colour (0xff15171a));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);

        if (! hasPlugin)
            return;

        auto meterArea = meterBounds.toFloat();
        g.setColour (juce::Colour (0xff0d0f11));
        g.fillRoundedRectangle (meterArea, 2.0f);

        auto level = juce::jlimit (0.0f, 1.0f, displayedLevel);
        auto filledHeight = level * meterArea.getHeight();
        auto filledArea = meterArea.withTop (meterArea.getBottom() - filledHeight);
        g.setColour (juce::Colours::green);
        g.fillRoundedRectangle (filledArea.reduced (1.0f), 1.5f);

        if (thumbnailImage.isValid())
        {
            g.saveState();
            juce::Path clipPath;
            clipPath.addRoundedRectangle (thumbnailBounds, 3.0f);
            g.reduceClipRegion (clipPath);

            g.setColour (juce::Colour (0xff0d0f11));
            g.fillRoundedRectangle (thumbnailBounds, 3.0f);

            g.drawImage (thumbnailImage, thumbnailBounds, juce::RectanglePlacement::centred);

            g.restoreState();
        }
        else
        {
            g.setColour (juce::Colour (0xff1a1d21));
            g.fillRoundedRectangle (thumbnailBounds, 3.0f);
        }

        g.setColour (juce::Colour (0xff15171a));
        g.drawRoundedRectangle (thumbnailBounds, 3.0f, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (4);

        nameButton.setBounds (area.removeFromTop (22));
        area.removeFromTop (4);

        // Colonne de gauche (IN + mini vumetre empiles) sur toute la hauteur
        // restante, puis la miniature a droite sur cette meme hauteur.
        auto leftColumn = area.removeFromLeft (34);
        area.removeFromLeft (4);

        inButton.setBounds (leftColumn.removeFromTop (22));
        leftColumn.removeFromTop (4);
        meterBounds = leftColumn;

        thumbnailBounds = area.toFloat();
    }

    std::function<void()> onMenuRequested;
    std::function<void()> onThumbnailClicked;
    std::function<void (bool isIn)> onInToggled;

private:
    juce::TextButton nameButton;
    juce::TextButton inButton;
    juce::Rectangle<int> meterBounds;
    juce::Rectangle<float> thumbnailBounds;
    juce::Image thumbnailImage;
    float displayedLevel = 0.0f;
    bool hasPlugin = false;
};
