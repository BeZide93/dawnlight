"""Shared configuration persists all next-run options without mutating a live run."""
from pathlib import Path
import subprocess,tempfile,os
root=Path(__file__).resolve().parents[1]
ui=(root/'src/survivors_maps_ui.inc').read_text()
controls=ui[ui.index('struct RunOptionControl'):ui.index('ModResult build_run_controls')]
fixture=r'''
#include <cassert>
#include <map>
#include <string>
#include "survivors_rules.hpp"
#include "mods/svc/config.h"
#define svc_config test_config
using namespace dawnlight::survivors;
ModContext* mod_ctx=nullptr;
RunOptions s_options;Run s_run;
bool s_active=false,s_restart=false,s_progressSaving=false,s_hubDeparture=false;
std::map<ConfigVarHandle,int64_t> persisted;std::map<std::string,ConfigVarHandle> configNames;
ConfigService service{};auto* test_config=&service;
ModResult register_owned_setting(const ConfigVarDesc* desc,ConfigVarHandle* out){
 assert(desc->type==CONFIG_VAR_INT);*out=configNames.size()+1;configNames[desc->name]=*out;
 persisted.try_emplace(*out,desc->default_int);return MOD_OK;
}
struct Log{void warn(ModContext*,const char*){assert(false);}}logService;auto* svc_log=&logService;
'''
main=r'''
int main(){
 service.get_int=[](ModContext*,ConfigVarHandle h,int64_t* out){*out=persisted.at(h);return MOD_OK;};
 service.set_int=[](ModContext*,ConfigVarHandle h,int64_t value){persisted[h]=value;return MOD_OK;};
 assert(!debug_session_enabled);
 assert(initialize_option_config()==MOD_OK&&configNames.size()==12);
 debug_session_enabled=true;store_option_config();
 debug_session_enabled=false;load_option_config();assert(!debug_session_enabled);
 for(const auto& [name,handle]:configNames){(void)handle;assert(name.find("debug")==std::string::npos);}
 assert(s_options.boss_hp==1);
 assert(s_options.camera_distance==200&&s_options.gyro_aim&&!s_options.switch_attack_lock);
 for(bool enabled:s_options.manual_aim)assert(enabled);
 assert(!s_options.enemy_collision);s_options.enemy_collision=true;
 assert(!s_options.allow_cheats);s_options.allow_cheats=true;
 assert(settings_editable()); // Mirrored tab is editable in ordinary Dawnlight saves.
 s_options.camera_distance=375;s_options.reticle_y=-17;s_options.switch_attack_lock=true;s_options.gyro_aim=false;
 s_options.enemy_hp=7;s_options.boss_hp=6;s_options.enemy_damage=8;s_options.enemy_speed=3;s_options.horde=4;
 for(unsigned i=0;i<s_options.manual_aim.size();++i)s_options.manual_aim[i]=i%2;
 store_option_config();s_run.start(123,s_options);
 s_options={};load_option_config();
 assert(s_options.allow_cheats&&s_run.options.allow_cheats);
 assert(s_options.enemy_collision&&s_run.options.enemy_collision);
 persisted[configNames.at("survivors-enemy-collision")]=0;load_option_config();
 assert(!s_options.enemy_collision&&s_run.options.enemy_collision);
 persisted[configNames.at("survivors-allow-cheats")]=0;load_option_config();
 assert(!s_options.allow_cheats&&s_run.options.allow_cheats);
 assert(s_options.camera_distance==375&&s_options.reticle_y==-17&&s_options.switch_attack_lock&&!s_options.gyro_aim);
 assert(s_options.boss_hp==6&&s_run.options.boss_hp==6);
 assert(s_options.enemy_hp==7&&s_options.enemy_damage==8&&s_options.enemy_speed==3&&s_options.horde==4);
 for(unsigned i=0;i<s_options.manual_aim.size();++i)assert(s_options.manual_aim[i]==bool(i%2));
 // Host imports and malformed config values use the same bounded readers.
 persisted[configNames.at("survivors-camera-distance")]=9999;
 persisted[configNames.at("survivors-reticle-y")]=-900;
 persisted[configNames.at("survivors-enemy-speed")]=-9;
 persisted[configNames.at("survivors-boss-hp")]=100;
 load_option_config();assert(s_options.boss_hp==10&&s_run.options.boss_hp==6);
 persisted[configNames.at("survivors-boss-hp")]=-2;
 load_option_config();assert(s_options.boss_hp==1&&s_run.options.boss_hp==6);
 assert(s_options.camera_distance==500&&s_options.reticle_y==-40&&s_options.enemy_speed==1);
 assert(s_run.options.camera_distance==375&&s_run.options.reticle_y==-17&&s_run.options.enemy_speed==3);
 s_active=true;assert(!settings_editable());s_run.phase=Phase::Ready;assert(settings_editable());
 s_hubDeparture=true;assert(!settings_editable());s_hubDeparture=false;
 s_progressSaving=true;assert(!settings_editable());s_progressSaving=false;
 s_restart=true;assert(!settings_editable());
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+controls+(root/'src/survivors_settings_config.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-I',str(root/'src'),'-I',str(root/'dusklight/sdk/include'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors shared settings: persistence, defaults, all ability flags, import clamping and live-run isolation passed')
