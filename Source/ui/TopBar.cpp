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
    g.setColour(getToggleState()?juce::Colour(0xff9fe8f7):juce::Colour(0xffb8b9bd));
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

void SkullToggle::paintButton(juce::Graphics& g,bool hover,bool pressed)
{
    const auto palette=skellcore::ui::skullPalette(selected);
    const auto area=getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(palette.panelHi.brighter(hover?0.09f:0.0f));
    g.fillRoundedRectangle(area,5.0f);
    g.setColour(palette.line);
    g.drawRoundedRectangle(area,5.0f,1.0f);
    const float scale=juce::jmin((area.getWidth()-8.0f)/74.0f,
                                 (area.getHeight()-8.0f)/28.0f);
    const float dx=area.getX()+(area.getWidth()-74.0f*scale)*0.5f;
    const float dy=area.getY()+(area.getHeight()-28.0f*scale)*0.5f;
    juce::Graphics::ScopedSaveState saved(g);
    g.addTransform(juce::AffineTransform::scale(scale).translated(dx,dy));
    skellcore::ui::drawSkullKeyMark(g,0.0f,0.0f,
                                    palette.accent,palette.panelHi);
    if (pressed)
    {
        g.setColour(palette.bone.withAlpha(0.15f));
        g.fillRect(0.0f,0.0f,74.0f,28.0f);
    }
}

TopBar::TopBar(SkellFanAudioProcessor& p)
    :processor(p),width(p,"width","",true),mix(p,"mix","",true)
{
    power.setButtonText({});
    power.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    powerAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"power",power);
    scope.setButtonText({});
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
    g.fillAll(palette.background);
    g.addTransform(juce::AffineTransform::scale(getWidth()/956.0f,getHeight()/60.0f));
    g.setColour(palette.line);g.fillRect(0,59,956,1);
    // One-word SKELLFAN label; second half uses the shared SkellCore accent.
    g.setColour(palette.bone);
    g.setFont(Theme::font(13.5f,true));
    g.drawText("SKELL",164,11,44,21,juce::Justification::centredLeft);
    g.setColour(palette.accent);
    g.drawText("FAN",207,11,33,21,juce::Justification::centredLeft);
    g.setColour(palette.boneDim);
    g.setFont(Theme::font(9.0f,true));
    g.drawText("FAN MOD FX",164,32,76,15,juce::Justification::centredLeft);
    g.setColour(palette.line);
    g.drawVerticalLine(239,13.0f,47.0f);
    g.setColour(palette.boneDim);
    g.setFont(Theme::font(15.0f,true));
    g.drawText("WIDTH",680,11,74,40,juce::Justification::centred);
    g.drawText("DRY / WET",802,11,92,40,juce::Justification::centred);
}
void TopBar::resized()
{
    const auto sx=getWidth()/956.0f,sy=getHeight()/60.0f;
    const auto place=[sx,sy](juce::Component& c,int x,int y,int w,int h)
    { c.setBounds(juce::roundToInt(x*sx),juce::roundToInt(y*sy),
                  juce::roundToInt(w*sx),juce::roundToInt(h*sy)); };
    place(power,9,9,39,42);place(scope,56,9,39,42);
    place(skull,98,10,62,40);
    place(width,748,0,48,58);place(mix,897,0,57,58);
}
