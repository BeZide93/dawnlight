// Keep elements in the layout so native 0.2s opacity transitions can run.
// Disable hit testing immediately, including utility children whose inline
// pointer-events would otherwise override the hidden parent's inherited value.
void set_extra_element_visibility(Rml::Element* element, bool shown, bool utility = false) {
    const Rml::String hidden = "hidden";
    s_touchApi.setPseudoClass(element, &hidden, !shown);
    extra_property(element, "pointer-events", shown ? "auto" : "none");
    if (utility) if (auto* button = s_touchApi.child(element, 0))
        extra_property(button, "pointer-events", shown ? "auto" : "none");
}
