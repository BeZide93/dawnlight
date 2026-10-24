// Apply the native cutscene visibility rules inside the UI only. Never change
// dEvt_control_c: even a temporary running event can be observed by other hooks.
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui12_GLOBAL__N_124game_controls_suppressedEv",
    bool(), TitleTouchGameplaySuppressed);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui13TouchControls21sync_action_bar_stateEv",
    void(NativeTouch*), TitleTouchActionBar);
HookAction before_title_gameplay_controls(ModContext*,void*,void* result,void*) {
    if (!survivors_title_presentation_active()) return HOOK_CONTINUE;
    *static_cast<bool*>(result)=true;
    return HOOK_SKIP_ORIGINAL;
}
// This flag belongs to the touch widget, not the game. Suppress only its
// action-bar update (including Skip); native face-button updates still own A/B.
struct TitleTouchBarState { NativeTouch* controls=nullptr; bool suppressed=false; };
std::vector<TitleTouchBarState> s_titleTouchBars;
HookAction before_title_action_bar(ModContext*,void* args,void*,void*) {
    auto* controls=mods::arg<NativeTouch*>(args,0);
    const bool title=controls && survivors_title_presentation_active();
    s_titleTouchBars.push_back(title ? TitleTouchBarState{controls,controls->mWasSuppressed} : TitleTouchBarState{});
    if (title) controls->mWasSuppressed=true;
    return HOOK_CONTINUE;
}
void after_title_action_bar(ModContext*,void*,void*,void*) {
    if (s_titleTouchBars.empty()) return;
    const auto saved=s_titleTouchBars.back();
    s_titleTouchBars.pop_back();
    if (saved.controls) saved.controls->mWasSuppressed=saved.suppressed;
}
HookAction before_title_touch(ModContext*,void* args,void*,void*) {
    auto* controls=mods::arg<NativeTouch*>(args,0);
    if (controls && survivors_title_presentation_active()) {
        // Cancel old gameplay holds before native long-press and pad handling.
        // A/B retain their native touch state for the logo/Start flow.
        for (size_t i=0;i<controls->mControlTouches.size();++i) {
            const auto control=static_cast<dusk::ui::Control>(i);
            if (control!=dusk::ui::Control::A && control!=dusk::ui::Control::B)
                controls->mControlTouches[i]={};
        }
        controls->mButtonMask &= PAD_BUTTON_A | PAD_BUTTON_B;
        controls->mLPressed=controls->mLLatched=controls->mManualLLatched=false;
        controls->mLReleasePending=controls->mRTriggerHeld=false;
        controls->mLPressStartTime=controls->mLastLTapTime={};
        controls->mQueuedActions.reset();
        controls->mMoveTouch={};
        controls->mCameraTouch={};
    }
    return HOOK_CONTINUE;
}
ModResult initialize_title_touch(ModError* error) {
    HookOptions before=HOOK_OPTIONS_INIT, after=HOOK_OPTIONS_INIT;
    before.priority=INT32_MAX;
    after.priority=INT32_MIN;
    auto result=mods::hook_add_pre<TitleTouchGameplaySuppressed>(svc_hook,before_title_gameplay_controls,&before);
    if (result==MOD_OK) result=mods::hook_add_pre<TitleTouchActionBar>(svc_hook,before_title_action_bar,&before);
    if (result==MOD_OK) result=mods::hook_add_post<TitleTouchActionBar>(svc_hook,after_title_action_bar,&after);
    if (result==MOD_OK) result=mods::hook_add_pre<ExtraTouchUpdate>(svc_hook,before_title_touch,&before);
    if (result==MOD_OK) result=mods::hook_add_pre<ExtraTouchSync>(svc_hook,before_title_touch,&before);
    return result==MOD_OK ? MOD_OK : mods::set_error(error,result,"failed to initialize Survivors title touch visibility");
}
void shutdown_title_touch() {
    while (!s_titleTouchBars.empty()) after_title_action_bar(nullptr,nullptr,nullptr,nullptr);
}
