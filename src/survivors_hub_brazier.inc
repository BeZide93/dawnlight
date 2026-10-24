// Native Hyrule Castle fire bowl. The engine owns its mesh, flame, sound and light.
ActorId s_hubBrazier = kNone;
bool s_hubBrazierRetiring = false;
unsigned s_hubBrazierRetry = 0;
// Obj_Lv2Candle: model 2 = L9Candle, point light 1, switch 0xff = lit.
// Off type 0 keeps the decorative fire lit even when hit by wind.
constexpr u32 kHubBrazierParams = 0x002010ff;
fopAc_ac_c* hub_brazier_actor() {
    auto* actor = actor_by_id(s_hubBrazier);
    return actor && fopAcM_GetName(actor) == fpcNm_Obj_Lv2Candle_e ? actor : nullptr;
}
bool hub_brazier_anchor(cXyz& position) {
    auto* actor = hub_brazier_actor();
    if (!actor || s_hubBrazierRetiring) return false;
    position = actor->current.pos;
    return true;
}
void update_hub_brazier(bool enabled) {
    if (!enabled) {s_hubBrazierRetiring = true; s_hubBrazierRetry = 0;}
    if (s_hubBrazier != kNone && !fpcM_IsCreating(s_hubBrazier)) {
        if (!hub_brazier_actor()) {
            s_hubBrazier = kNone;
            s_hubBrazierRetiring = false;
        } else if (s_hubBrazierRetiring) svc_actor->delete_actor(mod_ctx, s_hubBrazier);
    }
    // Keep ownership during asynchronous creation/deletion, including departure.
    if (!enabled || s_hubBrazier != kNone) return;
    s_hubBrazierRetiring = false;
    if (s_hubBrazierRetry) {--s_hubBrazierRetry; return;}
    s_hubBrazierRetry = 30;
    auto* link = daAlink_getAlinkActorClass();
    if (!link) return;
    cXyz probe(0, link->current.pos.y + 1000, 0);
    dBgS_ObjGndChk ground; ground.SetPos(&probe);
    const float height = dComIfG_Bgsp().GroundCross(&ground);
    if (!std::isfinite(height) || height <= -1.e8f) return;
    const ActorSpawnParams params{
        .parameters=kHubBrazierParams, .argument=-1, .room_num=arenas[0].room,
        .position={0,height,0}, .angle={0,0,0}, .scale={1,1,1}, .create_function=nullptr,
    };
    ActorId id = kNone;
    if (create_standalone_actor(fpcNm_Obj_Lv2Candle_e, params, id) == MOD_OK) s_hubBrazier = id;
}
