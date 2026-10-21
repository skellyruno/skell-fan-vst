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
    BreezeModAudioProcessor& p;
};
