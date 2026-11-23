#pragma once
#include <JuceHeader.h>

// Petite fenetre (CallOutBox) avec une case a cocher par chaine (1 a 32) :
// cocher/decocher une case applique immediatement le changement via onApply.
class CustomLayerPicker : public juce::Component
{
public:
    explicit CustomLayerPicker (const std::array<bool, 32>& initialSelection)
        : selection (initialSelection)
    {
        for (int i = 0; i < 32; ++i)
        {
            auto toggle = std::make_unique<juce::ToggleButton> (juce::String (i + 1));
            toggle->setToggleState (selection[(size_t) i], juce::dontSendNotification);

            auto* ptr = toggle.get();
            toggle->onClick = [this, i, ptr]
            {
                selection[(size_t) i] = ptr->getToggleState();
                if (onApply != nullptr)
                    onApply (selection);
            };

            addAndMakeVisible (*toggle);
            toggles.push_back (std::move (toggle));
        }

        setSize (280, 180);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8);

        constexpr int columns = 8;
        constexpr int rows = 4;
        auto cellWidth = area.getWidth() / columns;
        auto cellHeight = area.getHeight() / rows;

        for (int i = 0; i < (int) toggles.size(); ++i)
        {
            auto col = i % columns;
            auto row = i / columns;
            toggles[(size_t) i]->setBounds (area.getX() + col * cellWidth, area.getY() + row * cellHeight,
                                            cellWidth, cellHeight);
        }
    }

    std::function<void (const std::array<bool, 32>&)> onApply;

private:
    std::array<bool, 32> selection;
    std::vector<std::unique_ptr<juce::ToggleButton>> toggles;
};

// Barre laterale gauche de la vue OVERVIEW, inspiree de SuperRack : un bloc
// MODE en haut (un seul mode pour nous : RACKS), puis les banques de
// canaux empilees verticalement en dessous (LAYERS) -- 1-16, 17-32, et un
// calque CUSTOM pour choisir librement les chaines affichees.
class LeftSidebarComponent : public juce::Component
{
public:
    LeftSidebarComponent()
    {
        modeSectionLabel.setText ("MODE", juce::dontSendNotification);
        modeSectionLabel.setFont (10.0f);
        modeSectionLabel.setJustificationType (juce::Justification::centred);
        modeSectionLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a92a0));
        addAndMakeVisible (modeSectionLabel);

        racksModeButton.setButtonText ("RACKS");
        racksModeButton.setClickingTogglesState (true);
        racksModeButton.setToggleState (true, juce::dontSendNotification);
        racksModeButton.setRadioGroupId (3001);
        addAndMakeVisible (racksModeButton);

        layersSectionLabel.setText ("LAYERS", juce::dontSendNotification);
        layersSectionLabel.setFont (10.0f);
        layersSectionLabel.setJustificationType (juce::Justification::centred);
        layersSectionLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a92a0));
        addAndMakeVisible (layersSectionLabel);

        customSelection.fill (true);

        bankAButton.setButtonText ("1-16");
        bankBButton.setButtonText ("17-32");
        customButton.setButtonText ("CUSTOM");

        bankAButton.setClickingTogglesState (true);
        bankBButton.setClickingTogglesState (true);
        customButton.setClickingTogglesState (true);

        bankAButton.setRadioGroupId (3002);
        bankBButton.setRadioGroupId (3002);
        customButton.setRadioGroupId (3002);

        bankAButton.setToggleState (true, juce::dontSendNotification);

        bankAButton.onClick = [this] { if (onBankSelected != nullptr) onBankSelected (0); };
        bankBButton.onClick = [this] { if (onBankSelected != nullptr) onBankSelected (1); };
        customButton.onClick = [this]
        {
            if (onCustomLayerSelected != nullptr)
                onCustomLayerSelected (customSelection);

            showCustomPicker();
        };

        addAndMakeVisible (bankAButton);
        addAndMakeVisible (bankBButton);
        addAndMakeVisible (customButton);
    }

    void setSelectedBank (int bankIndex)
    {
        bankAButton.setToggleState (bankIndex == 0, juce::dontSendNotification);
        bankBButton.setToggleState (bankIndex == 1, juce::dontSendNotification);
        customButton.setToggleState (false, juce::dontSendNotification);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff14171a));
        g.fillRect (getLocalBounds());

        g.setColour (juce::Colour (0xff2c313a));
        g.drawLine ((float) getWidth() - 1, 0, (float) getWidth() - 1, (float) getHeight(), 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10);

        modeSectionLabel.setBounds (area.removeFromTop (16));
        area.removeFromTop (6);
        racksModeButton.setBounds (area.removeFromTop (34));

        area.removeFromTop (24);

        layersSectionLabel.setBounds (area.removeFromTop (16));
        area.removeFromTop (6);
        bankAButton.setBounds (area.removeFromTop (34));
        area.removeFromTop (6);
        bankBButton.setBounds (area.removeFromTop (34));
        area.removeFromTop (6);
        customButton.setBounds (area.removeFromTop (34));
    }

    std::function<void (int bankIndex)> onBankSelected;
    std::function<void (const std::array<bool, 32>& selection)> onCustomLayerSelected;

private:
    void showCustomPicker()
    {
        auto picker = std::make_unique<CustomLayerPicker> (customSelection);

        picker->onApply = [this] (const std::array<bool, 32>& selection)
        {
            customSelection = selection;

            if (onCustomLayerSelected != nullptr)
                onCustomLayerSelected (customSelection);
        };

        juce::CallOutBox::launchAsynchronously (std::move (picker), customButton.getScreenBounds(), nullptr);
    }

    juce::Label modeSectionLabel;
    juce::TextButton racksModeButton;

    juce::Label layersSectionLabel;
    juce::TextButton bankAButton, bankBButton, customButton;
    std::array<bool, 32> customSelection;
};
