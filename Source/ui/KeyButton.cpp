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
    if (!isSmall)
    {
        // The four effect pads used to be featureless grey blocks.
        // Subtle engraved vector symbols distinguish the modes without
        // introducing platform-dependent font glyphs or additional controls.
        // These are rendered only on the UI thread, not audio processing.
        const auto cx=r.getCentreX(), cy=r.getCentreY()-2.0f;
        juce::Path glyph;
        const auto mode=getName();
        if (mode=="VIBRATO")
        {
            // One complete smooth vibrato cycle.
            glyph.startNewSubPath(cx-15.0f,cy);
            glyph.cubicTo(cx-10.0f,cy-12.0f,cx-5.0f,cy-12.0f,cx,cy);
            glyph.cubicTo(cx+5.0f,cy+12.0f,cx+10.0f,cy+12.0f,cx+15.0f,cy);
        }
        else if (mode=="ROTARY")
        {
            glyph.addCentredArc(cx,cy,12.0f,12.0f,0.0f,
                                0.36f,5.72f,true);
            // A miniature vane gives the orbit a clear direction.
            glyph.startNewSubPath(cx-7.5f,cy+10.5f);
            glyph.lineTo(cx-11.0f,cy+4.0f);
            glyph.lineTo(cx-14.1f,cy+10.8f);
        }
        else if (mode=="TREMOLO")
        {
            // Amplitude pulse: alternating tall and short bars.
            glyph.startNewSubPath(cx-12.0f,cy+9.0f);
            glyph.lineTo(cx-12.0f,cy-9.0f);
            glyph.startNewSubPath(cx-4.0f,cy+4.0f);
            glyph.lineTo(cx-4.0f,cy-4.0f);
            glyph.startNewSubPath(cx+4.0f,cy+12.0f);
            glyph.lineTo(cx+4.0f,cy-12.0f);
            glyph.startNewSubPath(cx+12.0f,cy+6.0f);
            glyph.lineTo(cx+12.0f,cy-6.0f);
        }
        else if (mode=="CHORUS")
        {
            // Three layered voices, each with its own phase.
            for(int voice=-1;voice<=1;++voice)
            {
                const float yy=cy+static_cast<float>(voice)*7.0f;
                glyph.startNewSubPath(cx-14.0f,yy+2.5f);
                glyph.cubicTo(cx-5.0f,yy-6.5f,cx+5.0f,yy+8.5f,
                              cx+14.0f,yy-2.5f);
            }
        }
        g.setColour(juce::Colours::black.withAlpha(0.21f));
        g.strokePath(glyph,juce::PathStrokeType(4.0f,
                     juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded),
                     juce::AffineTransform::translation(0.0f,1.2f));
        g.setColour(lit?juce::Colour(0xffeffff1).withAlpha(0.95f)
                       :juce::Colour(0xffd6e4da).withAlpha(0.72f));
        g.strokePath(glyph,juce::PathStrokeType(lit?2.5f:2.1f,
                     juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded));
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
