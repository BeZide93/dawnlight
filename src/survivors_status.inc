// Run-local burns and secondary melee hits enter the owned damage path:
// native handlers for the original roster, the shared adapter for new species.
struct EnemyEffect {
    DamageQueue attribution;
    DamageSource fireEdgeSource=DamageSource::Sword;
    std::array<unsigned,2> burnBase{};
    ActorId id = kNone;
    unsigned burn = 0, pulse = ticks_per_second, pending = 0, meleeSerial = 0;
    unsigned damage = 0, jumpSerial = 0, fixedPending=0, fixedDamage=0;
    std::array<LegendaryBurn,3> legendaryBurns{};
    std::vector<uint64_t> bowVolleys;
    bool pendingStagger = false, stagger = false;
    bool injected = false, pendingCritical = false, critical = false;
    cXyz velocity{}, knockStep{};
    unsigned knockFrames=0, knockCooldown=0;
    std::array<u32, 2> emitters{};
    std::array<BlackFlameCallback,2> blackFlames{};
    dCcD_Stts status;
    dCcD_Sph collider;
};
std::array<EnemyEffect, enemy_capacity> s_effects;
std::array<bool,enemy_capacity> s_burnVisuals{};
void queue_effect_damage(EnemyEffect& effect,DamageSource source,unsigned power,bool fixed=false) {
    (fixed?effect.fixedPending:effect.pending)+=power;
    effect.attribution.add(source,power,fixed);
}
void clear_fire_edge();
unsigned s_meleeSerial = 1, s_spreadSerial = 0;
void stop_burn_visual(EnemyEffect& effect) {
    s_visualBudget.release(effect.emitters.data());
    for (unsigned i=0;i<effect.emitters.size();++i) {
        auto& key=effect.emitters[i];
        if (auto* emitter = key ? dComIfGp_particle_getEmitter(key) : nullptr) {
            effect.blackFlames[i].unbind(emitter);
            emitter->stopCreateParticle(); emitter->quitImmortalEmitter();
            emitter->becomeInvalidEmitter();
        }
        key = 0;
    }
}
void stop_burn(EnemyEffect& effect) {
    stop_burn_visual(effect);
    effect.burn = 0; effect.pulse = ticks_per_second;
    effect.legendaryBurns={};
}
void clear_enemy_effects() {
    clear_fire_edge();
    for (auto& effect : s_effects) {
        effect.attribution={};effect.burnBase={};effect.fireEdgeSource=DamageSource::Sword; stop_burn(effect); effect.id = kNone; effect.pending = effect.damage = effect.meleeSerial = 0;
        effect.fixedPending=effect.fixedDamage=0;
        effect.injected = false; effect.pendingCritical = effect.critical = false;
        effect.knockStep=cXyz(0,0,0);effect.knockFrames=effect.knockCooldown=0;
        effect.jumpSerial=0;effect.pendingStagger=effect.stagger=false;effect.bowVolleys={};
    }
}
bool effect_collider(const cCcD_Obj* collider) {
    for (const auto& effect : s_effects) if (collider == &effect.collider) return true;
    return false;
}
bool effect_staggers(const cCcD_Obj* collider) {
    for(const auto& effect:s_effects) if(collider==&effect.collider) return effect.stagger;
    return false;
}
EnemyEffect& effect_for(Enemy& enemy) {
    auto& effect = s_effects[unsigned(&enemy - s_enemies.data())];
    if (effect.id != enemy.id) {
        effect.attribution={};effect.burnBase={};effect.fireEdgeSource=DamageSource::Sword; stop_burn(effect); effect.id = enemy.id;
        effect.pending = effect.damage = effect.meleeSerial = 0;
        effect.fixedPending=effect.fixedDamage=0; effect.injected = false; effect.pendingCritical = effect.critical = false;
        effect.knockStep=cXyz(0,0,0);effect.knockFrames=effect.knockCooldown=0;
        effect.jumpSerial=0;effect.pendingStagger=effect.stagger=false;effect.bowVolleys={};
    }
    return effect;
}
#include "survivors_legendary_status.inc"
std::array<dCcD_Sph*, 3> enemy_spheres(fopAc_ac_c* actor, ProfileName profile) {
    if (profile == fpcNm_E_OC_e) {
        auto* e = static_cast<daE_OC_c*>(actor); return {&e->mSphs_cc[0], &e->mSphs_cc[1]};
    }
    if (profile == fpcNm_E_BA_e) return {&reinterpret_cast<e_ba_class*>(actor)->mSph, nullptr};
    if (profile == fpcNm_E_MS_e) return {&static_cast<e_ms_class*>(actor)->mSph1, nullptr};
    if (profile == fpcNm_E_RD_e) {
        auto* e=reinterpret_cast<e_rd_class*>(actor); return {&e->cc_sph[0], &e->cc_sph[1], &e->cc_sph[2]};
    }
    return {};
}
void prepare_effect_hit(fopAc_ac_c* actor, Enemy& enemy) {
    auto& effect = effect_for(enemy);
    const auto spheres = enemy_spheres(actor, enemy.profile);
    if (!spheres[0] || actor->health <= 0) return;
    bool natural = false;
    for (auto* sphere : spheres) if (sphere && sphere->ChkTgHit()) {
        if (s_run.dual_wield && effect.meleeSerial == s_meleeSerial &&
            s_swordAction == SwordAction::Normal && native_sword_collider(sphere->GetTgHitObj()))
            sphere->ResetTgHit(); // A queued/accepted multihit cannot also hit via another capsule.
        else natural = true;
    }
    if ((!effect.pending && !effect.fixedPending) || natural) return; // Preserve real hits; queued damage waits its turn.
    auto* link = daAlink_getAlinkActorClass();
    if (!link || fopAcM_GetID(link) != s_player) return;
    effect.attribution.consume();
    effect.damage = effect.pending; effect.pending = 0; effect.injected = true;
    effect.fixedDamage=effect.fixedPending;effect.fixedPending=0;
    effect.critical=effect.pendingCritical; effect.pendingCritical=false;
    effect.stagger=effect.pendingStagger;effect.pendingStagger=false;
    effect.status.Init(120, 0xff, link);
    effect.collider.Set(kAttackSource); effect.collider.SetStts(&effect.status);
    effect.collider.SetAtType(AT_TYPE_ARROW); // No native sword/ball/bomb special-case damage.
    effect.collider.SetAtAtp(1); effect.collider.SetAtMtrl(dCcD_MTRL_NONE);
    auto point = actor->eyePos;
    spheres[0]->SetTgHit(&effect.collider); spheres[0]->SetTgHitApid(s_player); spheres[0]->SetTgHitPos(point);
    if (enemy.profile == fpcNm_E_OC_e) static_cast<daE_OC_c*>(actor)->field_0x6cc = 0;
    else if (enemy.profile == fpcNm_E_BA_e) reinterpret_cast<e_ba_class*>(actor)->mIFrames = 0;
    else if (enemy.profile == fpcNm_E_MS_e) static_cast<e_ms_class*>(actor)->mCooldown1 = 0;
    else if (enemy.profile == fpcNm_E_RD_e) reinterpret_cast<e_rd_class*>(actor)->damage_timer = 0;
}
void finish_effect_hit(fopAc_ac_c* actor, ProfileName profile) {
    auto* enemy = tracked(fopAcM_GetID(actor)); if (!enemy) return;
    auto& effect = effect_for(*enemy); if (!effect.injected) return;
    for (auto* sphere : enemy_spheres(actor, profile))
        if (sphere && sphere->GetTgHitObj() == &effect.collider) sphere->ResetTgHit();
    dCcU_AtInfo* info = profile == fpcNm_E_OC_e ? &static_cast<daE_OC_c*>(actor)->mAtInfo :
        profile == fpcNm_E_BA_e ? &reinterpret_cast<e_ba_class*>(actor)->mAtInfo :
        profile == fpcNm_E_RD_e ? &reinterpret_cast<e_rd_class*>(actor)->at_info : &static_cast<e_ms_class*>(actor)->mAtInfo;
    if (info->mpCollider == &effect.collider) info->mpCollider = nullptr;
    effect.injected = false; effect.critical = false; effect.damage = 0;effect.fixedDamage=0;effect.stagger=false;
}
void spread_melee_hit(fopAc_ac_c* primary) {
    if (!s_run.dual_wield || s_spreadSerial == s_meleeSerial) return;
    s_spreadSerial = s_meleeSerial;
    if (auto* e = tracked(fopAcM_GetID(primary))) effect_for(*e).meleeSerial = s_meleeSerial;
    auto* link = daAlink_getAlinkActorClass(); if (!link) return;
    for (unsigned n = 0; n + 1 < s_run.melee_targets(); ++n) {
        Enemy* best = nullptr; float distance = s_run.melee_reach(280) * s_run.melee_reach(280);
        const cXyz from = link->current.pos + cXyz(0, 70, 0);
        for (auto& e : s_enemies) {
            if (!e.initialized || e.retiring || e.credited || effect_for(e).meleeSerial == s_meleeSerial) continue;
            auto* actor = actor_by_id(e.id); if (!actor || actor->health <= 0 || actor == primary || boss_blocks_from(e,actor,link->current.pos)) continue;
            const auto delta = actor->current.pos - link->current.pos;
            const float d = delta.x * delta.x + delta.z * delta.z;
            if (d >= distance || std::abs(delta.y) > 180) continue;
            cXyz to = actor->eyePos; dBgS_LinChk wall; wall.Set(&from, &to, link);
            if (dComIfG_Bgsp().LineCross(&wall)) continue;
            best = &e; distance = d;
        }
        if (!best) break;
        auto& effect = effect_for(*best); effect.meleeSerial = s_meleeSerial;
        // Roll once per secondary target before combining with non-critical burn ticks.
        bool critical=false;
        queue_effect_damage(effect,DamageSource::DualWield,s_run.critical_power(s_run.melee_power(),&critical,true));
        effect.attribution.swordPending=true;
        effect.pendingCritical = effect.pendingCritical || critical;
        if(auto* actor=actor_by_id(best->id))trigger_lightning(actor);
        if (s_run.rank(Upgrade::FireEdge)) {
            effect.fireEdgeSource=DamageSource::Sword;effect.burnBase[1]=s_run.melee_power();
            effect.legendaryBurns[1].ignite(s_run.dark_power(s_run.melee_power()));
        }
        if (auto* actor=actor_by_id(best->id)) queue_knockback(*best,actor,false,DamageSource::DualWield);
    }
}
void record_status_hit(fopAc_ac_c* actor, dCcU_AtInfo* info) {
    if (!in_arena() || s_run.phase != Phase::Running || !actor || !info || !info->mAttackPower) return;
    auto* enemy = tracked(fopAcM_GetID(actor));
    if (!enemy || !enemy->initialized || enemy->retiring || enemy->credited) return;
    // Collisions from enemies/the environment do not provoke a waiting guard.
    if (info->mpCollider && info->mpCollider->GetAc() == daAlink_getAlinkActorClass())
        enemy->dormant = false;
    // Synthetic aggregate/burn colliders were already accounted for at their
    // original contacts. They must not create a second push when consumed.
    if (info->mpCollider && info->mpCollider->GetAc() == daAlink_getAlinkActorClass() &&
        !effect_collider(info->mpCollider)) {
        bool heavy=false;DamageSource knockSource=DamageSource::Other;
        if(native_sword_collider(info->mpCollider))knockSource=s_swordAction==SwordAction::Spin?DamageSource::Spin:s_swordAction==SwordAction::Jump?DamageSource::Jump:DamageSource::Sword;
        auto* link=daAlink_getAlinkActorClass();
        if (link && info->mpCollider==&link->mGuardAtCps) heavy=true;
        for (const auto& attack:s_attacks) if (info->mpCollider==&attack.collider) {
            heavy=attack.weapon==unsigned(Upgrade::Ball) || attack.weapon==unsigned(Upgrade::ShieldAttack);
            knockSource=weapon_damage_source(attack.weapon);
        }
        queue_knockback(*enemy,actor,heavy,knockSource);
    }
    if (s_run.burn_damage() && actor->health > 0) for (const auto& arrow : s_attacks) {
        if (info->mpCollider == &arrow.collider && arrow.weapon == unsigned(Upgrade::Bow)) {
            auto& effect = effect_for(*enemy);
            if (!effect.burn) effect.pulse = ticks_per_second;
            effect.burn = s_run.burn_duration(); // Refresh duration, not the damage clock.
            break;
        }
    }
    if (actor->health>0 && !effect_collider(info->mpCollider)) {
        auto& effect=effect_for(*enemy);
        bool melee=native_sword_collider(info->mpCollider) ||
            (daAlink_getAlinkActorClass() && info->mpCollider==&daAlink_getAlinkActorClass()->mGuardAtCps);
        DamageSource meleeSource=DamageSource::Sword;
        if(native_sword_collider(info->mpCollider))meleeSource=s_swordAction==SwordAction::Jump?DamageSource::Jump:
            s_swordAction==SwordAction::Spin?DamageSource::Spin:DamageSource::Sword;
        else if(melee)meleeSource=DamageSource::Shield;
        Attack* bombArrow=nullptr;
        for (auto& attack:s_attacks) if (info->mpCollider==&attack.collider) {
            if (attack.weapon==unsigned(Upgrade::Bombs) && s_run.rank(Upgrade::FireBomb)) {
                effect.burnBase[0]=s_run.attack_power(unsigned(Upgrade::Bombs));
                effect.legendaryBurns[0].ignite(s_run.dark_power(effect.burnBase[0]));
            }
            meleeSource=weapon_damage_source(attack.weapon);
            melee=melee || attack.weapon==unsigned(Upgrade::Spin) || attack.weapon==unsigned(Upgrade::ShieldAttack);
            if (attack.weapon==unsigned(Upgrade::Bow) && attack.bombArrow && !attack.detonated) bombArrow=&attack;
            break;
        }
        if (melee && s_run.rank(Upgrade::FireEdge)) {
            effect.fireEdgeSource=meleeSource;effect.burnBase[1]=s_run.melee_power();
            effect.legendaryBurns[1].ignite(s_run.dark_power(effect.burnBase[1]));
        }
        // Allocation may grow the deque. Do it only after the traversal ends.
        if (bombArrow) detonate_bomb_arrow(*bombArrow,actor->eyePos);
    }
    const auto* link=daAlink_getAlinkActorClass();
    if(native_sword_collider(info->mpCollider) || (link && info->mpCollider==&link->mGuardAtCps)) trigger_lightning(actor);
    if (s_swordAction == SwordAction::Normal && native_sword_collider(info->mpCollider)) spread_melee_hit(actor);
}
void update_enemy_effects() {
    update_fire_edge();
    update_dark_aura();
    for (unsigned i = 0; i < s_enemies.size(); ++i) {
        auto& enemy = s_enemies[i]; auto& effect = s_effects[i];
        auto* actor = actor_by_id(enemy.id);
        if (!in_arena() || !enemy.initialized || enemy.retiring || enemy.credited || !actor ||
            actor->health <= 0 || effect.id != enemy.id) {
            effect.attribution={};effect.burnBase={};effect.fireEdgeSource=DamageSource::Sword; stop_burn(effect); effect.pending = effect.fixedPending = effect.fixedDamage = 0; effect.pendingCritical=effect.critical=false;
            effect.knockFrames=effect.knockCooldown=0;effect.knockStep=cXyz(0,0,0);continue;
        }
        update_knockback(enemy,actor);
        const std::array burnSources{DamageSource::Bombs,effect.fireEdgeSource,DamageSource::DarkAura};
        for(unsigned b=0;b<burnSources.size();++b) {
            auto& burn=effect.legendaryBurns[b];const unsigned power=burn.tick();
            const unsigned base=b<2?effect.burnBase[b]:burn.tenths;
            const unsigned bonus=base && burn.tenths>base ?
                unsigned((uint64_t(power)*(burn.tenths-base)+burn.tenths/2)/burn.tenths):0;
            queue_effect_damage(effect,burnSources[b],power-bonus,true);
            queue_effect_damage(effect,DamageSource::DarkLink,bonus,true);
        }
        if (effect.burn) {
            if (!--effect.pulse) { queue_effect_damage(effect,DamageSource::FireArrows,s_run.burn_damage()); effect.pulse = ticks_per_second; }
            --effect.burn;
        }
        const bool legendary=std::any_of(effect.legendaryBurns.begin(),effect.legendaryBurns.end(),
            [](const auto& burn){return burn.remaining!=0;});
        if (!effect.burn && !legendary) { stop_burn(effect); continue; }
        if(survivor_perf::enabled() && (!s_burnVisuals[i] ||
            !s_visualBudget.acquire(effect.emitters.data(),survivor_perf::Effect::Burn,2))) {
            stop_burn_visual(effect);continue; // Damage clocks above always advance.
        }
        // Same native BURNS_A/B, trace callback, anchor fallback and scale as
        // baxlek/Fire-Arrows-Standalone. No external texture/model assets.
        effect.velocity = actor->speed;
        cXyz point = actor->eyePos.abs(actor->current.pos) <= 300 ? actor->eyePos : actor->current.pos + cXyz(0, 40, 0);
        constexpr std::array<u16, 2> particles{ID_ZI_J_LK_BURNS_A, ID_ZI_J_LK_BURNS_B};
        for (unsigned j = 0; j < particles.size(); ++j) {
            auto& key = effect.emitters[j];
            auto* old = key ? dComIfGp_particle_getEmitter(key) : nullptr;
            if (!old || old->isEnableDeleteEmitter()) key = 0;
            key = dComIfGp_particle_set(key, particles[j], &point, nullptr, nullptr);
            if (auto* emitter = dComIfGp_particle_getEmitter(key)) {
                emitter->setParticleCallBackPtr(dPa_control_c::getParticleTracePCB());
                emitter->setUserWork(reinterpret_cast<uintptr_t>(&effect.velocity));
                emitter->setGlobalScale(cXyz(3, 3, 3));
                const bool black=effect.legendaryBurns[2].remaining!=0;
                if (black) effect.blackFlames[j].bind(emitter);else effect.blackFlames[j].unbind(emitter);
                emitter->setGlobalPrmColor(black?0:255,black?0:255,black?0:255);
                emitter->setGlobalEnvColor(black?0:255,black?0:255,black?0:255);
            }
        }
    }
}

DamageAmounts hit_damage_sources(const cCcD_Obj* collider) {
    if(!collider)return {};
    for(const auto& effect:s_effects)if(collider==&effect.collider)
        return effect.attribution.weights(effect.damage,s_run.dark_power(effect.damage),effect.fixedDamage);
    DamageAmounts result{};
    DamageSource source=DamageSource::Other;bool owned=false;
    if(native_sword_collider(collider)) {
        owned=true;
        source=s_swordAction==SwordAction::Jump?DamageSource::Jump:
            s_swordAction==SwordAction::Spin?DamageSource::Spin:DamageSource::Sword;
    }
    auto* link=daAlink_getAlinkActorClass();
    if(link && collider==&link->mGuardAtCps){owned=true;source=DamageSource::Shield;}
    for(const auto& attack:s_attacks)if(collider==&attack.collider){owned=true;source=weapon_damage_source(attack.weapon);break;}
    if(owned)result[unsigned(source)]=1;
    return result;
}

bool sword_kill_hit(const cCcD_Obj* collider) {
    for(const auto& effect:s_effects)if(collider==&effect.collider)return effect.attribution.swordApplied;
    const auto sources=hit_damage_sources(collider);
    for(unsigned i=0;i<sources.size();++i)if(sources[i]&&sword_kill_source(DamageSource(i)))return true;
    return false;
}
