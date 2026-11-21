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
        workBuffer.setSize (1, juce::jmax (samplesPerBlock, 32), false, false, true);

        for (auto& slot : slots)
            slot.prepare (sampleRate, samplesPerBlock);
    }

    // Passe 1 : copie l'entree selectionnee depuis le tampon du peripherique.
    // Si cette entree n'existe pas sur le peripherique actuel, le canal reste silencieux.
    void readInput (const juce::AudioBuffer<float>& deviceBuffer, int startSample, int numSamples)
    {
        if (workBuffer.getNumSamples() < numSamples)
            workBuffer.setSize (1, numSamples, false, false, true);

        if (inputChannelIndex < 0 || inputChannelIndex >= deviceBuffer.getNumChannels())
        {
            workBuffer.clear (0, 0, numSamples);
            return;
        }

        workBuffer.copyFrom (0, 0, deviceBuffer, inputChannelIndex, startSample, numSamples);
    }

    // Traite le signal, puis l'ecrit dans la sortie selectionnee du tampon du peripherique.
    // Si cette sortie n'existe pas sur le peripherique actuel, rien n'est ecrit.
    void processAndWriteOutput (juce::AudioBuffer<float>& deviceBuffer, int startSample, int numSamples)
    {
        auto* samples = workBuffer.getWritePointer (0);

        if (bypassed)
        {
            level = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                level = juce::jmax (level, std::abs (samples[i]));
        }
        else
        {
            for (int i = 0; i < numSamples; ++i)
                samples[i] *= gain;

            for (auto& slot : slots)
                slot.process (samples, numSamples);

            level = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                level = juce::jmax (level, std::abs (samples[i]));
        }

        if (outputChannelIndex >= 0 && outputChannelIndex < deviceBuffer.getNumChannels())
            deviceBuffer.copyFrom (outputChannelIndex, startSample, workBuffer, 0, 0, numSamples);
    }

    void setGain (float newGain)       { gain = newGain; }
    void setBypassed (bool shouldBeBypassed) { bypassed = shouldBeBypassed; }
    bool isBypassed() const            { return bypassed; }
    float getLevel() const             { return level; }

    void setInputChannelIndex (int index)  { inputChannelIndex = index; }
    void setOutputChannelIndex (int index) { outputChannelIndex = index; }
    int getInputChannelIndex() const       { return inputChannelIndex; }
    int getOutputChannelIndex() const      { return outputChannelIndex; }

    PluginSlot& getSlot (int index) { return slots[(size_t) index]; }
    static constexpr int getNumSlots() { return numSlotsPerChannel; }

private:
    double currentSampleRate = 44100.0;
    float gain = 1.0f;
    bool bypassed = false;
    float level = 0.0f;

    int inputChannelIndex = 0;
    int outputChannelIndex = 0;

    juce::AudioBuffer<float> workBuffer;
    std::array<PluginSlot, numSlotsPerChannel> slots;
};
