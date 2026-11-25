#pragma once
#include <JuceHeader.h>

class FileRow : public juce::Component
{
public:
    FileRow (const juce::String& displayName, const juce::File& fileIn, const juce::String& primaryActionLabel)
        : file (fileIn)
    {
        nameLabel.setText (displayName, juce::dontSendNotification);
        nameLabel.setFont (13.0f);
        nameLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (nameLabel);

        saveButton.setButtonText ("Sauver");
        addAndMakeVisible (saveButton);

        primaryButton.setButtonText (primaryActionLabel);
        addAndMakeVisible (primaryButton);

        deleteButton.setButtonText ("Suppr");
        deleteButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff5a2a2a));
        addAndMakeVisible (deleteButton);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff20242a));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (6, 3);
        deleteButton.setBounds (area.removeFromRight (70));
        area.removeFromRight (6);
        primaryButton.setBounds (area.removeFromRight (90));
        area.removeFromRight (6);
        saveButton.setBounds (area.removeFromRight (70));
        area.removeFromRight (8);
        nameLabel.setBounds (area);
    }

    juce::File file;
    juce::Label nameLabel;
    juce::TextButton saveButton;
    juce::TextButton primaryButton;
    juce::TextButton deleteButton;
};

class FileListSection : public juce::Component
{
public:
    FileListSection (const juce::String& title, const juce::String& addButtonText, const juce::String& primaryActionLabel)
        : rowActionLabel (primaryActionLabel)
    {
        titleLabel.setText (title, juce::dontSendNotification);
        titleLabel.setFont (juce::Font (13.0f, juce::Font::bold));
        titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a92a0));
        addAndMakeVisible (titleLabel);

        addButton.setButtonText (addButtonText);
        addButton.onClick = [this] { if (onAddRequested != nullptr) onAddRequested(); };
        addAndMakeVisible (addButton);
    }

    void setEntries (const std::vector<std::pair<juce::String, juce::File>>& entries)
    {
        rows.clear();

        for (auto& entry : entries)
        {
            auto row = std::make_unique<FileRow>(entry.first, entry.second, rowActionLabel);

            auto file = entry.second;

            row->saveButton.onClick = [this, file]
            {
                if (onSaveAction != nullptr)
                    onSaveAction (file);
            };
            row->primaryButton.onClick = [this, file]
            {
                if (onPrimaryAction != nullptr)
                    onPrimaryAction (file);
            };
            row->deleteButton.onClick = [this, file]
            {
                if (onDeleteAction != nullptr)
                    onDeleteAction (file);
            };

            addAndMakeVisible (*row);
            rows.push_back (std::move (row));
        }

        resized();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        if (rows.empty())
        {
            g.setColour (juce::Colour (0xff5a6170));
            g.setFont (13.0f);
            g.drawText ("(vide)", getLocalBounds().withTrimmedTop (30), juce::Justification::centred);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds();

        auto header = area.removeFromTop (24);
        titleLabel.setBounds (header.removeFromLeft (header.getWidth() - 110));
        addButton.setBounds (header.removeFromRight (100));

        area.removeFromTop (6);

        constexpr int rowHeight = 32;
        constexpr int rowSpacing = 4;

        for (auto& row : rows)
        {
            row->setBounds (area.removeFromTop (rowHeight));
            area.removeFromTop (rowSpacing);
        }
    }

    std::function<void()> onAddRequested;
    std::function<void (juce::File)> onSaveAction;
    std::function<void (juce::File)> onPrimaryAction;
    std::function<void (juce::File)> onDeleteAction;

private:
    juce::String rowActionLabel;
    juce::Label titleLabel;
    juce::TextButton addButton;
    std::vector<std::unique_ptr<FileRow>> rows;
};

class ShowView : public juce::Component
{
public:
    ShowView()
        : sessionsSection ("SESSIONS", "+ Nouvelle", "Charger"),
          snapshotsSection ("SNAPSHOTS RACK", "+ Nouveau", "Rappeler"),
          patchSnapshotsSection ("SNAPSHOTS PATCH", "+ Nouveau", "Rappeler")
    {
        addAndMakeVisible (sessionsSection);
        addAndMakeVisible (snapshotsSection);
        addAndMakeVisible (patchSnapshotsSection);

        sessionsSection.onAddRequested = [this] { if (onSaveSessionRequested != nullptr) onSaveSessionRequested(); };
        sessionsSection.onSaveAction = [this] (juce::File f) { if (onOverwriteSessionRequested != nullptr) onOverwriteSessionRequested (f); };
        sessionsSection.onPrimaryAction = [this] (juce::File f) { if (onLoadSessionRequested != nullptr) onLoadSessionRequested (f); };
        sessionsSection.onDeleteAction = [this] (juce::File f) { if (onDeleteSessionRequested != nullptr) onDeleteSessionRequested (f); };

        snapshotsSection.onAddRequested = [this] { if (onSaveSnapshotRequested != nullptr) onSaveSnapshotRequested(); };
        snapshotsSection.onSaveAction = [this] (juce::File f) { if (onOverwriteSnapshotRequested != nullptr) onOverwriteSnapshotRequested (f); };
        snapshotsSection.onPrimaryAction = [this] (juce::File f) { if (onRecallSnapshotRequested != nullptr) onRecallSnapshotRequested (f); };
        snapshotsSection.onDeleteAction = [this] (juce::File f) { if (onDeleteSnapshotRequested != nullptr) onDeleteSnapshotRequested (f); };

        patchSnapshotsSection.onAddRequested = [this] { if (onSavePatchSnapshotRequested != nullptr) onSavePatchSnapshotRequested(); };
        patchSnapshotsSection.onSaveAction = [this] (juce::File f) { if (onOverwritePatchSnapshotRequested != nullptr) onOverwritePatchSnapshotRequested (f); };
        patchSnapshotsSection.onPrimaryAction = [this] (juce::File f) { if (onRecallPatchSnapshotRequested != nullptr) onRecallPatchSnapshotRequested (f); };
        patchSnapshotsSection.onDeleteAction = [this] (juce::File f) { if (onDeletePatchSnapshotRequested != nullptr) onDeletePatchSnapshotRequested (f); };
    }

    void setSessions (const std::vector<std::pair<juce::String, juce::File>>& entries) { sessionsSection.setEntries (entries); }
    void setSnapshots (const std::vector<std::pair<juce::String, juce::File>>& entries) { snapshotsSection.setEntries (entries); }
    void setPatchSnapshots (const std::vector<std::pair<juce::String, juce::File>>& entries) { patchSnapshotsSection.setEntries (entries); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff181b1f));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (20);

        auto leftHalf = area.removeFromLeft (area.getWidth() / 2 - 10);
        area.removeFromLeft (20);
        auto rightHalf = area;

        sessionsSection.setBounds (leftHalf);

        constexpr int gapBetween = 16;
        auto snapshotsHeight = (rightHalf.getHeight() - gapBetween) * 55 / 100;

        snapshotsSection.setBounds (rightHalf.removeFromTop (snapshotsHeight));
        rightHalf.removeFromTop (gapBetween);
        patchSnapshotsSection.setBounds (rightHalf);
    }

    std::function<void()> onSaveSessionRequested;
    std::function<void (juce::File)> onOverwriteSessionRequested;
    std::function<void (juce::File)> onLoadSessionRequested;
    std::function<void (juce::File)> onDeleteSessionRequested;

    std::function<void()> onSaveSnapshotRequested;
    std::function<void (juce::File)> onOverwriteSnapshotRequested;
    std::function<void (juce::File)> onRecallSnapshotRequested;
    std::function<void (juce::File)> onDeleteSnapshotRequested;

    std::function<void()> onSavePatchSnapshotRequested;
    std::function<void (juce::File)> onOverwritePatchSnapshotRequested;
    std::function<void (juce::File)> onRecallPatchSnapshotRequested;
    std::function<void (juce::File)> onDeletePatchSnapshotRequested;

private:
    FileListSection sessionsSection;
    FileListSection snapshotsSection;
    FileListSection patchSnapshotsSection;
};
