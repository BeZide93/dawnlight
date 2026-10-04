// Use the same bundled Material Symbols Rounded font as Dusklight's utility
// controls and TE's bottle button (U+F69D), rather than approximating strokes.
// Relative font sizes follow the button/editor scale; the host supplies colour,
// antialiasing, rounded outlines and the usual pressed feedback.
std::string extra_button_content(size_t index) {
    if (index == touch::Jump) {
        // "upgrade": an upward arrow with a short take-off line.
        return "<icon style='pointer-events:none;'>"
               "<glyph style='font-size:100%;'>&#xf0fb;</glyph></icon>";
    }
    if (index == touch::DarkLink) {
        // "visibility": actual eye contours with pupils, tilted gently inward.
        return "<icon style='pointer-events:none;color:#ed5966;'>"
               "<glyph style='font-size:75%;transform:rotate(12deg);'>&#xe8f4;</glyph>"
               "<glyph style='font-size:75%;transform:rotate(-12deg);'>&#xe8f4;</glyph></icon>";
    }
    return "<span>" + std::string(touch::Labels[index]) + "</span>";
}

// Use TE's action-bar + utility button structure so the native stylesheet owns
// background/border colours, transparency, clipping and pressed feedback.
bool extra_action_button(size_t index) {
    return index == touch::Jump || index == touch::DarkLink;
}
std::string extra_action_button_rml(size_t index) {
    return "<action-bar class='control' style='pointer-events:auto;box-sizing:border-box;'>"
           "<button class='utility' style='width:100%;height:100%;pointer-events:auto;'>" +
           extra_button_content(index) + "</button></action-bar>";
}
