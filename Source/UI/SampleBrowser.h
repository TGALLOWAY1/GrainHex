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

        // Sample list
        addAndMakeVisible(listBox);
        listBox.setModel(this);
        listBox.setRowHeight(28);
        listBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(Theme::bgControl));
        listBox.setColour(juce::ListBox::outlineColourId, juce::Colour(Theme::border));

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
        previewButton.onClick = [this]
        {
            int selected = listBox.getSelectedRow();
            if (selected >= 0 && selected < static_cast<int>(filteredSamples.size()) && onPreviewSample)
                onPreviewSample(filteredSamples[static_cast<size_t>(selected)]);
        };

        // Title
        addAndMakeVisible(titleLabel);
        titleLabel.setText("BROWSER", juce::dontSendNotification);
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

    // ListBoxModel
    int getNumRows() override { return static_cast<int>(filteredSamples.size()); }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height,
                          bool rowIsSelected) override
    {
        if (rowNumber < 0 || rowNumber >= static_cast<int>(filteredSamples.size()))
            return;

        auto& sample = filteredSamples[static_cast<size_t>(rowNumber)];

        if (rowIsSelected)
            g.fillAll(juce::Colour(Theme::bgHover));

        g.setColour(juce::Colour(Theme::textNormal));
        g.setFont(12.0f);
        g.drawText(sample.name, 8, 0, width - 80, height, juce::Justification::centredLeft);

        g.setColour(juce::Colour(Theme::textDim));
        g.setFont(10.0f);
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

    void resized() override
    {
        auto area = getLocalBounds().reduced(Theme::panelPadding);

        auto titleRow = area.removeFromTop(22);
        titleLabel.setBounds(titleRow);
        area.removeFromTop(4);

        auto filterRow = area.removeFromTop(24);
        categoryLabel.setBounds(filterRow.removeFromLeft(60));
        filterRow.removeFromLeft(4);
        categoryBox.setBounds(filterRow);
        area.removeFromTop(4);

        auto buttonRow = area.removeFromBottom(28);
        loadButton.setBounds(buttonRow.removeFromRight(60));
        buttonRow.removeFromRight(4);
        previewButton.setBounds(buttonRow.removeFromRight(65));

        area.removeFromBottom(4);
        listBox.setBounds(area);
    }

    // Callbacks
    std::function<void(const FactorySample&)> onLoadSample;
    std::function<void(const FactorySample&)> onPreviewSample;

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
    juce::TextButton loadButton;
    juce::TextButton previewButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBrowser)
};

} // namespace grainhex
