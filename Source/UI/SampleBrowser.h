#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SourceInput/FactorySamples.h"
#include "UI/GrainHexLookAndFeel.h"
#include <vector>
#include <functional>

namespace grainhex {

/**
 * Internal sample browser — shows factory samples organized by category
 * with preview playback and load functionality.
 */
class SampleBrowser : public juce::Component,
                      public juce::ListBoxModel
{
public:
    SampleBrowser()
    {
        // Category filter
        addAndMakeVisible(categoryBox);
        categoryBox.addItem("All", 1);
        categoryBox.setSelectedId(1, juce::dontSendNotification);
        categoryBox.onChange = [this] { filterByCategory(); };

        addAndMakeVisible(categoryLabel);
        categoryLabel.setText("Category", juce::dontSendNotification);
        categoryLabel.setJustificationType(juce::Justification::centredRight);
        categoryLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::textDim));
        categoryLabel.setFont(juce::Font(Theme::fontSmall));

        // Sample list
        addAndMakeVisible(listBox);
        listBox.setModel(this);
        listBox.setRowHeight(32);
        listBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(Theme::bgControl));
        listBox.setColour(juce::ListBox::outlineColourId, juce::Colour(Theme::border));
        listBox.addMouseListener(this, true);

        // Import button
        addAndMakeVisible(importButton);
        importButton.setButtonText("Import WAV");
        importButton.setColour(juce::TextButton::buttonColourId, juce::Colour(Theme::buttonUtility));
        importButton.onClick = [this]
        {
            if (onImportSample)
                onImportSample();
        };

        // Load button
        addAndMakeVisible(loadButton);
        loadButton.setButtonText("Load");
        loadButton.setColour(juce::TextButton::buttonColourId, juce::Colour(Theme::accentOrange));
        loadButton.setColour(juce::TextButton::textColourOffId, juce::Colour(Theme::bgDarkest));
        loadButton.onClick = [this]
        {
            int selected = listBox.getSelectedRow();
            if (selected >= 0 && selected < static_cast<int>(filteredSamples.size()) && onLoadSample)
                onLoadSample(filteredSamples[static_cast<size_t>(selected)]);
        };

        // Preview button
        addAndMakeVisible(previewButton);
        previewButton.setButtonText("Preview");
        previewButton.setColour(juce::TextButton::buttonColourId, juce::Colour(Theme::buttonSecondary));
        previewButton.onClick = [this]
        {
            int selected = listBox.getSelectedRow();
            if (selected >= 0 && selected < static_cast<int>(filteredSamples.size()) && onPreviewSample)
                onPreviewSample(filteredSamples[static_cast<size_t>(selected)]);
        };

        // Title
        addAndMakeVisible(titleLabel);
        titleLabel.setText("FACTORY LIBRARY", juce::dontSendNotification);
        titleLabel.setFont(juce::Font(Theme::fontSectionHead).boldened());
        titleLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::accentOrange));

        // Generate samples
        generateFactorySamples();
    }

    void generateFactorySamples()
    {
        factorySamples = FactorySampleGenerator::generateAll();

        // Build category list
        juce::StringArray categories;
        for (auto& s : factorySamples)
        {
            if (!categories.contains(s.category))
                categories.add(s.category);
        }

        int id = 2; // 1 is "All"
        for (auto& cat : categories)
            categoryBox.addItem(cat, id++);

        filterByCategory();
    }

    void filterByCategory()
    {
        filteredSamples.clear();
        juce::String selectedCat;

        if (categoryBox.getSelectedId() > 1)
            selectedCat = categoryBox.getText();

        for (auto& s : factorySamples)
        {
            if (selectedCat.isEmpty() || s.category == selectedCat)
                filteredSamples.push_back(s);
        }

        listBox.updateContent();
        listBox.repaint();
    }

    void setLoadedSample(const juce::String& name, const juce::String& category)
    {
        loadedSampleName = name;
        loadedSampleCategory = category;
        hasLoadedFactorySample = true;
        listBox.repaint();
    }

    void clearLoadedSample()
    {
        loadedSampleName.clear();
        loadedSampleCategory.clear();
        hasLoadedFactorySample = false;
        listBox.repaint();
    }

    // ListBoxModel
    int getNumRows() override { return static_cast<int>(filteredSamples.size()); }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height,
                          bool rowIsSelected) override
    {
        if (rowNumber < 0 || rowNumber >= static_cast<int>(filteredSamples.size()))
            return;

        auto& sample = filteredSamples[static_cast<size_t>(rowNumber)];

        const bool isHovered = rowNumber == hoveredRow;
        const bool isLoaded = hasLoadedFactorySample
            && sample.name == loadedSampleName
            && sample.category == loadedSampleCategory;

        if (rowIsSelected)
            g.fillAll(juce::Colour(Theme::bgElevated));
        else if (isHovered)
            g.fillAll(juce::Colour(Theme::bgHover));

        if (rowIsSelected || isLoaded)
        {
            g.setColour(juce::Colour(Theme::accentOrange));
            g.fillRect(0, 2, 3, height - 4);
        }

        g.setColour(juce::Colour(Theme::textNormal));
        auto rowFont = juce::Font(Theme::fontValue);
        if (isLoaded)
            rowFont = rowFont.boldened();
        g.setFont(rowFont);
        g.drawText(sample.name, 8, 0, width - 80, height, juce::Justification::centredLeft);

        g.setColour(juce::Colour(Theme::textDim));
        g.setFont(juce::Font(Theme::fontSmall));
        g.drawText(sample.category, width - 75, 0, 70, height, juce::Justification::centredRight);
    }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override
    {
        if (row >= 0 && row < static_cast<int>(filteredSamples.size()) && onLoadSample)
            onLoadSample(filteredSamples[static_cast<size_t>(row)]);
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(Theme::bgPanel));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Theme::cornerRadius);

        g.setColour(juce::Colour(Theme::border));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f),
                               Theme::cornerRadius, Theme::borderWidth);
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        auto point = e.getEventRelativeTo(&listBox).position;
        auto row = listBox.getRowContainingPosition(static_cast<int>(point.x), static_cast<int>(point.y));
        if (row != hoveredRow)
        {
            hoveredRow = row;
            listBox.repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (hoveredRow >= 0)
        {
            hoveredRow = -1;
            listBox.repaint();
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(Theme::panelPadding);

        auto titleRow = area.removeFromTop(22);
        titleLabel.setBounds(titleRow);
        area.removeFromTop(Theme::controlGap);

        auto filterRow = area.removeFromTop(26);
        categoryLabel.setBounds(filterRow.removeFromLeft(60));
        filterRow.removeFromLeft(Theme::controlGap);
        categoryBox.setBounds(filterRow);
        area.removeFromTop(Theme::controlGap);

        auto buttonRow = area.removeFromBottom(30);
        importButton.setBounds(buttonRow.removeFromLeft(92));
        buttonRow.removeFromLeft(Theme::controlGap);
        previewButton.setBounds(buttonRow.removeFromLeft(74));
        buttonRow.removeFromLeft(Theme::controlGap);
        loadButton.setBounds(buttonRow.removeFromLeft(64));

        area.removeFromBottom(Theme::controlGap);
        listBox.setBounds(area);
    }

    // Callbacks
    std::function<void(const FactorySample&)> onLoadSample;
    std::function<void(const FactorySample&)> onPreviewSample;
    std::function<void()> onImportSample;

    // Get a factory sample to use as initial default sound
    const FactorySample* getDefaultSample() const
    {
        if (factorySamples.empty()) return nullptr;
        return &factorySamples[0]; // Classic Reese
    }

private:
    std::vector<FactorySample> factorySamples;
    std::vector<FactorySample> filteredSamples;

    juce::Label titleLabel;
    juce::ComboBox categoryBox;
    juce::Label categoryLabel;
    juce::ListBox listBox;
    juce::TextButton importButton;
    juce::TextButton loadButton;
    juce::TextButton previewButton;
    int hoveredRow = -1;
    juce::String loadedSampleName;
    juce::String loadedSampleCategory;
    bool hasLoadedFactorySample = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBrowser)
};

} // namespace grainhex
