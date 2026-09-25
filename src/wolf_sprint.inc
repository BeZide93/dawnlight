// Included inside jump_hooks.cpp's anonymous namespace.
daAlink_c* s_wolfSprintOwner = nullptr;

float wolf_dash_speed(daAlink_c* link) {
    const auto& move = link->mpHIO->mWolf.mWlMove.m;
    if (link->checkWolfSlowDash()) return move.mADashMaxSpeedSlow;
    if (link->field_0x2fc7 == 2) return move.mADashMaxSpeedSlow2;
    return move.mADashMaxSpeed;
}

bool wolf_sprint_requested(daAlink_c* link) {
    if (!link || !wolf_sprint_enabled() || !link->checkWolf() || !link->mpHIO ||
        !link->doButton() || !link->checkInputOnR() || !link->checkWolfDashMode() ||
        !link->mLinkAcch.ChkGroundHit()) return false;
    if (link->mProcID != daAlink_c::PROC_WOLF_WAIT &&
        link->mProcID != daAlink_c::PROC_WOLF_MOVE &&
        link->mProcID != daAlink_c::PROC_WOLF_DASH) return false;
    return !link->checkPlayerDemoMode() && !link->checkEventRun() &&
        !dComIfGp_event_runCheck() && !link->checkAttentionState() &&
        !link->checkSlope() && !link->checkNotJumpSinkLimit() &&
        !link->checkWolfGrabAnime() && link->mGrabItemAcKeep.getActor() == nullptr;
}

HookAction before_wolf_sprint_movement(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!wolf_sprint_requested(link)) {
        if (link && s_wolfSprintOwner == link) {
            // Dash's initializer sets this limit only once; undo our override
            // on release/disable. Ordinary movement selects its own limit.
            if (link->mProcID == daAlink_c::PROC_WOLF_DASH && link->mpHIO)
                link->mMaxSpeed = wolf_dash_speed(link);
            s_wolfSprintOwner = nullptr;
        }
        return HOOK_CONTINUE;
    }
    // Renew the dash state, not its starting animation or input trigger.
    // Native acceleration, steering, walls and action transitions still run.
    link->field_0x30d0 = std::max<int>(link->field_0x30d0, 2);
    link->mMaxSpeed = wolf_dash_speed(link) * wolf_speed_multiplier();
    s_wolfSprintOwner = link;
    return HOOK_CONTINUE;
}

struct WolfJumpMomentum {
    daAlink_c* link;
    void* args;
    float speed;
    bool carry;
};
thread_local std::vector<WolfJumpMomentum> s_wolfJumpMomentum;

HookAction before_wolf_sprint_jump(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    // Snapshot before native jump init caps horizontal speed and changes proc.
    // Releasing Dash at takeoff must not discard already-earned momentum.
    const bool carry = link && wolf_sprint_enabled() && link->checkWolf() &&
        s_wolfSprintOwner == link && link->checkWolfDashMode() &&
        ground_movement_proc(link) && !link->checkPlayerDemoMode() &&
        !link->checkEventRun() && !dComIfGp_event_runCheck();
    s_wolfJumpMomentum.push_back({link, args, carry ? link->speedF : 0.0f, carry});
    return HOOK_CONTINUE;
}

void after_wolf_sprint_jump(ModContext*, void* args, void* retval, void*) {
    if (s_wolfJumpMomentum.empty() || s_wolfJumpMomentum.back().args != args) return;
    const auto saved = s_wolfJumpMomentum.back();
    s_wolfJumpMomentum.pop_back();
    if (!saved.carry || !*static_cast<int*>(retval) ||
        saved.link->mProcID != daAlink_c::PROC_WOLF_AUTO_JUMP) return;
    saved.link->mNormalSpeed = saved.speed;
    saved.link->speedF = saved.speed;
    saved.link->mMaxSpeed = std::max(saved.link->mMaxSpeed, saved.speed);
    // Vertical launch speed and gravity remain native; Jump Height is separate.
}
