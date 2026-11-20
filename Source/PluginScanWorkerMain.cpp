#include <JuceHeader.h>
#include <iostream>

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    if (argc < 3)
    {
        std::cout << "ERROR|Usage incorrect" << std::endl;
        return 1;
    }

    juce::String formatName (argv[1]);
    juce::String fileOrId (argv[2]);

    juce::AudioPluginFormatManager formatManager;
   #if JUCE_PLUGINHOST_VST3
    formatManager.addFormat (new juce::VST3PluginFormat());
   #endif
   #if JUCE_PLUGINHOST_AU && JUCE_MAC
    formatManager.addFormat (new juce::AudioUnitPluginFormat());
   #endif

    juce::AudioPluginFormat* format = nullptr;
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
        if (formatManager.getFormat (i)->getName() == formatName)
            format = formatManager.getFormat (i);

    if (format == nullptr)
    {
        std::cout << "ERROR|Format inconnu" << std::endl;
        return 1;
    }

    juce::KnownPluginList knownPlugins;
    juce::OwnedArray<juce::PluginDescription> typesFound;
    knownPlugins.scanAndAddFile (fileOrId, false, typesFound, *format);

    for (auto* desc : typesFound)
    {
        if (auto xml = desc->createXml())
        {
            auto xmlString = xml->toString();
            auto encoded = juce::Base64::toBase64 (xmlString.toUTF8(), xmlString.getNumBytesAsUTF8());
            std::cout << "XMLDESC|" << encoded << std::endl;
        }
    }

    std::cout << "DONE" << std::endl;
    return 0;
}
