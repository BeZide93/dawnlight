// Included in jump_hooks.cpp so all ZR actions share the existing jump safety
// checks and the same manual-jump/Bullet Time ownership.
#include "jump_gale_visual.inc"

constexpr ActorId kNoGlideActor = fpcM_ERROR_PROCESS_ID_e;
enum class GaleCharge { Idle, Holding, Ready };
struct JumpAbilities {
    daAlink_c* owner = nullptr;
    ActorId ownerId = kNoGlideActor;
    ActorId cucco = kNoGlideActor;
    GaleCharge charge = GaleCharge::Idle;
    int chargeTicks = 0;
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
    s_jumpAbilities.chargeTicks = 0;
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
    s_deferredJumpTrigger = false;
}

bool gale_standing_context(daAlink_c* link) {
    const bool crouched = s_jumpAbilities.charge == GaleCharge::Ready &&
        link->mProcID == daAlink_c::PROC_CROUCH;
    return !link->checkInputOnR() && std::fabs(link->mNormalSpeed) < 0.1f &&
        !link->checkWolf() && link->mLinkAcch.ChkGroundHit() &&
        !link->checkEventRun() && !dComIfGp_event_runCheck() && !dComIfGp_isEnableNextStage() &&
        !link->checkMagneBootsOn() && !link->mGrabItemAcKeep.getActor() &&
        !link->checkPlayerDemoMode() && !link->getSumouMode() && !r_action_context_active(link) &&
        ((link->mProcID == daAlink_c::PROC_WAIT && ground_jump_context_ready(link)) || crouched);
}

bool launch_gale(daAlink_c* link) {
    const cXyz origin = link->current.pos;
    cancel_gale_charge(link, false);
    if (!link->procAutoJumpInit(1)) {
        if (link->mProcID == daAlink_c::PROC_CROUCH) link->procWaitInit();
        return false;
    }
    apply_manual_jump_movement(link);
    apply_manual_jump_height(link, 4.0f * jump_height_multiplier());
    s_manualJumpOwner = link;
    mark_manual_jump_started(link);
    s_galeVisual.launch(origin);
    return true;
}

bool handle_jump_abilities(daAlink_c* link) {
    if (!owns_jump_abilities(link) || s_jumpAbilities.handled) return false;
    s_jumpAbilities.handled = true;
    if (s_jumpAbilities.charge != GaleCharge::Idle) {
        if (!revalis_gale_enabled() || !gale_standing_context(link)) {
            cancel_gale_charge(link, true);
            return false;
        }
        if (!jump_held(active_jump_binding())) {
            if (s_jumpAbilities.charge == GaleCharge::Ready) return launch_gale(link);
            cancel_gale_charge(link, false);
            // Only stationary presses are deferred. Keep the normal jump's
            // checks, direction, height and sprint/Bullet Time handling.
            s_deferredJumpTrigger = true;
            const bool jumped = start_ground_jump(link);
            s_deferredJumpTrigger = false;
            return jumped;
        }
        if (s_jumpAbilities.charge == GaleCharge::Holding && ++s_jumpAbilities.chargeTicks >= 60) {
            s_jumpAbilities.charge = GaleCharge::Ready;
            if (!link->procCrouchInit()) cancel_gale_charge(link, false);
            else s_galeVisual.prepare();
        }
        return true;
    }
    if (!jump_pressed(active_jump_binding())) return false;
    if (revalis_gale_enabled() && gale_standing_context(link) &&
        !mDoCPd_c::getHoldB(PAD_1) && !mDoCPd_c::getTrigB(PAD_1))
    {
        s_jumpAbilities.charge = GaleCharge::Holding;
        s_jumpAbilities.chargeTicks = 0;
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
    if (nextProc != daAlink_c::PROC_WAIT && nextProc != daAlink_c::PROC_CROUCH)
        cancel_gale_charge(link, false);
    if (nextProc != daAlink_c::PROC_AUTO_JUMP && nextProc != daAlink_c::PROC_FALL)
        retire_glide_actor(link);
}

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
    // Link's ground collision is resolved at the end of execute, before draw.
    if (s_jumpAbilities.attached && !aerial_glide_context(link)) retire_glide_actor(link);
}

void after_jump_abilities_draw(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (owns_jump_abilities(link)) s_galeVisual.draw(link);
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
