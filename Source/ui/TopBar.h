#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "ChromeKnob.h"
class PowerToggle final : public juce::ToggleButton
{
public:
    void paintButton(juce::Graphics&,bool,bool) override;
};
class ScopeToggle final : public juce::TextButton
{
public:
    void paintButton(juce::Graphics&,bool,bool) override;
};
class TopBar final : public juce::Component
{
public:
    explicit TopBar(BreezeModAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
    std::function<void()> onScope;
private:
    PowerToggle power;
    ScopeToggle scope;
    ChromeKnob width,mix;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttach;
};
