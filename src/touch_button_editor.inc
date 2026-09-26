// Extend every native TouchControlsEditor without changing its nine-slot ABI.
// All controls share one document and working layout. Native gestures operate
// on one bank at a time; the inactive bank lives in mod-owned storage.
using NativeEditor = dusk::ui::TouchControlsEditor;
using LayoutSpan = std::span<const dusk::ui::TouchLayoutControlInfo>;
using EditorDocument = dusk::ui::Document;
using EditorListener = dusk::ui::ScopedEventListener;
DEFINE_HOOK_SYMBOL("dusk::ui::touch_layout_controls", LayoutSpan(), ExtraEditorControls);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::sync_control_layouts", void(NativeEditor*), ExtraEditorLayouts);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::sync_selection_frame", void(NativeEditor*), ExtraEditorFrame);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::begin_edit",
    bool(NativeEditor*, size_t, NativeEditor::EditHandle, Rml::Vector2f, bool, SDL_FingerID), ExtraEditorBegin);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::continue_edit", bool(NativeEditor*, Rml::Vector2f), ExtraEditorMove);
// end_edit is inlined in Android v2.0.1; scope its restoration call instead.
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::restore_active_control", void(NativeEditor*), ExtraEditorRestore);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::request_reset", void(NativeEditor*), ExtraEditorReset);
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
    void (*bindControls)(NativeEditor*) = nullptr;
    void (*listenTouch)(EditorDocument*, Rml::Element*, const Rml::String*, EditorListener::Callback, bool) = nullptr;
    void (*listenMouse)(EditorDocument*, Rml::Element*, Rml::EventId, EditorListener::Callback, bool) = nullptr;
    void (*deleteListener)(EditorListener*) = nullptr;
    void (*setClass)(Rml::Element*, const Rml::String*, bool) = nullptr;
} s_editorApi;
constexpr const char* kExtraEditorIds[] = {"dawnlight-lb", "dawnlight-up", "dawnlight-down", "dawnlight-left", "dawnlight-right", "dawnlight-midna"};
static_assert(touch::Count <= dusk::ui::kTouchLayoutControlCount);
struct EditorState {
    NativeEditor* editor = nullptr;
    decltype(NativeEditor::mElements) inactive{};
    std::array<dusk::ui::TouchLayoutControlInfo, touch::Count> controls{};
    std::array<EditorListener*, 2> captureListeners{};
    std::vector<EditorDocument*> children;
    bool extras = false;
};
std::vector<std::unique_ptr<EditorState>> s_editors;
EditorState* s_editorScope = nullptr;
std::vector<EditorState*> s_editorScopes;

EditorState* editor_state(NativeEditor* editor) {
    for (const auto& state : s_editors) if (state->editor == editor) return state.get();
    return nullptr;
}
struct EditorScope {
    EditorState* previous = s_editorScope;
    explicit EditorScope(EditorState* state) { s_editorScope = state; }
    ~EditorScope() { s_editorScope = previous; }
};
void swap_editor_bank(EditorState& state, bool extras) {
    if (state.extras == extras) return;
    std::swap(state.editor->mElements, state.inactive);
    state.extras = extras;
}
void select_editor_bank(EditorState& state, Rml::Event& event) {
    auto* editor = state.editor;
    // Another finger cannot change the bank under an active native gesture.
    if (editor->mPointerEdit.active || editor->mClosed || editor->mPendingClose) return;
    auto* target = s_touchApi.target(&event);
    for (; target && target != editor->mRoot; target = s_touchApi.parent(target)) {
        for (bool extras : {false, true}) {
            const auto& elements = extras == state.extras ? editor->mElements : state.inactive;
            for (const auto& element : elements) if (element.root && element.root == target) {
                if (extras != state.extras) {
                    const Rml::String selected = "editor-selected";
                    for (const auto& old : editor->mElements)
                        if (old.root) s_editorApi.setClass(old.root, &selected, false);
                    swap_editor_bank(state, extras);
                    editor->mSelectedIndex = dusk::ui::kTouchLayoutControlCount;
                    editor->mAppliedSelectionFrame.reset();
                }
                return;
            }
        }
    }
    // A resize handle or toolbar command preserves the current bank.
}
void detach_editor_listeners(EditorState& state, bool restoreBank = true) {
    // Document::hide is asynchronous with respect to destruction. Explicitly
    // destroy our callbacks while the module is still loaded.
    auto& listeners = state.editor->mListeners;
    for (auto* capture : state.captureListeners) {
        const auto it = std::find_if(listeners.begin(), listeners.end(),
            [capture](const auto& listener) { return listener.get() == capture; });
        if (it != listeners.end()) {
            auto* listener = it->release();
            // Leave a null slot for the host vector to destroy. Erasing here
            // would instantiate the final host class's deleter in the mod.
            s_editorApi.deleteListener(listener);
        }
    }
    state.captureListeners = {};
    if (restoreBank) {
        swap_editor_bank(state, false);
        state.editor->mPointerEdit = {};
        state.editor->mSelectedIndex = dusk::ui::kTouchLayoutControlCount;
    }
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
        state->controls[i] = {kExtraEditorIds[i], kExtraEditorIds[i], default_extra_props(i, size),
            dusk::ui::Control::COUNT, false};
        fragment += "<button class='control trigger'><span>";
        fragment += touch::Labels[i];
        fragment += "</span></button>";
        editor->mWorkingLayout.controls[kExtraEditorIds[i]] = saved_extra_props(i, size);
    }
    s_touchApi.setRml(root.get(), &fragment);
    auto* element = s_touchApi.append(editor->mRoot, std::move(root), true);
    if (!element) return nullptr;
    for (size_t i = 0; i < touch::Count; ++i) state->inactive[i].root = s_touchApi.child(element, int(i));
    s_editors.push_back(std::move(owned));
    // Capture runs before the native target listeners, including nested spans.
    const Rml::String touchStart = "touchstart";
    s_editorApi.listenTouch(editor, editor->mRoot, &touchStart,
        [state](Rml::Event& event) { select_editor_bank(*state, event); }, true);
    state->captureListeners[0] = editor->mListeners.back().get();
    s_editorApi.listenMouse(editor, editor->mRoot, Rml::EventId::Mousedown,
        [state](Rml::Event& event) { select_editor_bank(*state, event); }, true);
    state->captureListeners[1] = editor->mListeners.back().get();
    // Reuse native event callbacks and gestures; their indices stay within 0..8.
    EditorScope scope(state);
    swap_editor_bank(*state, true);
    s_editorApi.bindControls(editor);
    swap_editor_bank(*state, false);
    return state;
}
HookAction enter_extra_editor(ModContext*, void* args, void*, void*) {
    s_editorScopes.push_back(s_editorScope);
    s_editorScope = editor_state(mods::arg<NativeEditor*>(args, 0));
    return HOOK_CONTINUE;
}
void leave_extra_editor(ModContext*, void*, void*, void*) {
    s_editorScope = s_editorScopes.back();
    s_editorScopes.pop_back();
}
void extra_editor_controls(ModContext*, void*, void* result, void*) {
    if (s_editorScope && s_editorScope->extras) *static_cast<LayoutSpan*>(result) = s_editorScope->controls;
}
HookAction layout_extra_editor(ModContext*, void* args, void*, void*) {
    auto* editor = mods::arg<NativeEditor*>(args, 0);
    auto* state = attach_extra_editor(editor);
    if (!state) return HOOK_CONTINUE;
    EditorScope scope(state);
    const bool activeBank = state->extras;
    // Update every visible button, without changing the active selection/drag.
    swap_editor_bank(*state, false);
    ExtraEditorLayouts::g_orig(editor);
    swap_editor_bank(*state, true);
    ExtraEditorLayouts::g_orig(editor);
    swap_editor_bank(*state, activeBank);
    return HOOK_SKIP_ORIGINAL;
}
HookAction extra_editor_push(ModContext*, void* args, void*, void*) {
    if (s_editorScope) {
        auto* document = mods::arg<std::unique_ptr<EditorDocument>*>(args, 0)->get();
        if (document && document != s_editorScope->editor) s_editorScope->children.push_back(document);
    }
    return HOOK_CONTINUE;
}
HookAction extra_editor_delete(ModContext*, void* args, void*, void*) {
    auto* document = mods::arg<EditorDocument*>(args, 0);
    for (const auto& state : s_editors) {
        std::erase(state->children, document);
        if (state->editor == document) detach_editor_listeners(*state, false);
    }
    std::erase_if(s_editors, [document](const auto& state) { return state->editor == document; });
    return HOOK_CONTINUE;
}
HookAction save_extra_editor(ModContext*, void* args, void*, void*) {
    auto* editor = mods::arg<NativeEditor*>(args, 0);
    auto* state = editor_state(editor);
    if (!state) return HOOK_CONTINUE;
    std::array<std::string, touch::Count> values, previous;
    for (size_t i = 0; i < touch::Count; ++i) {
        const auto found = editor->mWorkingLayout.controls.find(kExtraEditorIds[i]);
        const auto props = found != editor->mWorkingLayout.controls.end() ? found->second : state->controls[i].props;
        if (!valid_extra_props(props)) return HOOK_SKIP_ORIGINAL;
        values[i] = encode_extra_props(props);
        char buffer[256]{};
        if (svc_config->get_string(mod_ctx, s_buttons[i].layout, buffer, sizeof(buffer), nullptr) != MOD_OK)
            return HOOK_SKIP_ORIGINAL;
        previous[i] = buffer;
    }
    for (size_t i = 0; i < touch::Count; ++i) {
        if (svc_config->set_string(mod_ctx, s_buttons[i].layout, values[i].c_str()) != MOD_OK) {
            for (size_t j = 0; j < i; ++j) svc_config->set_string(mod_ctx, s_buttons[j].layout, previous[j].c_str());
            svc_log->warn(mod_ctx, "Could not save Touch Buttons layout");
            return HOOK_SKIP_ORIGINAL;
        }
    }
    // Native Save persists the vanilla controls and closes the same editor.
    // Keep Dawnlight keys in their existing storage, including migrated layouts.
    auto working = editor->mWorkingLayout;
    for (const auto* id : kExtraEditorIds) editor->mWorkingLayout.controls.erase(id);
    ExtraEditorSave::g_orig(editor);
    editor->mWorkingLayout = std::move(working);
    return HOOK_SKIP_ORIGINAL;
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
    s_editorScope = nullptr;
    s_editorScopes.clear();
}
void open_native_touch_editor(ModContext*, void*) {
    if (!s_touchEditorAvailable) return;
    // This is only a shortcut to the same editor used by Dusklight's settings.
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
        !resolve_extra("dusk::ui::TouchControlsEditor::bind_control_events", s_editorApi.bindControls) ||
        !resolve_extra("_ZN4dusk2ui8Document6listenEPN3Rml7ElementERKNSt6__ndk112basic_stringIcNS5_11char_traitsIcEENS5_9allocatorIcEEEENS5_8functionIFvRNS2_5EventEEEEb", s_editorApi.listenTouch) ||
        !resolve_extra("_ZN4dusk2ui8Document6listenEPN3Rml7ElementENS2_7EventIdENSt6__ndk18functionIFvRNS2_5EventEEEEb", s_editorApi.listenMouse) ||
        !resolve_extra("_ZN4dusk2ui19ScopedEventListenerD0Ev", s_editorApi.deleteListener) ||
        !resolve_extra("_ZN3Rml7Element8SetClassERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEb", s_editorApi.setClass)) return MOD_OK;
    const std::array bindings = {
        editor_hook<ExtraEditorControls>("touch_layout_controls", nullptr, extra_editor_controls),
        editor_hook<ExtraEditorLayouts>("sync_control_layouts", layout_extra_editor, nullptr),
        editor_hook<ExtraEditorFrame>("sync_selection_frame", enter_extra_editor, leave_extra_editor),
        editor_hook<ExtraEditorBegin>("begin_edit", enter_extra_editor, leave_extra_editor),
        editor_hook<ExtraEditorMove>("continue_edit", enter_extra_editor, leave_extra_editor),
        editor_hook<ExtraEditorRestore>("restore_active_control", enter_extra_editor, leave_extra_editor),
        editor_hook<ExtraEditorReset>("request_reset", enter_extra_editor, leave_extra_editor),
        editor_hook<ExtraEditorSave>("save_layout", save_extra_editor, nullptr),
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
