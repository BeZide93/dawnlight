// Reuse the native Boss Rush light circle, including its animated model and
// moving background collision. Ownership is by ID, never by profile search.
struct BossRing {
    ActorId id=kNone;
    bool anchored=false,retiring=false;
    unsigned retry=0;
    cXyz center{};
};
BossRing s_bossRing;
bool inside_boss_ring(const cXyz& position,float margin=0.f) {
    return !s_bossRing.anchored||s_bossRing.retiring||
        (position-s_bossRing.center).absXZ()<=boss_ring_radius-margin;
}

void cleanup_boss_ring() {
    if(s_bossRing.id==kNone)return;
    auto* actor=actor_by_id(s_bossRing.id);
    if(!actor&&fpcM_IsCreating(s_bossRing.id))return;
    if(!actor||fopAcM_GetName(actor)!=fpcNm_OBJ_GB_e) {
        release_boss_ring_terrain();
        if(s_bossRing.retiring)s_bossRing={};
        else {s_bossRing.id=kNone;s_bossRing.retry=30;}
        return;
    }
    if(s_bossRing.retiring)svc_actor->delete_actor(mod_ctx,s_bossRing.id);
}
void retire_boss_ring() {
    if(s_bossRing.id==kNone){release_boss_ring_terrain();s_bossRing={};return;}
    s_bossRing.anchored=false;s_bossRing.retiring=true;
    // Keep pending creation IDs until native initialization can finish and its
    // normal Delete releases the registered dBgW and archive reference.
    cleanup_boss_ring();
}
void shutdown_boss_ring() {
    auto* actor=actor_by_id(s_bossRing.id);
    // Native deletion may finish after the loader removes our hooks. Detach the
    // model callback and its private vertex storage before the module unloads.
    release_boss_ring_terrain(actor&&fopAcM_GetName(actor)==fpcNm_OBJ_GB_e?
        static_cast<obj_gb_class*>(actor):nullptr);
    retire_boss_ring();
}
bool boss_ring_encounter_active() {
    if(!in_arena()||s_hub||s_victory||s_deathPending||transitioning())return false;
    if(s_run.phase!=Phase::Running&&s_run.phase!=Phase::Upgrade&&
       s_run.phase!=Phase::TreasureChoice&&s_run.phase!=Phase::Treasure)return false;
    for(const auto& e:s_enemies) {
        if(e.id==kNone||!e.elite||!e.initialized||e.dormant||e.retiring||e.credited)continue;
        if(auto* actor=actor_by_id(e.id);actor&&actor->health>0)return true;
    }
    return false;
}
void update_boss_ring() {
    if(!boss_ring_encounter_active()){retire_boss_ring();return;}
    if(s_bossRing.id!=kNone||!playable()||s_run.phase!=Phase::Running)return;
    if(s_bossRing.retry){--s_bossRing.retry;return;}
    auto* link=daAlink_getAlinkActorClass();if(!link)return;
    if(!s_bossRing.anchored) {
        cXyz center=link->current.pos;
        if(!loot_floor_position(center)){s_bossRing.retry=30;return;}
        s_bossRing.center=center;s_bossRing.anchored=true;
    }
    const auto& center=s_bossRing.center;
    const ActorSpawnParams params{
        .parameters=(ganondorf_barrier_parameters&0xff00ffffu)|(30u<<16),.argument=-1,.room_num=arena().room,
        .position={center.x,center.y,center.z},.angle={0,0,0},
        .scale={1.5f,kBossRingHeight,1.5f},.create_function=nullptr,
    };
    ActorId id=kNone;
    if(create_standalone_actor(fpcNm_OBJ_GB_e,params,id)==MOD_OK&&id!=kNone)
        s_bossRing.id=id;
    else s_bossRing.retry=30;
}
HookAction process_boss_ring(fopAc_ac_c* actor,void* args,void* result) {
    if(fopAcM_GetName(actor)!=fpcNm_OBJ_GB_e){release_boss_ring_terrain();s_bossRing={};return HOOK_CONTINUE;}
    const auto* methods=reinterpret_cast<const process_method_class*>(actor->sub_method);
    if(!methods)return HOOK_CONTINUE;
    const auto method=mods::arg<process_method_func>(args,0);
    if(method==methods->delete_method) {
        release_boss_ring_terrain(static_cast<obj_gb_class*>(actor));
        if(s_bossRing.retiring)s_bossRing={};
        else {s_bossRing.id=kNone;s_bossRing.retry=30;}
        return HOOK_CONTINUE;
    }
    if(method!=methods->execute_method)return HOOK_CONTINUE;
    if(s_bossRing.retiring||!boss_ring_encounter_active()) {
        retire_boss_ring();*static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
    }
    if(!playable()||s_run.phase!=Phase::Running) {
        *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
    }
    // Native Execute may switch to its fade state outside the final-battle
    // stage. Re-arm only our circle each frame; never modify room/save switches.
    auto* barrier=static_cast<obj_gb_class*>(actor);
    if(!prepare_boss_ring_terrain(barrier,s_bossRing.center)) {
        *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
    }
    configure_ganondorf_barrier(barrier);
    barrier->scale.y=kBossRingHeight;
    return HOOK_CONTINUE;
}
