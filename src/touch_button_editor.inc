// Extend the native editor document without replacing its controls, element
// slots or hook chain. Dawnlight selection/gestures/layouts are private state.
using NativeEditor = dusk::ui::TouchControlsEditor;
using EditorDocument = dusk::ui::Document;
using EditorListener = dusk::ui::ScopedEventListener;
using EditHandle = NativeEditor::EditHandle;
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::sync_control_layouts", void(NativeEditor*), ExtraEditorLayouts);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::reset_working_layout", void(NativeEditor*), ExtraEditorReset);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::save_layout", void(NativeEditor*), ExtraEditorSave);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui8DocumentD2Ev", void(EditorDocument*), ExtraEditorDelete);
DEFINE_HOOK_SYMBOL("dusk::ui::push_document",
    EditorDocument*(std::unique_ptr<EditorDocument>*, bool, bool), ExtraEditorPush);

struct EditorApi {
    void (*construct)(NativeEditor*) = nullptr;
    EditorDocument& (*push)(std::unique_ptr<EditorDocument>, bool, bool) = nullptr;
    EditorDocument* (*top)() = nullptr;
    void (*uncover)() = nullptr;
    void (*hide)(EditorDocument*, bool) = nullptr;
    void (*listenTouch)(EditorDocument*, Rml::Element*, const Rml::String*, EditorListener::Callback, bool) = nullptr;
    void (*listenMouse)(EditorDocument*, Rml::Element*, Rml::EventId, EditorListener::Callback, bool) = nullptr;
    void (*deleteListener)(EditorListener*) = nullptr;
    void (*setClass)(Rml::Element*, const Rml::String*, bool) = nullptr;
    Rml::Vector2f (*touchPosition)(const Rml::Event*) = nullptr;
    Rml::Vector2f (*mousePosition)(const Rml::Event*) = nullptr;
    int (*eventInt)(const Rml::Event*, const Rml::String*, const int*) = nullptr;
    dusk::ui::ControlAnchor (*dockAnchor)(dusk::ui::ControlRect, dusk::ui::ControlLayoutSize) = nullptr;
    void (*applyDock)(Rml::Element*, dusk::ui::ControlAnchor) = nullptr;
    void (*applyTransform)(Rml::Element*, std::optional<float>*, float) = nullptr;
} s_editorApi;
constexpr std::array kExtraHandles = {EditHandle::Left, EditHandle::Right, EditHandle::Top,
    EditHandle::Bottom, EditHandle::TopLeft, EditHandle::TopRight, EditHandle::BottomLeft, EditHandle::BottomRight};
struct EditorState {
    NativeEditor* editor = nullptr;
    std::array<NativeEditor::EditElement, touch::Count> elements{};
    std::array<dusk::ui::ControlProps, touch::Count> defaults{};
    std::array<std::optional<dusk::ui::ControlProps>, touch::Count> working{};
    NativeEditor::PointerEdit pointer{};
    size_t selected = touch::Count;
    Rml::Element* frame = nullptr;
    std::array<Rml::Element*, 8> handles{};
    std::optional<dusk::ui::ControlRect> frameBox;
    std::vector<EditorListener*> captureListeners;
    std::vector<EditorDocument*> children;
    std::array<std::string, touch::Count> savePrevious;
    bool savePending = false;
    bool saveFailed = false;
};
std::vector<std::unique_ptr<EditorState>> s_editors;
EditorState* editor_state(NativeEditor* editor) {
    for (const auto& state : s_editors) if (state->editor == editor) return state.get();
    return nullptr;
}
void editor_class(Rml::Element* element, const char* name, bool active) {
    if (!element) return;
    const Rml::String value = name;
    s_editorApi.setClass(element, &value, active);
}
dusk::ui::ControlProps extra_editor_props(const EditorState& state, size_t i) {
    return state.working[i].value_or(state.defaults[i]);
}
void draw_extra_editor(EditorState& state) {
    dusk::ui::ControlLayoutSize size;
    if (!extra_viewport(state.editor->mDocument, size)) return;
    for (size_t i = 0; i < touch::Count; ++i) {
        auto& element = state.elements[i];
        const auto layout = dusk::ui::resolve_control_layout(extra_editor_props(state, i), size);
        element.layout.visualRect = layout.visual;
        element.layout.layoutScale = layout.scale;
        s_touchApi.applyBox(element.root, &element.layout.appliedBox, layout.box);
        s_editorApi.applyTransform(element.root, &element.layout.appliedTransform, layout.scale);
        s_editorApi.applyDock(element.root, s_editorApi.dockAnchor(layout.visual, size));
        editor_class(element.root, "editor-selected", state.selected == i);
    }
    const bool selected = state.selected < touch::Count;
    editor_class(state.frame, "visible", selected);
    if (selected) s_touchApi.applyBox(state.frame, &state.frameBox, *state.elements[state.selected].layout.visualRect);
    else state.frameBox.reset();
}

// Geometry follows the pinned native editor: 6dp drag threshold, 36dp minimum,
// edge resizing, anchored corner scaling, viewport clamping and docking.
#include "touch_editor_geometry.inc"

bool begin_extra_edit(EditorState& state, size_t i, EditHandle handle,
    Rml::Vector2f position, bool touch, SDL_FingerID id) {
    if (i >= touch::Count || state.pointer.active || state.editor->mPointerEdit.active) return false;
    dusk::ui::ControlLayoutSize size;
    if (!extra_viewport(state.editor->mDocument, size)) return false;
    const auto props = extra_editor_props(state, i);
    const float scale = s_touchApi.scale(s_touchApi.context(state.editor->mDocument));
    state.pointer = {.index = i, .touchId = id, .startPointerDp = position / scale,
        .startVisual = dusk::ui::resolve_control_layout(props, size).visual,
        .startProps = props, .storedProps = state.working[i], .handle = handle, .active = true, .touch = touch};
    state.selected = i;
    // Selecting one of our buttons deselects the native selection only. Never
    // repurpose its slots, pointer state or metadata for our gestures.
    if (state.editor->mSelectedIndex < state.editor->mElements.size())
        editor_class(state.editor->mElements[state.editor->mSelectedIndex].root, "editor-selected", false);
    // The native sentinel (9) can be a valid ID in another mod's extended metadata.
    state.editor->mSelectedIndex = static_cast<size_t>(-1);
    editor_class(state.editor->mSelectionFrame, "visible", false);
    draw_extra_editor(state);
    return true;
}
void move_extra_edit(EditorState& state, Rml::Vector2f position) {
    auto& edit = state.pointer;
    dusk::ui::ControlLayoutSize size;
    if (!edit.active || !extra_viewport(state.editor->mDocument, size)) return;
    const auto point = position / s_touchApi.scale(s_touchApi.context(state.editor->mDocument));
    const auto delta = point - edit.startPointerDp;
    if (!edit.dragging && delta.x * delta.x + delta.y * delta.y < 36.f) return;
    edit.dragging = true;
    auto props = edit.startProps;
    auto rect = extra_edit_rect(edit, point, size, props);
    const float minW = std::min(36.f, size.w), minH = std::min(36.f, size.h);
    rect.w = std::clamp(rect.w, minW, size.w);
    rect.h = std::clamp(rect.h, minH, size.h);
    rect.l = std::clamp(rect.l, 0.f, std::max(0.f, size.w - rect.w));
    rect.t = std::clamp(rect.t, 0.f, std::max(0.f, size.h - rect.h));
    if (edit.handle >= EditHandle::TopLeft) props.scale = std::max(rect.w / std::max(edit.startProps.w, 1.f), 0.25f);
    else if (edit.handle == EditHandle::Left || edit.handle == EditHandle::Right) props.w = rect.w / std::max(props.scale, 0.25f);
    else if (edit.handle == EditHandle::Top || edit.handle == EditHandle::Bottom) props.h = rect.h / std::max(props.scale, 0.25f);
    state.working[edit.index] = dusk::ui::encode_control_props(rect, size, props, s_editorApi.dockAnchor(rect, size));
    draw_extra_editor(state);
}
enum class ExtraEditEvent { Down, Move, Up, Cancel };
void extra_editor_event(EditorState& state, Rml::Event& event, ExtraEditEvent kind, bool touch) {
    if (state.editor->mClosed || state.editor->mPendingClose) return;
    const auto id = touch ? s_touchApi.finger(&event) : 0;
    if (!touch && (kind == ExtraEditEvent::Down || kind == ExtraEditEvent::Up)) {
        const Rml::String button = "button";
        const int fallback = -1;
        if (s_editorApi.eventInt(&event, &button, &fallback) != 0) return;
    }
    if (kind != ExtraEditEvent::Down) {
        if (!state.pointer.active || state.pointer.touch != touch || (touch && state.pointer.touchId != id)) return;
        if (kind == ExtraEditEvent::Move) move_extra_edit(state, touch ? s_editorApi.touchPosition(&event) : s_editorApi.mousePosition(&event));
        else {
            if (kind == ExtraEditEvent::Cancel) state.working[state.pointer.index] = state.pointer.storedProps;
            state.pointer = {};
            draw_extra_editor(state);
        }
        s_touchApi.stop(&event);
        return;
    }
    auto* target = s_touchApi.target(&event);
    for (; target && target != state.editor->mRoot; target = s_touchApi.parent(target)) {
        size_t index = touch::Count;
        EditHandle handle = EditHandle::Move;
        for (size_t i = 0; i < touch::Count; ++i) if (target == state.elements[i].root) index = i;
        for (size_t i = 0; i < state.handles.size(); ++i) if (target == state.handles[i]) {
            index = state.selected; handle = kExtraHandles[i];
        }
        if (index < touch::Count) {
            begin_extra_edit(state, index, handle, touch ? s_editorApi.touchPosition(&event) : s_editorApi.mousePosition(&event), touch, id);
            s_touchApi.stop(&event);
            return;
        }
    }
    // Any foreign/native control (including unknown mod buttons) gets the
    // original event and editor state. Only clear our own idle selection.
    if (!state.pointer.active) { state.selected = touch::Count; draw_extra_editor(state); }
}
void detach_editor_listeners(EditorState& state) {
    auto& listeners = static_cast<EditorDocument*>(state.editor)->mListeners;
    for (auto* capture : state.captureListeners) {
        const auto it = std::find_if(listeners.begin(), listeners.end(),
            [capture](const auto& listener) { return listener.get() == capture; });
        if (it != listeners.end()) s_editorApi.deleteListener(it->release());
    }
    state.captureListeners.clear();
}
EditorState* attach_extra_editor(NativeEditor* editor) {
    if (auto* state = editor_state(editor)) return state;
    dusk::ui::ControlLayoutSize size;
    if (!s_touchEditorAvailable || editor->mClosed || editor->mPendingClose || !editor->mRoot ||
        !extra_viewport(editor->mDocument, size)) return nullptr;
    const Rml::String tag = "div";
    auto root = s_touchApi.create(editor->mDocument, &tag);
    if (!root) return nullptr;
    extra_property(root.get(), "position", "absolute");
    extra_property(root.get(), "width", "100%");
    extra_property(root.get(), "height", "100%");
    extra_property(root.get(), "left", "0dp");
    extra_property(root.get(), "top", "0dp");
    extra_property(root.get(), "pointer-events", "none");
    auto owned = std::make_unique<EditorState>();
    auto* state = owned.get();
    state->editor = editor;
    std::string fragment;
    for (size_t i = 0; i < touch::Count; ++i) {
        state->defaults[i] = default_extra_props(i, size);
        state->working[i] = saved_extra_props(i, size);
        fragment += "<button class='control trigger'><span>";
        fragment += touch::Labels[i];
        fragment += "</span></button>";
    }
    fragment += "<selection-frame>";
    for (const char* style : {"edge horizontal left", "edge horizontal right", "edge vertical top", "edge vertical bottom",
             "corner top left", "corner top right", "corner bottom left", "corner bottom right"}) {
        fragment += "<resize-handle class='"; fragment += style; fragment += "' />";
    }
    fragment += "</selection-frame>";
    s_touchApi.setRml(root.get(), &fragment);
    auto* element = s_touchApi.append(editor->mRoot, std::move(root), true);
    if (!element) return nullptr;
    for (size_t i = 0; i < touch::Count; ++i) state->elements[i].root = s_touchApi.child(element, int(i));
    state->frame = s_touchApi.child(element, int(touch::Count));
    for (size_t i = 0; i < state->handles.size(); ++i) state->handles[i] = s_touchApi.child(state->frame, int(i));
    s_editors.push_back(std::move(owned));
    const char* touchEvents[] = {"touchstart", "touchmove", "touchend", "touchcancel"};
    for (size_t i = 0; i < 4; ++i) {
        const Rml::String name = touchEvents[i];
        s_editorApi.listenTouch(editor, editor->mRoot, &name,
            [state, i](Rml::Event& event) { extra_editor_event(*state, event, static_cast<ExtraEditEvent>(i), true); }, true);
        state->captureListeners.push_back(editor->mListeners.back().get());
    }
    const Rml::EventId mouseEvents[] = {Rml::EventId::Mousedown, Rml::EventId::Mousemove, Rml::EventId::Mouseup};
    for (size_t i = 0; i < 3; ++i) {
        s_editorApi.listenMouse(editor, editor->mRoot, mouseEvents[i],
            [state, i](Rml::Event& event) { extra_editor_event(*state, event, static_cast<ExtraEditEvent>(i), false); }, true);
        state->captureListeners.push_back(editor->mListeners.back().get());
    }
    return state;
}
void layout_extra_editor(ModContext*, void* args, void*, void*) {
    if (auto* state = attach_extra_editor(mods::arg<NativeEditor*>(args, 0))) draw_extra_editor(*state);
}
void reset_extra_editor(ModContext*, void* args, void*, void*) {
    if (auto* state = editor_state(mods::arg<NativeEditor*>(args, 0))) {
        state->working = {}; state->pointer = {}; state->selected = touch::Count;
        draw_extra_editor(*state);
    }
}
HookAction extra_editor_push(ModContext*, void* args, void*, void*) {
    auto* top = s_editorApi.top();
    for (const auto& state : s_editors) if (state->editor == top) {
        auto* document = mods::arg<std::unique_ptr<EditorDocument>*>(args, 0)->get();
        if (document && document != top) state->children.push_back(document);
    }
    return HOOK_CONTINUE;
}
HookAction extra_editor_delete(ModContext*, void* args, void*, void*) {
    auto* document = mods::arg<EditorDocument*>(args, 0);
    for (const auto& state : s_editors) {
        std::erase(state->children, document);
        if (state->editor == document) detach_editor_listeners(*state);
    }
    std::erase_if(s_editors, [document](const auto& state) { return state->editor == document; });
    return HOOK_CONTINUE;
}
bool persist_extra_editor(EditorState& state) {
    std::array<std::string, touch::Count> values;
    for (size_t i = 0; i < touch::Count; ++i) {
        const auto props = extra_editor_props(state, i);
        if (!valid_extra_props(props)) return false;
        values[i] = encode_extra_props(props);
        char buffer[256]{};
        if (svc_config->get_string(mod_ctx, s_buttons[i].layout, buffer, sizeof(buffer), nullptr) != MOD_OK) return false;
        state.savePrevious[i] = buffer;
    }
    for (size_t i = 0; i < touch::Count; ++i) {
        if (svc_config->set_string(mod_ctx, s_buttons[i].layout, values[i].c_str()) != MOD_OK) {
            for (size_t j = 0; j < i; ++j) svc_config->set_string(mod_ctx, s_buttons[j].layout, state.savePrevious[j].c_str());
            svc_log->warn(mod_ctx, "Could not save Touch Buttons layout");
            return false;
        }
    }
    return true;
}
HookAction save_extra_editor(ModContext*, void* args, void*, void*) {
    auto* state = editor_state(mods::arg<NativeEditor*>(args, 0));
    if (!state) return HOOK_CONTINUE;
    state->saveFailed = !persist_extra_editor(*state);
    state->savePending = !state->saveFailed;
    // Only a real save error cancels the action. Normal saves retain every
    // following pre-hook, replacement and post-hook, with untouched host data.
    return state->saveFailed ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}
void after_save_extra_editor(ModContext*, void* args, void*, void*) {
    auto* editor = mods::arg<NativeEditor*>(args, 0);
    auto* state = editor_state(editor);
    if (!state) return;
    const bool saved = editor->mClosed || editor->mPendingClose;
    if (state->savePending && !saved) {
        // Another mod vetoed Save after our pre-hook: undo our writes too.
        for (size_t i = 0; i < touch::Count; ++i)
            svc_config->set_string(mod_ctx, s_buttons[i].layout, state->savePrevious[i].c_str());
    } else if (!state->savePending && !state->saveFailed && saved) {
        // A preceding replacement saved and closed without running our pre-hook.
        persist_extra_editor(*state);
    }
    state->savePending = state->saveFailed = false;
}
void close_extra_editor() {
    s_touchEditorAvailable = false;
    for (const auto& state : s_editors) {
        for (auto* child : state->children) s_editorApi.hide(child, true);
        detach_editor_listeners(*state);
        s_editorApi.hide(state->editor, true);
    }
    if (!s_editors.empty()) s_editorApi.uncover();
    s_editors.clear();
}
void open_native_touch_editor(ModContext*, void*) {
    if (!s_touchEditorAvailable) return;
    void* storage = ::operator new(sizeof(NativeEditor));
    auto* editor = static_cast<NativeEditor*>(storage);
    try { s_editorApi.construct(editor); }
    catch (...) { ::operator delete(storage); throw; }
    std::unique_ptr<EditorDocument> owned(editor);
    if (!editor->mDocument) return;
    auto* parent = s_editorApi.top();
    s_editorApi.push(std::move(owned), true, false);
    if (parent) parent->cover();
}

struct EditorHookBinding {
    const char* name;
    void* (*address)();
    ModResult (*install)();
    ModResult (*remove)();
    HookPreFn pre;
    HookPostFn post;
};
template<class Hook> EditorHookBinding editor_hook(
    const char* name, HookPreFn pre, HookPostFn post) {
    return {name, Hook::resolved_target,
        [] { return mods::hook_install<Hook>(svc_hook); },
        [] { return mods::hook_uninstall<Hook>(svc_hook); }, pre, post};
}
ModResult install_extra_editor(ModError*) {
    s_touchEditorAvailable = false;
    if (!SERVICE_HAS(svc_hook, HookService, uninstall) || !svc_hook->uninstall) {
        svc_log->warn(mod_ctx, "Touch editor unavailable: hook rollback is unsupported");
        return MOD_OK;
    }
    if (!resolve_extra("_ZN4dusk2ui19TouchControlsEditorC1Ev", s_editorApi.construct) ||
        !resolve_extra("dusk::ui::push_document", s_editorApi.push) ||
        !resolve_extra("dusk::ui::top_document", s_editorApi.top) ||
        !resolve_extra("dusk::ui::uncover_top_document", s_editorApi.uncover) ||
        !resolve_extra("dusk::ui::Document::hide", s_editorApi.hide) ||
        !resolve_extra("_ZN4dusk2ui8Document6listenEPN3Rml7ElementERKNSt6__ndk112basic_stringIcNS5_11char_traitsIcEENS5_9allocatorIcEEEENS5_8functionIFvRNS2_5EventEEEEb", s_editorApi.listenTouch) ||
        !resolve_extra("_ZN4dusk2ui8Document6listenEPN3Rml7ElementENS2_7EventIdENSt6__ndk18functionIFvRNS2_5EventEEEEb", s_editorApi.listenMouse) ||
        !resolve_extra("_ZN4dusk2ui19ScopedEventListenerD0Ev", s_editorApi.deleteListener) ||
        !resolve_extra("_ZN3Rml7Element8SetClassERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEb", s_editorApi.setClass) ||
        !resolve_extra("dusk::ui::touch_event_position", s_editorApi.touchPosition) ||
        !resolve_extra("dusk::ui::mouse_event_position", s_editorApi.mousePosition) ||
        !resolve_extra("_ZNK3Rml5Event12GetParameterIiEET_RKNSt6__ndk112basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEERKS2_", s_editorApi.eventInt) ||
        !resolve_extra("dusk::ui::touch_control_dock_anchor", s_editorApi.dockAnchor) ||
        !resolve_extra("dusk::ui::apply_control_dock_classes", s_editorApi.applyDock) ||
        !resolve_extra("dusk::ui::apply_control_transform_if_changed", s_editorApi.applyTransform)) return MOD_OK;
    const std::array bindings = {
        editor_hook<ExtraEditorLayouts>("sync_control_layouts", nullptr, layout_extra_editor),
        editor_hook<ExtraEditorReset>("reset_working_layout", nullptr, reset_extra_editor),
        editor_hook<ExtraEditorSave>("save_layout", save_extra_editor, after_save_extra_editor),
        editor_hook<ExtraEditorDelete>("Document destructor", extra_editor_delete, nullptr),
        editor_hook<ExtraEditorPush>("push_document", extra_editor_push, nullptr),
    };
    // Validate the complete optional adapter before changing any host function.
    // A missing editor symbol must not prevent the rest of Dawnlight loading.
    for (const auto& binding : bindings) {
        if (!binding.address()) {
            const auto message = std::string("Touch editor unavailable: missing hook ") + binding.name;
            svc_log->warn(mod_ctx, message.c_str());
            return MOD_OK;
        }
    }
    size_t installed = 0;
    for (const auto& binding : bindings) {
        auto result = binding.install();
        if (result == MOD_OK) {
            ++installed;
            if (binding.pre) result = svc_hook->add_pre(mod_ctx, binding.address(), binding.pre, nullptr);
            if (result == MOD_OK && binding.post)
                result = svc_hook->add_post(mod_ctx, binding.address(), binding.post, nullptr);
        }
        if (result != MOD_OK) {
            const auto message = std::string("Touch editor unavailable: failed hook ") + binding.name +
                " (" + std::to_string(result) + "); Dawnlight remains active";
            svc_log->warn(mod_ctx, message.c_str());
            while (installed) bindings[--installed].remove();
            return MOD_OK;
        }
    }
    s_touchEditorAvailable = true;
    return MOD_OK;
}
