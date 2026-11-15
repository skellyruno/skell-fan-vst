// SkellFan real JUCE editor local Quick4, independent of Windows release.
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/ui/TopBar.h"
#include <array>
#include <set>
#include <memory>
#include <cstdio>
#include <cmath>

namespace {
bool setParameter(SkellFanAudioProcessor& owner,const char* id,float value) {
    auto* p=owner.apvts.getParameter(id);
    if(!p)return false;
    p->beginChangeGesture();
    p->setValueNotifyingHost(p->convertTo0to1(value));
    p->endChangeGesture();
    return true;
}
bool writePng(const juce::Image& img,const juce::File& file) {
    if(!img.isValid())return false;
    auto out=file.createOutputStream();
    if(!out)return false;
    const bool result=juce::PNGImageFormat().writeImageToStream(img,*out);
    out->flush();
    return result && file.getSize()>1024;
}
int colours(const juce::Image& img) {
    std::set<juce::uint32> unique;
    for(int y=2;y<img.getHeight();y+=8)
        for(int x=2;x<img.getWidth();x+=8)
            unique.insert(img.getPixelAt(x,y).getARGB());
    return static_cast<int>(unique.size());
}
juce::Button* findSkull(juce::Component& editor) {
    for(int i=0;i<editor.getNumChildComponents();++i) {
        auto* top=dynamic_cast<TopBar*>(editor.getChildComponent(i));
        if(!top)continue;
        for(int j=0;j<top->getNumChildComponents();++j) {
            auto* c=top->getChildComponent(j);
            if(c && c->getName()=="SkellFan fan-skull")
                return dynamic_cast<juce::Button*>(c);
        }
    }
    return nullptr;
}
bool layoutOk(juce::Component& editor) {
    const auto bounds=editor.getLocalBounds();
    for(int i=0;i<editor.getNumChildComponents();++i) {
        const auto* c=editor.getChildComponent(i);
        if(c && c->isVisible() && !bounds.contains(c->getBounds()))return false;
    }
    return true;
}
void driveRealAudio(SkellFanAudioProcessor& p) {
    juce::AudioBuffer<float> audio(2,256);
    juce::MidiBuffer midi;
    for(int pass=0;pass<20;++pass) {
        for(int n=0;n<256;++n) {
            const float x=.28f*std::sin(float(pass*256+n)*.12f);
            audio.setSample(0,n,x);
            audio.setSample(1,n,x*.8f);
        }
        p.processBlock(audio,midi);
    }
}
} // namespace

int main(int argc,char** argv) {
    juce::String outputName="ui-quick4";
    bool full=false;
    for(int i=1;i<argc;++i) {
        const juce::String arg(argv[i]);
        if(arg=="--output" && i+1<argc)outputName=argv[++i];
        else if(arg=="--full")full=true;
        else {std::fprintf(stderr,"Usage: SkellFanQuick4 [--output path] [--full]\n");return 2;}
    }
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File output=juce::File::isAbsolutePath(outputName)
        ? juce::File(outputName)
        : juce::File::getCurrentWorkingDirectory().getChildFile(outputName);
    if(output.createDirectory().failed())return 3;
    SkellFanAudioProcessor p;
    p.prepareToPlay(48000.,512);
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    if(!editor || !findSkull(*editor)) {
        std::fprintf(stderr,"Missing real SkellFan editor/skull control\n");return 4;
    }
    struct Scene{const char* id;int width,height,kind;};
    const std::array<Scene,8> scenes{{
        {"main_default",765,787,0},
        {"rotary_controls",765,787,1},
        {"active_processing",765,787,2},
        {"theme_alternate",765,787,3},
        {"compact_574",574,590,0},
        {"large_956",956,984,0},
        {"max_1434",1434,1476,0},
        {"other_mode",765,787,4}
    }};
    juce::Array<juce::var> rows;
    juce::Image sheet(juce::Image::RGB,800*2,825*2,true);
    juce::Graphics grid(sheet);
    grid.fillAll(juce::Colour(0xff090e0c));
    int failures=0;
    for(int i=0;i<(full?8:4);++i) {
        const auto& scene=scenes[size_t(i)];
        p.loadFactoryPreset(0);
        // Return to the original Neon skin without manipulating DSP.
        for(int tries=0;tries<4;++tries) {
            const auto skin=static_cast<int>(p.apvts.state.getProperty("uiSkullSkin",1));
            if(skin==1)break;
            if(auto* skull=findSkull(*editor)) {
                if(skull->onClick) skull->onClick();
            }
        }
        bool ok=true;
        if(scene.kind==1) {
            ok=setParameter(p,"mode",2.f)
                && setParameter(p,"depth",45.f);
        } else if(scene.kind==2) {
            ok=setParameter(p,"mode",1.f)
                && setParameter(p,"depth",80.f)
                && setParameter(p,"mix",80.f);
            driveRealAudio(p);
        } else if(scene.kind==3) {
            if(auto* skull=findSkull(*editor)) {
                if(skull->onClick) skull->onClick();
                else ok=false;
            }
            else ok=false;
        } else if(scene.kind==4) {
            ok=setParameter(p,"mode",0.f);
        }
        if(scene.kind==3
            && static_cast<int>(p.apvts.state.getProperty("uiSkullSkin",1))==1)
            ok=false;  // Theme callback never changed the shared skull palette.
        if(!ok){++failures;break;}
        editor->setSize(scene.width,scene.height);
        editor->resized();
        juce::Thread::sleep(90);
        juce::Timer::callPendingTimersSynchronously();
        const auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true);
        if(!image.isValid() || image.getWidth()!=scene.width || image.getHeight()!=scene.height
           || colours(image)<12 || !layoutOk(*editor)) {
            std::fprintf(stderr,"Invalid SkellFan scene: %s\n",scene.id);
            ++failures;continue;
        }
        const auto file=output.getChildFile(juce::String(scene.id)+".png");
        if(!writePng(image,file)) {++failures;continue;}
        juce::var item(new juce::DynamicObject());
        item.getDynamicObject()->setProperty("file",file.getFileName());
        item.getDynamicObject()->setProperty("width",scene.width);
        item.getDynamicObject()->setProperty("height",scene.height);
        item.getDynamicObject()->setProperty("sampledColours",colours(image));
        rows.add(item);
        if(i<4) {
            const int x=(i%2)*800,y=(i/2)*825;
            grid.setColour(juce::Colours::white);
            grid.setFont(juce::Font(juce::FontOptions(16.f)));
            grid.drawText(juce::String(scene.id).replaceCharacter('_',' '),
                          x+12,y+6,770,25,juce::Justification::centredLeft);
            grid.drawImageWithin(image,x+10,y+32,780,785,
                                 juce::RectanglePlacement::centred);
        }
    }
    if(!writePng(sheet,output.getChildFile("contact-sheet.png")))++failures;
    juce::var manifest(new juce::DynamicObject());
    manifest.getDynamicObject()->setProperty("project","SkellFan real JUCE editor");
    manifest.getDynamicObject()->setProperty("count",rows.size());
    manifest.getDynamicObject()->setProperty("scenes",juce::var(rows));
    if(!output.getChildFile("manifest.json").replaceWithText(
        juce::JSON::toString(manifest,false)))++failures;
    p.releaseResources();
    std::printf("SkellFan actual local JUCE UI: %d/%d, errors %d\n",
                rows.size(),full?8:4,failures);
    return failures==0 && rows.size()==(full?8:4)?0:1;
}
