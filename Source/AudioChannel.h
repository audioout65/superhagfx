#pragma once
#include <JuceHeader.h>

class AudioChannel
{
public:
    AudioChannel() = default;

    void prepare (double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        juce::ignoreUnused (samplesPerBlock);
    }

    // Traite un seul canal audio (un pointeur de samples, pour ce canal uniquement)
    void process (float* samples, int numSamples)
    {
        if (bypassed)
            return;

        for (int i = 0; i < numSamples; ++i)
            samples[i] *= gain;

        level = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            level = juce::jmax (level, std::abs (samples[i]));
    }

    void setGain (float newGain)       { gain = newGain; }
    void setBypassed (bool shouldBeBypassed) { bypassed = shouldBeBypassed; }
    bool isBypassed() const            { return bypassed; }
    float getLevel() const             { return level; }

private:
    double currentSampleRate = 44100.0;
    float gain = 1.0f;
    bool bypassed = false;
    float level = 0.0f;
};
