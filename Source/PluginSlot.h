#pragma once
#include <JuceHeader.h>
#include "Compressor.h"

enum class SlotContentType
{
    empty,
    internalCompressor
    // D'autres effets internes et les VST3 viendront ensuite
};

// Un emplacement dans la chaine d'un canal : vide, ou contient un effet.
class PluginSlot
{
public:
    PluginSlot() = default;

    void prepare (double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        juce::ignoreUnused (samplesPerBlock);
        compressor.prepare (sampleRate);
    }

    void process (float* samples, int numSamples)
    {
        if (bypassed)
            return;

        switch (contentType)
        {
            case SlotContentType::internalCompressor:
                compressor.process (samples, numSamples);
                break;

            case SlotContentType::empty:
            default:
                break;
        }
    }

    void loadInternalCompressor()
    {
        contentType = SlotContentType::internalCompressor;
    }

    void clear()
    {
        contentType = SlotContentType::empty;
    }

    bool isEmpty() const { return contentType == SlotContentType::empty; }

    juce::String getDisplayName() const
    {
        switch (contentType)
        {
            case SlotContentType::internalCompressor: return "Compressor";
            case SlotContentType::empty:
            default:                                   return "+";
        }
    }

    void setBypassed (bool shouldBeBypassed) { bypassed = shouldBeBypassed; }
    bool isBypassed() const                   { return bypassed; }

    Compressor& getCompressor() { return compressor; }

private:
    double currentSampleRate = 44100.0;
    SlotContentType contentType = SlotContentType::empty;
    bool bypassed = false;

    Compressor compressor;
};
