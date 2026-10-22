// Shared by the hub and Dawnlight Settings. ConfigService owns persistence and
// export/import; active runs retain their immutable RunOptions snapshot.
std::array<ConfigVarHandle,12> s_optionConfig{};
// Controls place Boss HP before camera; retain established config slot identities.
constexpr std::array<unsigned,6> s_runControlConfigSlots{0,1,2,3,11,4};
bool settings_editable() {
    return !s_restart && !s_progressSaving && !s_hubDeparture &&
        (!s_active || s_run.phase==Phase::Ready);
}
void load_option_config() {
    for(unsigned i=0;i<s_runControls.size();++i) {
        const auto& setting=s_runControls[i];const auto handle=s_optionConfig[s_runControlConfigSlots[i]];
        if(!handle)continue;
        if(i<5) {
            double value=s_options.*setting.field;
            if(svc_config->get_float(mod_ctx,handle,&value)==MOD_OK)
                s_options.*setting.field=RunOptions::difficulty(float(value),setting.max);
        } else {
            int64_t value=int64_t(s_options.*setting.field);
            if(svc_config->get_int(mod_ctx,handle,&value)==MOD_OK)
                s_options.*setting.field=float(std::clamp<int64_t>(value,int64_t(setting.min),int64_t(setting.max)));
        }
    }
    int64_t value=0;
    if(s_optionConfig[10] && svc_config->get_int(mod_ctx,s_optionConfig[10],&value)==MOD_OK)s_options.enemy_collision=value!=0;
    if(s_optionConfig[9] && svc_config->get_int(mod_ctx,s_optionConfig[9],&value)==MOD_OK)s_options.allow_cheats=value!=0;
    if(s_optionConfig[5] && svc_config->get_int(mod_ctx,s_optionConfig[5],&value)==MOD_OK)s_options.reticle_y=int(std::clamp<int64_t>(value,-40,40));
    if(s_optionConfig[6] && svc_config->get_int(mod_ctx,s_optionConfig[6],&value)==MOD_OK)s_options.switch_attack_lock=value!=0;
    if(s_optionConfig[7] && svc_config->get_int(mod_ctx,s_optionConfig[7],&value)==MOD_OK)s_options.gyro_aim=value!=0;
    if(s_optionConfig[8] && svc_config->get_int(mod_ctx,s_optionConfig[8],&value)==MOD_OK)
        for(unsigned i=0;i<s_options.manual_aim.size();++i)s_options.manual_aim[i]=(uint64_t(value)&(1u<<i))!=0;
}
void store_option_config() {
    std::array<int64_t,12> values{};
    values[4]=int64_t(s_options.camera_distance);
    for(unsigned i=0;i<5;++i) {
        const auto handle=s_optionConfig[s_runControlConfigSlots[i]];
        if(handle&&svc_config->set_float(mod_ctx,handle,s_options.*s_runControls[i].field)!=MOD_OK)
            svc_log->warn(mod_ctx,"Survivors: could not persist a difficulty setting");
    }
    values[10]=s_options.enemy_collision;
    values[9]=s_options.allow_cheats;
    values[5]=s_options.reticle_y;values[6]=s_options.switch_attack_lock;values[7]=s_options.gyro_aim;
    for(unsigned i=0;i<s_options.manual_aim.size();++i)if(s_options.manual_aim[i])values[8]|=1u<<i;
    for(unsigned i=0;i<values.size();++i)
        if(i>=4 && i!=11 && s_optionConfig[i] && svc_config->set_int(mod_ctx,s_optionConfig[i],values[i])!=MOD_OK)
            svc_log->warn(mod_ctx,"Survivors: could not persist a run setting");
}
ModResult initialize_option_config() {
    constexpr std::array names{"survivors-enemy-hp","survivors-enemy-damage","survivors-enemy-speed",
        "survivors-horde","survivors-camera-distance","survivors-reticle-y","survivors-switch-attack-lock",
        "survivors-gyro-aim","survivors-manual-abilities","survivors-allow-cheats","survivors-enemy-collision","survivors-boss-hp"};
    constexpr std::array<int64_t,12> defaults{1,1,1,1,200,0,0,1,(1u<<unsigned(ManualAbility::Count))-1,0,0,1};
    for(unsigned i=0;i<names.size();++i) {
        ConfigVarDesc desc=CONFIG_VAR_DESC_INIT;
        desc.name=names[i];desc.type=(i<4||i==11)?CONFIG_VAR_FLOAT:CONFIG_VAR_INT;
        desc.default_int=defaults[i];desc.default_float=double(defaults[i]);
        const auto result=register_owned_setting(&desc,&s_optionConfig[i]);
        if(result!=MOD_OK)return result;
    }
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
    const auto diagnostics=initialize_performance_logs_setting();
    if(diagnostics!=MOD_OK)return diagnostics;
#endif
    load_option_config();return MOD_OK;
}
