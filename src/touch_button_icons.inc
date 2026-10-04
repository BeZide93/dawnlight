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
