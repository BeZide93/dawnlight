// Share Dusklight's context, units and layout representation between playback
// and the original host editor. Passing null to touch_document_size_dp returns
// an empty viewport; unlike touch_dp_scale it does NOT find a default context.
bool extra_viewport(Rml::Element* element, dusk::ui::ControlLayoutSize& size) {
    size = {};
    if (!element) return false;
    auto* context = s_touchApi.context(element);
    if (!context) return false;
    size = s_touchApi.dimensions(context);
    return std::isfinite(size.w) && std::isfinite(size.h) && size.w > 0 && size.h > 0;
}

bool valid_extra_props(const dusk::ui::ControlProps& p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.w) &&
        std::isfinite(p.h) && std::isfinite(p.scale) && std::abs(p.x) <= 16384 &&
        std::abs(p.y) <= 16384 && p.w >= 1 && p.w <= 8192 && p.h >= 1 && p.h <= 8192 &&
        p.scale >= 0.25f && p.scale <= 100 && static_cast<unsigned>(p.anchor) <= 8;
}
std::string encode_extra_props(const dusk::ui::ControlProps& props) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out.precision(9);
    out << "1 " << props.x << ' ' << props.y << ' ' << props.w << ' ' << props.h << ' '
        << props.scale << ' ' << int(props.anchor);
    return out.str();
}
bool decode_extra_props(const char* value, dusk::ui::ControlProps& props) {
    if (!value) return false;
    std::istringstream in(value);
    in.imbue(std::locale::classic());
    int version = 0, anchor = -1;
    dusk::ui::ControlProps parsed;
    if (!(in >> version >> parsed.x >> parsed.y >> parsed.w >> parsed.h >> parsed.scale >> anchor) ||
        version != 1 || anchor < 0 || anchor > 8) return false;
    in >> std::ws;
    if (!in.eof()) return false;
    parsed.anchor = static_cast<dusk::ui::ControlAnchor>(anchor);
    if (!valid_extra_props(parsed)) return false;
    props = parsed;
    return true;
}
dusk::ui::ControlProps default_extra_props(size_t index, dusk::ui::ControlLayoutSize size,
    bool migrateLegacy = false) {
    const auto legacy = migrateLegacy ? button_layout(index) : touch::Defaults[index];
    const auto rect = touch::rect(legacy, size.w, size.h);
    dusk::ui::ControlProps props;
    props.w = rect.size;
    props.h = rect.size;
    return dusk::ui::encode_control_props({rect.x, rect.y, rect.size, rect.size}, size, props,
        dusk::ui::ControlAnchor::None);
}
dusk::ui::ControlProps saved_extra_props(size_t index, dusk::ui::ControlLayoutSize size) {
    char value[256]{};
    dusk::ui::ControlProps props;
    if (svc_config->get_string(mod_ctx, s_buttons[index].layout, value, sizeof(value), nullptr) == MOD_OK &&
        decode_extra_props(value, props)) return props;
    // Preserve positions saved by the previous numeric editor until the user
    // saves through the native editor; enabled switches remain independent.
    return default_extra_props(index, size, true);
}
