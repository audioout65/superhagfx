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
        preFxBuffer.setSize (1, juce::jmax (samplesPerBlock, 32), false, false, true);

        for (auto& slot : slots)
            slot.prepare (sampleRate, samplesPerBlock);
    }

    void readInput (const juce::AudioBuffer<float>& deviceBuffer, int startSample, int numSamples)
    {
        if (workBuffer.getNumSamples() < numSamples)
            workBuffer.setSize (1, numSamples, false, false, true);

        if (! inputEnabled || inputChannelIndex < 0 || inputChannelIndex >= deviceBuffer.getNumChannels())
        {
            workBuffer.clear (0, 0, numSamples);
            inputLevel = 0.0f;
        }
        else
        {
            workBuffer.copyFrom (0, 0, deviceBuffer, inputChannelIndex, startSample, numSamples);

            auto inputGainLinear = juce::Decibels::decibelsToGain (inputGainDb, -60.0f);
            auto* samples = workBuffer.getWritePointer (0);

            for (int i = 0; i < numSamples; ++i)
                samples[i] *= inputGainLinear;

            inputLevel = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                inputLevel = juce::jmax (inputLevel, std::abs (samples[i]));
        }

        // Instantane "pre-effets" (juste apres le gain d'entree, avant le
        // gain de sortie et les propres plugins de la chaine) : permet a un
        // bus SEND de recuperer ce point precis, independamment de ce que
        // fait ensuite processAndWriteOutput() sur workBuffer.
        if (preFxBuffer.getNumSamples() < numSamples)
            preFxBuffer.setSize (1, numSamples, false, false, true);

        preFxBuffer.copyFrom (0, 0, workBuffer, 0, 0, numSamples);
    }

    void processAndWriteOutput (juce::AudioBuffer<float>& deviceBuffer, int startSample, int numSamples)
    {
        auto* samples = workBuffer.getWritePointer (0);

        if (outputMuted)
        {
            level = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                level = juce::jmax (level, std::abs (samples[i]));
        }
        else
        {
            auto outputGainLinear = juce::Decibels::decibelsToGain (outputGainDb, -60.0f);

            for (int i = 0; i < numSamples; ++i)
                samples[i] *= outputGainLinear;

            for (auto& slot : slots)
                slot.process (samples, numSamples);

            level = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                level = juce::jmax (level, std::abs (samples[i]));
        }

        if (outputChannelIndex >= 0 && outputChannelIndex < deviceBuffer.getNumChannels())
            deviceBuffer.copyFrom (outputChannelIndex, startSample, workBuffer, 0, 0, numSamples);
    }

    // --- Sends vers un bus SEND (Phase 1 : plomberie moteur) ----------------
    // Additionnent (jamais ne remplacent) le signal de cette chaine dans le
    // buffer mono d'un bus, en pre-effets ou en post-effets. Une chaine en
    // MUTE n'envoie rien, ni en pre ni en post.
    void addPreFxSendTo (juce::AudioBuffer<float>& dest, int destChannel, int numSamples) const
    {
        if (outputMuted)
            return;

        dest.addFrom (destChannel, 0, preFxBuffer, 0, 0, numSamples);
    }

    void addPostFxSendTo (juce::AudioBuffer<float>& dest, int destChannel, int numSamples) const
    {
        if (outputMuted)
            return;

        dest.addFrom (destChannel, 0, workBuffer, 0, 0, numSamples);
    }

    void setOutputGainDb (float newGainDb) { outputGainDb = newGainDb; }
    float getOutputGainDb() const          { return outputGainDb; }

    void setInputGainDb (float newGainDb)  { inputGainDb = newGainDb; }
    float getInputGainDb() const           { return inputGainDb; }

    // IN (panneau d'entree) : active ou coupe la reception du signal d'entree.
    void setInputEnabled (bool shouldBeEnabled) { inputEnabled = shouldBeEnabled; }
    bool isInputEnabled() const                  { return inputEnabled; }

    // MUTE (panneau de sortie) : coupe le traitement et la sortie du canal.
    void setOutputMuted (bool shouldBeMuted) { outputMuted = shouldBeMuted; }
    bool isOutputMuted() const                { return outputMuted; }

    float getLevel() const             { return level; }
    float getInputLevel() const        { return inputLevel; }

    void setInputChannelIndex (int index)  { inputChannelIndex = index; }
    void setOutputChannelIndex (int index) { outputChannelIndex = index; }
    int getInputChannelIndex() const       { return inputChannelIndex; }
    int getOutputChannelIndex() const      { return outputChannelIndex; }

    void setCustomName (const juce::String& name) { customName = name; }
    juce::String getCustomName() const             { return customName; }

    PluginSlot& getSlot (int index) { return slots[(size_t) index]; }
    static constexpr int getNumSlots() { return numSlotsPerChannel; }

    void saveToXml (juce::XmlElement& xml) const
    {
        xml.setAttribute ("input", inputChannelIndex);
        xml.setAttribute ("output", outputChannelIndex);
        xml.setAttribute ("outputGainDb", outputGainDb);
        xml.setAttribute ("inputGainDb", inputGainDb);
        xml.setAttribute ("inputEnabled", inputEnabled);
        xml.setAttribute ("outputMuted", outputMuted);
        xml.setAttribute ("customName", customName);

        for (int i = 0; i < numSlotsPerChannel; ++i)
        {
            auto* slotXml = xml.createNewChildElement ("Slot");
            slotXml->setAttribute ("index", i);
            slots[(size_t) i].saveToXml (*slotXml);
        }
    }

    void loadFromXmlAsync (const juce::XmlElement& xml,
                           juce::AudioPluginFormatManager& formatManager,
                           std::function<void()> onAllSlotsDone)
    {
        inputChannelIndex = xml.getIntAttribute ("input", inputChannelIndex);
        outputChannelIndex = xml.getIntAttribute ("output", outputChannelIndex);
        outputGainDb = (float) xml.getDoubleAttribute ("outputGainDb", outputGainDb);
        inputGainDb = (float) xml.getDoubleAttribute ("inputGainDb", inputGainDb);
        inputEnabled = xml.getBoolAttribute ("inputEnabled", inputEnabled);
        outputMuted = xml.getBoolAttribute ("outputMuted", outputMuted);
        customName = xml.getStringAttribute ("customName", customName);

        auto remaining = std::make_shared<int>(0);

        for (auto* slotXml : xml.getChildWithTagNameIterator ("Slot"))
            juce::ignoreUnused (slotXml), (*remaining)++;

        if (*remaining == 0)
        {
            if (onAllSlotsDone != nullptr)
                onAllSlotsDone();
            return;
        }

        for (auto* slotXml : xml.getChildWithTagNameIterator ("Slot"))
        {
            auto index = slotXml->getIntAttribute ("index", -1);

            if (index < 0 || index >= numSlotsPerChannel)
            {
                (*remaining)--;
                continue;
            }

            slots[(size_t) index].loadFromXmlAsync (*slotXml, formatManager, [remaining, onAllSlotsDone]
            {
                (*remaining)--;
                if (*remaining == 0 && onAllSlotsDone != nullptr)
                    onAllSlotsDone();
            });
        }
    }

    // Rappel a chaud : meme principe que loadFromXmlAsync, mais chaque
    // emplacement utilise loadFromXmlFast (voir PluginSlot.h), qui evite de
    // detruire/recreer un plugin deja en place s'il s'agit du meme.
    void loadFromXmlFast (const juce::XmlElement& xml,
                         juce::AudioPluginFormatManager& formatManager,
                         std::function<void()> onAllSlotsDone)
    {
        inputChannelIndex = xml.getIntAttribute ("input", inputChannelIndex);
        outputChannelIndex = xml.getIntAttribute ("output", outputChannelIndex);
        outputGainDb = (float) xml.getDoubleAttribute ("outputGainDb", outputGainDb);
        inputGainDb = (float) xml.getDoubleAttribute ("inputGainDb", inputGainDb);
        inputEnabled = xml.getBoolAttribute ("inputEnabled", inputEnabled);
        outputMuted = xml.getBoolAttribute ("outputMuted", outputMuted);
        customName = xml.getStringAttribute ("customName", customName);

        auto remaining = std::make_shared<int>(0);

        for (auto* slotXml : xml.getChildWithTagNameIterator ("Slot"))
            juce::ignoreUnused (slotXml), (*remaining)++;

        if (*remaining == 0)
        {
            if (onAllSlotsDone != nullptr)
                onAllSlotsDone();
            return;
        }

        for (auto* slotXml : xml.getChildWithTagNameIterator ("Slot"))
        {
            auto index = slotXml->getIntAttribute ("index", -1);

            if (index < 0 || index >= numSlotsPerChannel)
            {
                (*remaining)--;
                continue;
            }

            slots[(size_t) index].loadFromXmlFast (*slotXml, formatManager, [remaining, onAllSlotsDone]
            {
                (*remaining)--;
                if (*remaining == 0 && onAllSlotsDone != nullptr)
                    onAllSlotsDone();
            });
        }
    }

private:
    double currentSampleRate = 44100.0;
    float outputGainDb = 0.0f;
    float inputGainDb = 0.0f;
    bool inputEnabled = true;
    bool outputMuted = false;
    float level = 0.0f;
    float inputLevel = 0.0f;

    int inputChannelIndex = 0;
    int outputChannelIndex = 0;

    juce::String customName;

    juce::AudioBuffer<float> workBuffer;
    juce::AudioBuffer<float> preFxBuffer;
    std::array<PluginSlot, numSlotsPerChannel> slots;
};
