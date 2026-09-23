// Included by jump_hooks.cpp; all hooks are scoped to a manual ZR jump.
DEFINE_HOOK(&daAlink_c::checkCutJumpInFly, AirCutInFlyHook);
DEFINE_HOOK(&daAlink_c::procCutNormal, AirCutNormalHook);
DEFINE_HOOK(&daAlink_c::procCutFinish, AirCutFinishHook);
DEFINE_HOOK(&daAlink_c::procCutReverse, AirCutReverseHook);
DEFINE_HOOK(&daAlink_c::checkNextAction, AirNextActionHook);

struct AirCombo {
    bool active = false;
    bool used = false;
    bool recoil = false;
    int ticks = 0;
    unsigned strikes = 0;
} s_airCombo;

constexpr int kAirComboTicks = 90; // Three seconds at the game's 30 Hz tick rate.
constexpr float kAirComboFallSpeed = -0.8f;
constexpr float kAirComboMoveSpeed = 6.0f;

bool air_cut_proc(daAlink_c::daAlink_PROC proc) {
    return proc == daAlink_c::PROC_CUT_NORMAL || proc == daAlink_c::PROC_CUT_FINISH;
}

bool owns_air_combo(const daAlink_c* link) {
    return link && s_manualJumpOwner == link && s_airCombo.active;
}

void stop_air_combo(daAlink_c* link) {
    if (!owns_air_combo(link)) return;
    s_airCombo.active = false;
    s_airCombo.recoil = false;
    link->initGravity();
    link->clearComboReserb();
    link->resetCombo(TRUE);
    link->resetAtCollision(TRUE);
    link->offResetFlg0(daPy_py_c::RFLG0_UNK_2);
    link->offResetFlg0(daPy_py_c::RFLG0_UNK_40);
}

// commonProcInit resets mode flags and gravity on every hit in the string.
// Preserve ownership through these transitions, never through damage/events.
bool retain_air_combo(daAlink_c* link, daAlink_c::daAlink_PROC next) {
    if (!air_combos_enabled() || !r_jump_enabled()) return false;
    if (s_airCombo.active && air_cut_proc(next)) return true;
    if (s_airCombo.active && next == daAlink_c::PROC_CUT_REVERSE) {
        s_airCombo.recoil = true;
        link->initGravity();
        return true;
    }
    if (s_airCombo.used && next == daAlink_c::PROC_FALL) {
        stop_air_combo(link);
        return true; // Remember the spent combo until landing.
    }
    return false;
}

void apply_air_combo_physics(daAlink_c* link) {
    if (!owns_air_combo(link)) return;
    // Do not call setJumpMode(): it would reset fall height at every swing.
    link->onModeFlg(daAlink_c::MODE_JUMP);
    if (s_airCombo.recoil) return;
    if (link->speed.y > 0.0f) {
        link->initGravity(); // Finish the original ascent; never add lift.
    } else {
        link->speed.y = std::max(link->speed.y, kAirComboFallSpeed);
        link->setSpecialGravity(-0.08f, kAirComboFallSpeed, FALSE);
    }
    link->mNormalSpeed = std::clamp(link->mNormalSpeed, -kAirComboMoveSpeed, kAirComboMoveSpeed);
}

void leave_air_combo(daAlink_c* link) {
    const bool grounded = link->mLinkAcch.ChkGroundHit();
    stop_air_combo(link);
    if (grounded) {
        link->checkLandAction(0);
    } else {
        link->procFallInit(0, 3.0f);
    }
}

bool start_air_combo(daAlink_c* link) {
    if (!air_combos_enabled() || !r_jump_enabled() || s_manualJumpOwner != link ||
        s_airCombo.used || link->mProcID != daAlink_c::PROC_AUTO_JUMP ||
        link->mLinkAcch.ChkGroundHit() || link->mEquipItem != kSwordItem ||
        link->checkWolf() || link->checkEventRun() || dComIfGp_event_runCheck() ||
        link->checkBootsOrArmorHeavy() || link->checkNoResetFlg0(daPy_py_c::FLG0_WATER_IN_MOVE))
    {
        return false;
    }
    clear_manual_jump(link); // Restore Bullet Time before taking over gravity.
    s_airCombo = {true, true, false, 0, 1};
    link->resetCombo(TRUE);
    link->clearComboReserb();
    link->checkCutAction(); // Native animation, hit windows, damage, sound and finisher.
    apply_air_combo_physics(link);
    return true;
}

HookAction before_air_cut_in_fly(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!air_combos_enabled() || !r_jump_enabled() || s_manualJumpOwner != link)
        return HOOK_CONTINUE;
    // Covers the native B route too, including after ZR has been released.
    *static_cast<BOOL*>(retval) = link->swordSwingTrigger() && start_air_combo(link);
    return HOOK_SKIP_ORIGINAL;
}

void after_air_common_proc_init(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    apply_air_combo_physics(link);
}

HookAction before_air_cut(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!owns_air_combo(link)) return HOOK_CONTINUE;
    if (link->mLinkAcch.ChkGroundHit() || !air_combos_enabled() || !r_jump_enabled() ||
        link->mEquipItem != kSwordItem || link->checkBootsOrArmorHeavy() ||
        link->checkNoResetFlg0(daPy_py_c::FLG0_WATER_IN_MOVE) || ++s_airCombo.ticks > kAirComboTicks)
    {
        leave_air_combo(link);
        *static_cast<int*>(retval) = 1;
        return HOOK_SKIP_ORIGINAL;
    }
    apply_air_combo_physics(link);
    return HOOK_CONTINUE;
}

void after_air_cut(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!owns_air_combo(link)) return;
    // Native ground combos charge spin on held B; air combos only buffer taps.
    link->mProcVar5.mCutTurnChargeCheckTimer = 0;
    link->offResetFlg0(daPy_py_c::RFLG0_UNK_40);
    apply_air_combo_physics(link);
}

HookAction before_air_next_action(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!owns_air_combo(link)) return HOOK_CONTINUE;
    const int cancel = mods::arg<int>(args, 1);
    int result = 0;
    if (!s_airCombo.recoil && s_airCombo.strikes < 4 &&
        (link->swordSwingTrigger() || link->checkComboReserb()))
    {
        link->clearComboReserb();
        // procCutFinish can reset the native counter before its cancel check.
        link->mComboCutCount = s_airCombo.strikes++;
        link->checkCutAction();
        apply_air_combo_physics(link);
        result = 1;
    } else if (!cancel) {
        leave_air_combo(link);
        result = 1;
    }
    *static_cast<int*>(retval) = result;
    return HOOK_SKIP_ORIGINAL;
}

ModResult install_air_combo_hooks() {
    ModResult result = mods::hook_add_pre<AirCutInFlyHook>(svc_hook, before_air_cut_in_fly);
    if (result == MOD_OK) result = mods::hook_add_post<CommonProcInit>(svc_hook, after_air_common_proc_init);
    if (result == MOD_OK) result = mods::hook_add_pre<AirNextActionHook>(svc_hook, before_air_next_action);
    if (result == MOD_OK) result = mods::hook_add_pre<AirCutNormalHook>(svc_hook, before_air_cut);
    if (result == MOD_OK) result = mods::hook_add_post<AirCutNormalHook>(svc_hook, after_air_cut);
    if (result == MOD_OK) result = mods::hook_add_pre<AirCutFinishHook>(svc_hook, before_air_cut);
    if (result == MOD_OK) result = mods::hook_add_post<AirCutFinishHook>(svc_hook, after_air_cut);
    if (result == MOD_OK) result = mods::hook_add_pre<AirCutReverseHook>(svc_hook, before_air_cut);
    if (result == MOD_OK) result = mods::hook_add_post<AirCutReverseHook>(svc_hook, after_air_cut);
    return result;
}
