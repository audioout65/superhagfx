#include <JuceHeader.h>
#include "MainComponent.h"
#include "LiveRackLookAndFeel.h"
#include "MainMenu.h"

class LiveRackApplication : public juce::JUCEApplication
{
public:
    LiveRackApplication() = default;

    const juce::String getApplicationName() override       { return "LiveRack"; }
    const juce::String getApplicationVersion() override    { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override              { return true; }

    void initialise (const juce::String&) override
    {
        lookAndFeel = std::make_unique<LiveRackLookAndFeel>();
        juce::LookAndFeel::setDefaultLookAndFeel (lookAndFeel.get());

        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override
    {
        juce::MenuBarModel::setMacMainMenu (nullptr);
        mainWindow = nullptr;
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
        lookAndFeel = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (juce::String name)
            : DocumentWindow (name,
                               juce::Desktop::getInstance().getDefaultLookAndFeel()
                                   .findColour (juce::ResizableWindow::backgroundColourId),
                               DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);

            auto* content = new MainComponent();
            setContentOwned (content, true);

            mainMenu = std::make_unique<MainMenu>();
            mainMenu->onMenuItemSelected = [content] (int menuItemID)
            {
                content->handleMenuCommand (menuItemID);
            };
            juce::MenuBarModel::setMacMainMenu (mainMenu.get());

            // Taille minimale largement superieure a ce que demandent les
            // plugins les plus larges (Pro-Q3, etc.), pour que le panneau
            // OUTPUT garde toujours sa place sans jamais etre recouvert.
            setResizeLimits (1600, 800, 10000, 10000);

            centreWithSize (1700, 900);
            setResizable (true, true);
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        std::unique_ptr<MainMenu> mainMenu;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<LiveRackLookAndFeel> lookAndFeel;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (LiveRackApplication)
