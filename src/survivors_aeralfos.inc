// Owned elite Aeralfos only. Native action/animation IDs are local to B_GG.cpp.
Enemy* aeralfos_owner(daB_GG_c* actor) {
    auto* owner=actor?encounter_owner(actor):nullptr;
    return owner&&owner->elite&&owner->profile==fpcNm_B_GG_e?owner:nullptr;
}
float aeralfos_floor(daB_GG_c* actor) {
    const float floor=actor->mAcch.GetGroundH();
    return std::isfinite(floor)&&floor>-1.e8f?floor:actor->home.pos.y;
}
void aeralfos_end_attack(daB_GG_c* actor) {
    for(auto& sphere:actor->mCcSph){sphere.OffAtSetBit();sphere.ResetAtHit();}
    actor->field_0x65a=0;actor->SpeedClear();
}
void aeralfos_lock_dive(daB_GG_c* actor,AeralfosDive& dive) {
    if(dive.locked)return;
    auto* link=daAlink_getAlinkActorClass();
    dive.target=link?link->current.pos+cXyz(0,100,0):
        actor->current.pos+cXyz(cM_ssin(actor->current.angle.y)*600,0,cM_scos(actor->current.angle.y)*600);
    dive.locked=true;
}
HookAction before_aeralfos_air_attack(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daB_GG_c*>(args,0);auto* owner=aeralfos_owner(actor);
    if(!owner)return HOOK_CONTINUE;
    if(!encounter_running(*owner)||!playable()||owner->stagger)return HOOK_SKIP_ORIGINAL;
    if(owner->aeralfosCycle.ground)return HOOK_CONTINUE;
    // Lock before the visible windup, giving Link time to leave the attack lane.
    if(actor->mMode==0)aeralfos_lock_dive(actor,owner->aeralfosDive);
    if(actor->mMode!=1)return HOOK_CONTINUE;
    // Keep native contact bookkeeping, but replace its homing and <600-unit
    // proximity brake. FlyAction/Execute still handle movement and collision.
    actor->mSound.startCreatureSoundLevel(Z2SE_EN_GG_ATK_MOVE,0,-1);
    cLib_addCalcAngleS2(&actor->field_0x6c4,0,0x10,0x600);
    cLib_addCalcAngleS2(&actor->field_0x6be,0,0x10,0x1000);
    actor->F_AtHit();
    return HOOK_SKIP_ORIGINAL;
}
void aeralfos_dive_motion(daB_GG_c* actor,AeralfosDive& dive) {
    aeralfos_lock_dive(actor,dive);
    if(!dive.active) {
        const cXyz delta=dive.target-actor->current.pos;
        const float distance=delta.absXZ();
        const s16 yaw=distance>1.f?cM_atan2s(delta.x,delta.z):actor->current.angle.y;
        dive.velocity=cXyz(cM_ssin(yaw)*60.f,
            std::clamp(delta.y/std::max(200.f,distance)*60.f,-40.f,40.f),cM_scos(yaw)*60.f);
        dive.active=true;
    }
    // Keep the captured horizontal lane. Level out at the captured body height
    // instead of continuing down into the floor after passing through the target.
    const cXyz past=actor->current.pos-dive.target;
    const float passed=(past.x*dive.velocity.x+past.z*dive.velocity.z)/60.f;
    float vertical=dive.velocity.y;
    if(vertical<0)vertical=std::max(vertical,std::min(0.f,dive.target.y-actor->current.pos.y));
    else vertical=std::min(vertical,std::max(0.f,dive.target.y-actor->current.pos.y));
    const cXyz next=actor->current.pos+cXyz(dive.velocity.x,vertical,dive.velocity.z);
    dBgS_LinChk wall;wall.Set(&actor->current.pos,&next,actor);
    if(passed>=450.f||++dive.ticks>90||actor->mAcch.ChkWallHit()||actor->mAcch.ChkGroundHit()||
       !inside_boss_ring(next,100.f)||dComIfG_Bgsp().LineCross(&wall)) {
        aeralfos_end_attack(actor);dive={};
        actor->SetAnm(6,0,5.f,1.f);actor->SetAction(0,2,2);
        actor->field_0x648=actor->current.pos.y;actor->field_0x650=-1.f;
        return; // Native recovery, with damage disabled immediately.
    }
    const float movement=1.5f*float(std::min(32767.0,s_run.endless_factor()));
    actor->current.angle.y=actor->shape_angle.y=actor->field_0x5ba=cM_atan2s(dive.velocity.x,dive.velocity.z);
    actor->speedF=actor->field_0x5bc=60.f/movement;
    actor->speed.y=actor->field_0x5c0=vertical;actor->gravity=0;
}
bool aeralfos_ground_pressure(daB_GG_c* actor,AeralfosCycle& cycle) {
    auto* link=daAlink_getAlinkActorClass();
    if(!link)return false;
    if(cycle.groundTicks<90)++cycle.groundTicks;
    // Never interrupt an active strike or a native damage/death state.
    if(actor->mSubAction!=1){cycle.dashTicks=0;return false;}
    const cXyz delta=link->current.pos-actor->current.pos;
    const float distance=delta.absXZ();
    const s16 yaw=cM_atan2s(delta.x,delta.z);
    if(distance<=240.f&&std::abs(delta.y)<=120.f) {
        aeralfos_end_attack(actor);cycle.dashTicks=0;
        actor->current.angle.y=actor->shape_angle.y=actor->field_0x5ba=yaw;
        actor->mCcCyl.OnTgSetBit();actor->mCcShieldSph.OnTgSetBit();
        actor->SetAction(1,2,0); // Start the native strike now; no extra idle phase.
        return false;
    }
    if(!cycle.dashTicks) {
        if(cycle.groundTicks<90)return false;
        cycle.groundTicks=0;cycle.dashTicks=30;
        aeralfos_end_attack(actor);
        actor->SetAction(1,1,2);actor->SetAnm(30,2,3.f,2.f);
        actor->mCcCyl.OnTgSetBit();actor->mCcShieldSph.OnTgSetBit();
    }
    const float step=std::min(80.f,std::max(0.f,distance-180.f));
    cXyz next=actor->current.pos+cXyz(cM_ssin(yaw)*step,0,cM_scos(yaw)*step);
    // At most an 80-unit path: check slopes, water, walls and the light circle.
    if(!step||!inside_boss_ring(next,100.f)||!walkable_spawn_path(actor->current.pos,next,60.f)||!--cycle.dashTicks) {
        aeralfos_end_attack(actor);cycle.dashTicks=0;actor->mAnm=-1;
        return false;
    }
    const float movement=1.5f*float(std::min(32767.0,s_run.endless_factor()));
    actor->current.angle.y=actor->shape_angle.y=actor->field_0x5ba=yaw;
    actor->speedF=actor->field_0x5bc=step/movement;
    actor->gravity=-9.f;
    return true; // Execute still handles animation, movement and floor collision.
}
HookAction before_aeralfos_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daB_GG_c*>(args,0);auto* owner=aeralfos_owner(actor);
    if(!owner)return HOOK_CONTINUE;
    if(!encounter_running(*owner)||!playable()||owner->stagger)return HOOK_SKIP_ORIGINAL;
    auto& cycle=owner->aeralfosCycle;
    if(cycle.advance()) {
        aeralfos_end_attack(actor);
        owner->aeralfosDive={};
        if(!cycle.ground) {
            // Native takeoff animation, with a capped ascent below.
            actor->SetAction(1,1,4);actor->mAnm=-1;
        }
    }
    if(cycle.ground||actor->mAction!=0||actor->mSubAction!=2||(actor->mMode!=0&&actor->mMode!=1))
        owner->aeralfosDive={};
    if(cycle.ground) {
        if(actor->mAction!=1) {
            cXyz floor=actor->current.pos;
            // Descend only over a dry playable floor; never snap into a cliff.
            if(!loot_floor_position(floor))return HOOK_CONTINUE;
            aeralfos_end_attack(actor);
            actor->gravity=0;
            if(actor->current.pos.y>floor.y+10.f) {
                if(actor->mAnm!=14)actor->SetAnm(14,2,5.f,1.f);
                actor->speed.y=-std::min(15.f,actor->current.pos.y-floor.y);
                return HOOK_SKIP_ORIGINAL;
            }
            actor->current.pos.y=floor.y;actor->gravity=-9.f;
            actor->SetAction(1,1,2);actor->mAnm=-1;
        }
        // Keep the native walking/sword AI on the ground for this phase.
        actor->mTimers[2]=std::max<s16>(actor->mTimers[2],2);
        actor->field_0x648=actor->current.pos.y;
        if(actor->mSubAction==1&&actor->mMode==4){actor->SetAction(1,1,2);actor->mAnm=-1;}
        if(aeralfos_ground_pressure(actor,cycle))return HOOK_SKIP_ORIGINAL;
    }
    // Three extra readiness ticks make waits one quarter of native duration.
    // Animations, hit windows and phase timer retain their native speed.
    // Execute already decremented these clocks once; clamp the remainder at zero.
    const bool flyWait=actor->mAction==0 &&
        ((actor->mSubAction<=1&&actor->mMode==1)||
         (actor->mSubAction==2&&actor->mMode==10&&actor->mAnm==11));
    const bool groundWait=actor->mAction==1&&actor->mSubAction==1&&actor->mMode>=1&&actor->mMode<=3;
    if((flyWait||groundWait)&&actor->mTimers[0]>0)
        actor->mTimers[0]=std::max<s16>(0,actor->mTimers[0]-3);
    // The native type-2 pair can postpone a ready attack while its partner is
    // attacking. An owned solo boss must not wait for that shared partner state.
    if(!cycle.ground&&actor->mAction==0&&actor->mSubAction<=1&&actor->mMode==1&&actor->mTimers[0]==0)
        actor->SetAction(0,2,10);
    return HOOK_CONTINUE;
}
void after_aeralfos_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daB_GG_c*>(args,0);auto* owner=aeralfos_owner(actor);
    if(!owner||!encounter_running(*owner)||!playable()||owner->stagger||owner->aeralfosCycle.ground)return;
    if(actor->mAction==0&&actor->mSubAction==2&&actor->mMode==1)
        aeralfos_dive_motion(actor,owner->aeralfosDive);
    const float height=actor->current.pos.y-aeralfos_floor(actor);
    // Native free-flight band is 500..800. Use 250..400 above local terrain,
    // rather than an absolute world Y (Field contains slopes and plateaus).
    if(actor->mAction==0&&actor->mSubAction<=1) {
        actor->field_0x5c0=height<250?3.f:(height>400?-3.f:std::clamp(actor->field_0x5c0,-3.f,3.f));
        if(height>=400)actor->speed.y=std::min(0.f,actor->speed.y);
    }
    // Native recovery expects to climb 500 units; the lower ceiling must also
    // complete that recovery, otherwise the dive can wait forever at the cap.
    if(actor->mAction==0&&actor->mSubAction==2&&actor->mMode==2&&actor->mAnm==7&&height>=390) {
        aeralfos_end_attack(actor);actor->SetAnm(14,2,5.f,1.f);actor->SetAction(0,1,0);
    }
}
HookAction before_aeralfos_matrix(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daB_GG_c*>(args,0);auto* owner=aeralfos_owner(actor);
    if(!owner||!encounter_running(*owner)||!playable())return HOOK_CONTINUE;
    // After native movement/floor correction, before model and collision poses.
    const float cap=aeralfos_floor(actor)+400.f;
    if(actor->current.pos.y>cap) {
        actor->current.pos.y=cap;actor->speed.y=std::min(0.f,actor->speed.y);
        actor->field_0x5c0=std::min(0.f,actor->field_0x5c0);
    }
    return HOOK_CONTINUE;
}
