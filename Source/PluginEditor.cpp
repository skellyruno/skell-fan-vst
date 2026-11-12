#include "PluginEditor.h"
#include "ui/Theme.h"
#include <cmath>
namespace
{
void label(juce::Graphics& g,const juce::String& s,juce::Rectangle<float> r,float size=13.0f)
{
    g.setFont(Theme::font(size,true));
    g.setColour(Theme::label);
    g.drawText(s,r,juce::Justification::centred);
}
}
void SkellFanAudioProcessorEditor::Stage::paint(juce::Graphics& g)
{
    g.addTransform(juce::AffineTransform::scale(getWidth()/Theme::W,getHeight()/Theme::H));
    g.fillAll(Theme::black);
    juce::ColourGradient floor(juce::Colour(0xff15171a),0,700,
                               juce::Colour(0xff040405),0,984,false);
    floor.addColour(0.35,juce::Colour(0xff090b0e));
    g.setGradientFill(floor);g.fillRect(0,585,956,399);
    // Cached ambient light under the fan; no additional bitmap or frame cost.
    juce::ColourGradient halo(juce::Colour(0xff225a35).withAlpha(0.19f),466,376,
                              juce::Colours::transparentBlack,810,720,true);
    g.setGradientFill(halo);
    g.fillEllipse(115.0f,67.0f,701.0f,676.0f);
    g.setColour(juce::Colour(0xff131519));g.fillRect(0,585,956,2);
}
void SkellFanAudioProcessorEditor::PlateOverlay::paint(juce::Graphics& g)
{
    g.addTransform(juce::AffineTransform::scale(getWidth()/Theme::W,getHeight()/Theme::H));
    // Body is a separate static PNG. Plate is intentionally freshly drawn on top
    // so original photographed labels never become mistaken for live controls.
    auto r=Theme::plateRect;
    // Hardware bezel: shaded anodised aluminium rather than a flat
    // neon-green outline on the fan's physical control deck.
    const auto bezel=r.expanded(13.0f,9.0f);
    g.setColour(juce::Colours::black.withAlpha(0.29f));
    g.fillRoundedRectangle(bezel.translated(2.0f,3.0f),22.0f);
    juce::ColourGradient rim(juce::Colour(0xff4ea366),bezel.getTopLeft(),
                            juce::Colour(0xff163d27),bezel.getBottomRight(),false);
    rim.addColour(0.45,juce::Colour(0xff245e38));
    g.setGradientFill(rim);
    g.fillRoundedRectangle(bezel,22.0f);
    g.setColour(juce::Colour(0xffb2ddbc).withAlpha(0.55f));
    g.drawRoundedRectangle(bezel.reduced(1.1f),21.0f,1.2f);
    g.setColour(juce::Colour(0xff071309).withAlpha(0.64f));
    g.drawRoundedRectangle(r.expanded(5.0f,4.0f),16.0f,1.7f);
    Theme::metal(g,r);
    // Small machined cross screws are drawn on the outer bezel; they never
    // obstruct knobs, DEPTH, preset keys, or the engraved bottom nameplate.
    constexpr std::array<std::array<float,2>,4> screws {{
        {225.0f,744.0f},{727.0f,744.0f},
        {225.0f,899.0f},{727.0f,899.0f}
    }};
    for (const auto& screw : screws)
    {
        const float cx=screw[0],cy=screw[1];
        g.setColour(juce::Colour(0xff041008).withAlpha(0.7f));
        g.fillEllipse(cx-5.1f,cy-5.1f,10.2f,10.2f);
        juce::ColourGradient steel(juce::Colour(0xfff5fff8),cx-3.5f,cy-3.5f,
                                   juce::Colour(0xff64766a),cx+4.0f,cy+4.0f,false);
        g.setGradientFill(steel);
        g.fillEllipse(cx-3.8f,cy-3.8f,7.6f,7.6f);
        g.setColour(juce::Colour(0xff314038));
        g.drawLine(cx-2.4f,cy-2.4f,cx+2.4f,cy+2.4f,0.9f);
        g.drawLine(cx+2.4f,cy-2.4f,cx-2.4f,cy+2.4f,0.9f);
    }
    // Fine horizontal brush strokes.
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    for(int y=745;y<902;y+=3)g.drawLine(235.0f,static_cast<float>(y),718.0f,static_cast<float>(y),.5f);
    // Compact labels remain separated at the 574 px minimum editor width.
    // The mode information panel retains the full effect names.
    label(g,"VIB",{246,885,51,20},11.0f);
    label(g,"ROT",{301,885,50,20},11.0f);
    label(g,"TRM",{354,885,51,20},11.0f);
    label(g,"CHO",{408,885,49,20},11.0f);
    // Read the actual sync mode: never display Hz ticks for host divisions.
    const bool hostSync = processor.apvts.getRawParameterValue("syncMode")
                              ->load(std::memory_order_relaxed) > 0.5f;
    if (hostSync)
    {
        label(g,"1/2",{462,807,30,22},10);
        label(g,"1/32",{542,807,38,22},10);
        label(g,"1/1",{463,859,30,22},10);
        label(g,"1/64",{542,859,38,22},10);
    }
    else
    {
        label(g,"8",{464,807,28,22},12);
        label(g,"24",{547,807,27,22},12);
        label(g,"0",{465,859,27,22},12);
        label(g,"32",{549,859,28,22},12);
    }
    // Platform-independent wave labels: decorative Unicode glyphs showed up
    // as replacement characters on the Windows CI system font.
    label(g,"SIN",{577,807,31,26},10.0f);
    label(g,"TRI",{577,855,31,26},10.0f);
    label(g,"SQR",{687,807,31,26},10.0f);
    label(g,"SAW",{687,853,31,26},10.0f);
    // Proper recessed nameplate covers the uneven moulded plastic and
    // replaces the old text-over-image treatment. One unbroken wordmark.
    const juce::Rectangle<float> badge{354.0f,937.0f,248.0f,39.0f};
    g.setColour(juce::Colour(0xff0b130d).withAlpha(0.94f));
    g.fillRoundedRectangle(badge,9.0f);
    g.setColour(juce::Colour(0xff84918a).withAlpha(0.83f));
    g.drawRoundedRectangle(badge.reduced(0.8f),8.0f,1.3f);
    g.setColour(juce::Colour(0xff58db78).withAlpha(0.55f));
    g.drawLine(371.0f,941.0f,585.0f,941.0f,1.0f);
    juce::AttributedString brand;
    brand.setJustification(juce::Justification::centred);
    brand.append("SKELL",Theme::font(24.0f,true),juce::Colour(0xffe6eee8));
    brand.append("FAN",Theme::font(24.0f,true),juce::Colour(0xff70f18b));
    brand.draw(g,juce::Rectangle<float>{363.0f,940.0f,230.0f,32.0f});
}

void SkellFanAudioProcessorEditor::RateReadout::paint(juce::Graphics& g)
{
    const auto read=[this](const char* id)
    { return processor.apvts.getRawParameterValue(id)->load(std::memory_order_relaxed); };
    const bool synced=read("syncMode")>0.5f;
    juce::String text;
    if(synced)
    {
        constexpr const char* divisions[]={"1/1","1/2","1/4","1/8","1/16","1/32","1/64"};
        constexpr const char* note[]={""," DOT"," TRI"};
        text=juce::String("HOST  ")+divisions[juce::jlimit(0,6,static_cast<int>(read("syncDiv")))]
            +note[juce::jlimit(0,2,static_cast<int>(read("noteType")))]
            +" | "+juce::String(processor.displayBpm.load(std::memory_order_relaxed),0)+" BPM";
    }
    else
        text="MANUAL | "+juce::String(processor.displayRate.load(std::memory_order_relaxed),2)+" Hz";
    auto r=getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff0d1b12).withAlpha(.88f));
    g.fillRoundedRectangle(r,6.0f);
    g.setColour(juce::Colour(0xff4fd673).withAlpha(.62f));
    g.drawRoundedRectangle(r,6.0f,1.0f);
    g.setColour(juce::Colour(0xffe1f5e6));
    g.setFont(Theme::font(juce::jmax(9.0f,12.0f*getHeight()/23.0f),true));
    g.drawText(text,getLocalBounds().reduced(5,1),juce::Justification::centred);
}

SkellFanAudioProcessorEditor::SkellFanAudioProcessorEditor(SkellFanAudioProcessor& p)
    :AudioProcessorEditor(&p),plate(p),processor(p),fan(p),top(p),presets(p),depth(p),
     rate(p,"rate","RATE"),syncRate(p,"syncDiv","RATE"),wave(p,"wave","WAVE"),
     saturation(p,"saturation","",true,true),scope(p),rateReadout(p),
     outputMeter(p),modeInfo(p)
{
    setLookAndFeel(&theme);
    for(auto* c:{static_cast<juce::Component*>(&stage),static_cast<juce::Component*>(&fan),
                 static_cast<juce::Component*>(&plate),static_cast<juce::Component*>(&top),
                 static_cast<juce::Component*>(&presets),static_cast<juce::Component*>(&depth),
                 static_cast<juce::Component*>(&rate),static_cast<juce::Component*>(&syncRate),
                 static_cast<juce::Component*>(&wave),static_cast<juce::Component*>(&saturation)})
        addAndMakeVisible(c);
    fanSwitch.setButtonText("||");
    fanSwitch.setTooltip("Pause the fan animation (does not affect audio)");
    fanSwitch.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff18231b));
    fanSwitch.setColour(juce::TextButton::textColourOffId,juce::Colour(0xffeaf1f3));
    fanSwitch.onClick=[this]
    {
        bool enabled=static_cast<bool>(processor.apvts.state.getProperty("uiFanAnimation",true));
        enabled=!enabled;
        processor.apvts.state.setProperty("uiFanAnimation",enabled,nullptr);
        fan.setAnimationEnabled(enabled);
        fanSwitch.setButtonText(enabled?"||":">");
    };
    addAndMakeVisible(fanSwitch);
    addAndMakeVisible(rateReadout);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(modeInfo);
    const bool initialAnimation=static_cast<bool>(p.apvts.state.getProperty("uiFanAnimation",true));
    fan.setAnimationEnabled(initialAnimation);
    fanSwitch.setButtonText(initialAnimation?"||":">");
    scope.setVisible(static_cast<bool>(p.apvts.state.getProperty("uiScopeVisible",false)));
    addChildComponent(scope);
    top.onScope=[this]
    {
        const bool enabled=!scope.isVisible();
        scope.setVisible(enabled);
        processor.apvts.state.setProperty("uiScopeVisible",enabled,nullptr);
    };
    const juce::StringArray modes{"VIBRATO","ROTARY","TREMOLO","CHORUS"};
    const juce::Colour colours[]{juce::Colour(0xffe4e4e2),Theme::cyan,
                                Theme::lightBlue,Theme::deepBlue};
    for(int i=0;i<4;++i)
    {
        modeKeys[i]=std::make_unique<KeyButton>(modes[i],colours[i]);
        modeKeys[i]->onClick=[this,i]{setChoice(modeChoice,i);};
        modeKeys[i]->setTooltip("Select "+modes[i]+" modulation");
        addAndMakeVisible(*modeKeys[i]);
        modeChoice.addItem(modes[i],i+1);
    }
    for(int i=0;i<2;++i)
    {
        syncKeys[i]=std::make_unique<KeyButton>(i==0?"M":"H",
                   i==0?juce::Colour(0xff8eb79a):juce::Colour(0xff8ae8a0),true);
        syncKeys[i]->onClick=[this,i]{setChoice(syncChoice,i);};
        syncKeys[i]->setTooltip(i==0?"Manual LFO rate":"Host tempo sync");
        addAndMakeVisible(*syncKeys[i]);
        syncChoice.addItem(i==0?"Manual":"Host",i+1);
    }
    const char* notes[]={"S","D","T"};
    for(int i=0;i<3;++i)
    {
        noteKeys[i]=std::make_unique<KeyButton>(notes[i],juce::Colour(0xffd6dadd),true);
        noteKeys[i]->onClick=[this,i]{setChoice(noteChoice,i);};
        noteKeys[i]->setTooltip(i==0?"Straight notes":i==1?"Dotted notes":"Triplet notes");
        addAndMakeVisible(*noteKeys[i]);
        noteChoice.addItem(i==0?"Straight":i==1?"Dotted":"Triplet",i+1);
    }
    addChildComponent(modeChoice);addChildComponent(syncChoice);addChildComponent(noteChoice);
    modeAttach=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts,"mode",modeChoice);
    syncAttach=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts,"syncMode",syncChoice);
    noteAttach=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts,"noteType",noteChoice);
    constrainer.setSizeLimits(574,590,1434,1476);
    constrainer.setFixedAspectRatio(static_cast<double>(Theme::W)/Theme::H);
    setConstrainer(&constrainer);
    setResizable(true,true);
    setSize(static_cast<int>(p.apvts.state.getProperty("uiWidth",956)),
            static_cast<int>(p.apvts.state.getProperty("uiHeight",984)));
    // Hide the inactive RATE dial immediately (before the first 30 Hz timer).
    const bool hostInitiallySelected =
        p.apvts.getRawParameterValue("syncMode")->load(std::memory_order_relaxed) > 0.5f;
    rate.setVisible(!hostInitiallySelected);
    syncRate.setVisible(hostInitiallySelected);
    if (hostInitiallySelected) syncRate.toFront(false);
    else rate.toFront(false);
    startTimerHz(30);
}
SkellFanAudioProcessorEditor::~SkellFanAudioProcessorEditor()
{
    stopTimer();
    setConstrainer(nullptr);
    setLookAndFeel(nullptr);
}
void SkellFanAudioProcessorEditor::setChoice(juce::ComboBox& combo,int value)
{combo.setSelectedId(value+1,juce::sendNotificationSync);}
void SkellFanAudioProcessorEditor::resized()
{
    const float sx=getWidth()/Theme::W,sy=getHeight()/Theme::H;
    auto place=[sx,sy](juce::Component& c,float x,float y,float w,float h)
    {c.setBounds(juce::roundToInt(x*sx),juce::roundToInt(y*sy),
                 juce::roundToInt(w*sx),juce::roundToInt(h*sy));};
    stage.setBounds(getLocalBounds());fan.setBounds(getLocalBounds());
    plate.setBounds(getLocalBounds());
    place(top,0,0,956,60);place(presets,215,10,408,38);
    place(fanSwitch,284,697,36,35);
    place(depth,254,739,203,64);
    const float modeX[]={246,301,354,408};const float modeW[]={51,50,51,49};
    for(int i=0;i<4;++i)place(*modeKeys[i],modeX[i],807,modeW[i],75);
    place(*syncKeys[0],485,752,36,30);place(*syncKeys[1],522,752,36,30);
    const float noteX[]={565,603,641};
    for(int i=0;i<3;++i)place(*noteKeys[i],noteX[i],752,36,30);
    place(rate,476,791,85,104);place(syncRate,476,791,85,104);
    place(wave,601,791,85,104);place(saturation,648,624,57,70);
    place(scope,18,729,200,136);
    place(rateReadout,476,907,239,24);
    place(outputMeter,25,743,116,183);
    place(modeInfo,778,743,155,183);
    // Non-visible ComboBoxes stay out of the mouse hit region.
    processor.apvts.state.setProperty("uiWidth",getWidth(),nullptr);
    processor.apvts.state.setProperty("uiHeight",getHeight(),nullptr);
}
void SkellFanAudioProcessorEditor::timerCallback()
{
    const auto get=[this](const char* id){return processor.apvts.getRawParameterValue(id)->load();};
    const int m=juce::jlimit(0,3,static_cast<int>(get("mode")));
    const int s=juce::jlimit(0,1,static_cast<int>(get("syncMode")));
    const int t=juce::jlimit(0,2,static_cast<int>(get("noteType")));
    for(int i=0;i<4;++i)modeKeys[i]->setToggleState(i==m,juce::dontSendNotification);
    for(int i=0;i<2;++i)syncKeys[i]->setToggleState(i==s,juce::dontSendNotification);
    for(int i=0;i<3;++i)
    {
        noteKeys[i]->setEnabled(s==1);
        noteKeys[i]->setAlpha(s==1?1.0f:.4f);
        noteKeys[i]->setToggleState(i==t,juce::dontSendNotification);
    }
    // The editor is refreshed at 30 Hz, but changing the z-order every tick
    // causes unnecessary component invalidations. Update only on sync flips.
    if (lastPlateSync != s)
    {
        lastPlateSync = s;
        rate.setVisible(s==0);
        syncRate.setVisible(s!=0);
        if (s!=0) syncRate.toFront(false);
        else rate.toFront(false);
        plate.repaint(); // cached markings switch only MANUAL <-> HOST
    }
    top.refreshSkin();
    rateReadout.repaint();
    outputMeter.refresh();
    modeInfo.refresh();
}
