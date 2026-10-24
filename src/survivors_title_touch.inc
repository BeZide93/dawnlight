// Present the custom title to the host's native touch controller as a running,
// non-skippable cutscene. Keep this status scoped to UI/input evaluation: the
// title has no event-manager script, and the game must not sequence a fake one.
struct TitleTouchEvent {
    dEvt_control_c* event=nullptr;
    u8 status=0;
    dEvt_SkipCb skip=nullptr;
};
std::vector<TitleTouchEvent> s_titleTouchEvents;
HookAction before_title_touch(ModContext*,void* args,void*,void*) {
    auto* controls=mods::arg<NativeTouch*>(args,0);
    auto* event=dComIfGp_getEvent();
    const bool title=controls && event && survivors_title_presentation_active();
    s_titleTouchEvents.push_back(title ? TitleTouchEvent{event,event->mEventStatus,event->mSkipFunc} : TitleTouchEvent{});
    if (title) {
        event->mEventStatus=1;
        event->mSkipFunc=nullptr;
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
void after_title_touch(ModContext*,void*,void*,void*) {
    if (s_titleTouchEvents.empty()) return;
    const auto saved=s_titleTouchEvents.back();
    s_titleTouchEvents.pop_back();
    if (saved.event) {
        saved.event->mEventStatus=saved.status;
        saved.event->mSkipFunc=saved.skip;
    }
}
ModResult initialize_title_touch(ModError* error) {
    // Enclose other mods' touch callbacks as well as the native implementation.
    // No RmlUi methods, stylesheet or optional private helper resolution needed.
    HookOptions before=HOOK_OPTIONS_INIT, after=HOOK_OPTIONS_INIT;
    before.priority=INT32_MAX;
    after.priority=INT32_MIN;
    auto result=mods::hook_add_pre<ExtraTouchUpdate>(svc_hook,before_title_touch,&before);
    if (result==MOD_OK) result=mods::hook_add_post<ExtraTouchUpdate>(svc_hook,after_title_touch,&after);
    if (result==MOD_OK) result=mods::hook_add_pre<ExtraTouchSync>(svc_hook,before_title_touch,&before);
    if (result==MOD_OK) result=mods::hook_add_post<ExtraTouchSync>(svc_hook,after_title_touch,&after);
    return result==MOD_OK ? MOD_OK : mods::set_error(error,result,"failed to initialize Survivors title cutscene touch controls");
}
void shutdown_title_touch() {
    while (!s_titleTouchEvents.empty()) after_title_touch(nullptr,nullptr,nullptr,nullptr);
}
