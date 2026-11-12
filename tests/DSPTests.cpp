#include <JuceHeader.h>
#include "../Source/dsp/Lfo.h"
#include "../Source/dsp/ModDelay.h"
#include "../Source/dsp/RotarySim.h"
#include "../Source/dsp/Saturator.h"
#include <cmath>
#include <array>
#include <iostream>
#include <chrono>

namespace
{
constexpr int kAutomationSamples = 4096;
int errors = 0;
void check(bool ok, const char* what)
{
    if (!ok) { std::cerr << "FAIL: " << what << '\n'; ++errors; }
    else std::cout << "PASS: " << what << '\n';
}
void testLfo()
{
    breeze::dsp::Lfo lfo;
    lfo.prepare(48000.0);
    lfo.setFrequency(2.0, true); // 120 BPM, quarter note
    lfo.lockToPPQ(32.5, 1.0);
    check(std::abs(lfo.getPhase() - 0.5) < 1.0e-9, "Host PPQ phase lock");
    for (int n=0;n<24000;++n) lfo.tick();
    check(std::abs(lfo.getPhase() - 0.5) < 1.0e-6,
          "Host tempo is exact over a complete LFO cycle");
    lfo.setFrequency(0.0,true);
    const double frozen=lfo.getPhase();
    for (int n=0;n<1000;++n) lfo.tick();
    check(std::abs(lfo.getPhase()-frozen)<1.0e-9,"0 Hz really stops modulation");
    for(int type=0;type<4;++type)
        for(int n=0;n<=200;++n)
        {
            const float v=breeze::dsp::Lfo::sample(n/200.0,type,0.004);
            if(!std::isfinite(v) || std::abs(v)>1.1f)
            { check(false,"LFO finite and bounded");return; }
        }
    check(true,"LFO finite and bounded");
}
void testHostDivisionChangeGlide()
{
    breeze::dsp::Lfo lfo;
    lfo.prepare(48000.0);
    lfo.setFrequency(0.0,true);
    lfo.lockToPPQ(0.0,1.0);
    lfo.lockToPPQ(0.37,1.0,true);
    double prev=lfo.getPhase();
    double biggestStep=0.0;
    for(int i=0;i<480;++i)
    {
        const double next=lfo.tick();
        const double delta=next-prev;
        const double wrapped=delta-std::floor(delta+0.5);
        biggestStep=std::max(biggestStep,std::abs(wrapped));
        prev=next;
    }
    check(biggestStep<0.003,"HOST division automation glides without abrupt phase jumps");
    check(std::abs(lfo.getPhase()-0.37)<1.e-6,
          "HOST division glide converges to beat-exact target after 10 ms");
    lfo.lockToPPQ(0.81,1.0,false);
    check(std::abs(lfo.getPhase()-0.81)<1.e-9,
          "Transport seek still immediately locks to host PPQ");
}

void testDelay()
{
    breeze::dsp::ModDelay delay;
    delay.prepare(48000.0,30.0);
    float peak=0.0f;
    for(int n=0;n<40;++n)
    {
        delay.write(0,n==0?1.0f:0.0f);
        const float v=delay.read(0,4.0);
        if(n==4)peak=v;
        if(!std::isfinite(v)){check(false,"Fractional delay finite");return;}
        delay.advance();
    }
    check(std::abs(peak-1.0f)<1.0e-5f,"Hermite delay impulse at 4 samples");
    delay.clear();
    delay.write(0,0.75f);
    check(delay.read(0,0.0)==0.75f,"Zero delay returns current sample exactly");
    delay.clear();
    check(std::abs(delay.read(0,7.25))<1.0e-8f,"Reset clears delay feedback/history");
}
void testDelayPeakBound()
{
    breeze::dsp::ModDelay delay;
    delay.prepare(48000.0, 30.0);
    const float pattern[] = {-1.0f, 1.0f, 1.0f, -1.0f};
    bool finiteAndBounded=true;
    for (int n=0;n<20000;++n)
    {
        delay.write(0,pattern[n % 4]);
        const float out=delay.read(0,6.5);
        finiteAndBounded &= std::isfinite(out) && std::abs(out)<=1.000001f;
        delay.advance();
    }
    check(finiteAndBounded,"Hermite interpolator avoids transient peak overshoot");
}

void testSaturation()
{
    breeze::dsp::Saturator sat;
    sat.prepare(48000.0,256,0.0f);
    check(sat.getLatency()>=0 && sat.getLatency()<256,
          "Oversampling latency is finite and reportable");
    juce::AudioBuffer<float> input(2,256);
    double neutralEnergy=0.0;
    for(int block=0;block<60;++block)
    {
        for(int i=0;i<256;++i)
        {
            const float v=0.4f*std::sin(juce::MathConstants<float>::twoPi*1000.0f
                                        *static_cast<float>(block*256+i)/48000.0f);
            input.setSample(0,i,v);
            input.setSample(1,i,v);
        }
        sat.process(input,0,256,0.0f);
        if(block>=10)
            for(int i=0;i<256;++i)
            {
                const float x=input.getSample(0,i);
                neutralEnergy+=x*x;
                if(!std::isfinite(x)){check(false,"Saturation finite");return;}
            }
    }
    const double rms=std::sqrt(neutralEnergy/(50.0*256.0));
    check(std::abs(rms-0.4/std::sqrt(2.0))<0.03,
          "0% saturation preserves sine RMS (apart from filter latency)");
    for(int block=0;block<40;++block)
    {
        for(int i=0;i<256;++i)
        {
            const float v=0.95f*std::sin(juce::MathConstants<float>::twoPi*1000.0f
                                         *static_cast<float>(block*256+i)/48000.0f);
            input.setSample(0,i,v);
            input.setSample(1,i,v);
        }
        sat.process(input,0,256,1.0f);
        for(int i=0;i<256;++i)
            for(int ch=0;ch<2;++ch)
                if(!std::isfinite(input.getSample(ch,i)))
                {check(false,"Saturation remains finite at full drive");return;}
    }
    check(true,"Saturation remains finite at full drive");
}
void testVariableBlocksAndSaturationAutomation()
{
    // The 30 ms saturation ramp must be independent of callback block size.
    // All switch points are multiples of 256 and 64 samples.
    auto render=[](int blockSize)
    {
        breeze::dsp::Saturator sat;
        sat.prepare(48000.0,256,0.0f);
        juce::AudioBuffer<float> audio(2,256);
        std::array<float,kAutomationSamples> output {};
        for (int offset=0;offset<kAutomationSamples;offset+=blockSize)
        {
            for(int i=0;i<blockSize;++i)
            {
                const float t=static_cast<float>(offset+i)/48000.0f;
                const float sample=0.65f*std::sin(t*juce::MathConstants<float>::twoPi*733.0f);
                audio.setSample(0,i,sample);
                audio.setSample(1,i,sample);
            }
            const float amount=offset<1024?0.0f:offset<2048?0.95f:offset<3072?0.35f:0.0f;
            sat.process(audio,0,blockSize,amount);
            for(int i=0;i<blockSize;++i)output[static_cast<size_t>(offset+i)]=audio.getSample(0,i);
        }
        return output;
    };
    const auto small=render(64),large=render(256);
    float maxError=0.0f;
    for(int i=0;i<kAutomationSamples;++i)
    {
        if(!std::isfinite(small[static_cast<size_t>(i)])
            ||!std::isfinite(large[static_cast<size_t>(i)]))
        { check(false,"Saturation automation is finite"); return; }
        maxError=juce::jmax(maxError,std::abs(small[static_cast<size_t>(i)]
                                         -large[static_cast<size_t>(i)]));
    }
    check(maxError<0.0005f,"Saturation automation is block-size invariant (64 vs 256)");
}

void testSaturationStressAndCpu()
{
    juce::ScopedNoDenormals guard;
    for (const double sampleRate : {44100.0, 48000.0, 96000.0})
    {
        breeze::dsp::Saturator sat;
        constexpr int blockSize=256, blocks=96;
        sat.prepare(sampleRate,blockSize,0.0f);
        juce::AudioBuffer<float> input(2,blockSize);
        const int reportedLatency=sat.getLatency();
        check(reportedLatency>=0 && reportedLatency<blockSize,
              "Saturation reported latency is plausible");

        input.clear();
        input.setSample(0,0,0.8f);
        input.setSample(1,0,0.8f);
        sat.process(input,0,blockSize,0.0f);
        int impulsePeak=0;
        float highest=0.0f;
        for (int n=0;n<blockSize;++n)
        {
            const float x=std::abs(input.getSample(0,n));
            if (x>highest) {highest=x;impulsePeak=n;}
        }
        std::cout << "Impulse latency: reported=" << reportedLatency
                  << " samples, peak=" << impulsePeak
                  << " at " << sampleRate << " Hz\n";
        check(highest>0.1f && std::abs(impulsePeak-reportedLatency)<=12,
              "Oversampling impulse agrees with reported latency");

        bool finite=true, withinUnity=true, noSubnormals=true;
        double processingMs=0.0;
        for (int block=0;block<blocks;++block)
        {
            for (int n=0;n<blockSize;++n)
            {
                const float t=static_cast<float>(block*blockSize+n)/static_cast<float>(sampleRate);
                const float x=0.80f*std::sin(juce::MathConstants<float>::twoPi*997.0f*t);
                input.setSample(0,n,x);
                input.setSample(1,n,-x);
            }
            const float amount=block%4==0?0.0f:block%4==1?0.25f:block%4==2?0.7f:1.0f;
            const auto begin=std::chrono::steady_clock::now();
            sat.process(input,0,blockSize,amount);
            processingMs += std::chrono::duration<double,std::milli>(
                std::chrono::steady_clock::now()-begin).count();
            for (int ch=0;ch<2;++ch)
                for(int n=0;n<blockSize;++n)
                {
                    const float x=input.getSample(ch,n);
                    finite &= std::isfinite(x);
                    withinUnity &= std::abs(x)<1.0f;
                    noSubnormals &= std::fpclassify(x)!=FP_SUBNORMAL;
                }
        }
        const double realtimeMs=1000.0*blockSize*blocks/sampleRate;
        const double cpuPercent=100.0*processingMs/realtimeMs;
        std::cout << "Saturator 2x " << sampleRate << " Hz: " << processingMs
                  << " ms processing / " << realtimeMs << " ms audio ("
                  << cpuPercent << "% of one realtime core)\n";
        check(finite && noSubnormals,
              "Automated drive ramps produce finite, non-denormal output");
        check(withinUnity,
              "0.8-peak sine stays below 0 dBFS under drive automation");
        check(std::isfinite(cpuPercent) && cpuPercent<90.0,
              "Oversampled processing completes comfortably before deadline");
    }
}

void testRotary()
{
    breeze::dsp::RotarySim r;
    r.prepare(48000.0);
    float in[2]{},out[2]{};
    for(int i=0;i<5000;++i)
    {
        in[0]=in[1]=std::sin(i*0.031f)*0.8f;
        r.process(in,out,(i/2000.0f)*7.0f,0.75f,0.85f,0);
        if(!std::isfinite(out[0])||!std::isfinite(out[1]))
        {check(false,"Rotary finite on speed ramp");return;}
    }
    check(true,"Rotary finite on speed ramp");
}
}
int main()
{
    testLfo();testHostDivisionChangeGlide();testDelay();testDelayPeakBound();testSaturation();
    testVariableBlocksAndSaturationAutomation();testSaturationStressAndCpu();testRotary();
    std::cout << (errors==0?"All DSP smoke tests passed\n":"DSP smoke tests failed\n");
    return errors==0?0:1;
}
