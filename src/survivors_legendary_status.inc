// Native Bulblin flame particles anchored along the actual blade, never to a guessed
// world-space offset. Their lifetime belongs to the run, not Link's model.
std::array<u32,3> s_fireEdgeEmitters{};
void clear_fire_edge() {
    s_visualBudget.release(s_fireEdgeEmitters.data());
    for (auto& key:s_fireEdgeEmitters) {
        if (auto* emitter=key?dComIfGp_particle_getEmitter(key):nullptr) {
            emitter->stopCreateParticle();emitter->quitImmortalEmitter();emitter->becomeInvalidEmitter();
        }
        key=0;
    }
}
void update_fire_edge() {
    auto* link=daAlink_getAlinkActorClass();
    if (!in_arena() || !link || !s_run.rank(Upgrade::FireEdge) || !link->mSwordModel || link->checkWolf()) {
        clear_fire_edge();return;
    }
    const u16 flame=bow_fire_effect();
    if (!flame) {clear_fire_edge();return;}
    if(!full_visual(s_fireEdgeEmitters,survivor_perf::Effect::Other,3))return;
    for (unsigned i=0;i<s_fireEdgeEmitters.size();++i) {
        // Combat endpoints can be stale while sheathed and are extended by
        // Reach. Use the current visible sword model, including its back pose.
        const float length=link->checkMasterSwordEquip()?120.f:100.f;
        const cXyz local(length*float(i+1)/3.f,0,0);
        cXyz point;mDoMtx_multVec(link->mSwordModel->getBaseTRMtx(),&local,&point);
        auto& key=s_fireEdgeEmitters[i];
        auto* old=key?dComIfGp_particle_getEmitter(key):nullptr;
        if (!old || old->isEnableDeleteEmitter()) key=0;
        key=dComIfGp_particle_set(key,flame,&point,nullptr,nullptr);
        if (auto* emitter=dComIfGp_particle_getEmitter(key)) emitter->setGlobalScale(cXyz(.35f,.35f,.35f));
    }
}
void update_dark_aura() {
    auto* link=daAlink_getAlinkActorClass();
    if (!in_arena() || !link || !s_run.rank(Upgrade::DarkAura) || !s_run.dark_active()) return;
    const cXyz from=link->current.pos+cXyz(0,70,0);
    const float radius=s_run.dark_aura_radius();
    for (auto& enemy:s_enemies) {
        if (!enemy.initialized || enemy.retiring || enemy.credited) continue;
        auto* actor=actor_by_id(enemy.id);
        if (!actor || actor->health<=0 || (actor->current.pos-link->current.pos).abs2()>radius*radius) continue;
        cXyz to=actor->eyePos;dBgS_LinChk wall;wall.Set(&from,&to,link);
        if (dComIfG_Bgsp().LineCross(&wall)) continue;
        enemy.dormant=false;
        // Max run HP, not current HP. This fixed damage must not receive Dark
        // Link's multiplier, critical scaling or a second Fire Edge trigger.
        auto& burn=effect_for(enemy).legendaryBurns[2];
        burn.ignite(unsigned(std::max(1,int(actor->field_0x560))));
        // The status loop ticks immediately after this refresh. Preserve the
        // full five-second tail, including its fifth pulse, after leaving.
        ++burn.remaining;
    }
}
