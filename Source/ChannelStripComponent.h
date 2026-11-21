#pragma once
#include <JuceHeader.h>
#include "AudioChannel.h"
#include "SlotButton.h"

class ChannelNameButton : public juce::Label,
                          private juce::Timer
{
public:
    std::function<void()> onSingleClick;

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (isBeingEdited())
        {
            juce::Label::mouseDown (e);
            return;
        }

        pendingClick = true;
        startTimer (250);
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        pendingClick = false;
        stopTimer();
        juce::Label::mouseDoubleClick (e);
    }

private:
    void timerCallback() override
    {
        stopTimer();

        if (pendingClick)
        {
            pendingClick = false;

            if (onSingleClick != nullptr)
                onSingleClick();
        }
    }

    bool pendingClick = false;
};

// L'interface complete d'un seul canal : section INPUT en haut, pile de
// slots au milieu, section OUTPUT en bas. Les deux vumetres se partagent
// a parts rigoureusement egales l'espace vertical restant.
class ChannelStripComponent : public juce::Component
{
public:
    ChannelStripComponent (int channelIndexIn, AudioChannel& channelIn)
        : channelIndex (channelIndexIn), channel (channelIn)
    {
        channelLabel.setText ("Ch " + juce::String (channelIndex + 1), juce::dontSendNotification);
        channelLabel.setJustificationType (juce::Justification::centred);
        channelLabel.setEditable (false, true, false);
        channelLabel.setColour (juce::Label::backgroundColourId, juce::Colour (0xff2c313a));
        channelLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        channelLabel.onSingleClick = [this]
        {
            if (onChannelNameClicked != nullptr)
                onChannelNameClicked (channelIndex);
        };
        channelLabel.onTextChange = [this]
        {
            channel.setCustomName (channelLabel.getText());
        };
        addAndMakeVisible (channelLabel);

        inputSectionLabel.setText ("INPUT", juce::dontSendNotification);
        inputSectionLabel.setFont (10.0f);
        inputSectionLabel.setJustificationType (juce::Justification::centred);
        inputSectionLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a92a0));
        addAndMakeVisible (inputSectionLabel);

        for (int i = 1; i <= 32; ++i)
            inputCombo.addItem ("In " + juce::String (i), i);

        inputCombo.setSelectedId (channel.getInputChannelIndex() + 1, juce::dontSendNotification);
        inputCombo.onChange = [this]
        {
            channel.setInputChannelIndex (inputCombo.getSelectedId() - 1);
        };
        addAndMakeVisible (inputCombo);

        inputGainKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        inputGainKnob.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                           juce::MathConstants<float>::pi * 2.76f, true);
        inputGainKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 16);
        inputGainKnob.setRange (-60.0, 12.0, 0.1);
        inputGainKnob.setDoubleClickReturnValue (true, 0.0);
        inputGainKnob.onValueChange = [this]
        {
            channel.setInputGainDb ((float) inputGainKnob.getValue());
        };
        addAndMakeVisible (inputGainKnob);

        inButton.setButtonText ("IN");
        inButton.setClickingTogglesState (true);
        inButton.setToggleState (true, juce::dontSendNotification);
        inButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff3fae54));
        inButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        inButton.onClick = [this]
        {
            channel.setInputEnabled (inButton.getToggleState());
        };
        addAndMakeVisible (inButton);

        for (auto db : { 0, -10, -20, -30, -40, -50, -60 })
        {
            auto lbl = std::make_unique<juce::Label>("", juce::String (db));
            lbl->setFont (8.0f);
            lbl->setJustificationType (juce::Justification::centredRight);
            lbl->setColour (juce::Label::textColourId, juce::Colour (0xff6a7280));
            addAndMakeVisible (*lbl);
            inputMeterScaleLabels.push_back (std::move (lbl));
        }

        for (int slot = 0; slot < numSlotsPerChannel; ++slot)
        {
            auto slotBtn = std::make_unique<SlotButton>("+");
            auto* ptr = slotBtn.get();

            slotBtn->onClick = [this, slot]
            {
                if (onSlotClicked != nullptr)
                    onSlotClicked (channelIndex, slot);
            };
            slotBtn->onRightClick = [this, slot]
            {
                if (onSlotRightClicked != nullptr)
                    onSlotRightClicked (channelIndex, slot);
            };

            addAndMakeVisible (*ptr);
            slotButtons.push_back (std::move (slotBtn));
        }

        outputSectionLabel.setText ("OUTPUT", juce::dontSendNotification);
        outputSectionLabel.setFont (10.0f);
        outputSectionLabel.setJustificationType (juce::Justification::centred);
        outputSectionLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a92a0));
        addAndMakeVisible (outputSectionLabel);

        for (int i = 1; i <= 16; ++i)
            outputCombo.addItem ("Out " + juce::String (i), i);

        outputCombo.setSelectedId (channel.getOutputChannelIndex() + 1, juce::dontSendNotification);
        outputCombo.onChange = [this]
        {
            channel.setOutputChannelIndex (outputCombo.getSelectedId() - 1);
        };
        addAndMakeVisible (outputCombo);

        outputGainKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        outputGainKnob.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                            juce::MathConstants<float>::pi * 2.76f, true);
        outputGainKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 16);
        outputGainKnob.setRange (-60.0, 12.0, 0.1);
        outputGainKnob.setDoubleClickReturnValue (true, 0.0);
        outputGainKnob.onValueChange = [this]
        {
            channel.setOutputGainDb ((float) outputGainKnob.getValue());
        };
        addAndMakeVisible (outputGainKnob);

        muteButton.setButtonText ("MUTE");
        muteButton.setClickingTogglesState (true);
        muteButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::red);
        muteButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        muteButton.onClick = [this]
        {
            channel.setOutputMuted (muteButton.getToggleState());
        };
        addAndMakeVisible (muteButton);

        for (auto db : { 0, -10, -20, -30, -40, -50, -60 })
        {
            auto lbl = std::make_unique<juce::Label>("", juce::String (db));
            lbl->setFont (8.0f);
            lbl->setJustificationType (juce::Justification::centredRight);
            lbl->setColour (juce::Label::textColourId, juce::Colour (0xff6a7280));
            addAndMakeVisible (*lbl);
            outputMeterScaleLabels.push_back (std::move (lbl));
        }
    }

    void refreshSlotButtonText (int slotIndex, const juce::String& text)
    {
        if (slotIndex >= 0 && slotIndex < (int) slotButtons.size())
            slotButtons[(size_t) slotIndex]->setButtonText (text);
    }

    void refreshFromChannel()
    {
        inputCombo.setSelectedId (channel.getInputChannelIndex() + 1, juce::dontSendNotification);
        outputCombo.setSelectedId (channel.getOutputChannelIndex() + 1, juce::dontSendNotification);
        inputGainKnob.setValue (channel.getInputGainDb(), juce::dontSendNotification);
        outputGainKnob.setValue (channel.getOutputGainDb(), juce::dontSendNotification);
        inButton.setToggleState (channel.isInputEnabled(), juce::dontSendNotification);
        muteButton.setToggleState (channel.isOutputMuted(), juce::dontSendNotification);

        auto displayName = channel.getCustomName();
        if (displayName.isEmpty())
            displayName = "Ch " + juce::String (channelIndex + 1);
        channelLabel.setText (displayName, juce::dontSendNotification);

        for (int slot = 0; slot < numSlotsPerChannel; ++slot)
            refreshSlotButtonText (slot, channel.getSlot (slot).getDisplayName());
    }

    void updateMeter()
    {
        auto outTarget = channel.getLevel();
        outputDisplayedLevel = (outTarget > outputDisplayedLevel) ? outTarget : outputDisplayedLevel * 0.85f;

        auto inTarget = channel.getInputLevel();
        inputDisplayedLevel = (inTarget > inputDisplayedLevel) ? inTarget : inputDisplayedLevel * 0.85f;

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colour (0xff1e2227));
        g.fillRoundedRectangle (bounds, 6.0f);
        g.setColour (juce::Colour (0xff3d434d));
        g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

        drawMeter (g, inputMeterBounds, inputDisplayedLevel);
        drawMeter (g, outputMeterBounds, outputDisplayedLevel);
    }

    void resized() override
    {
        auto fullArea = getLocalBounds().reduced (10);

        // --- Reserve d'abord toutes les zones de taille FIXE, dans l'ordre
        // d'affichage, en travaillant sur une copie, pour calculer ensuite
        // l'espace total disponible pour les deux vumetres. ---
        auto probe = fullArea;
        probe.removeFromTop (22 + 6);                 // nom du canal
        probe.removeFromTop (14 + 4 + 20 + 8);         // titre+combo INPUT
        auto inputFixedBottom = 66 + 10 + 20 + 10;     // knob+marge+bouton IN+marge
        probe.removeFromTop (inputFixedBottom);

        auto slotsHeight = (int) slotButtons.size() * 34 - 4;
        probe.removeFromTop (slotsHeight + 10);        // slots + marge

        probe.removeFromTop (14 + 4 + 20 + 8);         // titre+combo OUTPUT
        auto outputFixedBottom = 80 + 16 + 30;         // knob+marge+MUTE
        probe.removeFromBottom (outputFixedBottom);

        // Ce qui reste dans "probe" est l'espace total pour les deux
        // vumetres reunis : on le partage strictement en deux.
        auto totalMeterHeight = juce::jmax (0, probe.getHeight());
        auto eachMeterHeight = totalMeterHeight / 2;

        // --- Applique maintenant les positions reelles, dans le meme ordre. ---
        auto area = fullArea;

        channelLabel.setBounds (area.removeFromTop (22));
        area.removeFromTop (6);

        inputSectionLabel.setBounds (area.removeFromTop (14));
        area.removeFromTop (4);
        inputCombo.setBounds (area.removeFromTop (20));
        area.removeFromTop (8);

        auto inputMeterRow = area.removeFromTop (eachMeterHeight);
        auto inputScaleColumn = inputMeterRow.removeFromLeft (20);
        inputMeterRow.removeFromLeft (4);
        inputMeterBounds = inputMeterRow;

        for (size_t i = 0; i < inputMeterScaleLabels.size(); ++i)
        {
            auto y = inputMeterBounds.getY() + (int) ((float) inputMeterBounds.getHeight() * (float) i / (float) (inputMeterScaleLabels.size() - 1)) - 6;
            inputMeterScaleLabels[i]->setBounds (inputScaleColumn.getX(), y, inputScaleColumn.getWidth(), 12);
        }

        inputGainKnob.setBounds (area.removeFromTop (66));
        area.removeFromTop (10);
        inButton.setBounds (area.removeFromTop (20));
        area.removeFromTop (10);

        for (auto& btn : slotButtons)
        {
            btn->setBounds (area.removeFromTop (30));
            area.removeFromTop (4);
        }

        area.removeFromTop (10);

        outputSectionLabel.setBounds (area.removeFromTop (14));
        area.removeFromTop (4);
        outputCombo.setBounds (area.removeFromTop (20));
        area.removeFromTop (8);

        auto outputMeterRow = area.removeFromTop (eachMeterHeight);
        auto outputScaleColumn = outputMeterRow.removeFromLeft (20);
        outputMeterRow.removeFromLeft (4);
        outputMeterBounds = outputMeterRow;

        for (size_t i = 0; i < outputMeterScaleLabels.size(); ++i)
        {
            auto y = outputMeterBounds.getY() + (int) ((float) outputMeterBounds.getHeight() * (float) i / (float) (outputMeterScaleLabels.size() - 1)) - 6;
            outputMeterScaleLabels[i]->setBounds (outputScaleColumn.getX(), y, outputScaleColumn.getWidth(), 12);
        }

        outputGainKnob.setBounds (area.removeFromTop (80));
        area.removeFromTop (16);
        muteButton.setBounds (area.removeFromTop (30));
    }

    std::function<void (int channelIndex)> onChannelNameClicked;
    std::function<void (int channelIndex, int slotIndex)> onSlotClicked;
    std::function<void (int channelIndex, int slotIndex)> onSlotRightClicked;

private:
    void drawMeter (juce::Graphics& g, juce::Rectangle<int> bounds, float displayedLevel)
    {
        auto meterArea = bounds.toFloat();

        g.setColour (juce::Colour (0xff0d0f11));
        g.fillRoundedRectangle (meterArea, 3.0f);
        g.setColour (juce::Colour (0xff3d434d));
        g.drawRoundedRectangle (meterArea, 3.0f, 1.0f);

        auto level = juce::jlimit (0.0f, 1.0f, displayedLevel);
        auto filledHeight = level * meterArea.getHeight();
        auto filledArea = meterArea.withTop (meterArea.getBottom() - filledHeight);

        g.setColour (level > 0.9f ? juce::Colours::red
                                   : (level > 0.7f ? juce::Colours::orange : juce::Colours::green));
        g.fillRoundedRectangle (filledArea.reduced (1.5f), 1.5f);

        for (int i = 0; i < 6; ++i)
        {
            auto y = meterArea.getY() + meterArea.getHeight() * (float) i / 6.0f;
            g.setColour (juce::Colour (0xff2c313a));
            g.drawHorizontalLine ((int) y, meterArea.getX(), meterArea.getRight());
        }
    }

    int channelIndex;
    AudioChannel& channel;
    float inputDisplayedLevel = 0.0f;
    float outputDisplayedLevel = 0.0f;
    juce::Rectangle<int> inputMeterBounds, outputMeterBounds;

    ChannelNameButton channelLabel;

    juce::Label inputSectionLabel;
    juce::ComboBox inputCombo;
    juce::Slider inputGainKnob;
    juce::TextButton inButton;
    std::vector<std::unique_ptr<juce::Label>> inputMeterScaleLabels;

    juce::Label outputSectionLabel;
    juce::ComboBox outputCombo;
    juce::Slider outputGainKnob;
    juce::TextButton muteButton;
    std::vector<std::unique_ptr<juce::Label>> outputMeterScaleLabels;

    std::vector<std::unique_ptr<SlotButton>> slotButtons;
};
