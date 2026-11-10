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
void BreezeModAudioProcessorEditor::Stage::paint(juce::Graphics& g)
{
    g.addTransform(juce::AffineTransform::scale(getWidth()/Theme::W,getHeight()/Theme::H));
    g.fillAll(Theme::black);
    juce::ColourGradient floor(juce::Colour(0xff15171a),0,700,
                               juce::Colour(0xff040405),0,984,false);
    floor.addColour(0.35,juce::Colour(0xff090b0e));
    g.setGradientFill(floor);g.fillRect(0,585,956,399);
    // Cached ambient light under the fan; no additional bitmap or frame cost.
    juce::ColourGradient halo(juce::Colour(0xff225579).withAlpha(0.19f),466,376,
                              juce::Colours::transparentBlack,810,720,true);
    g.setGradientFill(halo);
    g.fillEllipse(115.0f,67.0f,701.0f,676.0f);
    g.setColour(juce::Colour(0xff131519));g.fillRect(0,585,956,2);
}
void BreezeModAudioProcessorEditor::PlateOverlay::paint(juce::Graphics& g)
{
    g.addTransform(juce::AffineTransform::scale(getWidth()/Theme::W,getHeight()/Theme::H));
    // Body is a separate static PNG. Plate is intentionally freshly drawn on top
    // so original photographed labels never become mistaken for live controls.
    auto r=Theme::plateRect;
    g.setColour(juce::Colour(0xff1681bf));g.fillRoundedRectangle(r.expanded(13.0f,9.0f),22.0f);
    Theme::metal(g,r);
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
    // Original wordmark; no existing marks are drawn.
    g.setColour(juce::Colour(0xff2f3338));
    g.setFont(Theme::font(24.0f,true));
    g.drawText("B R E E Z E M O D",363,938,227,35,juce::Justification::centred);
    g.setColour(juce::Colour(0xff4791ad));
    g.fillEllipse(345.0f,954.0f,8.0f,8.0f);
    g.fillEllipse(600.0f,954.0f,8.0f,8.0f);
}

void BreezeModAudioProcessorEditor::RateReadout::paint(juce::Graphics& g)
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
    g.setColour(juce::Colour(0xff111a23).withAlpha(.88f));
    g.fillRoundedRectangle(r,6.0f);
    g.setColour(juce::Colour(0xff469bc0).withAlpha(.62f));
    g.drawRoundedRectangle(r,6.0f,1.0f);
    g.setColour(juce::Colour(0xffdaf2fa));
    g.setFont(Theme::font(juce::jmax(9.0f,12.0f*getHeight()/23.0f),true));
    g.drawText(text,getLocalBounds().reduced(5,1),juce::Justification::centred);
}

BreezeModAudioProcessorEditor::BreezeModAudioProcessorEditor(BreezeModAudioProcessor& p)
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
    fanSwitch.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff252b30));
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
                   i==0?juce::Colour(0xffa9c4be):juce::Colour(0xffc9a3e8),true);
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
BreezeModAudioProcessorEditor::~BreezeModAudioProcessorEditor()
{
    stopTimer();
    setConstrainer(nullptr);
    setLookAndFeel(nullptr);
}
void BreezeModAudioProcessorEditor::setChoice(juce::ComboBox& combo,int value)
{combo.setSelectedId(value+1,juce::sendNotificationSync);}
void BreezeModAudioProcessorEditor::resized()
{
    const float sx=getWidth()/Theme::W,sy=getHeight()/Theme::H;
    auto place=[sx,sy](juce::Component& c,float x,float y,float w,float h)
    {c.setBounds(juce::roundToInt(x*sx),juce::roundToInt(y*sy),
                 juce::roundToInt(w*sx),juce::roundToInt(h*sy));};
    stage.setBounds(getLocalBounds());fan.setBounds(getLocalBounds());
    plate.setBounds(getLocalBounds());
    place(top,0,0,956,60);place(presets,242,10,420,38);
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
void BreezeModAudioProcessorEditor::timerCallback()
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
    rateReadout.repaint();
    outputMeter.refresh();
    modeInfo.refresh();
}
