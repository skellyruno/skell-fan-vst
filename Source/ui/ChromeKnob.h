#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
class ChromeKnob final : public juce::Slider
{
public:
    ChromeKnob(BreezeModAudioProcessor&, const juce::String& parameterId,
               const juce::String& caption,bool miniature=false,bool blue=false);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
private:
    juce::String name, parameterId;
    juce::String displayValue() const;
    bool small=false,isBlue=false;
    juce::Image chromeImg;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};
