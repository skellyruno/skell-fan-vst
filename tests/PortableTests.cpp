#include "../Source/dsp/ModulationMath.h"
#include <array>
#include <cmath>
#include <iostream>
#include <random>

namespace
{
int errors = 0;
void check(bool ok, const char* msg)
{
    if (!ok) { ++errors; std::cerr << "FAIL: " << msg << '\n'; }
    else std::cout << "PASS: " << msg << '\n';
}
float weightSum(const std::array<float,4>& weights)
{
    return weights[0]+weights[1]+weights[2]+weights[3];
}
void tempoTests()
{
    using breeze::dsp::hostCycleBeats;
    check(std::abs(hostCycleBeats(0,0)-4.0)<1e-12, "whole note = four quarter-note beats");
    check(std::abs(hostCycleBeats(2,0)-1.0)<1e-12, "quarter-note sync = one beat");
    check(std::abs(hostCycleBeats(3,0)-0.5)<1e-12, "eighth-note sync = half beat");
    check(std::abs(hostCycleBeats(3,1)-0.75)<1e-12, "dotted eighth = three quarters of a beat");
    check(std::abs(hostCycleBeats(3,2)-1.0/3.0)<1e-12, "eighth triplet = one third beat");
    check(std::abs(hostCycleBeats(6,2)-1.0/24.0)<1e-12, "sixty-fourth triplet");
    check(std::abs(hostCycleBeats(99,0)-hostCycleBeats(6,0))<1e-12,
          "out-of-range host divisions clamp safely");
    const double bpm=120.0;
    check(std::abs(bpm/(60.0*hostCycleBeats(2,0))-2.0)<1e-12,
          "120 BPM quarter note gives two LFO cycles per second");
}
void dryTests()
{
    using breeze::dsp::vibratoDryAtMix;
    check(vibratoDryAtMix(-0.42f,0.83f,0.0f)==-0.42f,
          "zero MIX is independent of Vibrato dry time alignment");
    check(std::abs(vibratoDryAtMix(-0.42f,0.83f,0.5f)-0.205f)<1e-6f,
          "half mix gradually introduces Vibrato compensation");
    check(std::abs(vibratoDryAtMix(-0.42f,0.83f,1.0f)-0.83f)<1e-6f,
          "full mix uses compensated Vibrato dry path");
    check(vibratoDryAtMix(0.5f,1.0f,-5.0f)==0.5f,
          "Vibrato dry law clamps invalid negative mix");
}
void modeTests()
{
    breeze::dsp::ModeFader fade;
    fade.prepare(3,960);
    check(fade.current()[3]==1.0f,"startup chorus is exactly selected");
    fade.setTarget(0);
    auto last=fade.current();
    bool continuous=true,normalized=true;
    for(int n=0;n<310;++n)
    {
        auto w=fade.advance();
        if(std::abs(weightSum(w)-1.0f)>2e-5f)normalized=false;
        for(int m=0;m<4;++m)
            if(std::abs(w[m]-last[m])>0.002f)continuous=false;
        last=w;
    }
    fade.setTarget(1); // rapid re-target while 3 -> 0 has not finished
    for(int n=0;n<300;++n)
    {
        auto w=fade.advance();
        if(std::abs(weightSum(w)-1.0f)>2e-5f)normalized=false;
        for(int m=0;m<4;++m)
            if(std::abs(w[m]-last[m])>0.002f)continuous=false;
        last=w;
    }
    fade.setTarget(2);
    for(int n=0;n<960;++n)
    {
        auto w=fade.advance();
        if(std::abs(weightSum(w)-1.0f)>2e-5f)normalized=false;
        for(int m=0;m<4;++m)
            if(std::abs(w[m]-last[m])>0.002f)continuous=false;
        last=w;
    }
    check(continuous,"rapid mode changes stay sample-continuous");
    check(normalized,"mode weights remain unity-normalized through retargets");
    check(!fade.transitioning() && fade.current()[2]==1.0f,
          "finished mode fade lands exactly on its destination");

    std::minstd_rand generator(49);
    fade.prepare(0,960);
    bool finite=true;
    for (int i=0;i<15000;++i)
    {
        if(i%29==0)fade.setTarget(static_cast<int>(generator()%4));
        const auto& w=fade.advance();
        float sum=0.0f;
        for(float x:w) { finite &= std::isfinite(x)&&x>=0.0f&&x<=1.0f;sum+=x; }
        normalized &= std::abs(sum-1.0f)<3e-5f;
    }
    check(finite&&normalized,"15k samples of rapid random mode automation stay bounded");
    fade.prepare(0,0);
    fade.setTarget(3);
    fade.advance();
    check(fade.current()[3]==1.0f,"zero-duration fade gracefully clamps to one sample");
}
}
int main()
{
    tempoTests();dryTests();modeTests();
    std::cout << (errors==0?"All portable DSP tests passed\n":"Portable tests failed\n");
    return errors==0?0:1;
}
