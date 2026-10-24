// Separate from Dawnlight's optional extra-button setting: the title needs
// native A/B controls even when the gameplay touch extensions are disabled.
struct TitleTouchApi {
    void (*setClass)(Rml::Element*,const Rml::String*,bool)=nullptr;
    void (*release)(NativeTouch*,dusk::ui::Control)=nullptr;
    void (*clearMotion)(NativeTouch*)=nullptr;
} s_titleTouchApi;
UiStyleHandle s_titleTouchStyle=0;
NativeTouch* s_titleTouchOwner=nullptr;
constexpr char kTitleTouchStyle[] = R"RCSS(
#root.dawnlight-survivors-title > :not(#button-a):not(#button-b) {
    display: none;
}
#root.dawnlight-survivors-title > :not(#button-a):not(#button-b),
#root.dawnlight-survivors-title > :not(#button-a):not(#button-b) * {
    visibility: hidden;
    pointer-events: none;
}
#root.dawnlight-survivors-title #button-b-icon {
    display: none;
}
#root.dawnlight-survivors-title #button-b > span {
    display: block;
    position: static;
    font-size: inherit;
    color: inherit;
}
)RCSS";
void update_title_touch(NativeTouch* controls) {
    if (!s_titleTouchStyle || !controls || !controls->mRoot) return;
    s_titleTouchOwner=controls;
    const bool title=survivors_title_presentation_active();
    const Rml::String name="dawnlight-survivors-title";
    s_titleTouchApi.setClass(controls->mRoot,&name,title);
    if (!title) return;
    // Release hidden holds before native long-press/virtual-pad processing.
    // The stylesheet also covers mod-added child containers, independent of
    // their update order. Removing the class restores their normal styles.
    for (size_t i=0;i<controls->mControlElements.size();++i) {
        const auto control=static_cast<dusk::ui::Control>(i);
        if (control!=dusk::ui::Control::A && control!=dusk::ui::Control::B)
            s_titleTouchApi.release(controls,control);
    }
    controls->mQueuedActions.reset();
    s_titleTouchApi.clearMotion(controls);
}
HookAction before_title_touch(ModContext*,void* args,void*,void*) {
    update_title_touch(mods::arg<NativeTouch*>(args,0));
    return HOOK_CONTINUE;
}
HookAction before_title_touch_delete(ModContext*,void* args,void*,void*) {
    if (s_titleTouchOwner==mods::arg<NativeTouch*>(args,0)) s_titleTouchOwner=nullptr;
    return HOOK_CONTINUE;
}
ModResult initialize_title_touch(ModError* error) {
    if (!resolve_extra("_ZN3Rml7Element8SetClassERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEb",s_titleTouchApi.setClass) ||
        !resolve_extra("dusk::ui::TouchControls::release_control",s_titleTouchApi.release) ||
        !resolve_extra("dusk::ui::TouchControls::clear_motion_touch_input",s_titleTouchApi.clearMotion)) return MOD_OK;
    auto result=mods::hook_add_pre<ExtraTouchUpdate>(svc_hook,before_title_touch);
    if (result==MOD_OK) result=mods::hook_add_pre<ExtraTouchSync>(svc_hook,before_title_touch);
    if (result==MOD_OK) result=mods::hook_add_pre<ExtraTouchDelete>(svc_hook,before_title_touch_delete);
    if (result==MOD_OK) result=svc_ui->register_styles(mod_ctx,UI_SCOPE_TOUCH_CONTROLS,kTitleTouchStyle,&s_titleTouchStyle);
    return result==MOD_OK ? MOD_OK : mods::set_error(error,result,"failed to initialize Survivors title touch controls");
}
void shutdown_title_touch() {
    if (s_titleTouchOwner && s_titleTouchOwner->mRoot) {
        const Rml::String name="dawnlight-survivors-title";
        s_titleTouchApi.setClass(s_titleTouchOwner->mRoot,&name,false);
    }
    s_titleTouchOwner=nullptr;
    if (s_titleTouchStyle) svc_ui->unregister_styles(mod_ctx,s_titleTouchStyle);
    s_titleTouchStyle=0;
}
