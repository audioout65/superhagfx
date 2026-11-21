#pragma once
#include <JuceHeader.h>

// Bouton d'emplacement de plugin. Detecte le clic droit pour afficher un
// menu contextuel distinct du clic gauche normal. Affiche une miniature
// reelle de l'interface du plugin charge (une capture de son editeur),
// quand elle est disponible.
class SlotButton : public juce::TextButton
{
public:
    explicit SlotButton (const juce::String& buttonName) : juce::TextButton (buttonName) {}

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        {
            if (onRightClick != nullptr)
                onRightClick();
            return;
        }

        juce::TextButton::mouseDown (e);
    }

    // L'image miniature a afficher a gauche du nom. Une image par defaut
    // (invalide) efface la miniature.
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

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::TextButton::paintButton (g, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

        if (! thumbnailImage.isValid())
            return;

        auto bounds = getLocalBounds().toFloat();
        auto thumbnailSize = juce::jmin (bounds.getHeight() - 6.0f, 30.0f);
        auto thumbnailBounds = juce::Rectangle<float> (thumbnailSize, thumbnailSize)
                                    .withCentre ({ bounds.getX() + 6.0f + thumbnailSize * 0.5f, bounds.getCentreY() });

        g.saveState();
        juce::Path clipPath;
        clipPath.addRoundedRectangle (thumbnailBounds, 3.0f);
        g.reduceClipRegion (clipPath);

        g.drawImage (thumbnailImage, thumbnailBounds, juce::RectanglePlacement::fillDestination);

        g.restoreState();

        g.setColour (juce::Colour (0xff15171a));
        g.drawRoundedRectangle (thumbnailBounds, 3.0f, 1.0f);
    }

    std::function<void()> onRightClick;

private:
    juce::Image thumbnailImage;
};
