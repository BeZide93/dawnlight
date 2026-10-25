// Bind room ownership to the existing Survivors process/stage hooks. Keep this
// integration separate from enemy/pot processing and its standalone fixtures.
HookAction before_roundtable(ModContext*,void* args,void* result,void*) {
    return before_roundtable_process(args,result,survivors_title_preparing() || (s_hub && in_arena()));
}
HookAction before_roundtable_stage_actor(ModContext*,void* args,void*,void*) {
    if(!(survivors_title_preparing() || (s_hub && in_arena())))return HOOK_CONTINUE;
    const auto* record=mods::arg<stage_actor_data_class*>(args,0);
    auto* params=mods::arg<fopAcM_prm_class*>(args,1);
    if(!record || !params || (params->room_no!=51 && params->room_no!=-1))return HOOK_CONTINUE;
    char name[9]{};std::memcpy(name,record->name,8);
    const auto* info=dStage_searchName(name);if(!info)return HOOK_CONTINUE;
    // The replacement room supplies its own furnishings and lights. Keep the
    // authored exit and scene infrastructure; standalone Zelda/bowl/chest are
    // spawned separately, so this cannot remove them or their interactions.
    switch(info->procname) {
    case fpcNm_ALINK_e:case fpcNm_BG_e:case fpcNm_KANKYO_e:case fpcNm_ENVSE_e:return HOOK_CONTINUE;
    case fpcNm_DOOR20_e:case fpcNm_KNOB20_e:case fpcNm_DBDOOR_e:
    case fpcNm_BOSS_DOOR_e:case fpcNm_L1BOSS_DOOR_e:case fpcNm_L1MBOSS_DOOR_e:
    case fpcNm_L5BOSS_DOOR_e:case fpcNm_SPIRAL_DOOR_e:
        // Relocate before native construction: home/current position, cached
        // model/collision matrices, attention and event goals share one anchor.
        // Identify the southern exit by its corridor region, not Link's
        // BS_SHUTTER_START event position (which is NOT the door anchor).
        // Room/profile gating above keeps unrelated doors untouched.
        if(std::abs(float(params->base.position.x))<400.f &&
           std::isfinite(float(params->base.position.z)) &&
           float(params->base.position.z)>roundtable_layout::exitZ+300.f) {
            params->base.position.x=0;
            params->base.position.y=roundtable_layout::floorY;
            params->base.position.z=roundtable_layout::exitZ;
        }
        return HOOK_CONTINUE;
    default:JKRFree(params);return HOOK_SKIP_ORIGINAL;
    }
}
void after_roundtable(ModContext*,void* args,void* result,void*) {
    after_roundtable_process(args,result);
}
