#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/ui/TopBar.h"
#include "../Source/ui/Theme.h"
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <chrono>
#include <limits>

namespace
{
int failures=0;
void check(bool condition, const char* label)
{
    if (!condition) {++failures;std::cerr<<"FAIL: "<<label<<'\n';}
    else std::cout<<"PASS: "<<label<<'\n';
}

void setParameter(SkellFanAudioProcessor& processor, const char* id, float value)
{
    if (auto* param=processor.apvts.getParameter(id))
        param->setValueNotifyingHost(param->convertTo0to1(value));
    else
        check(false,"Audio parameter exists");
}

void testProcessing(SkellFanAudioProcessor& processor)
{
    processor.prepareToPlay(48000.0,512);
    setParameter(processor,"saturation",0.0f);
    setParameter(processor,"depth",0.0f);
    setParameter(processor,"mix",50.0f);
    setParameter(processor,"power",1.0f);
    juce::AudioBuffer<float> buffer(2,512);
    juce::MidiBuffer midi;
    bool finite=true, noPeakBoost=true;
    for(int mode=0;mode<4;++mode)
    {
        double processingMs=0.0;
        setParameter(processor,"mode",static_cast<float>(mode));
        for(int block=0;block<48;++block)
        {
            for(int n=0;n<512;++n)
            {
                const float t=static_cast<float>(mode*48*512+block*512+n)/48000.0f;
                const float v=0.7f*std::sin(juce::MathConstants<float>::twoPi*997.0f*t);
                buffer.setSample(0,n,v);
                buffer.setSample(1,n,-v);
            }
            const auto began=std::chrono::steady_clock::now();
            processor.processBlock(buffer,midi);
            processingMs += std::chrono::duration<double,std::milli>(
                std::chrono::steady_clock::now()-began).count();
            for(int ch=0;ch<2;++ch)
                for(int n=0;n<512;++n)
                {
                    const float v=buffer.getSample(ch,n);
                    finite &= std::isfinite(v);
                    noPeakBoost &= std::abs(v)<=0.95f;
                }
        }
        const double fraction=processingMs/(1000.0*48.0*512.0/48000.0);
        std::cout << "Mode " << mode << " CPU: " << processingMs
                  << " ms / 512 ms audio (" << 100.0*fraction << "% RT)\n";
        check(std::isfinite(fraction) && fraction<0.9,
              "Real processor mode completes under realtime deadline");
    }
    check(finite,"All four real processing modes remain finite");
    check(noPeakBoost,"0.7-peak mode transitions remain below 0 dBFS");

    setParameter(processor,"depth",32.0f);
    juce::MemoryBlock savedState;
    processor.getStateInformation(savedState);
    setParameter(processor,"depth",83.0f);
    processor.setStateInformation(savedState.getData(),
                                  static_cast<int>(savedState.getSize()));
    const float restored=processor.apvts.getRawParameterValue("depth")->load();
    check(std::abs(restored-32.0f)<0.2f,"APVTS state save/load roundtrip");
}

void testInvalidInputAndSilentTail()
{
    SkellFanAudioProcessor processor;
    processor.prepareToPlay(48000.0,256);
    setParameter(processor,"saturation",0.0f);
    setParameter(processor,"mix",65.0f);
    setParameter(processor,"depth",75.0f);
    juce::AudioBuffer<float> audio(2,256);
    juce::MidiBuffer midi;
    bool safe=true;
    for (int mode=0; mode<4; ++mode)
    {
        setParameter(processor,"mode",static_cast<float>(mode));
        for (int block=0; block<16; ++block)
        {
            audio.clear();
            if (block==0)
            {
                audio.setSample(0,5,std::numeric_limits<float>::quiet_NaN());
                audio.setSample(1,9,std::numeric_limits<float>::infinity());
                audio.setSample(0,13,-std::numeric_limits<float>::infinity());
            }
            processor.processBlock(audio,midi);
            for (int channel=0;channel<2;++channel)
                for (int sample=0;sample<256;++sample)
                    safe &= std::isfinite(audio.getSample(channel,sample));
        }
    }
    check(safe,"All real DSP modes safely process NaN/Inf upstream input");
    check(std::isfinite(processor.displayRms[0].load())
          && std::isfinite(processor.displayRms[1].load())
          && std::isfinite(processor.displayPeak[0].load())
          && std::isfinite(processor.displayPeak[1].load()),
          "Audio meters remain finite after hostile input");
}

void testEditor(SkellFanAudioProcessor& processor)
{
    // Start the editor from a real factory preset so the preset bar's
    // snapshot matches its label. The preceding DSP test intentionally
    // leaves modified values; otherwise an incorrect (*) appears in
    // the published preview even after restoring Chorus parameters.
    processor.loadFactoryPreset(0);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    check(editor != nullptr,"Editor created");
    if (!editor) return;

    // Buttons are actual JUCE children, not a mock layout.
    juce::Button* rotaryButton=nullptr;
    for (int i=0;i<editor->getNumChildComponents();++i)
        if (auto* child=editor->getChildComponent(i))
            if (child->getName()=="ROTARY")
                rotaryButton=dynamic_cast<juce::Button*>(child);
    check(rotaryButton!=nullptr,"Rotary mode control exists and is clickable");
    if (rotaryButton)
    {
        rotaryButton->triggerClick();
        juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
        check(static_cast<int>(processor.apvts.getRawParameterValue("mode")->load())==1,
              "Clicking ROTARY selects the DSP mode");
    }

    // Shared SkullTheme is UI-only and saves a non-automatable appearance
    // property; clicking the mark must never change active DSP parameters.
    juce::Button* skullButton=nullptr;
    for (int i=0;i<editor->getNumChildComponents();++i)
        if (auto* top=dynamic_cast<TopBar*>(editor->getChildComponent(i)))
            for (int j=0;j<top->getNumChildComponents();++j)
                if (auto* child=top->getChildComponent(j))
                    if (child->getName()=="SkellFan fan-skull")
                        skullButton=dynamic_cast<juce::Button*>(child);
    check(skullButton!=nullptr,"SkellCore shared skull toggle exists");
    if (skullButton)
    {
        const float depthBefore=processor.apvts.getRawParameterValue("depth")->load();
        const int skinBefore=static_cast<int>(
            processor.apvts.state.getProperty("uiSkullSkin",1));
        skullButton->triggerClick();
        juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
        check(static_cast<int>(processor.apvts.state.getProperty("uiSkullSkin",1))
              ==(skinBefore+1)%4,"Clicking shared skull cycles accent skin");
        check(processor.apvts.getRawParameterValue("depth")->load()==depthBefore,
              "Shared skull theme does not alter audio parameters");
        // Restore the initial Neon skin so exported screenshots represent the
        // plugin's default appearance, not the single-click Ivory test state.
        for (int cycle=0; cycle<3; ++cycle) skullButton->triggerClick();
        juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
        check(static_cast<int>(processor.apvts.state.getProperty("uiSkullSkin",1))
              ==skinBefore,"SkellCore accent selection wraps back to the default");
    }

    // UI placement is checked against actual JUCE bounds; Power must be
    // rightmost and the fan/skull control must be first at every size.
    juce::Button* powerButton=nullptr;
    TopBar* header=nullptr;
    for (int i=0;i<editor->getNumChildComponents();++i)
        if (auto* candidate=dynamic_cast<TopBar*>(editor->getChildComponent(i)))
        {
            header=candidate;
            for (int j=0;j<header->getNumChildComponents();++j)
                if (auto* child=header->getChildComponent(j))
                    if (child->getName()=="SkellFan power")
                        powerButton=dynamic_cast<juce::Button*>(child);
        }
    check(powerButton && skullButton,"Power and three-blade fan/skull are clickable");
    if (powerButton && skullButton && header)
    {
        check(skullButton->getX()<powerButton->getX(),
              "Fan/skull is left of power");
        check(powerButton->getRight()<=header->getWidth(),
              "Rightmost power control is inside top bar");
        const float before=processor.apvts.getRawParameterValue("power")->load();
        powerButton->triggerClick();
        juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
        check(processor.apvts.getRawParameterValue("power")->load()!=(before>0.5f?1.0f:0.0f),
              "Rightmost Power button is connected to bypass");
        powerButton->triggerClick();
        juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
    }
    const juce::Colour blue(0xff18a9df);
    const juce::Colour gray(0xff858585);
    juce::Image preview(juce::Image::ARGB,2,1,true);
    preview.setPixelAt(0,0,blue);
    preview.setPixelAt(1,0,gray);
    const auto green=Theme::greenFanArtwork(preview);
    check(green.getPixelAt(0,0).getGreen()>green.getPixelAt(0,0).getBlue(),
          "Blue fan paint is converted to green without changed alpha");
    check(green.getPixelAt(1,0)==gray,
          "Neutral chrome artwork remains unchanged");

    // The integration test clicks ROTARY and Power above. Restore the
    // intended startup preset before screenshots so the displayed factory
    // name and DSP mode agree, rather than "Chorus - Classic *" / ROTARY.
    processor.loadFactoryPreset(0);
    juce::MessageManager::getInstance()->runDispatchLoopUntil(250);
    check(static_cast<int>(processor.apvts.getRawParameterValue("mode")->load())==3
          && processor.apvts.state.getProperty("uiPresetName").toString()
                 =="Chorus - Classic",
          "Screenshots restore the actual factory Chorus startup state");

    const std::array<std::array<int,2>,4> sizes {{{574,590},{765,787},{956,984},{1434,1476}}};
    juce::PNGImageFormat png;
    for(const auto& sz:sizes)
    {
        editor->setSize(sz[0],sz[1]);
        const auto bounds=editor->getLocalBounds();
        bool inside=true;
        for(int i=0;i<editor->getNumChildComponents();++i)
        {
            auto* child=editor->getChildComponent(i);
            if (child && child->isVisible())
                inside &= bounds.contains(child->getBounds());
        }
        check(inside,"Visible editor components fit the selected window size");
        const auto snapshot=editor->createComponentSnapshot(bounds,true);
        check(snapshot.isValid() && snapshot.getWidth()==sz[0]
              && snapshot.getHeight()==sz[1],"JUCE editor renders at requested size");
        if(!snapshot.isValid())continue;
        check(snapshot.getPixelAt(sz[0]/2,sz[1]/2).getAlpha()!=0,
              "Fan hub is visible in actual JUCE snapshot");
        const auto file=juce::File::getCurrentWorkingDirectory().getChildFile(
            "SkellFan-ui-"+juce::String(sz[0])+"x"+juce::String(sz[1])+".png");
        juce::FileOutputStream output(file);
        check(output.openedOk() && png.writeImageToStream(snapshot,output),
              "Editor screenshot saved");
    }
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    SkellFanAudioProcessor processor;
    testProcessing(processor);
    testInvalidInputAndSilentTail();
    testEditor(processor);
    std::cout<<(failures==0?"All UI/processor tests passed\n":"UI/processor tests failed\n");
    return failures==0?0:1;
}
