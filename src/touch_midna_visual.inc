// TE's native trigger/button-z styling and centered 40dp icon on a 78x46dp button.
// Keep a separate hint pulse: baking j_light1 into the captured texture flickers.
std::string midna_button_rml(const std::string& source) {
    const std::string glow =
        "<span style='position:absolute;left:0dp;top:0dp;width:100%;height:100%;"
        "border-radius:23dp;background-color:rgba(150,255,170,65%);opacity:0;pointer-events:none;'></span>";
    return glow + (source.empty() ? "<span style='font-size:16dp;'>Midna</span>" :
        "<img class='midna-icon visible' style='left:50%;top:50%;width:40dp;height:40dp;"
        "margin-left:-20dp;margin-top:-20dp;pointer-events:none;' src='" + source + "' />");
}
void update_midna_touch_visual(Rml::Element* button, float width, float height, bool shown, bool held) {
    const Rml::String hidden = "hidden", pressed = "pressed";
    s_touchApi.setPseudoClass(button, &hidden, !shown);
    s_touchApi.setClass(button, &pressed, shown && held);
    // Override our inline hit-testing rule when hidden, just like TE's :hidden style.
    extra_property(button, "pointer-events", shown ? "auto" : "none");
    // A failed viewport query may have hidden all buttons with display:none.
    extra_property(button, "display", "flex");
    if (!shown) {
        s_midnaPulseFrame = 0;
        s_midnaGlowLevel = -1;
        return;
    }
    const auto source = midna_touch_button_icon();
    if (!s_midnaButtonFilled || source != s_midnaButtonSource) {
        const auto rml = midna_button_rml(source);
        s_touchApi.setRml(button, &rml);
        // The host rotates a bounded set of texture URLs. Release the old cache
        // entry so reusing a URL later cannot display an outdated capture.
        if (s_touchApi.releaseTexture && !s_midnaButtonSource.empty())
            s_touchApi.releaseTexture(&s_midnaButtonSource, nullptr);
        s_midnaButtonSource = source;
        s_midnaButtonFilled = true;
        s_midnaGlowLevel = -1;
    }
    const float scale = std::min(width / 78.f, height / 46.f);
    const auto dp = [](float value) { return std::to_string(value) + "dp"; };
    extra_property(button, "border-radius", dp(23.f * scale));
    auto* glow = s_touchApi.child(button, 0);
    auto* icon = s_touchApi.child(button, 1);
    if (glow) extra_property(glow, "border-radius", dp(23.f * scale));
    if (icon && !source.empty()) {
        extra_property(icon, "width", dp(40.f * scale));
        extra_property(icon, "height", dp(40.f * scale));
        extra_property(icon, "margin-left", dp(-20.f * scale));
        extra_property(icon, "margin-top", dp(-20.f * scale));
    }
    auto* meter = g_meter2_info.getMeterClass();
    auto* draw = meter ? meter->getMeterDrawPtr() : nullptr;
    int level = 0;
    if (draw && draw->field_0x738 > 0.f) {
        constexpr int pulseFrames = 30;
        const int phase = s_midnaPulseFrame;
        if (phase == 0)
            Z2GetAudioMgr()->seStart(Z2SE_SY_HINT_BUTTON_BLINK, nullptr, 0, 0, 1.f, 1.f, -1.f, -1.f, 0);
        level = 10 - (phase * 10) / pulseFrames;
        s_midnaPulseFrame = (phase + 1) % pulseFrames;
    } else s_midnaPulseFrame = 0;
    if (glow && level != s_midnaGlowLevel) {
        extra_property(glow, "opacity", std::to_string(float(level) / 10.f));
        s_midnaGlowLevel = level;
    }
}
