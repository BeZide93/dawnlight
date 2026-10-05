"""Exercise production Midna capture/visual code without game assets or a GPU."""
from pathlib import Path
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
dusk = Path(os.environ.get('DUSKLIGHT_DIR', root / 'dusklight'))
if not dusk.exists():
    dusk = root.parent / 'dusk-source'
fixture = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
using u8=uint8_t;
constexpr uint64_t tag(const char* p){uint64_t v=0;while(*p)v=v*31+*p++;return v;}
#define MULTI_CHAR(x) tag(#x)
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* a,int i){return *static_cast<T*>(static_cast<void**>(a)[i]);}}
struct J2DPane {
 uint64_t mInfoTag=0;u8 alpha=51,mColorAlpha=17;bool visible=true;
 J2DPane *child=nullptr,*next=nullptr;
 bool isVisible(){return visible;}u8 getAlpha(){return alpha;}
 void show(){visible=true;}void hide(){visible=false;}void setAlpha(u8 a){alpha=a;}
 J2DPane* getFirstChildPane(){return child;}J2DPane* getNextChildPane(){return next;}
};
bool active=true,ready=true,boss=false,riding=true,storyBlocked=false;
namespace dSv_event_flag_c {constexpr int M_067=1,F_0800=2;}
bool boss_rush_save_active(){return boss;}
bool dComIfGs_isEventBit(int bit){return bit==dSv_event_flag_c::M_067?riding:storyBlocked;}
// AVAILABILITY
bool dawnlight_touch_ui_active(){return active;}
bool midna_touch_button_ready(){return ready;}
#include "touch_midna_capture.inc"
namespace Rml {
 using String=std::string;
 struct Element {std::map<std::string,std::string> props;bool hidden=false,pressed=false;
  std::string rml;std::array<Element*,2> children{};};
}
int fills=0,releases=0,beeps=0;
struct {
 void (*setPseudoClass)(Rml::Element*,const std::string*,bool)=[](auto* e,auto*,bool v){e->hidden=v;};
 void (*setClass)(Rml::Element*,const std::string*,bool)=[](auto* e,auto*,bool v){e->pressed=v;};
 void (*setRml)(Rml::Element*,const std::string*)=[](auto* e,const auto* r){e->rml=*r;++fills;};
 bool (*releaseTexture)(const std::string*,void*)=[](const auto*,void*){++releases;return true;};
 Rml::Element* (*child)(Rml::Element*,int)=[](auto* e,int i){return e->children[i];};
} s_touchApi;
void extra_property(Rml::Element* e,const char* k,const std::string& v){e->props[k]=v;}
std::string source="meter://midna?slot=1",s_midnaButtonSource;
bool s_midnaButtonFilled=false;int s_midnaPulseFrame=0,s_midnaGlowLevel=-1;
std::string midna_touch_button_icon(){return source;}
struct Draw {float field_0x738=0;} draw;
struct Meter {Draw* getMeterDrawPtr(){return &draw;}} meter;
struct {Meter* getMeterClass(){return &meter;}} g_meter2_info;
constexpr int Z2SE_SY_HINT_BUTTON_BLINK=1;
struct Audio {template<class... T>void seStart(T...){++beeps;}} audio;
Audio* Z2GetAudioMgr(){return &audio;}
#include "dusk/ui/controls.hpp"
#include "touch_button_shape.inc"
#include "touch_button_visibility.inc"
#include "touch_midna_visual.inc"
int main(){
 assert(midna_touch_available());riding=false;assert(!midna_touch_available());
 riding=true;storyBlocked=true;assert(!midna_touch_available());
 boss=true;assert(midna_touch_available());boss=false;storyBlocked=false;
 J2DPane root,icon,hidden,glow,badge;
 root.child=&icon;icon.next=&hidden;hidden.next=&glow;glow.next=&badge;
 hidden.visible=false;glow.mInfoTag=MULTI_CHAR('j_light1');badge.mInfoTag=MULTI_CHAR('hd_mbtn');
 auto* p=&root;void* args[]={&p};
 for(int frame=0;frame<60;++frame){
  root.alpha=u8(frame);root.mColorAlpha=u8(frame+1);
  before_midna_icon_capture(nullptr,args,nullptr,nullptr);
  assert(root.alpha==255&&root.mColorAlpha==255&&icon.alpha==255);
  assert(!hidden.visible&&!glow.visible&&!badge.visible);
  after_midna_icon_capture(nullptr,nullptr,nullptr,nullptr);
  assert(root.alpha==frame&&root.mColorAlpha==frame+1&&icon.alpha==51&&icon.mColorAlpha==17);
  assert(!hidden.visible&&glow.visible&&badge.visible&&s_midnaCaptureCount==0);
 }
 root.hide();before_midna_icon_capture(nullptr,args,nullptr,nullptr);
 assert(root.visible&&hidden.visible&&!glow.visible&&!badge.visible);
 after_midna_icon_capture(nullptr,nullptr,nullptr,nullptr);
 assert(!root.visible&&!hidden.visible&&glow.visible&&badge.visible);
 for(bool* gate:{&active,&ready}){
  *gate=false;before_midna_icon_capture(nullptr,args,nullptr,nullptr);
  assert(!root.visible&&s_midnaCaptureCount==0);after_midna_icon_capture(nullptr,nullptr,nullptr,nullptr);*gate=true;
 }
 p=nullptr;before_midna_icon_capture(nullptr,args,nullptr,nullptr);after_midna_icon_capture(nullptr,nullptr,nullptr,nullptr);
 // A larger modded pane tree cannot overflow storage or leave modified panes behind.
 std::array<J2DPane,100> many;
 for(size_t i=1;i<many.size();++i)many[i-1].child=&many[i];
 p=many.data();before_midna_icon_capture(nullptr,args,nullptr,nullptr);
 assert(s_midnaCaptureCount==64);after_midna_icon_capture(nullptr,nullptr,nullptr,nullptr);
 for(auto& pane:many)assert(pane.alpha==51&&pane.mColorAlpha==17&&pane.visible);
 Rml::Element button,g,i;button.children={&g,&i};
 update_midna_touch_visual(&button,78,46,true,false);
 assert(fills==1&&releases==0&&!button.hidden&&!button.pressed);
 assert(i.props["width"]=="40.000000dp"&&i.props["margin-left"]=="-20.000000dp");
 assert(button.props["border-top-left-radius"]=="23.000000dp");
 assert(button.rml.find("left:50%;top:50%")!=std::string::npos);
 for(int frame=0;frame<60;++frame)update_midna_touch_visual(&button,78,46,true,true);
 assert(fills==1&&button.pressed&&beeps==0); // stable source never rebuilds the DOM
 source="meter://midna?slot=2";update_midna_touch_visual(&button,156,92,true,false);
 assert(fills==2&&releases==1&&i.props["width"]=="80.000000dp");
 draw.field_0x738=1;
 for(int frame=0;frame<60;++frame)update_midna_touch_visual(&button,78,46,true,false);
 assert(beeps==2&&fills==2&&s_midnaPulseFrame==0); // TE's 30-frame hint pulse
 update_midna_touch_visual(&button,78,46,false,true);
 assert(button.hidden&&!button.pressed&&s_midnaPulseFrame==0);
 assert(button.props["pointer-events"]=="none");
 draw.field_0x738=0;update_midna_touch_visual(&button,78,46,true,false);
 assert(g.props["opacity"]=="0.000000"&&button.props["pointer-events"]=="auto");
 source.clear();update_midna_touch_visual(&button,78,46,true,false);
 assert(button.rml.find(">Midna</span>")!=std::string::npos&&releases==2);
 // Dock/undock also reshapes the hint overlay, including after a texture rebuild.
 source="meter://midna?slot=4";
 update_midna_touch_visual(&button,156,92,true,false,dusk::ui::ControlAnchor::Bottom);
 for(auto* node:{&button,&g}){
  assert(node->props["border-bottom-left-radius"]=="0dp");
  assert(node->props["border-bottom-right-radius"]=="0dp");
  assert(node->props["border-top-left-radius"]=="46.000000dp");
 }
 update_midna_touch_visual(&button,156,92,true,false);
 assert(button.props["border-bottom-left-radius"]=="46.000000dp");
 assert(g.props["border-bottom-left-radius"]=="46.000000dp");
 s_touchApi.releaseTexture=nullptr;source="meter://midna?slot=3";
 update_midna_touch_visual(&button,78,46,true,false); // optional release API
}
'''
item_source = (root/'src/hud_touch_hooks.cpp').read_text()
start = item_source.index('bool midna_touch_available()')
fixture = fixture.replace('// AVAILABILITY', item_source[start:item_source.index('\n}', start)+2])
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-multichar',
                    '-I'+str(root/'src'), '-I'+str(dusk/'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
source = (root/'src/hud_touch_hooks.cpp').read_text()
assert 'refresh_midna_touch_icon_texture' not in source
assert 'hook_add_pre<UpdateMidnaIconTextureHook>' in source
assert 'hook_add_post<UpdateMidnaIconTextureHook>' in source
print('Midna visual passed: stable capture, full pane restoration, HD/glow exclusion, cached icon, TE styling/pulse, hide/reset')
