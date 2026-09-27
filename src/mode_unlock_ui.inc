// Included inside ui.cpp's anonymous namespace. A native disabled control
// cannot receive mouse/touch/controller activation. Its locked proxy remains
// focusable, but only opens a confirmation; the real setter stays guarded.
UiDialogHandle s_modeUnlockDialog = 0;

bool sync_mode_control(ModContext* ctx, ModeControlBinding& binding) {
    int64_t value = 0;
    const bool locked = dawnlight_mode_enabled() && mode_config_override(binding.var, value);
    if (!binding.control || !binding.unlock) return locked;
    if (!binding.initialized || binding.shownLocked != locked) {
        binding.initialized = true;
        binding.shownLocked = locked;
        svc_ui->elem_set_visible(ctx, binding.control, !locked);
        svc_ui->elem_set_visible(ctx, binding.unlock, locked);
    }
    if (locked) {
        std::string shown;
        if (binding.kind == UI_CONTROL_TOGGLE) shown = value ? "On" : "Off";
        else if (binding.kind == UI_CONTROL_SELECT && value >= 0 &&
                 static_cast<size_t>(value) < binding.options.size()) shown = binding.options[value];
        else shown = std::to_string(value) + binding.suffix;
        shown = binding.label + ": " + shown;
        if (shown != binding.shownLabel) {
            binding.shownLabel = shown;
            svc_ui->control_set_label(ctx, binding.unlock, shown.c_str());
        }
    }
    if (!locked && binding.focusAfterUnlock && svc_ui->elem_focus(ctx, binding.control) == MOD_OK)
        binding.focusAfterUnlock = false;
    return locked;
}

bool mode_unlock_disabled(ModContext* ctx, void* data) {
    return !sync_mode_control(ctx, *static_cast<ModeControlBinding*>(data));
}

void cancel_mode_unlock(ModContext*, UiDialogHandle, void*) {
    s_modeUnlockDialog = 0;
}

void confirm_mode_unlock(ModContext* ctx, UiDialogHandle dialog, void* data) {
    auto& binding = *static_cast<ModeControlBinding*>(data);
    const auto result = leave_dawnlight_mode_for_edit(binding.var);
    if (result != MOD_OK) {
        svc_ui->dialog_set_body(ctx, dialog,
            "The settings could not be adopted. No edit was applied. Cancel and try again.");
        return;
    }
    svc_ui->dialog_close(ctx, dialog);
    s_modeUnlockDialog = 0;
    binding.focusAfterUnlock = true;
    sync_mode_control(ctx, binding);
}

void request_mode_unlock(ModContext* ctx, void* data) {
    auto& binding = *static_cast<ModeControlBinding*>(data);
    int64_t value = 0;
    if (s_modeUnlockDialog || !dawnlight_mode_enabled() || !mode_config_override(binding.var, value)) return;
    std::string body = "Editing this setting will turn <b>Dawnlight Mode Off</b>. "
        "The current Dawnlight values will <b>replace your previously saved manual settings</b>. "
        "Any changes you make afterwards will be saved as your new manual settings.";
    if (edit_requires_progression_off(binding.var))
        body += "<br/><br/><b>Progression System will also be turned Off</b> so this setting can be edited manually.";
    body += "<br/><br/>Continue?";
    const std::string title = "Edit " + binding.label;
    std::array<UiDialogAction, 2> actions{{UI_DIALOG_ACTION_INIT, UI_DIALOG_ACTION_INIT}};
    actions[0].label = "Cancel";
    actions[0].on_pressed = cancel_mode_unlock;
    actions[1].label = "Use Dawnlight values and edit";
    actions[1].on_pressed = confirm_mode_unlock;
    actions[1].user_data = &binding;
    actions[1].keep_open = true;
    UiDialogDesc desc = UI_DIALOG_DESC_INIT;
    desc.title = title.c_str();
    desc.body_rml = body.c_str();
    desc.variant = UI_DIALOG_WARNING;
    desc.actions = actions.data();
    desc.action_count = actions.size();
    desc.on_dismiss = cancel_mode_unlock;
    svc_ui->dialog_push(ctx, &desc, &s_modeUnlockDialog);
}

ModResult add_mode_control(ModContext* ctx, UiElementHandle pane, UiControlDesc& desc) {
    if (mode_setting_for_config(desc.config_var) == ModeSetting::None)
        return svc_ui->pane_add_control(ctx, pane, &desc, nullptr);
    auto& binding = s_modeControls.at(desc.config_var);
    binding.label = desc.label;
    binding.suffix = desc.suffix ? desc.suffix : "";
    for (size_t i = 0; i < desc.option_count; ++i) binding.options.emplace_back(desc.options[i]);
    auto result = svc_ui->pane_add_control(ctx, pane, &desc, &binding.control);
    if (result != MOD_OK) return result;
    UiControlDesc locked = UI_CONTROL_DESC_INIT;
    locked.kind = UI_CONTROL_BUTTON;
    locked.label = desc.label;
    locked.help_rml = "Controlled by Dawnlight Mode. Select to review the warning before editing.";
    locked.on_pressed = request_mode_unlock;
    locked.is_disabled = mode_unlock_disabled;
    locked.user_data = &binding;
    result = svc_ui->pane_add_control(ctx, pane, &locked, &binding.unlock);
    if (result != MOD_OK) return result;
    svc_ui->elem_set_class(ctx, binding.unlock, "dawnlight-locked-setting", true);
    sync_mode_control(ctx, binding);
    return MOD_OK;
}
