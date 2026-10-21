#include "LfoScope.h"
#include "Theme.h"
LfoScope::LfoScope(BreezeModAudioProcessor& processor):p(processor)
{setInterceptsMouseClicks(false,false);startTimerHz(30);}
void LfoScope::paint(juce::Graphics& g)
{
    auto r=getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff0c1720).withAlpha(.94f));g.fillRoundedRectangle(r,8.0f);
    g.setColour(Theme::cyan);g.drawRoundedRectangle(r.reduced(1.0f),7.0f,1.5f);
    g.setFont(Theme::font(13.0f,true));g.drawText("LFO MONITOR",12,6,getWidth()-24,20,juce::Justification::centredLeft);
    auto plot=r.reduced(13.0f).withTrimmedTop(22.0f).withTrimmedBottom(17.0f);
    const int wave=static_cast<int>(p.apvts.getRawParameterValue("wave")->load());
    juce::Path line;
    for(int i=0;i<200;++i)
    {
        const float ph=i/199.0f;
        const float v=breeze::dsp::Lfo::sample(ph,wave);
        const auto x=plot.getX()+plot.getWidth()*ph;
        const auto y=plot.getCentreY()-v*plot.getHeight()*.42f;
        if(i==0)line.startNewSubPath(x,y);else line.lineTo(x,y);
    }
    g.setColour(Theme::cyan);g.strokePath(line,juce::PathStrokeType(2.0f));
    g.setColour(juce::Colours::white.withAlpha(.55f));
    const auto phase=p.lfoPhase.load(std::memory_order_relaxed);
    g.drawVerticalLine(static_cast<int>(plot.getX()+plot.getWidth()*phase),plot.getY(),plot.getBottom());
    g.setFont(Theme::font(11));g.setColour(Theme::muted);
    g.drawText(juce::String(p.displayRate.load(),2)+" Hz  •  "+juce::String(p.displayBpm.load(),0)+" BPM",
               10,getHeight()-18,getWidth()-20,16,juce::Justification::centred);
}
