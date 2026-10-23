// Run-owned combat director. Native actors keep their animation/collision and
// damage paths. Targets are sampled once, never by moving the real player.
cXyz boss_lead_target(float ticks=boss_lead_ticks) {
    auto* link=daAlink_getAlinkActorClass();if(!link)return {};
    cXyz lead=link->current.pos-link->old.pos;lead.y=0;
    const float length=lead.absXZ();
    if(length>80.f)lead.zero(); // Warp/Gale is not horizontal walking velocity.
    else if(length*ticks>boss_lead_limit)ticks=boss_lead_limit/length;
    return link->current.pos+lead*ticks;
}
s16 boss_target_yaw(fopAc_ac_c* actor,const cXyz& target) {
    return cM_atan2s(target.x-actor->current.pos.x,target.z-actor->current.pos.z);
}
void boss_ground_step(fopAc_ac_c* actor,const cXyz& target,float step) {
    cXyz delta=target-actor->current.pos;delta.y=0;
    const float distance=delta.absXZ();if(distance<1)return;
    cXyz next=actor->current.pos+delta*(std::min(step,distance)/distance);
    if(walkable_spawn_path(actor->current.pos,next,100.f))actor->current.pos=next;
}
// Stop in the enlarged hand's melee band before native movement/animation.
// Use world-space separation here; generic distance hooks can be inlined away.
void prepare_shadow_beast_melee(e_s1_class* actor) {
    if(actor->mAction!=2 && !(actor->mAction==3&&actor->mMode<2))return;
    auto* link=daAlink_getAlinkActorClass();if(!link)return;
    const cXyz delta=link->current.pos-actor->current.pos;
    const float distance=delta.absXZ(),scale=std::max(1.f,actor->scale.x);
    if(std::abs(delta.y)>180.f)return;
    if(distance>300.f*scale) {
        if(actor->mAction==3){actor->mAction=2;actor->mMode=0;}
        return;
    }
    if(distance>280.f*scale&&actor->mAction==2)return;
    actor->speedF=0;actor->speed.x=actor->speed.z=0;
    if(actor->mAction==2){actor->mAction=3;actor->mMode=0;}
    if(distance<220.f*scale) {
        const cXyz away=distance>1.f?delta*(-1.f/distance):
            cXyz(-cM_ssin(actor->shape_angle.y),0,-cM_scos(actor->shape_angle.y));
        const auto start=actor->current.pos;
        boss_ground_step(actor,start+away*(250.f*scale-distance),18.f);
        // Do not start a swing while successfully backing out. If terrain
        // blocks retreat, retain native combat instead of waiting forever.
        if((actor->current.pos-start).absXZ()>0&&actor->mMode==1)
            actor->mTimers[0]=std::max<int>(actor->mTimers[0],3);
    }
}
void limit_shadow_beast_approach(e_s1_class* actor,const cXyz& start) {
    if(actor->mAction!=2 && !(actor->mAction==3&&actor->mMode<2))return;
    auto* link=daAlink_getAlinkActorClass();if(!link)return;
    if(std::abs(link->current.pos.y-start.y)>180.f)return;
    cXyz offset=start-link->current.pos,step=actor->current.pos-start;
    offset.y=step.y=0;
    const float radius=250.f*std::max(1.f,actor->scale.x);
    const float length2=step.x*step.x+step.z*step.z;
    const float outside=offset.x*offset.x+offset.z*offset.z-radius*radius;
    const float along=offset.x*step.x+offset.z*step.z;
    if(length2<.001f||outside<=0||along>=0)return;
    const float discriminant=along*along-length2*outside;
    if(discriminant<0)return;
    const float fraction=(-along-std::sqrt(discriminant))/length2;
    if(fraction<0||fraction>1)return;
    // Clip the actual boosted step at its FIRST intersection with the melee
    // ring, even if high Endless speed would cross Link and exit the far side.
    actor->current.pos.x=start.x+step.x*fraction;
    actor->current.pos.z=start.z+step.z*fraction;
    actor->speedF=0;actor->speed.x=actor->speed.z=0;
}
bool boss_major_ready(Enemy& e,unsigned duration) {
    auto& own=s_bossTactics[e.id];
    for(const auto& [id,t]:s_bossTactics)
        if(id!=e.id && t.majorUntil>s_run.ticks && tracked(id) && !tracked(id)->retiring && !tracked(id)->credited)return false;
    own.majorUntil=s_run.ticks+duration;return true;
}
void boss_idle(Enemy& e,fopAc_ac_c* actor) {
    actor->speed.zero();actor->speedF=0;
    switch(e.profile) {
    case fpcNm_E_DN_e:{auto* a=reinterpret_cast<e_dn_class*>(actor);a->action=3;a->mode=0;break;}
    case fpcNm_E_MF_e:{auto* a=reinterpret_cast<e_mf_class*>(actor);a->mAction=3;a->field_0x5b4=0;break;}
    case fpcNm_E_MM_e:{auto* a=reinterpret_cast<e_mm_class*>(actor);a->action=0;a->mode=0;break;}
    case fpcNm_E_OC_e:static_cast<daE_OC_c*>(actor)->setActionMode(3,0);break;
    case fpcNm_E_RD_e:{auto* a=reinterpret_cast<e_rd_class*>(actor);a->action=5;a->mode=0;break;}
    case fpcNm_E_RDY_e:{auto* a=reinterpret_cast<e_rdy_class*>(actor);a->mAction=3;a->mMode=0;break;}
    case fpcNm_E_KR_e:{auto* a=reinterpret_cast<e_kr_class*>(actor);a->mCurAction=3;a->field_0x672=0;break;}
    case fpcNm_E_SH_e:{auto* a=reinterpret_cast<e_sh_class*>(actor);a->field_0x676=2;a->field_0x678=0;break;}
    case fpcNm_E_SF_e:{auto* a=reinterpret_cast<e_sf_class*>(actor);a->mAction=3;a->mActionPhase=0;break;}
    case fpcNm_E_S1_e:{auto* a=reinterpret_cast<e_s1_class*>(actor);a->mAction=2;a->mMode=0;break;}
    case fpcNm_B_TN_e:{auto* a=static_cast<daB_TN_c*>(actor);a->setActionMode(a->mActionMode1<8?3:9,0);break;}
    case fpcNm_E_RDB_e:{auto* a=reinterpret_cast<e_rdb_class*>(actor);a->mAction=1;a->mMode=0;break;}
    case fpcNm_E_VT_e:{auto* a=static_cast<daE_VA_c*>(actor);a->setActionMode(daE_VA_c::ACTION_OPACI_CHASE_e,12);a->setBck(30,2,3,1);a->gravity=-5;break;}
    case fpcNm_E_YMB_e:static_cast<daE_YMB_c*>(actor)->setActionMode(1,0);break;
    case fpcNm_E_YC_e:{auto* a=reinterpret_cast<e_yc_class*>(actor);a->mAction=e_yc_class::ACT_F_FLY;a->mMode=0;a->mTimer[2]=60;break;}
    case fpcNm_B_DS_e:{auto* a=static_cast<daB_DS_c*>(actor);a->mAction=daB_DS_c::ACT_B2_WAIT;a->mMode=0;a->mCreateFireBreath=false;break;}
    case fpcNm_E_TH_e:{auto* a=reinterpret_cast<e_th_class*>(actor);a->mAction=0;a->mMode=0;break;}
    case fpcNm_E_HZELDA_e:{auto* a=reinterpret_cast<e_hzelda_class*>(actor);a->mAction=0;a->mMode=0;a->mDrawTriangleAt=false;break;}
    case fpcNm_E_PZ_e:static_cast<daE_PZ_c*>(actor)->setActionMode(daE_PZ_c::ACTION_WAIT_e,0);break;
    case fpcNm_B_GND_e:{auto* a=reinterpret_cast<b_gnd_class*>(actor);a->mActionMode=10;a->mMoveMode=0;break;}
    }
}
void update_boss_watchdogs(daAlink_c* link) {
    if(!link||!playable()||s_run.phase!=Phase::Running)return;
    for(auto& e:s_enemies) {
        if(!boss_tactics_owned(e))continue;
        auto* actor=actor_by_id(e.id);if(!actor||actor->health<=0)continue;
        auto& t=s_bossTactics[e.id];
        if(t.grace)--t.grace;
        if(t.recovery)--t.recovery;
        // A remotely awakened guard or a boss escaping the native wall must
        // rejoin this arena. Reuse the bounded, collision-checked recovery.
        if(!inside_boss_ring(actor->current.pos)) {
            t.watchdog.health=actor->health;t.watchdog.quiet=300;
        }
        if(!t.watchdog.tick(actor->health))continue;
        for(unsigned attempt=0;attempt<12;++attempt) {
            const s16 yaw=s16(s_run.next_random());
            const float radius=600.f+float(s_run.next_random()%450);
            cXyz ground=link->current.pos+cXyz(cM_ssin(yaw)*radius,0,cM_scos(yaw)*radius);
            if(!inside_boss_ring(ground,150.f))continue;
            if(!walkable_spawn_path(link->current.pos,ground,100.f)||!elite_floor_position(ground))continue;
            bool occupied=false;
            for(const auto& other:s_enemies)if(other.id!=e.id&&boss_tactics_owned(other))
                if(auto* ac=actor_by_id(other.id);ac&&ac->current.pos.absXZ(ground)<350.f){occupied=true;break;}
            if(occupied)continue;
            cXyz pos=ground;pos.y+=native_roster[e.species].height;
            const cXyz shift=pos-actor->current.pos;
            actor->current.pos=actor->old.pos=actor->home.pos=pos;
            actor->eyePos+=shift;actor->attention_info.position+=shift;
            e.spawnPosition=pos;e.lootPosition=ground;e.encounterTarget=pos;
            e.stagger=0;e.throwLife=0;
            if(e.extraAttackReady)e.extraAttack.OffAtSetBit();
            // Retain available structural children, but never bring a projectile
            // into Link's path. Cancel unknown pending children; existing native
            // attachment guards retry structural creation after deferred teardown.
            for(auto& child:s_enemyArrows)if(child.id!=kNone&&child.parent==e.id) {
                auto* ac=actor_by_id(child.id);
                if(!ac||!structural_enemy_child(fopAcM_GetName(ac))){child.retiring=true;continue;}
                if(ac){ac->current.pos+=shift;ac->old.pos=ac->current.pos;ac->home.pos+=shift;ac->speed.zero();ac->speedF=0;}
                if(ac&&fopAcM_GetName(ac)==fpcNm_E_TH_BALL_e) {
                    auto* ball=static_cast<e_th_ball_class*>(ac);ball->mAction=0;ball->mMode=0;
                    for(auto& point:ball->field_0x65c.m_pos)point+=shift;
                    for(auto& point:ball->field_0x65c.field_0x320)point+=shift;
                    for(auto* chain:{&ball->field_0xdec,&ball->field_0x11d8}) {
                        for(auto& point:chain->m_pos)point+=shift;
                        for(auto& point:chain->field_0x140)point+=shift;
                        for(auto& velocity:chain->field_0x230)velocity.zero();
                    }
                }
            }
            if(e.profile==fpcNm_B_TN_e) {
                auto* a=static_cast<daB_TN_c*>(actor);
                for(auto& p:a->mPositions)p+=shift;
                for(auto& p:a->mPositionsCopy)p+=shift;
                a->mStates[13]=a->mActionMode1<8?1:7;
            }
            if(e.profile==fpcNm_E_VT_e) {
                auto* a=static_cast<daE_VA_c*>(actor);
                for(unsigned i=0;i<2;++i){a->field_0x1228[i]=0;a->mMagicSphs[i].OffAtSetBit();}
            }
            boss_idle(e,actor);
            clear_zelda_light_orbs(e.id);clear_king_waves(e.id);
            t=BossTactics{};t.watchdog.damaged(actor->health);t.grace=boss_relocation_grace;
            break;
        }
    }
}
struct BossAttackView {int action=0,mode=0;bool active=false;};
BossAttackView boss_attack_view(const Enemy& e,fopAc_ac_c* actor) {
#define VIEW(P,T,A,M,ACTIVE) case P:{auto* a=reinterpret_cast<T*>(actor);return {int(a->A),int(a->M),(ACTIVE)};}
    switch(e.profile) {
    VIEW(fpcNm_E_DN_e,e_dn_class,action,mode,a->action==4||a->action==5||a->action==6)
    VIEW(fpcNm_E_MF_e,e_mf_class,mAction,field_0x5b4,a->mAction==5||a->mAction==6)
    VIEW(fpcNm_E_OC_e,daE_OC_c,mActionMode,mOcState,a->mActionMode==4)
    VIEW(fpcNm_E_SH_e,e_sh_class,field_0x676,field_0x678,a->field_0x676==3)
    VIEW(fpcNm_E_SF_e,e_sf_class,mAction,mActionPhase,a->mAction==4||a->mAction==5)
    VIEW(fpcNm_E_S1_e,e_s1_class,mAction,mMode,a->mAction==3&&a->mMode==2)
    VIEW(fpcNm_B_TN_e,daB_TN_c,mActionMode1,mActionMode2,a->mActionMode1==4||a->mActionMode1==10)
    VIEW(fpcNm_E_RDB_e,e_rdb_class,mAction,mMode,a->mAction==3||a->mAction==4)
    default:return {};
    }
#undef VIEW
}
void boss_followup(Enemy& e,fopAc_ac_c* actor) {
    switch(e.profile) {
    case fpcNm_E_DN_e:{auto* a=reinterpret_cast<e_dn_class*>(actor);a->action=6;a->mode=0;break;}
    case fpcNm_E_MF_e:{auto* a=reinterpret_cast<e_mf_class*>(actor);a->mAction=6;a->field_0x5b4=0;break;}
    case fpcNm_E_OC_e:static_cast<daE_OC_c*>(actor)->setActionMode(4,0);break;
    case fpcNm_E_SH_e:{auto* a=reinterpret_cast<e_sh_class*>(actor);a->field_0x676=3;a->field_0x678=0;break;}
    case fpcNm_E_S1_e:{auto* a=reinterpret_cast<e_s1_class*>(actor);a->mAction=3;a->mMode=1;a->mTimers[0]=12;break;}
    case fpcNm_B_TN_e:static_cast<daB_TN_c*>(actor)->setActionMode(10,10);break;
    default:break;
    }
}
void prepare_stalhound_pounce(Enemy& e,e_sh_class* actor) {
    auto& t=s_bossTactics[e.id];
    if(actor->field_0x676!=3 || actor->field_0x678<2)return;
    auto* body=&actor->enemy;
    if(!t.targetLocked) {
        // Alignment can take many ticks. Commit when the visible tell begins,
        // not to the old direction sampled before native alignment started.
        t.target=boss_lead_target(6.f);t.yaw=boss_target_yaw(body,t.target);
        t.targetLocked=true;
    }
    actor->field_0x68c=t.yaw;
    body->current.angle.y=body->shape_angle.y=t.yaw;
}
void before_boss_tactics(Enemy& e,fopAc_ac_c* actor) {
    if(!boss_tactics_owned(e)||!playable())return;
    auto& t=s_bossTactics[e.id];
    boss_queue_shots(e,actor);
    auto view=boss_attack_view(e,actor);
    if(boss_attack_paused(e)) {boss_idle(e,actor);t.attacking=false;return;}
    if(e.profile==fpcNm_E_S1_e&&!e.stagger&&actor->health>0) {
        prepare_shadow_beast_melee(reinterpret_cast<e_s1_class*>(actor));
        t.origin=actor->current.pos;
    }
    if(e.profile==fpcNm_E_DN_e&&!e.stagger&&actor->health>0) {
        t.origin=actor->current.pos;
        auto* a=reinterpret_cast<e_dn_class*>(actor);
        auto* link=daAlink_getAlinkActorClass();
        if(link&&a->action==3) {
            const auto delta=link->current.pos-actor->current.pos;
            const float distance=delta.absXZ(),scale=std::max(1.f,actor->scale.x);
            if(std::abs(delta.y)<=180.f&&distance<220.f*scale) {
                const auto away=distance>1.f?delta*(-1.f/distance):
                    cXyz(-cM_ssin(actor->shape_angle.y),0,-cM_scos(actor->shape_angle.y));
                boss_ground_step(actor,link->current.pos+away*(250.f*scale),18.f);
            }
        }
    }
    if(e.profile==fpcNm_E_SF_e&&!e.stagger&&actor->health>0)t.origin=actor->current.pos;
    boss_mobile_patterns(e,actor);
    const bool entry=view.active&&!t.attacking;
    if(entry) {
        t.phaseTick=0;t.target=boss_lead_target();t.yaw=boss_target_yaw(actor,t.target);
        t.origin=actor->current.pos;t.targetLocked=e.profile!=fpcNm_E_SH_e;
        if(e.profile==fpcNm_E_SF_e) {
            auto* a=reinterpret_cast<e_sf_class*>(actor);
            a->mAction=(++t.sequence%2)?5:4;a->mActionPhase=0;
        }
    }
    if(view.active)++t.phaseTick;
    auto body=extended_body(actor,e.profile);
    const float frame=body.morph?body.morph->getFrame():0;
    if(e.profile==fpcNm_B_TN_e) {
        auto* a=static_cast<daB_TN_c*>(actor);
        if(a->mTimer3>t.lastTimer)a->mTimer3=std::max(1,int(a->mTimer3*.7f));
        t.lastTimer=a->mTimer3;
        if(view.active&&view.mode>0){a->mpModelMorf1->setPlaySpeed(1.2f);a->mpModelMorf2->setPlaySpeed(1.2f);}
        if(a->mActionMode1==10&&t.phaseTick<=6)boss_ground_step(actor,t.target,18.f);
    } else if(e.profile==fpcNm_E_RDB_e) {
        auto* a=reinterpret_cast<e_rdb_class*>(actor);
        if(a->mAction==3&&a->mMode>0&&frame<62)body.morph->setPlaySpeed(4.f/3.f);
        else if(a->mAction==3&&a->mMode>0)body.morph->setPlaySpeed(1.f);
        if(a->mAction==4) {
            if(entry&&!boss_major_ready(e,150)){boss_idle(e,actor);e.specialCooldown=15;return;}
            if(a->mMode==1 && t.phaseTick>18 && t.phaseTick<=30)boss_ground_step(actor,t.target,24.f);
            if(a->mMode==2) {
                const s16 desired=boss_target_yaw(actor,boss_lead_target(0));
                t.yaw+=std::clamp<int>(s16(desired-t.yaw),-180,180);
                boss_ground_step(actor,actor->current.pos+cXyz(cM_ssin(t.yaw)*100,0,cM_scos(t.yaw)*100),8.f);
            }
        }
    } else if(e.profile==fpcNm_E_MF_e) {
        auto* a=reinterpret_cast<e_mf_class*>(actor);
        if(view.active&&view.mode>0)body.morph->setPlaySpeed(1.2f);
        if(view.active&&frame>=15)a->mYAngleToPlayer=t.yaw;
        if(a->mAction==6)a->field_0x6c0[1]=0; // no unbounded native tail/knife loop
    } else if(e.profile==fpcNm_E_DN_e) {
        auto* a=reinterpret_cast<e_dn_class*>(actor);
        if(view.active&&t.phaseTick<=8&&t.combo==0) {
            boss_ground_step(actor,t.target,14.f);
            // Small hop with a checked landing; not a ballistic cliff launch.
            actor->current.pos.y+=std::sin(float(t.phaseTick)*3.14159265f/8.f)*35.f;
        }
        if(view.active&&frame>=12)a->search_angle_y=t.yaw;
    } else if(e.profile==fpcNm_E_OC_e) {
        if(!view.active&&fopAcM_searchPlayerDistance(actor)>400.f&&fopAcM_searchPlayerDistance(actor)<1000.f)
            boss_ground_step(actor,boss_lead_target(),e.eliteSlot>=int(timed_elite_count)?6.f:12.f);
    } else if(e.profile==fpcNm_E_SF_e) {
        auto* a=reinterpret_cast<e_sf_class*>(actor);
        // Preserve the native exact-frame jump at frame 10.
        if(view.active&&view.mode>0&&frame>55&&frame<81)body.morph->setPlaySpeed(4.f/3.f);
        else if(view.active&&view.mode>0)body.morph->setPlaySpeed(1.f);
        if(view.active&&t.phaseTick<=8)boss_ground_step(actor,t.target,12.f);
        if(view.active&&frame>=58)a->mPlayerAngleY=t.yaw;
    } else if(e.profile==fpcNm_E_SH_e) {
        auto* a=reinterpret_cast<e_sh_class*>(actor);
        if(view.active)prepare_stalhound_pounce(e,a);
        if(t.combo&&a->field_0x678==3)actor->speedF=std::min(actor->speedF,25.f);
    } else if(e.profile==fpcNm_E_RDY_e && e.eliteSlot>=int(timed_elite_count)) {
        auto* a=reinterpret_cast<e_rdy_class*>(actor);
        if(a->mAction==3&&fopAcM_searchPlayerDistance(actor)>400)boss_ground_step(actor,boss_lead_target(),6.f);
    }
    t.attacking=view.active;t.lastAction=view.action;t.lastMode=view.mode;
}
void after_boss_tactics(Enemy& e,fopAc_ac_c* actor) {
    if(!boss_tactics_owned(e)||!playable())return;
    boss_after_patterns(e,actor);
    auto& t=s_bossTactics[e.id];const auto view=boss_attack_view(e,actor);
    if(t.attacking&&!view.active) {
        bool follow=e.profile==fpcNm_E_MF_e||e.profile==fpcNm_E_OC_e||e.profile==fpcNm_E_S1_e||
            (e.profile==fpcNm_B_TN_e&&t.lastAction==10)||
            ((e.profile==fpcNm_E_DN_e||e.profile==fpcNm_E_SH_e)&&t.sequence++%2==0);
        if(e.eliteSlot>=int(timed_elite_count))follow=false;
        if(follow&&!t.combo&&fopAcM_searchPlayerDistance(actor)<900.f) {
            t.combo=1;boss_followup(e,actor);t.attacking=false;
        } else {t.combo=0;t.recovery=(e.profile==fpcNm_E_SF_e||e.profile==fpcNm_E_RDB_e)?12:24;t.attacking=false;}
    }
}

bool s_bossExtraShot=false;
csXyz boss_fireball_angle(const cXyz& position,const cXyz& target) {
    const cXyz d=target-position;
    // Native B/C bullets rotate +Z by Y then X: positive pitch points down.
    return csXyz(-cM_atan2s(d.y,d.absXZ()),cM_atan2s(d.x,d.z),0);
}
void after_boss_child_spawn(ModContext*,void* args,void* result,void*) {
    const ActorId id=*static_cast<ActorId*>(result);
    if(id==kNone||!active_enemy_arrow(id))return;
    auto* owner=enemy_child_root(mods::arg<ActorId>(args,1));
    if(!owner||!boss_tactics_owned(*owner))return;
    const auto profile=mods::arg<s16>(args,0);
    if(profile==fpcNm_E_FK_e) {
        const auto& t=s_bossTactics[owner->id];
        if(t.pendingRiders&&t.volley)s_enemyHitSources[id].projectile=t.volley;
        return;
    }
    if(profile!=fpcNm_B_DS_e&&profile!=fpcNm_E_ARROW_e)return;
    if(profile==fpcNm_B_DS_e&&(mods::arg<u32>(args,2)&0xff)!=daB_DS_c::TYPE_BULLET_C&&
       (mods::arg<u32>(args,2)&0xff)!=daB_DS_c::TYPE_BULLET_B)return;
    auto& t=s_bossTactics[owner->id];
    auto& shot=s_bossTactics[id];
    const auto* angle=mods::arg<const csXyz*>(args,5);
    if(angle)shot.shotAngle=*angle;
    shot.target=boss_lead_target(profile==fpcNm_B_DS_e?0.f:boss_lead_ticks)+cXyz(0,60,0);
    shot.targetLocked=profile==fpcNm_B_DS_e;
    if(profile==fpcNm_E_ARROW_e && !s_bossExtraShot) {
        if(!t.fanPending){t.volley=new_enemy_attack_use();t.fanPending=true;}
        s_enemyHitSources[id].projectile=t.volley;
        if(!s_spawningEnemyArrows)t.fanPending=false;
    }
    if(s_bossExtraShot||s_spawningEnemyArrows)return;
    if(owner->profile==fpcNm_E_YC_e && t.waitingVolley) {
        t.waitingVolley=false;t.motionTick=0;
    }
    if(profile==fpcNm_B_DS_e) {
        t.queuedShots=2;t.shotDelay=9;t.shotIndex=0;++t.sequence;
        t.shotTarget=shot.target;t.shotPosition=*mods::arg<const cXyz*>(args,3);
        t.shotParams=mods::arg<u32>(args,2);
        t.shotAngle=shot.shotAngle=boss_fireball_angle(t.shotPosition,t.shotTarget);
        if(t.sequence%3==0) {shot.shotAngle.y-=0x1000;shot.targetLocked=false;}
    } else if(owner->profile==fpcNm_E_RD_e && owner->arrowVolley%2==0) {
        t.queuedShots=1;t.shotDelay=9;t.shotIndex=0;
        t.shotPosition=*mods::arg<const cXyz*>(args,3);t.shotParams=mods::arg<u32>(args,2);
        if(angle)t.shotAngle=*angle;
    }
}
void boss_queue_shots(Enemy& e,fopAc_ac_c* actor) {
    auto& t=s_bossTactics[e.id];if(!t.queuedShots)return;
    if(t.shotDelay){--t.shotDelay;return;}
    ++t.shotIndex;--t.queuedShots;t.shotDelay=8;
    const bool head=e.profile==fpcNm_B_DS_e;
    cXyz pos=head?static_cast<daB_DS_c*>(actor)->mMouthPos:actor->current.pos+cXyz(0,130,0);
    csXyz angle=head?boss_fireball_angle(pos,t.shotTarget):t.shotAngle;
    const s16 centerYaw=angle.y;
    if(head&&t.sequence%3==0)angle.y+=s16((int(t.shotIndex)-1)*0x1000);
    s_bossExtraShot=true;
    const bool oldSpread=s_spawningEnemyArrows;s_spawningEnemyArrows=true;
    const ActorId id=fopAcM_createChild(head?fpcNm_B_DS_e:fpcNm_E_ARROW_e,e.id,t.shotParams,
        &pos,fopAcM_GetRoomNo(actor),&angle,nullptr,-1,nullptr);
    s_spawningEnemyArrows=oldSpread;s_bossExtraShot=false;
    if(id!=kNone&&head) {
        auto& child=s_bossTactics[id];child.shotAngle=angle;
        child.target=t.shotTarget+cXyz(cM_scos(centerYaw)*(t.shotIndex==1?-180.f:180.f),0,
                                   -cM_ssin(centerYaw)*(t.shotIndex==1?-180.f:180.f));
        child.targetLocked=t.sequence%3!=0;
    }
    if(id!=kNone&&!head&&t.volley)s_enemyHitSources[id].projectile=t.volley;
}
void update_boss_projectile(fopAc_ac_c* actor,bool after) {
    auto* owner=enemy_child_root(fopAcM_GetID(actor));
    if(!owner||!boss_tactics_owned(*owner))return;
    const auto profile=fopAcM_GetName(actor);
    auto& t=s_bossTactics[fopAcM_GetID(actor)];
    if(profile==fpcNm_E_ARROW_e) {
        if(after&&!t.projectileReady&&actor->speedF>0){actor->speedF*=1.15f;t.projectileReady=true;}
    } else if(profile==fpcNm_E_PZ_e) {
        auto* a=static_cast<daE_PZ_c*>(actor);
        if(a->arg0==40&&after&&!t.projectileReady&&a->mMoveMode==1) {
            actor->speedF*=1.3f;actor->speed.y*=1.3f;t.projectileReady=true;
        }
    } else if(profile==fpcNm_B_DS_e) {
        auto* a=static_cast<daB_DS_c*>(actor);
        if((a->arg0==daB_DS_c::TYPE_BULLET_B||a->arg0==daB_DS_c::TYPE_BULLET_C)&&
           !t.projectileReady&&a->mMode==2) {
            // Create executes the native initialization inline. Aim before the
            // first mode-2 movement, not after it has flown along the head pose.
            // The post pass also handles a deferred mode-1 -> mode-2 transition.
            actor->current.angle=t.targetLocked?boss_fireball_angle(actor->current.pos,t.target):t.shotAngle;
            actor->speedF=125.f;t.projectileReady=true;
        }
    } else if(profile==fpcNm_E_TH_BALL_e) {
        auto* a=static_cast<e_th_ball_class*>(actor);
        if(a->mAction!=2)t.projectileReady=false;
        else if(after&&!t.projectileReady&&a->speedF>0){a->speedF*=1.25f;t.projectileReady=true;}
    }
}
bool boss_pz_minion(Enemy& e,ProfileName profile) {
    auto& t=s_bossTactics[e.id];
    const unsigned cap=profile==fpcNm_E_YK_e?6:3;
    unsigned live=0;
    for(const auto& other:s_enemies)if(other.id!=kNone&&!other.retiring&&!other.credited) {
        auto it=s_bossTactics.find(other.id);
        if(it!=s_bossTactics.end()&&it->second.summoner==e.id)++live;
    }
    if(t.summons>=cap||live>=6)return false;
    return true;
}
void after_boss_sword_throw(ModContext*,void* args,void*,void*) {
    auto* a=mods::arg<daB_TN_c*>(args,0);auto* e=encounter_owner(a);
    if(!e||!e->throwLife)return;
    auto& t=s_bossTactics[e->id];
    if(a->mStates[13]==3 && t.swordState==2) {
        cXyz d=t.throwTarget+cXyz(0,200,0)-a->mPositions[13];
        if(d.abs()>1)a->field_0x8dc[13]=d*(80.f/d.abs());
    }
    t.swordState=a->mStates[13];
}
HookAction before_boss_ball_collision(ModContext*,void* args,void*,void*) {
    auto* object=mods::arg<cCcD_Obj*>(args,1);
    if(!object||!in_arena()||!playable()||s_run.phase!=Phase::Running)return HOOK_CONTINUE;
    auto* actor=object->GetAc();
    if(!actor||!active_enemy_arrow(fopAcM_GetID(actor)))return HOOK_CONTINUE;
    auto* owner=enemy_child_root(fopAcM_GetID(actor));
    if(!owner||!boss_tactics_owned(*owner))return HOOK_CONTINUE;
    if(fopAcM_GetName(actor)==fpcNm_B_DS_e&&owner->profile==fpcNm_B_DS_e) {
        auto* bullet=static_cast<daB_DS_c*>(actor);
        // Without the spinner-path charge, native B/C bullets stay at radius
        // 80 despite their full-size flame effect. Use the native charged
        // radius at registration, after executeBullet rewrites the sphere.
        if((bullet->arg0==daB_DS_c::TYPE_BULLET_B||bullet->arg0==daB_DS_c::TYPE_BULLET_C)&&
           object==&bullet->mBreathAtSph)bullet->mBreathAtSph.SetR(240.f);
        return HOOK_CONTINUE;
    }
    if(fopAcM_GetName(actor)!=fpcNm_E_TH_BALL_e||owner->profile!=fpcNm_E_TH_e)return HOOK_CONTINUE;
    auto* ball=static_cast<e_th_ball_class*>(actor);
    // Native startup hides this attack sphere 12,000 units away. While the
    // ball circles its owner, register it at the actual ball position instead.
    // Preserve the native radius, damage and the separate defensive collider.
    if(object==&ball->mAtSph&&ball->mAction==1&&ball->mMode>=1&&ball->mMode<=3)
        ball->mAtSph.SetC(ball->current.pos);
    return HOOK_CONTINUE;
}
HookAction before_boss_attack_safety(ModContext*,void* args,void*,void*) {
    auto* at=mods::arg<cCcD_Obj*>(args,1);auto* tg=mods::arg<cCcD_Obj*>(args,2);
    auto* link=daAlink_getAlinkActorClass();
    if(!in_arena()||!at||!tg||!link||tg->GetAc()!=link)return HOOK_CONTINUE;
    auto* ac=at->GetAc();auto* e=ac?enemy_child_root(fopAcM_GetID(ac)):nullptr;
    if(!e||!boss_tactics_owned(*e))return HOOK_CONTINUE;
    // Recovery applies to the body/weapon. A fired projectile keeps its travel.
    const auto& t=s_bossTactics[e->id];
    if(t.grace || (t.recovery&&fopAcM_GetID(ac)==e->id) ||
       (t.hasContact&&s_run.ticks-t.lastContact<15))return HOOK_SKIP_ORIGINAL;
    return HOOK_CONTINUE;
}

void boss_mobile_patterns(Enemy& e,fopAc_ac_c* actor) {
    auto& t=s_bossTactics[e.id];
    if(e.profile==fpcNm_E_MM_e) {
        auto* a=reinterpret_cast<e_mm_class*>(actor);
        if(a->action==1&&a->mode==3) {
            if(t.motionMode!=3){t.yaw=actor->current.angle.y;++t.passes;}
            a->target_ya=t.yaw;
            a->timers[0]=std::min<s16>(a->timers[0],36);
        }
        if(a->action==1&&a->mode==4&&actor->speedF<1.f) {
            if(t.passes==1){a->mode=0;}
            else {t.passes=0;t.recovery=36;boss_idle(e,actor);}
        }
        if(a->action==1&&a->mode==5&&t.motionMode!=5){t.passes=0;a->timers[0]=100;}
        t.motionMode=a->mode;
    } else if(e.profile==fpcNm_E_KR_e) {
        auto* a=reinterpret_cast<e_kr_class*>(actor);
        if(a->mCurAction!=3)return;
        if(a->field_0x672==4 && !a->field_0x69c[5] && fopAcM_searchPlayerDistance(actor)<1600.f) {
            a->field_0x672=7;t.target=boss_lead_target(9.f)+cXyz(0,70,0);
            t.origin=actor->current.pos;t.motionTick=0;++t.passes;
            encounter_animation(a->mpMorf,"E_kr",4,0);
        }
        if(a->field_0x672==7) {
            ++t.motionTick;
            a->field_0x678=t.target;
        } else if(t.motionMode==7) {
            if(t.passes==1) {
                const s16 side=boss_target_yaw(actor,boss_lead_target(0))+0x4000;
                a->field_0x678=boss_lead_target(0)+cXyz(cM_ssin(side)*500,180,cM_scos(side)*500);
                a->field_0x69c[0]=60;a->field_0x69c[1]=90;a->field_0x69c[5]=30;
            } else {t.passes=0;t.recovery=45;}
        }
        t.motionMode=a->field_0x672;
    } else if(e.profile==fpcNm_E_YMB_e) {
        auto* a=static_cast<daE_YMB_c*>(actor);
        if(a->mAction==2&&a->mMode==0) {
            if(!t.passes&&!boss_major_ready(e,170)){boss_idle(e,actor);t.recovery=15;return;}
        }
        if(a->mAction==2&&a->mMode==5) {
            if(t.motionMode!=5) {
                t.motionTick=0;
                const s16 side=boss_target_yaw(actor,boss_lead_target(0))+0x4000;
                t.target=boss_lead_target(0)+cXyz(cM_ssin(side)*450,200,cM_scos(side)*450);
            }
            if(t.passes==1) {
                cXyz delta=t.target-actor->current.pos;delta.y=0;
                const float distance=delta.absXZ();
                if(distance>1) {
                    cXyz next=actor->current.pos+delta*(std::min(distance,25.f)/distance);
                    dBgS_LinChk wall;wall.Set(&actor->current.pos,&next,actor);
                    if(!dComIfG_Bgsp().LineCross(&wall))actor->current.pos=next;
                }
                if(++t.motionTick>=21){a->setActionMode(2,0);a->speed.zero();a->speedF=0;}
            } else {t.passes=0;t.recovery=45;boss_idle(e,actor);}
        }
        t.motionMode=a->mMode;
    } else if(e.profile==fpcNm_E_TH_e) {
        auto* a=reinterpret_cast<e_th_class*>(actor);
        if(a->mAction==3&&a->mMode==2)a->mTimers[0]=std::min<s16>(a->mTimers[0],45);
    } else if(e.profile==fpcNm_B_GND_e) {
        auto* a=reinterpret_cast<b_gnd_class*>(actor);
        if(a->mActionMode==10) {
            if(a->field_0xc44[0]>t.lastTimer)a->field_0xc44[0]=s16(a->field_0xc44[0]*.8f);
            t.lastTimer=a->field_0xc44[0];
        }
        if(t.pendingRiders) {
            if(t.riderDelay)--t.riderDelay;
            else summon_ghost_riders(a,e);
        }
    }
}
void boss_after_patterns(Enemy& e,fopAc_ac_c* actor) {
    auto& t=s_bossTactics[e.id];
    if(e.profile==fpcNm_E_VT_e) {
        auto* a=static_cast<daE_VA_c*>(actor);
        for(unsigned i=0;i<2;++i) {
            if(a->field_0x1228[i]==2&&!t.magicActive[i]) {
                cXyz target=boss_lead_target(i?12.f:0.f);
                const cXyz d=target-a->mMagicPos[i];const float n=d.abs();
                if(n>1)a->mMagicSpeed[i]=d*(65.f/n);
                t.magicActive[i]=true;
            } else if(a->field_0x1228[i]!=2)t.magicActive[i]=false;
        }
    } else if(e.profile==fpcNm_E_YMB_e) {
        auto* a=static_cast<daE_YMB_c*>(actor);
        if(a->mAction==2&&a->mMode==1&&t.motionMode!=1)++t.passes;
    } else if(e.profile==fpcNm_E_HZELDA_e) {
        auto* a=reinterpret_cast<e_hzelda_class*>(actor);
        // Choose at the native WAIT -> attack boundary, before that attack's
        // first execution. Existing animations remain at their current 2x.
        if(a->mAction>=1&&a->mAction<=3&&a->mMode==0&&t.motionMode==0) {
            const unsigned roll=s_run.next_random()%10;
            a->mAction=roll<4?3:roll<7?1:2;
            if(!boss_major_ready(e,90)){a->mAction=0;a->mMode=0;a->mTimers[0]=15;}
        }
        if(a->mDrawTriangleAt&&!t.triangle) {
            cXyz point=(t.sequence++%2)?boss_lead_target(12.f):boss_lead_target(0);
            if(dry_arena_floor(point,500,2000))a->mTrianglePos=point+cXyz(0,5,0);
        }
        t.triangle=a->mDrawTriangleAt;t.motionMode=a->mAction;
    } else if(e.profile==fpcNm_E_S1_e) {
        auto* a=reinterpret_cast<e_s1_class*>(actor);
        if(a->mAction==3&&a->mMode==2) {
            if(!t.attacking) {
                a->mIsSlowAttack=t.combo?1:0;a->mAnm=t.combo?6:5;
                encounter_animation(a->mpMorf,"E_S2",a->mAnm,0);
                t.yaw=boss_target_yaw(actor,boss_lead_target(0));
            }
        }
    } else if(e.profile==fpcNm_E_SH_e) {
        auto* a=reinterpret_cast<e_sh_class*>(actor);
        prepare_stalhound_pounce(e,a);
    }
}

HookAction boss_carrier_pass(e_yc_class* actor,Enemy& e) {
    auto& t=s_bossTactics[e.id];
    actor->mAtSph.OffAtSetBit();
    if(boss_attack_paused(e))return HOOK_SKIP_ORIGINAL;
    if(t.motionMode!=1) {
        if(!boss_major_ready(e,150)){actor->mAction=e_yc_class::ACT_F_FLY;actor->mTimer[2]=15;return HOOK_SKIP_ORIGINAL;}
        t.motionMode=1;t.motionTick=0;t.origin=actor->current.pos;
        t.waitingVolley=actor_by_id(actor->mRiderID)!=nullptr;
        t.target=boss_lead_target(0)+cXyz(0,65,0);
        const cXyz d=t.target-t.origin;const float n=d.abs();t.direction=n>1?d*(1.f/n):cXyz(0,0,1);
        actor->mAnm=e_yc_class::ANM_FLY_BRAKE;
        encounter_animation(actor->mpMorf,"E_yc",actor->mAnm,2);
    }
    ++t.motionTick;
    if(t.waitingVolley) {
        // Let the rider finish its native draw/release before committing the dive.
        // A missing/blocked projectile cannot hold the boss indefinitely.
        if(t.motionTick<90){actor->mAcch.CrrPos(dComIfG_Bgsp());return HOOK_SKIP_ORIGINAL;}
        t.waitingVolley=false;t.motionTick=0;
    }
    if(t.motionTick==18) {
        actor->mAnm=e_yc_class::ANM_CATCH_START;
        encounter_animation(actor->mpMorf,"E_yc",actor->mAnm,0);
    }
    if(t.motionTick>=18&&t.motionTick<=47) {
        const float distance=(t.target-t.origin).abs();
        const float advance=(distance+350.f)/30.f;
        cXyz next=actor->current.pos+t.direction*advance;
        next.y=std::max(next.y,t.target.y);
        dBgS_LinChk wall;wall.Set(&actor->current.pos,&next,actor);
        if(!dComIfG_Bgsp().LineCross(&wall))actor->current.pos=next;
        else t.motionTick=48;
        actor->shape_angle.y=actor->current.angle.y=cM_atan2s(t.direction.x,t.direction.z);
        if(t.motionTick>=18&&t.motionTick<=47) {
            actor->mAtSph.SetC(actor->current.pos);actor->mAtSph.SetR(65.f*actor->scale.x);
            actor->mAtSph.OnAtSetBit();dComIfG_Ccsp()->Set(&actor->mAtSph);
        }
    }
    if(t.motionTick>=48) {
        t.motionMode=0;actor->mAction=e_yc_class::ACT_F_FLY;actor->mMode=0;actor->mTimer[2]=90;
        actor->mAnm=e_yc_class::ANM_FLY;encounter_animation(actor->mpMorf,"E_yc",actor->mAnm,2);
        actor->mAtSph.OffAtSetBit();
    }
    actor->mAcch.CrrPos(dComIfG_Bgsp());
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_boss_rider_arrow(ModContext*,void* args,void* result,void*) {
    if(!in_arena()||mods::arg<s16>(args,0)!=fpcNm_E_ARROW_e)return HOOK_CONTINUE;
    auto* e=enemy_child_root(mods::arg<ActorId>(args,1));
    if(!e||!e->elite||e->profile!=fpcNm_E_YC_e)return HOOK_CONTINUE;
    auto it=s_bossTactics.find(e->id);
    if(it!=s_bossTactics.end()&&it->second.motionMode==1&&!it->second.waitingVolley) {
        *static_cast<ActorId*>(result)=kNone;return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

void after_boss_aim(ModContext*,void* args,void* result,void*) {
    const auto* actor=mods::arg<const fopAc_ac_c*>(args,0);
    const auto* target=mods::arg<const fopAc_ac_c*>(args,1);
    if(!in_arena()||target!=daAlink_getAlinkActorClass()||!actor)return;
    auto* e=tracked(fopAcM_GetID(actor));if(!e||!boss_tactics_owned(*e))return;
    const auto it=s_bossTactics.find(e->id);if(it==s_bossTactics.end())return;
    const auto& t=it->second;bool locked=false;
    switch(e->profile) {
    case fpcNm_E_DN_e:case fpcNm_E_MF_e:case fpcNm_E_S1_e:
        locked=t.attacking&&t.phaseTick>=10;break;
    case fpcNm_E_SH_e:{const auto* a=reinterpret_cast<const e_sh_class*>(actor);
        // Phase 1 must align with the same live heading as general facing.
        // Lock only once native AI has accepted alignment and started the tell;
        // a tick-based lock here can keep its <0x800 yaw check false forever.
        locked=t.attacking&&a->field_0x676==3&&a->field_0x678>=2;break;}
    case fpcNm_B_GND_e:{const auto* a=reinterpret_cast<const b_gnd_class*>(actor);
        locked=a->mActionMode==11&&a->mMoveMode>=11&&a->mMoveMode<=13;break;}
    default:break;
    }
    if(locked)*static_cast<s16*>(result)=t.yaw;
}

void boss_record_contact(cCcD_Obj* attack) {
    auto* actor=attack?attack->GetAc():nullptr;
    auto* e=actor?enemy_child_root(fopAcM_GetID(actor)):nullptr;
    if(!e||!boss_tactics_owned(*e))return;
    auto& t=s_bossTactics[e->id];t.hasContact=true;t.lastContact=s_run.ticks;
}

HookAction before_boss_path(ModContext*,void* args,void*,void*) {
    auto* acch=mods::arg<dBgS_Acch*>(args,0);
    auto* actor=acch?acch->getMyAc():nullptr;
    auto* e=actor?tracked(fopAcM_GetID(actor)):nullptr;
    if(!e||!boss_tactics_owned(*e)||!playable()||e->stagger||actor->health<=0||
       s_run.phase!=Phase::Running||boss_attack_paused(*e))return HOOK_CONTINUE;
    auto& t=s_bossTactics[e->id];
    if(e->profile==fpcNm_E_S1_e) {
        auto* beast=reinterpret_cast<e_s1_class*>(actor);
        if(acch==&beast->mAcch)limit_shadow_beast_approach(beast,t.origin);
        return HOOK_CONTINUE;
    }
    if(e->profile==fpcNm_E_DN_e||e->profile==fpcNm_E_SF_e) {
        float meleeRadius;
        if(e->profile==fpcNm_E_DN_e) {
            auto* lizalfos=reinterpret_cast<e_dn_class*>(actor);
            if(acch!=&lizalfos->objacch||lizalfos->action<3||lizalfos->action>6)return HOOK_CONTINUE;
            meleeRadius=lizalfos->action==3?250.f:190.f;
        } else {
            auto* stalfos=reinterpret_cast<e_sf_class*>(actor);
            if(acch!=&stalfos->mBgc||(stalfos->mAction!=4&&stalfos->mAction!=5))return HOOK_CONTINUE;
            meleeRadius=250.f;
        }
        auto* link=daAlink_getAlinkActorClass();
        if(!link||std::abs(link->current.pos.y-t.origin.y)>180.f)return HOOK_CONTINUE;
        // Limit the complete native + director + movement-bonus segment at its
        // first entry into melee spacing, including a step that crosses Link.
        // Leave vertical motion and native weapon/tail hit windows untouched.
        cXyz offset=t.origin-link->current.pos,step=actor->current.pos-t.origin;
        offset.y=step.y=0;
        const float radius=meleeRadius*std::max(1.f,actor->scale.x);
        const float length2=step.x*step.x+step.z*step.z;
        const float outside=offset.x*offset.x+offset.z*offset.z-radius*radius;
        const float along=offset.x*step.x+offset.z*step.z;
        if(length2<.001f||along>=0)return HOOK_CONTINUE;
        float fraction=0;
        if(outside>0) {
            const float discriminant=along*along-length2*outside;
            if(discriminant<0)return HOOK_CONTINUE;
            fraction=(-along-std::sqrt(discriminant))/length2;
            if(fraction<0||fraction>1)return HOOK_CONTINUE;
        }
        actor->current.pos.x=t.origin.x+step.x*fraction;
        actor->current.pos.z=t.origin.z+step.z*fraction;
        actor->speedF=actor->speed.x=actor->speed.z=0;
        return HOOK_CONTINUE;
    }
    if(e->profile!=fpcNm_E_KR_e)return HOOK_CONTINUE;
    auto* bird=reinterpret_cast<e_kr_class*>(actor);
    if(acch!=&bird->mAcch||bird->field_0x672!=7||t.motionTick<15||t.motionTick>27)return HOOK_CONTINUE;
    const cXyz d=t.target-t.origin;
    actor->current.pos=t.origin+d*(float(t.motionTick-14)/10.f);
    actor->current.pos.y=std::max(actor->current.pos.y,t.target.y);
    actor->current.angle.y=actor->shape_angle.y=cM_atan2s(d.x,d.z);
    acch->OnLineCheck();return HOOK_CONTINUE;
}
