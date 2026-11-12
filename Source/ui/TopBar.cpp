#include "TopBar.h"
#include "Theme.h"
#include <cmath>

void PowerToggle::paintButton(juce::Graphics& g,bool hover,bool down)
{
    auto r=getLocalBounds().toFloat().reduced(1.5f);
    g.setColour(juce::Colour(0xff191a1c).brighter(hover?0.12f:0.0f));
    g.fillRoundedRectangle(r,5.0f);
    const float cx=r.getCentreX(),cy=r.getCentreY(),radius=11.5f;
    juce::Path powerSymbol;
    powerSymbol.addCentredArc(cx,cy+1.0f,radius,radius,0,
        juce::MathConstants<float>::pi*.27f,juce::MathConstants<float>::pi*1.73f,true);
    g.setColour(getToggleState()?Theme::cyan:juce::Colour(0xffb8b9bd));
    g.strokePath(powerSymbol,juce::PathStrokeType(2.9f));
    g.drawLine(cx,cy-13,cx,cy+1,2.9f);
    if(down){g.setColour(juce::Colours::white.withAlpha(.06f));g.fillRoundedRectangle(r,5);}
}
void ScopeToggle::paintButton(juce::Graphics& g,bool hover,bool down)
{
    auto r=getLocalBounds().toFloat().reduced(1.5f);
    g.setColour(juce::Colour(0xff191a1c).brighter(hover?0.12f:0.0f));
    g.fillRoundedRectangle(r,5.0f);
    juce::Path wave;
    const auto cx=r.getCentreX(),cy=r.getCentreY();
    wave.startNewSubPath(cx-14,cy);
    for(int i=1;i<=32;++i)
    {
        const float x=cx-14+28.0f*i/32.0f;
        const float y=cy-8.0f*std::sin(i*juce::MathConstants<float>::twoPi*2.0f/32.0f);
        wave.lineTo(x,y);
    }
    g.setColour(juce::Colour(0xffe6ecf0));
    g.strokePath(wave,juce::PathStrokeType(down?2.7f:2.0f));
}

void FanSkullToggle::paintButton(juce::Graphics& g,bool hover,bool down)
{
    const auto palette=skellcore::ui::skullPalette(selected);
    const auto box=getLocalBounds().toFloat().reduced(1.0f);
    const float scale=juce::jmin(box.getWidth()/53.0f,box.getHeight()/43.0f);
    const float cx=box.getCentreX(), cy=box.getCentreY();
    g.setColour(palette.panelHi.brighter(hover?0.13f:0.0f));
    g.fillRoundedRectangle(box,5.0f);
    g.setColour(palette.line);
    g.drawRoundedRectangle(box.reduced(0.5f),5.0f,1.0f);

    // Original three-blade motor-fan silhouette built from one vector petal.
    // 120-degree clones share the same hub; no image allocations in paint().
    juce::Path blade;
    blade.startNewSubPath(-4.0f*scale,-5.0f*scale);
    blade.cubicTo(-11.0f*scale,-8.0f*scale,-15.0f*scale,-20.0f*scale,-8.0f*scale,-21.0f*scale);
    blade.cubicTo(4.0f*scale,-25.0f*scale,12.0f*scale,-17.0f*scale,7.0f*scale,-8.0f*scale);
    blade.closeSubPath();
    for (int i=0;i<3;++i)
    {
        auto petal=blade;
        petal.applyTransform(juce::AffineTransform::rotation(
            juce::MathConstants<float>::twoPi*static_cast<float>(i)/3.0f)
                .translated(cx,cy));
        g.setGradientFill(juce::ColourGradient(palette.accent.brighter(0.22f),
            cx-10.0f*scale,cy-19.0f*scale,palette.accent.darker(0.30f),
            cx+15.0f*scale,cy+17.0f*scale,false));
        g.fillPath(petal);
        g.setColour(palette.bone.withAlpha(0.35f));
        g.strokePath(petal,juce::PathStrokeType(0.55f*scale));
    }
    g.setColour(palette.boneDim.withAlpha(0.72f));
    g.drawEllipse(cx-21.0f*scale,cy-21.0f*scale,42.0f*scale,42.0f*scale,1.1f*scale);
    g.setColour(palette.panelHi);
    g.fillEllipse(cx-10.5f*scale,cy-10.5f*scale,21.0f*scale,21.0f*scale);
    g.setColour(palette.boneDim);
    g.drawEllipse(cx-10.5f*scale,cy-10.5f*scale,21.0f*scale,21.0f*scale,1.0f*scale);

    // Clip SkellCore's shared skull-key geometry to the centre bearing:
    // the shared skull face is used, but the long key shaft is not visible.
    juce::Path centreMask;
    centreMask.addEllipse(cx-9.4f*scale,cy-9.4f*scale,18.8f*scale,18.8f*scale);
    juce::Graphics::ScopedSaveState safe(g);
    g.reduceClipRegion(centreMask);
    const float skullSize=0.66f*scale;
    g.addTransform(juce::AffineTransform::scale(skullSize).translated(
        cx-13.5f*skullSize,cy-13.5f*skullSize));
    skellcore::ui::drawSkullKeyMark(g,0.0f,0.0f,palette.bone,palette.panelHi);
    if (down)
    {
        g.setColour(palette.accent.withAlpha(0.18f));
        g.fillRect(0.0f,0.0f,27.0f,27.0f);
    }
}

TopBar::TopBar(SkellFanAudioProcessor& p)
    :processor(p),width(p,"width","",true),mix(p,"mix","",true)
{
    power.setButtonText({});
    power.setName("SkellFan power");
    power.setTooltip("Power bypass (right side)");
    power.setWantsKeyboardFocus(true);
    power.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    powerAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"power",power);
    scope.setButtonText({});
    scope.setTooltip("Show or hide modulation waveform monitor");
    scope.onClick=[this]{if(onScope)onScope();};
    scope.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    for(auto* b:{static_cast<juce::Button*>(&power),static_cast<juce::Button*>(&scope)})
    {
        b->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff18191b));
        b->setColour(juce::TextButton::textColourOffId,juce::Colours::white);
        addAndMakeVisible(b);
    }
    skin=skellcore::ui::skinFromIndex(
        static_cast<int>(p.apvts.state.getProperty("uiSkullSkin",1)));
    skull.setSkin(skin);
    skull.onClick=[this]
    {
        skin=skellcore::ui::nextSkin(skin);
        processor.apvts.state.setProperty("uiSkullSkin",
                                          static_cast<int>(skin),nullptr);
        skull.setSkin(skin);
        repaint();
    };
    addAndMakeVisible(skull);
    addAndMakeVisible(width);addAndMakeVisible(mix);
}
void TopBar::refreshSkin()
{
    const auto updated=skellcore::ui::skinFromIndex(
        static_cast<int>(processor.apvts.state.getProperty("uiSkullSkin",1)));
    if (updated==skin) return;
    skin=updated;
    skull.setSkin(skin);
    repaint();
}
void TopBar::paint(juce::Graphics& g)
{
    const auto palette=skellcore::ui::skullPalette(skin);
    g.fillAll(juce::Colour(0xff080a08));
    g.addTransform(juce::AffineTransform::scale(getWidth()/956.0f,getHeight()/60.0f));
    g.setColour(palette.line);g.fillRect(0,59,956,1);

    // One attributed text run: no artificial SKELL / FAN spacing.
    juce::AttributedString title;
    title.setJustification(juce::Justification::centredLeft);
    title.append("SKELL",Theme::font(17.0f,true),palette.bone);
    title.append("FAN",Theme::font(17.0f,true),palette.accent);
    title.draw(g,juce::Rectangle<float>{109.0f,9.0f,106.0f,26.0f});
    g.setColour(palette.boneDim);
    g.setFont(Theme::font(9.0f,true));
    g.drawText("FAN MOD FX",110,35,96,13,juce::Justification::centredLeft);
    g.setColour(palette.line);
    g.drawVerticalLine(208,10.0f,49.0f);
    g.drawVerticalLine(895,10.0f,49.0f);
    g.setColour(palette.boneDim);
    g.setFont(Theme::font(13.0f,true));
    g.drawText("WIDTH",632,11,58,36,juce::Justification::centred);
    g.drawText("DRY/WET",742,11,67,36,juce::Justification::centred);
}
void TopBar::resized()
{
    const auto sx=getWidth()/956.0f,sy=getHeight()/60.0f;
    const auto place=[sx,sy](juce::Component& c,int x,int y,int w,int h)
    { c.setBounds(juce::roundToInt(x*sx),juce::roundToInt(y*sy),
                  juce::roundToInt(w*sx),juce::roundToInt(h*sy)); };
    // Three-blade skull fan is first, Power is the rightmost control.
    place(skull,7,6,53,47);
    place(scope,66,9,36,42);
    place(width,688,1,47,58);
    place(mix,813,1,51,58);
    place(power,905,9,43,42);
}
