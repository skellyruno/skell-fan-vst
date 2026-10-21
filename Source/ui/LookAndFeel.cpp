#include "LookAndFeel.h"
#include "Theme.h"
BreezeLookAndFeel::BreezeLookAndFeel()
{
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff191b1f));
    setColour(juce::PopupMenu::textColourId,juce::Colours::white);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,Theme::deepBlue);
    setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xff272a30));
    setColour(juce::TextEditor::textColourId,juce::Colours::white);
}
void BreezeLookAndFeel::drawPopupMenuBackground(juce::Graphics& g,int w,int h)
{
    g.fillAll(juce::Colour(0xff1a1b1f));
    g.setColour(Theme::chrome);g.drawRect(0,0,w,h,1);
}
