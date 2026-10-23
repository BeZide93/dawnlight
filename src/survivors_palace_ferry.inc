// Own the Palace crossing explicitly. optLift waits for a switch EDGE even
// when its switch was already on at Create; L8LiftX has a native always-on mode.
// Both use room paths, but their path/speed bits occupy different positions.
struct PalaceFerry {
    fopAcM_prm_class source{};
    ActorId id=kNone;
    unsigned path=0,speed=0;
    bool east=false,selected=false,retiring=false,reported=false,ready=false,departed=false;
    bool returning=false;
};
std::vector<PalaceFerry> s_palaceFerries;
bool s_palaceFerryScanned=false;
unsigned s_palaceFerryRetry=0;
ActorId s_palaceFerryTurn=kNone;
unsigned s_palaceFerryDwell=0;
bool palace_ferry_profile(int profile) {
    return profile==fpcNm_Obj_Lv8Lift_e || profile==fpcNm_Obj_Lv8OptiLift_e;
}
bool capture_palace_ferry(int profile,const fopAcM_prm_class& params) {
    if(s_map!=3 || !in_arena() || !palace_ferry_profile(profile) ||
       (params.room_no!=arena().room && params.room_no!=-1))return false;
    const unsigned raw=params.base.parameters;
    const bool opti=profile==fpcNm_Obj_Lv8OptiLift_e;
    const unsigned path=(raw>>(opti?8:0))&255,speed=(raw>>(opti?16:8))&15;
    // Prefer the native east crossing (switch 0x64) when selecting the pair.
    const bool east=((raw>>(opti?0:12))&255)==0x64;
    for(auto& ferry:s_palaceFerries)
        if(!ferry.retiring && ferry.source.room_no==params.room_no && ferry.path==path) {
            ferry.east|=east;return true;
        }
    if(s_palaceFerries.size()>=32)return true; // Still suppress the native duplicate.
    PalaceFerry ferry;ferry.source=params;ferry.path=path;ferry.speed=speed;ferry.east=east;
    s_palaceFerries.push_back(ferry);
    return true;
}
void retire_palace_ferries() {
    s_palaceFerryScanned=false;s_palaceFerryRetry=0;
    s_palaceFerryTurn=kNone;s_palaceFerryDwell=0;
    for(auto& ferry:s_palaceFerries)ferry.retiring=true;
    std::erase_if(s_palaceFerries,[](const auto& ferry){return ferry.id==kNone;});
}
void cleanup_palace_ferries() {
    for(auto& ferry:s_palaceFerries) {
        if(ferry.id==kNone || fpcM_IsCreating(ferry.id))continue;
        auto* actor=actor_by_id(ferry.id);
        if(!actor) {
            if(s_palaceFerryTurn==ferry.id)s_palaceFerryTurn=kNone;
            ferry.id=kNone;ferry.ready=false;ferry.departed=false;ferry.returning=false;
        }
        else if(ferry.retiring)svc_actor->delete_actor(mod_ctx,ferry.id);
        else if(!ferry.ready) {
            ferry.ready=true;
            svc_log->info(mod_ctx,"Survivors Palace ferry: native actor initialization completed");
        }
    }
    std::erase_if(s_palaceFerries,[](const auto& ferry){return ferry.retiring && ferry.id==kNone;});
}
PalaceFerry* tracked_palace_ferry(ActorId id) {
    if(id!=kNone)for(auto& ferry:s_palaceFerries)if(ferry.id==id)return &ferry;
    return nullptr;
}
HookAction before_palace_ferry_move(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daL8Lift_c*>(args,0);
    auto* ferry=actor?tracked_palace_ferry(fopAcM_GetID(actor)):nullptr;
    if(!ferry)return HOOK_CONTINUE;
    if(ferry->retiring || !ferry->ready || !playable())return HOOK_SKIP_ORIGINAL;
    // Wait for both selected actors, including asynchronous replacements.
    PalaceFerry* first=nullptr;
    for(auto& candidate:s_palaceFerries)if(candidate.selected && !candidate.retiring) {
        if(candidate.id==kNone || !candidate.ready)return HOOK_SKIP_ORIGINAL;
        if(!first || (!first->east && candidate.east))first=&candidate;
    }
    if(!first)return HOOK_SKIP_ORIGINAL;
    if(s_palaceFerryTurn==kNone)s_palaceFerryTurn=first->id;
    if(s_palaceFerryTurn!=ferry->id)return HOOK_SKIP_ORIGINAL;
    if(s_palaceFerryDwell){--s_palaceFerryDwell;return HOOK_SKIP_ORIGINAL;}
    const auto* path=dPath_GetRoomPath(ferry->path,ferry->source.room_no);
    const bool endpoint=path && (actor->mCurrentPoint==0 || actor->mCurrentPoint==path->m_num-1);
    // WAIT_INIT follows completed braking. Keep the entire outbound/return
    // round trip exclusive: the partner stays at its start until we get home.
    if(ferry->departed && endpoint && actor->mMode==daL8Lift_c::MODE_WAIT_INIT_e) {
        ferry->departed=false;
        if(actor->mCurrentPoint==path->m_num-1) {
            ferry->returning=true; // Far dock: retain the turn for the return.
        } else if(ferry->returning) {
            ferry->returning=false; // Point zero reached after visiting the far end.
            for(const auto& next:s_palaceFerries)
                if(next.selected && !next.retiring && next.id!=ferry->id) {
                    s_palaceFerryTurn=next.id;break;
                }
        }
        s_palaceFerryDwell=60; // Preserve the two-second pause at each dock.
        return HOOK_SKIP_ORIGINAL;
    }
    if(actor->mMode==daL8Lift_c::MODE_ACC_e || actor->mMode==daL8Lift_c::MODE_MOVE_e ||
       actor->mMode==daL8Lift_c::MODE_BRK_e)ferry->departed=true;
    // Gate only moveLift: native Execute still refreshes the collision matrix,
    // lighting and ride callback state while a platform waits at its dock.
    return HOOK_CONTINUE;
}
HookAction before_palace_ferry_suspend(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<fopAc_ac_c*>(args,0);
    if(!actor || !tracked_palace_ferry(fopAcM_GetID(actor)))return HOOK_CONTINUE;
    // Native suspension volumes are checked before Execute and also hide Draw.
    // Only our owned replacement must stay available without the Sol puzzle.
    fopAcM_OffStatus(actor,fopAcStts_UNK_0x20000000_e);
    return HOOK_SKIP_ORIGINAL;
}
void discover_palace_ferries(const dStage_fileHeader* file) {
    if(!file || file->m_chunkCount<0 || file->m_chunkCount>512)return;
    for(int i=0;i<file->m_chunkCount;++i) {
        const auto& node=file->m_nodes[i];
        const auto* tag=reinterpret_cast<const char*>(&node.m_tag);
        const bool layer=(tag[3]>='0' && tag[3]<='9') || (tag[3]>='a' && tag[3]<='e');
        const bool actor=(!std::memcmp(tag,"ACT",3) && (tag[3]=='R'||layer)) || !std::memcmp(tag,"TGOB",4);
        const bool scaled=(!std::memcmp(tag,"SCO",3) && (tag[3]=='B'||layer)) || !std::memcmp(tag,"TGSC",4);
        if((!actor && !scaled) || node.m_entryNum<0 || node.m_entryNum>4096)continue;
        const auto* entries=static_cast<const unsigned char*>(node.m_offset);
        if(!entries)continue;
        const size_t stride=scaled?sizeof(stage_tgsc_data_class):sizeof(stage_actor_data_class);
        for(int n=0;n<node.m_entryNum;++n) {
            const auto* record=reinterpret_cast<const stage_actor_data_class*>(entries+n*stride);
            int profile;
            if(!std::memcmp(record->name,"L8LiftX\0",8))profile=fpcNm_Obj_Lv8Lift_e;
            else if(!std::memcmp(record->name,"optLift\0",8))profile=fpcNm_Obj_Lv8OptiLift_e;
            else continue;
            fopAcM_prm_class params{};params.base=record->base;params.room_no=arena().room;
            params.scale.x=params.scale.y=params.scale.z=10;
            if(scaled)params.scale=reinterpret_cast<const stage_tgsc_data_class*>(record)->scale;
            capture_palace_ferry(profile,params);
        }
    }
}
void update_palace_ferries() {
    if(s_map!=3 || !in_arena() || !playable())return;
    if(s_palaceFerryRetry){--s_palaceFerryRetry;return;}
    // Never overlap actors from the previous arena with a fresh pair.
    for(const auto& ferry:s_palaceFerries)if(ferry.retiring && ferry.id!=kNone)return;
    if(!s_palaceFerryScanned) {
        // Room loader has already relocated every node, including inactive
        // layers. Scan once, without changing the archive or its actor records.
        auto* file=static_cast<dStage_fileHeader*>(
            dComIfG_getStageRes(dComIfG_getRoomArcName(arena().room),"room.dzr"));
        if(!file){s_palaceFerryRetry=30;return;}
        discover_palace_ferries(file);s_palaceFerryScanned=true;
        // Keep both authored crossings, but never activate additional layer
        // variants. Lock the pair before creation, including failed retries.
        for(unsigned n=0;n<2;++n) {
            PalaceFerry* chosen=nullptr;
            for(auto& ferry:s_palaceFerries) {
                if(ferry.retiring || ferry.selected)continue;
                if(!chosen || (!chosen->east && ferry.east))chosen=&ferry;
            }
            if(chosen)chosen->selected=true;
        }
        char message[128];std::snprintf(message,sizeof(message),
            "Survivors Palace ferry: found %u authored routes across room layers",unsigned(s_palaceFerries.size()));
        svc_log->info(mod_ctx,message);
        if(s_palaceFerries.empty())svc_log->warn(mod_ctx,"Survivors Palace ferry: no native lift records found in room.dzr");
    }
    for(auto& ferry:s_palaceFerries) {
        if(ferry.retiring || !ferry.selected || ferry.id!=kNone)continue;
        const auto* path=dPath_GetRoomPath(ferry.path,ferry.source.room_no);
        // Native setNextPoint assumes at least two points and uses signed byte
        // endpoint indices. Never launch an actor stuck in Create on a bad path.
        bool valid=ferry.path!=255 && path && path->m_points!=nullptr && path->m_num>=2 && path->m_num<=127;
        if(valid)for(int i=0;i<path->m_num;++i) {
            const Vec point=path->m_points[i].m_position;
            if(!std::isfinite(point.x)||!std::isfinite(point.y)||!std::isfinite(point.z))valid=false;
        }
        if(!valid) {
            if(!ferry.reported)svc_log->warn(mod_ctx,"Survivors Palace ferry: authored path unavailable or invalid");
            ferry.reported=true;continue;
        }
        const Vec position=path->m_points[0].m_position;
        const csXyz angle=ferry.source.base.angle;
        const ActorSpawnParams params{
            .parameters=0xfff00000u|0x000ff000u|(ferry.speed<<8)|ferry.path,
            .argument=-1,.room_num=ferry.source.room_no,
            .position={position.x,position.y,position.z},.angle={angle.x,angle.y,angle.z},
            .scale={ferry.source.scale.x*.1f,ferry.source.scale.y*.1f,ferry.source.scale.z*.1f},
            .create_function=nullptr,
        };
        ActorId id=kNone;
        if(create_standalone_actor(fpcNm_Obj_Lv8Lift_e,params,id)==MOD_OK && id!=kNone) {
            ferry.id=id;
            char message[128];std::snprintf(message,sizeof(message),
                "Survivors Palace ferry: requested actor %u, room %d path %u (%u points)",
                unsigned(id),int(ferry.source.room_no),ferry.path,unsigned(path->m_num));
            svc_log->info(mod_ctx,message);
        }
        s_palaceFerryRetry=30;return;
    }
    s_palaceFerryRetry=30;
}
