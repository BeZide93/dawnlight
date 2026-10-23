// Portal outcomes share the run clock; native pickups retain their lifecycle.
bool activate_fate(daAlink_c*,const cXyz& position) {
    const auto effect=s_run.roll_fate();
    if (effect==FateEffect::Fairy &&
        create_run_pickup(position+cXyz(0,50,0),dItemNo_RECOVERY_FAILY_e)==kNone) return false;
    s_run.apply_fate(effect);
    if (effect==FateEffect::Horde) s_spawnCooldown=0;
    if (effect==FateEffect::Curse) dComIfGs_setLife(Run::cursed_life(dComIfGs_getLife()));
    // Remove an already queued positive heal as well as blocking new healing.
    if (effect==FateEffect::Greed && dComIfGp_getItemLifeCount()>0) dComIfGp_clearItemLifeCount();
    fate_feedback(position,effect);
    return true;
}
HookAction before_fate_rupees(ModContext*,void* args,void*,void*) {
    auto& amount=mods::arg_ref<s32>(args,0);
    if (in_arena() && s_run.greed() && amount>0)
        amount=s32(std::min<unsigned>(INT32_MAX,s_run.gain_rupees(unsigned(amount))));
    return HOOK_CONTINUE;
}
HookAction before_fate_sprint(ModContext*,void* args,void*,void*) {
    const auto* link=mods::arg<const daAlink_c*>(args,0);
    auto& animation=mods::arg_ref<daAlink_c::daAlink_ANM>(args,1);
    if (link && fopAcM_GetID(link)==s_player && in_arena() && playable() &&
        s_run.phase==Phase::Running && s_run.fate_ticks(FateEffect::Sprint) &&
        link->mProcID==daAlink_c::PROC_MOVE && animation==daAlink_c::ANM_RUN)
        animation=daAlink_c::ANM_RUN_B;
    return HOOK_CONTINUE;
}
HookAction before_fate_fairy_particles(ModContext*,void* args,void*,void*) {
    // Like Boss Rush: native fairy expiry/capture calls this on getEmitter()
    // without a null check. Missing stage particles have nothing to release.
    if (in_arena() && mods::arg<JPABaseEmitter*>(args,0)==nullptr) return HOOK_SKIP_ORIGINAL;
    return HOOK_CONTINUE;
}
