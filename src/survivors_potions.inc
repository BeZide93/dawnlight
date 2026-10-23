// Scale native red-potion drinking. From Endless 40:00, only this native
// recovery path remains allowed; level-ups and passive regen set life directly.
daAlink_c* s_potionDrinker = nullptr;
HookAction before_potion_drink(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    s_potionDrinker = in_arena() && s_run.phase == Phase::Running && s_run.red_potion &&
        link && fopAcM_GetID(link) == s_player && link->mEquipItem == dItemNo_RED_BOTTLE_e ? link : nullptr;
    return HOOK_CONTINUE;
}
void after_potion_drink(ModContext*, void*, void*, void*) { s_potionDrinker = nullptr; }
HookAction before_potion_heal(ModContext*, void* args, void*, void*) {
    if (in_arena() && s_run.greed() && mods::arg<float>(args,0)>0)
        return HOOK_SKIP_ORIGINAL;
    const bool redPotion = s_potionDrinker && in_arena() && s_run.phase == Phase::Running && s_run.red_potion &&
        fopAcM_GetID(s_potionDrinker) == s_player && s_potionDrinker->mEquipItem == dItemNo_RED_BOTTLE_e &&
        mods::arg<float>(args, 0) > 0 && mods::arg<u8>(args, 1) == 0;
    if (redPotion) mods::arg_ref<float>(args, 0) = float(s_run.potion_heal_quarters());
    else if (in_arena() && s_run.phase==Phase::Running && s_run.restricted_healing() && mods::arg<float>(args,0)>0)
        return HOOK_SKIP_ORIGINAL; // No hearts, fairy or other native recovery after Endless 40:00.
    // Native life-count units are quarter hearts; retain normal animation,
    // missing-health clamping, bottle consumption and empty-bottle handling.
    return HOOK_CONTINUE;
}
