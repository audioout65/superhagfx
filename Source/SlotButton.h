#pragma once
#include <JuceHeader.h>

// Un TextButton classique, mais qui sait aussi reagir au clic droit separement.
class SlotButton : public juce::TextButton
{
public:
    SlotButton (const juce::String& buttonName = {}) : juce::TextButton (buttonName) {}

    std::function<void()> onRightClick;

private:
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
};
