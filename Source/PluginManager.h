#pragma once
#include <JuceHeader.h>

struct ScannedPlugin
{
    juce::String name;
    juce::String manufacturer;
    juce::String formatName;
    juce::String fileOrIdentifier;
};

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

    // Charge le cache s'il existe. Retourne true si des plugins ont ete charges.
    bool loadCacheIfAvailable()
    {
        auto cacheFile = getCacheFile();

        if (! cacheFile.existsAsFile())
            return false;

        scannedPlugins.clear();

        auto lines = juce::StringArray::fromLines (cacheFile.loadFileAsString());

        for (auto& line : lines)
        {
            if (line.startsWith ("PLUGIN|"))
            {
                auto parts = juce::StringArray::fromTokens (line, "|", "");
                if (parts.size() >= 5)
                {
                    ScannedPlugin plugin;
                    plugin.name             = parts[1];
                    plugin.manufacturer     = parts[2];
                    plugin.formatName       = parts[3];
                    plugin.fileOrIdentifier = parts[4];
                    scannedPlugins.push_back (plugin);
                }
            }
        }

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

    const std::vector<ScannedPlugin>& getScannedPlugins() const { return scannedPlugins; }
    const juce::StringArray& getFailedFiles() const             { return failedFiles; }

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

        for (auto& plugin : scannedPlugins)
        {
            content << "PLUGIN|" << plugin.name << "|" << plugin.manufacturer
                    << "|" << plugin.formatName << "|" << plugin.fileOrIdentifier << "\n";
        }

        auto cacheFile = getCacheFile();
        cacheFile.replaceWithText (content);

        DBG ("Cache sauvegarde : " << cacheFile.getFullPathName());
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
        bool foundAny = false;

        for (auto& line : lines)
        {
            if (line.startsWith ("PLUGIN|"))
            {
                auto parts = juce::StringArray::fromTokens (line, "|", "");
                if (parts.size() >= 5)
                {
                    ScannedPlugin plugin;
                    plugin.name             = parts[1];
                    plugin.manufacturer     = parts[2];
                    plugin.formatName       = parts[3];
                    plugin.fileOrIdentifier = parts[4];
                    scannedPlugins.push_back (plugin);
                    foundAny = true;
                    DBG ("Plugin valide : " << plugin.name);
                }
            }
        }

        if (! foundAny)
        {
            DBG ("Aucun plugin valide trouve dans ce fichier");
            failedFiles.add (fileOrId);
        }
    }

    juce::AudioPluginFormatManager formatManager;
    std::vector<ScannedPlugin> scannedPlugins;
    juce::StringArray failedFiles;
};

