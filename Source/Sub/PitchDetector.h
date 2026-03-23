#pragma once

#include "Audio/AudioTypes.h"
#include <array>

namespace grainhex {

/**
 * Real-time pitch detector using YIN algorithm on a rolling ring buffer.
 * Designed for audio-thread use — all buffers preallocated, no dynamic allocation.
 * Analyzes granular output to determine sub oscillator target frequency.
 *
 * Key behaviors:
 * - ~50ms analysis window at 48kHz (~2400 samples, using 2048 for power-of-2)
 * - Focus on bass frequencies (20-500 Hz)
 * - Hold-last-note when confidence is low (avoids pitch flapping)
 * - Feed samples from audio thread, detect pitch per block
 */
class PitchDetector
{
public:
    // Buffer size: 4096 samples accommodates ~85ms at 48kHz
    // YIN needs 2x the period we want to detect. At 20Hz, period = 2400 samples at 48kHz.
    static constexpr int kBufferSize = 4096;
    static constexpr int kYinWindowSize = 2048;
    static constexpr float kYinThreshold = 0.20f; // Slightly more permissive than offline
    static constexpr float kMinFrequency = 20.0f;
    static constexpr float kMaxFrequency = 500.0f;
    static constexpr float kConfidenceThreshold = 0.5f; // Below this, hold last note

    PitchDetector();

    void setSampleRate(double sampleRate);

    // Feed audio samples into the ring buffer (called from audio thread)
    void feedSamples(const float* data, int numSamples);

    // Run pitch detection on current buffer contents (called from audio thread)
    PitchInfo detect();

    // Get the last stable detected pitch (holds note when confidence is low)
    PitchInfo getLastStablePitch() const { return lastStablePitch; }

private:
    float yinDetect(float& outConfidence);

    // Ring buffer for audio samples
    std::array<float, kBufferSize> ringBuffer {};
    int writePos = 0;

    // YIN working buffers (preallocated)
    std::array<float, kYinWindowSize / 2> diffBuffer {};
    std::array<float, kYinWindowSize / 2> cmndfBuffer {};

    // Analysis state
    double sampleRate = 44100.0;
    PitchInfo lastStablePitch {};
    int samplesAccumulated = 0;
};

} // namespace grainhex
