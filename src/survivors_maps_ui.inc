// Keep selection in the hub. The preview pane describes the destination; only
// the confirmed native door transition loads a run map.
constexpr const char* kArenaStyle=R"(
window { background-color: transparent; backdrop-filter: none; }
window content pane.survivors-settings { background-color: #171d19f5; padding: 16dp; gap: 12dp; }
window content pane.survivors-talents { flex: 1 1 100%; border-right-width: 0dp; overflow: hidden auto; background-color: #171d19f5; padding: 16dp; gap: 12dp; }
.survivors-talent-row { width: 100%; flex-shrink: 0; align-items: center; gap: 8dp; }
button.survivors-talent-name { flex: 1 1 0; min-width: 0; font-size: 17dp; padding: 10dp; }
button.survivors-talent-adjust { flex: 0 0 44dp; min-width: 44dp; font-size: 22dp; padding: 8dp; }
.survivors-talent-rank { flex: 0 0 64dp; text-align: center; font-size: 16dp; }
window tab-bar { background-color: #171d19ee; }
window content pane.survivors-map-list { flex: 0 0 35%; background-color: #171d19ef; padding: 16dp; gap: 8dp; font-size: 15dp; }
window content pane.survivors-map-preview { flex: 1 1 0; padding: 0dp 16dp 16dp; justify-content: flex-start; }
.survivors-map-preview-copy { display: block; background-color: #171d19e8; border-radius: 8dp; padding: 0dp 16dp 16dp; font-size: 16dp; }
.survivors-map-preview-title { display: block; font-family: var(--font-family-heading); font-size: 24dp; }
.survivors-map-launch { margin-top: auto; flex-direction: column; align-items: flex-end; gap: 8dp; }
.survivors-map-lock { font-size: 14dp; color: #ffdc75; background-color: #171d19ee; padding: 8dp; }
button.survivors-map-start { margin-top: 0dp; align-self: flex-end; padding: 12dp 28dp; font-size: 18dp; background-color: #171d19ee; }
button.survivors-map-option { padding: 12dp; font-size: 16dp; }
button.survivors-map-selected { background-color: #e5bc3b28; border-color: #ffdc75; }
.survivors-score-row { width: 100%; gap: 8dp; }
.survivors-score-row button { flex: 1 1 0; min-width: 0dp; font-size: 14dp; padding: 10dp; }
@media (max-width: 640dp) {
    window content pane.survivors-map-list { flex-basis: 42%; padding: 8dp; font-size: 12dp; }
    window content pane.survivors-map-preview { padding: 0dp 8dp 8dp; }
    button.survivors-map-option { font-size: 13dp; padding: 9dp; }
    .survivors-map-preview-copy { padding: 0dp 10dp 10dp; font-size: 13dp; }
    .survivors-map-preview-title { font-size: 18dp; }
}
)";
std::array<unsigned,arenas.size()> s_mapChoices{0,1,2,3,4};
void select_arena(ModContext*, void* data) {
    const unsigned choice=*static_cast<unsigned*>(data);
    if (s_run.phase!=Phase::Ready || s_restart || s_progressSaving || choice>=arenas.size() || choice==s_requestedMap) return;
    s_requestedMap=choice;
    s_refreshChoices=s_window; close_window();
}
UiElementHandle s_mapPreview=0,s_mapLockHint=0;
unsigned s_previewMap=0;
bool s_previewUnlocked=false;
std::string arena_preview() {
    return "<div class=\"survivors-map-preview-title\" style=\"color: "+
        std::string(arenas[s_requestedMap].accent)+";\">"+treasure_rml_escape(arenas[s_requestedMap].name)+"</div>";
}
ModResult update_arena_tab(ModContext*,void*,ModError*) {
    if(s_requestedMap>=arenas.size())return MOD_OK;
    const auto scoreResult=update_score_controls();
    if(scoreResult!=MOD_OK)return scoreResult;
    const bool unlocked=map_available(s_requestedMap);
    if(s_mapPreview && (s_previewMap!=s_requestedMap)) {
        auto result=svc_ui->elem_set_rml(mod_ctx,s_mapPreview,arena_preview().c_str());
        if(result!=MOD_OK)return result;
    }
    if(s_mapLockHint && (s_previewMap!=s_requestedMap || s_previewUnlocked!=unlocked)) {
        const std::string hint=unlocked?"":"Clear "+std::string(arenas[s_requestedMap-1].name)+" to unlock this map.";
        auto result=svc_ui->elem_set_text(mod_ctx,s_mapLockHint,hint.c_str());
        if(result!=MOD_OK)return result;
        result=svc_ui->elem_set_visible(mod_ctx,s_mapLockHint,!unlocked);
        if(result!=MOD_OK)return result;
    }
    s_previewMap=s_requestedMap;s_previewUnlocked=unlocked;
    return MOD_OK;
}
ModResult build_arena_choices(UiElementHandle left, UiElementHandle right) {
    s_mapPreview=s_mapLockHint=0;
    s_previewMap=unsigned(arenas.size());
    auto result=svc_ui->elem_set_class(mod_ctx,left,"survivors-map-list",true);
    if (result!=MOD_OK) return result;
    result=svc_ui->elem_set_class(mod_ctx,right,"survivors-map-preview",true);
    if (result!=MOD_OK) return result;
    result=svc_ui->pane_add_text(mod_ctx,left,"Choose your arena",nullptr);
    if (result!=MOD_OK) return result;
    for (unsigned i=0;i<arenas.size();++i) {
        UiControlDesc control=UI_CONTROL_DESC_INIT;
        control.label=arenas[i].name; control.on_pressed=select_arena; control.user_data=&s_mapChoices[i];
        UiElementHandle option=0;
        result=svc_ui->pane_add_control(mod_ctx,left,&control,&option);
        if (result!=MOD_OK) return result;
        result=svc_ui->elem_set_class(mod_ctx,option,"survivors-map-option",true);
        if (result!=MOD_OK) return result;
        if (i==s_requestedMap) {
            result=svc_ui->elem_set_class(mod_ctx,option,"survivors-map-selected",true);
            if (result!=MOD_OK) return result;
        }
    }
    if (!s_arenaMeshReady) result=svc_ui->pane_add_text(mod_ctx,left,
        "Hub collision is still loading. Please wait.",nullptr);
    if (result!=MOD_OK) return result;
    result=svc_ui->pane_add_text(mod_ctx,left,
        "Survive 20 minutes and defeat all four timed elites. Elites appear every 5 minutes and drop free big chests. Weapons attack automatically. "
        "Collect XP, hearts and rupees. Small chests cost 40 rupees. Start run closes this menu and opens the arena door.",nullptr);
    if (result!=MOD_OK) return result;
    char records[160];
    std::snprintf(records,sizeof(records),"Runs %u | Wins %u | Meta level %u",
        s_records.runs,s_records.wins,s_records.level());
    result=svc_ui->pane_add_text(mod_ctx,left,records,nullptr);
    if (result!=MOD_OK) return result;
    result=svc_ui->pane_add_rml(mod_ctx,right,arena_preview().c_str(),&s_mapPreview);
    if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,s_mapPreview,"survivors-map-preview-copy",true);
    if(result!=MOD_OK)return result;
    result=build_score_controls(right);
    if(result!=MOD_OK)return result;
    UiRowDesc rowDesc=UI_ROW_DESC_INIT;UiElementHandle launch=0;
    result=svc_ui->pane_add_row(mod_ctx,right,&rowDesc,&launch);
    if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,launch,"survivors-map-launch",true);
    if(result!=MOD_OK)return result;
    result=svc_ui->pane_add_text(mod_ctx,launch,"",&s_mapLockHint);
    if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,s_mapLockHint,"survivors-map-lock",true);
    if(result!=MOD_OK)return result;
    result=update_arena_tab(nullptr,nullptr,nullptr);
    if(result!=MOD_OK)return result;
    UiControlDesc start=UI_CONTROL_DESC_INIT;
    start.label="Start run"; start.on_pressed=start_run;
    start.is_disabled=[](ModContext*,void*) { return !map_available(s_requestedMap) || !s_arenaMeshReady || s_restart || s_progressSaving || s_hubDeparture; };
    UiElementHandle startButton=0;
    result=svc_ui->pane_add_control(mod_ctx,launch,&start,&startButton);
    if (result!=MOD_OK) return result;
    return svc_ui->elem_set_class(mod_ctx,startButton,"survivors-map-start",true);
}

UiElementHandle s_difficultySummary=0;
std::string difficulty_summary() {
    return "<h3>Twilit Rank "+number(s_options.twilit_rank())+"</h3><p>Enemy HP x"+number(s_options.enemy_hp)+
        ", Boss HP x"+number(s_options.boss_hp)+", damage x"+number(s_options.enemy_damage)+", movement speed x"+number(s_options.enemy_speed)+
        ", spawn batch x"+number(s_options.horde)+".</p><p>Score and talent XP: x"+
        result_decimal(s_options.score_percent()/100.0,2)+". Normal-kill score: x"+
        result_decimal(s_options.score_percent()/100.0/s_options.horde,2)+
        ". Normal-enemy run XP and loot per kill: x"+result_decimal(1.0/s_options.horde,2)+
        "; elite run XP: x1.00. More enemies offset the Horde reward reduction.</p>"
        "<p>Rank is the sum of Enemy HP, Damage, Speed and Horde increments above x1. Boss HP does not change rank or score. Score bonuses add: +10% per HP step, +15% per Damage step, +25% per Speed step, +10% per Horde step. Changes apply to the next run.</p>";
}
struct RunOptionControl { const char* label; unsigned RunOptions::*field; unsigned min,max,step; const char* help; };
std::array<RunOptionControl,6> s_runControls{{
    {"Enemy HP",&RunOptions::enemy_hp,1,10,1,"Multiplies normal enemy health only; bosses and stationary elites use Boss HP."},
    {"Enemy Damage",&RunOptions::enemy_damage,1,10,1,"Multiplies enemy damage to Link."},
    {"Enemy Speed",&RunOptions::enemy_speed,1,3,1,"Multiplies normal enemy locomotion only; attack movement and knockback remain native."},
    {"Horde",&RunOptions::horde,1,5,1,"More enemies. At x5, every fifth normal kill rolls loot and drops XP. Elite rewards and pots are unchanged."},
    {"Boss HP",&RunOptions::boss_hp,1,10,1,"Multiplies health of timed bosses and stationary elites, including Endless. Default: x1. Does not change rank or score."},
    {"Camera distance",&RunOptions::camera_distance,100,500,25,"100% is the normal distance. Default: 200%. Walls can limit the distance; event cameras remain native."}
}};
ModResult build_run_controls(UiElementHandle left, UiElementHandle right, unsigned first, unsigned last) {
    auto result=svc_ui->elem_set_class(mod_ctx,left,"survivors-settings",true);
    if (result!=MOD_OK) return result;
    result=svc_ui->elem_set_class(mod_ctx,right,"survivors-settings",true);
    if (result!=MOD_OK) return result;
    for (unsigned i=first;i<last;++i) {
        auto& setting=s_runControls[i];
        UiControlDesc control=UI_CONTROL_DESC_INIT;
        control.is_disabled=[](ModContext*,void*){return !settings_editable();};
        control.kind=UI_CONTROL_NUMBER; control.label=setting.label; control.help_rml=setting.help;
        control.user_data=&setting; control.min=setting.min; control.max=setting.max; control.step=setting.step;
        control.prefix=setting.field==&RunOptions::camera_distance ? nullptr : "x"; control.suffix=setting.field==&RunOptions::camera_distance ? "%" : nullptr;
        control.get=[](ModContext*,void* data,UiControlValue* value) {
            value->int_value=s_options.*static_cast<RunOptionControl*>(data)->field;
        };
        control.set=[](ModContext*,void* data,const UiControlValue* value) {
            if (!settings_editable()) return;
            const auto& setting=*static_cast<RunOptionControl*>(data);
            s_options.*setting.field=unsigned(std::clamp<int64_t>(value->int_value,setting.min,setting.max));
            store_option_config();
            if(s_difficultySummary)svc_ui->elem_set_rml(mod_ctx,s_difficultySummary,difficulty_summary().c_str());
        };
        result=svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);
        if (result!=MOD_OK) return result;
    }
    return svc_ui->pane_add_text(mod_ctx,right,first==0 ?
        "Score bonus: +10% per Enemy HP step, +15% per Enemy Damage step, +25% per Enemy Speed step and +10% per Horde step. Bonuses add together (x1.00 to x4.15). Boss HP does not change rank or score. Normal kill points are divided by Horde. Settings apply to the next run." :
        "Shared with Dawnlight Settings and saved automatically. Settings apply to the next run; edit them outside a run.",nullptr);
}
ModResult build_difficulty(ModContext*,UiWindowHandle,UiElementHandle left,UiElementHandle right,void*,ModError*) {
    s_difficultySummary=0;
    const auto result=build_run_controls(left,right,0,5);if(result!=MOD_OK)return result;
    return svc_ui->pane_add_rml(mod_ctx,left,difficulty_summary().c_str(),&s_difficultySummary);
}
struct ManualAimControl {const char* label;ManualAbility ability;};
std::array<ManualAimControl,10> s_manualAimControls{{
    {"Sword Attack",ManualAbility::Sword},{"Spin Attack",ManualAbility::Spin},
    {"Jump Strike",ManualAbility::JumpStrike},{"Sword Wave",ManualAbility::Wave},
    {"Bow",ManualAbility::Bow},{"Boomerang",ManualAbility::Boomerang},
    {"Bombs",ManualAbility::Bombs},{"Ball and Chain",ManualAbility::Ball},
    {"Shield Attack",ManualAbility::ShieldAttack},{"Shockwave",ManualAbility::Shockwave}
}};
ModResult build_manual_aim_settings(UiElementHandle pane) {
    auto result=svc_ui->pane_add_text(mod_ctx,pane,"Manual aiming abilities — on: follow the reticle while attacking; off: always use automatic targeting. All default to on. Unlocks, cooldowns and Jump Strike's nearby-enemy requirement still apply. Elemental perks inherit their weapon's aim; auras and passive effects stay automatic.",nullptr);
    if(result!=MOD_OK)return result;
    for(auto& setting:s_manualAimControls) {
        UiControlDesc control=UI_CONTROL_DESC_INIT;control.is_disabled=[](ModContext*,void*){return !settings_editable();};
        control.kind=UI_CONTROL_TOGGLE;
        control.label=setting.label;control.user_data=&setting;
        control.get=[](ModContext*,void* data,UiControlValue* value) {
            value->bool_value=s_options.manual_enabled(static_cast<ManualAimControl*>(data)->ability);
        };
        control.set=[](ModContext*,void* data,const UiControlValue* value) {
            if(!settings_editable())return;
            s_options.manual_aim[unsigned(static_cast<ManualAimControl*>(data)->ability)]=value->bool_value;
            store_option_config();
        };
        result=svc_ui->pane_add_control(mod_ctx,pane,&control,nullptr);if(result!=MOD_OK)return result;
    }
    return MOD_OK;
}
UiWindowHandle s_manualAimSettingsWindow=0;
void open_manual_aim_settings(ModContext*,void*) {
    if(s_manualAimSettingsWindow)return;
    UiTabDesc tab=UI_TAB_DESC_INIT;
    tab.title="Manual aiming abilities";
    tab.build=[](ModContext*,UiWindowHandle,UiElementHandle left,UiElementHandle,void*,ModError*) {
        // Interactive controls belong in the navigation pane. The host replaces
        // the right pane whenever a setting's contextual help receives focus.
        return build_manual_aim_settings(left);
    };
    UiWindowDesc window=UI_WINDOW_DESC_INIT;
    window.tabs=&tab;window.tab_count=1;
    window.on_closed=[](ModContext*,UiWindowHandle,void*){s_manualAimSettingsWindow=0;};
    if(svc_ui->window_push(mod_ctx,&window,&s_manualAimSettingsWindow)!=MOD_OK)s_manualAimSettingsWindow=0;
}
ModResult build_run_settings(ModContext*,UiWindowHandle,UiElementHandle left,UiElementHandle right,void*,ModError*) {
    auto result=build_run_controls(left,right,5,6);
    if(result!=MOD_OK)return result;
    UiControlDesc control=UI_CONTROL_DESC_INIT;
    control.is_disabled=[](ModContext*,void*){return !settings_editable();};
    control.kind=UI_CONTROL_NUMBER;control.label="3rd Person Reticle Y";
    control.min=-40;control.max=40;control.step=1;
    control.help_rml="Moves the sword-aim reticle vertically in Survivors only. Positive values move it up; negative values move it down.";
    control.get=[](ModContext*,void*,UiControlValue* value){value->int_value=s_options.reticle_y;};
    control.set=[](ModContext*,void*,const UiControlValue* value){
        if(settings_editable()){s_options.reticle_y=int(std::clamp<int64_t>(value->int_value,-40,40));store_option_config();}
    };
    result=svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);
    if(result!=MOD_OK)return result;
    control=UI_CONTROL_DESC_INIT;
    control.is_disabled=[](ModContext*,void*){return !settings_editable();};
    control.kind=UI_CONTROL_TOGGLE;control.label="Switch Attack/Lock-on Keybinds";
    control.help_rml="Swap attack and lock-on during Survivors gameplay, including the touch sword button. Default: off.";
    control.get=[](ModContext*,void*,UiControlValue* value){value->bool_value=s_options.switch_attack_lock;};
    control.set=[](ModContext*,void*,const UiControlValue* value){
        if(settings_editable()){s_options.switch_attack_lock=value->bool_value;store_option_config();}
    };
    result=svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);
    if(result!=MOD_OK)return result;
    control.label="Gyro Aim";
    control.help_rml="Use motion aiming while holding attack in the Survivors shoulder view. Uses the configured gyro sensitivity and inversion. Default: on.";
    control.get=[](ModContext*,void*,UiControlValue* value){value->bool_value=s_options.gyro_aim;};
    control.set=[](ModContext*,void*,const UiControlValue* value){
        if(settings_editable()){s_options.gyro_aim=value->bool_value;store_option_config();}
    };
    result=svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);
    if(result!=MOD_OK)return result;
    control.label="Enemy Collision";
    control.help_rml="Allow enemies to block Link's movement. Off lets Link pass through enemies; terrain, chests and pots still block him. Enemy attacks still deal damage. Default: off.";
    control.get=[](ModContext*,void*,UiControlValue* value){value->bool_value=s_options.enemy_collision;};
    control.set=[](ModContext*,void*,const UiControlValue* value){
        if(settings_editable()){s_options.enemy_collision=value->bool_value;store_option_config();}
    };
    result=svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);
    if(result!=MOD_OK)return result;
    control.label="Allow Cheats";
    control.help_rml="Allow Dusklight's Cheats settings during the next run. Marks its results Cheated, even if no cheat is used. Default: off.";
    control.get=[](ModContext*,void*,UiControlValue* value){value->bool_value=s_options.allow_cheats;};
    control.set=[](ModContext*,void*,const UiControlValue* value){
        if(settings_editable()){s_options.allow_cheats=value->bool_value;store_option_config();}
    };
    result=svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);
    if(result!=MOD_OK)return result;
    result=build_reset_scores(left);
    if(result!=MOD_OK)return result;
    control=UI_CONTROL_DESC_INIT;
    control.label="HUD-Editor";control.on_pressed=open_hud_editor;
    control.help_rml="Move and resize Survivors HUD elements with a transparent live preview.";
    result=svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);
    if(result!=MOD_OK)return result;
    control=UI_CONTROL_DESC_INIT;
    control.kind=UI_CONTROL_GROUP;control.label="Manual aiming abilities";
    control.help_rml="Choose which abilities follow the reticle during manual aiming. Disabled abilities keep their automatic targeting.";
    control.on_pressed=open_manual_aim_settings;
    return svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);
}
