#include "DepthSlider.h"
#include "Theme.h"
#include <cmath>
DepthSlider::DepthSlider(SkellFanAudioProcessor& p)
{
    setSliderStyle(juce::Slider::LinearHorizontal);
    setTextBoxStyle(juce::Slider::NoTextBox,true,0,0);
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    setWantsKeyboardFocus(true);
    setTooltip("Modulation depth (double-click to reset)");
    attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"depth",*this);
    setDoubleClickReturnValue(true,50.0);
}
void DepthSlider::paint(juce::Graphics& g)
{
    if (getWidth() <= 0 || getHeight() <= 0) return;

    // Paint in the 203x64 logical coordinate system used by the layout.
    // Previously the 22px thumb offset stayed unscaled at 574px editor width
    // and physically covered the DEPTH caption.
    g.addTransform(juce::AffineTransform::scale(getWidth()/203.0f,getHeight()/64.0f));
    constexpr float width=203.0f, cy=45.0f;
    g.setColour(Theme::label);
    g.setFont(Theme::font(15.0f,true));
    g.drawText("DEPTH  " + juce::String(juce::roundToInt(getValue())) + "%",
               juce::Rectangle<float>{0.0f,0.0f,width,21.0f},
               juce::Justification::centred);
    g.setColour(juce::Colour(0xff63676c));
    g.drawLine(11.0f,cy,width-11.0f,cy,3.0f);
    constexpr float trackWidth=width-22.0f;
    for(int i=0;i<=50;++i)
    {
        const float x=11.0f+trackWidth*static_cast<float>(i)/50.0f;
        const int len=i%10==0?19:i%5==0?13:7;
        g.setColour(juce::Colour(0xff41474e).withAlpha(i%5==0?0.85f:0.52f));
        g.drawLine(x,cy-4.0f-len*0.42f,x,cy+4.0f+len*0.42f,
                   i%5==0?1.5f:0.8f);
    }
    const float xp=juce::jmap(static_cast<float>(valueToProportionOfLength(getValue())),
                             11.0f,width-11.0f);
    g.setColour(juce::Colour(0xff2584b3).withAlpha(0.65f));
    g.drawLine(11.0f,cy,xp,cy,2.6f);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(xp-8.0f,cy-20.0f,19.0f,35.0f,4.0f);
    juce::ColourGradient gr(juce::Colour(0xfff6fafb),xp-8.0f,cy,
                            juce::Colour(0xff8c989f),xp+8.0f,cy,false);
    g.setGradientFill(gr);
    g.fillRoundedRectangle(xp-6.5f,cy-22.0f,13.0f,32.0f,3.0f);
    g.setColour(juce::Colour(0xff5a6064));
    g.drawLine(xp,cy-19.0f,xp,cy+8.0f,1.5f);
    if (hasKeyboardFocus(false))
    {
        g.setColour(Theme::cyan);
        g.drawRoundedRectangle(xp-9.0f,cy-24.0f,18.0f,36.0f,4.0f,1.6f);
    }
}
void DepthSlider::mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& w)
{ if (w.deltaY != 0.0f)
      setValue(juce::jlimit(0.0,100.0,getValue()+(w.deltaY>0.0f?1.0:-1.0)),juce::sendNotificationSync);
  juce::ignoreUnused(e); }
