#pragma once
#include <JuceHeader.h>
class BreezeLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    BreezeLookAndFeel();
    void drawPopupMenuBackground(juce::Graphics&,int,int) override;
};
