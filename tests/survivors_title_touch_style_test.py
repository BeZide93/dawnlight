"""Exercise native touch RCSS with the project's real RmlUi parser/style engine.

Usage: python3 tests/survivors_title_touch_style_test.py RMLUI_SOURCE RMLUI_BUILD
Build RmlUi with -DRMLUI_FONT_ENGINE=none -DBUILD_SHARED_LIBS=OFF first.
No graphics device, game assets or font files are required.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
rml_source, rml_build = map(Path, sys.argv[1:])
title = (root / 'src/survivors_title_touch.inc').read_text().split('R"RCSS(')[1].split(')RCSS"')[0]
native = (root / 'dusklight/res/rml/touch_controls.rcss').read_text()
fragment = (root / 'dusklight/src/dusk/ui/touch_controls_common.cpp').read_text().split('R"RML(')[1].split(')RML"')[0]
cpp = r'''
#include <RmlUi/Core.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
struct Renderer:Rml::RenderInterface {
 Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex>,Rml::Span<const int>)override{return 1;}
 void RenderGeometry(Rml::CompiledGeometryHandle,Rml::Vector2f,Rml::TextureHandle)override{}
 void ReleaseGeometry(Rml::CompiledGeometryHandle)override{}
 Rml::TextureHandle LoadTexture(Rml::Vector2i&,const Rml::String&)override{return 0;}
 Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>,Rml::Vector2i)override{return 0;}
 void ReleaseTexture(Rml::TextureHandle)override{}
 void EnableScissorRegion(bool)override{} void SetScissorRegion(Rml::Rectanglei)override{}
};
struct System:Rml::SystemInterface {
 bool parseError=false;
 bool LogMessage(Rml::Log::Type,const Rml::String& message)override {
  if(message.find("Syntax error")!=std::string::npos){parseError=true;std::cerr<<message<<'\n';}
  return true;
 }
};
int main(int,char**argv){
 Renderer render;System system;Rml::FontEngineInterface font;
 Rml::SetRenderInterface(&render);Rml::SetSystemInterface(&system);Rml::SetFontEngineInterface(&font);
 assert(Rml::Initialise());auto*c=Rml::CreateContext("test",{1280,720});
 std::ifstream f(argv[1]);std::stringstream s;s<<f.rdbuf();
 auto*d=c->LoadDocumentFromMemory(s.str());assert(d);d->Show();
 auto*r=d->GetElementById("root");r->SetAttribute("open","");
 auto get=[&](const char* id){auto*e=d->GetElementById(id);assert(e);return e;};
 auto value=[](Rml::Element* e,const char* p){return e->GetProperty(p)->ToString();};
 auto*a=get("button-a");auto*b=get("button-b");
 a->SetProperty("left","100px");b->SetProperty("left","200px");
 for(auto*e:{a,b}){e->SetProperty("top","100px");e->SetProperty("width","70px");e->SetProperty("height","70px");e->SetProperty("transition","none");}
 b->SetClass("has-item",true);get("button-b-icon")->SetClass("visible",true);
 for(bool title:{false,true,false,true}){
  // Even native :hidden must not hide A/B in the cinematic. Other native
  // controls start visible, as they do in the custom hub without a demo event.
  for(auto*e:{a,b})e->SetPseudoClass("hidden",true);
  r->SetClass("dawnlight-survivors-title",title);c->Update();
  for(auto id:{"trigger-l","trigger-r","button-x","button-y","button-z","skip","action-bar","control-stick","extra"}){
   auto*e=get(id);assert((value(e,"display")=="none")==title);
   assert((value(e,"pointer-events")=="none")== (title || std::string(id)=="control-stick"));
  }
  for(auto*e:{a,b}){
   assert(value(e,"display")=="flex");
   assert((value(e,"opacity")=="1")==title);
   assert((value(e,"pointer-events")=="auto")==title);
  }
  assert((value(get("button-b-icon"),"display")=="none")==title);
  if(title){
   auto*hit=c->GetElementAtPoint({135,135});
   while(hit&&hit!=a)hit=hit->GetParentNode();assert(hit==a);
   hit=c->GetElementAtPoint({235,135});
   while(hit&&hit!=b)hit=hit->GetParentNode();assert(hit==b);
  }
 }
 assert(!system.parseError);Rml::Shutdown();
 std::cout<<"Native RmlUi title styles: A/B visibility and hit tests, hidden native/custom controls, repeated mode switching passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    (tmp / 'test.cpp').write_text(cpp)
    # The host supplies these theme variables before the native touch sheet.
    theme = 'body { --font-size-4xl: 28dp; --font-size-3xl: 24dp; --font-size-xs: 12dp; --font-size-2xs: 10dp; --color-white-rgb: 255,255,255; --color-black-rgb: 0,0,0; --radius-control: 4dp; --radius-small: 2dp; --space-2xs: 2dp; }'
    (tmp / 'test.rml').write_text('<rml><head><style>' + theme + native + title + '</style></head><body id="root"><touch-stick id="control-stick"/>' + fragment + '<div id="extra"><button>Extra</button></div></body></rml>')
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-I'+str(rml_source/'Include'), str(tmp/'test.cpp'), str(rml_build/'librmlui.a'), '-o', str(tmp/'test')], check=True)
    subprocess.run([str(tmp/'test'), str(tmp/'test.rml')], check=True)
