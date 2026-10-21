#include "DepthSlider.h"
#include "Theme.h"
#include <cmath>
DepthSlider::DepthSlider(BreezeModAudioProcessor& p)
{
    setSliderStyle(juce::Slider::LinearHorizontal);
    setTextBoxStyle(juce::Slider::NoTextBox,true,0,0);
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"depth",*this);
    setDoubleClickReturnValue(true,50.0);
}
void DepthSlider::paint(juce::Graphics& g)
{
    const auto b=getLocalBounds().toFloat();
    const float cy=b.getCentreY()+9.0f;
    g.setColour(Theme::label);
    g.setFont(Theme::font(15.0f,true));
    g.drawText("DEPTH",0,0,getWidth(),22,juce::Justification::centred);
    g.setColour(juce::Colour(0xff63676c));
    g.drawLine(11,cy,b.getRight()-11,cy,3.0f);
    const int width=juce::jmax(1,getWidth()-22);
    for(int i=0;i<=50;++i)
    {
        const float x=11.0f+width*i/50.0f;
        const int len=i%10==0?19:i%5==0?13:7;
        g.setColour(juce::Colour(0xff41474e).withAlpha(i%5==0?0.85f:0.52f));
        g.drawLine(x,cy-4.0f-len*0.42f,x,cy+4.0f+len*0.42f,i%5==0?1.5f:0.8f);
    }
    const float xp=juce::jmap(static_cast<float>(valueToProportionOfLength(getValue())),
                             11.0f,b.getRight()-11.0f);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(xp-8,cy-20,19,35,4.0f);
    juce::ColourGradient gr(juce::Colour(0xfff6fafb),xp-8,cy,juce::Colour(0xff8c989f),xp+8,cy,false);
    g.setGradientFill(gr);
    g.fillRoundedRectangle(xp-6.5f,cy-22,13,32,3.0f);
    g.setColour(juce::Colour(0xff5a6064));g.drawLine(xp,cy-19,xp,cy+8,1.5f);
}
void DepthSlider::mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& w)
{ setValue(juce::jlimit(0.0,100.0,getValue()+((w.deltaY>=0)?1.0:-1.0)),juce::sendNotificationSync);
  juce::ignoreUnused(e); }
