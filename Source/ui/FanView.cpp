#include "FanView.h"
#include <BinaryData.h>
#include <cmath>
FanView::FanView(SkellFanAudioProcessor& p):processor(p)
{
    setInterceptsMouseClicks(false,false);
    body=Theme::greenFanArtwork(Theme::image(BinaryData::fan_body_png,BinaryData::fan_body_pngSize));
    rotor=Theme::greenFanArtwork(Theme::image(BinaryData::blades_png,BinaryData::blades_pngSize));
    blur=Theme::greenFanArtwork(Theme::image(BinaryData::blades_blur_png,BinaryData::blades_blur_pngSize));
    grille=Theme::image(BinaryData::grille_png,BinaryData::grille_pngSize);
    // Build one compact skeletal path: no SVG decoding or path allocation at 60 Hz.
    // Skull, jaw, collar bones, rib cage, pelvis and limb segments share one vector.
    skeletonPath.addEllipse(-8.0f,-33.0f,16.0f,17.0f);
    skeletonPath.addEllipse(-5.0f,-26.2f,3.2f,3.0f);
    skeletonPath.addEllipse( 1.8f,-26.2f,3.2f,3.0f);
    skeletonPath.startNewSubPath(-4.2f,-16.0f);
    skeletonPath.lineTo(-3.0f,-12.0f);
    skeletonPath.lineTo( 3.0f,-12.0f);
    skeletonPath.lineTo( 4.2f,-16.0f);
    skeletonPath.startNewSubPath(0.0f,-12.0f);
    skeletonPath.lineTo(0.0f,20.0f); // spine
    skeletonPath.startNewSubPath(-16.0f,-8.0f);
    skeletonPath.lineTo(16.0f,-8.0f); // shoulders
    for (int rib=0;rib<3;++rib)
    {
        const float y=-4.0f+rib*6.5f;
        skeletonPath.startNewSubPath(-14.0f+rib*1.8f,y);
        skeletonPath.quadraticTo(0.0f,y+7.0f,14.0f-rib*1.8f,y);
    }
    skeletonPath.startNewSubPath(-10.0f,18.0f);
    skeletonPath.lineTo(0.0f,24.0f);
    skeletonPath.lineTo(10.0f,18.0f);
    skeletonPath.startNewSubPath(-16.0f,-8.0f);
    skeletonPath.lineTo(-23.0f,6.0f);
    skeletonPath.lineTo(-19.0f,22.0f);
    skeletonPath.startNewSubPath(16.0f,-8.0f);
    skeletonPath.lineTo(23.0f,6.0f);
    skeletonPath.lineTo(19.0f,22.0f);
    skeletonPath.startNewSubPath(-8.0f,21.0f);
    skeletonPath.lineTo(-11.0f,37.0f);
    skeletonPath.lineTo(-9.0f,49.0f);
    skeletonPath.startNewSubPath(8.0f,21.0f);
    skeletonPath.lineTo(11.0f,37.0f);
    skeletonPath.lineTo(9.0f,49.0f);
    foregroundMask.startNewSubPath(274.0f,610.0f);
    foregroundMask.lineTo(683.0f,610.0f);
    foregroundMask.quadraticTo(727.0f,629.0f,745.0f,687.0f);
    foregroundMask.lineTo(788.0f,901.0f);
    foregroundMask.lineTo(797.0f,984.0f);
    foregroundMask.lineTo(160.0f,984.0f);
    foregroundMask.lineTo(166.0f,899.0f);
    foregroundMask.lineTo(211.0f,687.0f);
    foregroundMask.quadraticTo(232.0f,629.0f,274.0f,610.0f);
    foregroundMask.closeSubPath();
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
    // The casing is behind the fan, while shadowy figures live further
    // behind the blades. Their sway and orbit derive from the same angle;
    // stopping animation or bypassing the effect freezes every figure.
    {
        const auto ghost=[&](float x,float y,float scale,float phase,float opacity)
        {
            juce::Graphics::ScopedSaveState saved(g);
            const float sway=static_cast<float>(std::sin(angle*0.7+phase))*5.0f;
            g.addTransform(juce::AffineTransform::scale(scale)
                .rotated(static_cast<float>(std::sin(angle*0.5+phase))*0.14f)
                .translated(x+sway,y));
            g.setColour(juce::Colour(0xff80e899).withAlpha(opacity));
            g.strokePath(skeletonPath,juce::PathStrokeType(2.3f));
        };
        ghost(270.0f,355.0f,1.45f,0.0f,0.14f);
        ghost(660.0f,342.0f,1.36f,2.1f,0.13f);
        ghost(469.0f,547.0f,1.25f,4.2f,0.11f);
    }

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

    // Front pedestal occludes the physically impossible lower blade spill.
    // Repaint only the foreground casing from the existing source artwork
    // before the front grille: no extra PNG and no detached neon patch.
    if (body.isValid())
    {
        juce::Graphics::ScopedSaveState saved(g);
        g.reduceClipRegion(foregroundMask);
        g.drawImage(body,juce::Rectangle<float>{0.0f,0.0f,Theme::W,Theme::H},
                    juce::RectanglePlacement::stretchToFit);
    }

    // Premium mechanical bearing: replace the flat concentric green PNG
    // with steel bevels, recessed black inner barrel and an engraved skeleton.
    const auto hubRect=Theme::hubRect;
    const auto center=hubRect.getCentre();
    constexpr float r=95.0f;
    g.setColour(juce::Colours::black.withAlpha(0.84f));
    g.fillEllipse(center.x-r-3.0f,center.y-r+4.0f,2*r+6.0f,2*r+6.0f);
    juce::ColourGradient metal(juce::Colour(0xffd2e4d6),center.x-r,center.y-r,
                               juce::Colour(0xff24372a),center.x+r,center.y+r,false);
    metal.addColour(0.39,juce::Colour(0xff697b6e));
    metal.addColour(0.65,juce::Colour(0xff2b3a31));
    g.setGradientFill(metal);
    g.fillEllipse(center.x-r,center.y-r,2*r,2*r);
    g.setColour(juce::Colour(0xff09120d));
    g.fillEllipse(center.x-82.0f,center.y-82.0f,164.0f,164.0f);
    g.setColour(Theme::cyan.withAlpha(0.62f));
    g.drawEllipse(center.x-78.0f,center.y-78.0f,156.0f,156.0f,2.2f);
    juce::ColourGradient inset(juce::Colour(0xff2a4432),center.x-53.0f,center.y-61.0f,
                               juce::Colour(0xff060d08),center.x+61.0f,center.y+68.0f,false);
    g.setGradientFill(inset);
    g.fillEllipse(center.x-69.0f,center.y-69.0f,138.0f,138.0f);
    // Fine, fixed concentric lathe details: they remain stationary
    // while the skeletal mascot and blade assembly rotate together.
    g.setColour(juce::Colour(0xffeffff1).withAlpha(0.38f));
    g.drawEllipse(center.x-88.0f,center.y-88.0f,176.0f,176.0f,1.2f);
    g.setColour(juce::Colour(0xff07140b).withAlpha(0.6f));
    g.drawEllipse(center.x-74.0f,center.y-74.0f,148.0f,148.0f,1.5f);
    {
        juce::Graphics::ScopedSaveState saved(g);
        g.addTransform(juce::AffineTransform::scale(1.35f)
                        .rotated(static_cast<float>(angle))
                        .translated(center.x,center.y-9.0f));
        g.setColour(juce::Colour(0xff07160c));
        g.strokePath(skeletonPath,juce::PathStrokeType(5.5f));
        g.setColour(juce::Colour(0xffd7f1dc));
        g.strokePath(skeletonPath,juce::PathStrokeType(3.3f));
    }
    const auto c=Theme::grilleRect;
    if(grille.isValid())
        g.drawImage(grille,c,juce::RectanglePlacement::stretchToFit);
    if(processor.apvts.getRawParameterValue("power")->load()<0.5f)
    {
        g.setColour(Theme::black.withAlpha(0.35f));
        g.fillEllipse(c.reduced(10.0f));
    }
}
