#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include "dsp/Lfo.h"
#include "dsp/ModulationMath.h"
#include "dsp/ModDelay.h"
#include "dsp/RotarySim.h"
#include "dsp/Saturator.h"

class BreezeModAudioProcessor final : public juce::AudioProcessor
{
public:
    BreezeModAudioProcessor();
    ~BreezeModAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BreezeMod"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.15; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    const juce::String getProgramName(int) override { return {}; }
    void setCurrentProgram(int) override {}
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> lfoPhase {0.0f};
    std::atomic<float> displayRate {0.0f};
    std::atomic<float> displayBpm {120.0f};
    void loadFactoryPreset(int index);
private:
    breeze::dsp::Lfo lfo;
    breeze::dsp::ModDelay vibratoDelay, chorusDelay, dryDelay;
    breeze::dsp::RotarySim rotary;
    breeze::dsp::Saturator saturation;
    juce::AudioBuffer<float> original, processed;
    double sr = 44100.0;
    int maxBlock=512, dryCompensation=0;
    breeze::dsp::ModeFader modeFader;
    bool wasHostSynced = false, wasTransportPlaying = false, haveLastPPQ = false;
    double lastPPQ = 0.0, lastHostBpm = 120.0;
    int lastBlockSamples = 0, lastSyncDivision = -1, lastNoteType = -1;
    juce::SmoothedValue<float,juce::ValueSmoothingTypes::Linear>
        depthSmooth, widthSmooth, mixSmooth, powerSmooth,
        dryGainSmooth, wetGainSmooth;
    std::array<std::array<juce::SmoothedValue<float,
        juce::ValueSmoothingTypes::Linear>,3>,2> chorusGains;
    float p(const char* id) const noexcept
    { return apvts.getRawParameterValue(id)->load(std::memory_order_relaxed); }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BreezeModAudioProcessor)
};
