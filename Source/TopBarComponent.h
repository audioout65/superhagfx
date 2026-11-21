#pragma once
#include <JuceHeader.h>

class LockButton : public juce::Button
{
public:
    LockButton() : juce::Button ("lock")
    {
        setClickingTogglesState (true);
    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);

        auto baseColour = getToggleState() ? juce::Colours::red : juce::Colour (0xff2c313a);
        if (shouldDrawButtonAsDown)
            baseColour = baseColour.darker (0.2f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter (0.1f);

        g.setColour (baseColour);
        g.fillRoundedRectangle (bounds, 6.0f);
        g.setColour (baseColour.brighter (0.15f));
        g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

        auto centre = bounds.getCentre();
        auto bodyWidth = bounds.getWidth() * 0.40f;
        auto bodyHeight = bounds.getHeight() * 0.30f;
        auto bodyBounds = juce::Rectangle<float> (bodyWidth, bodyHeight).withCentre ({ centre.x, centre.y + bodyHeight * 0.45f });

        auto shackleRadius = bodyWidth * 0.40f;
        auto shackleThickness = juce::jmax (1.6f, bodyWidth * 0.16f);

        if (getToggleState())
        {
            auto shackleCentre = juce::Point<float> (centre.x, bodyBounds.getY());
            g.setColour (juce::Colours::white);
            g.drawEllipse (shackleCentre.x - shackleRadius, shackleCentre.y - shackleRadius,
                           shackleRadius * 2.0f, shackleRadius * 2.0f, shackleThickness);
        }
        else
        {
            juce::Path openShackle;
            auto shackleCentre = juce::Point<float> (centre.x + shackleRadius * 0.6f, bodyBounds.getY() - shackleRadius * 0.5f);
            openShackle.addCentredArc (shackleCentre.x, shackleCentre.y, shackleRadius, shackleRadius,
                                       0.0f,
                                       juce::MathConstants<float>::pi * 1.05f,
                                       juce::MathConstants<float>::pi * 1.95f,
                                       true);
            g.setColour (juce::Colours::white);
            g.strokePath (openShackle, juce::PathStrokeType (shackleThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setColour (juce::Colours::white);
        g.fillRoundedRectangle (bodyBounds, 1.5f);
    }
};

// Une seule petite colonne compacte : un libelle minuscule en haut, une
// fine barre horizontale en dessous qui se remplit selon le niveau.
class CompactMeterColumn : public juce::Component
{
public:
    explicit CompactMeterColumn (const juce::String& labelText)
    {
        label.setText (labelText, juce::dontSendNotification);
        label.setFont (juce::Font (8.0f, juce::Font::bold));
        label.setJustificationType (juce::Justification::centredLeft);
        label.setColour (juce::Label::textColourId, juce::Colour (0xff8a92a0));
        addAndMakeVisible (label);
    }

    void setUsage (double usageRatio)
    {
        usage = juce::jlimit (0.0, 1.0, usageRatio);
        repaint();
    }

    void resized() override
    {
        label.setBounds (getLocalBounds().removeFromTop (11));
        barBounds = getLocalBounds().removeFromBottom (4);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = barBounds.toFloat();

        g.setColour (juce::Colour (0xff0d0f11));
        g.fillRoundedRectangle (bounds, 2.0f);

        auto filledWidth = (float) usage * bounds.getWidth();
        auto filledArea = bounds.withWidth (filledWidth);

        auto colour = usage > 0.9 ? juce::Colours::red
                                  : (usage > 0.7 ? juce::Colours::orange : juce::Colour (0xff4ea1ff));
        g.setColour (colour);
        g.fillRoundedRectangle (filledArea, 2.0f);
    }

private:
    juce::Label label;
    juce::Rectangle<int> barBounds;
    double usage = 0.0;
};

// Pilule arrondie regroupant plusieurs petites colonnes compactes
// (CPU, etc.), dans le meme esprit visuel compact qu'une console pro.
class PillGroup : public juce::Component
{
public:
    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff22262b));
        g.fillRoundedRectangle (bounds, bounds.getHeight() * 0.5f);
        g.setColour (juce::Colour (0xff3d434d));
        g.drawRoundedRectangle (bounds, bounds.getHeight() * 0.5f, 1.0f);
    }
};

// La barre superieure de l'application : logo, onglets de navigation, Link,
// Latency, BPM+TAP, verrouillage, CPU (petite colonne compacte), horloge --
// dans un style compact de pilules, inspire des consoles professionnelles.
class TopBarComponent : public juce::Component,
                        private juce::Timer
{
public:
    TopBarComponent()
        : cpuColumn ("CPU")
    {
        logoLabel.setText ("SHFX", juce::dontSendNotification);
        logoLabel.setFont (juce::Font (20.0f, juce::Font::bold));
        logoLabel.setColour (juce::Label::textColourId, juce::Colour (0xff4ea1ff));
        logoLabel.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (logoLabel);

        linkButton.setButtonText ("LINK");
        linkButton.setClickingTogglesState (true);
        linkButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe8a33d));
        linkButton.onClick = [this] { if (onLinkToggled != nullptr) onLinkToggled (linkButton.getToggleState()); };
        addAndMakeVisible (linkButton);

        latencyButton.setButtonText ("Latency");
        latencyButton.onClick = [this] { showLatencyMenu(); };
        addAndMakeVisible (latencyButton);

        overviewTabButton.setButtonText ("OVERVIEW");
        overviewTabButton.setClickingTogglesState (true);
        overviewTabButton.setRadioGroupId (2001);
        overviewTabButton.setToggleState (true, juce::dontSendNotification);
        overviewTabButton.onClick = [this] { if (onOverviewTabClicked != nullptr) onOverviewTabClicked(); };
        addAndMakeVisible (overviewTabButton);

        rackTabButton.setButtonText ("RACK");
        rackTabButton.setClickingTogglesState (true);
        rackTabButton.setRadioGroupId (2001);
        rackTabButton.onClick = [this] { if (onRackTabClicked != nullptr) onRackTabClicked(); };
        addAndMakeVisible (rackTabButton);

        // --- Pilule BPM : champ + TAP, cote a cote, dans la pilule arrondie ---
        addAndMakeVisible (infoPill);
        bpmEditor.setText ("120", juce::dontSendNotification);
        bpmEditor.setJustification (juce::Justification::centred);
        bpmEditor.setInputRestrictions (3, "0123456789");
        bpmEditor.setFont (13.0f);
        infoPill.addAndMakeVisible (bpmEditor);

        tapButton.setButtonText ("TAP");
        infoPill.addAndMakeVisible (tapButton);
        tapButton.onClick = [this] { registerTap(); };

        infoPill.addAndMakeVisible (cpuColumn);

        clockLabel.setJustificationType (juce::Justification::centred);
        clockLabel.setFont (13.0f);
        infoPill.addAndMakeVisible (clockLabel);

        addAndMakeVisible (lockButton);
        lockButton.onClick = [this] { if (onLockToggled != nullptr) onLockToggled (lockButton.getToggleState()); };

        startTimerHz (2);
        updateClock();
    }

    void setCpuUsage (double usageRatio)
    {
        cpuColumn.setUsage (usageRatio);
    }

    double getBpm() const
    {
        return bpmEditor.getText().getDoubleValue();
    }

    void setActiveTab (bool rackTabActive)
    {
        overviewTabButton.setToggleState (! rackTabActive, juce::dontSendNotification);
        rackTabButton.setToggleState (rackTabActive, juce::dontSendNotification);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff14171a));
        g.fillRect (getLocalBounds());

        g.setColour (juce::Colour (0xff2c313a));
        g.drawLine (0, (float) getHeight() - 1, (float) getWidth(), (float) getHeight() - 1, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8, 4);

        logoLabel.setBounds (area.removeFromLeft (60));
        area.removeFromLeft (12);

        linkButton.setBounds (area.removeFromLeft (50));
        area.removeFromLeft (6);
        latencyButton.setBounds (area.removeFromLeft (64));
        area.removeFromLeft (16);

        lockButton.setBounds (area.removeFromRight (28));
        area.removeFromRight (10);

        infoPill.setBounds (area.removeFromRight (240));
        area.removeFromRight (16);

        auto tabsArea = area.withSizeKeepingCentre (juce::jmin (180, area.getWidth()), area.getHeight());
        overviewTabButton.setBounds (tabsArea.removeFromLeft (tabsArea.getWidth() / 2));
        rackTabButton.setBounds (tabsArea);

        // --- Interieur de la pilule : BPM | TAP | CPU | TIME, serre et compact ---
        auto pillContent = infoPill.getLocalBounds().reduced (10, 6);

        clockLabel.setBounds (pillContent.removeFromRight (56));
        pillContent.removeFromRight (10);

        cpuColumn.setBounds (pillContent.removeFromRight (46));
        pillContent.removeFromRight (10);

        tapButton.setBounds (pillContent.removeFromRight (36));
        pillContent.removeFromRight (6);

        bpmEditor.setBounds (pillContent);
    }

    std::function<void()> onOverviewTabClicked;
    std::function<void()> onRackTabClicked;
    std::function<void (bool linkEnabled)> onLinkToggled;
    std::function<void (bool locked)> onLockToggled;
    std::function<void (int newBufferSize)> onLatencyChanged;

private:
    void timerCallback() override
    {
        updateClock();
    }

    void updateClock()
    {
        auto now = juce::Time::getCurrentTime();
        clockLabel.setText (now.toString (false, true, false, true), juce::dontSendNotification);
    }

    void showLatencyMenu()
    {
        juce::PopupMenu menu;

        for (auto size : { 64, 128, 256, 512, 1024, 2048 })
            menu.addItem (size, juce::String (size) + " samples");

        menu.showMenuAsync (juce::PopupMenu::Options(), [this] (int result)
        {
            if (result > 0 && onLatencyChanged != nullptr)
                onLatencyChanged (result);
        });
    }

    void registerTap()
    {
        auto now = juce::Time::getMillisecondCounterHiRes();

        if (lastTapTimeMs > 0.0 && now - lastTapTimeMs < 2000.0)
        {
            auto intervalMs = now - lastTapTimeMs;
            auto bpm = 60000.0 / intervalMs;

            bpmEditor.setText (juce::String (juce::roundToInt (bpm)), juce::dontSendNotification);
        }

        lastTapTimeMs = now;
    }

    juce::Label logoLabel;
    juce::TextButton overviewTabButton, rackTabButton;
    juce::TextButton linkButton, latencyButton;
    LockButton lockButton;

    PillGroup infoPill;
    juce::TextEditor bpmEditor;
    juce::TextButton tapButton;
    CompactMeterColumn cpuColumn;
    juce::Label clockLabel;

    double lastTapTimeMs = 0.0;
};
