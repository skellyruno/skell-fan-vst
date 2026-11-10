#include "KeyButton.h"
#include "Theme.h"
KeyButton::KeyButton(const juce::String& name,juce::Colour c,bool small)
 : juce::Button(name),tint(c),isSmall(small)
{
    setClickingTogglesState(false);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);
}
void KeyButton::paintButton(juce::Graphics& g,bool hover,bool down)
{
    auto r=getLocalBounds().toFloat().reduced(isSmall?1.0f:2.0f);
    const bool lit=getToggleState();
    g.setColour(juce::Colours::black.withAlpha(.3f));
    g.fillRoundedRectangle(r.translated(1.0f,3.0f),4.0f);
    juce::ColourGradient gradient((lit?tint:tint.interpolatedWith(juce::Colours::white,.45f))
                                     .brighter(hover?.12f:0.0f),r.getTopLeft(),
          (lit?tint:tint.interpolatedWith(juce::Colour(0xff8c8f93),.46f))
                                     .darker(down?.2f:0.0f),r.getBottomRight(),false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(r.reduced(lit?2.0f:0.0f),4.0f);
    g.setColour(lit?tint.brighter(.5f):juce::Colour(0xff9ca0a4));
    g.drawRoundedRectangle(r.reduced(1.0f),4.0f,lit?2.0f:1.0f);
    if(lit&&!isSmall)
    {
        g.setColour(juce::Colours::white.withAlpha(.15f));
        g.fillRoundedRectangle(r.reduced(7.0f),5.0f);
    }
    if(isSmall)
    {
        g.setColour(Theme::label);
        g.setFont(Theme::font(13.0f,true));
        g.drawText(getName(),r,juce::Justification::centred);
    }
    if (hasKeyboardFocus(false))
    {
        g.setColour(Theme::cyan.withAlpha(0.9f));
        g.drawRoundedRectangle(r.reduced(0.5f),4.0f,2.0f);
    }
}
