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
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (baseColour.brighter (0.15f));
        g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

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

// Une seule petite colonne compacte : un libelle en haut, une fine barre
// horizontale en dessous qui se remplit selon le niveau.
class CompactMeterColumn : public juce::Component
{
public:
    explicit CompactMeterColumn (const juce::String& labelText)
    {
        label.setText (labelText, juce::dontSendNotification);
        label.setFont (juce::Font (9.0f, juce::Font::bold));
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
        label.setBounds (getLocalBounds().removeFromTop (14));
        barBounds = getLocalBounds().removeFromBottom (6);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = barBounds.toFloat();

        g.setColour (juce::Colour (0xff0d0f11));
        g.fillRoundedRectangle (bounds, 3.0f);

        auto filledWidth = (float) usage * bounds.getWidth();
        auto filledArea = bounds.withWidth (filledWidth);

        auto colour = usage > 0.9 ? juce::Colours::red
                                  : (usage > 0.7 ? juce::Colours::orange : juce::Colour (0xff4ea1ff));
        g.setColour (colour);
        g.fillRoundedRectangle (filledArea, 3.0f);
    }

private:
    juce::Label label;
    juce::Rectangle<int> barBounds;
    double usage = 0.0;
};

// Pilule arrondie regroupant les petites colonnes (BPM/CPU, TAP/horloge).
class PillGroup : public juce::Component
{
public:
    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff22262b));
        g.fillRoundedRectangle (bounds, 10.0f);
        g.setColour (juce::Colour (0xff3d434d));
        g.drawRoundedRectangle (bounds, 10.0f, 1.0f);
    }
};

// Petite fleche de navigation (< ou >), style discret.
class NavArrowButton : public juce::TextButton
{
public:
    explicit NavArrowButton (const juce::String& symbol) : juce::TextButton (symbol) {}

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        g.setColour (shouldDrawButtonAsDown ? juce::Colour (0xff4ea1ff)
                                             : (shouldDrawButtonAsHighlighted ? juce::Colours::white : juce::Colour (0xff8a92a0)));
        g.setFont (juce::Font (20.0f, juce::Font::bold));
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred);
    }
};

// Le nom de la chaine selectionnee, en gros, souligne, avec un petit
// chevron en dessous : cliquer dessus (ou sur le chevron) ouvre la liste
// complete des chaines.
class ChannelPickerButton : public juce::TextButton
{
public:
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool) override
    {
        auto bounds = getLocalBounds();
        auto chevronArea = bounds.removeFromBottom (10);

        g.setFont (juce::Font (22.0f, juce::Font::bold));
        g.setColour (juce::Colours::white);
        g.drawFittedText (getButtonText(), bounds, juce::Justification::centred, 1);

        auto underlineWidth = (float) bounds.getWidth() * 0.6f;
        auto underlineY = (float) bounds.getBottom() - 4.0f;
        auto cx = (float) bounds.getCentreX();
        g.setColour (juce::Colour (0xff4ea1ff));
        g.fillRect (juce::Rectangle<float> (cx - underlineWidth * 0.5f, underlineY, underlineWidth, 2.0f));

        auto ccx = (float) chevronArea.getCentreX();
        auto ccy = (float) chevronArea.getCentreY();
        juce::Path chevron;
        chevron.addTriangle (ccx - 5.0f, ccy - 2.0f, ccx + 5.0f, ccy - 2.0f, ccx, ccy + 3.0f);
        g.setColour (shouldDrawButtonAsHighlighted ? juce::Colours::white : juce::Colour (0xff8a92a0));
        g.fillPath (chevron);
    }
};

// Le groupe complet de navigation de chaine : un cadre, avec la fleche
// gauche et la fleche droite directement accolees au nom, au milieu.
class ChannelNavigatorBox : public juce::Component
{
public:
    ChannelNavigatorBox()
    {
        addAndMakeVisible (prevButton);
        addAndMakeVisible (nextButton);
        addAndMakeVisible (nameButton);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (juce::Colour (0xff1a1e23));
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (juce::Colour (0xff3d434d));
        g.drawRoundedRectangle (bounds, 8.0f, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (4, 4);
        prevButton.setBounds (area.removeFromLeft (26));
        nextButton.setBounds (area.removeFromRight (26));
        nameButton.setBounds (area);
    }

    NavArrowButton prevButton { "<" };
    NavArrowButton nextButton { ">" };
    ChannelPickerButton nameButton;
};

// Bouton discret (LINK, Latency) : texte simple, surbrillance legere au
// survol, couleur d'accent uniquement quand actif.
class DiscreetButton : public juce::TextButton
{
public:
    explicit DiscreetButton (const juce::String& name) : juce::TextButton (name) {}

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat();

        if (getToggleState())
        {
            g.setColour (juce::Colour (0xffe8a33d).withAlpha (0.22f));
            g.fillRoundedRectangle (bounds, 4.0f);
        }
        else if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
        {
            g.setColour (juce::Colours::white.withAlpha (0.06f));
            g.fillRoundedRectangle (bounds, 4.0f);
        }

        g.setColour (getToggleState() ? juce::Colour (0xffe8a33d) : juce::Colour (0xff9aa1ae));
        g.setFont (juce::Font (12.0f, juce::Font::bold));
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred);
    }
};

// Un seul onglet dans la pilule partagee : pas de fond propre ni de coins
// arrondis (la pilule parente dessine deja le cadre), juste le texte et
// un petit trait d'accent sous l'onglet actif.
class TabButton : public juce::TextButton
{
public:
    explicit TabButton (const juce::String& name) : juce::TextButton (name) {}

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool) override
    {
        if (shouldDrawButtonAsHighlighted && ! getToggleState())
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.fillRect (getLocalBounds());
        }

        g.setColour (getToggleState() ? juce::Colours::white : juce::Colour (0xff8a92a0));
        g.setFont (juce::Font (13.0f, juce::Font::bold));
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred);

        if (getToggleState())
        {
            auto bounds = getLocalBounds().toFloat();
            g.setColour (juce::Colour (0xff4ea1ff));
            g.fillRect (juce::Rectangle<float> (bounds.getX() + 10.0f, bounds.getBottom() - 4.0f,
                                                juce::jmax (0.0f, bounds.getWidth() - 20.0f), 3.0f));
        }
    }
};

// Pilule partagee pour les 4 onglets (OVERVIEW / RACK / SHOW / PATCH) :
// un seul cadre arrondi, des traits verticaux fins pour les separer.
class TabsPillGroup : public juce::Component
{
public:
    TabsPillGroup()
    {
        for (auto* btn : { &overviewButton, &rackButton, &showButton, &patchButton })
        {
            btn->setClickingTogglesState (true);
            btn->setRadioGroupId (2001);
            addAndMakeVisible (*btn);
        }

        overviewButton.setToggleState (true, juce::dontSendNotification);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff22262b));
        g.fillRoundedRectangle (bounds, 10.0f);
        g.setColour (juce::Colour (0xff3d434d));
        g.drawRoundedRectangle (bounds, 10.0f, 1.0f);

        auto quarter = bounds.getWidth() / 4.0f;
        for (int i = 1; i < 4; ++i)
        {
            auto x = bounds.getX() + quarter * (float) i;
            g.drawLine (x, bounds.getY() + 10.0f, x, bounds.getBottom() - 10.0f, 1.0f);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (2);
        auto quarter = area.getWidth() / 4;
        overviewButton.setBounds (area.removeFromLeft (quarter));
        rackButton.setBounds (area.removeFromLeft (quarter));
        showButton.setBounds (area.removeFromLeft (quarter));
        patchButton.setBounds (area);
    }

    TabButton overviewButton { "OVERVIEW" };
    TabButton rackButton { "RACK" };
    TabButton showButton { "SHOW" };
    TabButton patchButton { "PATCH" };
};

// La barre superieure de l'application : logo, navigateur de chaine
// (cadre avec < Nom v >), Link/Latency discrets empiles, 4 onglets
// partages, BPM/CPU empiles et TAP/horloge empiles, verrouillage.
class TopBarComponent : public juce::Component,
                        private juce::Timer
{
public:
    TopBarComponent()
        : cpuColumn ("CPU")
    {
        logoLabel.setText ("SHFX", juce::dontSendNotification);
        logoLabel.setFont (juce::Font (24.0f, juce::Font::bold));
        logoLabel.setColour (juce::Label::textColourId, juce::Colour (0xff4ea1ff));
        logoLabel.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (logoLabel);

        channelNavigator.prevButton.onClick = [this] { if (onPreviousChannel != nullptr) onPreviousChannel(); };
        channelNavigator.nextButton.onClick = [this] { if (onNextChannel != nullptr) onNextChannel(); };
        channelNavigator.nameButton.setButtonText ("");
        channelNavigator.nameButton.onClick = [this] { if (onChannelPickerRequested != nullptr) onChannelPickerRequested(); };
        addAndMakeVisible (channelNavigator);

        linkButton.setButtonText ("LINK");
        linkButton.setClickingTogglesState (true);
        linkButton.onClick = [this] { if (onLinkToggled != nullptr) onLinkToggled (linkButton.getToggleState()); };
        addAndMakeVisible (linkButton);

        latencyButton.setButtonText ("Latency");
        latencyButton.onClick = [this] { showLatencyMenu(); };
        addAndMakeVisible (latencyButton);

        tabsPill.overviewButton.onClick = [this] { if (onOverviewTabClicked != nullptr) onOverviewTabClicked(); };
        tabsPill.rackButton.onClick = [this] { if (onRackTabClicked != nullptr) onRackTabClicked(); };
        tabsPill.showButton.onClick = [this] { if (onShowTabClicked != nullptr) onShowTabClicked(); };
        tabsPill.patchButton.onClick = [this] { if (onPatchTabClicked != nullptr) onPatchTabClicked(); };
        addAndMakeVisible (tabsPill);

        // --- Pilule BPM/CPU/TAP/horloge, en deux colonnes empilees ---
        addAndMakeVisible (infoPill);
        bpmEditor.setText ("120", juce::dontSendNotification);
        bpmEditor.setJustification (juce::Justification::centred);
        bpmEditor.setInputRestrictions (3, "0123456789");
        bpmEditor.setFont (14.0f);
        infoPill.addAndMakeVisible (bpmEditor);

        tapButton.setButtonText ("TAP");
        infoPill.addAndMakeVisible (tapButton);
        tapButton.onClick = [this] { registerTap(); };

        infoPill.addAndMakeVisible (cpuColumn);

        clockLabel.setJustificationType (juce::Justification::centred);
        clockLabel.setFont (14.0f);
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

    void setSelectedChannelName (const juce::String& name)
    {
        channelNavigator.nameButton.setButtonText (name);
    }

    double getBpm() const
    {
        return bpmEditor.getText().getDoubleValue();
    }

    // tabIndex : 0 = OVERVIEW, 1 = RACK, 2 = SHOW, 3 = PATCH
    void setActiveTab (int tabIndex)
    {
        tabsPill.overviewButton.setToggleState (tabIndex == 0, juce::dontSendNotification);
        tabsPill.rackButton.setToggleState (tabIndex == 1, juce::dontSendNotification);
        tabsPill.showButton.setToggleState (tabIndex == 2, juce::dontSendNotification);
        tabsPill.patchButton.setToggleState (tabIndex == 3, juce::dontSendNotification);
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
        auto area = getLocalBounds().reduced (10, 8);

        logoLabel.setBounds (area.removeFromLeft (60));
        area.removeFromLeft (12);

        auto navSlot = area.removeFromLeft (190);
        channelNavigator.setBounds (navSlot.withSizeKeepingCentre (190, 84));
        area.removeFromLeft (16);

        auto linkLatencySlot = area.removeFromLeft (60);
        auto linkLatencyBox = linkLatencySlot.withSizeKeepingCentre (60, 56);
        linkButton.setBounds (linkLatencyBox.removeFromTop (26));
        linkLatencyBox.removeFromTop (4);
        latencyButton.setBounds (linkLatencyBox);
        area.removeFromLeft (20);

        auto lockSlot = area.removeFromRight (56);
        lockButton.setBounds (lockSlot.withSizeKeepingCentre (44, 44));
        area.removeFromRight (14);

        auto infoSlot = area.removeFromRight (190);
        infoPill.setBounds (infoSlot.withSizeKeepingCentre (190, 84));
        area.removeFromRight (20);

        auto tabsArea = area.withSizeKeepingCentre (juce::jmin (320, area.getWidth()), 72);
        tabsPill.setBounds (tabsArea);

        // --- Interieur de la pilule : BPM/CPU a gauche (empiles), TAP/horloge a droite (empiles) ---
        auto pillContent = infoPill.getLocalBounds().reduced (10, 8);
        auto leftColumn = pillContent.removeFromLeft (pillContent.getWidth() / 2 - 4);
        pillContent.removeFromLeft (8);
        auto rightColumn = pillContent;

        bpmEditor.setBounds (leftColumn.removeFromTop (leftColumn.getHeight() / 2 - 2));
        leftColumn.removeFromTop (4);
        cpuColumn.setBounds (leftColumn);

        tapButton.setBounds (rightColumn.removeFromTop (rightColumn.getHeight() / 2 - 2));
        rightColumn.removeFromTop (4);
        clockLabel.setBounds (rightColumn);
    }

    std::function<void()> onOverviewTabClicked;
    std::function<void()> onRackTabClicked;
    std::function<void()> onShowTabClicked;
    std::function<void()> onPatchTabClicked;
    std::function<void (bool linkEnabled)> onLinkToggled;
    std::function<void (bool locked)> onLockToggled;
    std::function<void (int newBufferSize)> onLatencyChanged;
    std::function<void()> onPreviousChannel;
    std::function<void()> onNextChannel;
    std::function<void()> onChannelPickerRequested;

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
    ChannelNavigatorBox channelNavigator;
    DiscreetButton linkButton { "LINK" };
    DiscreetButton latencyButton { "Latency" };
    TabsPillGroup tabsPill;
    LockButton lockButton;

    PillGroup infoPill;
    juce::TextEditor bpmEditor;
    juce::TextButton tapButton;
    CompactMeterColumn cpuColumn;
    juce::Label clockLabel;

    double lastTapTimeMs = 0.0;
};
