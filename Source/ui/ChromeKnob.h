#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
class ChromeKnob final : public juce::Slider, private juce::Timer
{
public:
    ChromeKnob(SkellFanAudioProcessor&, const juce::String& parameterId,
               const juce::String& caption,bool miniature=false,bool blue=false);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails&) override;
    bool keyPressed(const juce::KeyPress&) override;
private:
    juce::String name, parameterId;
    juce::String displayValue() const;
    void beginValueFeedback();
    void timerCallback() override;
    bool small=false,isBlue=false,showValue=false;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};
