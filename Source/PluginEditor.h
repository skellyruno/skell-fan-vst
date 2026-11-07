#pragma once
#include <JuceHeader.h>
#include <array>
#include "PluginProcessor.h"
#include "ui/LookAndFeel.h"
#include "ui/FanView.h"
#include "ui/TopBar.h"
#include "ui/PresetBar.h"
#include "ui/DepthSlider.h"
#include "ui/ChromeKnob.h"
#include "ui/KeyButton.h"
#include "ui/LfoScope.h"
#include "ui/StatusPanels.h"

class BreezeModAudioProcessorEditor final : public juce::AudioProcessorEditor,private juce::Timer
{
public:
    explicit BreezeModAudioProcessorEditor(BreezeModAudioProcessor&);
    ~BreezeModAudioProcessorEditor() override;
    void resized() override;
private:
    class Stage final : public juce::Component
    {
    public:
        Stage(){setInterceptsMouseClicks(false,false);setBufferedToImage(true);}
        void paint(juce::Graphics&) override;
    } stage;
    class PlateOverlay final : public juce::Component
    {
    public:
        PlateOverlay(){setInterceptsMouseClicks(false,false);setBufferedToImage(true);}
        void paint(juce::Graphics&) override;
    } plate;
    class RateReadout final : public juce::Component
    {
    public:
        explicit RateReadout(BreezeModAudioProcessor& owner) : processor(owner)
        { setInterceptsMouseClicks(false,false); }
        void paint(juce::Graphics&) override;
    private:
        BreezeModAudioProcessor& processor;
    };
    void timerCallback() override;
    void setChoice(juce::ComboBox& choice,int i);
    BreezeModAudioProcessor& processor;
    BreezeLookAndFeel theme;
    FanView fan;
    TopBar top;
    PresetBar presets;
    DepthSlider depth;
    ChromeKnob rate,syncRate,wave,saturation;
    juce::TextButton fanSwitch;
    LfoScope scope;
    RateReadout rateReadout;
    LiveMeter outputMeter;
    ModeInfo modeInfo;
    std::array<std::unique_ptr<KeyButton>,4> modeKeys;
    std::array<std::unique_ptr<KeyButton>,2> syncKeys;
    std::array<std::unique_ptr<KeyButton>,3> noteKeys;
    juce::ComboBox modeChoice,syncChoice,noteChoice;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttach,syncAttach,noteAttach;
    juce::ComponentBoundsConstrainer constrainer;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BreezeModAudioProcessorEditor)
};
