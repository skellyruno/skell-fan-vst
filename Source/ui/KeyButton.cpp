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
    // A genuine selection indicator: only the active mode/sync key is
    // illuminated. Previously inactive keys borrowed their individual bright
    // tint, so ROT and TRM looked ON while the actual mode was CHO.
    // The same neutral-off treatment makes the manual/host clock obvious.
    const auto selected=Theme::cyan.interpolatedWith(tint,0.25f);
    const auto offTop=juce::Colour(0xff77877e);
    const auto offBottom=juce::Colour(0xff46574d);
    const auto topColour=(lit?selected.brighter(0.05f):offTop)
                             .brighter(hover?0.10f:0.0f);
    const auto bottomColour=(lit?selected.darker(0.37f):offBottom)
                                .darker(down?0.16f:0.0f);
    juce::ColourGradient gradient(topColour,r.getTopLeft(),
                                  bottomColour,r.getBottomRight(),false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(r.reduced(lit?1.2f:0.0f),4.0f);
    g.setColour(lit?Theme::cyan:juce::Colour(0xff33483a));
    g.drawRoundedRectangle(r.reduced(0.9f),4.0f,lit?2.2f:1.0f);
    if(lit && !isSmall)
    {
        // The narrow inner rim reads as a pressed/engaged hardware key.
        g.setColour(juce::Colour(0xffeaffed).withAlpha(0.27f));
        g.drawRoundedRectangle(r.reduced(5.0f),3.0f,1.0f);
        g.setColour(juce::Colours::black.withAlpha(0.13f));
        g.fillRoundedRectangle(r.reduced(9.0f),3.0f);
    }
    if(isSmall)
    {
        g.setColour(lit?juce::Colour(0xff08210e):juce::Colour(0xffe0ebe2));
        g.setFont(Theme::font(13.0f,true));
        g.drawText(getName(),r,juce::Justification::centred);
    }
    if (hasKeyboardFocus(false))
    {
        g.setColour(Theme::cyan.withAlpha(0.9f));
        g.drawRoundedRectangle(r.reduced(0.5f),4.0f,2.0f);
    }
}
