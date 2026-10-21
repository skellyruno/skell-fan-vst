#pragma once
#include <JuceHeader.h>
#include "ModDelay.h"
#include "Lfo.h"

namespace breeze::dsp
{
class RotarySim
{
public:
    void prepare(double sampleRate)
    {
        sr = juce::jmax(1.0, sampleRate);
        crossover = 1.0f - std::exp(-juce::MathConstants<float>::twoPi*800.0f/static_cast<float>(sr));
        // These time constants depend only on sample rate. They must not be
        // recomputed with exp() for each sample on the audio thread.
        hornA = 1.0f - std::exp(-1.0f/static_cast<float>(sr*0.40));
        drumA = 1.0f - std::exp(-1.0f/static_cast<float>(sr*0.65));
        hornDelay.prepare(sr, 16.0); drumDelay.prepare(sr, 16.0);
        low[0] = low[1] = 0.0f;
        hornPhase = drumPhase = 0.0;
        hornSpeed = drumSpeed = 0.0f;
    }
    void process(const float in[2], float out[2], float rate, float depth,
                 float width, int wave, bool render = true) noexcept
    {
        // The delays and inertia stay warm when Rotary isn't audible. Skipping
        // fractional interpolation and wave shaping saves work in other modes.
        hornSpeed += hornA * (rate - hornSpeed);
        drumSpeed += drumA * (rate*0.7f - drumSpeed);
        hornPhase = std::fmod(hornPhase + hornSpeed/sr, 1.0);
        drumPhase = std::fmod(drumPhase + drumSpeed/sr, 1.0);
        float hi[2],lo[2];
        for (int ch=0;ch<2;++ch)
        {
            low[ch] += crossover*(in[ch]-low[ch]);
            lo[ch]=low[ch]; hi[ch]=in[ch]-lo[ch];
            hornDelay.write(ch,hi[ch]);drumDelay.write(ch,lo[ch]);
        }
        if (!render)
        {
            out[0] = out[1] = 0.0f;
            hornDelay.advance(); drumDelay.advance();
            return;
        }
        for (int ch=0;ch<2;++ch)
        {
            const double offset = ch == 0 ? 0.0 : static_cast<double>(width)*0.5;
            const float hornLfo = Lfo::sample(hornPhase+offset, wave);
            const float drumLfo = Lfo::sample(drumPhase+offset, wave);
            const float hornMs = 2.5f + 1.0f*depth*hornLfo;
            const float drumMs = 3.5f + 0.6f*depth*drumLfo;
            const float horn = hornDelay.read(ch,sr*0.001*hornMs);
            const float drum = drumDelay.read(ch,sr*0.001*drumMs);
            const float hornAmp = 1.0f-depth*0.36f*(0.5f+0.5f*hornLfo);
            const float drumAmp = 1.0f-depth*0.20f*(0.5f+0.5f*drumLfo);
            out[ch] = horn*hornAmp + drum*drumAmp;
        }
        hornDelay.advance();drumDelay.advance();
    }
private:
    ModDelay hornDelay,drumDelay;
    double sr=44100.0, hornPhase=0, drumPhase=0;
    float hornSpeed=0,drumSpeed=0,crossover=0.1f, hornA=0.0f, drumA=0.0f,low[2]{};
};
}
