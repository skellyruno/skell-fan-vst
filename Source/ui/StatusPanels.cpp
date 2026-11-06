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
    return juce::jlimit(0.0f, 1.0f, (db + 54.0f)/60.0f);
}
void frame(juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::ColourGradient gradient(juce::Colour(0xff141d25),r.getTopLeft(),
                                  juce::Colour(0xff05090e),r.getBottomRight(),false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(r,12.0f);
    g.setColour(juce::Colour(0xff385161));
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
    g.setColour(juce::Colour(0xffc8e9f5));
    g.drawText("OUTPUT",12,5,92,20,juce::Justification::centred);

    constexpr int segments=21;
    constexpr float top=44.0f, step=5.05f;
    for (int ch=0; ch<2; ++ch)
    {
        const float x = 26.0f+ch*43.0f;
        g.setColour(juce::Colour(0xff9aabb8));
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
                                      : juce::Colour(0xff4bbbe8);
            g.setColour(lit?colour:colour.withAlpha(0.10f));
            g.fillRoundedRectangle(x,y,24.0f,3.7f,1.1f);
        }
        if (peak>0.0f)
        {
            const float y=top+(1.0f-peak)*(segments-1)*step;
            g.setColour(juce::Colour(0xffe7f7ff));
            g.fillRoundedRectangle(x-1.0f,y,26.0f,2.0f,0.5f);
        }
        g.setColour(clipHold[static_cast<size_t>(ch)]>0
                        ?juce::Colour(0xffff6f5e):juce::Colour(0xff3b454e));
        g.fillRoundedRectangle(x+5.0f,154.0f,14.0f,5.0f,2.0f);
    }
    g.setFont(Theme::font(10.0f));
    g.setColour(juce::Colour(0xff6d96a9));
    g.drawText("−54",9,153,24,11,juce::Justification::left);
    g.drawText("0 dB",74,166,31,13,juce::Justification::right);
}

void ModeInfo::refresh()
{
    const auto value=[this](const char* id)
    {return processor.apvts.getRawParameterValue(id)->load(std::memory_order_relaxed);};
    const int mode=static_cast<int>(value("mode"));
    const int wave=static_cast<int>(value("wave"));
    const int sync=static_cast<int>(value("syncMode"));
    const int power=static_cast<int>(value("power"));
    const float rate=processor.displayRate.load(std::memory_order_relaxed);
    if (mode!=lastMode || wave!=lastWave || sync!=lastSync || power!=lastPower
        || std::abs(rate-lastRate)>=0.015f)
    {
        lastMode=mode; lastWave=wave; lastSync=sync; lastPower=power; lastRate=rate;
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
    g.setColour(juce::Colour(0xff91b0c0));
    g.setFont(Theme::font(12.5f,true));
    g.drawText("MODULATION",11,6,133,20,juce::Justification::centred);
    g.setColour(on?juce::Colour(0xff70c9ee):juce::Colour(0xff86949e));
    g.setFont(Theme::font(19.0f,true));
    g.drawText(on?modes[mode]:"BYPASS",10,37,135,25,juce::Justification::centred);
    g.setColour(juce::Colour(0xffa9b9c3));
    g.setFont(Theme::font(11.5f,true));
    g.drawText(sub[mode],10,66,135,17,juce::Justification::centred);
    g.setColour(juce::Colour(0xff374d5e));
    g.drawHorizontalLine(94,13.0f,142.0f);
    g.setColour(juce::Colour(0xff8d9eab));
    g.setFont(Theme::font(11.5f));
    g.drawText("WAVE",15,101,50,17,juce::Justification::left);
    g.setColour(juce::Colour(0xffd5edf6));
    g.drawText(waves[shape],62,101,78,17,juce::Justification::right);
    g.setColour(juce::Colour(0xff8d9eab));
    g.drawText("CLOCK",15,122,50,17,juce::Justification::left);
    g.setColour(juce::Colour(0xffd5edf6));
    g.drawText(host?"HOST":"FREE",62,122,78,17,juce::Justification::right);
    g.setColour(juce::Colour(0xff3d5b6c));
    g.drawHorizontalLine(147,14.0f,142.0f);
    g.setColour(juce::Colour(0xffc7e8f6));
    g.setFont(Theme::font(12.5f,true));
    g.drawText(juce::String(processor.displayRate.load(std::memory_order_relaxed),2)+" Hz",
               13,152,128,23,juce::Justification::centred);
}
