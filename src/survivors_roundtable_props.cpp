#include "survivors_roundtable_props.hpp"
#include "roundtable_prop_source.hpp"
#include "generated/roundtable_props.hpp"
#include "service_imports.hpp"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_actor_mng.h"
#include "m_Do/m_Do_ext.h"
#include "JSystem/JKernel/JKRArchive.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/J3DGraphBase/J3DShape.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <vector>

namespace dawnlight {
namespace {
ProfileName profile=-1;
ActorHandle registration=0;
struct Dressing { unsigned next=0; std::vector<ActorId> actors; };
std::map<ActorId,Dressing> rooms;
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

class PropActor : public fopAc_ac_c {
public:
    request_of_phase_process_class phase{};
    J3DModel* model=nullptr;
    const roundtable_props::ModelSource* source=nullptr;
    bool ready=false;
    ~PropActor(){if(source)dComIfG_resDelete(&phase,source->archive);}
    static int make_heap(fopAc_ac_c* actor) {
        auto* self=static_cast<PropActor*>(actor);
        auto* data=static_cast<J3DModelData*>(dComIfG_getObjectRes(self->source->archive,self->source->model));
        if(!data || data->getJointNum()!=1 || !data->getShapeNum())return 0;
        self->model=mDoExt_J3DModel__create(data,0x80000,0x11000084);
        return self->model!=nullptr;
    }
    bool place() {
        const auto& p=roundtable_layout::props[fopAcM_GetParam(this)];
        cXyz lo(1.e9f,1.e9f,1.e9f),hi(-1.e9f,-1.e9f,-1.e9f);
        auto* data=model->getModelData();
        for(unsigned i=0;i<data->getShapeNum();++i) {
            auto* shape=data->getShapeNodePointer(i);if(!shape)return false;
            const auto& a=*shape->getMin();const auto& b=*shape->getMax();
            if(!std::isfinite(a.x+a.y+a.z+b.x+b.y+b.z))return false;
            lo.x=std::min(lo.x,a.x);lo.y=std::min(lo.y,a.y);lo.z=std::min(lo.z,a.z);
            hi.x=std::max(hi.x,b.x);hi.y=std::max(hi.y,b.y);hi.z=std::max(hi.z,b.z);
        }
        if(hi.x-lo.x<.01f || hi.y-lo.y<.01f || hi.z-lo.z<.01f)return false;
        const float scale=std::min({p.width/(hi.x-lo.x),p.height/(hi.y-lo.y),p.depth/(hi.z-lo.z)});
        mDoMtx_stack_c::transS(p.x,p.y,p.z);
        mDoMtx_stack_c::YrotM(static_cast<s16>(std::lround(p.yaw*(32768.f/3.141592654f))));
        mDoMtx_stack_c::scaleM(scale,scale,scale);
        mDoMtx_stack_c::transM(-(hi.x+lo.x)*.5f,-lo.y,-(hi.z+lo.z)*.5f);
        model->setBaseTRMtx(mDoMtx_stack_c::get());model->setBaseScale(cXyz(1,1,1));
        model->calc();fopAcM_SetMtx(this,model->getBaseTRMtx());attention_info.flags=0;
        return true;
    }
};
int create_prop(void* actor) {
    auto* self=static_cast<PropActor*>(actor);fopAcM_ct(self,PropActor);
    if(!rooms.count(self->parentActorID) || fopAcM_GetParam(self)>=std::size(roundtable_layout::props))return cPhs_ERROR_e;
    if(self->ready)return cPhs_COMPLEATE_e;
    const auto kind=roundtable_layout::props[fopAcM_GetParam(self)].kind;
    self->source=&roundtable_props::sources[types[kind]];
    const auto phase=dComIfG_resLoad(&self->phase,self->source->archive);
    if(phase!=cPhs_COMPLEATE_e)return phase;
    if(!fopAcM_entrySolidHeap(self,PropActor::make_heap,0x10000) || !self->place())return cPhs_ERROR_e;
    self->ready=true;return cPhs_COMPLEATE_e;
}
int delete_prop(void* actor){static_cast<PropActor*>(actor)->~PropActor();return 1;}
int execute_prop(void* actor) {
    auto* self=static_cast<PropActor*>(actor);
    if(!rooms.count(self->parentActorID))svc_actor->delete_actor(mod_ctx,fopAcM_GetID(self));
    return 1;
}
int can_delete_prop(void*){return 1;}
int draw_prop(void* actor) {
    auto* self=static_cast<PropActor*>(actor);
    if(!self->ready || !rooms.count(self->parentActorID))return 1;
    g_env_light.settingTevStruct(8,&self->current.pos,&self->tevStr);
    g_env_light.setLightTevColorType_MAJI(self->model,&self->tevStr);
    mDoExt_modelUpdateDL(self->model);return 1;
}
}
ModResult initialize_roundtable_props(ModError* error) {
    if(registration)return MOD_OK;
    const ActorProfileDesc desc={.name="DLRProp",.priority_group=7,.process_size=sizeof(PropActor),
        .draw_priority=fpcDwPi_Obj_Carry_e,.status=fopAcStts_UNK_0x40000_e,.group=fopAc_ACTOR_e,
        .cull_type=fopAc_CULLBOX_CUSTOM_e,.create_function=create_prop,.delete_function=delete_prop,
        .execute_function=execute_prop,.is_delete_function=can_delete_prop,.draw_function=draw_prop};
    const auto result=svc_actor->register_actor(mod_ctx,&desc,&profile,&registration);
    return result==MOD_OK?MOD_OK:mods::set_error(error,result,"failed to register Roundtable dressing");
}
void update_roundtable_props(ActorId owner) {
    if(!registration)return;
    read_source();
    auto& room=rooms[owner];
    // Stagger archive/model creation without adding work to presentation frames.
    for(unsigned count=0;count<3 && room.next<std::size(roundtable_layout::props);++count) {
        const auto index=room.next++;const auto& p=roundtable_layout::props[index];
        ActorSpawnParams params{};params.parameters=index;params.argument=-1;params.room_num=51;
        params.position={p.x,p.y,p.z};params.scale={1,1,1};
        ActorId id=fpcM_ERROR_PROCESS_ID_e;
        if(svc_actor->create_child_actor(mod_ctx,profile,owner,&params,&id)==MOD_OK)room.actors.push_back(id);
    }
}
void release_roundtable_props(ActorId owner) {
    auto it=rooms.find(owner);if(it==rooms.end())return;
    auto ids=std::move(it->second.actors);rooms.erase(it);
    // Pending creates reject an absent owner; live actors retry deferred deletion.
    for(auto id:ids)svc_actor->delete_actor(mod_ctx,id);
}
void shutdown_roundtable_props() {
    while(!rooms.empty())release_roundtable_props(rooms.begin()->first);
    if(registration && svc_actor->unregister_actor(mod_ctx,registration)==MOD_OK){registration=0;profile=-1;}
    sourceRead=false;types=roundtable_props::default_types;
}
}
