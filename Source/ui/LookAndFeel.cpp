#include "LookAndFeel.h"
#include "Theme.h"
SkellFanLookAndFeel::SkellFanLookAndFeel()
{
    setSkin(skellcore::ui::SkullSkin::Neon);
}
void SkellFanLookAndFeel::setSkin(skellcore::ui::SkullSkin selected)
{
    skin=selected;
    const auto palette=skellcore::ui::skullPalette(skin);
    setColour(juce::PopupMenu::backgroundColourId,palette.panel);
    setColour(juce::PopupMenu::textColourId,palette.bone);
    setColour(juce::PopupMenu::headerTextColourId,palette.accent);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,palette.panelHi);
    setColour(juce::PopupMenu::highlightedTextColourId,palette.bone);
    setColour(juce::TextEditor::backgroundColourId,palette.panelHi);
    setColour(juce::TextEditor::textColourId,palette.bone);
    setColour(juce::TextEditor::outlineColourId,palette.line);
    setColour(juce::TextEditor::focusedOutlineColourId,palette.accent);
    setColour(juce::AlertWindow::backgroundColourId,palette.panel);
    setColour(juce::AlertWindow::textColourId,palette.bone);
    setColour(juce::AlertWindow::outlineColourId,palette.line);
    setColour(juce::TextButton::buttonColourId,palette.panelHi);
    setColour(juce::TextButton::buttonOnColourId,palette.accent);
    setColour(juce::TextButton::textColourOffId,palette.bone);
}
juce::Font SkellFanLookAndFeel::getPopupMenuFont()
{
    return Theme::font(14.0f,true);
}
void SkellFanLookAndFeel::drawCornerResizer(juce::Graphics& g,int w,int h,
                                           bool hover,bool dragging)
{
    // The default JUCE diagonal white scratches sat outside the fan housing.
    // Keep the resize target functional but integrate its artwork into Skell.
    const auto palette=skellcore::ui::skullPalette(skin);
    const float x=static_cast<float>(w),y=static_cast<float>(h);
    g.setColour(palette.background.withAlpha(0.94f));
    g.fillRoundedRectangle(x-17.0f,y-17.0f,16.0f,16.0f,4.0f);
    g.setColour(palette.accent.withAlpha(dragging?0.95f:hover?0.72f:0.42f));
    for(int row=0;row<3;++row)
        for(int col=0;col<=row;++col)
            g.fillEllipse(x-5.5f-col*4.2f,y-5.5f-row*4.2f,2.0f,2.0f);
}
void SkellFanLookAndFeel::drawPopupMenuBackground(juce::Graphics& g,int w,int h)
{
    const auto palette=skellcore::ui::skullPalette(skin);
    const juce::Rectangle<float> rect{0.0f,0.0f,static_cast<float>(w),static_cast<float>(h)};
    juce::ColourGradient panel(palette.panelHi,rect.getTopLeft(),palette.background,rect.getBottomRight(),false);
    g.setGradientFill(panel);
    g.fillRect(rect);
    g.setColour(palette.accent.withAlpha(0.65f));
    g.drawRect(0,0,w,h,1);
}
