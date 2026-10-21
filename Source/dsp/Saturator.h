#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <memory>
#include <vector>

namespace breeze::dsp
{
// Constant-latency oversampled soft clipper. An amount of zero is truly linear;
// keeping the oversampling filters active avoids a latency jump during automation.
class Saturator
{
public:
    void prepare(double sampleRate, int blockSize, float initialAmount)
    {
        maxBlock = juce::jmax(16, blockSize);
        dcBlend.resize(static_cast<size_t>(maxBlock), 0.0f);
        oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
            2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        oversampling->setUsingIntegerLatency(true);
        oversampling->initProcessing(static_cast<size_t>(maxBlock));
        oversampling->reset();
        latency = juce::roundToInt(oversampling->getLatencyInSamples());
        amount.reset(sampleRate * 2.0, 0.030);
        amount.setCurrentAndTargetValue(juce::jlimit(0.0f, 1.0f, initialAmount));
        dcR = std::exp(-juce::MathConstants<float>::twoPi * 12.0f
                         / static_cast<float>(sampleRate));
        for (int ch = 0; ch < 2; ++ch) x0[ch] = y0[ch] = 0.0f;
    }

    int getLatency() const noexcept { return latency; }

    void process(juce::AudioBuffer<float>& input, int start, int count, float targetAmount)
    {
        if (!oversampling || count <= 0) return;
        jassert(count <= maxBlock);
        amount.setTargetValue(juce::jlimit(0.0f, 1.0f, targetAmount));
        auto block = juce::dsp::AudioBlock<float>(input)
                        .getSubBlock(static_cast<size_t>(start), static_cast<size_t>(count));
        auto hi = oversampling->processSamplesUp(block);
        const auto samples = hi.getNumSamples();
        // One smoothed drive value per oversampled frame, shared by both channels.
        for (size_t n = 0; n < samples; ++n)
        {
            const float a = amount.getNextValue();
            // The output DC block follows the *per-sample* automated amount.
            // Using a single end-of-block value made saturation sound different
            // across DAW buffer sizes and caused discontinuities on quick moves.
            dcBlend[n / 2] = a;
            if (a <= 0.000001f) continue; // exact neutral drive, skip nonlinear CPU work
            const float drive = 1.0f + a * 5.0f;
            const float ref = 0.5f;
            const float norm = ref / std::tanh(ref * drive);
            for (size_t ch = 0; ch < hi.getNumChannels(); ++ch)
            {
                float& x = hi.getChannelPointer(ch)[n];
                // Crossfade from mathematically exact unity to the soft clipper.
                const float clipped = std::tanh(x * drive) * norm;
                x += a * (clipped - x);
            }
        }
        oversampling->processSamplesDown(block);
        // DC rejection is also blended by the smoothed, sample-accurate amount,
        // not permanently inserted into the neutral effect path.
        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto* data = block.getChannelPointer(ch);
            for (int i = 0; i < count; ++i)
            {
                const float x = data[i];
                const float y = x - x0[ch] + dcR * y0[ch];
                x0[ch] = x;
                y0[ch] = y;
                data[i] = x + dcBlend[static_cast<size_t>(i)] * (y - x);
            }
        }
    }

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> amount;
    int maxBlock = 512, latency = 0;
    std::vector<float> dcBlend; // allocated in prepareToPlay, never processBlock
    float dcR = 0.998f, x0[2]{}, y0[2]{};
};
}
