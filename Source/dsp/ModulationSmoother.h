#pragma once
#include <algorithm>
#include <cmath>

namespace breeze::dsp
{
// One-pole motion control for delay times and modulation gains. A smooth
// waveform stays largely unchanged, while abrupt square/saw edges no longer
// jump a fractional-delay read head or amplitude from one sample to the next.
// All transcendental math happens at prepare time, outside processBlock.
class ModulationSmoother
{
public:
    void prepare(double sampleRate, double milliseconds, float startingValue = 0.0f) noexcept
    {
        const double samples = std::max(1.0, sampleRate * std::max(0.01, milliseconds) * 0.001);
        step = static_cast<float>(1.0 - std::exp(-1.0 / samples));
        reset(startingValue);
    }

    void reset(float value) noexcept
    {
        state = std::isfinite(value) ? value : 0.0f;
    }

    float tick(float target) noexcept
    {
        if (!std::isfinite(target)) target = state;
        state += step * (target - state);
        return state;
    }

    float value() const noexcept { return state; }

private:
    float step = 1.0f;
    float state = 0.0f;
};
}
