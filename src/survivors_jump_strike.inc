// Own only the automatically started hidden-skill Jump Strike. Manual jumps,
// Gale and glide must never acquire this exception to the traversal pause.
std::array<u32, 2> s_jumpEmitters{};
bool jump_strike_pose(const daAlink_c* link) {
    return link->mProcID == daAlink_c::PROC_CUT_LARGE_JUMP ||
        link->mProcID == daAlink_c::PROC_CUT_LARGE_JUMP_LAND;
}
void clear_jump_effects() {
    s_visualBudget.release(s_jumpEmitters.data());
    for (auto& key : s_jumpEmitters) {
        if (auto* emitter = key ? dComIfGp_particle_getEmitter(key) : nullptr) {
            emitter->stopCreateParticle(); emitter->quitImmortalEmitter();
            emitter->becomeInvalidEmitter();
        }
        key = 0;
    }
}
bool try_jump_strike(daAlink_c* link) {
    if (s_meleeCooldown || s_cooldowns[unsigned(Upgrade::JumpStrike)]) return false;
    cXyz targetPoint;
    auto* target = melee_target(link, &targetPoint);
    if (!target || !tracked(fopAcM_GetID(target)) || !s_run.roll_jump_strike()) return false;
    const cXyz facing=manual_skill(link,Upgrade::JumpStrike)?manual_direction(link):targetPoint-link->current.pos;
    link->shape_angle.y=cM_atan2s(facing.x,facing.z);
    // The nearby target only permits the skill; it never supplies movement.
    link->current.angle.y = link->mMoveAngle;
    if (!link->procCutLargeJumpInit()) return false;
    s_swordAction = SwordAction::Jump;
    new_melee_swing();
    s_trackMelee = s_wavePending = false; s_meleeTarget = kNone;
    s_comboStep = s_comboTimeout = 0;
    link->mUnderFrameCtrl[0].setRate(2.0f);
    link->mUpperFrameCtrl[0].setRate(2.0f);
    s_cooldowns[unsigned(Upgrade::JumpStrike)] = s_run.jump_cooldown();
    s_meleeCooldown = s_run.cooldown(30);
    return true;
}
HookAction before_jump_target(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!link||!playable()||s_run.phase!=Phase::Running||fopAcM_GetID(link)!=s_player||
       s_swordAction!=SwordAction::Jump||!jump_strike_pose(link)||jump_abilities_busy_for(link))return HOOK_CONTINUE;
    // Native launch already set vertical speed/gravity. Skip target-derived yaw
    // and horizontal launch speed; the movement adapter reads the stick instead.
    return HOOK_SKIP_ORIGINAL;
}
bool update_jump_strike(daAlink_c* link) {
    if (s_swordAction != SwordAction::Jump) return false;
    if (jump_strike_pose(link)) return !jump_abilities_busy_for(link);
    // Damage, a chest interaction or another traversal action owns the pose now.
    s_swordAction = SwordAction::None;
    return false;
}
void after_jump_strike(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !playable() || s_run.phase != Phase::Running ||
        fopAcM_GetID(link) != s_player || s_swordAction != SwordAction::Jump ||
        !jump_strike_pose(link) || jump_abilities_busy_for(link)) return;
    // The native start, airborne slash and landing each install a new animation.
    link->mUnderFrameCtrl[0].setRate(2.0f);
    link->mUpperFrameCtrl[0].setRate(2.0f);
    link->field_0x3198 = 2; // No unowned follow-up spin from stick input.
    if (link->mProcID == daAlink_c::PROC_CUT_LARGE_JUMP_LAND)
        link->mAtSph.SetR(s_run.melee_reach(link->field_0x347c));
}
HookAction before_jump_strike_effect(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !playable() || s_run.phase != Phase::Running ||
        fopAcM_GetID(link) != s_player || s_swordAction != SwordAction::Jump ||
        !jump_strike_pose(link) || jump_abilities_busy_for(link)) return HOOK_CONTINUE;
    clear_jump_effects();
    // Original Survivors bomb shockwave pair, before the full nine-part native
    // bomb explosion was adopted. Scale the visual with the landing hit radius.
    constexpr std::array<u16, 2> effects{ID_ZI_J_BOMBex_A, ID_ZI_J_BOMBex_B};
    const float size = s_run.melee_reach(link->field_0x3478) / 200.0f;
    const cXyz scale(size, size, size);
    if(!full_visual(s_jumpEmitters,survivor_perf::Effect::Other,2))return HOOK_SKIP_ORIGINAL;
    for (unsigned i = 0; i < effects.size(); ++i)
        s_jumpEmitters[i] = dComIfGp_particle_set(0, effects[i], &link->current.pos,
            &link->tevStr, nullptr, &scale, 0xff, nullptr, -1, nullptr, nullptr, nullptr);
    return HOOK_SKIP_ORIGINAL;
}
