// Development-only, last tab in the Ready menu. Run overrides are session-only;
// the explicit Meta Level editor updates ordinary saved progression.
UiElementHandle s_debugProgressSummary=0;
std::string debug_progress_summary() {
    const char* status=s_progressSaving ? "Saving..." : s_storeFailed ? "Save failed. Retry the edit or Save progress." :
        s_progressSaved ? "Progress saved." : "Progress saves when you close this menu, or choose Save progress.";
    return "Meta Level "+number(s_records.level())+" | Unlock Points: "+number(s_records.available_points())+"\n"+status;
}
ModResult update_debug_tab(ModContext*,void*,ModError*) {
    if(!s_debugProgressSummary)return MOD_OK;
    return svc_ui->elem_set_text(mod_ctx,s_debugProgressSummary,debug_progress_summary().c_str());
}
struct DebugControl { const char* label; unsigned DebugOptions::*field; const char* const* choices; unsigned count; };
constexpr const char* kDebugRarities[]{"Common","Uncommon","Rare","Epic","Legendary"};
constexpr const char* kDebugSwitch[]{"Off","On"};
constexpr const char* kDebugTimers[]{"20 min","10 min","5 min"};
std::array<DebugControl,8> s_debugControls{{
    {"Minimum Rarity",&DebugOptions::minimum_rarity,kDebugRarities,5},
    {"Player damage",&DebugOptions::player_damage,nullptr,0},
    {"Infinite rerolls",&DebugOptions::infinite_rerolls,kDebugSwitch,2},
    {"Infinite banishes",&DebugOptions::infinite_banishes,kDebugSwitch,2},
    {"Run timer",&DebugOptions::run_minutes,kDebugTimers,3},
    {"Max Equipment Drop Rate",&DebugOptions::max_equipment_drop_rate,kDebugSwitch,2},
    {"Always Legendary Equipment",&DebugOptions::always_legendary_equipment,kDebugSwitch,2},
    {"Max Normal Enemies",&DebugOptions::max_normal_enemies,nullptr,0}
}};
ModResult build_debug(ModContext*,UiWindowHandle,UiElementHandle left,UiElementHandle right,void*,ModError*) {
    s_debugProgressSummary=0;
    s_options.debug.clamp();s_run.options.debug.clamp();
    if (!debug_enabled) return MOD_OK;
    UiControlDesc enable=UI_CONTROL_DESC_INIT;
    enable.kind=UI_CONTROL_TOGGLE;enable.label="Enable Debug";
    enable.help_rml="Session only. Debug runs award no high scores, Survivors XP or permanent map unlocks. Resets to Off when the game restarts.";
    enable.get=[](ModContext*,void*,UiControlValue* value){value->bool_value=debug_enabled && debug_session_enabled;};
    enable.is_disabled=[](ModContext*,void*){return !debug_enabled || !settings_editable() || s_run.phase!=Phase::Ready;};
    enable.set=[](ModContext*,void*,const UiControlValue* value){
        if(!debug_enabled || !settings_editable() || s_run.phase!=Phase::Ready || s_debugNotice ||
           value->bool_value==debug_session_enabled)return;
        UiDialogAction action=UI_DIALOG_ACTION_INIT;action.label="OK";
        action.on_pressed=[](ModContext*,UiDialogHandle handle,void*){if(s_debugNotice==handle)s_debugNotice=0;};
        UiDialogDesc notice=UI_DIALOG_DESC_INIT;notice.title=value->bool_value?"Debug enabled":"Debug disabled";
        notice.variant=UI_DIALOG_WARNING;
        notice.body_rml=value->bool_value?
            "<p>Debug runs do not save high scores, award Survivors XP or permanently unlock maps. This includes Endless Mode. In-run XP and level-ups still work.</p><p>Enable Debug is not saved and resets to Off when you restart the game.</p>":
            "<p>Debug settings are disabled. New runs can save high scores, earn Survivors XP and permanently unlock maps again. Runs already started with Debug remain excluded.</p><p>Enable Debug is not saved and starts Off after restarting the game.</p>";
        notice.actions=&action;notice.action_count=1;notice.on_dismiss=action.on_pressed;
        if(svc_ui->dialog_push(mod_ctx,&notice,&s_debugNotice)!=MOD_OK){s_debugNotice=0;return;}
        debug_session_enabled=value->bool_value;
        if(!debug_session_enabled)s_options.debug={};
    };
    auto enabledResult=svc_ui->pane_add_control(mod_ctx,left,&enable,nullptr);
    if(enabledResult!=MOD_OK)return enabledResult;
    for (auto& setting:s_debugControls) {
        UiControlDesc control=UI_CONTROL_DESC_INIT;
        control.kind=setting.choices==kDebugSwitch?UI_CONTROL_TOGGLE:setting.choices?UI_CONTROL_SELECT:UI_CONTROL_NUMBER;
        control.label=setting.label;control.user_data=&setting;
        if(setting.field==&DebugOptions::max_equipment_drop_rate)
            control.help_rml="100% equipment drop chance per boss, including Debug runs. Weapon/Tunic stays 80/20; all ten weapon variants remain random. Stats follow Equipment Rarity unless Always Legendary Equipment is enabled. Equipment is saved. Scores, Survivors XP and permanent map unlocks remain disabled. Default: off.";
        if(setting.field==&DebugOptions::always_legendary_equipment)
            control.help_rml="New equipment drops are always Legendary with maximum base stats and a random Legendary-tier weapon bonus. Weapon/Tunic and weapon variants stay random. Allows saved drops in Debug at the normal talent-adjusted chance; combine with Max Equipment Drop Rate for a drop from every boss. Applies to the next run; existing items are unchanged. Default: off.";
        control.options=control.kind==UI_CONTROL_TOGGLE?nullptr:setting.choices;control.option_count=control.kind==UI_CONTROL_TOGGLE?0:setting.count;
        control.min=1;control.max=10;control.step=1;control.prefix=setting.choices?nullptr:"x";
        if(setting.field==&DebugOptions::max_normal_enemies) {
            control.min=0;control.max=normal_enemy_limit;control.prefix=nullptr;
            control.help_rml="Maximum simultaneous normal wave enemies, including Horde waves. Default and maximum: 345. Set to 0 to disable normal waves. Bosses, guards and boss summons keep their separate limits. Session only; applies to the next run.";
        }
        control.get=[](ModContext*,void* data,UiControlValue* value) {
            s_options.debug.clamp();
            const auto& setting=*static_cast<DebugControl*>(data);
            const auto current=s_options.debug.*setting.field;
            if(setting.choices==kDebugSwitch){value->bool_value=current!=0;return;}
            value->int_value=setting.field==&DebugOptions::run_minutes ? (current==20?0:current==10?1:2) : current;
        };
        control.set=[](ModContext*,void* data,const UiControlValue* value) {
            s_options.debug.clamp();
            if (!debug_enabled || !debug_session_enabled || !settings_editable() || s_run.phase!=Phase::Ready) return;
            const auto& setting=*static_cast<DebugControl*>(data);
            if(setting.choices==kDebugSwitch)s_options.debug.*setting.field=value->bool_value?1u:0u;
            else if (setting.field==&DebugOptions::run_minutes) {
                constexpr unsigned minutes[]{20,10,5};
                s_options.debug.run_minutes=minutes[std::clamp<int64_t>(value->int_value,0,2)];
            } else if(setting.field==&DebugOptions::max_normal_enemies) {
                s_options.debug.max_normal_enemies=unsigned(std::clamp<int64_t>(value->int_value,0,normal_enemy_limit));
            } else s_options.debug.*setting.field=unsigned(std::clamp<int64_t>(value->int_value,
                setting.choices?0:1,setting.choices?setting.count-1:10));
            s_options.debug.clamp();
        };
        control.is_disabled=[](ModContext*,void*){return !debug_enabled || !debug_session_enabled || !settings_editable() || s_run.phase!=Phase::Ready;};
        const auto result=svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);
        if (result!=MOD_OK) return result;
    }
    UiControlDesc unlock=UI_CONTROL_DESC_INIT;
    unlock.kind=UI_CONTROL_TOGGLE;unlock.label="Unlock all maps";
    unlock.help_rml="Temporarily bypass map locks. Does not change saved clears. Default: off.";
    unlock.get=[](ModContext*,void*,UiControlValue* value){value->bool_value=debug_enabled && debug_session_enabled && s_options.debug.unlock_all_maps;};
    unlock.set=[](ModContext*,void*,const UiControlValue* value){
        if(debug_enabled && debug_session_enabled && settings_editable())s_options.debug.unlock_all_maps=value->bool_value;
    };
    unlock.is_disabled=[](ModContext*,void*){return !debug_enabled || !debug_session_enabled || !settings_editable();};
    auto result=svc_ui->pane_add_control(mod_ctx,left,&unlock,nullptr);
    if(result!=MOD_OK)return result;
    UiControlDesc level=UI_CONTROL_DESC_INIT;
    level.kind=UI_CONTROL_NUMBER;level.label="Meta Level";
    level.min=1;level.max=1+meta_levels(max_meta_xp);level.step=1;
    level.help_rml="Permanently sets your saved Meta Level and its Unlock Points. A changed level starts at 0 XP toward the next level. Persists after disabling Debug, restarting or using a build without the Debug tab. Normal runs continue earning XP. Purchased talents are kept; refund talents first to go below their required level. Save progress or close this menu to save to disk.";
    level.get=[](ModContext*,void*,UiControlValue* value){value->int_value=s_records.level();};
    level.is_disabled=[](ModContext*,void*){return !debug_enabled || !debug_session_enabled || !settings_editable() || s_run.phase!=Phase::Ready;};
    level.set=[](ModContext*,void*,const UiControlValue* value){
        if(!debug_enabled || !debug_session_enabled || !settings_editable() || s_run.phase!=Phase::Ready)return;
        auto next=s_records;
        if(!next.set_meta_level(unsigned(std::clamp<int64_t>(value->int_value,1,1+meta_levels(max_meta_xp)))))return;
        const auto bytes=next.encode();
        s_storeFailed=svc_save->set_blob(mod_ctx,kBlob,bytes.data(),bytes.size())!=MOD_OK;
        if(!s_storeFailed){s_records=next;s_progressSaved=false;}
    };
    result=svc_ui->pane_add_control(mod_ctx,left,&level,nullptr);if(result!=MOD_OK)return result;
    UiControlDesc save=UI_CONTROL_DESC_INIT;save.label="Save progress";save.on_pressed=save_progress;
    save.is_disabled=[](ModContext*,void*){return !settings_editable() || s_run.phase!=Phase::Ready;};
    result=svc_ui->pane_add_control(mod_ctx,left,&save,nullptr);if(result!=MOD_OK)return result;
    result=svc_ui->pane_add_text(mod_ctx,left,debug_progress_summary().c_str(),&s_debugProgressSummary);
    if(result!=MOD_OK)return result;
    return svc_ui->pane_add_text(mod_ctx,right,
        "Enable Debug to edit these options. Debug runs earn no high scores, Survivors XP or permanent map unlocks, including in Endless Mode. The toggle is session-only and starts Off after restarting the game. Meta Level edits change saved progression permanently; disable Debug before starting a run to earn XP normally again. Other debug options apply to the next run. Minimum Rarity covers level-ups and treasures. Player damage includes all attacks and burns. Infinite rerolls are free; chest prices still apply. Four bosses arrive at 25%, 50%, 75% and 100% of the run timer. Other progression uses real elapsed time.",nullptr);
}
