"""Exercise production touch visibility gates and input cleanup without game assets."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
native = (root / 'src/touch_buttons_native.inc').read_text()

def function(name):
    start = native.index(name + '(')
    return native[start:native.index('\n}', start) + 2]

fixture = r'''
#include <string>
#include <string_view>
#include <map>
#include <cassert>
#include "touch_button_state.hpp"
using namespace dawnlight;
namespace Rml {
 using String=std::string;
 struct Element {bool hidden=false;Element* child=nullptr;std::map<std::string,std::string> styles;};
}
namespace dusk::config {
 struct ConfigVarBase {};
 template<class T>struct ConfigVar:ConfigVarBase {T value=true;T getValue()const{return value;}};
}
dusk::config::ConfigVar<bool> touchConfig;
bool configPresent=true;
struct NativeTouch {bool mWasSuppressed=false,visible=true;Rml::Element* mActionBar=nullptr;};
struct {
 bool (*visible)(const NativeTouch*)=[](const NativeTouch* p){return p->visible;};
 dusk::config::ConfigVarBase* (*config)(std::string_view)=[](std::string_view)->dusk::config::ConfigVarBase*{return configPresent?&touchConfig:nullptr;};
 bool (*isPseudoClassSet)(const Rml::Element*,const Rml::String*)=[](auto* e,auto*){return e->hidden;};
 void (*setPseudoClass)(Rml::Element*,const Rml::String*,bool)=[](auto* e,auto*,bool v){e->hidden=v;};
 Rml::Element* (*child)(Rml::Element*,int)=[](auto* e,int){return e->child;};
} s_touchApi;
void extra_property(Rml::Element* e,const char* k,const std::string& v){e->styles[k]=v;}
#include "touch_button_visibility.inc"
bool s_extraHooksReady=true,s_extraSessionEnabled=true;
bool player=true,event=false,pause=false,nextStage=false,shop=false,talk=false;
const char* stage="F_SP103";int window=0;
struct {int pause=0;int getPauseStatus(){return pause;}} g_meter2_info;
void* dComIfGp_getLinkPlayer(){return player?&player:nullptr;}
bool dComIfGp_event_runCheck(){return event;}
bool dComIfGp_isPauseFlag(){return pause;}
bool dComIfGp_isEnableNextStage(){return nextStage;}
const char* dComIfGp_getStartStageName(){return stage;}
int dMeter2Info_getWindowStatus(){return window;}
bool dMeter2Info_isShopTalkFlag(){return shop;}
void* dComIfGp_getMsgObjectClass(){return &talk;}
bool dMsgObject_isTalkNowCheck(){return talk;}
// AVAILABLE
std::array<bool,touch::Count> enabled;
bool midnaReady=true;
bool button_enabled(size_t i){return enabled[i];}
bool midna_touch_button_available(){return midnaReady;}
touch::Presses s_extraPresses;
touch::ActionInput s_jumpTouchInput,s_darkLinkTouchInput;
bool s_midnaTouchPending=false,s_midnaTouchTriggered=false;
// PRUNE
int main(){
 Rml::Element bar,utility,host;bar.child=&utility;
 NativeTouch controls;controls.mActionBar=&host;enabled.fill(true);
 assert(extra_controls_available(&controls));
 for(const char* name:{"F_SP102","title","opening","name",""}){
  stage=name;assert(!extra_controls_available(&controls));
 }
 stage=nullptr;assert(!extra_controls_available(&controls));stage="F_SP103";
 assert(extra_controls_available(&controls)); // Ordon gameplay is not the title demo.
 host.hidden=true;assert(!extra_controls_available(&controls));
 // A native hidden bar covers faders, heap locks, scene overlaps and skip scenes.
 // Returning window status to zero must not reveal extras ahead of the host.
 window=0;pause=false;assert(!extra_controls_available(&controls));
 host.hidden=false;assert(extra_controls_available(&controls));
 controls.mActionBar=nullptr;assert(!extra_controls_available(&controls));controls.mActionBar=&host;
 for(bool* gate:{&s_extraHooksReady,&s_extraSessionEnabled,&controls.visible,&touchConfig.value,&configPresent,&player}){
  *gate=false;assert(!extra_controls_available(&controls));*gate=true;
 }
 for(bool* gate:{&controls.mWasSuppressed,&event,&pause,&nextStage,&shop,&talk}){
  *gate=true;assert(!extra_controls_available(&controls));*gate=false;
 }
 g_meter2_info.pause=1;assert(!extra_controls_available(&controls));g_meter2_info.pause=0;
 window=1;assert(!extra_controls_available(&controls));window=0;
 for(int i=0;i<5;++i){
  set_extra_element_visibility(&bar,false,true);
  assert(bar.hidden&&bar.styles.at("pointer-events")=="none"&&utility.styles.at("pointer-events")=="none");
  assert(!bar.styles.contains("display")); // Retain native opacity transitions.
  set_extra_element_visibility(&bar,true,true);
  assert(!bar.hidden&&bar.styles.at("pointer-events")=="auto"&&utility.styles.at("pointer-events")=="auto");
 }
 // Blocked UI transitions immediately drop held/queued actions, despite fade-out.
 for(size_t i=0;i<touch::Count;++i)assert(s_extraPresses.press(i,i));
 s_jumpTouchInput.press();s_darkLinkTouchInput.press();
 s_midnaTouchPending=s_midnaTouchTriggered=true;host.hidden=true;
 prune_extra_presses(&controls);
 for(size_t i=0;i<touch::Count;++i)assert(!s_extraPresses.held(i));
 assert(!s_midnaTouchPending&&!s_midnaTouchTriggered);
 s_jumpTouchInput.sample();s_darkLinkTouchInput.sample();
 assert(!s_jumpTouchInput.pressed&&!s_jumpTouchInput.pending&&!s_darkLinkTouchInput.pressed);
 host.hidden=false;prune_extra_presses(&controls);
 assert(!s_extraPresses.held(touch::Jump));
}
'''
fixture = fixture.replace('// AVAILABLE', function('bool extra_controls_available'))
fixture = fixture.replace('// PRUNE', function('void prune_extra_presses'))
# Visibility must never collapse a live button or bypass the native fade.
assert '"display"' not in function('void update_extra_elements')
assert 'display:none' not in function('bool create_extra_elements')
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I'+str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Touch visibility passed: native-bar synchronization, title/loading/menu gates, transition-safe hiding and input cleanup')
