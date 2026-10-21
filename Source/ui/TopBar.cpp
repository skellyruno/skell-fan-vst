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

TopBar::TopBar(BreezeModAudioProcessor& p)
    :width(p,"width","",true),mix(p,"mix","",true)
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
    addAndMakeVisible(width);addAndMakeVisible(mix);
}
void TopBar::paint(juce::Graphics& g)
{
    g.fillAll(Theme::bar);
    g.addTransform(juce::AffineTransform::scale(getWidth()/956.0f,getHeight()/60.0f));
    g.setColour(juce::Colour(0xff272729));g.fillRect(0,59,956,1);
    g.setColour(Theme::muted);
    g.setFont(Theme::font(15.0f,true));
    g.drawText("W I D T H",680,11,74,40,juce::Justification::centred);
    g.drawText("D R Y / W E T",802,11,92,40,juce::Justification::centred);
}
void TopBar::resized()
{
    const auto sx=getWidth()/956.0f,sy=getHeight()/60.0f;
    const auto place=[sx,sy](juce::Component& c,int x,int y,int w,int h)
    { c.setBounds(juce::roundToInt(x*sx),juce::roundToInt(y*sy),
                  juce::roundToInt(w*sx),juce::roundToInt(h*sy)); };
    place(power,9,9,39,42);place(scope,56,9,39,42);
    place(width,748,0,48,58);place(mix,897,0,57,58);
}
