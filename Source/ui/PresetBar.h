#pragma once
#include <JuceHeader.h>
#include <array>
#include "../PluginProcessor.h"
class PresetBar final : public juce::Component,private juce::Timer
{
public:
    explicit PresetBar(SkellFanAudioProcessor&);
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override;
    void popupMenu();
    void storePreset();
    void erasePreset();
    void randomize();
    void chooseFactory(int i);
    void capturePresetSnapshot();
    bool isModified() const;
    juce::File folder() const;
    juce::File legacyFolder() const;
    juce::String name() const;
    SkellFanAudioProcessor& p;
    int factoryIndex=0;
    juce::String snapshotName;
    std::array<float, 10> snapshotValues {};
    juce::TextButton prev,next,current,save,remove,shuffle;
};
