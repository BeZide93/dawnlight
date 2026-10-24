// Keep the native OPENING_SCENE and TITLE actor (logo, Start, NAME_SCENE).
// Only its stage and presentation change. A title is never a started run or
// a loaded save; gameplay, rewards, talent UI and save callbacks stay gated.
bool s_titleLaunched = false, s_titleReloadPending = false;
ActorId s_titlePoseOwner = kNone;
scene_class* opening_scene() {
    auto* scene = fopScnM_SearchByID(dStage_roomControl_c::getProcID());
    return scene && fpcM_GetName(scene) == fpcNm_OPENING_SCENE_e ? scene : nullptr;
}
void reset_title_presentation() {
    s_titleLaunched = false;
    s_titlePoseOwner = kNone;
}
void request_title_preview() {
    reset_title_presentation();
    // Carousel selection changes callbacks/save namespace, NOT the scene.
    // Startup's LOGO scene already requests an opening; only reload an existing
    // opening, and retain the request across rapid changes during its fade.
    s_titleReloadPending = s_titleReloadPending || opening_scene() != nullptr;
}
ModResult play_survivors(void*, ModError*) {
    s_titleLaunched = true;
    return MOD_OK; // Keep the prepared preview actors; Play is not activation.
}
void update_title_preview() {
    if (!s_titleReloadPending || transitioning() || mDoRst::isReset() || mDoRst::isReturnToMenu()) return;
    auto* scene = opening_scene();
    if (!scene) return;
    s_titleReloadPending = false;
    s_titlePoseOwner = kNone;
    // Preserve the host menu. The normal opening hook chooses hub or vanilla
    // from the final selected mode, also when Survivors has been deselected.
    dComIfG_changeOpeningScene(scene,fpcNm_OPENING_SCENE_e);
}
bool survivors_title_active() {
    if (!s_active || s_gameStarted ||
        !arena_matches(0,dComIfGp_getStartStageName(),dComIfGp_getStartStageRoomNo())) return false;
    return opening_scene() != nullptr;
}
bool title_floor_anchor(cXyz& anchor) {
    if (hub_brazier_anchor(anchor)) return true;
    auto* link = daAlink_getAlinkActorClass();
    if (!link) return false;
    cXyz probe(0,link->current.pos.y+1000,0);
    dBgS_ObjGndChk ground; ground.SetPos(&probe);
    const float height = dComIfG_Bgsp().GroundCross(&ground);
    if (!std::isfinite(height) || height <= -1.e8f) return false;
    anchor.set(0,height,0);
    return true;
}
bool title_scene_ready() {
    auto* link = daAlink_getAlinkActorClass();
    cXyz anchor;
    return !s_titleReloadPending && link && s_titlePoseOwner == fopAcM_GetID(link) &&
        hub_brazier_anchor(anchor) && owned_hub_zelda(actor_by_id(s_hubZelda));
}
bool title_ui_blocked() {
    return s_active && !s_gameStarted && opening_scene() &&
        !(s_titleLaunched && survivors_title_active() && title_scene_ready());
}
HookAction before_title_controls(ModContext*, void*, void*, void*) {
    // Gate the input/scene-changing procs as well as drawing. Let loadWait_proc
    // finish native asynchronous UI initialization even behind prelaunch.
    return title_ui_blocked() ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}
HookAction before_title_ui(ModContext*, void*, void* result, void*) {
    if (!title_ui_blocked()) return HOOK_CONTINUE;
    *static_cast<int*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_title_player_draw(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args,0);
    if (!survivors_title_active() || link != daAlink_getAlinkActorClass() || title_scene_ready()) return HOOK_CONTINUE;
    *static_cast<int*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_title_fall(ModContext*, void* args, void* result, void*) {
    if (!survivors_title_active() || mods::arg<daAlink_c*>(args,0) != daAlink_getAlinkActorClass()) return HOOK_CONTINUE;
    // checkAutoJumpAction runs AFTER setStickData and otherwise replaces WAIT
    // with a fall while initial ground collision is still being established.
    *static_cast<BOOL*>(result) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_title_stage(ModContext*, void* args, void*, void*) {
    const auto* stage = mods::arg<const char*>(args,0);
    // Exact native opening request, including reset-to-title. Do not redirect
    // file selection, new-game setup or an actual gameplay transition.
    if (!s_active || s_gameStarted || !stage || std::strcmp(stage,"F_SP102") ||
        mods::arg<s16>(args,1) != 100 || mods::arg<s8>(args,2) != 0 ||
        mods::arg<s8>(args,3) != 10) return HOOK_CONTINUE;
    mods::arg_ref<const char*>(args,0) = arenas[0].stage;
    mods::arg_ref<s16>(args,1) = arenas[0].point;
    mods::arg_ref<s8>(args,2) = arenas[0].room;
    mods::arg_ref<s8>(args,3) = 0;
    return HOOK_CONTINUE;
}
HookAction before_title_stage_actor(ModContext*, void* args, void*, void*) {
    if (!survivors_title_active()) return HOOK_CONTINUE;
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
        params->base.position.z = 260;
        params->base.angle.y = static_cast<s16>(0x8000);
    } else if (arena_story_actor(info->procname) || info->procname == fpcNm_SCENE_EXIT_e ||
               info->procname == fpcNm_SCENE_EXIT2_e) {
        JKRFree(params);
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
void after_title_input(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args,0);
    if (!survivors_title_active() || !link || link != daAlink_getAlinkActorClass()) return;
    // Clear Link's private input, never the pad read by the title's A/Start.
    link->mItemButton = link->mItemTrigger = 0;
    link->mStickValue = link->mMoveValue = 0;
    link->mNormalSpeed = link->speedF = 0;
    link->speed.set(0,0,0);
    if (link->mProcID != daAlink_c::PROC_WAIT) link->procWaitInit();
}
HookAction before_title_wait(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args,0);
    if (!survivors_title_active() || !link || link != daAlink_getAlinkActorClass()) return HOOK_CONTINUE;
    cXyz anchor;
    s_titlePoseOwner = kNone;
    if (title_floor_anchor(anchor)) {
        cXyz position = anchor + cXyz(0,0,260);
        cXyz probe = position + cXyz(0,100,0);
        dBgS_ObjGndChk ground; ground.SetPos(&probe);
        const float height = dComIfG_Bgsp().GroundCross(&ground);
        if (std::isfinite(height) && height > -1.e8f && std::abs(height-anchor.y) <= 50) {
            position.y = height;
            link->setPlayerPosAndAngle(&position,static_cast<s16>(0x8000),true);
            s_titlePoseOwner = fopAcM_GetID(link);
        }
    }
    // Native GET_READY_SIT enters TRES_OPEN_SMALL at frame 36, already kneeling.
    // Hold that pose instead of playing chest-opening or item-get event logic.
    if (!link->checkUnderMove0BckNoArc(daAlink_c::ANM_TRES_OPEN_SMALL))
        link->setSingleAnime(daAlink_c::ANM_TRES_OPEN_SMALL,0.f,36.f,-1,0.f);
    for (auto* ctrl : {&link->mUnderFrameCtrl[0],&link->mUpperFrameCtrl[0]}) {
        ctrl->setFrame(36.f); ctrl->setRate(0.f);
    }
    *static_cast<int*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
}
void after_title_camera(ModContext*, void* args, void*, void*) {
    auto* camera = mods::arg<dCamera_c*>(args,0);
    cXyz anchor;
    if (!survivors_title_active() || !camera || !title_floor_anchor(anchor)) return;
    // Three-quarter view: Link in front, bowl in the middle, Zelda to its left.
    // Native Run still maintains viewport, interpolation, audio and room state.
    camera->Set(anchor+cXyz(-70,110,80),anchor+cXyz(650,330,900),45.f,s16(0));
    camera->mUp.set(0,1,0);
    camera->mDirection.Val(camera->mEye-camera->mCenter);
}
HookAction before_title_logo(ModContext*, void*, void* result, void*) {
    if (!survivors_title_active()) return HOOK_CONTINUE;
    // Replace only the missing STB cue. Native A/Start fast-forward, logo
    // animation, keyWait and NAME_SCENE transitions still run unchanged.
    *static_cast<int*>(result) = s_titleLaunched && title_scene_ready() ? 1 : -1;
    return HOOK_SKIP_ORIGINAL;
}
