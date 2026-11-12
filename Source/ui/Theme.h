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
    label{0xff282e32},plate{0xffc2c5c8},deepBlue{0xff249844},
    cyan{0xff67f783},lightBlue{0xff46c96d},ivory{0xffe9e9e6},
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
// Images are loaded from the existing compiled fan assets. Convert only
// saturated cyan/blue paint into Skell neon green, preserving alpha, chrome,
// and metal luminance. Done once on the UI thread, never in paint or DSP.
inline juce::Image greenFanArtwork(const juce::Image& source)
{
    if (!source.isValid()) return {};
    auto result=source.createCopy();
    juce::Image::BitmapData pixels(result,juce::Image::BitmapData::readWrite);
    for (int y=0;y<result.getHeight();++y)
        for (int x=0;x<result.getWidth();++x)
        {
            const auto colour=pixels.getPixelColour(x,y);
            const float hue=colour.getHue(), sat=colour.getSaturation();
            if (colour.getFloatAlpha()>0.04f && sat>0.24f
                && hue>=0.48f && hue<=0.73f)
            {
                // Retain source luminance/texture and avoid flat, clipped
                // RGB green: match the mint-neon UI with controlled saturation.
                const float newSat=juce::jlimit(0.0f,0.82f,sat*0.88f);
                const float newValue=colour.getBrightness();
                pixels.setPixelColour(x,y,juce::Colour::fromHSV(
                    0.36f,newSat,newValue,colour.getFloatAlpha()));
            }
        }
    return result;
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
