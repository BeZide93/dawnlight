"""Exercise production registration: report the exact failed hook and stop."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/survivors.cpp').read_text()
body=source[source.index('ModResult initialize_survivors('):source.index('void shutdown_survivors()')]
body=body.replace('#include "survivors_roster_hooks.inc"',(root/'src/survivors_roster_hooks.inc').read_text())
hooks=re.findall(r'\b(?:PRE|POST)\((\w+),\s*(\w+)\);',body)
fixture=r'''
#include <cassert>
#include <string>
#include <vector>
using ModResult=int;constexpr int MOD_OK=0,MOD_UNAVAILABLE=1;
struct ModError{std::string message;};
#define MOD_ERROR_INIT {}
int svc_hook=0;void* mod_ctx=nullptr;bool s_registered=false;
unsigned failAt=0,calls=0,registrations=0,initFailure=0,warnings=0;bool modeFailure=false,unregisterFailure=false,enabled=true,active=false;
unsigned removals=0,deactivations=0;
bool survivors_enabled(){return enabled;}
bool s_enemyArchivesAvailable=false;
struct SurvivorsArchiveReady{static constexpr const char* name="SurvivorsArchiveReady";};
constexpr int after_enemy_archive_ready=0;
struct Log{void warn(void*,const char*){++warnings;}}logService;
auto* svc_log=&logService;
std::vector<std::string> attempted;
namespace mods{
int set_error(ModError* e,int result,const char* text){if(e)e->message=text;return result;}
template<class H>int hook_add_pre(int,int){attempted.push_back(H::name);return ++calls==failAt?MOD_UNAVAILABLE:MOD_OK;}
template<class H>int hook_add_post(int service,int callback){return hook_add_pre<H>(service,callback);}
}
int init(unsigned n,ModError* e){return initFailure==n?mods::set_error(e,MOD_UNAVAILABLE,"initializer-specific failure"):MOD_OK;}
int initialize_survivors_integrity(ModError* e){return init(1,e);}
int initialize_save_pointer(ModError* e){return init(4,e);}
int initialize_hud_config(){return init(5,nullptr);}
namespace enemy_hud {int initialize(){return init(6,nullptr);}}
int initialize_option_config(){return init(2,nullptr);}
int initialize_midna_dialog(){return init(3,nullptr);}
void initialize_menu_handoff(){}
constexpr int activated=0,deactivated=0,loaded=0,new_save=0,reset=0;
constexpr const char* kMode="survivors";
struct GameModeDesc{unsigned struct_size;const char* game_mode_id;const char* full_name;const char* save_name;void* user_data;
 int on_activated,on_deactivated,on_play,on_save_loaded,on_new_save;void* on_new_save_select;int on_game_reset;void* on_tick;};
struct Service{
 int register_game_mode(void*,const GameModeDesc* mode){assert(std::string(mode->save_name)=="gczelda2-survivors");++registrations;return modeFailure?MOD_UNAVAILABLE:MOD_OK;}
 int unregister_game_mode(void*,const char* id){assert(std::string(id)==kMode);++removals;if(unregisterFailure)return MOD_UNAVAILABLE;if(active){active=false;++deactivations;}return MOD_OK;}
}service;
auto* svc_game_mode=&service;
'''
for alias in sorted({h for h,_ in hooks}):fixture+=f'struct {alias}{{static constexpr const char* name="{alias}";}};\n'
for callback in sorted({c for _,c in hooks}):fixture+=f'constexpr int {callback}=0;\n'
main=r'''
int main(){
 ModError error;assert(initialize_survivors(&error)==MOD_OK&&s_registered&&registrations==1);
 assert(s_enemyArchivesAvailable&&!warnings);
 const auto expected=attempted;
 assert(expected.front()=="SurvivorsArchiveReady");
 for(unsigned i=1;i<=expected.size();++i){
  failAt=i;calls=registrations=0;attempted.clear();s_registered=false;error={};
  if(i==1){
   assert(initialize_survivors(&error)==MOD_OK&&s_registered&&registrations==1);
   assert(!s_enemyArchivesAvailable&&warnings==1&&error.message.empty()&&attempted==expected);
   continue;
  }
  assert(initialize_survivors(&error)==MOD_UNAVAILABLE&&!s_registered&&!registrations);
  assert(calls==i&&attempted.back()==expected[i-1]&&error.message.find(expected[i-1])!=std::string::npos);
 }
 failAt=0;
 for(initFailure=1;initFailure<=6;++initFailure){
  calls=registrations=0;error={};
  assert(initialize_survivors(&error)==MOD_UNAVAILABLE&&!calls&&!registrations);
  assert(error.message==((initFailure==1||initFailure==4)?"initializer-specific failure":initFailure==2?
   "failed to initialize Hyrule Survivors settings":initFailure==6?"failed to initialize Survivors enemy HUD settings":initFailure==5?"failed to initialize Survivors HUD settings":"failed to initialize Hyrule Survivors Midna dialog"));
 }
 initFailure=0;modeFailure=true;calls=registrations=0;error={};
 assert(initialize_survivors(&error)==MOD_UNAVAILABLE&&!s_registered&&registrations==1);
 assert(error.message=="failed to register Hyrule Survivors");
 modeFailure=false;enabled=false;registrations=0;
 assert(initialize_survivors(&error)==MOD_OK&&!s_registered&&!registrations);
 update_survivors_registration();assert(!registrations&&!removals);
 enabled=true;modeFailure=true;update_survivors_registration();assert(!s_registered&&registrations==1);
 modeFailure=false;update_survivors_registration();assert(s_registered&&registrations==2);
 update_survivors_registration();assert(registrations==2); // Never duplicate the mode.
 active=true;enabled=false;unregisterFailure=true;
 update_survivors_registration();assert(s_registered&&active&&!deactivations);
 unregisterFailure=false;update_survivors_registration();assert(!s_registered&&!active&&deactivations==1);
 update_survivors_registration();assert(removals==2);
 enabled=true;update_survivors_registration();assert(s_registered&&registrations==3);

}
'''
# These particular v2.0.3 x64 entry branches cannot be relocated by funchook.
assert 'DEFINE_HOOK(&dSv_player_status_a_c::setSelectItemIndex' not in source
assert 'DEFINE_HOOK(&dSv_player_status_a_c::setMixItemIndex' not in source
assert 'DEFINE_HOOK_SYMBOL("dBgS_LinChk::Set"' not in source
assert 'DEFINE_HOOK(&dComIfGp_setSelectItem, SurvivorsBind)' in source
assert 'DEFINE_HOOK(&cBgS::LineCross, SurvivorsEnemyNavigationLine)' in source
assert ('SurvivorsPlayer','before_player_bindings') in hooks
assert ('SurvivorsPalacePlatformSwitch','before_palace_platform_switch') in hooks
assert ('SurvivorsEliteMinimap','before_survivors_minimap') in hooks
assert ('SurvivorsEliteMinimap','after_elite_minimap') in hooks
assert ('SurvivorsMinimapCursor','before_survivors_minimap_cursor') in hooks
assert ('SurvivorsSaveSlotInfo','after_survivors_save_slot') in hooks
assert ('SurvivorsSaveSlotDraw','after_survivors_save_slot_draw') in hooks
assert 'DEFINE_HOOK_SYMBOL("renderingPlusDoorAndCursor_c::drawCursor"' in source
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test';cpp.write_text(fixture+(root/'src/survivors_mode.inc').read_text()+body+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors hooks: stable Windows targets, every registration failure, initializer errors and mode registration passed')
