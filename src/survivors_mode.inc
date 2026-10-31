// Mode registration owns availability; the host deactivates an active mode and
// returns to prelaunch when it is unregistered, without deleting its save file.
ModResult register_survivors_mode(ModError* error) {
    if(s_registered)return MOD_OK;
    const GameModeDesc mode{
        .struct_size = sizeof(GameModeDesc), .game_mode_id = kMode,
        .full_name = "Hyrule Survivors", .save_name = "gczelda2-survivors", .user_data = nullptr,
        .on_activated = activated, .on_deactivated = deactivated, .on_play = play_survivors,
        .on_save_loaded = loaded, .on_new_save = new_save, .on_new_save_select = nullptr,
        .on_game_reset = reset, .on_tick = nullptr,
    };
    const auto result = svc_game_mode->register_game_mode(mod_ctx, &mode);
    if (result != MOD_OK) return mods::set_error(error, result, "failed to register Hyrule Survivors");
    s_registered = true;
    return MOD_OK;
}
void update_survivors_registration() {
    if(survivors_enabled()==s_registered)return;
    if(survivors_enabled()) {
        ModError error=MOD_ERROR_INIT;
        if(register_survivors_mode(&error)!=MOD_OK)
            svc_log->warn(mod_ctx,"Dawnlight: failed to enable Hyrule Survivors mode");
    } else if(svc_game_mode->unregister_game_mode(mod_ctx,kMode)==MOD_OK) {
        s_registered=false;
    }
}
