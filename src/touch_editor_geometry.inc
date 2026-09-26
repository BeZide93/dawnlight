// Corner/edge geometry adapted from the pinned Dusklight TouchControlsEditor::rect_for_edit.
// Pure inputs keep native/foreign editor state untouched.
dusk::ui::ControlRect extra_edit_rect(const NativeEditor::PointerEdit& edit,
    Rml::Vector2f pointerDp, dusk::ui::ControlLayoutSize docSize, dusk::ui::ControlProps& props) {
    auto rect = edit.startVisual;
    const auto delta = pointerDp - edit.startPointerDp;

    switch (edit.handle) {
    case EditHandle::Move:
        rect.l += delta.x;
        rect.t += delta.y;
        return rect;
    case EditHandle::Left: {
        const float right = edit.startVisual.l + edit.startVisual.w;
        rect.l = pointerDp.x;
        rect.w = right - rect.l;
        return rect;
    }
    case EditHandle::Right:
        rect.w = pointerDp.x - edit.startVisual.l;
        return rect;
    case EditHandle::Top: {
        const float bottom = edit.startVisual.t + edit.startVisual.h;
        rect.t = pointerDp.y;
        rect.h = bottom - rect.t;
        return rect;
    }
    case EditHandle::Bottom:
        rect.h = pointerDp.y - edit.startVisual.t;
        return rect;
    case EditHandle::TopLeft:
    case EditHandle::TopRight:
    case EditHandle::BottomLeft:
    case EditHandle::BottomRight:
        break;
    }

    const bool left = edit.handle == EditHandle::TopLeft || edit.handle == EditHandle::BottomLeft;
    const bool top = edit.handle == EditHandle::TopLeft || edit.handle == EditHandle::TopRight;
    const Rml::Vector2f fixed{
        left ? edit.startVisual.l + edit.startVisual.w : edit.startVisual.l,
        top ? edit.startVisual.t + edit.startVisual.h : edit.startVisual.t,
    };
    const float desiredW = left ? fixed.x - pointerDp.x : pointerDp.x - fixed.x;
    const float desiredH = top ? fixed.y - pointerDp.y : pointerDp.y - fixed.y;
    const Rml::Vector2f minSize{36.f, 36.f};
    const float minRatio =
        std::max(minSize.x / std::max(edit.startVisual.w, 1.f),
            minSize.y / std::max(edit.startVisual.h, 1.f));
    const float maxW = left ? fixed.x : docSize.w - fixed.x;
    const float maxH = top ? fixed.y : docSize.h - fixed.y;
    const float maxRatio =
        std::max(minRatio, std::min(maxW / std::max(edit.startVisual.w, 1.f),
                                maxH / std::max(edit.startVisual.h, 1.f)));
    const float ratio =
        std::clamp(std::max(desiredW / std::max(edit.startVisual.w, 1.f),
                       desiredH / std::max(edit.startVisual.h, 1.f)),
            minRatio, maxRatio);

    rect.w = edit.startVisual.w * ratio;
    rect.h = edit.startVisual.h * ratio;
    rect.l = left ? fixed.x - rect.w : fixed.x;
    rect.t = top ? fixed.y - rect.h : fixed.y;
    props.scale = std::max(edit.startProps.scale * ratio, 0.25f);
    return rect;
}
