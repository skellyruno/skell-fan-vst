#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
class LfoScope final : public juce::Component,private juce::Timer
{
public:
    explicit LfoScope(BreezeModAudioProcessor&);
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    void visibilityChanged() override
    { if (isVisible()) startTimerHz(30); else stopTimer(); }
    BreezeModAudioProcessor& p;
};
