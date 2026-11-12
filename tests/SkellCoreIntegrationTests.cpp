#include <SkellCore/Core.h>
#include <cmath>
#include <iostream>
#include <limits>

int main()
{
    int failures=0;
    const auto check=[&](bool valid,const char* label)
    {
        if (!valid) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
        else std::cout << "PASS: " << label << '\n';
    };
    skellcore::TempoClock clock;
    clock.prepare(48000.0);
    clock.update(120.0,8.0,true);
    check(std::abs(clock.samplesPerBeat()-24000.0)<1.0e-7,
          "SkellCore host clock: 120 BPM yields 24000 samples/beat");
    clock.advance(12000);
    check(std::abs(clock.beatPosition()-8.5)<1.0e-9,
          "SkellCore host clock advances half a beat");
    clock.update(90.0,15.0,false);
    clock.advance(16000);
    check(std::abs(clock.beatPosition()-15.0)<1.0e-9,
          "Stopped transport holds the beat position");
    clock.update(std::numeric_limits<double>::quiet_NaN(),15.0,true);
    check(std::abs(clock.bpm()-90.0)<1.0e-9,
          "Invalid host tempo leaves the last valid tempo intact");
    clock.advance(32000);
    check(std::abs(clock.beatPosition()-16.0)<1.0e-9,
          "SkellCore resumes at 90 BPM without a phase jump");
    check(clock.elapsedSamples()==60000,
          "SkellCore counts processed samples at sample-rate");
    return failures==0?0:1;
}
