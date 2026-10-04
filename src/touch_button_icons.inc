// Minimal symbols shared by gameplay and the layout editor.
// RmlUi corner radii use lengths; percentage radii are unsupported by the host.
std::string extra_icon_part(const char* style) {
    return "<span style='position:absolute;display:block;pointer-events:none;" +
        std::string(style) + "'></span>";
}
std::string extra_button_content(size_t index) {
    if (index != touch::Jump && index != touch::DarkLink)
        return "<span>" + std::string(touch::Labels[index]) + "</span>";
    std::string icon = "<span style='position:relative;display:block;width:76%;height:76%;pointer-events:none;'>";
    if (index == touch::Jump) {
        // Upward arrow over a short take-off line: jump, lift and flight.
        const char* parts[] = {
            "left:44%;top:24%;width:12%;height:42%;border-radius:100dp;background-color:#f8f4e8;",
            "left:24%;top:24%;width:34%;height:12%;border-radius:100dp;background-color:#f8f4e8;transform:rotate(-45deg);",
            "left:42%;top:24%;width:34%;height:12%;border-radius:100dp;background-color:#f8f4e8;transform:rotate(45deg);",
            "left:28%;top:78%;width:44%;height:8%;border-radius:100dp;background-color:#f8f4e8;",
        };
        for (const auto* part : parts) icon += extra_icon_part(part);
    } else {
        // Two restrained red eyes retain Dark Link's signature at small sizes.
        const char* parts[] = {
            "left:16%;top:44%;width:29%;height:12%;border-radius:2dp;background-color:#ff354b;transform:rotate(14deg);",
            "left:55%;top:44%;width:29%;height:12%;border-radius:2dp;background-color:#ff354b;transform:rotate(-14deg);",
        };
        for (const auto* part : parts) icon += extra_icon_part(part);
    }
    return icon + "</span>";
}
