// Scope a shadow decision to one actor's native Draw call. Never skip the actor
// draw itself: geometry, particles, gameplay and other mods still run normally.
struct ShadowDrawScope {const void* args=nullptr;bool normal=false,suppress=false;};
std::array<ShadowDrawScope,64> s_shadowDrawScopes{};
size_t s_shadowDrawDepth=0;
bool shadow_budget_active() {
    return survivor_perf::presentation.shadowAvailable && survivor_perf::enabled() &&
        in_arena() && !s_hub && !transitioning() && s_run.phase==Phase::Running;
}
HookAction before_shadow_actor_draw(ModContext*,void* args,void*,void*) {
    // Track nested non-enemy draws too, so they cannot inherit a parent's policy.
    if(!shadow_budget_active() && !s_shadowDrawDepth)return HOOK_CONTINUE;
    ShadowDrawScope scope{args};
    auto* process=mods::arg<void*>(args,1);
    if(shadow_budget_active() && process && is_actor_process(process)) {
        auto* actor=static_cast<fopAc_ac_c*>(process);
        const auto* enemy=tracked(fopAcM_GetID(actor));
        auto* player=dComIfGp_getPlayer(0);
        if(enemy && enemy->initialized && !enemy->elite && !enemy->retiring && player) {
            scope.normal=true;
            scope.suppress=survivor_perf::suppress_normal_shadow(survivor_perf::tier(),
                (actor->current.pos-player->current.pos).abs());
        }
    }
    if(s_shadowDrawDepth<s_shadowDrawScopes.size())s_shadowDrawScopes[s_shadowDrawDepth]=scope;
    ++s_shadowDrawDepth; // Overflow fails open until nested calls unwind.
    return HOOK_CONTINUE;
}
void after_shadow_actor_draw(ModContext*,void* args,void*,void*) {
    if(!s_shadowDrawDepth)return;
    if(s_shadowDrawDepth>s_shadowDrawScopes.size()){--s_shadowDrawDepth;return;}
    if(s_shadowDrawScopes[s_shadowDrawDepth-1].args==args)
        s_shadowDrawScopes[--s_shadowDrawDepth]={};
}
HookAction before_normal_enemy_shadow(ModContext*,void*,void* result,void*) {
    if(!shadow_budget_active() || !s_shadowDrawDepth || s_shadowDrawDepth>s_shadowDrawScopes.size())return HOOK_CONTINUE;
    const auto& scope=s_shadowDrawScopes[s_shadowDrawDepth-1];
    if(!scope.normal)return HOOK_CONTINUE;
    if(!scope.suppress){++survivor_perf::presentation.shadowAllowed;return HOOK_CONTINUE;}
    ++survivor_perf::presentation.shadowSuppressed;
    // Zero is the native no-shadow result. The actor replaces its old key;
    // the host clears shadow submissions each frame. Never retain model pointers.
    *static_cast<int*>(result)=0;
    return HOOK_SKIP_ORIGINAL;
}
void initialize_shadow_budget() {
    const bool ready=mods::hook_add_pre<SurvivorsShadowActorDraw>(svc_hook,before_shadow_actor_draw)==MOD_OK &&
        mods::hook_add_post<SurvivorsShadowActorDraw>(svc_hook,after_shadow_actor_draw)==MOD_OK &&
        mods::hook_add_pre<SurvivorsNormalShadow>(svc_hook,before_normal_enemy_shadow)==MOD_OK;
    survivor_perf::presentation.shadowAvailable=ready;
    if(!ready) {
        mods::hook_uninstall<SurvivorsShadowActorDraw>(svc_hook);
        mods::hook_uninstall<SurvivorsNormalShadow>(svc_hook);
        svc_log->warn(mod_ctx,"Survivors normal-enemy shadow budget unavailable; keeping native shadows");
    }
}
void shutdown_shadow_budget() {
    mods::hook_uninstall<SurvivorsShadowActorDraw>(svc_hook);
    mods::hook_uninstall<SurvivorsNormalShadow>(svc_hook);
    s_shadowDrawDepth=0;s_shadowDrawScopes={};
    survivor_perf::presentation.shadowAvailable=false;
}
