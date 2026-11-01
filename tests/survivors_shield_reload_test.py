"""Run the gameplay shield handoff with native reload and resource-phase code."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'dusklight'


def function(text, signature):
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 0
    for end in range(opening, len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if not depth:
            return text[start:end + 1] + '\n'
    raise AssertionError(signature)


fixture = r'''
#include <cassert>
#include <cstring>
#include <deque>
#include <map>
#include <string>
using u8=unsigned char;
using ActorId=int;
constexpr int kNone=-1;
constexpr u8 dItemNo_NONE_e=255,dItemNo_WOOD_SHIELD_e=42,dItemNo_SHIELD_e=43,dItemNo_HYLIA_SHIELD_e=44;
struct ModContext{};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* args,int){return static_cast<T>(args);}}
using cPhs__Handler=int(*)(void*);
enum {cPhs_INIT_e,cPhs_LOADING_e,cPhs_NEXT_e,cPhs_UNK3_e,cPhs_COMPLEATE_e,cPhs_ERROR_e};
struct request_of_phase_process_class {cPhs__Handler* mpHandlerTable=nullptr;int id=0;};
#define JUT_ASSERT(line,condition) assert(condition)
// NATIVE_PHASE

struct JKRHeap {unsigned frees=0;void freeAll(){++frees;}} heap;
struct Resource {int refs=0;int state=0;bool model=true;};
std::map<std::string,Resource> resources;
bool failAcquire=false;
unsigned acquisitions=0,releases=0,models=0;
int dComIfG_setObjectRes(const char* name,u8,JKRHeap*) {
    if(failAcquire)return 0;
    ++acquisitions;++resources[name].refs;return 1;
}
int dComIfG_syncObjectRes(const char* name){assert(resources.at(name).refs>0);return resources.at(name).state;}
int dComIfG_deleteObjectResMain(const char* name){assert(resources.at(name).refs>0);--resources.at(name).refs;++releases;return 1;}
void* dComIfG_getObjectRes(const char* name,int index){
    auto& resource=resources.at(name);assert(index==3&&resource.refs>0&&resource.state==0);
    return resource.model?&resource:nullptr;
}
struct phaseParam_c {const char* mResName;JKRHeap* mpHeap;
    phaseParam_c(const char* name,JKRHeap* h):mResName(name),mpHeap(h) {}};
// NATIVE_RESOURCE_LOAD

using MtxP=float*;
struct Model {std::string archive;bool alive=true;float matrix[12]{};
    MtxP getBaseTRMtx(){assert(alive&&resources.at(archive).refs>0);return matrix;}
};
std::deque<Model> modelStore;
u8 equipped=dItemNo_NONE_e;
const char* desiredArchive(){return equipped==dItemNo_HYLIA_SHIELD_e?"HyShd":equipped==dItemNo_SHIELD_e?"SWShd":"CWShd";}
struct daAlink_c {
    ActorId id=1;bool statusWindow=false;
    int mShieldChangeWaitTimer=0;
    const char* mShieldArcName="CWShd";
    request_of_phase_process_class mShieldPhaseReq{};
    JKRHeap* mpShieldArcHeap=&heap;Model* mShieldModel=nullptr;
    bool checkStatusWindowDraw(){return statusWindow;}
    int getShieldChangeWaitTimer(){return mShieldChangeWaitTimer;}
    void setShieldChange();int loadShieldModelDVD();
    void setShieldArcName(){mShieldArcName=desiredArchive();}
    void setShieldModel(){
        assert(dComIfG_getObjectRes(mShieldArcName,3));
        for(auto& model:modelStore)model.alive=false; // Native model heap freeAll.
        modelStore.push_back({mShieldArcName});mShieldModel=&modelStore.back();++models;
    }
    // NATIVE_SHIELD_MATRIX
} player;
// NATIVE_SHIELD_RELOAD
bool arena=true,transition=false,present=true;
bool in_arena(){return arena;}bool transitioning(){return transition;}
daAlink_c* daAlink_getAlinkActorClass(){return present?&player:nullptr;}
ActorId fopAcM_GetID(daAlink_c* p){return p->id;}
u8 dComIfGs_getSelectEquipShield(){return equipped;}
'''
main = r'''
void fresh(const char* old="CWShd") {
    clear_shield_reload();resources.clear();modelStore.clear();player={};heap={};
    arena=present=true;transition=failAcquire=false;acquisitions=releases=models=0;
    equipped=std::strcmp(old,"HyShd")==0?dItemNo_HYLIA_SHIELD_e:
        std::strcmp(old,"SWShd")==0?dItemNo_SHIELD_e:dItemNo_NONE_e;
    player.mShieldArcName=old;player.mShieldPhaseReq.id=2;
    resources[old].refs=1;player.setShieldModel();models=0;
}
void request(u8 item){equipped=item;request_shield_reload(&player,item);}
void tick() {
    assert(before_survivors_shield_update(nullptr,&player,nullptr,nullptr)==HOOK_CONTINUE);
    // setCollision() uses getShieldMtx() unconditionally. Read it immediately
    // after the pre-hook, and again as a later draw/foreign callback would.
    assert(player.mShieldModel&&player.getShieldMtx());
    player.mShieldModel->getBaseTRMtx();
    assert(player.mShieldChangeWaitTimer==0);
}
int main() {
    // Demonstrate the old failure using the actual native reload, without
    // dereferencing the null model and terminating the test process.
    fresh();equipped=dItemNo_HYLIA_SHIELD_e;player.setShieldChange();
    player.loadShieldModelDVD();player.loadShieldModelDVD();assert(!player.mShieldModel);

    // Every tier transition, direct multi-tier upgrades, and no-shield acquisition.
    for(const char* old:{"CWShd","SWShd","HyShd"})for(u8 item:
        {dItemNo_WOOD_SHIELD_e,dItemNo_SHIELD_e,dItemNo_HYLIA_SHIELD_e}) {
        fresh(old);auto* before=player.mShieldModel;request(item);
        const std::string target=desiredArchive();
        if(target!=old)resources[target].state=1;
        for(int i=0;i<8;++i){tick();assert(player.mShieldModel==before&&models==0);}
        if(target==old){assert(!acquisitions&&s_shieldReload.player==kNone);continue;}
        assert(acquisitions==1&&resources[old].refs==1&&resources[target].refs==1);
        resources[target].state=0;tick();
        assert(models==1&&s_shieldReload.player==kNone&&player.mShieldModel!=before);
        assert(std::strcmp(player.mShieldArcName,target.c_str())==0);
        assert(resources[old].refs==0&&resources[target].refs==1&&heap.frees==1);
        tick();assert(models==1&&acquisitions==2&&releases==2);
        // Native actor teardown owns/releases the remaining reference.
        dComIfG_resDelete(&player.mShieldPhaseReq,player.mShieldArcName);
        assert(resources[target].refs==0);
    }
    // Load failure or missing model retains the valid old shield and native timer.
    for(int failure=0;failure<3;++failure) {
        fresh();auto* before=player.mShieldModel;request(dItemNo_HYLIA_SHIELD_e);
        if(failure==0)failAcquire=true;
        if(failure==1)resources["HyShd"].state=-1;
        if(failure==2)resources["HyShd"].model=false;
        tick();assert(player.mShieldModel==before&&heap.frees==0&&models==0);
        assert(s_shieldReload.player==kNone&&resources["HyShd"].refs==0);
    }
    // Cancellation while I/O is pending: reset, scene, mode, player lifetime,
    // selection changes and replacement requests each release exactly one hold.
    for(int reason=0;reason<7;++reason) {
        fresh();request(dItemNo_HYLIA_SHIELD_e);resources["HyShd"].state=1;tick();
        if(reason==0)clear_shield_reload();
        if(reason==1)transition=true;
        if(reason==2)arena=false;
        if(reason==3)present=false;
        if(reason==4)++player.id;
        if(reason==5)equipped=dItemNo_NONE_e;
        if(reason==6)request(dItemNo_SHIELD_e);
        validate_shield_reload();
        assert(resources["HyShd"].refs==0&&resources["CWShd"].refs==1);
        assert(!models&&!heap.frees&&player.getShieldMtx());clear_shield_reload();
    }
    // Collection and other actors retain ownership of their native reloads.
    fresh();request(dItemNo_HYLIA_SHIELD_e);player.statusWindow=true;
    before_survivors_shield_update(nullptr,&player,nullptr,nullptr);assert(!acquisitions);
    player.statusWindow=false;player.mShieldChangeWaitTimer=4;
    before_survivors_shield_update(nullptr,&player,nullptr,nullptr);
    assert(player.mShieldChangeWaitTimer==4&&!acquisitions);
    player.mShieldChangeWaitTimer=0;auto other=player;
    before_survivors_shield_update(nullptr,&other,nullptr,nullptr);
    before_survivors_shield_update(nullptr,nullptr,nullptr,nullptr);assert(!acquisitions);
    clear_shield_reload();player.setShieldChange();
    before_survivors_shield_update(nullptr,&player,nullptr,nullptr);
    assert(player.mShieldChangeWaitTimer==4); // Unrelated native requests are not pumped.
    // Shutdown/cancel doesn't release references owned by another consumer.
    fresh();resources["HyShd"].refs=1;resources["HyShd"].state=1;
    request(dItemNo_HYLIA_SHIELD_e);tick();assert(resources["HyShd"].refs==2);
    clear_shield_reload();clear_shield_reload();assert(resources["HyShd"].refs==1);
}
'''
phase = (sdk / 'src/SSystem/SComponent/c_phase.cpp').read_text()
fixture = fixture.replace('// NATIVE_PHASE', phase[phase.index('void cPhs_Reset'):])
resource = (sdk / 'src/d/d_com_inf_game.cpp').read_text()
fixture = fixture.replace('// NATIVE_RESOURCE_LOAD', '\n'.join(function(resource, signature) for signature in [
    'int dComLbG_PhaseHandler(', 'static int phase_01(', 'static int phase_02(',
    'static int phase_03(',
    'int dComIfG_resLoad(request_of_phase_process_class* i_phase, char const* i_resName,',
    'int dComIfG_resDelete(']))
native = (sdk / 'src/d/actor/d_a_alink_swindow.inc').read_text()
fixture = fixture.replace('// NATIVE_SHIELD_RELOAD', function(native, 'void daAlink_c::setShieldChange()') +
                          function(native, 'int daAlink_c::loadShieldModelDVD()'))
header = (sdk / 'include/d/actor/d_a_alink.h').read_text()
fixture = fixture.replace('// NATIVE_SHIELD_MATRIX', function(header, 'MtxP getShieldMtx() const'))
core = (root / 'src/survivors.cpp').read_text()
assert 'PRE(SurvivorsPlayer, before_survivors_shield_update)' in core
assert 'clear_shield_reload();' in function(core, 'void reset_runtime()')
assert 'validate_shield_reload();' in function(core, 'void update_survivors()')
source = fixture + (root / 'src/survivors_shield_reload.inc').read_text() + main
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    binary = Path(tmp) / 'test'
    cpp.write_text(source)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-variable', '-Wno-unused-but-set-variable', str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Survivors shield: native load phases, immediate matrix reads, delayed I/O, failures and reference cleanup passed')
