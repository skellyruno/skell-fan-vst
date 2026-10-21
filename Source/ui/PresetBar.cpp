#include "PresetBar.h"
#include "Theme.h"
namespace
{
constexpr const char* factories[] = {"Chorus - Classic","Chorus - Wide",
 "Vibrato - Tape","Rotary - Vintage","Tremolo - Pulse","Rotary - Slow"};
constexpr int presetCount=6;
}
PresetBar::PresetBar(BreezeModAudioProcessor& proc):p(proc)
{
    for(auto* b : {&prev,&next,&current,&save,&remove,&shuffle})
    {
        addAndMakeVisible(*b);
        b->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff171719));
        b->setColour(juce::TextButton::textColourOffId,juce::Colour(0xffedf1f5));
        b->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    prev.setButtonText("▲");next.setButtonText("▼");
    save.setButtonText("▣");remove.setButtonText("▤");shuffle.setButtonText("⤨");
    current.onClick=[this]{popupMenu();};
    prev.onClick=[this]{chooseFactory((factoryIndex+presetCount-1)%presetCount);};
    next.onClick=[this]{chooseFactory((factoryIndex+1)%presetCount);};
    save.onClick=[this]{storePreset();};
    remove.onClick=[this]{erasePreset();};
    shuffle.onClick=[this]{randomize();};
    timerCallback();startTimerHz(6);
}
void PresetBar::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0b0b0c));
    g.setColour(juce::Colour(0xff3b3d41));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f),4.0f,1.0f);
}
void PresetBar::resized()
{
    const auto sx=getWidth()/420.0f,sy=getHeight()/38.0f;
    const auto place=[sx,sy](juce::Component& c,int x,int y,int w,int h)
    {c.setBounds(juce::roundToInt(x*sx),juce::roundToInt(y*sy),
                 juce::roundToInt(w*sx),juce::roundToInt(h*sy));};
    place(prev,0,0,31,19);place(next,0,19,31,19);
    place(current,34,0,267,38);place(save,304,0,36,38);
    place(remove,343,0,36,38);place(shuffle,382,0,38,38);
}
void PresetBar::timerCallback()
{
    const auto text=name();
    for(int i=0;i<presetCount;++i)
        if(text==factories[i]) factoryIndex=i;
    if(current.getButtonText()!=text)current.setButtonText(text);
}
juce::String PresetBar::name() const
{return p.apvts.state.getProperty("uiPresetName","Chorus - Classic").toString();}
juce::File PresetBar::folder() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
       .getChildFile("Skell Audio").getChildFile("BreezeMod").getChildFile("Presets");
}
void PresetBar::chooseFactory(int i)
{factoryIndex=juce::jlimit(0,presetCount-1,i);p.loadFactoryPreset(factoryIndex);timerCallback();}
void PresetBar::popupMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader("FACTORY PRESETS");
    for(int i=0;i<presetCount;++i)menu.addItem(i+1,factories[i]);
    menu.addSeparator();menu.addSectionHeader("USER PRESETS");
    juce::Array<juce::File> files;
    for(const auto& file : folder().findChildFiles(juce::File::findFiles,false,"*.xml"))
    {
        files.add(file);
        menu.addItem(1000+files.size()-1,file.getFileNameWithoutExtension());
    }
    if(files.isEmpty())menu.addItem(999,"No saved presets",false);
    juce::Component::SafePointer<PresetBar> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&current),
        [safe,files](int choice)
        {
            if(safe==nullptr)return;
            if(choice>=1&&choice<=presetCount){safe->chooseFactory(choice-1);return;}
            const int idx=choice-1000;
            if(idx>=0&&idx<files.size())
            {
                if(auto xml=juce::XmlDocument::parse(files[idx]))
                {
                    if(xml->hasTagName(safe->p.apvts.state.getType().toString()))
                    {
                        safe->p.apvts.replaceState(juce::ValueTree::fromXml(*xml));
                        safe->p.apvts.state.setProperty("uiPresetName",files[idx].getFileNameWithoutExtension(),nullptr);
                        safe->timerCallback();
                    }
                }
            }
        });
}
void PresetBar::storePreset()
{
    auto* dialog=new juce::AlertWindow("Save BreezeMod preset","Preset name",
                                       juce::MessageBoxIconType::NoIcon);
    dialog->addTextEditor("name",name(),"Name");
    dialog->addButton("Save",1);dialog->addButton("Cancel",0);
    juce::Component::SafePointer<PresetBar> safe(this);
    dialog->enterModalState(true,juce::ModalCallbackFunction::create([safe,dialog](int result)
    {
        if(result==1&&safe!=nullptr)
        {
            const auto title=juce::File::createLegalFileName(dialog->getTextEditorContents("name").trim());
            if(title.isNotEmpty())
            {
                auto directory=safe->folder();directory.createDirectory();
                auto tree=safe->p.apvts.copyState();
                tree.setProperty("uiPresetName",title,nullptr);
                if(auto xml=tree.createXml())
                    if(xml->writeTo(directory.getChildFile(title+".xml")))
                        safe->p.apvts.state.setProperty("uiPresetName",title,nullptr);
            }
        }
        delete dialog;
    }),false);
}
void PresetBar::erasePreset()
{
    const auto nameNow=name();
    bool factory=false;
    for(auto* item:factories)if(nameNow==item)factory=true;
    const auto file=folder().getChildFile(juce::File::createLegalFileName(nameNow)+".xml");
    if(factory||!file.existsAsFile())return;
    juce::Component::SafePointer<PresetBar> safe(this);
    juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon,
        "Delete preset?","Delete '"+nameNow+"' from your saved presets?","Delete","Cancel",
        this,juce::ModalCallbackFunction::create([safe,file](int result)
        {
            if(result==1&&safe!=nullptr){file.deleteFile();safe->chooseFactory(0);}
        }));
}
void PresetBar::randomize()
{
    auto& rng=juce::Random::getSystemRandom();
    auto param=[this](const char* id,float v)
    {
        if(auto* a=p.apvts.getParameter(id))
        {a->beginChangeGesture();a->setValueNotifyingHost(a->convertTo0to1(v));a->endChangeGesture();}
    };
    param("mode",static_cast<float>(rng.nextInt(4)));
    param("depth",static_cast<float>(10+rng.nextInt(71)));
    param("rate",0.05f+std::pow(rng.nextFloat(),2.0f)*8.0f);
    param("syncMode",rng.nextBool()?1.0f:0.0f);
    param("syncDiv",static_cast<float>(rng.nextInt(7)));
    param("noteType",static_cast<float>(rng.nextInt(3)));
    param("wave",static_cast<float>(rng.nextInt(4)));
    param("saturation",static_cast<float>(rng.nextInt(61)));
    param("width",static_cast<float>(rng.nextInt(101)));
    param("mix",static_cast<float>(25+rng.nextInt(66)));
    p.apvts.state.setProperty("uiPresetName","Randomized",nullptr);
}
