#if DAWNLIGHT_FREEZE_DIAGNOSTICS
survivor_perf::BufferScene survivors_buffer_scene() {
    using survivor_perf::Visual;
    survivor_perf::BufferScene scene;
    scene.map=s_map;scene.phase=unsigned(s_run.phase);scene.tick=s_run.ticks;
    scene.hub=s_hub;scene.transition=transitioning();scene.paused=dComIfGp_isPauseFlag();
    scene.event=dComIfGp_event_runCheck();
    scene.menu=ui_visible()||dMeter2Info_getWindowStatus()!=0||dMeter2Info_getPauseStatus()!=0;
    scene.itemEvent=hub_zelda_gift_busy()?1:s_run.phase==Phase::Treasure?2:0;
    scene.reward=scene.itemEvent==1?unsigned(ChestReward::OrdonSword):scene.itemEvent==2?unsigned(s_chosenTreasure):unsigned(ChestReward::Count);
    scene.reserved=s_visualBudget.used();
    const auto emitter_count=[](const auto& keys) {
        unsigned count=0;
        for(auto key:keys)if(auto* e=key?dComIfGp_particle_getEmitter(key):nullptr)
            count+=!e->isEnableDeleteEmitter();
        return count;
    };
    const auto add=[&](Visual kind,bool reduced,unsigned emitters) {
        auto& e=scene.effects[unsigned(kind)];++e.active;e.emitters+=emitters;
        e.reduced+=reduced;e.full+=!reduced&&emitters!=0;
    };
    for(unsigned i=0;i<s_enemies.size();++i) {
        const auto& e=s_enemies[i];
        if(!e.initialized||e.retiring||e.credited)continue;
        auto* actor=actor_by_id(e.id);if(!actor||actor->health<=0)continue;
        ++scene.enemies;const auto& f=s_effects[i];
        if(f.id!=e.id||(!f.burn&&!std::any_of(f.legendaryBurns.begin(),f.legendaryBurns.end(),[](const auto& b){return b.remaining!=0;})))continue;
        const auto emitters=emitter_count(f.emitters);
        add(Visual::Burn,survivor_perf::enabled()&&!emitters,emitters);
    }
    for(const auto& a:s_attacks) {
        if(!a.active)continue;
        ++scene.projectiles;
        const auto n=emitter_count(a.emitters);
        if(a.exploding)add(Visual::Explosion,a.reducedVisual,n);
        else if(a.weapon==0||a.weapon==unsigned(Upgrade::Shockwave))add(Visual::Wave,a.reducedVisual,n);
        else if(a.weapon==unsigned(Upgrade::Boomerang))add(Visual::Boomerang,a.reducedVisual,n);
        else if(a.weapon==unsigned(Upgrade::Bow)&&s_run.rank(Upgrade::FireArrows))add(Visual::FireArrow,survivor_perf::enabled()&&!n,n);
        else if(a.weapon==unsigned(Upgrade::Ball)) {
            auto& e=scene.effects[unsigned(Visual::Chain)];++e.active;
            e.reduced+=survivor_perf::enabled();e.full+=!survivor_perf::enabled();
        }
    }
    for(const auto& b:s_lightning)if(b.active) {
        ++scene.projectiles;
        add(Visual::Lightning,b.reducedVisual,emitter_count(b.emitters));
    }
    // These decorations have no independent gameplay lifetime: report living emitters only.
    if(const auto n=emitter_count(s_fireEdgeEmitters))add(Visual::FireEdge,false,n);
    if(const auto n=emitter_count(s_jumpEmitters))add(Visual::Jump,false,n);
    return scene;
}
#endif
