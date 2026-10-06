// Inline radii scale with our buttons, so native docking CSS cannot override
// them. Flatten only the corners touching an edge and restore all others.
void apply_extra_dock_corners(Rml::Element* element, dusk::ui::ControlAnchor anchor, float radius) {
    using A = dusk::ui::ControlAnchor;
    const bool top = anchor == A::Top || anchor == A::TopLeft || anchor == A::TopRight;
    const bool bottom = anchor == A::Bottom || anchor == A::BottomLeft || anchor == A::BottomRight;
    const bool left = anchor == A::Left || anchor == A::TopLeft || anchor == A::BottomLeft;
    const bool right = anchor == A::Right || anchor == A::TopRight || anchor == A::BottomRight;
    const auto rounded = std::to_string(radius) + "dp";
    extra_property(element, "border-top-left-radius", top || left ? "0dp" : rounded);
    extra_property(element, "border-top-right-radius", top || right ? "0dp" : rounded);
    extra_property(element, "border-bottom-left-radius", bottom || left ? "0dp" : rounded);
    extra_property(element, "border-bottom-right-radius", bottom || right ? "0dp" : rounded);
}

void apply_extra_action_style(Rml::Element* bar, float width, float height, bool held,
    dusk::ui::ControlAnchor dock) {
    const float scale = std::min(width / 57.f, height / 46.f);
    apply_extra_dock_corners(bar, dock, 23.f * scale);
    if (auto* button = s_touchApi.child(bar, 0)) {
        const Rml::String pressed = "pressed";
        s_touchApi.setClass(button, &pressed, held);
        extra_property(button, "height", std::to_string(44.f * scale) + "dp");
        // The glyph markup uses relative sizes; match TE's native 24dp glyph.
        extra_property(button, "font-size", std::to_string(24.f * scale) + "dp");
    }
}
