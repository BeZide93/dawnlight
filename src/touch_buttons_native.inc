using NativeTouch = dusk::ui::TouchControls;
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui13TouchControls6updateEv", void(NativeTouch*), ExtraTouchUpdate);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui13TouchControls18sync_virtual_inputEv", void(NativeTouch*), ExtraTouchSync);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui13TouchControls19clear_control_inputEv", void(NativeTouch*), ExtraTouchClear);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui13TouchControls17handle_touch_downERN3Rml5EventE", void(NativeTouch*, Rml::Event*), ExtraTouchDown);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui13TouchControls19handle_touch_motionERN3Rml5EventE", void(NativeTouch*, Rml::Event*), ExtraTouchMotion);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui13TouchControls15handle_touch_upERN3Rml5EventE", void(NativeTouch*, Rml::Event*), ExtraTouchUp);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui13TouchControls19handle_touch_cancelERN3Rml5EventE", void(NativeTouch*, Rml::Event*), ExtraTouchCancel);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui13TouchControlsD2Ev", void(NativeTouch*), ExtraTouchDelete);
DEFINE_HOOK(&PADSetVirtualStatus, ExtraTouchSetPad);
DEFINE_HOOK(&PADClearVirtualStatus, ExtraTouchClearPad);
DEFINE_HOOK(&PADClearAllVirtualStatus, ExtraTouchClearAllPads);

struct TouchApi {
    Rml::ElementPtr (*create)(Rml::ElementDocument*, const Rml::String*) = nullptr;
    Rml::Element* (*append)(Rml::Element*, Rml::ElementPtr, bool) = nullptr;
    Rml::ElementPtr (*remove)(Rml::Element*, Rml::Element*) = nullptr;
    void (*setRml)(Rml::Element*, const Rml::String*) = nullptr;
    bool (*setProperty)(Rml::Element*, const Rml::String*, const Rml::String*) = nullptr;
    Rml::Element* (*child)(const Rml::Element*, int) = nullptr;
    Rml::Element* (*parent)(const Rml::Element*) = nullptr;
    Rml::Element* (*target)(const Rml::Event*) = nullptr;
    void (*stop)(Rml::Event*) = nullptr;
    SDL_FingerID (*finger)(const Rml::Event*) = nullptr;
    dusk::ui::ControlLayoutSize (*dimensions)(Rml::Context*) = nullptr;
    float (*scale)(Rml::Context*) = nullptr;
    bool (*visible)(const dusk::ui::Document*) = nullptr;
    dusk::config::ConfigVarBase* (*config)(std::string_view) = nullptr;
} s_touchApi;
NativeTouch* s_touchOwner = nullptr;
Rml::Element* s_extraRoot = nullptr;
std::array<Rml::Element*, touch::Count> s_extraElements{};
std::array<std::string, touch::Count> s_extraStyles{};
touch::Presses s_extraPresses;
bool s_extraSessionEnabled = false;
bool s_extraHooksReady = false;
PADStatus s_extraMergedPad{};
PADStatus s_extraBasePad{};
bool s_extraPadOwned = false;

void extra_property(Rml::Element* element, const char* name, const std::string& value) {
    const std::string property = name;
    s_touchApi.setProperty(element, &property, &value);
}
bool extra_controls_available(NativeTouch* controls) {
    if (!s_extraHooksReady || !s_extraSessionEnabled || controls == nullptr || controls->mWasSuppressed ||
        !s_touchApi.visible(controls)) return false;
    const auto* config = s_touchApi.config("game.enableTouchControls");
    if (config == nullptr || !static_cast<const dusk::config::ConfigVar<bool>*>(config)->getValue()) return false;
    return dComIfGp_getLinkPlayer() != nullptr && !dComIfGp_event_runCheck() &&
        !dComIfGp_isPauseFlag() && !dComIfGp_isEnableNextStage() &&
        dMeter2Info_getWindowStatus() == 0 &&
        !(dComIfGp_getMsgObjectClass() != nullptr && dMsgObject_isTalkNowCheck());
}
void prune_extra_presses(NativeTouch* controls) {
    if (!extra_controls_available(controls)) s_extraPresses.clear();
    else for (size_t i = 0; i < touch::Count; ++i) if (!button_enabled(i)) s_extraPresses.disable(i);
}
void remove_extra_elements() {
    // Called before TouchControls destruction, while its document is still live.
    if (s_extraRoot && s_touchOwner && s_touchOwner->mRoot) {
        auto removed = s_touchApi.remove(s_touchOwner->mRoot, s_extraRoot);
    }
    s_extraRoot = nullptr;
    s_touchOwner = nullptr;
    s_extraElements = {};
    s_extraStyles = {};
    s_extraPresses.clear();
}
bool create_extra_elements(NativeTouch* controls) {
    if (controls->mRoot == nullptr || controls->mDocument == nullptr) return false;
    if (s_touchOwner != controls) remove_extra_elements();
    if (s_extraRoot != nullptr) return true;
    const std::string tag = "div";
    auto root = s_touchApi.create(controls->mDocument, &tag);
    if (!root) return false;
    extra_property(root.get(), "position", "absolute");
    extra_property(root.get(), "left", "0dp");
    extra_property(root.get(), "top", "0dp");
    extra_property(root.get(), "width", "100%");
    extra_property(root.get(), "height", "100%");
    extra_property(root.get(), "pointer-events", "none");
    extra_property(root.get(), "z-index", "2");
    std::string rml;
    for (size_t i = 0; i < touch::Count; ++i) {
        rml += "<button class='control trigger' style='display:none;pointer-events:auto;box-sizing:border-box;'><span>";
        rml += touch::Labels[i];
        rml += "</span></button>";
    }
    s_touchApi.setRml(root.get(), &rml);
    s_extraRoot = s_touchApi.append(controls->mRoot, std::move(root), true);
    s_touchOwner = controls;
    if (!s_extraRoot) return false;
    for (size_t i = 0; i < touch::Count; ++i) s_extraElements[i] = s_touchApi.child(s_extraRoot, int(i));
    return true;
}
void update_extra_elements(ModContext*, void* args, void*, void*) {
    if (!s_extraHooksReady) return;
    auto* controls = mods::arg<NativeTouch*>(args, 0);
    if (!create_extra_elements(controls)) return;
    prune_extra_presses(controls);
    const bool available = extra_controls_available(controls);
    const float scale = std::max(0.01f, s_touchApi.scale(nullptr));
    const auto size = s_touchApi.dimensions(nullptr);
    const auto inset = controls->mSafeInsets;
    const float left = inset.left / scale, top = inset.top / scale;
    const float width = std::max(1.f, size.w - left - inset.right / scale);
    const float height = std::max(1.f, size.h - top - inset.bottom / scale);
    s_viewWidth = width;
    s_viewHeight = height;
    for (size_t i = 0; i < touch::Count; ++i) {
        auto* element = s_extraElements[i];
        if (element == nullptr) continue;
        const auto rect = touch::rect(button_layout(i), width, height, left, top);
        const bool show = available && button_enabled(i);
        const bool held = s_extraPresses.held(i);
        char key[160];
        std::snprintf(key, sizeof(key), "%.2f/%.2f/%.2f/%d/%d", rect.x, rect.y, rect.size, show, held);
        if (s_extraStyles[i] == key) continue;
        s_extraStyles[i] = key;
        const auto dp = [](float value) { return std::to_string(value) + "dp"; };
        extra_property(element, "left", dp(rect.x));
        extra_property(element, "top", dp(rect.y));
        extra_property(element, "width", dp(rect.size));
        extra_property(element, "height", dp(rect.size));
        extra_property(element, "font-size", dp(rect.size * 0.42f));
        extra_property(element, "border-radius", dp(rect.size * 0.3f));
        extra_property(element, "background-color", held ? "rgba(63,78,90,90%)" : "rgba(22,24,28,58%)");
        extra_property(element, "display", show ? "flex" : "none");
    }
}
HookAction extra_touch_down(ModContext*, void* args, void*, void*) {
    auto* controls = mods::arg<NativeTouch*>(args, 0);
    auto* event = mods::arg<Rml::Event*>(args, 1);
    prune_extra_presses(controls);
    if (s_touchOwner != controls || !extra_controls_available(controls)) return HOOK_CONTINUE;
    // Match the event's actual target, including its text span. Do not steal
    // presses on a vanilla button or stick merely because rectangles overlap.
    auto* target = s_touchApi.target(event);
    for (int depth = 0; target && target != s_extraRoot && depth < 4; ++depth) {
        for (size_t i = 0; i < touch::Count; ++i) if (target == s_extraElements[i] && button_enabled(i)) {
            s_extraPresses.press(s_touchApi.finger(event), i);
            s_touchApi.stop(event);
            return HOOK_SKIP_ORIGINAL;
        }
        target = s_touchApi.parent(target);
    }
    return HOOK_CONTINUE;
}
HookAction extra_touch_motion(ModContext*, void* args, void*, void*) {
    auto* event = mods::arg<Rml::Event*>(args, 1);
    if (!s_extraPresses.owns(s_touchApi.finger(event))) return HOOK_CONTINUE;
    s_touchApi.stop(event);
    return HOOK_SKIP_ORIGINAL;
}
HookAction extra_touch_up(ModContext*, void* args, void*, void*) {
    auto* event = mods::arg<Rml::Event*>(args, 1);
    if (!s_extraPresses.release(s_touchApi.finger(event))) return HOOK_CONTINUE;
    s_touchApi.stop(event);
    return HOOK_SKIP_ORIGINAL;
}
HookAction extra_clear_input(ModContext*, void*, void*, void*) {
    s_extraPresses.clear();
    return HOOK_CONTINUE;
}
HookAction extra_sync_input(ModContext*, void* args, void*, void*) {
    prune_extra_presses(mods::arg<NativeTouch*>(args, 0));
    return HOOK_CONTINUE;
}
HookAction extra_delete(ModContext*, void* args, void*, void*) {
    if (mods::arg<NativeTouch*>(args, 0) == s_touchOwner) remove_extra_elements();
    return HOOK_CONTINUE;
}
HookAction extra_set_pad(ModContext*, void* args, void*, void*) {
    if (mods::arg<u32>(args, 0) != PAD_1) return HOOK_CONTINUE;
    const auto* status = mods::arg<const PADStatus*>(args, 1);
    if (status == nullptr) return HOOK_CONTINUE;
    prune_extra_presses(s_touchOwner);
    s_extraBasePad = *status;
    s_extraMergedPad = *status;
    s_extraPadOwned = s_extraPresses.merge(s_extraMergedPad);
    if (s_extraPadOwned) mods::arg_ref<const PADStatus*>(args, 1) = &s_extraMergedPad;
    return HOOK_CONTINUE;
}
HookAction extra_clear_pad(ModContext*, void* args, void*, void*) {
    if (mods::arg<u32>(args, 0) != PAD_1) return HOOK_CONTINUE;
    prune_extra_presses(s_touchOwner);
    s_extraBasePad = {};
    s_extraBasePad.err = PAD_ERR_NONE;
    s_extraMergedPad = s_extraBasePad;
    s_extraPadOwned = s_extraPresses.merge(s_extraMergedPad);
    if (!s_extraPadOwned) return HOOK_CONTINUE;
    // Native touch may have no active buttons/stick and request a clear. Keep
    // only our held inputs in that case; never replace physical controller data.
    ExtraTouchSetPad::g_orig(PAD_1, &s_extraMergedPad);
    return HOOK_SKIP_ORIGINAL;
}

template<class Fn> bool resolve_extra(const char* name, Fn& function) {
    void* address = nullptr;
    if (svc_hook->resolve(mod_ctx, name, &address, nullptr) != MOD_OK || address == nullptr) {
        const auto message = std::string("Touch Buttons unavailable: could not resolve ") + name;
        svc_log->warn(mod_ctx, message.c_str());
        return false;
    }
    function = reinterpret_cast<Fn>(address);
    return true;
}
ModResult install_native_buttons(ModError* error) {
    s_extraSessionEnabled = dawnlight_touch_ui_enabled();
    if (!s_extraSessionEnabled) return MOD_OK;
    // Resolve host methods rather than adding mandatory RmlUi imports. A host
    // lacking these private APIs can still load the rest of Dawnlight.
    if (!resolve_extra("Rml::ElementDocument::CreateElement", s_touchApi.create) ||
        !resolve_extra("Rml::Element::AppendChild", s_touchApi.append) ||
        !resolve_extra("Rml::Element::RemoveChild", s_touchApi.remove) ||
        !resolve_extra("_ZN3Rml7Element11SetInnerRMLERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE", s_touchApi.setRml) ||
        !resolve_extra("_ZN3Rml7Element11SetPropertyERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEES9_", s_touchApi.setProperty) ||
        !resolve_extra("_ZNK3Rml7Element8GetChildEi", s_touchApi.child) ||
        !resolve_extra("Rml::Element::GetParentNode", s_touchApi.parent) ||
        !resolve_extra("Rml::Event::GetTargetElement", s_touchApi.target) ||
        !resolve_extra("Rml::Event::StopPropagation", s_touchApi.stop) ||
        !resolve_extra("dusk::ui::touch_event_id", s_touchApi.finger) ||
        !resolve_extra("dusk::ui::touch_document_size_dp", s_touchApi.dimensions) ||
        !resolve_extra("dusk::ui::touch_dp_scale", s_touchApi.scale) ||
        !resolve_extra("dusk::ui::Document::visible", s_touchApi.visible) ||
        !resolve_extra("dusk::config::GetConfigVar", s_touchApi.config)) return MOD_OK;
    ModResult result = mods::hook_add_post<ExtraTouchUpdate>(svc_hook, update_extra_elements);
    if (result == MOD_OK) result = mods::hook_add_pre<ExtraTouchSync>(svc_hook, extra_sync_input);
    if (result == MOD_OK) result = mods::hook_add_pre<ExtraTouchClear>(svc_hook, extra_clear_input);
    if (result == MOD_OK) result = mods::hook_add_pre<ExtraTouchDelete>(svc_hook, extra_delete);
    if (result == MOD_OK) result = mods::hook_add_pre<ExtraTouchDown>(svc_hook, extra_touch_down);
    if (result == MOD_OK) result = mods::hook_add_pre<ExtraTouchMotion>(svc_hook, extra_touch_motion);
    if (result == MOD_OK) result = mods::hook_add_pre<ExtraTouchUp>(svc_hook, extra_touch_up);
    if (result == MOD_OK) result = mods::hook_add_pre<ExtraTouchCancel>(svc_hook, extra_touch_up);
    if (result == MOD_OK) result = mods::hook_add_pre<ExtraTouchSetPad>(svc_hook, extra_set_pad);
    if (result == MOD_OK) result = mods::hook_add_pre<ExtraTouchClearPad>(svc_hook, extra_clear_pad);
    if (result == MOD_OK) result = mods::hook_add_pre<ExtraTouchClearAllPads>(svc_hook, extra_clear_input);
    if (result != MOD_OK) return mods::set_error(error, result, "failed to install extra touch button hooks");
    s_extraHooksReady = true;
    s_touchRuntimeAvailable = true;
    return MOD_OK;
}
void shutdown_native_buttons() {
    s_extraHooksReady = false;
    s_touchRuntimeAvailable = false;
    s_extraPresses.clear();
    if (s_extraPadOwned && ExtraTouchSetPad::g_orig) ExtraTouchSetPad::g_orig(PAD_1, &s_extraBasePad);
    s_extraPadOwned = false;
    remove_extra_elements();
    s_extraSessionEnabled = false;
}
