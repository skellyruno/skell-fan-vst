#include "ChromeKnob.h"
#include "Theme.h"
#include <BinaryData.h>
#include <cmath>
#include <cstdlib>
ChromeKnob::ChromeKnob(BreezeModAudioProcessor& p,const juce::String& id,
                      const juce::String& caption,bool miniature,bool blue)
    : name(caption),parameterId(id),small(miniature),isBlue(blue)
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setVelocityBasedMode(false);
    setTextBoxStyle(juce::Slider::NoTextBox,true,0,0);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);
    setRotaryParameters(juce::MathConstants<float>::pi*1.25f,
                        juce::MathConstants<float>::pi*2.75f,true);
    attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,id,*this);
    setTooltip(id=="syncDiv" ? "HOST SYNC: cycles per note" : caption.isNotEmpty()?caption:id.toUpperCase());
    setDoubleClickReturnValue(true,p.apvts.getParameter(id)->convertFrom0to1(
                                     p.apvts.getParameter(id)->getDefaultValue()));
    chromeImg=blue?Theme::image(BinaryData::knob_blue_png,BinaryData::knob_blue_pngSize)
                  :Theme::image(BinaryData::knob_chrome_png,BinaryData::knob_chrome_pngSize);
}
juce::String ChromeKnob::displayValue() const
{
    if(parameterId=="syncDiv")
    {
        constexpr const char* divisions[] = {"1/1","1/2","1/4","1/8","1/16","1/32","1/64"};
        return divisions[juce::jlimit(0,6,juce::roundToInt(getValue()))];
    }
    if(parameterId=="wave")
    {
        constexpr const char* waves[] = {"SINE","TRI","SQR","SAW"};
        return waves[juce::jlimit(0,3,juce::roundToInt(getValue()))];
    }
    if(parameterId=="rate") return juce::String(getValue(),2)+" Hz";
    return juce::String(getValue(),1)+"%";
}
void ChromeKnob::mouseDown(const juce::MouseEvent& e)
{
    if(e.mods.isPopupMenu())
    {
        juce::PopupMenu m;m.addItem(1,"Enter numeric value...");
        m.addSeparator();m.addItem(2,"Vertical drag");m.addItem(3,"Circular drag");
        juce::Component::SafePointer<ChromeKnob> self(this);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),[self](int i)
        {
            if(self==nullptr)return;
            if(i==2)self->setSliderStyle(juce::Slider::RotaryVerticalDrag);
            if(i==3)self->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            if(i==1)
            {
                auto* win=new juce::AlertWindow("Set value",self->name,juce::MessageBoxIconType::NoIcon);
                win->addTextEditor("value",juce::String(self->getValue(),2),"Value");
                win->addButton("Apply",1);win->addButton("Cancel",0);
                win->enterModalState(true,juce::ModalCallbackFunction::create([self,win](int r)
                {
                    if(r==1&&self!=nullptr)
                    {
                        const auto valueText=win->getTextEditorContents("value").trim();
                        const char* const begin=valueText.toRawUTF8();
                        char* end=nullptr;
                        const double v=std::strtod(begin,&end);
                        if (end!=begin && *end=='\0' && std::isfinite(v))
                            self->setValue(juce::jlimit(self->getMinimum(),self->getMaximum(),v),
                                           juce::sendNotificationSync);
                        else
                            juce::AlertWindow::showMessageBoxAsync(
                                juce::MessageBoxIconType::WarningIcon,"Invalid value",
                                "Enter a valid finite number, for example 3.5.");
                    }
                    delete win;
                }),false);
            }
        });return;
    }
    juce::Slider::mouseDown(e);
}
void ChromeKnob::paint(juce::Graphics& g)
{
    const auto area=getLocalBounds().toFloat();
    const float scale=juce::jmin(area.getWidth()/(small?52.0f:94.0f),
                                area.getHeight()/(small?60.0f:111.0f));
    const float cx=area.getCentreX(),cy=area.getY()+ (small?24.0f:47.0f)*scale;
    const float radius=(small?21.0f:38.0f)*scale;
    const float f=static_cast<float>(valueToProportionOfLength(getValue()));
    const float angle=juce::MathConstants<float>::pi*(1.25f+1.5f*f);
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillEllipse(cx-radius-3*scale,cy-radius+5*scale,radius*2+6*scale,radius*2+4*scale);
    if(chromeImg.isValid())
    {
        auto tr=juce::AffineTransform::scale(2*radius/chromeImg.getWidth(),2*radius/chromeImg.getHeight())
            .rotated(angle-2.3f,radius,radius).translated(cx-radius,cy-radius);
        g.drawImageTransformed(chromeImg,tr);
    }
    g.setColour(juce::Colour(0xfff44939));
    juce::Path pointer;
    pointer.startNewSubPath(cx+std::sin(angle)*radius*.54f,cy-std::cos(angle)*radius*.54f);
    pointer.lineTo(cx+std::sin(angle)*radius*.84f,cy-std::cos(angle)*radius*.84f);
    g.strokePath(pointer,juce::PathStrokeType(2.5f*scale));
    g.setFont(Theme::font(11.0f*scale,true));
    g.setColour(isBlue?Theme::label:Theme::label);
    g.drawText(name,area.withTop(cy+radius+1.0f*scale),juce::Justification::centredTop);
    if (small && name.isEmpty() && !isMouseOverOrDragging())
    {
        // Permanent top-bar Width/Mix percentage rather than hover-only feedback.
        g.setColour(juce::Colour(0xffcde2ed));
        g.setFont(Theme::font(10.0f*scale,true));
        g.drawText(juce::String(juce::roundToInt(getValue())) + "%",
                   juce::Rectangle<float>{0.0f,cy+radius+scale,
                                          area.getWidth(),14.0f*scale},
                   juce::Justification::centred);
    }
    if(isMouseOverOrDragging())
    {
        g.setColour(juce::Colours::black.withAlpha(0.72f));
        g.fillRoundedRectangle(cx-32*scale,cy-radius-27*scale,64*scale,20*scale,3*scale);
        g.setColour(juce::Colours::white);
        g.setFont(Theme::font(12*scale,true));
        g.drawText(displayValue(),
                   juce::Rectangle<float>{cx-32.0f*scale,cy-radius-27.0f*scale,
                                          64.0f*scale,20.0f*scale},
                   juce::Justification::centred);
    }
}
