#pragma once
#include <JuceHeader.h>
#include "Compressor.h"
#include "NativeWindowCapture.h"

enum class SlotContentType
{
    empty,
    internalCompressor,
    vst3
};

class PluginSlot
{
public:
    PluginSlot() = default;

    ~PluginSlot()
    {
        const juce::SpinLock::ScopedLockType lock (pluginLock);
        vstInstance.reset();
    }

    void prepare (double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        currentBlockSize = samplesPerBlock;

        compressor.prepare (sampleRate);
        scratchBuffer.setSize (2, juce::jmax (samplesPerBlock, 32), false, false, true);

        const juce::SpinLock::ScopedLockType lock (pluginLock);
        if (vstInstance != nullptr)
            vstInstance->prepareToPlay (sampleRate, samplesPerBlock);
    }

    void process (float* samples, int numSamples)
    {
        if (bypassed)
            return;

        const juce::SpinLock::ScopedTryLockType lock (pluginLock);

        if (! lock.isLocked())
            return;

        switch (contentType)
        {
            case SlotContentType::internalCompressor:
                compressor.process (samples, numSamples);
                break;

            case SlotContentType::vst3:
                processVst3 (samples, numSamples);
                break;

            case SlotContentType::empty:
            default:
                break;
        }
    }

    void loadInternalCompressor()
    {
        const juce::SpinLock::ScopedLockType lock (pluginLock);
        vstInstance.reset();
        contentType = SlotContentType::internalCompressor;
        thumbnailImage = juce::Image();
    }

    void clear()
    {
        const juce::SpinLock::ScopedLockType lock (pluginLock);
        vstInstance.reset();
        contentType = SlotContentType::empty;
        vstDisplayName.clear();
        lastPluginDescription = juce::PluginDescription();
        thumbnailImage = juce::Image();
    }

    void loadVst3Async (const juce::PluginDescription& desc,
                        juce::AudioPluginFormatManager& formatManager,
                        std::function<void (bool success, juce::String error)> onDone)
    {
        formatManager.createPluginInstanceAsync (desc, currentSampleRate, currentBlockSize,
            [this, desc, onDone] (std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
            {
                if (instance == nullptr)
                {
                    if (onDone != nullptr)
                        onDone (false, error);
                    return;
                }

                instance->prepareToPlay (currentSampleRate, currentBlockSize);

                {
                    const juce::SpinLock::ScopedLockType lock (pluginLock);
                    vstInstance = std::move (instance);
                    vstDisplayName = desc.name;
                    lastPluginDescription = desc;
                    contentType = SlotContentType::vst3;
                }

                startThumbnailCapture();

                if (onDone != nullptr)
                    onDone (true, {});
            });
    }

    bool isEmpty() const { return contentType == SlotContentType::empty; }
    bool isVst3() const  { return contentType == SlotContentType::vst3; }

    juce::String getDisplayName() const
    {
        switch (contentType)
        {
            case SlotContentType::internalCompressor: return "Compressor";
            case SlotContentType::vst3:                return vstDisplayName;
            case SlotContentType::empty:
            default:                                    return "+";
        }
    }

    const juce::Image& getThumbnail() const { return thumbnailImage; }

    void setBypassed (bool shouldBeBypassed) { bypassed = shouldBeBypassed; }
    bool isBypassed() const                   { return bypassed; }

    Compressor& getCompressor() { return compressor; }

    juce::AudioPluginInstance* getVstInstance()
    {
        const juce::SpinLock::ScopedLockType lock (pluginLock);
        return vstInstance.get();
    }

    void saveToXml (juce::XmlElement& xml) const
    {
        switch (contentType)
        {
            case SlotContentType::internalCompressor:
            {
                xml.setAttribute ("type", "compressor");
                auto* compXml = xml.createNewChildElement ("Compressor");
                compressor.saveToXml (*compXml);
                break;
            }

            case SlotContentType::vst3:
            {
                xml.setAttribute ("type", "vst3");

                if (auto descXml = lastPluginDescription.createXml())
                    xml.addChildElement (descXml.release());

                if (vstInstance != nullptr)
                {
                    juce::MemoryBlock state;
                    vstInstance->getStateInformation (state);
                    xml.setAttribute ("state", state.toBase64Encoding());
                }
                break;
            }

            case SlotContentType::empty:
            default:
                xml.setAttribute ("type", "empty");
                break;
        }

        xml.setAttribute ("bypassed", bypassed);
    }

    void loadFromXmlAsync (const juce::XmlElement& xml,
                           juce::AudioPluginFormatManager& formatManager,
                           std::function<void()> onDone)
    {
        bypassed = xml.getBoolAttribute ("bypassed", false);

        auto type = xml.getStringAttribute ("type", "empty");

        if (type == "compressor")
        {
            if (auto* compXml = xml.getChildByName ("Compressor"))
                compressor.loadFromXml (*compXml);

            loadInternalCompressor();

            if (onDone != nullptr)
                onDone();
            return;
        }

        if (type == "vst3")
        {
            juce::PluginDescription desc;
            bool descLoaded = false;

            for (auto* child : xml.getChildIterator())
            {
                if (child->hasTagName ("PLUGIN"))
                {
                    descLoaded = desc.loadFromXml (*child);
                    break;
                }
            }

            if (! descLoaded)
            {
                if (onDone != nullptr)
                    onDone();
                return;
            }

            juce::MemoryBlock state;
            auto stateBase64 = xml.getStringAttribute ("state");
            if (stateBase64.isNotEmpty())
                state.fromBase64Encoding (stateBase64);

            loadVst3Async (desc, formatManager, [this, state, onDone] (bool success, juce::String)
            {
                if (success && state.getSize() > 0)
                {
                    {
                        const juce::SpinLock::ScopedLockType lock (pluginLock);
                        if (vstInstance != nullptr)
                            vstInstance->setStateInformation (state.getData(), (int) state.getSize());
                    }

                    startThumbnailCapture();
                }

                if (onDone != nullptr)
                    onDone();
            });
            return;
        }

        clear();
        if (onDone != nullptr)
            onDone();
    }

private:
    void processVst3 (float* samples, int numSamples)
    {
        if (vstInstance == nullptr)
            return;

        auto numInputChannels = vstInstance->getTotalNumInputChannels();
        auto numOutputChannels = vstInstance->getTotalNumOutputChannels();
        auto numChannelsToUse = juce::jmax (1, juce::jmax (numInputChannels, numOutputChannels));

        if (scratchBuffer.getNumChannels() < numChannelsToUse || scratchBuffer.getNumSamples() < numSamples)
            scratchBuffer.setSize (numChannelsToUse, numSamples, false, false, true);

        scratchBuffer.clear();

        for (int ch = 0; ch < numChannelsToUse; ++ch)
        {
            auto* dest = scratchBuffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                dest[i] = samples[i];
        }

        juce::AudioBuffer<float> view (scratchBuffer.getArrayOfWritePointers(), numChannelsToUse, numSamples);

        midiBuffer.clear();
        vstInstance->processBlock (view, midiBuffer);

        for (int i = 0; i < numSamples; ++i)
        {
            float mixed = 0.0f;
            for (int ch = 0; ch < numChannelsToUse; ++ch)
                mixed += scratchBuffer.getSample (ch, i);

            samples[i] = mixed / (float) numChannelsToUse;
        }
    }

    // Affiche brievement l'editeur dans une vraie fenetre visible a l'ecran,
    // a pleine opacite (necessaire pour que les pixels captures ne soient pas
    // transparents), puis capture et referme aussitot.
    //
    // Certains plugins (Ozone, RC 48...) redimensionnent leur editeur juste
    // apres sa creation (taille initiale = placeholder, taille finale posee
    // un peu plus tard par le plugin lui-meme). On resynchronise donc la
    // taille de la fenetre juste avant la capture, avec un court delai
    // supplementaire pour laisser ce redimensionnement se produire.
    void startThumbnailCapture()
    {
        if (! juce::MessageManager::getInstance()->isThisTheMessageThread())
        {
            juce::MessageManager::callAsync ([this] { startThumbnailCapture(); });
            return;
        }

        juce::AudioPluginInstance* instance = nullptr;
        {
            const juce::SpinLock::ScopedLockType lock (pluginLock);
            instance = vstInstance.get();
        }

        if (instance == nullptr)
            return;

        auto editor = std::shared_ptr<juce::AudioProcessorEditor> (instance->createEditorIfNeeded());

        if (editor == nullptr)
            return;

        auto window = std::make_shared<juce::DocumentWindow>(
            "capture", juce::Colours::black, juce::DocumentWindow::TitleBarButtons::closeButton);

        window->setContentNonOwned (editor.get(), true);
        window->setAlwaysOnTop (true);
        window->setAlpha (1.0f);
        window->setUsingNativeTitleBar (false);
        window->setDropShadowEnabled (false);
        window->setVisible (true);
        window->setBounds (20, 20, editor->getWidth(), editor->getHeight());
        window->toFront (false);

        auto tempFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getChildFile ("liverack_thumb_" + juce::String (juce::Random::getSystemRandom().nextInt()) + ".png");
        auto tempPath = tempFile.getFullPathName().toStdString();

        // Etape 1 (300ms) : laisser l'editeur finir son premier affichage
        // et son eventuel auto-redimensionnement, puis resynchroniser la
        // fenetre sur sa taille reelle actuelle.
        juce::Timer::callAfterDelay (300, [this, editor, window, tempFile, tempPath]
        {
            auto realWidth  = editor->getWidth();
            auto realHeight = editor->getHeight();

            if (realWidth > 0 && realHeight > 0)
                window->setBounds (20, 20, realWidth, realHeight);

            // Etape 2 (200ms de plus) : laisser ce resize se composer
            // reellement a l'ecran avant de capturer les pixels.
            juce::Timer::callAfterDelay (200, [this, editor, window, tempFile, tempPath]
            {
                auto* peer = editor->getPeer();
                auto nativeHandle = peer != nullptr ? peer->getNativeHandle() : nullptr;

                if (nativeHandle != nullptr)
                {
                    auto captureOk = captureNativeWindowToFile (nativeHandle, tempPath);

                    if (captureOk)
                        thumbnailImage = juce::ImageFileFormat::loadFrom (tempFile);
                }

                tempFile.deleteFile();

                window->setContentNonOwned (nullptr, false);
                window->setVisible (false);
            });
        });
    }

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    SlotContentType contentType = SlotContentType::empty;
    bool bypassed = false;

    Compressor compressor;

    juce::SpinLock pluginLock;
    std::unique_ptr<juce::AudioPluginInstance> vstInstance;
    juce::String vstDisplayName;
    juce::PluginDescription lastPluginDescription;
    juce::AudioBuffer<float> scratchBuffer;
    juce::MidiBuffer midiBuffer;
    juce::Image thumbnailImage;
};
