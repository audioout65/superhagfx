#pragma once
#include <JuceHeader.h>

// Une ligne de la liste : nom du fichier + un bouton d'action principale
// (Charger ou Rappeler) + un bouton Supprimer.
class FileRow : public juce::Component
{
public:
    FileRow (const juce::String& name, const juce::String& primaryActionLabel)
    {
        nameLabel.setText (name, juce::dontSendNotification);
        nameLabel.setFont (juce::Font (14.0f));
        nameLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (nameLabel);

        primaryButton.setButtonText (primaryActionLabel);
        addAndMakeVisible (primaryButton);

        deleteButton.setButtonText ("Suppr.");
        addAndMakeVisible (deleteButton);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff20242a));
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (6, 2);
        deleteButton.setBounds (area.removeFromRight (70));
        area.removeFromRight (6);
        primaryButton.setBounds (area.removeFromRight (90));
        area.removeFromRight (10);
        nameLabel.setBounds (area);
    }

    juce::Label nameLabel;
    juce::TextButton primaryButton;
    juce::TextButton deleteButton;
};

// Une section complete : un titre, un bouton "+ Nouveau", et la liste des
// lignes (Sessions ou Snapshots).
class FileListSection : public juce::Component
{
public:
    FileListSection (const juce::String& title, const juce::String& addButtonText, const juce::String& primaryActionLabel)
        : rowActionLabel (primaryActionLabel)
    {
        titleLabel.setText (title, juce::dontSendNotification);
        titleLabel.setFont (juce::Font (15.0f, juce::Font::bold));
        titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a92a0));
        addAndMakeVisible (titleLabel);

        addButton.setButtonText (addButtonText);
        addAndMakeVisible (addButton);
        addButton.onClick = [this] { if (onAddRequested != nullptr) onAddRequested(); };
    }

    void setEntries (const std::vector<std::pair<juce::String, juce::File>>& entries)
    {
        rows.clear();

        for (auto& entry : entries)
        {
            auto row = std::make_unique<FileRow> (entry.first, rowActionLabel);
            auto file = entry.second;

            row->primaryButton.onClick = [this, file] { if (onPrimaryAction != nullptr) onPrimaryAction (file); };
            row->deleteButton.onClick = [this, file] { if (onDeleteAction != nullptr) onDeleteAction (file); };

            addAndMakeVisible (*row);
            rows.push_back (std::move (row));
        }

        resized();
    }

    void paint (juce::Graphics& g) override
    {
        if (rows.empty())
        {
            g.setColour (juce::Colour (0xff5a6170));
            g.setFont (13.0f);
            g.drawText ("(vide)", getLocalBounds().withTrimmedTop (32), juce::Justification::centredTop);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds();
        auto header = area.removeFromTop (28);
        titleLabel.setBounds (header.removeFromLeft (header.getWidth() - 110));
        addButton.setBounds (header.removeFromRight (100));

        area.removeFromTop (8);

        constexpr int rowHeight = 32;
        constexpr int rowSpacing = 4;

        for (auto& row : rows)
        {
            row->setBounds (area.removeFromTop (rowHeight));
            area.removeFromTop (rowSpacing);
        }
    }

    std::function<void()> onAddRequested;
    std::function<void (juce::File)> onPrimaryAction;
    std::function<void (juce::File)> onDeleteAction;

private:
    juce::String rowActionLabel;
    juce::Label titleLabel;
    juce::TextButton addButton;
    std::vector<std::unique_ptr<FileRow>> rows;
};

// Vue SHOW : Sessions a gauche, Snapshots a droite.
class ShowView : public juce::Component
{
public:
    ShowView()
        : sessionsSection ("SESSIONS", "+ Nouvelle", "Charger"),
          snapshotsSection ("SNAPSHOTS", "+ Nouveau", "Rappeler")
    {
        addAndMakeVisible (sessionsSection);
        addAndMakeVisible (snapshotsSection);

        sessionsSection.onAddRequested = [this] { if (onSaveSessionRequested != nullptr) onSaveSessionRequested(); };
        sessionsSection.onPrimaryAction = [this] (juce::File f) { if (onLoadSessionRequested != nullptr) onLoadSessionRequested (f); };
        sessionsSection.onDeleteAction = [this] (juce::File f) { if (onDeleteSessionRequested != nullptr) onDeleteSessionRequested (f); };

        snapshotsSection.onAddRequested = [this] { if (onSaveSnapshotRequested != nullptr) onSaveSnapshotRequested(); };
        snapshotsSection.onPrimaryAction = [this] (juce::File f) { if (onRecallSnapshotRequested != nullptr) onRecallSnapshotRequested (f); };
        snapshotsSection.onDeleteAction = [this] (juce::File f) { if (onDeleteSnapshotRequested != nullptr) onDeleteSnapshotRequested (f); };
    }

    void setSessions (const std::vector<std::pair<juce::String, juce::File>>& entries) { sessionsSection.setEntries (entries); }
    void setSnapshots (const std::vector<std::pair<juce::String, juce::File>>& entries) { snapshotsSection.setEntries (entries); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff181b1f));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (20);
        auto half = area.getWidth() / 2 - 10;
        sessionsSection.setBounds (area.removeFromLeft (half));
        area.removeFromLeft (20);
        snapshotsSection.setBounds (area);
    }

    std::function<void()> onSaveSessionRequested;
    std::function<void (juce::File)> onLoadSessionRequested;
    std::function<void (juce::File)> onDeleteSessionRequested;

    std::function<void()> onSaveSnapshotRequested;
    std::function<void (juce::File)> onRecallSnapshotRequested;
    std::function<void (juce::File)> onDeleteSnapshotRequested;

private:
    FileListSection sessionsSection;
    FileListSection snapshotsSection;
};
