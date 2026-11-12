#include "StatusPanels.h"
#include "Theme.h"
#include <cmath>

namespace
{
constexpr float meterW = 116.0f, meterH = 183.0f;
float toMeter(float level) noexcept
{
    if (!std::isfinite(level) || level <= 0.0f) return 0.0f;
    const float db = 20.0f*std::log10(level);
    return juce::jlimit(0.0f, 1.0f, (db + 54.0f)/54.0f);
}
void frame(juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::ColourGradient gradient(juce::Colour(0xff101b14),r.getTopLeft(),
                                  juce::Colour(0xff080c09),r.getBottomRight(),false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(r,12.0f);
    g.setColour(juce::Colour(0xff31583c));
    g.drawRoundedRectangle(r.reduced(0.5f),11.5f,1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawHorizontalLine(28,12.0f,r.getRight()-12.0f);
}
}

void LiveMeter::refresh() noexcept
{
    const auto nextFrame=processor.meterFrame.load(std::memory_order_acquire);
    const bool fresh=nextFrame!=lastFrame;
    lastFrame=nextFrame;
    for (size_t ch=0; ch<2; ++ch)
    {
        // Drop to silence when the DAW stops calling processBlock rather than
        // freezing the last meter reading indefinitely.
        const float inputRms=fresh?processor.displayRms[ch].load(std::memory_order_relaxed):0.0f;
        const float inputPeak=fresh?processor.displayPeak[ch].load(std::memory_order_relaxed):0.0f;
        const float validRms=std::isfinite(inputRms) ? juce::jlimit(0.0f,2.0f,inputRms) : 0.0f;
        const float validPeak=std::isfinite(inputPeak) ? juce::jlimit(0.0f,2.0f,inputPeak) : 0.0f;
        rms[ch]=juce::jmax(validRms,rms[ch]*0.87f);
        peaks[ch]=juce::jmax(validPeak,peaks[ch]*0.967f);
        if (validPeak >= 1.0f) clipHold[ch]=26;
        else if (clipHold[ch]>0) --clipHold[ch];
    }
    repaint();
}

void LiveMeter::paint(juce::Graphics& g)
{
    g.addTransform(juce::AffineTransform::scale(getWidth()/meterW,getHeight()/meterH));
    frame(g,{0.0f,0.0f,meterW,meterH});
    g.setFont(Theme::font(14.0f,true));
    g.setColour(juce::Colour(0xffd4efdd));
    g.drawText("OUTPUT",12,5,92,20,juce::Justification::centred);

    // Inset calibrated scale; the previous -54 label collided with CLIP LEDs.
    g.setColour(juce::Colour(0xff476a51).withAlpha(0.5f));
    g.drawHorizontalLine(96,31.0f,105.0f);
    constexpr int segments=21;
    constexpr float top=44.0f, step=5.05f;
    for (int ch=0; ch<2; ++ch)
    {
        const float x = 35.0f+ch*40.0f;
        g.setColour(juce::Colour(0xffa3b9aa));
        g.setFont(Theme::font(12.0f,true));
        g.drawText(ch==0?"L":"R",juce::Rectangle<float>{x,29.0f,24.0f,13.0f},
                   juce::Justification::centred);
        const float on=toMeter(rms[static_cast<size_t>(ch)]);
        const float peak=toMeter(peaks[static_cast<size_t>(ch)]);
        for (int i=0; i<segments; ++i)
        {
            const float y=top+(segments-1-i)*step;
            const bool lit=static_cast<float>(i+1)/segments<=on;
            juce::Colour colour = i>=19 ? juce::Colour(0xfff26c60)
                              : i>=16 ? juce::Colour(0xffffca73)
                                      : juce::Colour(0xff56de7a);
            g.setColour(lit?colour:colour.withAlpha(0.10f));
            g.fillRoundedRectangle(x,y,24.0f,3.7f,1.1f);
        }
        if (peak>0.0f)
        {
            const float y=top+(1.0f-peak)*(segments-1)*step;
            g.setColour(juce::Colour(0xffe2f8e8));
            g.fillRoundedRectangle(x-1.0f,y,26.0f,2.0f,0.5f);
        }
        g.setColour(clipHold[static_cast<size_t>(ch)]>0
                        ?juce::Colour(0xffff6f5e):juce::Colour(0xff35473b));
        g.fillRoundedRectangle(x+5.0f,154.0f,14.0f,5.0f,2.0f);
    }
    g.setFont(Theme::font(10.0f,true));
    g.setColour(juce::Colour(0xffa5bbaa));
    g.drawText("0",8,39,22,12,juce::Justification::centredRight);
    g.drawText("-24",4,91,26,12,juce::Justification::centredRight);
    g.drawText("-54",4,139,26,12,juce::Justification::centredRight);
    g.setColour(juce::Colour(0xff718a78));
    g.setFont(Theme::font(9.0f,true));
    g.drawText("dBFS",30,165,55,12,juce::Justification::centred);
}

void ModeInfo::refresh()
{
    const auto value=[this](const char* id)
    {return processor.apvts.getRawParameterValue(id)->load(std::memory_order_relaxed);};
    const int mode=static_cast<int>(value("mode"));
    const int wave=static_cast<int>(value("wave"));
    const int sync=static_cast<int>(value("syncMode"));
    const int power=static_cast<int>(value("power"));
    const int sat=juce::roundToInt(value("saturation"));
    const int mix=juce::roundToInt(value("mix"));
    if (mode!=lastMode || wave!=lastWave || sync!=lastSync || power!=lastPower
        || sat!=lastSat || mix!=lastMix)
    {
        lastMode=mode; lastWave=wave; lastSync=sync; lastPower=power;
        lastSat=sat; lastMix=mix;
        repaint();
    }
}

void ModeInfo::paint(juce::Graphics& g)
{
    g.addTransform(juce::AffineTransform::scale(getWidth()/155.0f,getHeight()/183.0f));
    frame(g,{0.0f,0.0f,155.0f,183.0f});
    const auto get=[this](const char* id)
    {return processor.apvts.getRawParameterValue(id)->load(std::memory_order_relaxed);};
    const int mode=juce::jlimit(0,3,static_cast<int>(get("mode")));
    const int shape=juce::jlimit(0,3,static_cast<int>(get("wave")));
    const bool host=get("syncMode")>0.5f;
    const bool on=get("power")>0.5f;
    constexpr const char* modes[]={"VIBRATO","ROTARY","TREMOLO","CHORUS"};
    constexpr const char* sub[]={"PITCH MOTION","DUAL-ROTOR","LEVEL PULSE","3-VOICE SPREAD"};
    constexpr const char* waves[]={"SINE","TRIANGLE","SQUARE","SAW"};
    g.setColour(juce::Colour(0xffa5c1ac));
    g.setFont(Theme::font(12.5f,true));
    g.drawText("MODULATION",11,6,133,20,juce::Justification::centred);
    g.setColour(on?juce::Colour(0xff6af589):juce::Colour(0xff86949e));
    g.setFont(Theme::font(19.0f,true));
    g.drawText(on?modes[mode]:"BYPASS",10,37,135,25,juce::Justification::centred);
    g.setColour(juce::Colour(0xffa9b9c3));
    g.setFont(Theme::font(11.5f,true));
    g.drawText(sub[mode],10,66,135,17,juce::Justification::centred);
    g.setColour(juce::Colour(0xff335844));
    g.drawHorizontalLine(94,13.0f,142.0f);
    g.setColour(juce::Colour(0xff8d9eab));
    g.setFont(Theme::font(11.5f));
    g.drawText("WAVE",15,101,50,17,juce::Justification::left);
    g.setColour(juce::Colour(0xffe1f6e6));
    g.drawText(waves[shape],62,101,78,17,juce::Justification::right);
    g.setColour(juce::Colour(0xff8d9eab));
    g.drawText("CLOCK",15,122,50,17,juce::Justification::left);
    g.setColour(juce::Colour(0xffe1f6e6));
    g.drawText(host?"HOST":"FREE",62,122,78,17,juce::Justification::right);
    g.setColour(juce::Colour(0xff365a43));
    g.drawHorizontalLine(147,14.0f,142.0f);
    g.setColour(juce::Colour(0xffbcebc8));
    g.setFont(Theme::font(11.5f,true));
    const auto saturation=juce::roundToInt(get("saturation"));
    const auto mix=juce::roundToInt(get("mix"));
    g.drawText("SAT " + juce::String(saturation) + "%  MIX " +
                   juce::String(mix) + "%",
               13,152,128,23,juce::Justification::centred);
}
