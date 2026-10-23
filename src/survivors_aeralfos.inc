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
HookAction before_aeralfos_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daB_GG_c*>(args,0);auto* owner=aeralfos_owner(actor);
    if(!owner)return HOOK_CONTINUE;
    if(!encounter_running(*owner)||!playable()||owner->stagger)return HOOK_SKIP_ORIGINAL;
    auto& cycle=owner->aeralfosCycle;
    if(cycle.advance()) {
        aeralfos_end_attack(actor);
        if(!cycle.ground) {
            // Native takeoff animation, with a capped ascent below.
            actor->SetAction(1,1,4);actor->mAnm=-1;
        }
    }
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
    return HOOK_CONTINUE;
}
void after_aeralfos_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daB_GG_c*>(args,0);auto* owner=aeralfos_owner(actor);
    if(!owner||!encounter_running(*owner)||!playable()||owner->stagger||owner->aeralfosCycle.ground)return;
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
