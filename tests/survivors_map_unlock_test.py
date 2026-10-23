"""Map locks, live preview refresh and ranked/cheated browsing and confirmed score reset."""
from pathlib import Path
import os, re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
core=(root/'src/survivors.cpp').read_text()
available=re.search(r'bool map_available\(unsigned map\) \{.*?\n\}',core,re.S).group()
fixture=r'''
bool fateTips=true;bool show_fate_tooltips(){return fateTips;}void set_fate_tooltips(bool value){fateTips=value;}

#include <cassert>
#include <numeric>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include "survivors_rules.hpp"
#include "survivors_maps.hpp"
#include "mods/svc/ui.h"
#define svc_ui test_ui
namespace enemy_hud {ModResult build(UiElementHandle){return MOD_OK;}}
using namespace dawnlight::survivors;
UiDialogHandle s_debugNotice=0;
Records s_records;Run s_run;RunOptions s_options;
bool s_restart=false,s_progressSaving=false,s_hubDeparture=false,s_arenaMeshReady=true;
bool s_storeFailed=false,s_progressSaved=true,s_hubSaveRequested=false,failSave=false;
bool s_active=true,s_gameStarted=true,s_saveReady=true,s_loaded=true,s_hub=true;
unsigned s_map=0;unsigned s_requestedMap=0;UiWindowHandle s_window=1,s_refreshChoices=0;
void open_hud_editor(ModContext*,void*){}
ModContext* mod_ctx=nullptr;constexpr const char* kBlob="test";
std::vector<UiControlDesc> controls;
std::vector<std::string> classes;
std::unordered_map<UiElementHandle,std::string> text;
std::unordered_map<UiElementHandle,bool> visible;
UiElementHandle nextHandle=1;
UiWindowDesc openedWindow{};UiTabDesc openedTab{};UiDialogDesc openedDialog{};
std::array<UiDialogAction,2> actions{};unsigned closedWindows=0,closedDialogs=0;
struct Service {
 ModResult window_push(ModContext*,const UiWindowDesc* desc,UiWindowHandle* h){openedWindow=*desc;openedTab=desc->tabs[0];*h=90;return MOD_OK;}
 ModResult window_close(ModContext*,UiWindowHandle h){++closedWindows;if(openedWindow.on_closed)openedWindow.on_closed(nullptr,h,nullptr);return MOD_OK;}
 ModResult dialog_push(ModContext*,const UiDialogDesc* desc,UiDialogHandle* h){openedDialog=*desc;std::copy_n(desc->actions,desc->action_count,actions.begin());*h=91;return MOD_OK;}
 ModResult dialog_close(ModContext*,UiDialogHandle){++closedDialogs;return MOD_OK;}
 ModResult dialog_set_body(ModContext*,UiDialogHandle,const char* body){text[91]=body;return MOD_OK;}
 ModResult control_set_label(ModContext*,UiElementHandle h,const char* label){text[h]=label;return MOD_OK;}
 ModResult elem_set_class(ModContext*,UiElementHandle,const char* name,bool){classes.emplace_back(name);return MOD_OK;}
 ModResult elem_set_text(ModContext*,UiElementHandle h,const char* s){text[h]=s;return MOD_OK;}
 ModResult elem_set_rml(ModContext* c,UiElementHandle h,const char* s){return elem_set_text(c,h,s);}
 ModResult elem_set_visible(ModContext*,UiElementHandle h,bool v){visible[h]=v;return MOD_OK;}
 ModResult pane_add_text(ModContext*,UiElementHandle,const char* s,UiElementHandle* h){if(h){*h=nextHandle++;text[*h]=s;}return MOD_OK;}
 ModResult pane_add_rml(ModContext* c,UiElementHandle p,const char* s,UiElementHandle* h){return pane_add_text(c,p,s,h);}
 ModResult pane_add_row(ModContext*,UiElementHandle,const UiRowDesc*,UiElementHandle* h){*h=nextHandle++;return MOD_OK;}
 ModResult pane_add_control(ModContext*,UiElementHandle,const UiControlDesc* c,UiElementHandle* h){controls.push_back(*c);if(h){*h=nextHandle++;text[*h]=c->label;}return MOD_OK;}
}ui;auto* svc_ui=&ui;
Records saved;unsigned writes=0;
struct Save {
 ModResult set_blob(ModContext*,const char*,const void* data,size_t size){
  ++writes;if(failSave)return MOD_INVALID_ARGUMENT;
  assert(saved.decode(std::span(static_cast<const uint8_t*>(data),size)));return MOD_OK;
 }
}save;auto* svc_save=&save;
bool settings_editable(){return s_run.phase==Phase::Ready&&!s_restart&&!s_progressSaving&&!s_hubDeparture;}
std::string number(unsigned n){return std::to_string(n);}
std::string treasure_rml_escape(const std::string& s){return s;}
unsigned treasure_item(ChestReward,const Run&){return 0x43;}
const char* reward_name(ChestReward,const Run&){return "Sword";}
void continue_endless(ModContext*,void*){assert(false);}
void save_results(ModContext*,void*){assert(false);}void replay(ModContext*,void*){assert(false);}
void close_high_score(ModContext*,void*);
void request_delete_score(ModContext*,void*);
ModResult button(UiElementHandle pane,const char* label,UiPressedFn callback){UiControlDesc c=UI_CONTROL_DESC_INIT;c.label=label;c.on_pressed=callback;return svc_ui->pane_add_control(nullptr,pane,&c,nullptr);}
void start_run(ModContext*,void*){}
void save_progress(ModContext*,void*){}
void store_option_config(){}
void close_window(){s_window=0;}
'''
main=r'''
int main(){
 s_run.phase=Phase::Ready;
 for(unsigned map=0;map<arenas.size();++map){
  controls.clear();classes.clear();s_requestedMap=map;
  assert(build_arena_choices(1,2)==MOD_OK);
  assert(text[s_mapPreview].find(arenas[map].name)!=std::string::npos);
  assert(text[s_mapPreview].find(arenas[map].description)==std::string::npos);
  // The real window installs this stylesheet outside the isolated builder.
  // Verify that all classes assigned to map controls have matching rules.
  const std::string stylesheet=kArenaStyle;
  for(const auto& name:classes)assert(stylesheet.find("."+name+" {")!=std::string::npos);
  auto start=controls.back();assert(std::string(start.label)=="Start run");
  assert(start.is_disabled(nullptr,nullptr)==(map!=0));
  assert(visible[s_mapLockHint]==(map!=0));
  if(map)assert(text[s_mapLockHint]=="Clear "+std::string(arenas[map-1].name)+" to unlock this map.");
  // A stale debug value cannot unlock maps in a disabled build.
  debug_session_enabled=true;s_options.debug.unlock_all_maps=true;update_arena_tab(nullptr,nullptr,nullptr);
  assert(start.is_disabled(nullptr,nullptr)==(!debug_enabled&&map!=0));
  assert(visible[s_mapLockHint]==(!debug_enabled&&map!=0));
  debug_session_enabled=false;s_options.debug.unlock_all_maps=false;update_arena_tab(nullptr,nullptr,nullptr);
  assert(start.is_disabled(nullptr,nullptr)==(map!=0));
  if(map){s_records.cleared_maps=1u<<(map-1);update_arena_tab(nullptr,nullptr,nullptr);}
  assert(!start.is_disabled(nullptr,nullptr)&&!visible[s_mapLockHint]);
  s_progressSaving=true;assert(start.is_disabled(nullptr,nullptr));s_progressSaving=false;
  s_records.cleared_maps=0;
 }
 // Both lists display an explicit empty value and cannot be opened yet.
 assert(controls[5].is_disabled(nullptr,controls[5].user_data));
 assert(text[s_scoreButtons[0]]=="Ranked: —"&&text[s_scoreButtons[1]]=="Cheated: —");
 Run clean;clean.start(1);clean.ticks=300;clean.kills=20;clean.damage.dealt[unsigned(DamageSource::Bow)]=987;
 clean.phase=Phase::Victory;s_records.finish(clean,s_requestedMap);
 Run assisted=clean;assisted.modded=true;assisted.kills=100;s_records.finish(assisted,s_requestedMap);
 update_arena_tab(nullptr,nullptr,nullptr);
 assert(text[s_scoreButtons[0]]=="Ranked: "+result_number(clean.score()));
 assert(text[s_scoreButtons[1]]=="Cheated: "+result_number(assisted.score()));
 const auto live=s_run;const auto original=s_records.encode();
 for(auto& kind:s_scoreKinds){
  if(kind.endless)continue;
  open_high_score(nullptr,&kind);assert(s_scoreWindow==90);
  controls.clear();assert(openedTab.build(nullptr,90,1,2,nullptr,nullptr)==MOD_OK);
  assert(controls.size()==2&&std::string(controls[0].label)=="Back to maps");
  assert(std::string(controls.back().label)=="Delete Score"&&controls.back().on_pressed==request_delete_score);
  assert(s_scoreView.cheated()==kind.assisted&&s_scoreView.run.damage.dealt==clean.damage.dealt);
  controls[0].on_pressed(nullptr,nullptr);assert(!s_scoreWindow);
  assert(s_run.phase==live.phase&&s_run.damage.dealt==live.damage.dealt&&s_records.encode()==original&&!writes);
 }
 controls.clear();assert(build_debug(nullptr,0,1,2,nullptr,nullptr)==MOD_OK);
 if(debug_enabled){
  const auto found=std::find_if(controls.begin(),controls.end(),[](const auto& control){return std::string(control.label)=="Unlock all maps";});
  assert(found!=controls.end());debug_session_enabled=true;auto toggle=*found;
  assert(toggle.kind==UI_CONTROL_TOGGLE&&std::string(toggle.label)=="Unlock all maps");
  UiControlValue value=UI_CONTROL_VALUE_INIT;value.bool_value=true;toggle.set(nullptr,nullptr,&value);
  assert(s_options.debug.unlock_all_maps);value.bool_value=false;toggle.set(nullptr,nullptr,&value);
 }else assert(controls.empty());
 debug_session_enabled=false;
 controls.clear();assert(build_run_settings(nullptr,0,1,2,nullptr,nullptr)==MOD_OK);
 auto found=std::find_if(controls.begin(),controls.end(),[](const auto& control){return std::string(control.label)=="Reset High Scores";});
 assert(found!=controls.end());auto reset=*found;assert(!reset.is_disabled(nullptr,nullptr));
 reset.on_pressed(nullptr,nullptr);assert(s_scoreResetDialog==91&&openedDialog.variant==UI_DIALOG_WARNING);
 assert(std::string(actions[0].label)=="No"&&std::string(actions[1].label)=="Yes"&&actions[1].keep_open);
 actions[0].on_pressed(nullptr,91,nullptr);assert(!s_scoreResetDialog&&s_records.encode()==original&&!writes);
 request_reset_scores(nullptr,nullptr);openedDialog.on_dismiss(nullptr,91,nullptr);
 assert(!s_scoreResetDialog&&s_records.encode()==original&&!writes);
 // All callbacks must recheck eligibility, including stale confirmation clicks.
 for(int reason=0;reason<9;++reason){
  request_reset_scores(nullptr,nullptr);assert(s_scoreResetDialog);
  if(reason==0)s_restart=true;if(reason==1)s_progressSaving=true;if(reason==2)s_hubDeparture=true;
  if(reason==3)s_run.phase=Phase::Running;if(reason==4)s_active=false;if(reason==5)s_loaded=false;
  if(reason==6)s_saveReady=false;if(reason==7)s_gameStarted=false;if(reason==8)s_hub=false;
  assert(reset.is_disabled(nullptr,nullptr)&&actions[1].is_disabled(nullptr,nullptr));
  actions[1].on_pressed(nullptr,91,nullptr);assert(!writes&&s_records.encode()==original);
  s_restart=s_progressSaving=s_hubDeparture=false;s_run.phase=Phase::Ready;
  s_active=s_loaded=s_saveReady=s_gameStarted=s_hub=true;
  actions[0].on_pressed(nullptr,91,nullptr);
 }
 request_reset_scores(nullptr,nullptr);failSave=true;actions[1].on_pressed(nullptr,91,nullptr);
 assert(s_scoreResetDialog&&s_storeFailed&&s_records.encode()==original&&!s_hubSaveRequested);
 assert(text[91].find("Nothing was reset")!=std::string::npos);
 failSave=false;actions[1].on_pressed(nullptr,91,nullptr);
 assert(!s_scoreResetDialog&&!s_storeFailed&&s_hubSaveRequested&&!s_progressSaved&&writes==2);
 Records expected;assert(expected.decode(original));expected.clear_high_scores();assert(s_records.encode()==expected.encode());
 assert(saved.encode()==expected.encode()&&s_records.map_unlocked(0)&&s_records.cleared_maps);
 update_arena_tab(nullptr,nullptr,nullptr);
 assert(text[s_scoreButtons[0]]=="Ranked: —"&&text[s_scoreButtons[1]]=="Cheated: —");
 // Individual deletion affects only the captured map/category, persists atomically,
 // and never changes the live run, map clears, talents or lifetime totals.
 assert(s_records.decode(original));
 for(unsigned map=0;map<arenas.size();++map){
  s_records.finish(clean,map);s_records.finish(assisted,map);
  auto extra=clean;extra.continue_endless();extra.end(false);s_records.finish(extra,map);
  extra=assisted;extra.continue_endless();extra.end(false);s_records.finish(extra,map);
 }
 for(unsigned map=0;map<arenas.size();++map)for(auto& kind:s_scoreKinds){
  const bool category=kind.assisted;
  s_requestedMap=map;open_high_score(nullptr,&kind);assert(s_scoreWindow);
  const auto before=s_records;const auto oldWrites=writes;
  request_delete_score(nullptr,nullptr);assert(s_scoreDeleteDialog&&openedDialog.variant==UI_DIALOG_WARNING);
  assert(std::string(actions[0].label)=="No"&&std::string(actions[1].label)=="Yes"&&actions[1].keep_open);
  actions[0].on_pressed(nullptr,91,nullptr);assert(!s_scoreDeleteDialog&&s_scoreWindow&&writes==oldWrites);
  request_delete_score(nullptr,nullptr);openedDialog.on_dismiss(nullptr,91,nullptr);
  assert(!s_scoreDeleteDialog&&s_records.encode()==before.encode());
  request_delete_score(nullptr,nullptr);
  s_progressSaving=true;assert(actions[1].is_disabled(nullptr,nullptr));
  actions[1].on_pressed(nullptr,91,nullptr);assert(writes==oldWrites);s_progressSaving=false;
  // A stale view cannot delete a replacement record.
  ++(kind.endless?(category?s_records.endless_cheated:s_records.endless_ranked):(category?s_records.cheated:s_records.ranked))[map].score;
  actions[1].on_pressed(nullptr,91,nullptr);assert(writes==oldWrites);s_records=before;
  failSave=true;actions[1].on_pressed(nullptr,91,nullptr);
  assert(s_storeFailed&&s_scoreDeleteDialog&&s_scoreWindow&&s_records.encode()==before.encode());
  assert(text[91].find("Nothing was deleted")!=std::string::npos);
  // Changing the map selection behind the modal must not change the deletion target.
  s_requestedMap=(map+1)%arenas.size();
  failSave=false;actions[1].on_pressed(nullptr,91,nullptr);
  assert(!s_scoreDeleteDialog&&!s_scoreWindow&&!s_storeFailed&&s_hubSaveRequested);
  auto expected=before;expected.delete_high_score(map,category,kind.endless);
  assert(s_records.encode()==expected.encode()&&saved.encode()==expected.encode());
  assert(!s_records.high_score(map,category,kind.endless).present());
  assert(s_records.high_score(map,!category).encode()==before.high_score(map,!category).encode());
  for(unsigned other=0;other<arenas.size();++other)if(other!=map) {
   assert(s_records.ranked[other].encode()==before.ranked[other].encode());
   assert(s_records.cheated[other].encode()==before.cheated[other].encode());
  }
  assert(s_records.cleared_maps==before.cleared_maps&&s_records.total_xp==before.total_xp);
  assert(s_records.talents.ranks==before.talents.ranks&&s_records.runs==before.runs&&s_records.wins==before.wins);
  assert(s_records.best_ticks==before.best_ticks&&s_records.best_kills==before.best_kills&&s_records.best_level==before.best_level);
  assert(s_run.phase==live.phase&&s_run.damage.dealt==live.damage.dealt);
  const auto written=writes;confirm_delete_score(nullptr,91,nullptr);assert(writes==written);
 }
 // Closing the archived window also closes its confirmation and invalidates callbacks.
 s_records.finish(clean,0);s_requestedMap=0;open_high_score(nullptr,&s_scoreKinds[0]);
 request_delete_score(nullptr,nullptr);assert(s_scoreDeleteDialog);
 const auto beforeClose=s_records.encode();const auto writesBeforeClose=writes;
 openedWindow.on_closed(nullptr,s_scoreWindow,nullptr);
 assert(!s_scoreDeleteDialog&&!s_scoreWindow);
 confirm_delete_score(nullptr,91,nullptr);assert(writes==writesBeforeClose&&s_records.encode()==beforeClose);
 // Teardown invalidates dialogs, archived views and all child control handles.
 request_reset_scores(nullptr,nullptr);close_score_windows();
 confirm_reset_scores(nullptr,91,nullptr);assert(writes==writesBeforeClose&&!s_scoreResetDialog&&!s_scoreDeleteDialog&&!s_scoreWindow&&!s_scoreButtons[0]);

}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+available+(root/'src/survivors_results.inc').read_text()+(root/'src/survivors_scores_ui.inc').read_text()+(root/'src/survivors_maps_ui.inc').read_text()+(root/'src/survivors_debug_ui.inc').read_text()+main)
 for enabled in [0,1]:
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-attributes',f'-DDAWNLIGHT_SURVIVORS_DEBUG={enabled}','-I',str(root/'src'),'-I',str(root/'include'),'-I',str(root/'dusklight/sdk/include'),str(cpp),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
print('Survivors maps: locks, two score categories, archived results, reset confirmation, persistence and guards passed')
