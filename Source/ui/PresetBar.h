#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
class PresetBar final : public juce::Component,private juce::Timer
{
public:
    explicit PresetBar(BreezeModAudioProcessor&);
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override;
    void popupMenu();
    void storePreset();
    void erasePreset();
    void randomize();
    void chooseFactory(int i);
    juce::File folder() const;
    juce::String name() const;
    BreezeModAudioProcessor& p;
    int factoryIndex=0;
    juce::TextButton prev,next,current,save,remove,shuffle;
};
