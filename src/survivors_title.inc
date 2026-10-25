// Keep the native OPENING_SCENE and TITLE actor (logo, Start, NAME_SCENE).
// Only its stage and presentation change. A title is never a started run or
// a loaded save; gameplay, rewards, talent UI and save callbacks stay gated.
bool s_titleReloadPending = false;
unsigned s_titleOpeningDepth = 0;
std::vector<void*> s_titleOpeningCalls;
TitleFlow s_titleFlow;
auto s_titleClock = std::chrono::steady_clock::now();
bool title_model_ready(daAlink_c* link);
ActorId s_titlePoseOwner = kNone;
scene_class* opening_scene() {
    auto* scene = fopScnM_SearchByID(dStage_roomControl_c::getProcID());
    return scene && fpcM_GetName(scene) == fpcNm_OPENING_SCENE_e ? scene : nullptr;
}
void reset_title_presentation() {
    s_titleFlow = {};
    s_titleClock = std::chrono::steady_clock::now();
    s_titlePoseOwner = kNone;
}
void request_title_preview() {
    reset_title_presentation();
    // The carousel can change while the old scene is gone and the next opening
    // is still being created. Keep that selection until an opening request
    // actually consumes it; a missing scene is not an acknowledgement.
    s_titleReloadPending = true;
}
ModResult play_survivors(void*, ModError*) {
    s_titleFlow.play();
    s_titleClock = std::chrono::steady_clock::now();
    return MOD_OK; // Keep the prepared preview actors; Play is not activation.
}
void update_title_preview() {
    if (!s_titleReloadPending || transitioning() || fopOvlpM_IsDoingReq() ||
        mDoRst::isReset() || mDoRst::isReturnToMenu()) return;
    auto* scene = opening_scene();
    // An opening is searchable before its asynchronous creation has finished.
    // Do not replace it in the gap between its fade and actor initialization.
    if (!scene || fpcM_IsCreating(dStage_roomControl_c::getProcID())) return;
    // The native changeOpeningScene helper writes next-stage data BEFORE
    // requesting a scene and returns success even when ChangeReq fails. Reserve
    // the opening first, so a busy fade cannot leave a hub PLAY_SCENE queued.
    if (!fopScnM_ChangeReq(scene,fpcNm_OPENING_SCENE_e,0,30)) return;
    if (s_active) reset_runtime();
    s_titlePoseOwner = kNone;
    dComIfGp_offEnableNextStage();
    ++s_titleOpeningDepth;
    dComIfGp_setNextStage("F_SP102",100,0,10);
    --s_titleOpeningDepth;
    // The accepted OPENING_SCENE request owns these parameters. Prevent the
    // old scene's Draw from independently requesting PLAY_SCENE with them.
    dComIfGp_offEnableNextStage();
    mDoAud_setSceneName(dComIfGp_getNextStageName(),dComIfGp_getNextStageRoomNo(),dComIfGp_getNextStageLayer());
    dComIfGs_setRestartRoomParam(0);
    if (mDoGph_gInf_c::getFader()->getStatus()==1) {
        JUtility::TColor black(0,0,0,255);
        mDoGph_gInf_c::setFadeColor(black);
    }
}
HookAction before_title_opening(ModContext*,void* args,void*,void*) {
    s_titleOpeningCalls.push_back(args);
    ++s_titleOpeningDepth;
    return HOOK_CONTINUE;
}
void after_title_opening(ModContext*,void* args,void*,void*) {
    // Post hooks also run when another mod skipped our pre hook.
    if (s_titleOpeningCalls.empty() || s_titleOpeningCalls.back()!=args) return;
    s_titleOpeningCalls.pop_back();
    --s_titleOpeningDepth;
}
bool survivors_title_active() {
    // Selection changes immediately, but the outgoing hub can still be loading
    // Link/story actors. It remains our title until the scene is replaced.
    if (s_gameStarted ||
        !arena_matches(0,dComIfGp_getStartStageName(),dComIfGp_getStartStageRoomNo())) return false;
    return opening_scene() != nullptr;
}
// Armed by file creation/loading and returning from a run. The gameplay
// scene gate excludes outgoing transitions, title actors and NAME_SCENE.
bool survivors_hub_arrival_active() {
    if (!s_hubArrivalPending || !s_active || !s_gameStarted || !s_hub || s_map != 0 ||
        !arena_matches(0,dComIfGp_getStartStageName(),dComIfGp_getStartStageRoomNo()) ||
        dComIfGp_isEnableNextStage() || mDoRst::isReset() || mDoRst::isReturnToMenu()) return false;
    auto* scene = fopScnM_SearchByID(dStage_roomControl_c::getProcID());
    return scene && fpcM_GetName(scene) == fpcNm_PLAY_SCENE_e;
}
bool survivors_rest_active() { return survivors_title_active() || survivors_hub_arrival_active(); }
float hub_arrival_progress() { return std::clamp((s_hubArrivalSeconds-.35f)/.7f,0.f,1.f); }
void advance_hub_arrival(float seconds,bool ready) {
    if (ready) s_hubArrivalSeconds += std::clamp(seconds,0.f,.1f);
    if (hub_arrival_progress() < 1.f) return;
    s_hubArrivalPending = false;
    // Restore the idle playback frozen by before_title_wait before handing
    // input back. No event status, cutscene flags or global pad state is edited.
    if (auto* link = daAlink_getAlinkActorClass())
        link->setSingleAnime(daAlink_c::ANM_WAIT,1.f,0.f,-1,0.f);
}
bool survivors_title_preparing() {
    // Resource creation can overlap the native fade-in. Never start a second
    // scene's actors while an outgoing stage/reset request is pending.
    return s_active && survivors_title_active() && !s_titleReloadPending && !dComIfGp_isEnableNextStage() &&
        !mDoRst::isReset() && !mDoRst::isReturnToMenu();
}
bool title_floor_anchor(cXyz& anchor) {
    return hub_center_ground(anchor);
}
bool title_scene_ready() {
    auto* link = daAlink_getAlinkActorClass();
    cXyz anchor;
    // Room assets load synchronously with its collider. A valid floor also
    // permits the native-room fallback if custom assets could not be loaded.
    return !s_titleReloadPending && link && s_titlePoseOwner == fopAcM_GetID(link) && title_model_ready(link) &&
        title_floor_anchor(anchor) && owned_hub_zelda(actor_by_id(s_hubZelda));
}
bool title_ui_blocked() {
    return (s_active || survivors_title_active()) && !s_gameStarted && opening_scene() &&
        !(s_titleFlow.show_logo() && survivors_title_active() && title_scene_ready());
}
HookAction before_title_controls(ModContext*, void*, void*, void*) {
    // Gate the input/scene-changing procs as well as drawing. Let loadWait_proc
    // finish native asynchronous UI initialization even behind prelaunch.
    return title_ui_blocked() ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}
HookAction before_title_logo_animation(ModContext*,void* args,void*,void*) {
    if (!s_active || !survivors_title_active() || title_ui_blocked() ||
        ui_visible() || transitioning() || mDoCPd_c::getTrig(PAD_1)==0) return HOOK_CONTINUE;
    finish_survivors_title_logo(mods::arg<void*>(args,0));
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_title_ui(ModContext*, void*, void* result, void*) {
    if (!title_ui_blocked()) return HOOK_CONTINUE;
    *static_cast<int*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_title_player_draw(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args,0);
    if (!survivors_rest_active() || link != daAlink_getAlinkActorClass() || title_scene_ready()) return HOOK_CONTINUE;
    *static_cast<int*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_title_fall(ModContext*, void* args, void* result, void*) {
    if (!survivors_rest_active() || mods::arg<daAlink_c*>(args,0) != daAlink_getAlinkActorClass()) return HOOK_CONTINUE;
    // checkAutoJumpAction runs AFTER setStickData and otherwise replaces WAIT
    // with a fall while initial ground collision is still being established.
    *static_cast<BOOL*>(result) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_title_stage(ModContext*, void* args, void*, void*) {
    if (!s_titleOpeningDepth) {
        // Native title-demo events also call setNextStage (sometimes with the
        // exact same F_SP102 tuple), but their Draw creates PLAY_SCENE. They
        // cannot drive a selected/pending Survivors preview or an outgoing hub.
        if (!s_gameStarted && opening_scene() &&
            (s_active || s_titleReloadPending || survivors_title_active())) {
            return HOOK_SKIP_ORIGINAL;
        }
        return HOOK_CONTINUE;
    }
    const auto* stage = mods::arg<const char*>(args,0);
    // Exact native opening request, including reset-to-title. Do not redirect
    // file selection, new-game setup or an actual gameplay transition.
    if (s_gameStarted || !stage || std::strcmp(stage,"F_SP102") ||
        mods::arg<s16>(args,1) != 100 || mods::arg<s8>(args,2) != 0 ||
        mods::arg<s8>(args,3) != 10) return HOOK_CONTINUE;
    // This native opening request picks up the latest carousel selection,
    // including startup. Later changes will arm another refresh independently.
    s_titleReloadPending = false;
    if (!s_active) return HOOK_CONTINUE;
    mods::arg_ref<const char*>(args,0) = arenas[0].stage;
    mods::arg_ref<s16>(args,1) = arenas[0].point;
    mods::arg_ref<s8>(args,2) = arenas[0].room;
    mods::arg_ref<s8>(args,3) = 0;
    return HOOK_CONTINUE;
}
HookAction before_title_start_demo(ModContext*,void*,void* result,void*) {
    if (!survivors_rest_active()) return HOOK_CONTINUE;
    // Link calls orderStartDemo even with idle spawn mode and event 0xff.
    // The native fallback then orders this room's DEFAULT_START. The custom
    // title/hub arrival owns its pose and must not start a dungeon entrance event.
    *static_cast<int*>(result)=-2; // Native "no start demo" result.
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_title_stage_actor(ModContext*, void* args, void*, void*) {
    if (!survivors_rest_active()) return HOOK_CONTINUE;
    const auto* record = mods::arg<stage_actor_data_class*>(args,0);
    auto* params = mods::arg<fopAcM_prm_class*>(args,1);
    if (!record || !params) return HOOK_CONTINUE;
    char name[9]{}; std::memcpy(name,record->name,8);
    const auto* info = dStage_searchName(name);
    if (!info) return HOOK_CONTINUE;
    if (info->procname == fpcNm_ALINK_e) {
        // Idle spawn 16, no entrance event; the ground is probed once loaded.
        params->base.parameters = (params->base.parameters & 0x00fe0fffu) | 0xff010000u;
        params->base.position.x = 0;
        params->base.position.y = roundtable_layout::floorY;
        params->base.position.z = roundtable_layout::restZ;
        params->base.angle.y = static_cast<s16>(0x8000);
    } else if (survivors_title_active() && (arena_story_actor(info->procname) ||
               info->procname == fpcNm_SCENE_EXIT_e || info->procname == fpcNm_SCENE_EXIT2_e)) {
        JKRFree(params);
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
void after_title_input(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args,0);
    if (!survivors_rest_active() || !link || link != daAlink_getAlinkActorClass()) return;
    // Clear Link's private input, never the pad read by the title's A/Start.
    link->mItemButton = link->mItemTrigger = 0;
    link->mStickValue = link->mMoveValue = 0;
    link->mNormalSpeed = link->speedF = 0;
    link->speed.set(0,0,0);
    if (link->mProcID != daAlink_c::PROC_WAIT) link->procWaitInit();
}
HookAction before_title_wait(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args,0);
    if (!survivors_rest_active() || !link || link != daAlink_getAlinkActorClass()) return HOOK_CONTINUE;
    cXyz anchor;
    s_titlePoseOwner = kNone;
    if (title_floor_anchor(anchor)) {
        cXyz position = anchor + cXyz(0,0,roundtable_layout::restZ);
        cXyz probe = position + cXyz(0,100,0);
        dBgS_ObjGndChk ground; ground.SetPos(&probe);
        const float height = dComIfG_Bgsp().GroundCross(&ground);
        if (std::isfinite(height) && height > -1.e8f && std::abs(height-anchor.y) <= 50) {
            position.y = height;
            link->setPlayerPosAndAngle(&position,static_cast<s16>(0x8000),true);
            s_titlePoseOwner = fopAcM_GetID(link);
        }
    }
    // The title model overlay authors the one-knee rest pose from idle.
    // Keep native chest-opening and item-get procedures out of this scene.
    if (!link->checkUnderMove0BckNoArc(daAlink_c::ANM_WAIT))
        link->setSingleAnime(daAlink_c::ANM_WAIT,0.f,0.f,-1,0.f);
    for (auto* ctrl : {&link->mUnderFrameCtrl[0],&link->mUpperFrameCtrl[0]}) {
        ctrl->setFrame(0.f); ctrl->setRate(0.f);
    }
    *static_cast<int*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
}
void after_title_camera(ModContext*, void* args, void*, void*) {
    auto* camera = mods::arg<dCamera_c*>(args,0);
    cXyz anchor;
    if (!survivors_rest_active() || !camera || !title_floor_anchor(anchor)) return;
    const auto now = std::chrono::steady_clock::now();
    const float seconds = std::chrono::duration<float>(now-s_titleClock).count();
    s_titleClock = now;
    if (survivors_hub_arrival_active()) {
        // Keep the rest pose visible through fade-in and decoration loading,
        // then show the rise from the same side as the title's final view.
        camera->Set(anchor+cXyz(0,170,350),anchor+cXyz(640,250,roundtable_layout::restZ+90),48.f,s16(0));
        camera->mUp.set(0,1,0);
        camera->mDirection.Val(camera->mEye-camera->mCenter);
        advance_hub_arrival(seconds,title_scene_ready() && !transitioning() && !ui_visible() &&
            !dComIfGp_isPauseFlag() && !dScnPly_c::isPause());
        return;
    }
    // Native pad input includes keyboard bindings and virtual touch buttons.
    const bool held = mDoCPd_c::getHold(PAD_1) != 0;
    const bool pressed = mDoCPd_c::getTrig(PAD_1) != 0;
    s_titleFlow.tick(seconds, title_scene_ready() && !transitioning(), ui_visible(), held, pressed);
    // One eased arc: reduce distance/height/FOV while rotating to Link's side.
    // Logo input does not stop or restart this movement.
    const float t = s_titleFlow.camera();
    const float startAngle = std::atan2(700.f,490.f);
    const float angle = startAngle+(std::atan2(640.f,90.f)-startAngle)*t;
    const float startRadius = std::sqrt(700.f*700.f+490.f*490.f);
    const float endRadius = std::sqrt(640.f*640.f+90.f*90.f);
    const float radius = startRadius+(endRadius-startRadius)*t;
    const cXyz eye(radius*std::sin(angle),400-150*t,roundtable_layout::restZ+radius*std::cos(angle));
    camera->Set(anchor+cXyz(-80+80*t,190-20*t,450-100*t),
                anchor+eye,52.f-4.f*t,s16(0));
    camera->mUp.set(0,1,0);
    camera->mDirection.Val(camera->mEye-camera->mCenter);
}
HookAction before_title_logo(ModContext*, void*, void* result, void*) {
    if (!survivors_title_active()) return HOOK_CONTINUE;
    // Replace only the missing STB cue. Native A/Start fast-forward, logo
    // animation, keyWait and NAME_SCENE transitions still run unchanged.
    *static_cast<int*>(result) = s_titleFlow.show_logo() && title_scene_ready() ? 1 : -1;
    return HOOK_SKIP_ORIGINAL;
}
