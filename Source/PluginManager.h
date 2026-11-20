#pragma once
#include <JuceHeader.h>

class PluginManager
{
public:
    PluginManager()
    {
       #if JUCE_PLUGINHOST_VST3
        formatManager.addFormat (new juce::VST3PluginFormat());
       #endif
       #if JUCE_PLUGINHOST_AU && JUCE_MAC
        formatManager.addFormat (new juce::AudioUnitPluginFormat());
       #endif
    }

    bool loadCacheIfAvailable()
    {
        auto cacheFile = getCacheFile();

        if (! cacheFile.existsAsFile())
            return false;

        scannedPlugins.clear();

        auto lines = juce::StringArray::fromLines (cacheFile.loadFileAsString());

        for (auto& line : lines)
            tryParseEncodedLine (line);

        DBG ("Cache charge : " << (int) scannedPlugins.size() << " plugins.");
        return ! scannedPlugins.empty();
    }

    void scanForPlugins()
    {
        scannedPlugins.clear();
        failedFiles.clear();

        auto workerPath = findWorkerExecutable();

        if (! workerPath.existsAsFile())
        {
            DBG ("PluginScanWorker introuvable a : " << workerPath.getFullPathName());
            return;
        }

        for (int i = 0; i < formatManager.getNumFormats(); ++i)
        {
            auto* format = formatManager.getFormat (i);

            juce::FileSearchPath searchPath (format->getDefaultLocationsToSearch());
            auto filesOrIds = format->searchPathsForPlugins (searchPath, true, false);

            DBG ("Format " << format->getName() << " : " << filesOrIds.size() << " fichiers trouves.");

            for (auto& fileOrId : filesOrIds)
                scanOneFileIsolated (workerPath, format->getName(), fileOrId);
        }

        DBG ("Scan termine. Plugins valides : " << (int) scannedPlugins.size()
             << " | Plugins en echec : " << failedFiles.size());

        saveCacheToDisk();
    }

    const std::vector<juce::PluginDescription>& getScannedPlugins() const { return scannedPlugins; }
    const juce::StringArray& getFailedFiles() const                       { return failedFiles; }

    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }

private:
    juce::File getCacheFile() const
    {
        auto appDataDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                            .getChildFile ("LiveRack");
        appDataDir.createDirectory();
        return appDataDir.getChildFile ("plugin_cache.txt");
    }

    void saveCacheToDisk()
    {
        juce::String content;

        for (auto& desc : scannedPlugins)
        {
            if (auto xml = desc.createXml())
            {
                auto xmlString = xml->toString();
                auto encoded = juce::Base64::toBase64 (xmlString.toUTF8(), xmlString.getNumBytesAsUTF8());
                content << "XMLDESC|" << encoded << "\n";
            }
        }

        auto cacheFile = getCacheFile();
        cacheFile.replaceWithText (content);

        DBG ("Cache sauvegarde : " << cacheFile.getFullPathName());
    }

    void tryParseEncodedLine (const juce::String& line)
    {
        if (! line.startsWith ("XMLDESC|"))
            return;

        auto encoded = line.substring (8);

        juce::MemoryOutputStream decodedStream;
        if (! juce::Base64::convertFromBase64 (decodedStream, encoded))
            return;

        auto xmlString = decodedStream.toString();
        auto xml = juce::parseXML (xmlString);

        if (xml == nullptr)
            return;

        juce::PluginDescription desc;
        if (desc.loadFromXml (*xml))
            scannedPlugins.push_back (desc);
    }

    juce::File findWorkerExecutable() const
    {
        auto appBundleDir = juce::File::getSpecialLocation (juce::File::currentApplicationFile);
        auto debugDir = appBundleDir.getParentDirectory();
        auto liveRackArtefactsDir = debugDir.getParentDirectory();
        auto buildDir = liveRackArtefactsDir.getParentDirectory();

        return buildDir.getChildFile ("PluginScanWorker_artefacts")
                        .getChildFile ("Debug")
                        .getChildFile ("PluginScanWorker");
    }

    void scanOneFileIsolated (const juce::File& workerPath, const juce::String& formatName, const juce::String& fileOrId)
    {
        DBG ("Scan isole : " << fileOrId);

        juce::ChildProcess process;

        juce::StringArray args;
        args.add (workerPath.getFullPathName());
        args.add (formatName);
        args.add (fileOrId);

        if (! process.start (args))
        {
            DBG ("Impossible de lancer le sous-processus");
            failedFiles.add (fileOrId);
            return;
        }

        auto output = process.readAllProcessOutput();

        auto finished = process.waitForProcessToFinish (10000);

        if (! finished)
        {
            DBG ("Timeout, le plugin met trop de temps ou est bloque");
            process.kill();
            failedFiles.add (fileOrId);
            return;
        }

        if (process.getExitCode() != 0)
        {
            DBG ("Echec du scan pour ce fichier");
            failedFiles.add (fileOrId);
            return;
        }

        auto lines = juce::StringArray::fromLines (output);
        auto before = scannedPlugins.size();

        for (auto& line : lines)
            tryParseEncodedLine (line);

        if (scannedPlugins.size() == before)
        {
            DBG ("Aucun plugin valide trouve dans ce fichier");
            failedFiles.add (fileOrId);
        }
        else
        {
            for (auto i = before; i < scannedPlugins.size(); ++i)
                DBG ("Plugin valide : " << scannedPlugins[i].name);
        }
    }

    juce::AudioPluginFormatManager formatManager;
    std::vector<juce::PluginDescription> scannedPlugins;
    juce::StringArray failedFiles;
};
