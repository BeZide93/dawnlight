"""Production Survivors HUD editor callbacks, persistence and bounded layout."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <cassert>
#include <map>
#include <string>
#include <vector>
#include "mods/svc/ui.h"
#include "survivors_hud_layout.hpp"
#define svc_ui test_ui
#define svc_config test_config
using namespace dawnlight::survivors;
ModContext* mod_ctx=nullptr;
ConfigService config{};auto* svc_config=&config;
UiService ui{};auto* svc_ui=&ui;
bool s_restart=false,s_progressSaving=false,s_hubDeparture=false,saving=false,transition=false;
bool save_modal_active(){return saving;}bool transitioning(){return transition;}
unsigned warnings=0,pushes=0,closes=0;
struct Logger{void warn(ModContext*,const char*){++warnings;}}logger;auto* svc_log=&logger;
std::map<std::string,ConfigVarHandle> names;std::map<ConfigVarHandle,int64_t> values;
bool failRegister=false,failPush=false,failWrite=false;
ModResult register_owned_setting(const ConfigVarDesc* desc,ConfigVarHandle* handle){
 if(failRegister)return MOD_ERROR;
 assert(desc->type==CONFIG_VAR_INT);
 if(!names.contains(desc->name))names[desc->name]=names.size()+1;
 *handle=names.at(desc->name);values.try_emplace(*handle,desc->default_int);return MOD_OK;
}
std::vector<UiControlDesc> controls;UiTabDesc tab{};UiWindowClosedFn onClosed=nullptr;
'''
main=r'''
int main(){
 config.get_int=[](ModContext*,ConfigVarHandle h,int64_t* value){*value=values.at(h);return MOD_OK;};
 config.set_int=[](ModContext*,ConfigVarHandle h,int64_t value){if(failWrite)return MOD_ERROR;values[h]=value;return MOD_OK;};
 ui.pane_add_control=[](ModContext*,UiElementHandle pane,const UiControlDesc* c,UiElementHandle*){
  assert(pane==1);controls.push_back(*c);return MOD_OK;
 };
 ui.window_push=[](ModContext*,const UiWindowDesc* desc,UiWindowHandle* handle){
  ++pushes;if(failPush)return MOD_ERROR;
  assert(desc->tab_count==1);tab=desc->tabs[0];onClosed=desc->on_closed;
  assert(std::string(desc->rcss).find("background-color: transparent")!=std::string::npos);
  assert(std::string(desc->rcss).find("backdrop-filter: none")!=std::string::npos);
  *handle=100+pushes;return MOD_OK;
 };
 ui.window_close=[](ModContext*,UiWindowHandle h){assert(!s_hudEditorWindow);++closes;onClosed(nullptr,h,nullptr);return MOD_OK;};
 assert(initialize_hud_config()==MOD_OK&&names.size()==31);
 assert(show_fate_tooltips()&&values.at(names.at("survivors-fate-tooltips"))==1);
 set_fate_tooltips(false);assert(!show_fate_tooltips());
 assert(initialize_hud_config()==MOD_OK&&!show_fate_tooltips()); // Reload keeps the persisted choice.
 values.at(names.at("survivors-fate-tooltips"))=1;assert(show_fate_tooltips()); // Import is live.
 failWrite=true;set_fate_tooltips(false);assert(show_fate_tooltips()&&warnings==1);failWrite=false;warnings=0;
 for(unsigned i=0;i<hud_defaults.size();++i)for(unsigned axis=0;axis<3;++axis)assert(hud_value(i,axis)==hud_defaults[i][axis]);
 open_hud_editor(nullptr,nullptr);assert(s_hudEditorWindow&&pushes==1);
 open_hud_editor(nullptr,nullptr);assert(pushes==1);
 assert(tab.build(nullptr,s_hudEditorWindow,1,2,nullptr,nullptr)==MOD_OK&&controls.size()==5);
 assert(controls[0].option_count==10&&std::string(controls[0].options[9])=="Item Banner");
 UiControlValue value=UI_CONTROL_VALUE_INIT;
 for(unsigned i=0;i<10;++i){
  value.int_value=i;controls[0].set(nullptr,nullptr,&value);
  for(unsigned axis=0;axis<3;++axis){
   auto& c=controls[axis+1];assert(c.kind==UI_CONTROL_NUMBER);
   value.int_value=axis==2?150+i:10+i+axis;c.set(nullptr,c.user_data,&value);
   const auto expected=value.int_value;value.int_value=0;c.get(nullptr,c.user_data,&value);assert(value.int_value==expected);
  }
  const auto p=hud_position(HudPart(i),304);assert(p.x==314+i&&p.y==11+i&&p.scale==float(150+i)/100);
 }
 value.int_value=8;controls[0].set(nullptr,nullptr,&value);
 assert(hud_value(9,2)==159&&names.contains("survivors-hud-item-banner-scale"));
 // Re-registering (new session) and host imports use the same persistent values.
 assert(initialize_hud_config()==MOD_OK&&names.size()==31&&hud_value(8,2)==158);
 values[s_hudConfig[8][2]]=-100;assert(hud_position(HudPart::BossBar,304).scale==.1f);
 values[s_hudConfig[8][0]]=100000;assert(hud_value(8,0)==9999);
 values[s_hudConfig[8][2]]=100000;assert(hud_position(HudPart::BossBar,304).scale==4.f);
 value.int_value=-40;controls[3].set(nullptr,controls[3].user_data,&value);assert(hud_value(8,2)==10);
 value.int_value=900;controls[3].set(nullptr,controls[3].user_data,&value);assert(hud_value(8,2)==400);
 for(unsigned reason=0;reason<5;++reason){
  s_restart=reason==0;s_progressSaving=reason==1;s_hubDeparture=reason==2;saving=reason==3;transition=reason==4;
  assert(controls[3].is_disabled(nullptr,nullptr));value.int_value=200;
  controls[3].set(nullptr,controls[3].user_data,&value);controls[4].on_pressed(nullptr,nullptr);
  assert(hud_value(8,2)==400);
 }
 s_restart=s_progressSaving=s_hubDeparture=saving=transition=false;
 controls[4].on_pressed(nullptr,nullptr);
 for(unsigned axis=0;axis<3;++axis)assert(hud_value(8,axis)==hud_defaults[8][axis]);
 assert(hud_value(0,2)==150); // Reset never touches another element.
 failWrite=true;value.int_value=200;controls[3].set(nullptr,controls[3].user_data,&value);
 assert(warnings==1&&hud_value(8,2)==100);failWrite=false;
 const auto old=s_hudEditorWindow;close_hud_editor();assert(!s_hudEditorWindow&&closes==1);
 close_hud_editor();assert(closes==1);open_hud_editor(nullptr,nullptr);
 onClosed(nullptr,old,nullptr);assert(s_hudEditorWindow); // Delayed old callback must not clear a new editor.
 onClosed(nullptr,s_hudEditorWindow,nullptr);assert(!s_hudEditorWindow);
 failPush=true;open_hud_editor(nullptr,nullptr);assert(!s_hudEditorWindow);
 failRegister=true;assert(initialize_hud_config()==MOD_ERROR);
}
'''
with tempfile.TemporaryDirectory() as temp:
 cpp=Path(temp)/'test.cpp';exe=Path(temp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_hud_config.inc').read_text()+(root/'src/survivors_hud_editor.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-I',str(root/'src'),'-I',str(root/'dusklight/sdk/include'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors HUD editor: all ten elements, persistence/import bounds, reset, failures and window lifecycle passed')
