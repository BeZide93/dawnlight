// Stabilize the host's single capture, without changing the on-screen HUD.
// Like TE, exclude the hint glow and HD HUD badge from the icon itself.
struct MidnaPaneState {
    J2DPane* pane;
    u8 alpha;
    u8 colorAlpha;
    bool visible;
};
std::array<MidnaPaneState, 64> s_midnaCaptureStates{};
size_t s_midnaCaptureCount = 0;

void prepare_midna_capture(J2DPane* pane, bool showHidden) {
    if (pane == nullptr || s_midnaCaptureCount == s_midnaCaptureStates.size()) return;
    const bool overlay = pane->mInfoTag == MULTI_CHAR('hd_mbtn') ||
                         pane->mInfoTag == MULTI_CHAR('j_light1');
    if (!overlay && !showHidden && !pane->isVisible()) return;
    s_midnaCaptureStates[s_midnaCaptureCount++] =
        {pane, pane->getAlpha(), pane->mColorAlpha, pane->isVisible()};
    if (overlay) {
        pane->hide();
        return;
    }
    pane->show();
    pane->setAlpha(255);
    pane->mColorAlpha = 255;
    for (auto* child = pane->getFirstChildPane(); child; child = child->getNextChildPane())
        prepare_midna_capture(child, showHidden);
}
HookAction before_midna_icon_capture(ModContext*, void* args, void*, void*) {
    s_midnaCaptureCount = 0;
    if (!dawnlight_touch_ui_active() || !midna_touch_button_ready()) return HOOK_CONTINUE;
    auto* pane = mods::arg<J2DPane*>(args, 0);
    if (pane) prepare_midna_capture(pane, !pane->isVisible());
    return HOOK_CONTINUE;
}
void after_midna_icon_capture(ModContext*, void*, void*, void*) {
    while (s_midnaCaptureCount > 0) {
        const auto& state = s_midnaCaptureStates[--s_midnaCaptureCount];
        state.pane->setAlpha(state.alpha);
        state.pane->mColorAlpha = state.colorAlpha;
        if (state.visible) state.pane->show();
        else state.pane->hide();
    }
}
