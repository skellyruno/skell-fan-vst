#include "FanView.h"
#include <BinaryData.h>
#include <cmath>
FanView::FanView(SkellFanAudioProcessor& p):processor(p)
{
    setInterceptsMouseClicks(false,false);
    body=Theme::image(BinaryData::fan_body_png,BinaryData::fan_body_pngSize);
    rotor=Theme::image(BinaryData::blades_png,BinaryData::blades_pngSize);
    blur=Theme::image(BinaryData::blades_blur_png,BinaryData::blades_blur_pngSize);
    grille=Theme::image(BinaryData::grille_png,BinaryData::grille_pngSize);
    hub=Theme::image(BinaryData::hub_cap_png,BinaryData::hub_cap_pngSize);
    lastTime=juce::Time::getMillisecondCounterHiRes();
    startTimerHz(60);
}
void FanView::setAnimationEnabled(bool b)
{
    enabled=b;
    if(!enabled) speed=0.0;
    lastTime=juce::Time::getMillisecondCounterHiRes();
    startTimerHz(enabled?60:12);
    repaint();
}
void FanView::timerCallback()
{
    const auto now=juce::Time::getMillisecondCounterHiRes();
    const double dt=juce::jlimit(0.0,0.08,(now-lastTime)*0.001);
    lastTime=now;
    const bool powerOn = processor.apvts.getRawParameterValue("power")->load() > 0.5f;
    const bool powerChanged = powerOn != lastPowerOn;
    lastPowerOn = powerOn;
    const double speedBefore = speed;
    const double target=(enabled&&powerOn)?juce::jlimit(0.0,3.0,
                       static_cast<double>(processor.displayRate.load())*0.32):0.0;
    const double coeff=1.0-std::exp(-dt/0.40);
    speed+=(target-speed)*coeff;
    angle=std::fmod(angle+dt*speed*juce::MathConstants<double>::twoPi,
                    juce::MathConstants<double>::twoPi);
    // Avoid 60 FPS repaints when the fan is stopped; dynamically restore full
    // framerate when it accelerates, without suppressing bypass redraws.
    const int desiredHz=(!enabled || speed<0.012)?12:60;
    if(getTimerInterval() != 1000/desiredHz)
        startTimerHz(desiredHz);
    // Only repaint the fan head, keeping static artwork out of the dirty region.
    const float x=getWidth()/Theme::W,y=getHeight()/Theme::H;
    if (powerChanged || speedBefore > 0.008 || speed > 0.008)
        repaint(juce::Rectangle<int>(juce::roundToInt(145*x),juce::roundToInt(68*y),
                                     juce::roundToInt(645*x),juce::roundToInt(642*y)));
}
void FanView::paint(juce::Graphics& g)
{
    const float sx=getWidth()/Theme::W,sy=getHeight()/Theme::H;
    g.addTransform(juce::AffineTransform::scale(sx,sy));
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    // Floating-point destination rectangles avoid implicit float->int truncation
    // and keep artwork aligned at fractional HiDPI scales.
    if(body.isValid())
        g.drawImage(body,juce::Rectangle<float>{0.0f,0.0f,Theme::W,Theme::H},
                    juce::RectanglePlacement::stretchToFit);
    auto rotorRect=Theme::rotorRect;
    const auto drawBlade=[&](const juce::Image& im,float opacity,double a)
    {
        if(!im.isValid())return;
        const auto t=juce::AffineTransform::scale(rotorRect.getWidth()/im.getWidth(),
                                 rotorRect.getHeight()/im.getHeight())
                     .rotated(static_cast<float>(a),rotorRect.getWidth()*0.5f,
                                 rotorRect.getHeight()*0.5f)
                     .translated(rotorRect.getX(),rotorRect.getY());
        g.setOpacity(opacity);
        g.drawImageTransformed(im,t,false);
    };
    if(speed<1.8)drawBlade(rotor,1.0f,angle);
    else
    {
        for(int k=4;k>=0;--k)
            drawBlade(rotor,0.13f+(4-k)*0.035f,angle-k*speed*0.038);
        if(speed>2.5)drawBlade(blur,static_cast<float>(juce::jlimit(0.0,0.7,(speed-2.5)*1.4)),angle);
    }
    g.setOpacity(1.0f);
    const auto hubRect=Theme::hubRect;
    if(hub.isValid())
        g.drawImage(hub,hubRect,juce::RectanglePlacement::stretchToFit);
    const auto c=Theme::grilleRect;
    if(grille.isValid())
        g.drawImage(grille,c,juce::RectanglePlacement::stretchToFit);
    if(processor.apvts.getRawParameterValue("power")->load()<0.5f)
    {
        g.setColour(Theme::black.withAlpha(0.35f));
        g.fillEllipse(c.reduced(10.0f));
    }
}
