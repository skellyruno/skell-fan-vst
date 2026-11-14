#include "ChromeKnob.h"
#include "Theme.h"
#include <cmath>
#include <cstdlib>
ChromeKnob::ChromeKnob(SkellFanAudioProcessor& p,const juce::String& id,
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
    beginValueFeedback();
}
void ChromeKnob::mouseDrag(const juce::MouseEvent& e)
{
    juce::Slider::mouseDrag(e);
    beginValueFeedback();
}
void ChromeKnob::mouseUp(const juce::MouseEvent& e)
{
    juce::Slider::mouseUp(e);
    // The value is intentionally visible only during and briefly after editing.
    if (showValue) startTimer(650);
}
void ChromeKnob::mouseWheelMove(const juce::MouseEvent& e,
                                const juce::MouseWheelDetails& wheel)
{
    juce::Slider::mouseWheelMove(e,wheel);
    if (wheel.deltaX != 0.0f || wheel.deltaY != 0.0f) beginValueFeedback();
}
bool ChromeKnob::keyPressed(const juce::KeyPress& key)
{
    const bool handled=juce::Slider::keyPressed(key);
    if (handled) beginValueFeedback();
    return handled;
}
void ChromeKnob::beginValueFeedback()
{
    showValue=true;
    startTimer(650);
    repaint();
}
void ChromeKnob::timerCallback()
{
    if (isMouseButtonDown())
    {
        startTimer(650);
        return;
    }
    showValue=false;
    stopTimer();
    repaint();
}
void ChromeKnob::paint(juce::Graphics& g)
{
    const auto area=getLocalBounds().toFloat();
    const float scale=juce::jmin(area.getWidth()/(small?52.0f:94.0f),
                                area.getHeight()/(small?60.0f:111.0f));
    const float cx=area.getCentreX();
    const float cy=area.getY()+(small?24.0f:47.0f)*scale;
    const float radius=(small?21.0f:38.0f)*scale;
    if (radius <= 0.0f) return;
    const float f=static_cast<float>(valueToProportionOfLength(getValue()));
    const float angle=juce::MathConstants<float>::pi*(1.25f+1.5f*f);

    // Multi-layer machined hardware. The chrome rim, bevel, recessed face and
    // reflected light use the same materials for Width, Dry/Wet and every dial.
    g.setColour(juce::Colours::black.withAlpha(0.58f));
    g.fillEllipse(cx-radius-2.8f*scale,cy-radius+4.2f*scale,
                  2.0f*radius+5.6f*scale,2.0f*radius+3.5f*scale);
    juce::ColourGradient rim(juce::Colour(0xffe7f0ea),cx-radius,cy-radius,
                             juce::Colour(0xff34423a),cx+radius,cy+radius,false);
    rim.addColour(0.27,juce::Colour(0xff8eaca0));
    rim.addColour(0.64,juce::Colour(0xff38463e));
    g.setGradientFill(rim);
    g.fillEllipse(cx-radius,cy-radius,2.0f*radius,2.0f*radius);
    g.setColour(juce::Colour(0xff08100c));
    g.drawEllipse(cx-radius+1.8f*scale,cy-radius+1.8f*scale,
                  2.0f*(radius-1.8f*scale),2.0f*(radius-1.8f*scale),1.5f*scale);

    // Deep annular groove and a heavier dark-metal knob cap.
    const float faceR=radius*0.83f;
    g.setColour(juce::Colour(0xff050b07));
    g.fillEllipse(cx-faceR-1.3f*scale,cy-faceR-1.3f*scale,
                  2.0f*(faceR+1.3f*scale),2.0f*(faceR+1.3f*scale));
    juce::ColourGradient cap(juce::Colour(0xff71877b),cx-faceR,cy-faceR,
                             juce::Colour(0xff15211a),cx+faceR,cy+faceR,false);
    cap.addColour(0.33,juce::Colour(0xff374a3e));
    cap.addColour(0.69,juce::Colour(0xff111b15));
    g.setGradientFill(cap);
    g.fillEllipse(cx-faceR,cy-faceR,2.0f*faceR,2.0f*faceR);
    g.setColour(juce::Colour(0xffb5d1bb).withAlpha(0.48f));
    g.drawEllipse(cx-faceR+1.0f*scale,cy-faceR+1.0f*scale,
                  2.0f*(faceR-1.0f*scale),2.0f*(faceR-1.0f*scale),1.0f*scale);

    // A soft radial reflection adds face curvature instead of a flat texture.
    const float inset=radius*0.61f;
    juce::ColourGradient dome(juce::Colour(0xff516b59),cx-inset*0.4f,cy-inset*0.5f,
                              juce::Colour(0xff080f0b),cx+inset,cy+inset,true);
    dome.addColour(0.66,juce::Colour(0xff1e3024));
    g.setGradientFill(dome);
    g.fillEllipse(cx-inset,cy-inset,2.0f*inset,2.0f*inset);
    g.setColour(Theme::cyan.withAlpha(isBlue?0.30f:0.21f));
    g.drawEllipse(cx-inset,cy-inset,2.0f*inset,2.0f*inset,1.05f*scale);

    // The old red needle was the mismatched highlight in the top bar.
    // Use one restrained neon-green illuminated pointer on every knob.
    juce::Path pointer;
    pointer.startNewSubPath(cx+std::sin(angle)*radius*0.47f,
                            cy-std::cos(angle)*radius*0.47f);
    pointer.lineTo(cx+std::sin(angle)*radius*0.78f,
                   cy-std::cos(angle)*radius*0.78f);
    g.setColour(juce::Colour(0xff021308));
    g.strokePath(pointer,juce::PathStrokeType(4.0f*scale,
                  juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    g.setColour(Theme::cyan.withAlpha(0.20f));
    g.strokePath(pointer,juce::PathStrokeType(5.0f*scale,
                  juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    g.setColour(juce::Colour(0xff81ffa1));
    g.strokePath(pointer,juce::PathStrokeType(2.0f*scale,
                  juce::PathStrokeType::curved,juce::PathStrokeType::rounded));

    if (hasKeyboardFocus(false))
    {
        g.setColour(Theme::cyan.withAlpha(0.8f));
        g.drawEllipse(cx-radius-3.0f*scale,cy-radius-3.0f*scale,
                      2.0f*(radius+3.0f*scale),2.0f*(radius+3.0f*scale),1.3f*scale);
    }
    g.setFont(Theme::font(11.0f*scale,true));
    g.setColour(Theme::label);
    if (name.isNotEmpty())
        g.drawText(name,area.withTop(cy+radius+1.0f*scale),
                   juce::Justification::centredTop);

    // Idle face remains clean: SAT 20%, Width and Mix never stick around.
    // Value feedback is reserved for direct drag, wheel and keyboard edits.
    if (showValue)
    {
        const auto valueRect=small
            ? juce::Rectangle<float>{1.0f,area.getBottom()-17.0f*scale,
                                      area.getWidth()-2.0f,16.0f*scale}
            : juce::Rectangle<float>{cx-32.0f*scale,cy-radius+8.0f*scale,
                                      64.0f*scale,19.0f*scale};
        g.setColour(juce::Colour(0xff051008).withAlpha(0.94f));
        g.fillRoundedRectangle(valueRect,4.0f*scale);
        g.setColour(juce::Colour(0xff4aa963).withAlpha(0.68f));
        g.drawRoundedRectangle(valueRect,4.0f*scale,0.8f*scale);
        g.setColour(juce::Colour(0xffe5ffeb));
        g.setFont(Theme::font(juce::jmax(8.0f,10.3f*scale),true));
        g.drawText(displayValue(),valueRect,juce::Justification::centred);
    }
}
