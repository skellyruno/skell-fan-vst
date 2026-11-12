#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "ChromeKnob.h"
#include <SkellCore/SkullTheme.h>
class PowerToggle final : public juce::ToggleButton
{
public:
    void paintButton(juce::Graphics&,bool,bool) override;
};
class ScopeToggle final : public juce::TextButton
{
public:
    void paintButton(juce::Graphics&,bool,bool) override;
};
// SkellCore's shared skull geometry; fan artwork and audio controls remain independent.
class FanSkullToggle final : public juce::Button
{
public:
    FanSkullToggle() : juce::Button("SkellFan fan-skull")
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        setTooltip("Three-blade SkellFan logo: click to cycle accent themes");
        setWantsKeyboardFocus(true);
    }
    void setSkin(skellcore::ui::SkullSkin next) { selected=next; repaint(); }
    void paintButton(juce::Graphics&,bool,bool) override;
private:
    skellcore::ui::SkullSkin selected=skellcore::ui::SkullSkin::Neon;
};
class TopBar final : public juce::Component
{
public:
    explicit TopBar(SkellFanAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
    void refreshSkin();
    skellcore::ui::SkullSkin currentSkin() const noexcept { return skin; }
    std::function<void()> onScope;
private:
    SkellFanAudioProcessor& processor;
    skellcore::ui::SkullSkin skin=skellcore::ui::SkullSkin::Neon;
    FanSkullToggle skull;
    PowerToggle power;
    ScopeToggle scope;
    ChromeKnob width,mix;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttach;
};
