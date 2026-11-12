#include "LookAndFeel.h"
#include "Theme.h"
SkellFanLookAndFeel::SkellFanLookAndFeel()
{
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff0e1711));
    setColour(juce::PopupMenu::textColourId,juce::Colours::white);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,Theme::deepBlue);
    setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xff142119));
    setColour(juce::TextEditor::textColourId,juce::Colours::white);
}
void SkellFanLookAndFeel::drawPopupMenuBackground(juce::Graphics& g,int w,int h)
{
    const juce::Rectangle<float> area{0.0f,0.0f,static_cast<float>(w),
                                      static_cast<float>(h)};
    juce::ColourGradient panel(juce::Colour(0xff1d2b21),area.getTopLeft(),
                               juce::Colour(0xff080d0a),area.getBottomRight(),false);
    g.setGradientFill(panel);
    g.fillRect(area);
    g.setColour(juce::Colour(0xff426349).withAlpha(0.8f));
    g.drawRect(0,0,w,h,1);
}
