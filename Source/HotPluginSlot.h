#pragma once
#include <JuceHeader.h>

// Represente un "Hot Plugin" : un plugin favori sauvegarde avec son etat
// exact (tous ses reglages) et une miniature, pour le recharger en un clic
// dans n'importe quel slot, sur n'importe quel canal. Les Hot Plugins sont
// stockes separement des sessions (.liverack) : ce sont des favoris
// permanents qui survivent entre les lancements de l'application.
class HotPluginSlot
{
public:
    bool hasPlugin = false;
    juce::String displayName;
    juce::PluginDescription description;
    juce::MemoryBlock state;
    juce::Image thumbnail;

    void setFromPlugin (const juce::String& name, const juce::PluginDescription& desc,
                         const juce::MemoryBlock& pluginState, const juce::Image& thumb)
    {
        displayName = name;
        description = desc;
        state = pluginState;
        thumbnail = thumb;
        hasPlugin = true;
    }

    void clear()
    {
        hasPlugin = false;
        displayName.clear();
        description = juce::PluginDescription();
        state.reset();
        thumbnail = juce::Image();
    }

    void saveToXml (juce::XmlElement& xml) const
    {
        xml.setAttribute ("empty", ! hasPlugin);

        if (! hasPlugin)
            return;

        xml.setAttribute ("displayName", displayName);

        if (auto descXml = description.createXml())
            xml.addChildElement (descXml.release());

        xml.setAttribute ("state", state.toBase64Encoding());

        if (thumbnail.isValid())
        {
            juce::MemoryOutputStream pngStream;
            juce::PNGImageFormat png;

            if (png.writeImageToStream (thumbnail, pngStream))
                xml.setAttribute ("thumbnail", pngStream.getMemoryBlock().toBase64Encoding());
        }
    }

    void loadFromXml (const juce::XmlElement& xml)
    {
        if (xml.getBoolAttribute ("empty", true))
        {
            clear();
            return;
        }

        displayName = xml.getStringAttribute ("displayName");

        juce::PluginDescription desc;
        for (auto* child : xml.getChildIterator())
        {
            if (child->hasTagName ("PLUGIN"))
            {
                desc.loadFromXml (*child);
                break;
            }
        }
        description = desc;

        juce::MemoryBlock s;
        auto stateBase64 = xml.getStringAttribute ("state");
        if (stateBase64.isNotEmpty())
            s.fromBase64Encoding (stateBase64);
        state = s;

        auto thumbBase64 = xml.getStringAttribute ("thumbnail");
        if (thumbBase64.isNotEmpty())
        {
            juce::MemoryBlock thumbData;
            thumbData.fromBase64Encoding (thumbBase64);
            thumbnail = juce::ImageFileFormat::loadFrom (thumbData.getData(), thumbData.getSize());
        }

        hasPlugin = true;
    }
};

inline juce::File getHotPluginsFile()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("LiveRack");
    dir.createDirectory();
    return dir.getChildFile ("hotplugins.xml");
}

template <size_t N>
void saveHotPlugins (const std::array<HotPluginSlot, N>& slots)
{
    juce::XmlElement root ("HotPlugins");

    for (size_t i = 0; i < N; ++i)
    {
        auto* slotXml = root.createNewChildElement ("Slot");
        slotXml->setAttribute ("index", (int) i);
        slots[i].saveToXml (*slotXml);
    }

    root.writeTo (getHotPluginsFile());
}

template <size_t N>
void loadHotPlugins (std::array<HotPluginSlot, N>& slots)
{
    auto file = getHotPluginsFile();

    if (! file.existsAsFile())
        return;

    auto xml = juce::parseXML (file);

    if (xml == nullptr || ! xml->hasTagName ("HotPlugins"))
        return;

    for (auto* slotXml : xml->getChildWithTagNameIterator ("Slot"))
    {
        auto index = slotXml->getIntAttribute ("index", -1);

        if (index >= 0 && (size_t) index < N)
            slots[(size_t) index].loadFromXml (*slotXml);
    }
}
