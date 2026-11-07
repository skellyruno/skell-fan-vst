#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
using APVTS=juce::AudioProcessorValueTreeState;
using PI=juce::MathConstants<float>;
constexpr std::array<const char*,11> paramIds {
    "power","mode","depth","rate","syncMode","syncDiv","noteType",
    "wave","saturation","width","mix" };
}

BreezeModAudioProcessor::BreezeModAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
                                  .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
      apvts(*this,nullptr,"BREEZEMOD_PARAMETERS",makeLayout())
{
    apvts.state.setProperty("uiPresetName","Chorus - Classic",nullptr);
    apvts.state.setProperty("uiFanAnimation",true,nullptr);
    apvts.state.setProperty("uiScopeVisible",false,nullptr);
    apvts.state.setProperty("uiWidth",765,nullptr);
    apvts.state.setProperty("uiHeight",787,nullptr);
}

APVTS::ParameterLayout BreezeModAudioProcessor::makeLayout()
{
    using namespace juce;
    APVTS::ParameterLayout result;
    result.add(std::make_unique<AudioParameterBool>(ParameterID{"power",1},"Power",true));
    result.add(std::make_unique<AudioParameterChoice>(ParameterID{"mode",1},"Effect Mode",
        StringArray{"Vibrato","Rotary","Tremolo","Chorus"},3));
    result.add(std::make_unique<AudioParameterFloat>(ParameterID{"depth",1},"Depth",
        NormalisableRange<float>(0.0f,100.0f,0.1f),50.0f));
    result.add(std::make_unique<AudioParameterFloat>(ParameterID{"rate",1},"Rate Hz",
        NormalisableRange<float>(0.0f,32.0f,0.01f,0.45f),3.0f));
    result.add(std::make_unique<AudioParameterChoice>(ParameterID{"syncMode",1},"Sync Mode",
        StringArray{"Manual","Host"},0));
    result.add(std::make_unique<AudioParameterChoice>(ParameterID{"syncDiv",1},"Sync Division",
        StringArray{"1/1","1/2","1/4","1/8","1/16","1/32","1/64"},3));
    result.add(std::make_unique<AudioParameterChoice>(ParameterID{"noteType",1},"Note Type",
        StringArray{"Straight","Dotted","Triplet"},0));
    result.add(std::make_unique<AudioParameterChoice>(ParameterID{"wave",1},"Waveform",
        StringArray{"Sine","Triangle","Square","Saw"},0));
    result.add(std::make_unique<AudioParameterFloat>(ParameterID{"saturation",1},"Saturation",
        NormalisableRange<float>(0.0f,100.0f,0.1f),20.0f));
    result.add(std::make_unique<AudioParameterFloat>(ParameterID{"width",1},"Width",
        NormalisableRange<float>(0.0f,100.0f,0.1f),70.0f));
    result.add(std::make_unique<AudioParameterFloat>(ParameterID{"mix",1},"Dry Wet",
        NormalisableRange<float>(0.0f,100.0f,0.1f),50.0f));
    return result;
}

bool BreezeModAudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto in=layout.getMainInputChannelSet();
    return (in==juce::AudioChannelSet::mono()||in==juce::AudioChannelSet::stereo())
        && layout.getMainOutputChannelSet()==in;
}

void BreezeModAudioProcessor::prepareToPlay(double rate,int block)
{
    sr=juce::jmax(8000.0,rate);
    maxBlock=juce::jmax(16,block);
    saturation.prepare(sr,maxBlock,p("saturation")*.01f);
    // Polyphase oversampling is an intentional reported latency. Dry is delayed to match.
    setLatencySamples(saturation.getLatency());
    dryCompensation=juce::roundToInt(sr*0.004);
    lfo.prepare(sr);
    vibratoDelay.prepare(sr,20.0);
    chorusDelay.prepare(sr,45.0);
    dryDelay.prepare(sr,35.0);
    rotary.prepare(sr);
    for(int ch=0; ch<2; ++ch)
    {
        vibratoMotion[static_cast<size_t>(ch)].prepare(sr, 1.5, 4.0f);
        tremoloMotion[static_cast<size_t>(ch)].prepare(sr, 1.4, 0.0f);
        for(int voice=0; voice<3; ++voice)
            chorusMotion[static_cast<size_t>(ch)][static_cast<size_t>(voice)]
                .prepare(sr, 2.0, 12.0f + voice * 4.0f);
    }
    for(int ch=0; ch<2; ++ch)
    {
        displayRms[static_cast<size_t>(ch)].store(0.0f, std::memory_order_relaxed);
        displayPeak[static_cast<size_t>(ch)].store(0.0f, std::memory_order_relaxed);
    }
    original.setSize(2,maxBlock,false,true,true);
    processed.setSize(2,maxBlock,false,true,true);
    modeFader.prepare(juce::jlimit(0,3,static_cast<int>(p("mode"))),
                      juce::jmax(1,juce::roundToInt(sr*0.020)));
    wasHostSynced=wasTransportPlaying=haveLastPPQ=false;
    lastSyncDivision=lastNoteType=-1;
    lastPPQ=0.0;
    lastHostBpm=120.0;
    lastBlockSamples=0;
    auto reset=[this](auto& v,float value,double seconds)
    {v.reset(sr,seconds);v.setCurrentAndTargetValue(value);};
    reset(depthSmooth,p("depth")*.01f,.025);
    reset(widthSmooth,p("width")*.01f,.035);
    const float initialMix=juce::jlimit(0.0f,1.0f,p("mix")*.01f);
    reset(mixSmooth,initialMix,.025);
    reset(dryGainSmooth,std::cos(initialMix*PI::halfPi),.025);
    reset(wetGainSmooth,std::sin(initialMix*PI::halfPi),.025);
    reset(powerSmooth,p("power"),.012);
    const float initialWidth=juce::jlimit(0.0f,1.0f,p("width")*.01f);
    for(int ch=0;ch<2;++ch)
        for(int k=0;k<3;++k)
        {
            const float pan=(static_cast<float>(k)-1.0f)*initialWidth;
            const float balance=ch==0?(1.0f-pan): (1.0f+pan);
            reset(chorusGains[static_cast<size_t>(ch)][static_cast<size_t>(k)],
                  std::sqrt(juce::jmax(0.0f,balance)*0.5f),.035);
        }
    displayRate.store(p("rate"),std::memory_order_relaxed);
    lfoPhase.store(0.0f,std::memory_order_relaxed);
}

void BreezeModAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels=buffer.getNumChannels(), n=buffer.getNumSamples();
    if(channels<1||n<1)return;
    std::array<double, 2> meterEnergy {{0.0, 0.0}};
    std::array<float, 2> meterPeak {{0.0f, 0.0f}};
    const int wanted=juce::jlimit(0,3,static_cast<int>(p("mode")));
    modeFader.setTarget(wanted);
    depthSmooth.setTargetValue(p("depth")*.01f);
    widthSmooth.setTargetValue(p("width")*.01f);
    const float mixTarget=juce::jlimit(0.0f,1.0f,p("mix")*.01f);
    mixSmooth.setTargetValue(mixTarget);
    // Equal-power trigonometry is computed once per host block, rather than
    // calling sin + cos at every audio sample.
    dryGainSmooth.setTargetValue(std::cos(mixTarget*PI::halfPi));
    wetGainSmooth.setTargetValue(std::sin(mixTarget*PI::halfPi));
    powerSmooth.setTargetValue(p("power")>0.5f?1.0f:0.0f);
    const float widthTarget=juce::jlimit(0.0f,1.0f,p("width")*.01f);
    for(int ch=0;ch<2;++ch)
        for(int k=0;k<3;++k)
        {
            const float pan=(static_cast<float>(k)-1.0f)*widthTarget;
            const float balance=ch==0?(1.0f-pan):(1.0f+pan);
            chorusGains[static_cast<size_t>(ch)][static_cast<size_t>(k)].setTargetValue(
                std::sqrt(juce::jmax(0.0f,balance)*0.5f));
        }
    const bool hostSync=p("syncMode")>=0.5f;
    const int syncDiv=juce::jlimit(0,6,static_cast<int>(p("syncDiv")));
    const int noteType=juce::jlimit(0,2,static_cast<int>(p("noteType")));
    const int wave=juce::jlimit(0,3,static_cast<int>(p("wave")));
    const double beats=breeze::dsp::hostCycleBeats(syncDiv,noteType);
    double bpm=120.0, ppq=0.0;
    bool transportPlaying=false, validPPQ=false;
    if (auto* playhead=getPlayHead())
        if (auto pos=playhead->getPosition())
        {
            if(auto hostBpm=pos->getBpm())bpm=*hostBpm;
            transportPlaying=pos->getIsPlaying();
            if(auto hostPPQ=pos->getPpqPosition())
            {
                ppq=*hostPPQ;
                validPPQ=std::isfinite(ppq);
            }
        }
    if(!std::isfinite(bpm)||bpm<20.0||bpm>400.0)bpm=120.0;
    displayBpm.store(static_cast<float>(bpm),std::memory_order_relaxed);
    const double hz=hostSync?bpm/(60.0*beats):static_cast<double>(p("rate"));
    // Only hard-lock PPQ on entry/playback or a real seek/loop jump.
    // Re-locking on every audio block while rate-smoothing caused phase clicks.
    if(hostSync && transportPlaying && validPPQ)
    {
        const double expected=lastPPQ
               + static_cast<double>(lastBlockSamples)*lastHostBpm/(sr*60.0);
        const bool jumped=haveLastPPQ && std::abs(ppq-expected)>0.05;
        if(!wasHostSynced || !wasTransportPlaying || !haveLastPPQ
           || jumped || syncDiv!=lastSyncDivision || noteType!=lastNoteType)
            lfo.lockToPPQ(ppq,beats);
    }
    haveLastPPQ=validPPQ;
    if(validPPQ)lastPPQ=ppq;
    lastBlockSamples=n;
    lastHostBpm=bpm;
    wasTransportPlaying=transportPlaying;
    wasHostSynced=hostSync;
    lastSyncDivision=syncDiv;
    lastNoteType=noteType;
    lfo.setFrequency(hz,hostSync);
    // Keep all delay histories warm, but skip the expensive interpolation for
    // modes absent from the current/fading mix.
    const auto& weightsAtBlockStart=modeFader.current();
    const bool needsVibrato=wanted==0 || weightsAtBlockStart[0]>0.0f;
    const bool needsRotary=wanted==1 || weightsAtBlockStart[1]>0.0f;
    const bool needsTremolo=wanted==2 || weightsAtBlockStart[2]>0.0f;
    const bool needsChorus=wanted==3 || weightsAtBlockStart[3]>0.0f;

    // No allocations in the audio callback: fixed-size block slices capped by prepareToPlay size.
    for(int blockStart=0;blockStart<n;blockStart+=maxBlock)
    {
        const int count=juce::jmin(maxBlock,n-blockStart);
        for(int ch=0;ch<2;++ch)
        {
            const int sourceChannel=juce::jmin(ch,channels-1);
            juce::FloatVectorOperations::copy(original.getWritePointer(ch),
                                             buffer.getReadPointer(sourceChannel,blockStart),count);
            juce::FloatVectorOperations::copy(processed.getWritePointer(ch),
                                             buffer.getReadPointer(sourceChannel,blockStart),count);
            // Keep NaN/Inf from upstream effects out of persistent delay,
            // rotary and saturation state. Vector-copied finite audio is untouched.
            auto* const dry = original.getWritePointer(ch);
            auto* const wetInput = processed.getWritePointer(ch);
            for (int j=0; j<count; ++j)
                if (!std::isfinite(wetInput[j]))
                    dry[j] = wetInput[j] = breeze::dsp::sanitizeAudioSample(wetInput[j]);
        }
        const float satAmount=p("saturation")*.01f;
        // Saturation never mutates the host buffer before the final output stage.
        saturation.process(processed,0,count,satAmount);

        for(int i=0;i<count;++i)
        {
            const int sample=blockStart+i;
            const float phase=static_cast<float>(lfo.tick());
            const float d=depthSmooth.getNextValue();
            const float width=widthSmooth.getNextValue();
            const float wet=juce::jlimit(0.0f,1.0f,mixSmooth.getNextValue());
            const float dryGain=dryGainSmooth.getNextValue();
            const float wetGain=wetGainSmooth.getNextValue();
            const float power=powerSmooth.getNextValue();
            const float rateHz=static_cast<float>(lfo.getCurrentFrequency());
            float input[2]{};
            for(int ch=0;ch<2;++ch)
            {
                input[ch]=processed.getSample(ch,i);
                vibratoDelay.write(ch,input[ch]);
                chorusDelay.write(ch,input[ch]);
                dryDelay.write(ch,original.getSample(ch<channels?ch:0,i));
            }
            float rotaryOut[2];
            rotary.process(input,rotaryOut,rateHz,d,width,wave,needsRotary);
            float dryNormal[2],dryVibrato[2],modeOut[4][2]{};
            for(int ch=0;ch<2;++ch)
            {
                const double phaseOffset=ch==0?0.0:0.5*width;
                if(needsVibrato || needsTremolo)
                {
                    const float vp=breeze::dsp::Lfo::sample(phase+phaseOffset,wave,rateHz/sr);
                    if(needsVibrato)
                    {
                        const float delayMs = vibratoMotion[static_cast<size_t>(ch)]
                            .tick(4.0f + 3.0f*d*vp);
                        modeOut[0][ch] = vibratoDelay.read(ch, delayMs*sr*.001);
                    }
                    if(needsTremolo)
                    {
                        const float shape = tremoloMotion[static_cast<size_t>(ch)].tick(vp);
                        modeOut[2][ch] = input[ch]*(1.0f-d*(0.5f+0.5f*shape));
                    }
                }
                if(needsChorus)
                {
                    float sum=0.0f, weightSum=0.0f;
                    for(int k=0;k<3;++k)
                    {
                        const float vo=breeze::dsp::Lfo::sample(
                            phase+static_cast<float>(k)/3.0f+phaseOffset,wave,rateHz/sr);
                        const float ms=chorusMotion[static_cast<size_t>(ch)]
                            [static_cast<size_t>(k)].tick(12.0f+k*4.0f+4.0f*d*vo);
                        const float w=chorusGains[static_cast<size_t>(ch)]
                            [static_cast<size_t>(k)].getNextValue();
                        sum+=chorusDelay.read(ch,ms*sr*.001)*w;
                        weightSum+=w;
                    }
                    modeOut[3][ch]=sum/juce::jmax(weightSum,0.001f);
                }
                modeOut[1][ch]=rotaryOut[ch];
                dryNormal[ch]=dryDelay.read(ch,saturation.getLatency());
                dryVibrato[ch]=dryDelay.read(ch,saturation.getLatency()+dryCompensation);
            }
            const auto& modeWeights=modeFader.advance();
            for(int ch=0;ch<channels;++ch)
            {
                float wetValue=0.0f,dryValue=0.0f;
                for(int m=0;m<4;++m)
                {
                    const float weight=modeWeights[static_cast<size_t>(m)];
                    wetValue+=modeOut[m][ch]*weight;
                    // A fully dry signal must be independent of the selected
                    // effect. Introduce Vibrato's delay compensation only as
                    // its wet blend rises; otherwise switching into Vibrato
                    // would shift the dry audio by 4 ms even with MIX at 0%.
                    const float alignedDry = m==0
                        ? breeze::dsp::vibratoDryAtMix(dryNormal[ch],dryVibrato[ch],wet)
                        : dryNormal[ch];
                    dryValue+=alignedDry*weight;
                }
                const float mixed=dryValue*dryGain+wetValue*wetGain;
                const float output=dryNormal[ch]+power*(mixed-dryNormal[ch]);
                buffer.setSample(ch,sample,output);
                const float magnitude=std::abs(output);
                meterPeak[static_cast<size_t>(ch)]=juce::jmax(meterPeak[static_cast<size_t>(ch)],magnitude);
                meterEnergy[static_cast<size_t>(ch)]+=static_cast<double>(output)*output;
            }
            dryDelay.advance();vibratoDelay.advance();chorusDelay.advance();
        }
    }
    for(int ch=0; ch<2; ++ch)
    {
        const int source=juce::jmin(ch,channels-1);
        displayRms[static_cast<size_t>(ch)].store(
            static_cast<float>(std::sqrt(meterEnergy[static_cast<size_t>(source)] / n)),
            std::memory_order_relaxed);
        displayPeak[static_cast<size_t>(ch)].store(
            meterPeak[static_cast<size_t>(source)],std::memory_order_relaxed);
    }
    meterFrame.fetch_add(1, std::memory_order_release);
    lfoPhase.store(static_cast<float>(lfo.getPhase()),std::memory_order_relaxed);
    displayRate.store(static_cast<float>(lfo.getCurrentFrequency()),std::memory_order_relaxed);
}

void BreezeModAudioProcessor::loadFactoryPreset(int i)
{
    struct Preset{const char* name;float mode,depth,rate, sync,div,note,wave,sat,width,mix;};
    constexpr Preset presets[] {
        {"Chorus - Classic",3,50,3,0,3,0,0,20,70,50},
        {"Chorus - Wide",3,72,0.8f,0,3,0,1,13,100,60},
        {"Vibrato - Tape",0,53,4.1f,0,3,0,0,8,20,100},
        {"Rotary - Vintage",1,75,5,0,3,0,0,31,90,80},
        {"Tremolo - Pulse",2,85,0.7f,1,3,0,2,10,65,75},
        {"Rotary - Slow",1,54,0.6f,0,3,0,0,20,80,78}
    };
    i=juce::jlimit(0,5,i);
    const auto& z=presets[i];
    const float values[]={1,z.mode,z.depth,z.rate,z.sync,z.div,z.note,z.wave,z.sat,z.width,z.mix};
    for(size_t j=0;j<paramIds.size();++j)
    {
        if(auto* param=apvts.getParameter(paramIds[j]))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(values[j]));
            param->endChangeGesture();
        }
    }
    apvts.state.setProperty("uiPresetName",z.name,nullptr);
}
void BreezeModAudioProcessor::getStateInformation(juce::MemoryBlock& data)
{
    if(auto xml=apvts.copyState().createXml())copyXmlToBinary(*xml,data);
}
void BreezeModAudioProcessor::setStateInformation(const void* data,int size)
{
    if(auto xml=getXmlFromBinary(data,size))
        if(xml->hasTagName(apvts.state.getType().toString()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* BreezeModAudioProcessor::createEditor()
{return new BreezeModAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{return new BreezeModAudioProcessor();}
