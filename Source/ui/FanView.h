#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "Theme.h"

class FanView final : public juce::Component, private juce::Timer
{
public:
    explicit FanView(SkellFanAudioProcessor&);
    void paint(juce::Graphics&) override;
    void setAnimationEnabled(bool b);
private:
    void timerCallback() override;
    SkellFanAudioProcessor& processor;
    juce::Image body,rotor,blur,grille,hub;
    bool enabled=true, lastPowerOn=true;
    double lastTime=0.0,angle=0.0,speed=0.0;
};
