// Included in jump_hooks.cpp so all ZR actions share the existing jump safety
// checks and the same manual-jump/Bullet Time ownership.
#include "jump_gale_visual.inc"

constexpr ActorId kNoGlideActor = fpcM_ERROR_PROCESS_ID_e;
// Link executes at 30 simulation ticks per second, regardless of render FPS.
constexpr int kGaleCrouchTicks = 30;
enum class GaleCharge { Idle, Airborne, Ready };
struct JumpAbilities {
    daAlink_c* owner = nullptr;
    ActorId ownerId = kNoGlideActor;
    ActorId cucco = kNoGlideActor;
    GaleCharge charge = GaleCharge::Idle;
    int crouchTicks = 0;
    float launchSpeed = 0.0f;
    s16 launchAngle = 0;
    int loadTicks = 0;
    bool attached = false;
    bool retiring = false;
    bool handled = false;
};
JumpAbilities s_jumpAbilities;

bool owns_jump_abilities(daAlink_c* link) {
    return link && link == s_jumpAbilities.owner && fopAcM_GetID(link) == s_jumpAbilities.ownerId;
}

fopAc_ac_c* glide_actor() {
    fopAc_ac_c* actor = nullptr;
    if (s_jumpAbilities.cucco != kNoGlideActor && !fpcM_IsCreating(s_jumpAbilities.cucco))
        fopAcM_SearchByID(s_jumpAbilities.cucco, &actor);
    return actor && fopAcM_GetName(actor) == fpcNm_NI_e ? actor : nullptr;
}

void retire_glide_actor(daAlink_c* link) {
    clear_glider_visual();
    if (s_jumpAbilities.cucco == kNoGlideActor) return;
    s_jumpAbilities.retiring = true;
    auto* actor = glide_actor();
    if (owns_jump_abilities(link) && actor && link->mGrabItemAcKeep.getActor() == actor) {
        link->freeGrabItem();
        if (link->mProcID == daAlink_c::PROC_AUTO_JUMP) {
            link->mProcVar2.field_0x300c = 0;
            link->resetUpperAnime(daAlink_c::UPPER_1, 3.0f);
            link->setSpecialGravity(link->mpHIO->mAutoJump.m.mGravity,
                link->mpHIO->mAutoJump.m.mMaxFallSpeed, TRUE);
        }
    }
    s_jumpAbilities.attached = false;
    if (actor) {
        fopAcM_offDraw(actor);
        if (!svc_actor || svc_actor->delete_actor(mod_ctx, s_jumpAbilities.cucco) != MOD_OK) return;
    } else if (fpcM_IsCreating(s_jumpAbilities.cucco)) {
        // A landing or scene transition may beat asynchronous archive loading.
        // Cancel that request too; retain its ID if it is currently executing.
        bool cancelled = false;
        for (auto* node = g_fpcCtTg_Queue.mpHead; node; node = NODE_GET_NEXT(node)) {
            auto* request = static_cast<create_request*>(reinterpret_cast<create_tag*>(node)->base.mpTagData);
            if (request->id == s_jumpAbilities.cucco) {
                if (fpcCtRq_IsDoing(request) || !fpcCtRq_Cancel(request)) return;
                cancelled = true;
                break;
            }
        }
        if (!cancelled) return;
    }
    s_jumpAbilities.cucco = kNoGlideActor;
    s_jumpAbilities.retiring = false;
    s_jumpAbilities.loadTicks = 0;
}

bool aerial_glide_context(daAlink_c* link) {
    return link && !link->checkWolf() && !link->mLinkAcch.ChkGroundHit() &&
        (link->mProcID == daAlink_c::PROC_AUTO_JUMP || link->mProcID == daAlink_c::PROC_FALL) &&
        !link->checkEventRun() && !dComIfGp_event_runCheck() && !dComIfGp_isEnableNextStage() &&
        !link->checkMagneBootsOn() && !link->checkBootsOrArmorHeavy() &&
        !link->checkModeFlg(daAlink_c::MODE_SWIMMING);
}

void request_glide(daAlink_c* link) {
    if (s_jumpAbilities.cucco != kNoGlideActor || link->mGrabItemAcKeep.getActor()) return;
    // White, ordinary Cucco; no dialogue flow. The standalone helper assigns
    // Link's scene layer and isolates room-state side effects during creation.
    const ActorSpawnParams params{
        .parameters = 0, .argument = -1, .room_num = fopAcM_GetRoomNo(link),
        .position = {link->current.pos.x, link->current.pos.y + 80, link->current.pos.z},
        .angle = {-1, link->shape_angle.y, 0}, .scale = {1, 1, 1}, .create_function = nullptr,
    };
    ActorId id = kNoGlideActor;
    if (create_standalone_actor(fpcNm_NI_e, params, id) == MOD_OK) {
        s_jumpAbilities.cucco = id;
        s_jumpAbilities.loadTicks = 0;
    }
}

void attach_glide_actor(daAlink_c* link, fopAc_ac_c* actor) {
    const float fallHeight = link->mFallHeight;
    const float verticalSpeed = link->speed.y;
    const float horizontalSpeed = link->mNormalSpeed;
    link->deleteEquipItem(TRUE, FALSE);
    if (link->mProcID != daAlink_c::PROC_AUTO_JUMP)
        link->commonProcInit(daAlink_c::PROC_AUTO_JUMP);
    link->setGrabItemActor(actor);
    link->field_0x33e4 = 38.0f;
    link->setGrabUpperAnime(3.0f);
    // Enter the native Cucco-glide section without calling the launch
    // initializer: deploying a Cucco during a fall must never launch Link up.
    link->mMaxSpeed = link->mpHIO->mAutoJump.m.mCuccoJumpMaxSpeed;
    link->field_0x3478 = link->mpHIO->mAutoJump.m.mCuccoFallMaxSpeed;
    link->mProcVar0.field_0x3008 = 0;
    link->mProcVar2.field_0x300c = 1;
    link->field_0x3198 = 0x33;
    link->field_0x2f99 = 0x20;
    link->offModeFlg(4);
    link->setSingleAnimeBaseSpeed(daAlink_c::ANM_JUMP_LAND, 0, 3.0f);
    link->setUpperAnime(dRes_ID_ALANM_BCK_WALKHBS_e, daAlink_c::UPPER_1, 1, 0, -1, 3);
    link->setSpecialGravity(-1.0f, link->field_0x3478, FALSE);
    link->speed.y = std::max(verticalSpeed, link->field_0x3478);
    link->mNormalSpeed = horizontalSpeed;
    link->mFallHeight = fallHeight;
    actor->current.pos = link->current.pos + cXyz(0, 80, 0);
    actor->old.pos = actor->current.pos;
    link->setGrabItemPos();
    s_jumpAbilities.attached = true;
}

void update_glide(daAlink_c* link) {
    if (s_jumpAbilities.cucco == kNoGlideActor) return;
    auto* actor = glide_actor();
    if (s_jumpAbilities.retiring || !glide_enabled() || !aerial_glide_context(link) ||
        (s_jumpAbilities.attached && (!actor || link->mGrabItemAcKeep.getActor() != actor)) ||
        (!s_jumpAbilities.attached && link->mGrabItemAcKeep.getActor()))
    {
        retire_glide_actor(link);
        return;
    }
    if (!s_jumpAbilities.attached) {
        if (actor) attach_glide_actor(link, actor);
        else if (++s_jumpAbilities.loadTicks > 90 || !fpcM_IsCreating(s_jumpAbilities.cucco))
            retire_glide_actor(link);
    }
}

void cancel_gale_charge(daAlink_c* link, bool restorePose) {
    const bool wasReady = s_jumpAbilities.charge == GaleCharge::Ready;
    s_jumpAbilities.charge = GaleCharge::Idle;
    s_jumpAbilities.crouchTicks = 0;
    s_jumpAbilities.launchSpeed = 0.0f;
    s_jumpAbilities.launchAngle = 0;
    if (restorePose && wasReady && owns_jump_abilities(link) && link->mProcID == daAlink_c::PROC_CROUCH)
        link->procWaitInit();
}

void reset_jump_abilities(daAlink_c* link) {
    cancel_gale_charge(link, true);
    retire_glide_actor(link);
    s_galeVisual.release();
    s_jumpAbilities.owner = nullptr;
    s_jumpAbilities.ownerId = kNoGlideActor;
    s_jumpAbilities.handled = false;
}

bool gale_context_ready(daAlink_c* link) {
    return !link->checkWolf() && !link->checkEventRun() &&
        !dComIfGp_event_runCheck() && !dComIfGp_isEnableNextStage() &&
        !link->checkMagneBootsOn() && !link->checkModeFlg(daAlink_c::MODE_SWIMMING) &&
        !link->mGrabItemAcKeep.getActor() && s_jumpAbilities.cucco == kNoGlideActor &&
        !link->checkPlayerDemoMode() && !link->getSumouMode() && !r_action_context_active(link);
}

bool gale_landing_proc(u16 proc) {
    return proc == daAlink_c::PROC_AUTO_JUMP || proc == daAlink_c::PROC_FALL ||
        proc == daAlink_c::PROC_LAND;
}

void stop_for_gale(daAlink_c* link) {
    link->mNormalSpeed = 0.0f;
    link->speedF = 0.0f;
    link->speed.set(0.0f, 0.0f, 0.0f);
    link->shape_angle.y = s_jumpAbilities.launchAngle;
    link->current.angle.y = s_jumpAbilities.launchAngle;
}

bool launch_gale(daAlink_c* link) {
    if (!gale_charge_available()) {
        cancel_gale_charge(link, true);
        return false;
    }
    const cXyz origin = link->current.pos;
    const float launchSpeed = s_jumpAbilities.launchSpeed;
    const s16 launchAngle = s_jumpAbilities.launchAngle;
    cancel_gale_charge(link, false);
    if (!link->procAutoJumpInit(1)) {
        if (link->mProcID == daAlink_c::PROC_CROUCH) link->procWaitInit();
        return false;
    }
    consume_gale_charge();
    apply_manual_jump_movement(link);
    // Restore the actual ground speed captured BEFORE the first jump resets it.
    // Do not recalculate from the stick or sprint toggle after waiting in crouch.
    link->mNormalSpeed = launchSpeed;
    link->speedF = launchSpeed;
    link->mMaxSpeed = std::max(link->mMaxSpeed, launchSpeed);
    link->shape_angle.y = launchAngle;
    link->current.angle.y = launchAngle;
    apply_manual_jump_height(link, jump_height_multiplier() + gale_height_bonus());
    s_manualJumpOwner = link;
    mark_manual_jump_started(link);
    s_galeVisual.launch(origin);
    return true;
}

bool handle_jump_abilities(daAlink_c* link) {
    if (!owns_jump_abilities(link) || s_jumpAbilities.handled) return false;
    s_jumpAbilities.handled = true;
    if (s_jumpAbilities.charge != GaleCharge::Idle) {
        if (!revalis_gale_enabled() || !gale_context_ready(link)) {
            cancel_gale_charge(link, true);
            return false;
        }
        if (s_jumpAbilities.charge == GaleCharge::Ready) {
            if (link->mProcID != daAlink_c::PROC_CROUCH || !link->mLinkAcch.ChkGroundHit()) {
                cancel_gale_charge(link, true);
                return false;
            }
            // Each call follows a full tick spent crouching, including the
            // release tick. The landing tick and prior airtime do not count.
            if (s_jumpAbilities.crouchTicks < kGaleCrouchTicks)
                ++s_jumpAbilities.crouchTicks;
            if (!jump_held(active_jump_binding())) {
                if (s_jumpAbilities.crouchTicks >= kGaleCrouchTicks) return launch_gale(link);
                cancel_gale_charge(link, true);
                return false;
            }
            stop_for_gale(link);
            return true;
        }
        if (!jump_held(active_jump_binding()) || !gale_landing_proc(link->mProcID)) {
            // Releasing in the first jump only cancels Gale. There is no delayed
            // normal jump and no extra airborne launch.
            cancel_gale_charge(link, false);
            return false;
        }
        if (!link->mLinkAcch.ChkGroundHit()) return false;
        s_jumpAbilities.charge = GaleCharge::Ready;
        s_jumpAbilities.crouchTicks = 0;
        if (!link->procCrouchInit()) {
            cancel_gale_charge(link, false);
            return false;
        }
        stop_for_gale(link);
        s_galeVisual.prepare();
        return true;
    }
    if (!jump_pressed(active_jump_binding())) return false;
    if (revalis_gale_enabled() && gale_context_ready(link) && ground_jump_context_ready(link) &&
        !mDoCPd_c::getHoldB(PAD_1) && !mDoCPd_c::getTrigB(PAD_1))
    {
        const float launchSpeed = std::max(0.0f, link->mNormalSpeed);
        const s16 launchAngle = link->checkInputOnR() ? link->mMoveAngle : link->shape_angle.y;
        if (!start_ground_jump(link)) return false;
        if (link->mProcID == daAlink_c::PROC_AUTO_JUMP && gale_charge_available()) {
            s_jumpAbilities.charge = GaleCharge::Airborne;
            s_jumpAbilities.launchSpeed = launchSpeed;
            s_jumpAbilities.launchAngle = launchAngle;
        }
        return true;
    }
    if (glide_enabled() && aerial_glide_context(link)) {
        request_glide(link);
        return true;
    }
    return false;
}

void jump_abilities_proc_change(daAlink_c* link, daAlink_c::daAlink_PROC nextProc) {
    if (!owns_jump_abilities(link)) return;
    const bool continuingJump = s_jumpAbilities.charge == GaleCharge::Airborne && gale_landing_proc(nextProc);
    const bool enteringCrouch = s_jumpAbilities.charge == GaleCharge::Ready && nextProc == daAlink_c::PROC_CROUCH;
    if (!continuingJump && !enteringCrouch) cancel_gale_charge(link, false);
    if (nextProc != daAlink_c::PROC_AUTO_JUMP && nextProc != daAlink_c::PROC_FALL)
        retire_glide_actor(link);
}

#include "glide_presentation.inc"

HookAction before_jump_abilities_execute(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!owns_jump_abilities(link)) {
        reset_jump_abilities(nullptr);
        s_jumpAbilities.owner = link;
        s_jumpAbilities.ownerId = fopAcM_GetID(link);
    }
    s_jumpAbilities.handled = false;
    if (dComIfGp_isPauseFlag()) {
        cancel_gale_charge(link, true);
        return HOOK_CONTINUE;
    }
    if (dComIfGp_event_runCheck() || dComIfGp_isEnableNextStage() || link->checkWolf()) {
        cancel_gale_charge(link, true);
        retire_glide_actor(link);
        s_galeVisual.ticks = 0;
    } else {
        update_glide(link);
        s_galeVisual.update();
    }
    return HOOK_CONTINUE;
}

void after_jump_abilities_execute(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!owns_jump_abilities(link) || dComIfGp_isPauseFlag()) return;
    if (glide_item() == GlideItem::Glider) {
        if (auto* actor = glide_actor()) static_cast<ni_class*>(actor)->mSound.stopAllSounds(0);
    }
    // Link's ground collision is resolved at the end of execute, before draw.
    if (s_jumpAbilities.attached && !aerial_glide_context(link)) retire_glide_actor(link);
}

void after_jump_abilities_draw(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    clear_glider_visual();
    if (owns_jump_abilities(link)) {
        s_galeVisual.draw(link);
        if (custom_glider_active(link))
            queue_glider_visual(link);
    }
}

HookAction before_jump_abilities_delete(ModContext*, void* args, void*, void*) {
    auto* process = mods::arg<leafdraw_class*>(args, 0);
    if (process && fpcM_GetName(process) == fpcNm_ALINK_e &&
        owns_jump_abilities(static_cast<daAlink_c*>(process)))
    {
        reset_jump_abilities(static_cast<daAlink_c*>(process));
        s_manualJumpOwner = nullptr;
        s_sprintOwner = nullptr;
    }
    return HOOK_CONTINUE;
}
