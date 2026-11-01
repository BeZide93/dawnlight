// Optional integration fixture against the host's actual RmlUi fork. See feature docs.
#include <RmlUi/Core.h>
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/StreamMemory.h>
#include <RmlUi/Core/StyleSheetContainer.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

struct Clock : Rml::SystemInterface {
    double now=0;
    double GetElapsedTime() override {return now;}
};
struct Renderer : Rml::RenderInterface {
    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex>, Rml::Span<const int>) override {return 1;}
    void RenderGeometry(Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override {}
    void ReleaseGeometry(Rml::CompiledGeometryHandle) override {}
    Rml::TextureHandle LoadTexture(Rml::Vector2i&, const Rml::String&) override {return 0;}
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override {return 0;}
    void ReleaseTexture(Rml::TextureHandle) override {}
    void EnableScissorRegion(bool) override {}
    void SetScissorRegion(Rml::Rectanglei) override {}
};
std::string read(const std::string& path) {
    std::ifstream stream(path); assert(stream);
    return {std::istreambuf_iterator<char>(stream), {}};
}
std::string literal(const std::string& path, const std::string& name) {
    const auto source=read(path);
    const auto start=source.find("R\"(",source.find(name)); assert(start!=std::string::npos);
    const auto end=source.find(")\";",start); assert(end!=std::string::npos);
    return source.substr(start+3,end-start-3);
}
auto sheet(const std::string& css) {
    auto result=std::make_shared<Rml::StyleSheetContainer>();
    Rml::StreamMemory stream(reinterpret_cast<const Rml::byte*>(css.data()),css.size());
    assert(result->LoadStyleSheetContainer(&stream));
    return result;
}
int main(int argc,char** argv) {
    assert(argc==3); // repository root, Dusklight root
    const std::string root=argv[1], host=argv[2];
    Clock clock; Rml::SetSystemInterface(&clock);
    Renderer renderer;
    Rml::FontEngineInterface fonts;
    Rml::SetFontEngineInterface(&fonts);
    Rml::SetRenderInterface(&renderer);
    assert(Rml::Initialise());
    auto* context=Rml::CreateContext("test",{1280,1080}); assert(context);
    auto* doc=context->CreateDocument(); assert(doc);
    doc->SetInnerRML(R"(
      <window id="own" mod-id="dev.bezide.dawnlight" open>
        <close id="close"/>
        <tab-bar><tab id="tab">Tab</tab></tab-bar>
        <ui-row id="card" class="survivors-treasure-card">
          <div class="survivors-treasure-copy"><img id="icon" style="border-color: #ab34ef;"/></div>
          <button id="pick" class="survivors-treasure-pick"/>
        </ui-row>
        <button id="button"/><button id="disabled" disabled/>
      </window>
      <window id="foreign" mod-id="other.mod" open><button id="foreign-button"/></window>
      <window id="host" open/>
    )");
    std::string base;
    for(const auto* file:{"theme.rcss","global.rcss","controls.rcss","tabbing.rcss","window.rcss","mod_common.rcss"})
        base+=read(host+"/res/rml/"+file)+"\n";
    const auto local=literal(root+"/src/survivors_choice_ui.inc","kTreasureStyle");
    const auto perf=literal(root+"/src/survivors_performance_ui.inc","kSurvivorsPerformanceUiStyle");
    // Host combines scope styles before document-local styles.
    auto element=[&](const char* id){auto* e=doc->GetElementById(id);assert(e);return e;};
    auto values=[&](const char* id)->const Rml::ComputedValues&{return element(id)->GetComputedValues();};
    auto restyle=[&](bool on){doc->SetStyleSheetContainer(sheet(base+(on?perf:"")+local));context->Update();clock.now+=1;context->Update();};
    doc->Show(); restyle(false);
    assert(values("own").has_box_shadow() && values("own").has_backdrop_filter());
    assert(values("card").border_top_left_radius()==8);
    const auto rarity=values("icon").border_top_color();
    const auto disabledOpacity=values("disabled").opacity();
    assert(disabledOpacity<1);
    for(int cycle=0;cycle<3;++cycle) {
        restyle(true);
        assert(!values("own").has_box_shadow() && !values("own").has_backdrop_filter());
        assert(values("own").has_filter()); // Native window fade remains.
        for(const auto* id:{"own","card","icon","pick","button","close"})
            assert(values(id).border_top_left_radius()==0);
        assert(values("foreign").has_box_shadow() && values("foreign").has_backdrop_filter());
        assert(values("host").has_box_shadow());
        assert(values("foreign-button").has_box_shadow());
        assert(values("close").has_decorator()); // Do not erase the close icon.
        assert(values("icon").border_top_color()==rarity);
        assert(values("disabled").opacity()==disabledOpacity);
        for(const auto* state:{"hover","focus-visible","selected","active"}) {
            for(const auto* id:{"pick","button","tab","foreign-button"}) element(id)->SetPseudoClass(state,true);
            context->Update();
            assert(!values("pick").has_box_shadow() && !values("button").has_box_shadow());
            assert(!values("tab").has_font_effect() && !values("tab").has_decorator());
            assert(values("foreign-button").has_box_shadow());
            assert(values("pick").border_top_color()!=Rml::Colourb(120,136,115));
            if(std::string(state)=="selected") assert(values("tab").border_bottom_width()==4);
            if(std::string(state)=="focus-visible") assert(values("tab").background_color().alpha>0);
            for(const auto* id:{"pick","button","tab","foreign-button"}) element(id)->SetPseudoClass(state,false);
        }
        restyle(false);
        assert(values("own").has_box_shadow() && values("own").has_backdrop_filter());
        assert(values("card").border_top_left_radius()==8);
        assert(values("button").has_box_shadow() && values("tab").has_decorator());
    }
    doc->Close(); context->Update(); Rml::Shutdown();
    std::cout<<"Survivors real RmlUi style/cascade/restoration passed\n";
}
