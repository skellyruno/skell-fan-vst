#include "FanView.h"
#include <BinaryData.h>
#include <cmath>
FanView::FanView(SkellFanAudioProcessor& p):processor(p)
{
    setInterceptsMouseClicks(false,false);
    rawBody=Theme::image(BinaryData::fan_body_png,BinaryData::fan_body_pngSize);
    rawRotor=Theme::image(BinaryData::blades_png,BinaryData::blades_pngSize);
    rawBlur=Theme::image(BinaryData::blades_blur_png,BinaryData::blades_blur_pngSize);
    grille=Theme::image(BinaryData::grille_png,BinaryData::grille_pngSize);
    setSkin(skellcore::ui::skinFromIndex(
        static_cast<int>(processor.apvts.state.getProperty("uiSkullSkin",1))));
    // Source-photo repairs are cached once per skin, never in paint().
    lastTime=juce::Time::getMillisecondCounterHiRes();
    startTimerHz(60);
}
void FanView::setSkin(skellcore::ui::SkullSkin skin)
{
    const int index=static_cast<int>(skin);
    if(index==skinIndex) return;
    skinIndex=index;
    body=Theme::fanArtworkForSkin(rawBody,skin);
    // The source photograph includes an unrelated saturated arc at the foot
    // of the guard. It used to read as a detached fourth green blade.
    // Convert ONLY that static lower accent to brushed dark metal; keep the
    // real animated blades and the green lower housing untouched.
    if (body.isValid())
    {
        juce::Image::BitmapData pixels(body,juce::Image::BitmapData::readWrite);
        const int left=juce::roundToInt(body.getWidth()*252.0f/Theme::W);
        const int right=juce::roundToInt(body.getWidth()*708.0f/Theme::W);
        const int top=juce::roundToInt(body.getHeight()*588.0f/Theme::H);
        const int bottom=juce::roundToInt(body.getHeight()*707.0f/Theme::H);
        for (int y=top;y<juce::jmin(bottom,body.getHeight());++y)
            for (int x=left;x<juce::jmin(right,body.getWidth());++x)
            {
                const auto c=pixels.getPixelColour(x,y);
                if (c.getFloatAlpha()<0.08f) continue;
                // Fully neutralize every static under-guard pixel. The
                // previous hue gate left green low-saturation antialiasing
                // specks and made the arc appear dusty in HiDPI snapshots.
                const float metalValue=c.getSaturation()>0.08f
                    ? juce::jlimit(0.16f,0.67f,c.getBrightness()*0.68f)
                    : c.getBrightness();
                pixels.setPixelColour(x,y,juce::Colour::fromHSV(
                    0.0f,0.0f,metalValue,c.getFloatAlpha()));
            }
    }

    // The two neon side inserts beside the lower control deck are baked
    // into the still photograph, not part of the real moving rotor.
    // Remove their hue only; preserve the source alpha and subtle shading.
    // This runs once per UI theme switch, never in the DSP or paint loop.
    if(body.isValid())
    {
        juce::Image::BitmapData pixels(body,juce::Image::BitmapData::readWrite);
        const float sx=static_cast<float>(body.getWidth())/Theme::W;
        const float sy=static_cast<float>(body.getHeight())/Theme::H;
        const auto correctSide=[&](int x0,int x1,int y0,int y1)
        {
            const int left=juce::jlimit(0,body.getWidth(),juce::roundToInt(x0*sx));
            const int right=juce::jlimit(0,body.getWidth(),juce::roundToInt(x1*sx));
            const int top=juce::jlimit(0,body.getHeight(),juce::roundToInt(y0*sy));
            const int bottom=juce::jlimit(0,body.getHeight(),juce::roundToInt(y1*sy));
            for(int y=top;y<bottom;++y)
                for(int x=left;x<right;++x)
                {
                    const auto c=pixels.getPixelColour(x,y);
                    if(c.getFloatAlpha()<0.07f || c.getSaturation()<0.22f
                       || c.getHue()<0.20f || c.getHue()>0.49f) continue;
                    const float value=juce::jlimit(0.64f,0.84f,
                        0.65f+c.getBrightness()*0.22f);
                    pixels.setPixelColour(x,y,juce::Colour::fromHSV(
                        0.55f,0.028f,value,c.getFloatAlpha()));
                }
        };
        correctSide(177,247,713,927);
        correctSide(704,774,713,927);

        // Blend the original SAT tower out of the photographed casing by
        // borrowing texture from the equivalent LEFT shoulder. The fan body
        // is approximately symmetric, unlike the detached SAT module.
        // Feather all four sides, preserve authentic lighting/noise, and
        // never fill the region with a flat white geometric blob.
        // Executed once per theme change on the message thread.
        const int x0=juce::jlimit(0,body.getWidth(),juce::roundToInt(619.0f*sx));
        const int x1=juce::jlimit(0,body.getWidth(),juce::roundToInt(763.0f*sx));
        const int y0=juce::jlimit(0,body.getHeight(),juce::roundToInt(599.0f*sy));
        const int y1=juce::jlimit(0,body.getHeight(),juce::roundToInt(751.0f*sy));
        for(int y=y0;y<y1;++y)
            for(int x=x0;x<x1;++x)
            {
                const int mirrorX=body.getWidth()-1-x;
                if(mirrorX<0 || mirrorX>=body.getWidth()) continue;
                const auto original=pixels.getPixelColour(x,y);
                // Mirroring x=284..322 at y=700..734 would clone the baked
                // left-hand motion-button graphic onto the right shoulder.
                // Source the same white plastic a little further inboard
                // in that small area and feather to the symmetric texture.
                const float lx=static_cast<float>(x)/sx;
                const float ly=static_cast<float>(y)/sy;
                const float coverButtonX=juce::jmin(
                    juce::jlimit(0.0f,1.0f,(lx-622.0f)/10.0f),
                    juce::jlimit(0.0f,1.0f,(688.0f-lx)/10.0f));
                const float coverButtonY=juce::jmin(
                    juce::jlimit(0.0f,1.0f,(ly-688.0f)/10.0f),
                    juce::jlimit(0.0f,1.0f,(748.0f-ly)/12.0f));
                const int cleanX=juce::jlimit(0,body.getWidth()-1,
                    mirrorX+juce::roundToInt(70.0f*sx));
                const auto donor=pixels.getPixelColour(mirrorX,y)
                    .interpolatedWith(pixels.getPixelColour(cleanX,y),
                                      coverButtonX*coverButtonY);
                const float fx=juce::jmin(
                    juce::jlimit(0.0f,1.0f,(x-x0)/(12.0f*sx)),
                    juce::jlimit(0.0f,1.0f,(x1-x-1)/(13.0f*sx)));
                const float fy=juce::jmin(
                    juce::jlimit(0.0f,1.0f,(y-y0)/(12.0f*sy)),
                    juce::jlimit(0.0f,1.0f,(y1-y-1)/(17.0f*sy)));
                const float weight=fx*fy;
                if(weight>0.0f)
                    pixels.setPixelColour(x,y,original.interpolatedWith(donor,weight));
            }
        // Repair the tiny photographed SAT marker remnant on the upper
        // shoulder using clean plastic from the same vertical texture.
        // This touches only 619..662 by 686..703 of the body, never audio.
        const int cleanRow=juce::jlimit(0,body.getHeight()-1,
                                       juce::roundToInt(681.0f*sy));
        for(int y=juce::jmax(0,juce::roundToInt(686.0f*sy));
                y<juce::jmin(body.getHeight(),juce::roundToInt(703.0f*sy));++y)
            for(int x=juce::jmax(0,juce::roundToInt(619.0f*sx));
                    x<juce::jmin(body.getWidth(),juce::roundToInt(662.0f*sx));++x)
            {
                const float lx=static_cast<float>(x)/sx;
                const float ly=static_cast<float>(y)/sy;
                const float weight=juce::jmin(
                     juce::jlimit(0.0f,1.0f,(lx-619.0f)/6.0f),
                     juce::jlimit(0.0f,1.0f,(662.0f-lx)/6.0f))
                   *juce::jmin(
                     juce::jlimit(0.0f,1.0f,(ly-686.0f)/4.0f),
                     juce::jlimit(0.0f,1.0f,(703.0f-ly)/4.0f));
                const auto c=pixels.getPixelColour(x,y);
                pixels.setPixelColour(x,y,c.interpolatedWith(
                       pixels.getPixelColour(x,cleanRow),weight));
            }

        // The original photograph's alpha edge has loose white and black
        // speckles on BOTH shoulders, especially below y=853. Color/hue
        // neutralization cannot repair those alpha holes. Trace a smooth
        // casing silhouette through the original white plastic edge, then
        // reconstruct an antialiased border from intact inside pixels.
        // Replaces the old brittle, row-limited rectangular cutout.
        constexpr float shell[][3]={
            {710.0f,204.0f,751.0f},{750.0f,190.0f,759.0f},
            {800.0f,178.0f,767.0f},{850.0f,167.0f,773.0f},
            {900.0f,153.0f,780.0f},{930.0f,158.0f,780.0f},
            {950.0f,173.0f,767.0f},{970.0f,205.0f,737.0f},
            {979.0f,239.0f,717.0f}
        };
        for(int y=juce::jmax(0,juce::roundToInt(shell[0][0]*sy));
                y<juce::jmin(body.getHeight(),juce::roundToInt(979.0f*sy));++y)
        {
            const float fy=static_cast<float>(y)/sy;
            int segment=0;
            while(segment<7 && fy>shell[segment+1][0])++segment;
            const float alpha=juce::jlimit(0.0f,1.0f,
                (fy-shell[segment][0])/(shell[segment+1][0]-shell[segment][0]));
            const float leftEdge=juce::jmap(alpha,shell[segment][1],shell[segment+1][1]);
            const float rightEdge=juce::jmap(alpha,shell[segment][2],shell[segment+1][2]);
            const int leftDonor=juce::jlimit(0,body.getWidth()-1,
                juce::roundToInt((leftEdge+12.0f)*sx));
            // Sample the intact symmetric LEFT shell: the photographed
            // right-hand donor was itself contaminated with bright specks.
            const int rightDonor=juce::jlimit(0,body.getWidth()-1,
                juce::roundToInt((Theme::W-rightEdge+23.0f)*sx));
            const auto cleanLeft=pixels.getPixelColour(leftDonor,y);
            const auto cleanRight=pixels.getPixelColour(rightDonor,y);
            const int leftEnd=juce::jlimit(0,body.getWidth(),
                juce::roundToInt((leftEdge+15.0f)*sx));
            const int rightStart=juce::jlimit(0,body.getWidth(),
                juce::roundToInt((rightEdge-26.0f)*sx));
            for(int x=juce::jmax(0,juce::roundToInt(125.0f*sx));x<leftEnd;++x)
            {
                const float dist=static_cast<float>(x)/sx-leftEdge;
                const float coverage=juce::jlimit(0.0f,1.0f,(dist+0.75f)/2.5f);
                const float keep=juce::jlimit(0.0f,1.0f,(dist-1.0f)/14.0f);
                const auto smooth=cleanLeft.interpolatedWith(
                    pixels.getPixelColour(x,y),keep);
                pixels.setPixelColour(x,y,smooth.withAlpha(
                    smooth.getFloatAlpha()*coverage));
            }
            for(int x=rightStart;x<juce::jmin(body.getWidth(),
                      juce::roundToInt(835.0f*sx));++x)
            {
                const float dist=rightEdge-static_cast<float>(x)/sx;
                const float coverage=juce::jlimit(0.0f,1.0f,(dist+0.75f)/2.5f);
                const float keep=juce::jlimit(0.0f,1.0f,(dist-1.0f)/24.0f);
                const auto smooth=cleanRight.interpolatedWith(
                    pixels.getPixelColour(x,y),keep);
                pixels.setPixelColour(x,y,smooth.withAlpha(
                    smooth.getFloatAlpha()*coverage));
            }
        }
    }
    rotor=Theme::fanArtworkForSkin(rawRotor,skin);
    blur=Theme::fanArtworkForSkin(rawBlur,skin);
    repaint();
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
    // A clean cavity replaces the photographed black/grey patches.
    const auto bladeWindow=Theme::rotorRect.reduced(1.0f);
    juce::ColourGradient cavity(juce::Colour(0xff101b15),469.0f,100.0f,
                                juce::Colour(0xff040807),469.0f,690.0f,false);
    cavity.addColour(0.5,juce::Colour(0xff070e0b));
    g.setGradientFill(cavity);
    g.fillEllipse(bladeWindow);
    // A round rotor aperture, not the old straight pedestal mask, ensures
    // three complete blades remain visible at every animation angle.
    juce::Path rotorAperture;
    rotorAperture.addEllipse(bladeWindow);
    {
        juce::Graphics::ScopedSaveState rotorClip(g);
        g.reduceClipRegion(rotorAperture);
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
    }
    g.setOpacity(1.0f);

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
    // Shared SkellCore skull face, no full-body skeleton in the bearing.
    g.setColour(juce::Colour(0xffe4f5e7).withAlpha(0.25f));
    g.drawEllipse(center.x-58.0f,center.y-58.0f,116.0f,116.0f,1.0f);
    {
        juce::Path skullClip;
        skullClip.addEllipse(center.x-57.0f,center.y-57.0f,107.0f,114.0f);
        juce::Graphics::ScopedSaveState skullSafe(g);
        g.reduceClipRegion(skullClip);
        constexpr float skullScale=4.0f;
        g.addTransform(juce::AffineTransform::scale(skullScale).translated(
             center.x-13.5f*skullScale,center.y-13.5f*skullScale));
        const auto skin=skellcore::ui::skinFromIndex(skinIndex);
        const auto palette=skellcore::ui::skullPalette(skin);
        skellcore::ui::drawSkullKeyMark(g,0.0f,0.0f,palette.bone,palette.panelHi);
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
