#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <memory>

namespace grainhex {

/**
 * Exports audio buffers to WAV files.
 * Supports 16-bit, 24-bit, and 32-bit float output.
 */
class WavExporter
{
public:
    enum class BitDepth
    {
        Bit16 = 16,
        Bit24 = 24,
        Float32 = 32
    };

    /**
     * Export an audio buffer to a WAV file.
     * @param buffer   The audio data to export
     * @param sampleRate  The sample rate of the audio
     * @param file     The destination file path
     * @param bitDepth The output bit depth
     * @return true on success, false on failure
     */
    static bool exportToWav(const juce::AudioBuffer<float>& buffer,
                            double sampleRate,
                            const juce::File& file,
                            BitDepth bitDepth = BitDepth::Bit24)
    {
        juce::WavAudioFormat wavFormat;

        auto outputStream = file.createOutputStream();
        if (outputStream == nullptr)
            return false;

        int bits = static_cast<int>(bitDepth);
        auto* writer = wavFormat.createWriterFor(outputStream.release(),
                                                  sampleRate,
                                                  static_cast<unsigned int>(buffer.getNumChannels()),
                                                  bits,
                                                  {},
                                                  0);
        if (writer == nullptr)
            return false;

        std::unique_ptr<juce::AudioFormatWriter> writerPtr(writer);
        return writerPtr->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
    }

    /**
     * Create a temporary WAV file for drag-and-drop operations.
     * Returns the temp file path, or an empty File on failure.
     */
    static juce::File exportToTempWav(const juce::AudioBuffer<float>& buffer,
                                       double sampleRate,
                                       const juce::String& baseName = "GrainHex_resample")
    {
        auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
        auto tempFile = tempDir.getChildFile(baseName + "_" + juce::String(juce::Time::currentTimeMillis()) + ".wav");

        if (exportToWav(buffer, sampleRate, tempFile, BitDepth::Bit24))
            return tempFile;

        return {};
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavExporter)
};

} // namespace grainhex
