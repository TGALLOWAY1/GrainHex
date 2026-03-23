#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <vector>
#include <cmath>

namespace grainhex {

/**
 * Factory sample generator — creates synthesized bass samples
 * at runtime for the internal browser. No external files needed.
 *
 * Categories: reeses, growls, FM basses, noise textures, vocal-style.
 */
struct FactorySample
{
    juce::String name;
    juce::String category;
    int rootMidiNote = 36; // C2
    std::shared_ptr<juce::AudioBuffer<float>> buffer;
    double sampleRate = 44100.0;
};

class FactorySampleGenerator
{
public:
    static std::vector<FactorySample> generateAll()
    {
        std::vector<FactorySample> samples;
        constexpr double sr = 44100.0;

        // === REESES (detuned saw pairs) ===
        samples.push_back(makeSample("Classic Reese", "Reeses", 36, sr, 2.0,
            [](double phase, double t, double freq) {
                double saw1 = 2.0 * std::fmod(phase, 1.0) - 1.0;
                double phase2 = phase * 1.007; // slight detune
                double saw2 = 2.0 * std::fmod(phase2, 1.0) - 1.0;
                return (saw1 + saw2) * 0.4 * envelope(t, 2.0);
            }));

        samples.push_back(makeSample("Wide Reese", "Reeses", 36, sr, 2.0,
            [](double phase, double t, double freq) {
                double saw1 = 2.0 * std::fmod(phase, 1.0) - 1.0;
                double saw2 = 2.0 * std::fmod(phase * 1.015, 1.0) - 1.0;
                double saw3 = 2.0 * std::fmod(phase * 0.993, 1.0) - 1.0;
                return (saw1 + saw2 + saw3) * 0.3 * envelope(t, 2.0);
            }));

        samples.push_back(makeSample("Dark Reese", "Reeses", 24, sr, 2.5,
            [](double phase, double t, double freq) {
                double saw1 = 2.0 * std::fmod(phase, 1.0) - 1.0;
                double saw2 = 2.0 * std::fmod(phase * 1.005, 1.0) - 1.0;
                double lp = softClip((saw1 + saw2) * 0.5);
                return lp * 0.5 * envelope(t, 2.5);
            }));

        // === GROWLS (waveshaped/modulated) ===
        samples.push_back(makeSample("Basic Growl", "Growls", 36, sr, 1.5,
            [](double phase, double t, double freq) {
                double saw = 2.0 * std::fmod(phase, 1.0) - 1.0;
                double mod = std::sin(2.0 * pi() * 5.0 * t);
                double shaped = std::tanh(saw * (2.0 + mod * 1.5));
                return shaped * 0.35 * envelope(t, 1.5);
            }));

        samples.push_back(makeSample("Neuro Growl", "Growls", 36, sr, 2.0,
            [](double phase, double t, double freq) {
                double saw = 2.0 * std::fmod(phase, 1.0) - 1.0;
                double mod = std::sin(2.0 * pi() * 3.7 * t) * std::sin(2.0 * pi() * 0.5 * t);
                double shaped = wavefold(saw * (1.5 + mod * 2.0));
                return shaped * 0.35 * envelope(t, 2.0);
            }));

        samples.push_back(makeSample("Screech", "Growls", 48, sr, 1.5,
            [](double phase, double t, double freq) {
                double saw = 2.0 * std::fmod(phase, 1.0) - 1.0;
                double modFreq = 7.0 + 5.0 * std::sin(2.0 * pi() * 0.8 * t);
                double mod = std::sin(2.0 * pi() * modFreq * t);
                double shaped = std::tanh(saw * (3.0 + mod * 2.5));
                return shaped * 0.3 * envelope(t, 1.5);
            }));

        // === FM BASSES ===
        samples.push_back(makeSample("FM Sub", "FM Basses", 36, sr, 2.0,
            [](double phase, double t, double freq) {
                double modIndex = 2.0 * (1.0 - t / 2.0);
                double modulator = std::sin(2.0 * pi() * freq * 2.0 * t) * modIndex;
                double carrier = std::sin(2.0 * pi() * phase + modulator);
                return carrier * 0.45 * envelope(t, 2.0);
            }));

        samples.push_back(makeSample("FM Growl", "FM Basses", 36, sr, 2.0,
            [](double phase, double t, double freq) {
                double lfo = std::sin(2.0 * pi() * 4.0 * t);
                double modIndex = 3.0 + lfo * 1.5;
                double modulator = std::sin(2.0 * pi() * freq * 3.0 * t) * modIndex;
                double carrier = std::sin(2.0 * pi() * phase + modulator);
                return carrier * 0.35 * envelope(t, 2.0);
            }));

        samples.push_back(makeSample("Metallic FM", "FM Basses", 48, sr, 1.5,
            [](double phase, double t, double freq) {
                double modIndex = 5.0 * envelope(t, 1.5);
                double mod1 = std::sin(2.0 * pi() * freq * 1.414 * t) * modIndex;
                double mod2 = std::sin(2.0 * pi() * freq * 2.76 * t) * modIndex * 0.5;
                double carrier = std::sin(2.0 * pi() * phase + mod1 + mod2);
                return carrier * 0.3 * envelope(t, 1.5);
            }));

        // === NOISE TEXTURES ===
        samples.push_back(makeSample("Filtered Noise", "Noise", 36, sr, 2.0,
            [](double phase, double t, double freq) {
                // Simple noise with amplitude modulation
                double noise = (std::fmod(phase * 13.7 + 0.5, 1.0) * 2.0 - 1.0);
                double tonalMod = std::sin(2.0 * pi() * phase) * 0.6;
                return (noise * 0.3 + tonalMod) * 0.35 * envelope(t, 2.0);
            }));

        samples.push_back(makeSample("Gritty Texture", "Noise", 36, sr, 2.0,
            [](double phase, double t, double freq) {
                double saw = 2.0 * std::fmod(phase, 1.0) - 1.0;
                double noise = std::sin(phase * 137.0) * std::cos(phase * 251.0);
                return (saw * 0.5 + noise * 0.3) * 0.35 * envelope(t, 2.0);
            }));

        samples.push_back(makeSample("White Wash", "Noise", 60, sr, 2.0,
            [](double phase, double t, double freq) {
                double n1 = std::sin(phase * 97.3) * std::cos(phase * 151.7);
                double n2 = std::sin(phase * 53.1 + 0.3);
                double tone = std::sin(2.0 * pi() * phase) * 0.2;
                return (n1 * 0.3 + n2 * 0.2 + tone) * envelope(t, 2.0);
            }));

        // === VOCAL-STYLE (formant synthesis) ===
        samples.push_back(makeSample("Vowel A", "Vocal", 36, sr, 2.0,
            [](double phase, double t, double freq) {
                // Formants for "ah": ~730, 1090, 2440 Hz
                double carrier = pulseWave(phase, 0.3 + 0.1 * std::sin(2.0 * pi() * 0.5 * t));
                double f1 = resonator(carrier, freq, 730.0, 80.0);
                double f2 = resonator(carrier, freq, 1090.0, 90.0);
                double f3 = resonator(carrier, freq, 2440.0, 120.0);
                return (f1 + f2 * 0.6 + f3 * 0.3) * 0.35 * envelope(t, 2.0);
            }));

        samples.push_back(makeSample("Vowel E", "Vocal", 36, sr, 2.0,
            [](double phase, double t, double freq) {
                double carrier = pulseWave(phase, 0.4);
                double f1 = resonator(carrier, freq, 530.0, 60.0);
                double f2 = resonator(carrier, freq, 1840.0, 100.0);
                double f3 = resonator(carrier, freq, 2480.0, 120.0);
                return (f1 + f2 * 0.5 + f3 * 0.25) * 0.35 * envelope(t, 2.0);
            }));

        samples.push_back(makeSample("Vowel O", "Vocal", 36, sr, 2.0,
            [](double phase, double t, double freq) {
                double carrier = pulseWave(phase, 0.35 + 0.05 * std::sin(2.0 * pi() * 0.3 * t));
                double f1 = resonator(carrier, freq, 570.0, 70.0);
                double f2 = resonator(carrier, freq, 840.0, 80.0);
                double f3 = resonator(carrier, freq, 2410.0, 110.0);
                return (f1 + f2 * 0.7 + f3 * 0.2) * 0.35 * envelope(t, 2.0);
            }));

        // === ADDITIONAL BASSES ===
        samples.push_back(makeSample("808 Sub", "Reeses", 36, sr, 3.0,
            [](double phase, double t, double /*freq*/) {
                double pitchEnv = std::exp(-t * 8.0) * 0.5 + 1.0;
                double pitched = std::sin(2.0 * pi() * phase * pitchEnv);
                return pitched * 0.5 * envelope808(t, 3.0);
            }));

        samples.push_back(makeSample("Square Bass", "FM Basses", 36, sr, 2.0,
            [](double phase, double t, double /*freq*/) {
                double sq = (std::fmod(phase, 1.0) < 0.5) ? 1.0 : -1.0;
                double sq2 = (std::fmod(phase * 2.0, 1.0) < 0.5) ? 1.0 : -1.0;
                return (sq * 0.4 + sq2 * 0.15) * envelope(t, 2.0);
            }));

        samples.push_back(makeSample("Distorted Sub", "Growls", 36, sr, 2.0,
            [](double phase, double t, double /*freq*/) {
                double sine = std::sin(2.0 * pi() * phase);
                double dist = std::tanh(sine * 4.0);
                return dist * 0.35 * envelope(t, 2.0);
            }));

        return samples;
    }

private:
    static constexpr double pi() { return juce::MathConstants<double>::pi; }

    static double envelope(double t, double duration)
    {
        double attack = 0.005;
        double release = 0.1;
        if (t < attack) return t / attack;
        if (t > duration - release) return (duration - t) / release;
        return 1.0;
    }

    static double envelope808(double t, double duration)
    {
        double attack = 0.002;
        double decay = 2.5;
        if (t < attack) return t / attack;
        return std::exp(-t / decay) * ((t < duration) ? 1.0 : 0.0);
    }

    static double softClip(double x)
    {
        return std::tanh(x);
    }

    static double wavefold(double x)
    {
        // Triangle wavefolder
        x = std::fmod(x + 1.0, 4.0);
        if (x < 0.0) x += 4.0;
        if (x < 2.0) return x - 1.0;
        return 3.0 - x;
    }

    static double pulseWave(double phase, double duty)
    {
        return (std::fmod(phase, 1.0) < duty) ? 1.0 : -1.0;
    }

    static double resonator(double input, double freq, double formantFreq, double bandwidth)
    {
        // Simple formant approximation using sine modulation
        double ratio = formantFreq / std::max(freq, 20.0);
        double emphasis = std::exp(-0.5 * std::pow((ratio - std::round(ratio)) * bandwidth / formantFreq, 2.0));
        return input * emphasis;
    }

    template<typename SynthFunc>
    static FactorySample makeSample(const juce::String& name, const juce::String& category,
                                     int rootNote, double sampleRate, double duration,
                                     SynthFunc synthFunc)
    {
        int numSamples = static_cast<int>(sampleRate * duration);
        auto buffer = std::make_shared<juce::AudioBuffer<float>>(2, numSamples);

        double freq = 440.0 * std::pow(2.0, (rootNote - 69.0) / 12.0);
        double phase = 0.0;
        double phaseIncrement = freq / sampleRate;

        for (int i = 0; i < numSamples; ++i)
        {
            double t = static_cast<double>(i) / sampleRate;
            float sample = static_cast<float>(synthFunc(phase, t, freq));

            // Soft-limit output
            sample = static_cast<float>(std::tanh(static_cast<double>(sample)));

            buffer->setSample(0, i, sample);
            buffer->setSample(1, i, sample);

            phase += phaseIncrement;
        }

        return { name, category, rootNote, buffer, sampleRate };
    }
};

} // namespace grainhex
