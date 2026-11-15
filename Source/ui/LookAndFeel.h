#pragma once
#include <JuceHeader.h>
#include <SkellCore/SkullTheme.h>
class SkellFanLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    SkellFanLookAndFeel();
    void setSkin(skellcore::ui::SkullSkin);
    void drawPopupMenuBackground(juce::Graphics&,int,int) override;
    void drawCornerResizer(juce::Graphics&,int,int,bool,bool) override;
    juce::Font getPopupMenuFont() override;
private:
    skellcore::ui::SkullSkin skin=skellcore::ui::SkullSkin::Neon;
};
