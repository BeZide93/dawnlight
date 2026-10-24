// Public UI service only: the host stacks/hides the settings window and restores
// it on Back. The transparent document is local to this editor, including on iOS.
constexpr const char* kSurvivorsHudEditorStyle=R"(
body { padding: 16dp; }
window { background-color: transparent; backdrop-filter: none; box-shadow: none;
    width: 100%; max-width: 800dp; height: 46%; max-height: 420dp; margin: auto auto 0dp; }
window tab-bar { background-color: #171d19b0; font-size: 18dp; }
window content pane { background-color: transparent; padding: 8dp; gap: 4dp; font-size: 15dp; }
window content pane button { background-color: #171d19b0; padding: 6dp; }
)";
std::array<unsigned,3> s_hudAxes{0,1,2};
bool hud_editor_editable() {
    return !s_restart && !s_progressSaving && !s_hubDeparture && !save_modal_active() && !transitioning();
}
void set_hud_value(unsigned part,unsigned axis,int64_t value) {
    if(s_hudConfig[part][axis] && svc_config->set_int(mod_ctx,s_hudConfig[part][axis],clamp_hud_value(axis,value))!=MOD_OK)
        svc_log->warn(mod_ctx,"Survivors: could not persist a HUD setting");
}
ModResult build_hud_editor(ModContext*,UiWindowHandle,UiElementHandle left,UiElementHandle,void*,ModError*) {
    UiControlDesc control=UI_CONTROL_DESC_INIT;
    control.kind=UI_CONTROL_SELECT;control.label="Element";
    control.options=hud_part_names.data();control.option_count=hud_part_names.size();
    control.get=[](ModContext*,void*,UiControlValue* value){value->int_value=s_hudEditorPart;};
    control.set=[](ModContext*,void*,const UiControlValue* value){
        s_hudEditorPart=unsigned(std::clamp<int64_t>(value->int_value,0,hud_defaults.size()-1));
    };
    auto result=svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);if(result!=MOD_OK)return result;
    constexpr std::array names{"X Position","Y Position","Scale"};
    for(auto& axis:s_hudAxes) {
        control=UI_CONTROL_DESC_INIT;control.kind=UI_CONTROL_NUMBER;control.label=names[axis];
        control.min=axis==2?10:-9999;control.max=axis==2?400:9999;control.step=1;
        control.suffix=axis==2?"%":" px";control.user_data=&axis;
        control.help_rml="Live preview. X is measured from the screen center, Y from the top. Item Banner uses its center as the anchor; its icon, text and equip prompt scale together. Scale preserves proportions. Changes save automatically. Info-Texts move together and leave room for a visible boss. The preview includes sample equipment, a boss, Twilit Rank and the Enemy Counter, even when its visibility toggle is off.";
        control.is_disabled=[](ModContext*,void*){return !hud_editor_editable();};
        control.get=[](ModContext*,void* data,UiControlValue* value){value->int_value=hud_value(s_hudEditorPart,*static_cast<unsigned*>(data));};
        control.set=[](ModContext*,void* data,const UiControlValue* value){
            if(hud_editor_editable())set_hud_value(s_hudEditorPart,*static_cast<unsigned*>(data),value->int_value);
        };
        result=svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);if(result!=MOD_OK)return result;
    }
    control=UI_CONTROL_DESC_INIT;control.label="Reset selected element";
    control.is_disabled=[](ModContext*,void*){return !hud_editor_editable();};
    control.on_pressed=[](ModContext*,void*){
        if(hud_editor_editable())for(unsigned axis=0;axis<3;++axis)set_hud_value(s_hudEditorPart,axis,hud_defaults[s_hudEditorPart][axis]);
    };
    return svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);
}
void open_hud_editor(ModContext*,void*) {
    if(s_hudEditorWindow || !hud_editor_editable())return;
    UiTabDesc tab=UI_TAB_DESC_INIT;tab.title="HUD-Editor";tab.build=build_hud_editor;
    UiWindowDesc desc=UI_WINDOW_DESC_INIT;desc.tabs=&tab;desc.tab_count=1;desc.rcss=kSurvivorsHudEditorStyle;
    desc.on_closed=[](ModContext*,UiWindowHandle handle,void*){if(s_hudEditorWindow==handle)s_hudEditorWindow=0;};
    if(svc_ui->window_push(mod_ctx,&desc,&s_hudEditorWindow)!=MOD_OK)s_hudEditorWindow=0;
}
