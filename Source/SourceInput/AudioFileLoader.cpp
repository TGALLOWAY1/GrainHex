#include "SourceInput/AudioFileLoader.h"

namespace grainhex {

AudioFileLoader::AudioFileLoader()
{
    formatManager.registerBasicFormats(); // WAV, AIFF, FLAC, etc.
}

AudioFileLoader::LoadResult AudioFileLoader::loadFile(const juce::File& file)
{
    LoadResult result;

    if (!file.existsAsFile())
    {
        result.errorMessage = "File does not exist: " + file.getFullPathName();
        return result;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
    {
        result.errorMessage = "Unsupported format: " + file.getFileName();
        return result;
    }

    // Capture metadata
    result.metadata.filename = file.getFileName();
    result.metadata.originalSampleRate = reader->sampleRate;
    result.metadata.numChannels = static_cast<int>(reader->numChannels);
    result.metadata.numSamples = static_cast<int64_t>(reader->lengthInSamples);
    result.metadata.lengthSeconds = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;

    // Decode to stereo float buffer
    int outputChannels = juce::jmax(static_cast<int>(reader->numChannels), 2);
    // Cap at stereo for Phase 1
    outputChannels = juce::jmin(outputChannels, 2);

    auto buffer = std::make_shared<juce::AudioBuffer<float>>(outputChannels,
                                                               static_cast<int>(reader->lengthInSamples));

    reader->read(buffer.get(), 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

    // If mono source, copy channel 0 to channel 1 for stereo output
    if (reader->numChannels == 1 && outputChannels == 2)
    {
        buffer->copyFrom(1, 0, *buffer, 0, 0, buffer->getNumSamples());
    }

    result.buffer = buffer;
    result.success = true;
    return result;
}

bool AudioFileLoader::isFormatSupported(const juce::File& file) const
{
    auto extension = file.getFileExtension().toLowerCase();
    return extension == ".wav" || extension == ".aiff" || extension == ".aif"
        || extension == ".flac";
}

juce::String AudioFileLoader::getSupportedFormatsWildcard() const
{
    return "*.wav;*.aiff;*.aif;*.flac";
}

} // namespace grainhex
