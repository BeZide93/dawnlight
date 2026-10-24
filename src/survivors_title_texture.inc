// Runtime registration keeps this replacement exclusive to the selected mode.
// The filename identifies the same native title texture as the Boss Rush logo,
// but its separate res/ directory never registers it as a global replacement.
TextureReplacementHandle s_titleTexture = 0;
ModResult register_survivors_title_logo(ModError* error) {
    if (s_titleTexture) return MOD_OK;
    const auto result = svc_texture->register_file(mod_ctx,
        "res/survivors/tex1_608x100_0c1c70378fb8cb46_6.png",&s_titleTexture);
    if (result != MOD_OK) {
        s_titleTexture = 0;
        return mods::set_error(error,result,"failed to register Hyrule Survivors title texture");
    }
    return MOD_OK;
}
void unregister_survivors_title_logo() {
    if (!s_titleTexture) return;
    const auto handle = s_titleTexture;
    s_titleTexture = 0;
    if (svc_texture->unregister(mod_ctx,handle) != MOD_OK)
        svc_log->warn(mod_ctx,"Hyrule Survivors: failed to unregister title texture");
}
