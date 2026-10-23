// One encounter owns the Bullbo and both riders. Only the mount has run HP,
// kill credit and rewards; rider contacts feed its existing child-hit channel.
void after_bullbo_field_move(ModContext*,void* args,void*,void*) {
    auto* boar=mods::arg<e_wb_class*>(args,0);
    auto* owner=boar?encounter_owner(&boar->enemy):nullptr;
    if(!owner||!owner->elite||!encounter_running(*owner)||!playable()||owner->stagger)return;
    if(!s_bossRing.anchored||s_bossRing.retiring) {owner->bullboReturning=false;return;}
    auto& actor=boar->enemy;
    const cXyz offset=actor.current.pos-s_bossRing.center;
    // Run after native wait/run steering but BEFORE action() integrates motion.
    // Anticipate six frames at the boss/Endless movement speed, with body room
    // inside the wall. Native run mode 3 otherwise charges straight for 40 frames.
    const float movement=1.5f*float(std::min(32767.0,s_run.endless_factor()));
    const float lookahead=std::max(0.f,actor.speedF)*movement*6.f;
    const cXyz projected=actor.current.pos+
        cXyz(cM_ssin(actor.current.angle.y)*lookahead,0,cM_scos(actor.current.angle.y)*lookahead);
    if(owner->bullboReturning) {
        if(offset.absXZ()<=boss_ring_radius*.5f)owner->bullboReturning=false;
    } else if(!inside_boss_ring(projected,300.f))owner->bullboReturning=true;
    if(!owner->bullboReturning)return;

    // Keep an inward goal until well clear of the wall, even when Link stands
    // at its edge. Stop while turning instead of sliding sideways into it.
    const s16 inward=cM_atan2s(-offset.x,-offset.z);
    cLib_addCalcAngleS2(&actor.current.angle.y,inward,1,0x1000);
    const int error=s16(inward-actor.current.angle.y);
    if(std::abs(error)>0x1000)actor.speedF=0;
    else {
        actor.speedF=std::min(actor.speedF,37.5f/movement);
        if(boar->action==ACT_F_WAIT) {
            boar->action=ACT_C_F_RUN;boar->ride_mode=0;
        }
    }
}
Enemy* bullbo_rider_owner(e_rd_class* rider) {
    if(!rider || !in_arena())return nullptr;
    const auto* child=tracked_enemy_arrow(fopAcM_GetID(&rider->enemy));
    auto* owner=child?tracked(child->parent):nullptr;
    return owner && owner->profile==fpcNm_E_WB_e ? owner : nullptr;
}
bool maintain_bullbo_riders(e_wb_class* boar,Enemy& owner) {
    // Keep IDs with the owner through watchdog/tactics resets and pending
    // creations. Child hooks own pause/deletion and nested rider arrows.
    auto& riders=owner.bullboRiders;
    bool ready=true;
    for(unsigned seat=0;seat<riders.size();++seat) {
        auto* rider=actor_by_id(riders[seat]);
        if(rider && fpcM_IsExecuting(riders[seat]))continue;
        ready=false;
        if(!rider && !fpcM_IsCreating(riders[seat]) && owner.age%30==0) {
            // Front driver carries a club; the rear passenger shoots a bow.
            const u32 params=seat==0?0xff000101u:0xff000202u;
            riders[seat]=fopAcM_createChild(fpcNm_E_RD_e,owner.id,params,
                &boar->enemy.current.pos,fopAcM_GetRoomNo(&boar->enemy),
                &boar->enemy.shape_angle,nullptr,-1,nullptr);
        }
    }
    boar->rd_id=riders[0];
    return ready;
}
HookAction before_bullbo_action(ModContext*,void* args,void*,void*) {
    auto* boar=mods::arg<e_wb_class*>(args,0);
    auto* owner=boar?encounter_owner(&boar->enemy):nullptr;
    if(!owner)return HOOK_CONTINUE;
    if(!encounter_running(*owner))return HOOK_SKIP_ORIGINAL;
    owner->bullboTargetsPending[0]=0x7f; // Native Execute rewrites all seven radii.
    boar->status_flag|=3; // Both seats occupied; never enter riderless flight.
    boar->demo_mode=0;boar->no_draw=false;
    if(!maintain_bullbo_riders(boar,*owner)) {
        boar->enemy.speedF=0;
        return HOOK_SKIP_ORIGINAL;
    }
    if(!owner->encounterReady) {
        // Native field wait/run chases Link on foot, charges, brakes and turns
        // back. It needs neither a course path nor the horseback story event.
        boar->action=ACT_F_WAIT;boar->ride_mode=0;boar->arg0=0;boar->leader=0;
        boar->field_0x7a6=boar->field_0x7a7=0;
        owner->encounterReady=true;
    }
    return HOOK_CONTINUE;
}
HookAction before_bullbo_rider(ModContext*,void* args,void* result,void*) {
    auto* rider=mods::arg<e_rd_class*>(args,0);
    auto* owner=bullbo_rider_owner(rider);
    const auto* child=rider?tracked_enemy_arrow(fopAcM_GetID(&rider->enemy)):nullptr;
    if(!owner && !(child && child->retiring))return HOOK_CONTINUE;
    auto* mount=owner?actor_by_id(owner->id):nullptr;
    if(!owner || child->retiring || !encounter_running(*owner) || owner->stagger ||
       !mount || mount->health<=0 || !fpcM_IsExecuting(owner->id)) {
        *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
    }
    auto* boar=reinterpret_cast<e_wb_class*>(mount);
    rider->boar_id=owner->id;
    rider->boar_stand=(fopAcM_GetParam(&rider->enemy)&255)==1?1:2;
    owner->bullboTargetsPending[rider->boar_stand]=7;
    rider->ride_mode=2;rider->actor_set=0;rider->demo_mode=0;
    rider->damage_timer=0;rider->one_hit_kill=0;
    rider->enemy.health=rider->enemy.field_0x560=mount->health;
    if(rider->action!=13){rider->action=13;rider->mode=0;} // Native ACTION_WB_RUN.
    boar->status_flag|=3;
    if(rider->boar_stand==1)boar->rd_id=fopAcM_GetID(&rider->enemy);
    // Native damage is bypassed above; an invulnerability timer would move
    // all three rider targets far outside the map during native Execute.
    // One extra decrement halves only the mounted bow's readiness timer.
    if(rider->weapon_type>=2&&rider->boar_stand==2&&rider->timer[1]>0)--rider->timer[1];
    return HOOK_CONTINUE; // Native saddle-joint placement, animations and attacks.
}
HookAction before_bullbo_rider_damage(ModContext*,void* args,void*,void*) {
    auto* rider=mods::arg<e_rd_class*>(args,0);
    auto* owner=bullbo_rider_owner(rider);
    if(!owner)return HOOK_CONTINUE;
    for(auto& target:rider->cc_sph) {
        if(!owner->retiring && !owner->credited && !owner->childHit &&
           target.ChkTgHit() && roster_attack(target.GetTgHitObj()))
            owner->childHit=target.GetTgHitObj();
        target.ResetTgHit();
    }
    return HOOK_SKIP_ORIGINAL; // No native dismount, rider HP or story death.
}
HookAction before_field_boss_targets(ModContext*,void* args,void*,void*) {
    auto* object=mods::arg<cCcD_Obj*>(args,1);
    auto* actor=object?object->GetAc():nullptr;
    auto* owner=actor&&in_arena()?enemy_child_root(fopAcM_GetID(actor)):nullptr;
    if(!owner || !owner->elite ||
       (owner->profile!=fpcNm_E_WB_e && owner->profile!=fpcNm_B_GG_e))return HOOK_CONTINUE;
    // Normalize native attack power to 1: Bullbo/riders/arrows and elite
    // Aeralfos use x8 (two hearts), applied once at Link's damage boundary.
    // Normalize charge/sword/rider/projectile colliders at Set,
    // after native code writes its per-animation power. Never multiply in place.
    object->SetAtAtp(1);
    if(owner->profile==fpcNm_E_WB_e && fopAcM_GetName(actor)==fpcNm_E_RD_e) {
        auto* rider=reinterpret_cast<e_rd_class*>(actor);
        const unsigned seat=(fopAcM_GetParam(actor)&255)==1?1:2;
        for(unsigned i=0;i<3;++i)if(object==&rider->cc_sph[i]) {
            auto& target=rider->cc_sph[i];
            if(owner->retiring||owner->credited)target.OffTgSetBit();
            else allow_roster_target(&target);
            if(owner->bullboTargetsPending[seat]&(1u<<i)) {
                target.SetR(target.GetR()*1.5f);owner->bullboTargetsPending[seat]&=~(1u<<i);
            }
        }
    }
    if(owner->profile==fpcNm_E_WB_e && fopAcM_GetName(actor)==fpcNm_E_WB_e) {
        auto* boar=reinterpret_cast<e_wb_class*>(actor);
        // Stagger re-registers existing targets without native Execute. Consume
        // one bit per native radius write so that repeated Sets never compound.
        for(unsigned i=0;i<7;++i)if(object==&boar->cc_sph[i]&&(owner->bullboTargetsPending[0]&(1u<<i))) {
            auto& target=boar->cc_sph[i];target.SetR(target.GetR()*1.5f);
            owner->bullboTargetsPending[0]&=~(1u<<i);
        }
    }
    if(owner->retiring || owner->credited || owner->stagger)object->OffAtSetBit();
    return HOOK_CONTINUE;
}
