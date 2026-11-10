#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// Host-independent control mathematics. This header deliberately does not
// include JUCE so the transition and timing rules can be tested offline.
namespace breeze::dsp
{
// Block NaN/Infinity from upstream effects before persistent DSP state.
// Leave all finite audio unchanged, including levels above 0 dBFS.
inline float sanitizeAudioSample(float input) noexcept
{
    return std::isfinite(input) ? input : 0.0f;
}

inline double hostCycleBeats(int division, int noteType) noexcept
{
    division = std::clamp(division, 0, 6);
    const double straight = 4.0 / static_cast<double>(1 << division);
    return straight * (noteType == 1 ? 1.5 : noteType == 2 ? 2.0 / 3.0 : 1.0);
}

// Correlated dry and wet paths must not receive an equal-power +3 dB boost.
// Interpolation is unity when paths are identical and cannot exceed either endpoint.
inline float dryWetLinear(float dry, float wet, float mix) noexcept
{
    const float t = std::clamp(std::isfinite(mix) ? mix : 0.0f, 0.0f, 1.0f);
    return dry + t * (wet - dry);
}

// Plain Catmull-Rom/Hermite can peak at 1.25 on {-1,1,1,-1} input.
// Bound the cubic by its adjacent samples to avoid synthesized peak overloads.
// Double intermediates protect the finite-float extreme test from overflow.
inline float boundedCubicHermite(float a, float b, float c, float d, float phase) noexcept
{
    const double t = std::clamp(static_cast<double>(phase), 0.0, 1.0);
    const double aa=a, bb=b, cc=c, dd=d;
    const double c1=(cc-aa)*0.5;
    const double c2=aa-2.5*bb+2.0*cc-0.5*dd;
    const double c3=(dd-aa)*0.5+1.5*(bb-cc);
    const double v=((c3*t+c2)*t+c1)*t+bb;
    return static_cast<float>(std::clamp(v, std::min(bb,cc), std::max(bb,cc)));
}

inline float vibratoDryAtMix(float normal, float compensated, float mix) noexcept
{
    // Blend dry time alignment only when the vibrato effect is heard.
    // A MIX value of zero gives exactly the normal latency-matched dry path.
    const auto t = std::clamp(mix, 0.0f, 1.0f);
    return normal + t * (compensated - normal);
}

class ModeFader
{
public:
    void prepare(int initialMode, int crossfadeSamples) noexcept
    {
        duration = std::max(1, crossfadeSamples);
        target = std::clamp(initialMode, 0, 3);
        remaining = 0;
        weights.fill(0.0f);
        weights[static_cast<size_t>(target)] = 1.0f;
        start = weights;
    }

    void setTarget(int newMode) noexcept
    {
        newMode = std::clamp(newMode, 0, 3);
        if (newMode == target) return;
        // Re-target from the *current audible blend*, not from the old mode.
        // This is continuous even under fast or randomized host automation.
        start = weights;
        target = newMode;
        remaining = duration;
    }

    const std::array<float, 4>& advance() noexcept
    {
        if (remaining > 0)
        {
            const float t = static_cast<float>(duration - remaining + 1)
                                / static_cast<float>(duration);
            for (int i = 0; i < 4; ++i)
                weights[static_cast<size_t>(i)] =
                    start[static_cast<size_t>(i)] * (1.0f - t)
                    + (i == target ? t : 0.0f);
            --remaining;
        }
        return weights;
    }

    const std::array<float, 4>& current() const noexcept { return weights; }
    int destination() const noexcept { return target; }
    bool transitioning() const noexcept { return remaining != 0; }

private:
    std::array<float, 4> weights {0.0f, 0.0f, 0.0f, 1.0f};
    std::array<float, 4> start = weights;
    int duration = 1, remaining = 0, target = 3;
};
}
