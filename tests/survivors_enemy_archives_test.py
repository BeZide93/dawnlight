"""Run-owned native resource retention across all maps, descendants and teardown."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
core = (root / 'src/survivors.cpp').read_text()
production = (root / 'src/survivors_enemy_archives.inc').read_text()
spawner = (root / 'src/enemy_spawner.cpp').read_text()
start = spawner.index('bool enemy_spawner_process_active() {')
scope = spawner[start:spawner.index('\n}', start)+2]

fixture = r'''
#include <any>
#include <array>
#include <cassert>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <vector>
#include "survivors_roster.hpp"
#define DEFINE_HOOK(...)
using u8=unsigned char;
struct ModContext {};
struct JKRHeap {};
struct JKRArchive {unsigned serial;};
namespace mods {template<class T>T arg(void* a,int n){return std::any_cast<T>(static_cast<std::any*>(a)[n]);}}
bool s_active=true,s_gameStarted=true,s_hub=false,s_restart=false,arena=true,transition=false;
unsigned s_map=0;
bool in_arena(){return arena;}bool transitioning(){return transition;}
std::vector<std::pair<void*,bool>> s_testProcessStack;
struct dRes_info_c {
    std::string name;
    unsigned count=0;
    JKRArchive* archive=nullptr;
    JKRHeap* heap=nullptr;
    void* pending=nullptr;
    unsigned getCount(){return count;}
    JKRArchive* getArchive(){return archive;}
    void* getDMCommand(){return pending;}
    const char* getArchiveName(){return name.c_str();}
};
std::map<std::string,dRes_info_c> resources;
std::deque<JKRArchive> archives;
unsigned loads=0,pins=0,unloads=0;
bool failPin=false;
std::string canonical(const char* name){std::string s=name;for(auto& c:s)if(c>='a'&&c<='z')c-=32;return s;}
dRes_info_c* dComIfG_getObjectResInfo(const char* name){
    auto it=resources.find(canonical(name));return it==resources.end()?nullptr:&it->second;
}
int dComIfG_setObjectRes(const char* name,u8,JKRHeap* heap){
    assert(!heap);auto* info=dComIfG_getObjectResInfo(name);
    // Retention must only add a reference to a completed native load; never load.
    assert(info&&info->archive&&!info->pending);
    if(failPin)return 0;
    ++info->count;++pins;return 1;
}
int dComIfG_deleteObjectResMain(const char* name){
    auto* info=dComIfG_getObjectResInfo(name);assert(info&&info->count);
    if(--info->count==0){resources.erase(canonical(name));++unloads;}return 1;
}
void native_load(const char* name){
    auto key=canonical(name);auto* info=dComIfG_getObjectResInfo(name);
    if(!info){archives.push_back({++loads});resources[key]={key,0,&archives.back(),nullptr,nullptr};info=&resources[key];}
    ++info->count;
}
'''

cases = r'''
void notify_info(dRes_info_c* info,int status=0){
    std::any args[]={info};
    const auto expected=status;
    after_enemy_archive_ready(nullptr,args,&status,nullptr);
    assert(status==expected); // Native status is never changed.
}
void notify(const char* name,int status=0){notify_info(dComIfG_getObjectResInfo(name),status);}
void fresh(unsigned map=0){
    release_enemy_archives();resources.clear();archives.clear();
    s_active=s_gameStarted=arena=true;s_hub=s_restart=transition=false;
    s_map=map;loads=pins=unloads=0;failPin=false;s_testProcessStack={{nullptr,true}};
    s_enemyArchivesAvailable=true;begin_enemy_archives();
}
int main(){
    // Representative native primary, secondary and child archives per map.
    // Names are test inputs only: production has no hard-coded name/map whitelist.
    const std::array<std::vector<const char*>,5> maps{{
        {"E_oc","E_ocb","E_ba","E_MS","E_dn","E_MM","E_MM_MT","E_mf","B_tn","B_tnp"},
        {"E_oc","E_ocb","E_RD","E_TTR","E_TT","E_GE","E_kr","E_rdb"},
        {"E_BS","E_RD","E_sh","E_dd","E_GI","E_sf","E_VA","B_DS"},
        {"E_rdy","E_YK","E_S2","E_yc","E_YB","E_PZ"},
        {"E_oc","E_RD","E_MS","E_dn","E_MM","E_MM_MT","B_tn","B_tnp2","E_th","Hzelda","B_gnd","B_hg","E_fk"}
    }};
    static_assert(maps.size()==dawnlight::survivors::map_rosters.size());
    for(unsigned map=0;map<maps.size();++map){
        fresh(map);
        for(auto* name:maps[map]){
            native_load(name);notify(name);notify(name);
            assert(dComIfG_getObjectResInfo(name)->count==2);
            dComIfG_deleteObjectResMain(name); // Last actor dies; cache retains one.
            const auto count=loads;
            for(unsigned spawn=0;spawn<20;++spawn){
                native_load(name);notify(name);dComIfG_deleteObjectResMain(name);
                assert(loads==count&&dComIfG_getObjectResInfo(name)->count==1);
            }
        }
        assert(pins==maps[map].size()&&loads==pins&&!unloads);
        release_enemy_archives();assert(resources.empty()&&unloads==loads);
        release_enemy_archives();assert(unloads==loads); // Idempotent reset/shutdown.
    }
    fresh();native_load("E_ba");
    notify("E_ba",1);notify("E_ba",-1);notify_info(nullptr);
    assert(!pins);notify("E_ba");assert(pins==1);
    notify("e_BA");assert(pins==1); // Engine-canonical case-insensitive identity.
    // Releasing our hold while the actor lives must leave its resources valid.
    release_enemy_archives();assert(dComIfG_getObjectResInfo("E_ba")->count==1&&!unloads);
    notify("E_ba");assert(pins==1);
    dComIfG_deleteObjectResMain("E_ba");assert(unloads==1);
    // Unrelated nested/native actors cannot inherit the outer spawner ownership.
    fresh();native_load("E_MS");s_testProcessStack.push_back({nullptr,false});
    notify("E_MS");assert(!pins);s_testProcessStack.pop_back();
    notify("E_MS");assert(pins==1);
    // Each lifecycle exit drains the cache and disallows late async completions.
    for(unsigned reason=0;reason<8;++reason){
        fresh();native_load("E_RD");notify("E_RD");dComIfG_deleteObjectResMain("E_RD");
        if(reason==0)s_active=false;if(reason==1)s_gameStarted=false;
        if(reason==2)s_hub=true;if(reason==3)s_restart=true;
        if(reason==4)s_map=1;if(reason==5)arena=false;if(reason==6)transition=true;
        if(reason==7)release_enemy_archives();else update_enemy_archives();
        assert(resources.empty()&&!s_enemyArchivesEnabled);
        native_load("E_RD");notify("E_RD");assert(pins==1);
    }
    // Restart/Endless can open a new cache; old map archives never accumulate.
    fresh(2);native_load("B_DS");notify("B_DS");dComIfG_deleteObjectResMain("B_DS");
    s_map=4;begin_enemy_archives();assert(resources.empty()&&s_enemyArchivesEnabled);
    native_load("B_gnd");notify("B_gnd");assert(dComIfG_getObjectResInfo("B_gnd")->count==2);
    // Custom heaps cannot outlive their actor, even via a later default load.
    fresh();native_load("E_th");JKRHeap custom;
    resources["E_TH"].heap=&custom;notify("E_th");assert(!pins);resources["E_TH"].heap=nullptr;
    resources["E_TH"].pending=&custom;notify("E_th");assert(!pins);resources["E_TH"].pending=nullptr;
    failPin=true;notify("E_th");assert(!pins&&resources["E_TH"].count==1);
    failPin=false;notify("E_th");assert(pins==1);
    // A host-forced teardown must not release a newly mounted same-name archive.
    resources.clear();native_load("E_th");release_enemy_archives();assert(resources["E_TH"].count==1);
    fresh();native_load("E_th");resources["E_TH"].count=0xffff;notify("E_th");assert(!pins);
    // Same-name stage and standalone records must never retain the object entry.
    fresh();native_load("E_ba");
    auto stage=resources["E_BA"];notify_info(&stage);assert(!pins);
    resources["E_BA"].count=0;notify("E_ba");assert(!pins);resources["E_BA"].count=1;
    // A platform that cannot install this optimization still starts normal runs.
    s_enemyArchivesAvailable=false;begin_enemy_archives();notify("E_ba");
    assert(!s_enemyArchivesEnabled&&!pins&&resources["E_BA"].count==1);
    dComIfG_deleteObjectResMain("E_ba");assert(resources.empty());
    // Bounded cache overflow falls back to native refcounts without leaking.
    fresh();
    for(unsigned i=0;i<s_enemyArchives.size()+2;++i){
        const auto name="R"+std::to_string(i);native_load(name.c_str());notify(name.c_str());
        dComIfG_deleteObjectResMain(name.c_str());
    }
    assert(pins==s_enemyArchives.size()&&resources.size()==pins&&unloads==2);
    release_enemy_archives();assert(resources.empty()&&unloads==loads);
}
'''

# Verify that the real lifecycle, including Endless, wires the tested cache in.
assert '#include "survivors_enemy_archives.inc"' in core
assert 'void retire_enemies() {\n    release_enemy_archives();' in core
assert 'if(!s_run.continue_endless())return;\n    begin_enemy_archives();' in core
begin=core[core.index('void begin_run() {'):core.index('void start_run(')]
assert 'begin_enemy_archives();' in begin
assert 'hook_add_post<SurvivorsArchiveReady>(svc_hook,after_enemy_archive_ready)' in core
assert '&dComIfG_resLoad' not in production
assert 'static_cast<int (dRes_info_c::*)()>(&dRes_info_c::setRes)' in production
update=core[core.index('void update_survivors() {'):core.index('ModResult initialize_survivors(')]
assert update.index('update_enemy_archives();')<update.index('if (!s_active || !s_gameStarted) return;')

with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';binary=Path(tmp)/'test'
    cpp.write_text(fixture+scope+production+cases)
    for diagnostics in (0,1):
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',f'-DDAWNLIGHT_FREEZE_DIAGNOSTICS={diagnostics}','-include',str(root/'tests/survivors_freeze_fixture.hpp'),'-Wall','-Wextra','-Werror',
                        '-Wno-misleading-indentation','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

print('Survivors archive retention: five maps, variants/children, single load across respawns, scoped ownership and balanced teardown passed')
