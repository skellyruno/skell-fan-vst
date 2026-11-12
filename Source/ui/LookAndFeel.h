#pragma once
#include <JuceHeader.h>
class SkellFanLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    SkellFanLookAndFeel();
    void drawPopupMenuBackground(juce::Graphics&,int,int) override;
};
