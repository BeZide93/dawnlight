// Native carry pots own their model, collision, carrying/throwing and shatter
// effects. Track only our instances; never infer a break from actor deletion.
struct ArenaPot {
    ActorId id=kNone;
    bool initialized=false, broken=false, retiring=false;
    cXyz position{};
    unsigned respawnAt=0;
};
std::array<ArenaPot,pot_capacity> s_pots;
ActorSlotIndex<ActorId> s_potIndex;
unsigned s_potTarget=0, s_potDelay=0;
ArenaPot* tracked_pot(ActorId id) {
    if (id==kNone) return nullptr;
    return s_potIndex.find(s_pots,id);
}
void detach_pot_attacks() {
    for (const auto& pot:s_pots) {
        if (!pot.initialized) continue;
        auto* actor=static_cast<daObjCarry_c*>(actor_by_id(pot.id));
        if (actor && own_collider(actor->mCyl.GetTgHitObj())) actor->mCyl.ResetTgHit();
        if (actor && own_collider(actor->mCyl.GetAtHitObj())) actor->mCyl.ResetAtHit();
    }
}
void retire_pots() {
    s_potTarget=s_potDelay=0;
    for (auto& pot:s_pots) {
        if (pot.id==kNone) pot={};
        else pot.retiring=true;
    }
}
void cleanup_pots() {
    for (auto& pot:s_pots) {
        if (pot.id==kNone) continue;
        auto* actor=actor_by_id(pot.id);
        if (!actor && !fpcM_IsCreating(pot.id)) {
            s_potIndex.erase(pot.id);
            pot.id=kNone; pot.initialized=false;
            if (pot.retiring) pot={};
        } else if (actor && pot.retiring) svc_actor->delete_actor(mod_ctx,pot.id);
    }
}
bool pot_position(daAlink_c* link, cXyz& position) {
    if (!random_arena_floor(position) || position.absXZ(link->current.pos)<300) return false;
    for (const auto& pot:s_pots)
        if (pot.id!=kNone && position.absXZ(pot.position)<250) return false;
    return separated_guard(position,250);
}
void update_pots(daAlink_c* link) {
    if (!playable() || s_run.phase!=Phase::Running || !s_arenaMeshReady || !arena_distribution_ready()) return;
    if (!s_potTarget) s_potTarget=pot_budget(s_map,arena_distribution_area());
    if (s_potDelay) {--s_potDelay;return;}
    for (unsigned i=0;i<s_potTarget;++i) {
        auto& pot=s_pots[i];
        if (pot.id!=kNone || (pot.broken && s_run.ticks < pot.respawnAt)) continue;
        cXyz pos;bool found=false;
        for (unsigned attempt=0;attempt<32 && !found;++attempt)
            found=pot_position(link,pos) && (!pot.broken || pos.absXZ(pot.position)>=300);
        s_potDelay=found ? 5 : 30;
        if (!found) return;
        // Same native small-pot parameters as the mirror-hub pots. No save bit,
        // switch or built-in item; the confirmed break hook rolls the reward.
        const ActorSpawnParams params{
            .parameters=0x00003fff, .argument=-1, .room_num=arena().room,
            .position={pos.x,pos.y,pos.z},
            .angle={s16(0xffff),s16(s_run.next_random()),0x1040},
            .scale={1,1,1}, .create_function=nullptr,
        };
        ActorId id=kNone;
        if (create_standalone_actor(fpcNm_Obj_Carry_e,params,id)==MOD_OK && id!=kNone) {
            pot={};pot.id=id;pot.position=pos;
            s_potIndex.add(id,i);
        } else s_potDelay=30;
        return; // Stagger resource loading; at most one native creation per pass.
    }
}
HookAction before_pot_break(ModContext*, void* args, void*, void*) {
    auto* actor=mods::arg<daObjCarry_c*>(args,0);
    auto* pot=actor ? tracked_pot(fopAcM_GetID(actor)) : nullptr;
    if (!pot) return HOOK_CONTINUE;
    const bool createItem=mods::arg<bool>(args,1);
    mods::arg_ref<bool>(args,1)=false; // Native shatter/carry cleanup still runs.
    if (pot->broken || pot->retiring || !pot->initialized || !in_arena() ||
        transitioning() || s_run.phase!=Phase::Running) return HOOK_CONTINUE;
    pot->broken=true;
    pot->position=actor->current.pos;
    pot->respawnAt=s_run.ticks+pot_respawn_interval;
    if (!createItem) return HOOK_CONTINUE; // Sinking/hazards do not grant loot.
    constexpr std::array<int,unsigned(Run::PotDrop::Count)> items{
        dItemNo_KAKERA_HEART_e,dItemNo_GREEN_RUPEE_e,dItemNo_BLUE_RUPEE_e,
        dItemNo_YELLOW_RUPEE_e,dItemNo_RED_RUPEE_e,dItemNo_PURPLE_RUPEE_e,
        dItemNo_ORANGE_RUPEE_e,dItemNo_SILVER_RUPEE_e,dItemNo_KEY_OF_FILONE_e};
    const int item=items[unsigned(s_run.roll_pot_drop())];
    create_run_pickup(actor->current.pos,item);
    return HOOK_CONTINUE;
}
