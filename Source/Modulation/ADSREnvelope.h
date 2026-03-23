#pragma once

#include <atomic>
#include <cmath>
#include <juce_core/juce_core.h>

namespace grainhex {

/**
 * ADSR envelope generator with clean state transitions.
 * Output: 0.0 to 1.0 normalized value.
 * Designed for audio-thread use — no allocations.
 * Default route: filter cutoff modulation.
 */
class ADSREnvelope
{
public:
    enum class State
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    ADSREnvelope() = default;

    void setSampleRate(double sr) { sampleRate = sr; }

    void setEnabled(bool e) { enabled.store(e, std::memory_order_relaxed); }
    bool isEnabled() const { return enabled.load(std::memory_order_relaxed); }

    // Times in seconds
    void setAttack(float seconds)  { attack.store(juce::jlimit(0.001f, 10.0f, seconds), std::memory_order_relaxed); }
    void setDecay(float seconds)   { decay.store(juce::jlimit(0.001f, 10.0f, seconds), std::memory_order_relaxed); }
    void setSustain(float level)   { sustain.store(juce::jlimit(0.0f, 1.0f, level), std::memory_order_relaxed); }
    void setRelease(float seconds) { release.store(juce::jlimit(0.001f, 10.0f, seconds), std::memory_order_relaxed); }

    // Trigger note on
    void noteOn()
    {
        state = State::Attack;
    }

    // Trigger note off
    void noteOff()
    {
        if (state != State::Idle)
            state = State::Release;
    }

    // Advance one sample, return envelope value [0, 1]
    float tick()
    {
        if (!enabled.load(std::memory_order_relaxed))
            return 0.0f;

        switch (state)
        {
            case State::Idle:
                output = 0.0f;
                break;

            case State::Attack:
            {
                float a = attack.load(std::memory_order_relaxed);
                float rate = 1.0f / (a * static_cast<float>(sampleRate));
                output += rate;
                if (output >= 1.0f)
                {
                    output = 1.0f;
                    state = State::Decay;
                }
                break;
            }

            case State::Decay:
            {
                float d = decay.load(std::memory_order_relaxed);
                float s = sustain.load(std::memory_order_relaxed);
                float rate = 1.0f / (d * static_cast<float>(sampleRate));
                output -= rate;
                if (output <= s)
                {
                    output = s;
                    state = State::Sustain;
                }
                break;
            }

            case State::Sustain:
                output = sustain.load(std::memory_order_relaxed);
                break;

            case State::Release:
            {
                float r = release.load(std::memory_order_relaxed);
                float rate = 1.0f / (r * static_cast<float>(sampleRate));
                output -= rate;
                if (output <= 0.0f)
                {
                    output = 0.0f;
                    state = State::Idle;
                }
                break;
            }
        }

        return output;
    }

    State getState() const { return state; }
    float getCurrentValue() const { return output; }

private:
    std::atomic<bool> enabled { false };
    std::atomic<float> attack  { 0.01f };   // 10ms default
    std::atomic<float> decay   { 0.1f };    // 100ms default
    std::atomic<float> sustain { 0.7f };    // 70% default
    std::atomic<float> release { 0.3f };    // 300ms default

    State state = State::Idle;
    float output = 0.0f;
    double sampleRate = 44100.0;
};

} // namespace grainhex
