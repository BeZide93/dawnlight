// Native Stalfos assumes a lock-on duel: player_way_check requires Link to face
// it, and its final action() block can cancel every swing into GUARD. Survivors
// allows free movement/automatic attacks, so scope those rules to this actor.
struct StalfosActionFrame {
    void* args;e_sf_class* actor;u8 shield;
    bool readinessMasked=false,attackEligible=false;
};
std::vector<StalfosActionFrame> s_stalfosActionFrames;
Enemy* active_stalfos(e_sf_class* actor) {
    auto* body=reinterpret_cast<fopAc_ac_c*>(actor);
    auto* e=body?tracked(fopAcM_GetID(body)):nullptr;
    return e&&e->profile==fpcNm_E_SF_e&&boss_tactics_owned(*e)&&!e->stagger&&
        body->health>0&&s_run.phase==Phase::Running&&playable()?e:nullptr;
}
// damage_check is an existing exported hook, called after action() refreshes
// player angles/distance. player_way_check is absent from the iOS manifest.
HookAction before_stalfos_readiness(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<e_sf_class*>(args,0);
    if(s_stalfosActionFrames.empty()||s_stalfosActionFrames.back().actor!=actor||
       !active_stalfos(actor)||actor->mAction!=3||actor->mTimers[2]!=0)return HOOK_CONTINUE;
    auto* link=daAlink_getAlinkActorClass();if(!link)return HOOK_CONTINUE;
    auto* body=reinterpret_cast<fopAc_ac_c*>(actor);
    auto& frame=s_stalfosActionFrames.back();
    frame.readinessMasked=true;
    frame.attackEligible=actor->mPlayerAngleX<0x1000&&actor->mPlayerAngleX>-0x1000&&
        (link->current.pos-body->current.pos).absXZ()<300.f*body->scale.x&&
        !fopAcM_otherBgCheck(link,body);
    // Let native chase/notice/movement run, but defer its ready attack selection
    // until action's post hook. This prevents a second roll when Link faces us.
    actor->mTimers[2]=1;
    return HOOK_CONTINUE;
}
HookAction before_stalfos_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<e_sf_class*>(args,0);auto* e=active_stalfos(actor);
    if(!e)return HOOK_CONTINUE;
    auto* body=reinterpret_cast<fopAc_ac_c*>(actor);
    auto& t=s_bossTactics[e->id];
    if(t.stalfosGuardCooldown)--t.stalfosGuardCooldown;
    const u8 shield=actor->arg3;
    s_stalfosActionFrames.push_back({args,actor,shield});
    // Only hide the shield from action()'s automatic guard selector. Restore
    // before Execute builds shield matrices/colliders; native guard stays intact.
    actor->arg3=0xff;
    if(actor->mAction==7) {
        if(actor->mActionPhase==2&&++t.stalfosGuardHold>=6) {
            actor->mActionPhase=3;actor->mTimers[0]=0;
            actor->mpModelMorf->setPlaySpeed(1.f);
        }
        return HOOK_CONTINUE;
    }
    t.stalfosGuardHold=0;
    if(actor->mAction!=3||shield==0xff||t.stalfosGuardCooldown||boss_attack_paused(*e))return HOOK_CONTINUE;
    auto* link=daAlink_getAlinkActorClass();if(!link)return HOOK_CONTINUE;
    const auto delta=link->current.pos-body->current.pos;
    const auto yaw=s16(cM_atan2s(delta.x,delta.z)-body->shape_angle.y);
    if(delta.absXZ()>300.f*body->scale.x||std::abs(delta.y)>180.f||std::abs(int(yaw))>0x4000||
       !(cc_pl_cut_bit_get()&0x3ff)||fopAcM_otherBgCheck(link,body))return HOOK_CONTINUE;
    // One 20% roll per two active seconds, including failed rolls. Repeated
    // auto-slashes cannot reroll every frame or interrupt an existing attack.
    t.stalfosGuardCooldown=60;
    if(s_run.next_random()%5==0) {
        actor->mAction=7;actor->mActionPhase=0;actor->mTimers[0]=0;
        body->speedF=0;
    }
    return HOOK_CONTINUE;
}
void after_stalfos_action(ModContext*,void* args,void*,void*) {
    if(s_stalfosActionFrames.empty()||s_stalfosActionFrames.back().args!=args)return;
    const auto frame=s_stalfosActionFrames.back();s_stalfosActionFrames.pop_back();
    auto* actor=frame.actor;
    actor->arg3=frame.shield;
    if(frame.readinessMasked) {
        actor->mTimers[2]=0;
        // Preserve native loss-of-target transitions and the 50% attack roll,
        // 10-29 tick retry wait and weapon-type alternation. Like native chase,
        // selection here starts the attack animation on the next Execute.
        if(frame.attackEligible&&actor->mAction==3&&active_stalfos(actor)) {
            actor->mTimers[2]=cM_rndF(20.f)+10.f;
            if(cM_rndF(1.f)<0.5f) {
                actor->mAction=actor->mType==0?5:4;
                actor->mActionPhase=0;
            }
        }
    }
    // Guard's vanilla invulnerability parks ALL body targets far away. Actual
    // directional blocks below must leave the rear and recovery vulnerable.
    frame.actor->mInvulnerabilityTimer=0;
}
