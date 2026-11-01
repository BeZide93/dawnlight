// UI_SCOPE_WINDOW includes other mods and host menus. Always qualify selectors
// with ModWindow's owner attribute; never apply a global window/control reset.
// Include interactive states to override the host's focus/hover shadow rules.
constexpr const char* kSurvivorsPerformanceUiStyle = R"(
body window[mod-id="dev.bezide.dawnlight"],
body window[mod-id="dev.bezide.dawnlight"] *,
body window[mod-id="dev.bezide.dawnlight"] :hover,
body window[mod-id="dev.bezide.dawnlight"] :focus-visible,
body window[mod-id="dev.bezide.dawnlight"] :active,
body window[mod-id="dev.bezide.dawnlight"] :selected,
body window[mod-id="dev.bezide.dawnlight"] :disabled {
    box-shadow: none;
    backdrop-filter: none;
    border-radius: 0;
    font-effect: none;
}
body window[mod-id="dev.bezide.dawnlight"] tab-bar tab,
body window[mod-id="dev.bezide.dawnlight"] tab-bar tab:hover,
body window[mod-id="dev.bezide.dawnlight"] tab-bar tab:focus-visible,
body window[mod-id="dev.bezide.dawnlight"] tab-bar tab:active {
    decorator: none;
    transition: opacity 0.1s linear-in-out;
}
body window[mod-id="dev.bezide.dawnlight"] tab-bar tab:hover,
body window[mod-id="dev.bezide.dawnlight"] tab-bar tab:focus-visible {
    background-color: rgba(var(--color-accent-rgb), 15%);
}
body window[mod-id="dev.bezide.dawnlight"] tab-bar tab:active {
    background-color: rgba(var(--color-accent-rgb), 25%);
}
)";

UiStyleHandle s_performanceUiStyle = 0;
bool s_performanceUiRequested = false;

void clear_performance_ui() {
    if (s_performanceUiStyle) {
        svc_ui->unregister_styles(mod_ctx, s_performanceUiStyle);
        s_performanceUiStyle = 0;
    }
    s_performanceUiRequested = false;
}
void update_performance_ui() {
    const bool wanted = s_active && survivor_perf::enabled();
    if (wanted == s_performanceUiRequested) return;
    if (!wanted) {
        clear_performance_ui();
        return;
    }
    // Register once per activation, including failure: do not reparse/rebuild
    // every document each frame if a host rejects the optional style sheet.
    s_performanceUiRequested = true;
    if (svc_ui->register_styles(mod_ctx, UI_SCOPE_WINDOW,
            kSurvivorsPerformanceUiStyle, &s_performanceUiStyle) != MOD_OK) {
        s_performanceUiStyle = 0;
        svc_log->warn(mod_ctx, "Survivors Performance Mode: simplified window styling unavailable");
    }
}
