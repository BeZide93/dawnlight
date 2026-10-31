enum class CombatText { Block, Dodge, Crit, Fate };
struct FloatingCombatText {
    cXyz position{};
    unsigned remaining=0;
    CombatText kind=CombatText::Crit;
    unsigned lifetime=30;
    FateEffect fate=FateEffect::Horde;
};
constexpr unsigned combat_text_lifetime=30;
std::array<FloatingCombatText,64> s_combatTexts{};
struct DamageFeedback {
    void* args;
    fopAc_ac_c* actor;
    dCcU_AtInfo* info;
    int health;
    bool critical=false;
    DamageAmounts sources{};
    bool sword=false;
};
std::vector<DamageFeedback> s_damageFeedback;
void clear_combat_feedback() { s_combatTexts={}; s_damageFeedback.clear(); }
void combat_feedback(fopAc_ac_c* actor,CombatText kind) {
    if (!actor || !in_arena() || s_run.phase!=Phase::Running) return;
    auto slot=std::min_element(s_combatTexts.begin(),s_combatTexts.end(),
        [](const auto& a,const auto& b){return a.remaining<b.remaining;});
    const float height=std::max(100.f,std::clamp(actor->eyePos.y-actor->current.pos.y,0.f,400.f))+35.f;
    *slot={actor->current.pos+cXyz(0,height,0),combat_text_lifetime,kind};
}
void fate_feedback(const cXyz& position,FateEffect effect) {
    auto slot=std::min_element(s_combatTexts.begin(),s_combatTexts.end(),
        [](const auto& a,const auto& b){return a.remaining<b.remaining;});
    *slot={position+cXyz(0,80,0),60,CombatText::Fate,60,effect};
}
void update_combat_feedback() {
    for (auto& text:s_combatTexts) if (text.remaining) { --text.remaining; text.position.y+=1.5f; }
}
void begin_damage_feedback(void* args,fopAc_ac_c* actor,dCcU_AtInfo* info) {
    s_damageFeedback.push_back({args,actor,info,actor->health,false,hit_damage_sources(info->mpCollider),sword_kill_hit(info->mpCollider)});
}
void mark_damage_critical(dCcU_AtInfo* info,bool critical) {
    if (!s_damageFeedback.empty() && s_damageFeedback.back().info==info)
        s_damageFeedback.back().critical=critical;
}
void mark_damage_split(dCcU_AtInfo* info,unsigned base,unsigned scaled) {
    if(s_damageFeedback.empty() || s_damageFeedback.back().info!=info)return;
    auto& weights=s_damageFeedback.back().sources;
    // Direct hits start with a single ownership marker. Aggregates are already split.
    if(damage_sum(weights)!=1)return;
    for(auto& weight:weights)weight*=std::min(base,scaled);
    if(scaled>base)weights[unsigned(DamageSource::DarkLink)]+=scaled-base;
}
void end_damage_feedback(void* args) {
    if (s_damageFeedback.empty() || s_damageFeedback.back().args!=args) return;
    const auto hit=s_damageFeedback.back();s_damageFeedback.pop_back();
    if(in_arena() && !s_victory && s_run.phase==Phase::Running)
        s_run.damage.record(hit.sources,hit.health,hit.actor->health);
    if(hit.actor->health<hit.health)if(auto* enemy=tracked(fopAcM_GetID(hit.actor)))enemy->swordKill=hit.sword;
    if (hit.critical && hit.info->mAttackPower && hit.actor->health<hit.health)
        combat_feedback(hit.actor,CombatText::Crit);
}
