// Reuse the running Dusklight TouchControlsEditor, including its RML/CSS,
// drag threshold, resize handles, docking, reset confirmation and navigation.
// Only our instance sees five alternative controls. The host's nine-element
// array stays unchanged, and vanilla editor instances use their original data.
using NativeEditor = dusk::ui::TouchControlsEditor;
using LayoutSpan = std::span<const dusk::ui::TouchLayoutControlInfo>;
using EditorDocument = dusk::ui::Document;
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControls::sync_visibility", void(NativeTouch*), ExtraEditorBackground);
DEFINE_HOOK_SYMBOL("dusk::ui::touch_layout_controls", LayoutSpan(), ExtraEditorControls);
DEFINE_HOOK_SYMBOL("dusk::ui::touch_controls_rml_fragment", std::string_view(), ExtraEditorFragment);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::sync_control_layouts", void(NativeEditor*), ExtraEditorLayouts);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::sync_selection_frame", void(NativeEditor*), ExtraEditorFrame);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::begin_edit",
    bool(NativeEditor*, size_t, NativeEditor::EditHandle, Rml::Vector2f, bool, SDL_FingerID), ExtraEditorBegin);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::continue_edit", bool(NativeEditor*, Rml::Vector2f), ExtraEditorMove);
// end_edit has no hookable entry in Android v2.0.1. Only its cancel branch
// needs our metadata: scope the actual layout-restoration operation instead.
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::restore_active_control", void(NativeEditor*), ExtraEditorRestore);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::request_reset", void(NativeEditor*), ExtraEditorReset);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::save_layout", void(NativeEditor*), ExtraEditorSave);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui8DocumentD2Ev", void(EditorDocument*), ExtraEditorDelete);
// C++ passes non-trivial unique_ptr values indirectly and references as
// pointers. This ABI signature avoids copying unique_ptr in the hook trampoline.
DEFINE_HOOK_SYMBOL("dusk::ui::push_document",
    EditorDocument*(std::unique_ptr<EditorDocument>*, bool, bool), ExtraEditorPush);

struct EditorApi {
    void (*construct)(NativeEditor*) = nullptr;
    EditorDocument& (*push)(std::unique_ptr<EditorDocument>, bool, bool) = nullptr;
    EditorDocument* (*top)() = nullptr;
    void (*uncover)() = nullptr;
    void (*hide)(EditorDocument*, bool) = nullptr;
    void (*cancel)(NativeEditor*) = nullptr;
    void (*clearTouch)(NativeTouch*) = nullptr;
    void (*showTouch)(NativeTouch*) = nullptr;
    void (*setPseudo)(Rml::Element*, const Rml::String*, bool) = nullptr;
} s_editorApi;
NativeEditor* s_extraEditor = nullptr;
unsigned s_editorScope = 0;
bool s_editorConstructing = false;
std::array<dusk::ui::TouchLayoutControlInfo, touch::Count> s_editorControls{};
std::string s_editorFragment;
std::vector<EditorDocument*> s_editorChildren;
constexpr const char* kExtraEditorIds[] = {"dawnlight-zl", "dawnlight-up", "dawnlight-down", "dawnlight-left", "dawnlight-right"};
static_assert(touch::Count <= dusk::ui::kTouchLayoutControlCount);

bool editor_background_active() {
    if (!s_extraEditor || s_extraEditor->mClosed || s_extraEditor->mPendingClose) return false;
    auto* top = s_editorApi.top();
    return top == s_extraEditor || std::find(s_editorChildren.begin(), s_editorChildren.end(), top) !=
        s_editorChildren.end();
}
void sync_editor_background(NativeTouch* controls) {
    if (!editor_background_active()) return;
    // Native update hides individual controls while input is suppressed, even
    // when the document itself is shown. Restore the reference AFTER that update.
    const Rml::String hidden = "hidden";
    for (const auto control : {dusk::ui::Control::L, dusk::ui::Control::R, dusk::ui::Control::Z,
             dusk::ui::Control::A, dusk::ui::Control::B, dusk::ui::Control::X, dusk::ui::Control::Y}) {
        auto* element = controls->mControlElements[static_cast<size_t>(control)].root;
        if (element) s_editorApi.setPseudo(element, &hidden, false);
    }
    if (controls->mActionBar) s_editorApi.setPseudo(controls->mActionBar, &hidden, false);
    // Skip is a cutscene action at the same default location as R; it must not
    // cover the gameplay reference while arranging extra buttons.
    auto* skip = controls->mControlElements[static_cast<size_t>(dusk::ui::Control::SKIP)].root;
    if (skip) s_editorApi.setPseudo(skip, &hidden, true);
}
HookAction show_editor_background(ModContext*, void* args, void*, void*) {
    if (!editor_background_active()) return HOOK_CONTINUE;
    auto* controls = mods::arg<NativeTouch*>(args, 0);
    // Keep the real native document and saved layout visible behind our editor.
    // Input stays suppressed: background buttons must not trigger game actions,
    // stick motion, or native menu commands while arranging the extra controls.
    controls->mWasSuppressed = true;
    s_editorApi.clearTouch(controls);
    s_editorApi.showTouch(controls);
    return HOOK_SKIP_ORIGINAL;
}
HookAction enter_extra_editor(ModContext*, void* args, void*, void*) {
    if (mods::arg<NativeEditor*>(args, 0) == s_extraEditor) ++s_editorScope;
    return HOOK_CONTINUE;
}
void leave_extra_editor(ModContext*, void* args, void*, void*) {
    if (mods::arg<NativeEditor*>(args, 0) == s_extraEditor && s_editorScope) --s_editorScope;
}
void extra_editor_controls(ModContext*, void*, void* result, void*) {
    if (s_editorScope) *static_cast<LayoutSpan*>(result) = s_editorControls;
}
void extra_editor_fragment(ModContext*, void*, void* result, void*) {
    if (s_editorConstructing) *static_cast<std::string_view*>(result) = s_editorFragment;
}
HookAction extra_editor_push(ModContext*, void* args, void*, void*) {
    if (s_editorScope && !s_editorConstructing) {
        auto* document = mods::arg<std::unique_ptr<EditorDocument>*>(args, 0)->get();
        if (document && document != s_extraEditor) s_editorChildren.push_back(document);
    }
    return HOOK_CONTINUE;
}
HookAction extra_editor_delete(ModContext*, void* args, void*, void*) {
    auto* document = mods::arg<EditorDocument*>(args, 0);
    std::erase(s_editorChildren, document);
    if (document == s_extraEditor) s_extraEditor = nullptr;
    return HOOK_CONTINUE;
}
HookAction save_extra_editor(ModContext*, void* args, void*, void*) {
    auto* editor = mods::arg<NativeEditor*>(args, 0);
    if (editor != s_extraEditor) return HOOK_CONTINUE;
    std::array<std::string, touch::Count> values;
    for (size_t i = 0; i < touch::Count; ++i) {
        const auto found = editor->mWorkingLayout.controls.find(kExtraEditorIds[i]);
        const auto props = found != editor->mWorkingLayout.controls.end() ? found->second : s_editorControls[i].props;
        if (!valid_extra_props(props)) return HOOK_SKIP_ORIGINAL;
        values[i] = encode_extra_props(props);
    }
    for (size_t i = 0; i < touch::Count; ++i) {
        if (svc_config->set_string(mod_ctx, s_buttons[i].layout, values[i].c_str()) != MOD_OK) {
            svc_log->warn(mod_ctx, "Could not save Touch Buttons layout");
            return HOOK_SKIP_ORIGINAL;
        }
    }
    // Cancel/pop closes the native document without writing its working layout
    // into Dusklight's game.touchControlsLayout. Only Dawnlight config is saved.
    s_editorApi.cancel(editor);
    return HOOK_SKIP_ORIGINAL;
}
void close_extra_editor() {
    s_touchEditorAvailable = false;
    if (!s_extraEditor) return;
    // Reset opens a native modal whose callback captures the editor. Close it
    // before the editor during mod unload so that callback cannot outlive it.
    for (auto* child : s_editorChildren) s_editorApi.hide(child, true);
    s_editorChildren.clear();
    s_editorApi.hide(s_extraEditor, true);
    s_editorApi.uncover();
    s_extraEditor = nullptr;
}
void open_native_touch_editor(ModContext*, void*) {
    if (!s_touchEditorAvailable || s_extraEditor) return;
    dusk::ui::ControlLayoutSize size;
    if (!s_touchOwner || !extra_viewport(s_touchOwner->mDocument, size)) return;
    s_extraPresses.clear();
    s_editorFragment.clear();
    for (size_t i = 0; i < touch::Count; ++i) {
        s_editorControls[i] = {kExtraEditorIds[i], kExtraEditorIds[i], default_extra_props(i, size),
            dusk::ui::Control::COUNT, false};
        s_editorFragment += "<button id='";
        s_editorFragment += kExtraEditorIds[i];
        s_editorFragment += "' class='control trigger'><span>";
        s_editorFragment += touch::Labels[i];
        s_editorFragment += "</span></button>";
    }
    // Construct the original host type in storage with its pinned ABI. Its own
    // vtable and destructor remain native; the UI stack owns it after push.
    void* storage = ::operator new(sizeof(NativeEditor));
    s_extraEditor = static_cast<NativeEditor*>(storage);
    s_editorConstructing = true;
    ++s_editorScope;
    try {
        s_editorApi.construct(s_extraEditor);
    } catch (...) {
        --s_editorScope;
        s_editorConstructing = false;
        s_extraEditor = nullptr;
        ::operator delete(storage);
        throw;
    }
    --s_editorScope;
    s_editorConstructing = false;
    std::unique_ptr<EditorDocument> owned(s_extraEditor);
    if (!s_extraEditor->mDocument) return;
    s_extraEditor->mWorkingLayout = {};
    for (size_t i = 0; i < touch::Count; ++i)
        s_extraEditor->mWorkingLayout.controls[kExtraEditorIds[i]] = saved_extra_props(i, size);
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
        !resolve_extra("dusk::ui::TouchControlsEditor::cancel_edit", s_editorApi.cancel) ||
        !resolve_extra("dusk::ui::TouchControls::clear_virtual_input", s_editorApi.clearTouch) ||
        !resolve_extra("dusk::ui::TouchControls::show", s_editorApi.showTouch) ||
        !resolve_extra("Rml::Element::SetPseudoClass", s_editorApi.setPseudo)) return MOD_OK;
    const std::array bindings = {
        editor_hook<ExtraEditorBackground>("TouchControls::sync_visibility", show_editor_background, nullptr),
        editor_hook<ExtraEditorControls>("touch_layout_controls", nullptr, extra_editor_controls),
        editor_hook<ExtraEditorFragment>("touch_controls_rml_fragment", nullptr, extra_editor_fragment),
        editor_hook<ExtraEditorLayouts>("sync_control_layouts", enter_extra_editor, leave_extra_editor),
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
