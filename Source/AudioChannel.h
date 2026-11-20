#pragma once
#include <JuceHeader.h>
#include "PluginSlot.h"

static constexpr int numSlotsPerChannel = 6;

class AudioChannel
{
public:
    AudioChannel() = default;

    void prepare (double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;

        for (auto& slot : slots)
            slot.prepare (sampleRate, samplesPerBlock);
    }

    void process (float* samples, int numSamples)
    {
        if (bypassed)
        {
            level = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                level = juce::jmax (level, std::abs (samples[i]));
            return;
        }

        for (int i = 0; i < numSamples; ++i)
            samples[i] *= gain;

        for (auto& slot : slots)
            slot.process (samples, numSamples);

        level = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            level = juce::jmax (level, std::abs (samples[i]));
    }

    void setGain (float newGain)       { gain = newGain; }
    void setBypassed (bool shouldBeBypassed) { bypassed = shouldBeBypassed; }
    bool isBypassed() const            { return bypassed; }
    float getLevel() const             { return level; }

    PluginSlot& getSlot (int index) { return slots[(size_t) index]; }
    static constexpr int getNumSlots() { return numSlotsPerChannel; }

private:
    double currentSampleRate = 44100.0;
    float gain = 1.0f;
    bool bypassed = false;
    float level = 0.0f;

    std::array<PluginSlot, numSlotsPerChannel> slots;
};
