// Settings persist independently of run options, scores and the Debug tab.
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
ConfigVarHandle s_performanceLogsConfig{};
bool performance_logs_saved() {
    bool value=false;
    if(s_performanceLogsConfig)svc_config->get_bool(mod_ctx,s_performanceLogsConfig,&value);
    return value;
}
ModResult initialize_performance_logs_setting() {
    ConfigVarDesc desc=CONFIG_VAR_DESC_INIT;
    desc.name="survivors-performance-logs";desc.type=CONFIG_VAR_BOOL;desc.default_bool=false;
    return register_owned_setting(&desc,&s_performanceLogsConfig);
}
ModResult build_performance_logs(UiElementHandle pane) {
    UiControlDesc control=UI_CONTROL_DESC_INIT;
    control.kind=UI_CONTROL_TOGGLE;control.label="Performance Logs";
    control.help_rml="Records freeze timings and buffer pressure, effect counts and scene context, including when Performance Mode is off. Default: Off. Restart Dusklight after changing this setting. The current session keeps its previous setting until restart.";
    control.get=[](ModContext*,void*,UiControlValue* value){value->bool_value=performance_logs_saved();};
    control.set=[](ModContext*,void*,const UiControlValue* value){
        if(s_performanceLogsConfig && svc_config->set_bool(mod_ctx,s_performanceLogsConfig,value->bool_value)!=MOD_OK)
            svc_log->warn(mod_ctx,"Survivors: could not save Performance Logs setting");
    };
    const auto result=svc_ui->pane_add_control(mod_ctx,pane,&control,nullptr);
    if(result!=MOD_OK)return result;
    return svc_ui->pane_add_text(mod_ctx,pane,freeze_diag::enabled?
        "Performance Logs: On for this session. Changes require restarting Dusklight.":
        "Performance Logs: Off for this session. Changes require restarting Dusklight.",nullptr);
}
#endif
