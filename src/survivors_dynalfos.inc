// Scope native duel assumptions to an owned Dynalfos action. The temporary
// guard mask is exactly 1: native guard requires <1, recoil movement requires >1.
struct DynalfosActionFrame {
    void* args;e_mf_class* actor;float recoil;
    bool wasGuard=false,masked=false,ready=false,eligible=false;
};
std::vector<DynalfosActionFrame> s_dynalfosActionFrames;
Enemy* active_dynalfos(e_mf_class* actor) {
    auto* body=reinterpret_cast<fopAc_ac_c*>(actor);
    auto* e=body?tracked(fopAcM_GetID(body)):nullptr;
    return e&&e->profile==fpcNm_E_MF_e&&boss_tactics_owned(*e)&&!e->stagger&&
        body->health>0&&s_run.phase==Phase::Running&&playable()?e:nullptr;
}
HookAction before_dynalfos_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<e_mf_class*>(args,0);auto* e=active_dynalfos(actor);
    if(!e)return HOOK_CONTINUE;
    auto* body=reinterpret_cast<fopAc_ac_c*>(actor);auto& t=s_bossTactics[e->id];
    if(t.dynalfosGuardCooldown)--t.dynalfosGuardCooldown;
    t.origin=body->current.pos;
    s_dynalfosActionFrames.push_back({args,actor,actor->field_0x72c,actor->mAction==7});
    if(actor->mAction==7) {
        if(actor->field_0x5b4==7&&++t.dynalfosGuardHold>=6) {
            actor->field_0x5b4=8;actor->field_0x6c0[0]=0;actor->mpModelMorf->setPlaySpeed(1.f);
        }
    } else t.dynalfosGuardHold=0;
    return HOOK_CONTINUE;
}
HookAction before_dynalfos_readiness(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<e_mf_class*>(args,0);auto* e=active_dynalfos(actor);
    if(!e||s_dynalfosActionFrames.empty()||s_dynalfosActionFrames.back().actor!=actor||actor->mAction!=3)return HOOK_CONTINUE;
    auto* link=daAlink_getAlinkActorClass();if(!link)return HOOK_CONTINUE;
    auto* body=reinterpret_cast<fopAc_ac_c*>(actor);auto& frame=s_dynalfosActionFrames.back();
    auto& t=s_bossTactics[e->id];const float scale=std::max(1.f,body->scale.x);
    auto delta=link->current.pos-body->current.pos;
    const bool sight=std::abs(delta.y)<=180.f&&!fopAcM_otherBgCheck(link,body);
    if(sight&&delta.absXZ()<220.f*scale) {
        const float distance=delta.absXZ();
        const auto away=distance>1.f?delta*(-1.f/distance):cXyz(-cM_ssin(body->shape_angle.y),0,-cM_scos(body->shape_angle.y));
        boss_ground_step(body,link->current.pos+away*(250.f*scale),18.f);
        delta=link->current.pos-body->current.pos;
    }
    // action() has just refreshed this field in world units. Normalize once.
    actor->mPlayerDistance=delta.absXZ()/scale;
    const bool inRange=sight&&actor->mPlayerDistance>=220.f&&actor->mPlayerDistance<350.f&&
        actor->mXAngleToPlayer<0x1000&&actor->mXAngleToPlayer>-0x1000;
    if(sight&&actor->mPlayerDistance<350.f)body->speedF=body->speed.x=body->speed.z=0;
    const s16 yaw=s16(cM_atan2s(delta.x,delta.z)-body->shape_angle.y);
    if(inRange&&!boss_attack_paused(*e)&&!t.dynalfosGuardCooldown&&
       std::abs(int(yaw))<=0x4000&&(cc_pl_cut_bit_get()&0x3ff)) {
        t.dynalfosGuardCooldown=60; // Failed rolls also wait two active seconds.
        if(s_run.next_random()%5==0) {
            actor->mAction=7;actor->field_0x5b4=5;actor->field_0x6c0[0]=6;
            body->speedF=body->speed.x=body->speed.z=0;
            return HOOK_CONTINUE;
        }
    }
    if(actor->field_0x72c<1.f){frame.masked=true;actor->field_0x72c=1.f;}
    // Defer ready selection until after native chase, retaining its entry wait,
    // cliff navigation and loss-of-target transitions without Link-facing veto.
    if(actor->field_0x6c0[2]==0) {
        frame.ready=true;frame.eligible=inRange&&!boss_attack_paused(*e);
        actor->field_0x6c0[2]=1;
    }
    return HOOK_CONTINUE;
}
void after_dynalfos_action(ModContext*,void* args,void*,void*) {
    if(s_dynalfosActionFrames.empty()||s_dynalfosActionFrames.back().args!=args)return;
    const auto frame=s_dynalfosActionFrames.back();s_dynalfosActionFrames.pop_back();auto* a=frame.actor;
    if(frame.masked&&a->field_0x72c==1.f)a->field_0x72c=frame.recoil;
    if(frame.ready&&a->field_0x6c0[2]==1) {
        a->field_0x6c0[2]=0;
        if(frame.eligible&&a->mAction==3&&active_dynalfos(a)) {
            a->field_0x6c0[2]=cM_rndF(60.f)+30.f;
            a->mAction=cM_rndF(1.f)<.65f?5:6;a->field_0x5b4=0;
        }
    }
    // Native guard hides every body target. Keep rear/startup/release vulnerable;
    // the shared directional shield-contact path handles the visible block.
    if(frame.wasGuard||a->mAction==7)a->field_0x6c8=0;
}
HookAction before_dynalfos_path(ModContext*,void* args,void*,void*) {
    auto* acch=mods::arg<dBgS_Acch*>(args,0);auto* body=acch?acch->getMyAc():nullptr;
    auto* a=reinterpret_cast<e_mf_class*>(body);auto* e=active_dynalfos(a);
    if(!e||acch!=&a->mObjAcch||boss_attack_paused(*e)||!(a->mAction==3||a->mAction==5))return HOOK_CONTINUE;
    auto* link=daAlink_getAlinkActorClass();if(!link)return HOOK_CONTINUE;
    const auto start=s_bossTactics[e->id].origin;
    if(std::abs(link->current.pos.y-start.y)>180.f)return HOOK_CONTINUE;
    cXyz offset=start-link->current.pos,step=body->current.pos-start;offset.y=step.y=0;
    const float radius=(a->mAction==3?250.f:190.f)*std::max(1.f,body->scale.x);
    const float length2=step.x*step.x+step.z*step.z;
    const float outside=offset.x*offset.x+offset.z*offset.z-radius*radius;
    const float along=offset.x*step.x+offset.z*step.z;
    if(length2<.001f||along>=0)return HOOK_CONTINUE;
    float fraction=0;
    if(outside>0) {
        const float discriminant=along*along-length2*outside;if(discriminant<0)return HOOK_CONTINUE;
        fraction=(-along-std::sqrt(discriminant))/length2;if(fraction<0||fraction>1)return HOOK_CONTINUE;
    }
    body->current.pos.x=start.x+step.x*fraction;body->current.pos.z=start.z+step.z*fraction;
    body->speedF=body->speed.x=body->speed.z=0;
    return HOOK_CONTINUE;
}
