#pragma once
#include <JuceHeader.h>
#include <array>
#include "../PluginProcessor.h"

// Compact vector panels occupy the unused space beside the fan's base.
// Their audio telemetry is carried only through lock-free atomic floats.
class LiveMeter final : public juce::Component
{
public:
    explicit LiveMeter(SkellFanAudioProcessor& owner) : processor(owner)
    { setInterceptsMouseClicks(false, false); }
    void refresh() noexcept;
    void paint(juce::Graphics&) override;
private:
    SkellFanAudioProcessor& processor;
    std::array<float, 2> rms {{0.0f, 0.0f}};
    std::array<float, 2> peaks {{0.0f, 0.0f}};
    std::array<int, 2> clipHold {{0, 0}};
    uint64_t lastFrame=0;
};

class ModeInfo final : public juce::Component
{
public:
    explicit ModeInfo(SkellFanAudioProcessor& owner) : processor(owner)
    { setInterceptsMouseClicks(false, false); }
    void paint(juce::Graphics&) override;
    void refresh();
private:
    SkellFanAudioProcessor& processor;
    int lastMode=-1, lastWave=-1, lastSync=-1, lastPower=-1;
    float lastRate=-1.0f;
};
