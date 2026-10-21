#pragma once
#include <JuceHeader.h>
class KeyButton final : public juce::Button
{
public:
    KeyButton(const juce::String& title,juce::Colour key,bool small=false);
    void paintButton(juce::Graphics&,bool hover,bool down) override;
private:
    juce::Colour tint;
    bool isSmall=false;
};
