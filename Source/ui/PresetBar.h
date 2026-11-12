#pragma once
#include <JuceHeader.h>
#include <array>
#include "../PluginProcessor.h"
// Engraved hardware glyphs, not platform-dependent Unicode or S/X/R letters.
class PresetIconButton final : public juce::TextButton
{
public:
    enum class Icon { Previous, Next, Save, Remove, Shuffle };
    explicit PresetIconButton(Icon artwork) : icon(artwork)
    {
        setWantsKeyboardFocus(true);
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    void paintButton(juce::Graphics&,bool hover,bool pressed) override;
private:
    Icon icon;
};

class PresetBar final : public juce::Component,private juce::Timer
{
public:
    explicit PresetBar(SkellFanAudioProcessor&);
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override;
    void popupMenu();
    void storePreset();
    void erasePreset();
    void randomize();
    void chooseFactory(int i);
    void capturePresetSnapshot();
    bool isModified() const;
    juce::File folder() const;
    juce::File legacyFolder() const;
    juce::String name() const;
    SkellFanAudioProcessor& p;
    int factoryIndex=0;
    juce::String snapshotName;
    std::array<float, 10> snapshotValues {};
    PresetIconButton prev{PresetIconButton::Icon::Previous};
    PresetIconButton next{PresetIconButton::Icon::Next};
    PresetIconButton save{PresetIconButton::Icon::Save};
    PresetIconButton remove{PresetIconButton::Icon::Remove};
    PresetIconButton shuffle{PresetIconButton::Icon::Shuffle};
    juce::TextButton current;
};
