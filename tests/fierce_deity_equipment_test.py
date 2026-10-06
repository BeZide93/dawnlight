"""Exercise actual model-ownership capture through the player draw dispatcher."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/fierce_deity_equipment.inc').read_text()
fixture = r'''
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
using fpc_ProcID=unsigned;
constexpr fpc_ProcID fpcM_ERROR_PROCESS_ID_e=~0u;
struct ModContext {};
enum HookAction {HOOK_CONTINUE};
using process_method_func=int(*)(void*);
int draw(void*) {return 1;}
int execute(void*) {return 1;}
struct leafdraw_method_class {process_method_func draw_method=draw;};
struct J3DModelData {};
struct J3DModel {J3DModelData* data=nullptr;J3DModelData* getModelData() {return data;}};
struct daAlink_c {
    void* sub_method=nullptr;
    J3DModel* body=nullptr;
    unsigned id=7;
    bool wolf=false,noDraw=false;
    bool checkWolf() {return wolf;}
    bool checkPlayerNoDraw() {return noDraw;}
};
daAlink_c* current=nullptr;
daAlink_c* daAlink_getAlinkActorClass() {return current;}
unsigned fopAcM_GetID(daAlink_c* p) {return p->id;}
bool player_model(daAlink_c* link,const J3DModel* model) {return model==link->body;}
namespace mods {template<class T>T arg(void* args,int i) {return reinterpret_cast<T>(static_cast<void**>(args)[i]);}}
'''
checks = r'''
int main() {
    leafdraw_method_class methods,otherMethods;
    daAlink_c link{&methods},other{&otherMethods};current=&link;
    J3DModelData data,replacement;
    J3DModel body{&data},shield{&data},bow{&data},quiver{&data},lantern{&data},enemy{&data};
    link.body=&body;
#ifdef __APPLE__
    void* actorArgs[]={reinterpret_cast<void*>(draw),&link};
    void* enemyArgs[]={reinterpret_cast<void*>(draw),&other};
    void* executeArgs[]={reinterpret_cast<void*>(execute),&link};
#else
    void* actorArgs[]={&methods,&link};
    void* enemyArgs[]={&otherMethods,&other};
    void* executeArgs[]={&otherMethods,&link};
#endif
    auto submit=[&](J3DModel* model) {void* args[]={model};observe_equipment_model(nullptr,args,nullptr,nullptr);};
    submit(&shield);assert(!observed_player_equipment(&link,&shield));
    before_player_draw(nullptr,actorArgs,nullptr,nullptr);
    // Both mDoExt_modelUpdateDL and modelEntryDL use this callback before
    // entering native code, including TE post-draw hooks/interpolated frames.
    for(auto* model : {&body,&shield,&bow,&quiver,&lantern}) submit(model);
    submit(&shield); // duplicate helper calls consume no extra slots
    assert(s_observedEquipment[4].model==nullptr);
    assert(!observed_player_equipment(&link,&body));
    // A nested foreign actor suspends capture, even with identical model data.
    before_player_draw(nullptr,enemyArgs,nullptr,nullptr);
    submit(&enemy);assert(!observed_player_equipment(&link,&enemy));
    end_equipment_draw();submit(&quiver);
    end_equipment_draw(); // later deferred material/shape pass retains ownership
    for(auto* model : {&shield,&bow,&quiver,&lantern}) assert(observed_player_equipment(&link,model));
    assert(!observed_player_equipment(&other,&shield));
    assert(!observed_player_equipment(&link,&enemy));
    // Submission outside the Link draw cannot acquire ownership.
    submit(&enemy);assert(!observed_player_equipment(&link,&enemy));
    // Live instance changed data, or player ID changed at the same address.
    shield.data=&replacement;assert(!observed_player_equipment(&link,&shield));
    shield.data=&data;++link.id;assert(!observed_player_equipment(&link,&shield));--link.id;
    link.wolf=true;assert(!observed_player_equipment(&link,&shield));link.wolf=false;
    // A subsequent draw rebuilds the set: unequipped/released TE models expire.
    before_player_draw(nullptr,actorArgs,nullptr,nullptr);submit(&bow);end_equipment_draw();
    assert(observed_player_equipment(&link,&bow));assert(!observed_player_equipment(&link,&shield));
    // Main-menu/null player, skipped/no-draw player and the Apple generic
    // dispatcher's non-draw methods must not collect models.
    before_player_draw(nullptr,executeArgs,nullptr,nullptr);submit(&enemy);end_equipment_draw();
    assert(!observed_player_equipment(&link,&enemy));
    link.noDraw=true;before_player_draw(nullptr,actorArgs,nullptr,nullptr);submit(&shield);end_equipment_draw();
    assert(!observed_player_equipment(&link,&shield));link.noDraw=false;
    current=nullptr;before_player_draw(nullptr,actorArgs,nullptr,nullptr);submit(&shield);end_equipment_draw();current=&link;
    assert(!observed_player_equipment(&link,&shield));
    // Fixed capacities fail closed and unwind without stale scopes.
    std::array<J3DModel,33> models{};
    before_player_draw(nullptr,actorArgs,nullptr,nullptr);
    for(auto& model:models) {model.data=&data;submit(&model);}
    assert(observed_player_equipment(&link,&models[31]));
    assert(!observed_player_equipment(&link,&models[32]));
    for(int i=0;i<20;++i) before_player_draw(nullptr,enemyArgs,nullptr,nullptr);
    submit(&enemy);
    for(int i=0;i<20;++i) end_equipment_draw();
    assert(s_equipmentDrawDepth==1);end_equipment_draw();end_equipment_draw();
    assert(s_equipmentDrawDepth==0);
    for(auto* owner:s_equipmentDrawScopes) assert(owner==nullptr);
    reset_observed_equipment();assert(!observed_player_equipment(&link,&models[0]));
}
'''
with tempfile.TemporaryDirectory(prefix='dawnlight-equipment-') as temp:
    cpp, exe = Path(temp)/'test.cpp', Path(temp)/'test'
    cpp.write_text(fixture + source + checks)
    for platform in ([], ['-D__APPLE__']):
        subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', *platform,
                        str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
print('Dark Link equipment capture: TE attachments, deferred draw, actor isolation, lifetimes and Apple dispatch passed')
