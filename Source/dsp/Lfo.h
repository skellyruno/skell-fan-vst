#pragma once
#include <JuceHeader.h>
#include <cmath>

namespace breeze::dsp
{
class Lfo
{
public:
    void prepare(double sampleRate)
    {
        sr = juce::jmax(1.0, sampleRate);
        rate.reset(sr, 0.35);
        rate.setCurrentAndTargetValue(3.0);
        phase = 0.0;
        correctionRemaining=0;
        correctionStep=0.0;
    }
    // The musical host clock must not be slewed: otherwise the LFO slowly drifts
    // away from beats even though it has a correct PPQ starting phase.
    void setFrequency(double hz, bool hostSynced) noexcept
    {
        const double f = juce::jlimit(0.0, 256.0, std::isfinite(hz) ? hz : 0.0);
        if (hostSynced) rate.setCurrentAndTargetValue(f);
        else rate.setTargetValue(f);
    }
    void lockToPPQ(double beatPosition, double beatsPerCycle,
                   bool smoothDivisionChange=false) noexcept
    {
        if (!std::isfinite(beatPosition) || !std::isfinite(beatsPerCycle)
            || beatsPerCycle<=0.0) return;
        const double p=beatPosition/beatsPerCycle;
        const double target=p-std::floor(p);
        if (!smoothDivisionChange)
        {
            phase=target;
            correctionRemaining=0;
            correctionStep=0.0;
            return;
        }
        // Only host note-division changes glide phase; transport seeks and
        // playback starts still hard-lock. Shortest circular distance avoids
        // forcing a nearly full-cycle phase sweep during automation.
        double difference=target-phase;
        difference-=std::floor(difference+0.5);
        correctionRemaining=juce::jmax(1,juce::roundToInt(sr*0.010));
        correctionStep=difference/static_cast<double>(correctionRemaining);
    }
    double getPhase() const noexcept { return phase; }
    double getCurrentFrequency() const noexcept { return rate.getCurrentValue(); }
    double tick() noexcept
    {
        const double frequency = rate.getNextValue();
        phase += frequency / sr;
        if (correctionRemaining>0)
        {
            phase += correctionStep;
            --correctionRemaining;
        }
        phase -= std::floor(phase);
        return phase;
    }
    static float sample(double p, int shape, double phaseStep = 0.0) noexcept
    {
        p -= std::floor(p);
        switch (shape)
        {
            case 1: return static_cast<float>(1.0 - 4.0 * std::abs(p - 0.5));
            case 2:
            {
                const double dt = juce::jlimit(0.0000001, 0.49, phaseStep);
                return static_cast<float>((p < 0.5 ? 1.0 : -1.0)
                    + polyBlep(p, dt) - polyBlep(std::fmod(p + 0.5, 1.0), dt));
            }
            case 3:
            {
                const double dt = juce::jlimit(0.0000001, 0.49, phaseStep);
                return static_cast<float>(2.0 * p - 1.0 - polyBlep(p, dt));
            }
            default: return static_cast<float>(std::sin(juce::MathConstants<double>::twoPi * p));
        }
    }
private:
    static double polyBlep(double x, double dt) noexcept
    {
        if (x < dt) { x /= dt; return x + x - x * x - 1.0; }
        if (x > 1.0 - dt) { x = (x - 1.0) / dt; return x * x + x + x + 1.0; }
        return 0.0;
    }
    double sr = 44100.0, phase = 0.0, correctionStep = 0.0;
    int correctionRemaining = 0;
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> rate;
};
}
