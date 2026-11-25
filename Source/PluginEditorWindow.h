#pragma once
#include <JuceHeader.h>

// Fenetre flottante NON MODALE pour l'editeur d'un plugin (VST3 ou
// compresseur interne). Contrairement a juce::DialogWindow::LaunchOptions
// ::launchAsync() (qui entre en etat modal et bloque toute interaction avec
// le reste de l'application tant qu'elle est ouverte), cette fenetre reste
// independante : on peut continuer a cliquer dans la fenetre principale
// (ajouter des chaines a un bus, changer de chaine, etc.) pendant qu'elle
// est ouverte. Elle reste au premier plan (setAlwaysOnTop) pour rester
// visible, mais ne bloque rien derriere elle.
class PluginEditorWindow : public juce::DocumentWindow
{
public:
    PluginEditorWindow (const juce::String& windowTitle, juce::Component* contentToOwn)
        : juce::DocumentWindow (windowTitle, juce::Colours::black,
                                juce::DocumentWindow::closeButton, true)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, false);
        setContentOwned (contentToOwn, true);
        centreWithSize (getWidth(), getHeight());
        setAlwaysOnTop (true);
        setVisible (true);
    }

    void closeButtonPressed() override
    {
        setVisible (false);

        if (onClosed != nullptr)
            onClosed();
    }

    std::function<void()> onClosed;
};
