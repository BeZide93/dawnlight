// Essentials 2.2 creates this gameplay button in sync_control_displays(), with
// fixed dp geometry and no TouchControlsEditor registration. Adapt its layout,
// never its input, visibility, icon or lifetime. Look it up afresh after the host
// update (which encloses every display-sync hook, regardless of mod load order).
Rml::Element* (*s_quickAccessById)(Rml::Element*, const Rml::String*) = nullptr;
bool s_quickAccessOverridden = false;

bool quick_access_editor_available() {
    if (!s_quickAccessById) return false;
    for (const auto key : {"mod.com_dusklight_twilit__essentials.enabled",
             "mod.com_dusklight_twilit__essentials.quickAccessEnabled"}) {
        const auto* var = s_touchApi.config(key);
        if (!var || !static_cast<const dusk::config::ConfigVar<bool>*>(var)->getValue()) return false;
    }
    return true;
}
Rml::Element* quick_access_element(NativeTouch* controls) {
    if (!s_quickAccessById || !controls || !controls->mRoot) return nullptr;
    const Rml::String id = "twe-quick-access-touch";
    return s_quickAccessById(controls->mRoot, &id);
}
void restore_quick_access_layout() {
    if (s_quickAccessOverridden) if (auto* button = quick_access_element(s_touchOwner)) {
        extra_property(button, "left", "298dp");
        extra_property(button, "top", "auto");
        extra_property(button, "bottom", "0dp");
        extra_property(button, "width", "64dp");
        extra_property(button, "height", "46dp");
        extra_property(button, "transform", "none");
    }
    s_quickAccessOverridden = false;
}
void update_quick_access_layout(NativeTouch* controls, dusk::ui::ControlLayoutSize size) {
    if (!quick_access_editor_available()) return;
    auto* button = quick_access_element(controls);
    if (!button) return; // Not created yet, or Essentials is being unloaded.
    char value[256]{};
    dusk::ui::ControlProps props;
    if (svc_config->get_string(mod_ctx, s_quickAccessLayout, value, sizeof(value), nullptr) != MOD_OK ||
        !decode_extra_props(value, props)) return; // Leave the original geometry until the first Save.
    const auto layout = dusk::ui::resolve_control_layout(props, size);
    std::optional<dusk::ui::ControlRect> applied;
    s_touchApi.applyBox(button, &applied, layout.box);
    extra_property(button, "bottom", "auto");
    extra_property(button, "transform", "scale(" + std::to_string(layout.scale) + ")");
    s_quickAccessOverridden = true;
}
