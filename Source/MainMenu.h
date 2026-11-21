#pragma once
#include <JuceHeader.h>

// Menu natif de la barre de menu (en haut de l'ecran sur Mac), pour
// regrouper les actions globales : scan, session, reglages audio.
class MainMenu : public juce::MenuBarModel
{
public:
    enum CommandIDs
    {
        scanPlugins = 1,
        saveSession,
        loadSession,
        audioSettings
    };

    juce::StringArray getMenuBarNames() override
    {
        return { "Fichier", "Audio" };
    }

    juce::PopupMenu getMenuForIndex (int topLevelMenuIndex, const juce::String&) override
    {
        juce::PopupMenu menu;

        if (topLevelMenuIndex == 0)
        {
            menu.addItem (saveSession, "Sauvegarder session...");
            menu.addItem (loadSession, "Charger session...");
        }
        else if (topLevelMenuIndex == 1)
        {
            menu.addItem (scanPlugins, "Scanner les plugins VST3");
            menu.addSeparator();
            menu.addItem (audioSettings, "Reglages audio...");
        }

        return menu;
    }

    void menuItemSelected (int menuItemID, int) override
    {
        if (onMenuItemSelected != nullptr)
            onMenuItemSelected (menuItemID);
    }

    std::function<void (int)> onMenuItemSelected;
};
