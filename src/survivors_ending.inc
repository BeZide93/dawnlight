#include "survivors_victory_wave.inc"

void reset_ending() {
    s_victoryWave.release();
    if (s_deathGameOver!=kNone) {
        d_GameOver_Delete(s_deathGameOver);
        dComIfGp_setGameoverStatus(0);
    }
    s_victory=s_deathPending=s_deathDismiss=false;
    s_victoryTicks=0;
}
void begin_victory(const cXyz& position) {
    if (s_victory || s_deathPending || s_resultsStored || s_run.phase!=Phase::Running) return;
    s_victory=true; s_victoryTicks=0; s_run.clear_fate();
    s_victoryWave.origin=position;
    s_victoryWave.radius=1500;
    for (const auto& enemy:s_enemies) if (enemy.id!=kNone && !enemy.retiring) {
        if (auto* actor=actor_by_id(enemy.id))
            s_victoryWave.radius=std::max(s_victoryWave.radius,(actor->current.pos-position).absXZ()+200);
    }
    // Keep the boss loot alive; retire_enemies() also retires pickups and must
    // therefore wait until the results screen. No further combat is allowed.
    clear_attacks(); clear_enemy_effects(); retire_enemy_arrows();
    if (auto* link=daAlink_getAlinkActorClass()) interrupt_auto_actions(link);
    s_victoryWave.sample();
}
void update_victory() {
    ++s_victoryTicks;
    s_victoryWave.sample();
    const float radius=s_victoryWave.radius*std::min(1.f,float(s_victoryTicks)/60.f);
    for (auto& enemy:s_enemies) {
        if (enemy.id==kNone || enemy.retiring) continue;
        auto* actor=actor_by_id(enemy.id);
        // Sweep pending creations and off-floor actors too at the outer edge.
        // Deferred deletion does not enter credit_death, HP accounting, native
        // death effects or drop creation, including for surviving elite guards.
        if (s_victoryTicks>=60 || (actor && (actor->current.pos-s_victoryWave.origin).absXZ()<=radius))
            enemy.retiring=true;
    }
}
void begin_defeat() {
    if (s_victory || s_deathPending || s_resultsStored ||
        (s_run.phase!=Phase::Running && s_run.phase!=Phase::Upgrade &&
         s_run.phase!=Phase::TreasureChoice && s_run.phase!=Phase::Treasure)) return;
    s_deathPending=true; s_deathDismiss=false;
    s_run.end(false);
    close_window(); close_powerup_window(); clear_attacks(); retire_enemies();
    // Do not heal, replace Link's procedure, or open the results here.
}
HookAction before_game_over(ModContext*,void* args,void* result,void*) {
    auto* screen=mods::arg<dGameover_c*>(args,0);
    if (!in_arena() || !s_deathPending || !screen || dMeter2Info_getGameOverType()!=0) return HOOK_CONTINUE;
    s_deathGameOver=fpcM_GetID(screen);
    // Native animation -> fade out -> Game Over fade in. Give the visible title
    // two seconds (including its one-second fade), then hand off before Save/Open.
    if (screen->mProc==PROC_DISP_WAIT && screen->mTimer<=30) {
        screen->mProc=PROC_DELETE_WAIT;
        s_deathDismiss=true;
    }
    if (s_deathDismiss) { *static_cast<int*>(result)=1; return HOOK_SKIP_ORIGINAL; }
    return HOOK_CONTINUE;
}
bool update_ending(daAlink_c*) {
    if (!s_deathPending) return false;
    if (!s_deathDismiss || s_deathGameOver==kNone) return true;
    if (fopMsgM_SearchByID(s_deathGameOver) || fpcM_IsCreating(s_deathGameOver)) {
        // Keep the ID until native deletion releases the shared 2D heap.
        auto id=s_deathGameOver;
        d_GameOver_Delete(id);
        return true;
    }
    s_deathGameOver=kNone;
    dComIfGp_setGameoverStatus(0);
    dComIfGp_offPauseFlag();
    dComIfGp_event_reset();
    finish(false);
    return false;
}
