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
