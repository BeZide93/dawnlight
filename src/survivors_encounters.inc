// These actors normally require a dungeon arena, Sol orb or scripted rider.
// Replace only owned action logic, retaining native resources, matrices and CC.
Enemy* encounter_owner(fopAc_ac_c* actor) {
    return roster_owned(actor)?tracked(fopAcM_GetID(actor)):nullptr;
}
void encounter_move(fopAc_ac_c* actor,const cXyz& target,float horizontal,float vertical) {
    const auto delta=target-actor->current.pos;
    const float distance=delta.absXZ();
    const float step=distance>0?std::min(1.f,horizontal/distance):0;
    actor->speed.set(delta.x*step,std::clamp(delta.y,-vertical,vertical),delta.z*step);
    actor->current.pos+=actor->speed;
    actor->speedF=0;actor->gravity=0;
    if(distance>1)actor->current.angle.y=actor->shape_angle.y=cM_atan2s(delta.x,delta.z);
}
void encounter_animation(mDoExt_McaMorfSO* morph,const char* archive,int animation,int loop=2) {
    morph->setAnm(static_cast<J3DAnmTransform*>(dComIfG_getObjectRes(archive,animation)),loop,4.f,1.f,0.f,-1.f);
}
bool encounter_running(Enemy& enemy) {
    return enemy.initialized && !enemy.retiring && !enemy.credited &&
        s_run.phase==Phase::Running && !transitioning();
}
#include "survivors_boss_specials.inc"
// Carrier Kargarok's native catch attack dereferences its scripted Bulblin
// rider. Keep its approach/brake/15-frame telegraph/catch animation sequence,
// replacing only the rider cinematic with a talon hit against Link.
HookAction before_roster_yc_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<e_yc_class*>(args,0);auto* enemy=encounter_owner(actor);
    if(!enemy)return HOOK_CONTINUE;
    if(!enemy->encounterReady) {
        actor->mRiderID=kNone;actor->mAction=e_yc_class::ACT_F_FLY;actor->mMode=0;
        actor->mAnm=e_yc_class::ANM_FLY;actor->mTimer[2]=150;
        encounter_animation(actor->mpMorf,"E_yc",actor->mAnm);enemy->encounterReady=true;
    }
    if(!encounter_running(*enemy))return HOOK_SKIP_ORIGINAL;
    auto* link=daAlink_getAlinkActorClass();if(!link)return HOOK_SKIP_ORIGINAL;
    if(enemy->elite) {
        maintain_carrier_rider(actor,*enemy);
        if(actor->mAction==e_yc_class::ACT_ATTACK)return boss_carrier_pass(actor,*enemy);
    }
    const auto animate=[&](int anm,int loop) {
        actor->mAnm=anm;encounter_animation(actor->mpMorf,"E_yc",anm,loop);
    };
    actor->mAtSph.OffAtSetBit();
    if(actor->mAction==e_yc_class::ACT_F_FLY) {
        encounter_move(actor,link->current.pos+cXyz(0,500,700),10,10);
        if(!actor->mTimer[2]){actor->mAction=e_yc_class::ACT_ATTACK;actor->mMode=0;}
    } else {
        if(actor->mMode<4)enemy->encounterTarget=link->current.pos+cXyz(0,50,0);
        const auto delta=enemy->encounterTarget-actor->current.pos;
        switch(actor->mMode) {
        case 0:actor->mMode=1;break;
        case 1:
            encounter_move(actor,enemy->encounterTarget,30,30);
            if(delta.abs()<1000){actor->mMode=2;actor->speedF=30;animate(e_yc_class::ANM_FLY_BRAKE,2);}
            break;
        case 2: {
            const float step=std::max(0.f,actor->speedF-.75f);
            encounter_move(actor,enemy->encounterTarget,step,step);actor->speedF=step;
            if(step<10){actor->mMode=3;actor->mTimer[0]=15;}break;
        }
        case 3:
            if(!actor->mTimer[0]){animate(e_yc_class::ANM_CATCH_START,0);actor->mMode=4;}
            break;
        case 4:
            encounter_move(actor,enemy->encounterTarget+cXyz(0,350,0),30,30);
            if(actor->mpMorf->checkFrame(11)) {
                actor->mAtSph.OnAtSetBit();actor->mAtSph.SetC(enemy->encounterTarget);
                actor->mAtSph.SetR(65.f*actor->scale.x);dComIfG_Ccsp()->Set(&actor->mAtSph);
            }
            if(actor->mpMorf->isStop()) {
                actor->mAction=e_yc_class::ACT_F_FLY;actor->mMode=0;actor->mTimer[2]=150;
                animate(e_yc_class::ANM_FLY,2);
            }
            break;
        }
    }
    actor->mAcch.CrrPos(dComIfG_Bgsp());
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_head_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daB_DS_c*>(args,0);auto* enemy=encounter_owner(actor);
    if(!enemy)return HOOK_CONTINUE;
    if(!enemy->encounterReady) {
        actor->current.pos=actor->old.pos=enemy->spawnPosition;
        actor->shape_angle.x=actor->shape_angle.z=0;
        actor->mBossPhase=1;actor->mIsOpeningDemo=false;actor->mEyeColorAlpha=255;
        actor->mAction=daB_DS_c::ACT_B2_WAIT;actor->mMode=0;actor->mCreateFireBreath=false;
        actor->mDrawZant=false;actor->mNoDrawSword=false;actor->mWallR=200;actor->mGroundUpY=0;
        actor->home.pos=actor->current.pos;enemy->encounterReady=true;
    }
    if(encounter_running(*enemy)&&actor->mAction==daB_DS_c::ACT_B2_F_MOVE&&
       (actor->mMode==1||actor->mMode==2)&&actor->mModeTimer<=1&&fopAcM_searchPlayerDistance(actor)<=6000.f && !boss_attack_paused(*enemy) && boss_major_ready(*enemy,120)) {
        actor->setBck(daB_DS_c::ANM_HEAD_FATTACK_A,0,3.f,1.f);actor->mMode=3;
    }
    return HOOK_CONTINUE; // native breath telegraph, release, fireball and recovery
}
HookAction before_roster_head_move(ModContext*,void* args,void* result,void*) {
    auto* actor=mods::arg<daB_DS_c*>(args,0);if(!encounter_owner(actor))return HOOK_CONTINUE;
    auto* link=daAlink_getAlinkActorClass();
    if(link) {
        const auto delta=link->current.pos-actor->current.pos;
        cXyz target=actor->current.pos;
        if(delta.absXZ()>1800){target.x=link->current.pos.x;target.z=link->current.pos.z;}
        encounter_move(actor,target,9,0); // Native MoveF clears horizontal speed before Acch.actor->home.pos=link->current.pos;
        actor->mBh2AttackAngleF=0x2000;actor->mWallR=200;
    }
    *static_cast<bool*>(result)=true;return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_head_hover(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daB_DS_c*>(args,0);if(!encounter_owner(actor))return HOOK_CONTINUE;
    if(auto* link=daAlink_getAlinkActorClass()) {
        // Recovery after the native shot remains reachable with the basic sword.
        const float height=actor->mMode==100?80.f:320.f;
        actor->current.pos.y+=std::clamp(link->current.pos.y+height-actor->current.pos.y,-12.f,12.f);
        actor->speed.y=0;actor->gravity=0;actor->mSoundPos=actor->current.pos;
    }
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_hand_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daE_ZH_c*>(args,0);auto* enemy=encounter_owner(actor);
    if(!enemy)return HOOK_CONTINUE;
    if(!enemy->encounterReady) {
        actor->mS_Ball=nullptr;actor->mActionMode=4;actor->mMoveMode=1;
        actor->field_0x78c=0;actor->setBck(0x0d,2,3.f,1.f); // native search
        enemy->encounterReady=true;
    }
    if(!encounter_running(*enemy))return HOOK_SKIP_ORIGINAL;
    auto* link=daAlink_getAlinkActorClass();if(!link)return HOOK_SKIP_ORIGINAL;
    const unsigned phase=enemy->encounterTick++%180;
    if(phase==60) {
        enemy->encounterTarget=link->current.pos;
        actor->setBck(0x0b,0,3.f,1.f); // open hand telegraphs the locked landing point
    }
    if(phase==90)actor->setBck(0x06,0,3.f,1.f); // catch/slam
    if(phase==0)actor->setBck(0x0d,2,3.f,1.f);
    const auto target=phase<60?link->current.pos+cXyz(0,280,0):
        enemy->encounterTarget+cXyz(0,phase<90?300.f:phase<150?0.f:280.f,0);
    encounter_move(actor,target,phase<90?6.f:0.f,phase>=90&&phase<108?32.f:8.f);
    actor->mBgc.CrrPos(dComIfG_Bgsp());
    actor->mpBtkAnm->play();actor->mpModelMorf->play(0,dComIfGp_getReverb(fopAcM_GetRoomNo(actor)));
    return HOOK_SKIP_ORIGINAL;
}
void after_roster_hand_collision(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daE_ZH_c*>(args,0);auto* enemy=encounter_owner(actor);
    if(!enemy)return;
    actor->field_0xc7c.OffAtSetBit();
    const unsigned phase=(enemy->encounterTick+179)%180;
    if(encounter_running(*enemy)&&!enemy->stagger&&phase>=90&&phase<108) {
        actor->field_0xc7c.OnAtSetBit();actor->field_0xc7c.SetC(actor->current.pos);
        actor->field_0xc7c.SetR(150);actor->field_0xc7c.SetH(150);
        dComIfG_Ccsp()->Set(&actor->field_0xc7c);
    }
}

// Suppress only this owned head's creation-time dungeon writes/query. Nested
// unrelated processes continue to see the real room flags.
struct RosterCreateFrame {void* args;bool owned;};
std::vector<RosterCreateFrame> s_rosterHeadCreates;
HookAction before_roster_head_create(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daB_DS_c*>(args,0);
    s_rosterHeadCreates.push_back({args,encounter_owner(actor)!=nullptr||
        (in_arena()&&active_enemy_arrow(fopAcM_GetID(actor)))});
    return HOOK_CONTINUE;
}
void after_roster_head_create(ModContext*,void* args,void*,void*) {
    if(!s_rosterHeadCreates.empty()&&s_rosterHeadCreates.back().args==args)s_rosterHeadCreates.pop_back();
}
HookAction before_roster_head_cleared(ModContext*,void*,void* result,void*) {
    if(s_rosterHeadCreates.empty()||!s_rosterHeadCreates.back().owned||!enemy_spawner_process_active())return HOOK_CONTINUE;
    *static_cast<s32*>(result)=0;return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_head_zone(ModContext*,void*,void*,void*) {
    return !s_rosterHeadCreates.empty()&&s_rosterHeadCreates.back().owned&&enemy_spawner_process_active()?HOOK_SKIP_ORIGINAL:HOOK_CONTINUE;
}

// Stalhounds use native emergence, movement and effects even in daylight.
// Scope midnight to their Execute, restoring it for nested unrelated actors.
struct RosterTimeFrame {void* args;float daytime;};
std::vector<RosterTimeFrame> s_rosterTimes;
HookAction before_roster_time(ModContext*,void* args,void*,void*) {
    auto* process=mods::arg<void*>(args,1);bool owned=false;
    if(process&&is_actor_process(process)&&in_arena()) {
        auto* actor=static_cast<fopAc_ac_c*>(process);
        const auto* enemy=fopAcM_GetName(actor)==fpcNm_E_SH_e?tracked(fopAcM_GetID(actor)):nullptr;
        const auto* methods=reinterpret_cast<const process_method_class*>(actor->sub_method);
        owned=enemy&&enemy->profile==fpcNm_E_SH_e&&methods&&
            mods::arg<process_method_func>(args,0)==methods->execute_method;
    }
    if(owned||!s_rosterTimes.empty()) {
        auto& time=dKy_getEnvlight()->daytime;
        const float value=owned?0.f:s_rosterTimes.front().daytime;
        s_rosterTimes.push_back({args,time});time=value;
    }
    return HOOK_CONTINUE;
}
void after_roster_time(ModContext*,void* args,void*,void*) {
    if(!s_rosterTimes.empty()&&s_rosterTimes.back().args==args) {
        dKy_getEnvlight()->daytime=s_rosterTimes.back().daytime;s_rosterTimes.pop_back();
    }
}
HookAction before_roster_dd_search(ModContext*,void* args,void* result,void*) {
    auto* actor=mods::arg<fopAc_ac_c*>(args,0);
    // The full-angle calls are acquisition/leash; keep fire-cone checks native.
    if(!arena_aggro(actor,fpcNm_E_DD_e)||mods::arg<s16>(args,2)!=0x7fff)return HOOK_CONTINUE;
    *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_rdy_search(ModContext*,void* args,void* result,void*) {
    auto* actor=mods::arg<e_rdy_class*>(args,0);auto* enemy=encounter_owner(&actor->actor);
    if(!enemy)return HOOK_CONTINUE;
    if(enemy->dormant){*static_cast<int*>(result)=0;return HOOK_SKIP_ORIGINAL;}
    const float range=mods::arg<float>(args,1);
    if(!arena_aggro(&actor->actor,fpcNm_E_RDY_e)||(range!=actor->mPlayerCheckDist&&range!=actor->mPlayerCheckDist+50.f))return HOOK_CONTINUE;
    *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_yk_search(ModContext*,void* args,void* result,void*) {
    auto* actor=mods::arg<e_yk_class*>(args,0);const float range=mods::arg<float>(args,1);
    if(!arena_aggro(actor,fpcNm_E_YK_e)||(range!=actor->mPlayerTrigger&&range!=actor->mPlayerTrigger+50.f))return HOOK_CONTINUE;
    *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_s1_search(ModContext*,void* args,void* result,void*) {
    if(!arena_aggro(mods::arg<e_s1_class*>(args,0),fpcNm_E_S1_e))return HOOK_CONTINUE;
    *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
}
// Stop the whole action dispatcher: its sound/social reactions run AFTER normal
// idle and can switch to EXCITE despite pl_check returning false. Execute still
// animates and registers targets; update_extended_enemy consumes player hits
// before this hook and clears dormant, restoring native AI on the first hit.
HookAction before_roster_rdy_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<e_rdy_class*>(args,0);
    if(prepare_carrier_rider(actor))return HOOK_CONTINUE;
    auto* enemy=encounter_owner(&actor->actor);
    if(!enemy||!enemy->dormant||enemy->retiring||enemy->credited)return HOOK_CONTINUE;
    if(!enemy->idleAnimation) {
        encounter_animation(actor->mpMorf,"E_rdy",0x4c);actor->mAnm=0x4c;enemy->idleAnimation=true;
    }
    actor->mAction=0;actor->mMode=0;
    actor->actor.current.pos=actor->actor.old.pos=enemy->lootPosition;
    actor->actor.speedF=0;actor->actor.speed.set(0,0,0);
    actor->field_0xabc=actor->mKnockback=0;actor->field_0xa7b=0;
    return HOOK_SKIP_ORIGINAL;
}
