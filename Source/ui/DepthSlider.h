#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
class DepthSlider final : public juce::Slider
{
public:
    explicit DepthSlider(SkellFanAudioProcessor&);
    void paint(juce::Graphics&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};
