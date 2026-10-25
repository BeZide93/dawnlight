#include "survivors_roundtable_props.hpp"
#include "roundtable_prop_source.hpp"
#include "generated/roundtable_props.hpp"
#include "service_imports.hpp"
#include "process_actor.hpp"
#include "d/d_com_inf_game.h"
#include "d/actor/d_a_obj_carry.h"
#include "f_pc/f_pc_name.h"
#include "m_Do/m_Do_ext.h"
#include "JSystem/JKernel/JKRArchive.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include <cmath>
#include <map>
#include <vector>

namespace dawnlight {
namespace {
struct Dressing { unsigned next=0; };
std::map<ActorId,Dressing> rooms;
std::map<ActorId,ActorId> owners;
auto types=roundtable_props::default_types;
bool sourceRead=false;

void read_source() {
    if(sourceRead)return;
    sourceRead=true;
    types=roundtable_props::default_types;
    bool selected=false;
    // Mount a private archive only to inspect its actor records. Never install
    // room 8 as a stage resource or run its actors, events or collision.
    auto* archive=JKRArchive::mount("/res/Stage/D_MN09/R08_00.arc",
        JKRArchive::MOUNT_DVD,mDoExt_getArchiveHeap(),JKRArchive::MOUNT_DIRECTION_HEAD);
    if(archive) {
        const auto* bytes=static_cast<const u8*>(archive->getResource(0,"room.dzr"));
        const auto size=bytes?archive->getExpandedResSize(bytes):0;
        selected=size<=1024*1024 && roundtable_props::select_room_models(bytes,size,types);
        archive->unmount();
    }
    if(!selected)svc_log->warn(mod_ctx,"Roundtable dressing: optional room 8 variants unavailable; using native Carry models");
}

}
void update_roundtable_props(ActorId owner) {
    read_source();
    auto& room=rooms[owner];
    // Native Carry owns size, physics, pickup, break effects and projected shadow.
    // No custom model, static collision proxy, save bit or built-in item drop.
    for(unsigned count=0;count<3 && room.next<std::size(roundtable_layout::props);++count) {
        const auto& p=roundtable_layout::props[room.next];
        ActorSpawnParams params{};params.parameters=0x003fffff;params.argument=-1;params.room_num=51;
        params.position={p.x,p.y,p.z};params.scale={1,1,1};
        params.angle={s16(0xffff),static_cast<s16>(std::lround(p.yaw*(32768.f/3.141592654f))),
                      static_cast<s16>(0x1040 | (types[p.kind]<<1))};
        ActorId id=fpcM_ERROR_PROCESS_ID_e;
        if(svc_actor->create_child_actor(mod_ctx,fpcNm_Obj_Carry_e,owner,&params,&id)!=MOD_OK ||
           id==fpcM_ERROR_PROCESS_ID_e)break; // Retry a rejected request next update.
        owners.emplace(id,owner);++room.next;
    }
}
HookAction before_roundtable_prop_process(void* args,void* result) {
    auto* process=mods::arg<void*>(args,1);
    if(!is_actor_process(process))return HOOK_CONTINUE;
    auto* actor=static_cast<fopAc_ac_c*>(process);
    if(fopAcM_GetName(actor)!=fpcNm_Obj_Carry_e)return HOOK_CONTINUE;
    auto it=owners.find(fopAcM_GetID(actor));if(it==owners.end())return HOOK_CONTINUE;
    const auto* methods=reinterpret_cast<const leafdraw_method_class*>(actor->sub_method);
    if(!methods)return HOOK_CONTINUE;
    const auto method=mods::arg<process_method_func>(args,0);
    if(method==methods->base.delete_method) {
        fopAcM_cancelCarryNow(actor);fopAcM_cancelHookCarryNow(actor);
        owners.erase(it);return HOOK_CONTINUE; // Native teardown releases models/contacts.
    }
    if(rooms.count(it->second))return HOOK_CONTINUE;
    // A pending create cannot outlive its room. Already live props stop drawing
    // and retry deletion on Execute if the host initially deferred the request.
    if(method==methods->base.create_method) {
        // Construct the native instance/variant before a possible Delete, even
        // when its room disappeared before the very first create callback.
        static_cast<daObjCarry_c*>(actor)->preInit();
        *static_cast<int*>(result)=cPhs_ERROR_e;return HOOK_SKIP_ORIGINAL;
    }
    if(method==methods->base.execute_method) {
        fopAcM_cancelCarryNow(actor);fopAcM_cancelHookCarryNow(actor);
        svc_actor->delete_actor(mod_ctx,it->first);
    } else if(method!=methods->draw_method)return HOOK_CONTINUE;
    *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
}
void release_roundtable_props(ActorId owner) {
    rooms.erase(owner);
    std::vector<ActorId> ids;
    for(const auto& [id,parent]:owners)if(parent==owner)ids.push_back(id);
    for(auto id:ids)svc_actor->delete_actor(mod_ctx,id);
}
void shutdown_roundtable_props() {
    while(!rooms.empty())release_roundtable_props(rooms.begin()->first);
    // These are native profiles: remaining deferred teardown is engine-owned.
    owners.clear();sourceRead=false;types=roundtable_props::default_types;
}
}
