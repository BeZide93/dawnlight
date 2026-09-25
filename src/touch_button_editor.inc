// Reuse the running Dusklight TouchControlsEditor, including its RML/CSS,
// drag threshold, resize handles, docking, reset confirmation and navigation.
// Only our instance sees five alternative controls. The host's nine-element
// array stays unchanged, and vanilla editor instances use their original data.
using NativeEditor = dusk::ui::TouchControlsEditor;
using LayoutSpan = std::span<const dusk::ui::TouchLayoutControlInfo>;
using EditorDocument = dusk::ui::Document;
DEFINE_HOOK_SYMBOL("dusk::ui::touch_layout_controls", LayoutSpan(), ExtraEditorControls);
DEFINE_HOOK_SYMBOL("dusk::ui::touch_controls_rml_fragment", std::string_view(), ExtraEditorFragment);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::sync_control_layouts", void(NativeEditor*), ExtraEditorLayouts);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::sync_selection_frame", void(NativeEditor*), ExtraEditorFrame);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::begin_edit",
    bool(NativeEditor*, size_t, NativeEditor::EditHandle, Rml::Vector2f, bool, SDL_FingerID), ExtraEditorBegin);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::continue_edit", bool(NativeEditor*, Rml::Vector2f), ExtraEditorMove);
DEFINE_HOOK_SYMBOL("dusk::ui::TouchControlsEditor::end_edit", bool(NativeEditor*, bool, SDL_FingerID, bool), ExtraEditorEnd);
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
} s_editorApi;
NativeEditor* s_extraEditor = nullptr;
unsigned s_editorScope = 0;
bool s_editorConstructing = false;
std::array<dusk::ui::TouchLayoutControlInfo, touch::Count> s_editorControls{};
std::string s_editorFragment;
std::vector<EditorDocument*> s_editorChildren;
constexpr const char* kExtraEditorIds[] = {"dawnlight-zl", "dawnlight-up", "dawnlight-down", "dawnlight-left", "dawnlight-right"};
static_assert(touch::Count <= dusk::ui::kTouchLayoutControlCount);

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

template<class Hook> ModResult scope_editor_hook() {
    auto result = mods::hook_add_pre<Hook>(svc_hook, enter_extra_editor);
    if (result == MOD_OK) result = mods::hook_add_post<Hook>(svc_hook, leave_extra_editor);
    return result;
}
ModResult install_extra_editor(ModError* error) {
    if (!resolve_extra("_ZN4dusk2ui19TouchControlsEditorC1Ev", s_editorApi.construct) ||
        !resolve_extra("dusk::ui::push_document", s_editorApi.push) ||
        !resolve_extra("dusk::ui::top_document", s_editorApi.top) ||
        !resolve_extra("dusk::ui::uncover_top_document", s_editorApi.uncover) ||
        !resolve_extra("dusk::ui::Document::hide", s_editorApi.hide) ||
        !resolve_extra("dusk::ui::TouchControlsEditor::cancel_edit", s_editorApi.cancel)) return MOD_OK;
    auto result = mods::hook_add_post<ExtraEditorControls>(svc_hook, extra_editor_controls);
    if (result == MOD_OK) result = mods::hook_add_post<ExtraEditorFragment>(svc_hook, extra_editor_fragment);
    if (result == MOD_OK) result = scope_editor_hook<ExtraEditorLayouts>();
    if (result == MOD_OK) result = scope_editor_hook<ExtraEditorFrame>();
    if (result == MOD_OK) result = scope_editor_hook<ExtraEditorBegin>();
    if (result == MOD_OK) result = scope_editor_hook<ExtraEditorMove>();
    if (result == MOD_OK) result = scope_editor_hook<ExtraEditorEnd>();
    if (result == MOD_OK) result = scope_editor_hook<ExtraEditorReset>();
    if (result == MOD_OK) result = mods::hook_add_pre<ExtraEditorSave>(svc_hook, save_extra_editor);
    if (result == MOD_OK) result = mods::hook_add_pre<ExtraEditorDelete>(svc_hook, extra_editor_delete);
    if (result == MOD_OK) result = mods::hook_add_pre<ExtraEditorPush>(svc_hook, extra_editor_push);
    if (result != MOD_OK) return mods::set_error(error, result, "failed to install native touch editor adapter");
    s_touchEditorAvailable = true;
    return MOD_OK;
}
