#pragma once
#include <JuceHeader.h>
namespace Theme
{
constexpr float W=956.0f,H=984.0f;
constexpr int topHeight=60;
// JUCE 8 Rectangle<float> construction is not constexpr under MSVC.
// C++17 inline const defines a single immutable instance across translation units.
// Numeric base dimensions above remain valid compile-time constants.
inline const juce::Colour black{0xff040405},bar{0xff0b0b0c},chrome{0xffacb4bc},
    label{0xff282e32},plate{0xffc2c5c8},deepBlue{0xff1f4fd8},
    cyan{0xff2cc4f2},lightBlue{0xff3aa6ee},ivory{0xffe9e9e6},
    muted{0xffabb0b3};
inline const juce::Rectangle<float> rotorRect{166.0f,85.0f,600.0f,600.0f};
inline const juce::Rectangle<float> grilleRect{146.0f,65.0f,640.0f,640.0f};
inline const juce::Rectangle<float> hubRect{371.0f,290.0f,190.0f,190.0f};
inline const juce::Rectangle<float> plateRect{231.0f,738.0f,490.0f,169.0f};
inline juce::Font font(float pt,bool bold=false)
{
    return juce::Font(juce::FontOptions(pt).withStyle(bold?"Bold":"Regular"));
}
inline juce::Image image(const void* data,int size)
{
    return juce::ImageFileFormat::loadFrom(data,static_cast<size_t>(size));
}
inline void metal(juce::Graphics& g,juce::Rectangle<float> r)
{
    juce::ColourGradient gradient(juce::Colour(0xffe0e1e3),r.getTopLeft(),
                                  juce::Colour(0xffa4a9b0),r.getBottomRight(),false);
    gradient.addColour(0.24,juce::Colour(0xffb9bec2));
    gradient.addColour(0.6,juce::Colour(0xffd2d6d8));
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(r,13.0f);
    g.setColour(juce::Colour(0xff76848a));
    g.drawRoundedRectangle(r.reduced(1.0f),12.0f,2.0f);
}
}
