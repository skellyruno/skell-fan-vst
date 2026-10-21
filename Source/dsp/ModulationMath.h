#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// Host-independent control mathematics. This header deliberately does not
// include JUCE so the transition and timing rules can be tested offline.
namespace breeze::dsp
{
inline double hostCycleBeats(int division, int noteType) noexcept
{
    division = std::clamp(division, 0, 6);
    const double straight = 4.0 / static_cast<double>(1 << division);
    return straight * (noteType == 1 ? 1.5 : noteType == 2 ? 2.0 / 3.0 : 1.0);
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
