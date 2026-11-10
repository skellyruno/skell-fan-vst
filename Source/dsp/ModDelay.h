#pragma once
#include <JuceHeader.h>
#include <cmath>
#include "ModulationMath.h"

namespace breeze::dsp
{
class ModDelay
{
public:
    void prepare(double sampleRate, double maxMilliseconds = 90.0)
    {
        size = juce::jmax(16, static_cast<int>(std::ceil(sampleRate * maxMilliseconds * 0.001)) + 8);
        buffer.setSize(2, size, false, true, true);
        buffer.clear(); head = 0;
    }
    void clear() noexcept { buffer.clear(); head = 0; }
    void write(int channel, float s) noexcept { buffer.setSample(channel, head, s); }
    void advance() noexcept { if (++head >= size) head = 0; }
    float read(int channel, double delaySamples) const noexcept
    {
        // Exact zero-delay reads must return the current write. This matters
        // if a JUCE oversampling filter reports zero algorithmic latency.
        if (delaySamples <= 0.0) return buffer.getSample(channel, head);
        const double p = wrap(static_cast<double>(head) - juce::jlimit(1.0, static_cast<double>(size-4), delaySamples));
        const int i = static_cast<int>(p);
        const float f = static_cast<float>(p - i);
        const auto get = [this, channel](int j) noexcept -> float
        {
            while (j < 0) j += size;
            while (j >= size) j -= size;
            return buffer.getSample(channel, j);
        };
        const float a = get(i-1), b = get(i), c = get(i+1), d = get(i+2);
        return boundedCubicHermite(a,b,c,d,f);
    }
private:
    double wrap(double x) const noexcept
    {
        while (x < 0.0) x += static_cast<double>(size);
        while (x >= static_cast<double>(size)) x -= static_cast<double>(size);
        return x;
    }
    juce::AudioBuffer<float> buffer;
    int size = 16, head = 0;
};
}
