// Native attacks stay in the original actors. These adapters supply the arena
// anchors/entry state that the dungeon scripts would normally initialize.
bool portable_boss(ProfileName p) {
    switch(p) {
    case fpcNm_E_RDB_e:case fpcNm_E_VT_e:case fpcNm_E_YMB_e:case fpcNm_E_PZ_e:
    case fpcNm_E_TH_e:case fpcNm_E_HZELDA_e:case fpcNm_B_GND_e:case fpcNm_B_DS_e:return true;
    default:return false;
    }
}
struct BossScope {void* args;ActorId id;bool owned;int skip,weather,pat0,pat1;};
std::vector<BossScope> s_bossScopes;
bool boss_script_scope() {return !s_bossScopes.empty()&&s_bossScopes.back().owned;}
HookAction before_boss_scope(ModContext*,void* args,void*,void*) {
    auto* process=mods::arg<void*>(args,1);ActorId id=kNone;bool owned=false;
    if(process&&is_actor_process(process)&&in_arena()) {
        auto* actor=static_cast<fopAc_ac_c*>(process);id=fopAcM_GetID(actor);
        owned=portable_boss(fopAcM_GetName(actor))&&(tracked(id)||active_enemy_arrow(id));
    }
    if(owned||!s_bossScopes.empty()) {
        s_bossScopes.push_back({args,id,owned,cDmr_SkipInfo,g_env_light.mColpatWeather,g_env_light.wether_pat0,g_env_light.wether_pat1});
        if(owned)cDmr_SkipInfo=0;
    }
    return HOOK_CONTINUE;
}
void after_boss_scope(ModContext*,void* args,void*,void*) {
    if(s_bossScopes.empty()||s_bossScopes.back().args!=args)return;
    const auto frame=s_bossScopes.back();s_bossScopes.pop_back();
    if(frame.owned) {
        cDmr_SkipInfo=frame.skip;g_env_light.mColpatWeather=frame.weather;
        g_env_light.wether_pat0=frame.pat0;g_env_light.wether_pat1=frame.pat1;
    }
}
HookAction before_boss_story_write(ModContext*,void*,void*,void*) {
    return boss_script_scope()?HOOK_SKIP_ORIGINAL:HOOK_CONTINUE;
}
HookAction before_boss_camera(ModContext*,void* args,void*,void*) {
    return encounter_owner(mods::arg<fopAc_ac_c*>(args,0))?HOOK_SKIP_ORIGINAL:HOOK_CONTINUE;
}
HookAction before_boss_rdb(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<e_rdb_class*>(args,0);auto* owner=encounter_owner(&e->enemy);
    if(!owner)return HOOK_CONTINUE;
    if(!owner->encounterReady) {
        e->enemy.current.pos=e->enemy.old.pos=owner->spawnPosition;
        e->mAction=1;e->mMode=0;e->mDemoMode=0;owner->encounterReady=true;
    }
    if(encounter_running(*owner)&&!owner->specialCooldown&&e->mAction==1&&fopAcM_searchPlayerDistance(&e->enemy)<900.f) {
        e->mAction=4;e->mMode=0;owner->specialCooldown=180; // six seconds, safe attack boundary
    }
    return HOOK_CONTINUE; // native chase, axe combos, spin and defence
}
HookAction before_boss_th(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<e_th_class*>(args,0);auto* owner=encounter_owner(e);
    if(!owner)return HOOK_CONTINUE;
    if(!owner->encounterReady) {
        e->current.pos=e->old.pos=owner->spawnPosition;e->mAction=0;e->mMode=0;
        e->mNoDraw=false;e->mDemoCamMode=0;owner->encounterReady=true;
    }
    // The ball is asynchronous; RETURN dereferences it without a null check.
    if(!actor_by_id(e->mBallID)) {
        if(!fpcM_IsCreating(e->mBallID)&&owner->age%30==0)
            e->mBallID=fopAcM_createChild(fpcNm_E_TH_BALL_e,owner->id,0,&e->current.pos,fopAcM_GetRoomNo(e),&e->shape_angle,nullptr,-1,nullptr);
        return HOOK_SKIP_ORIGINAL;
    }
    if(e->mAction==2&&e->mMode==3&&e->field_0x5dc>45&&e->mPlayerDist>200) {
        e->mMode=4;e->mTimers[0]=18; // no corridor-only flank requirement for a throw
    }
    return HOOK_CONTINUE; // native ball spin, throw and retrieval
}
HookAction before_boss_gnd_execute(ModContext*,void* args,void* result,void*) {
    auto* e=mods::arg<b_gnd_class*>(args,0);auto* owner=encounter_owner(e);
    if(!owner)return HOOK_CONTINUE;
    e->mNoDrawTimer=0; // never run the horse-to-duel story transition
    auto* cloak=actor_by_id(e->mMantChildID);
    if(!cloak||!fopAcM_IsExecuting(e->mMantChildID)) {
        if(!cloak&&!fpcM_IsCreating(e->mMantChildID)&&owner->age%30==0)
            e->mMantChildID=fopAcM_createChild(fpcNm_MANT_e,owner->id,0,&e->current.pos,fopAcM_GetRoomNo(e),nullptr,nullptr,-1,nullptr);
        *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
    }
    if(!owner->encounterReady) {
        e->current.pos=e->old.pos=owner->spawnPosition;
        e->speed.zero();e->speedF=0;e->mActionMode=10;e->mMoveMode=0;
        e->mDrawHorse=false;e->mDemoCamMode=0;e->mGakeChkType=0;
        e->field_0x1e08=0;e->field_0x1e0a=0;e->field_0xc44[8]=100;
        e->field_0x2710.x=55;e->field_0xc7d=1;e->field_0x2740=0;
        reinterpret_cast<mant_class*>(cloak)->field_0x3969=1;
        owner->encounterReady=true;
    }
    // Sword-lock is an interactive cinematic; return to his native sword AI.
    if(e->mActionMode==19){e->mActionMode=10;e->mMoveMode=0;}
    e->mDemoCamMode=0;
    if(encounter_running(*owner)) {
        if(!owner->specialCooldown&&e->mActionMode==10&&fopAcM_searchPlayerDistance(e)>550.f) {
            e->mActionMode=11;e->mMoveMode=10; // Charge hook starts the shared cooldown.
        }
        if(!owner->summonCooldown && e->mActionMode==10 && !boss_attack_paused(*owner) && boss_major_ready(*owner,120)) {
            summon_ghost_riders(e,*owner);owner->summonCooldown=240;
        }
    }
    return HOOK_CONTINUE;
}
HookAction before_boss_va(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<daE_VA_c*>(args,0);auto* owner=encounter_owner(e);
    if(!owner)return HOOK_CONTINUE;
    e->mAlphaType=2;e->mDrawRopes=false;e->mRopesEnabled=false;e->mFadeAwayTimer=30000;
    e->mGlowLightA=e->mEyeAlpha=e->mFootSmokeAlpha=255;
    e->attention_info.flags=fopAc_AttnFlag_BATTLE_e;
    if(!owner->encounterReady) {
        e->mWeponEfMode=3;e->setActionMode(daE_VA_c::ACTION_OPACI_CHASE_e,12);
        e->setBck(30,2,3.f,1.f);e->gravity=-5.f;owner->encounterReady=true;
    }
    // A distant player must not prevent the magic phase indefinitely. Only
    // transition out of chase; an active sword/magic animation is never reset.
    if(encounter_running(*owner)&&e->mAction==daE_VA_c::ACTION_OPACI_CHASE_e&&e->mMode==12&&
       ++owner->encounterTick>=75&&fopAcM_searchPlayerDistance(e)>600.f)
        e->setActionMode(daE_VA_c::ACTION_OPACI_FLY_e,0);
    // Fly modes 0..4 and chase modes 0..11 orbit the old world origin.
    // Keep native chase/strike and the complete two-projectile attack instead.
    if(e->mAction==daE_VA_c::ACTION_OPACI_FLY_e) {
        if(e->mMode==0) {e->mMode=10;e->speedF=0;e->speed.y=0;e->gravity=0;e->setBck(12,2,3.f,1.f);owner->encounterTick=0;}
        e->shape_angle.y=e->current.angle.y=fopAcM_searchPlayerAngleY(e);
        if(e->mMode<10) {e->setActionMode(daE_VA_c::ACTION_OPACI_CHASE_e,12);e->gravity=-5;e->setBck(30,2,3.f,1.f);}
    }
    return HOOK_CONTINUE;
}
HookAction before_boss_ymb(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<daE_YMB_c*>(args,0);auto* owner=encounter_owner(e);
    if(!owner)return HOOK_CONTINUE;
    if(!owner->encounterReady) {e->setActionMode(1,0);owner->encounterReady=true;}
    e->field_0x6d8=255;e->field_0x6dc=150;e->field_0x710=0;e->field_0x717=1;e->field_0x764=1;
    if(auto* link=daAlink_getAlinkActorClass()) {
        e->field_0x69c=link->current.pos+cXyz(0,450,0);
        // Native electric charge uses floor+550 as its target; adapt the lake
        // plane so the charge crosses human Link's height on every map.
        e->field_0x6cc=link->current.pos.y-500.f;
        e->field_0x6d0=link->current.pos.y-450.f;
        e->field_0x6d4=link->current.pos.y;
    }
    if(e->mAction==0||e->mAction>=5)e->setActionMode(1,0);
    e->field_0x70c=0;
    if(e->mAction==1&&encounter_running(*owner)&&!boss_attack_paused(*owner)&&boss_major_ready(*owner,170))e->setActionMode(2,0);
    return HOOK_CONTINUE; // retain the native telegraph, charge and recovery
}
HookAction before_boss_hzelda(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<e_hzelda_class*>(args,0);auto* owner=encounter_owner(e);
    if(!owner)return HOOK_CONTINUE;
    if(!owner->encounterReady) {e->mMode=0;e->mAction=0;owner->encounterReady=true;}
    e->mDemoMode=0;e->mMsgSetTimer=0;e->mMissedBallCount=0;
    // Native Execute already decrements this attack/readiness clock once.
    // A second decrement halves wait/charge/recovery without skipping animation cues.
    if(encounter_running(*owner) && !owner->stagger && e->mTimers[0]>0)--e->mTimers[0];
    return HOOK_CONTINUE;
}
void after_boss_hzelda(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<e_hzelda_class*>(args,0);auto* owner=encounter_owner(e);
    if(!owner || !encounter_running(*owner) || owner->stagger || !e->mpModelMorf)return;
    // Native attack animations are all authored at 1x. Set an absolute rate
    // after action() chooses the animation, before Execute advances its frames.
    e->mpModelMorf->setPlaySpeed(e->mAction>=1 && e->mAction<=3?2.f:1.f);
}
void after_boss_hzelda_triangle(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<e_hzelda_class*>(args,0);
    if(!encounter_owner(e)||!e->mDrawTriangleAt)return;
    cXyz floor=e->mTrianglePos;floor.y=e->current.pos.y;
    if(dry_arena_floor(floor,500,2000))e->mTrianglePos.y=floor.y+5.f;
}
HookAction before_boss_pz(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<daE_PZ_c*>(args,0);auto* owner=encounter_owner(e);
    if(!owner)return HOOK_CONTINUE;
    if(!owner->encounterReady) {
        e->field_0x7d3=0;e->field_0x844=1;e->gravity=-3;
        e->attention_info.distances[fopAc_attn_BATTLE_e]=3;
        e->attention_info.flags=fopAc_AttnFlag_BATTLE_e;
        e->setActionMode(daE_PZ_c::ACTION_WAIT_e,0);owner->encounterReady=true;
    }
    if(e->mActionMode==daE_PZ_c::ACTION_ATTACK_e&&e->mMoveMode==0&&!boss_major_ready(*owner,100)) {
        e->setActionMode(daE_PZ_c::ACTION_WAIT_e,0);s_bossTactics[owner->id].recovery=15;
    }
    if(e->mActionMode==daE_PZ_c::ACTION_WAIT_e&&e->mMoveMode==0&&e->field_0x7d3==0) {
        // Native teleport animation/selection, supplied with reachable map points.
        e->mPointNum=1;e->mPointPos[0]=e->current.pos;e->field_0x780[0]=1;
        if(auto* link=daAlink_getAlinkActorClass())for(unsigned i=0;i<8;++i) {
            const s16 angle=s16(i*8192);cXyz pos=link->current.pos+cXyz(cM_ssin(angle)*900,0,cM_scos(angle)*900);
            if(elite_floor_position(pos)&&walkable_spawn_path(link->current.pos,pos,100)) {
                e->mPointPos[e->mPointNum]=pos;e->field_0x780[e->mPointNum++]=1;
            }
        }
        e->mCurrentPoint=0;e->field_0x846=1;
    }
    return HOOK_CONTINUE;
}
void after_boss_pz_attack(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<daE_PZ_c*>(args,0);auto* owner=encounter_owner(e);
    if(!owner)return;
    // Keep native portals and volleys. Summon the two native minion types
    // supported by this map's roster, avoiding dungeon-only Baba seed scripts.
    if(e->mMoveMode==1) {
        auto& t=s_bossTactics[owner->id];
        if(t.pzStage!=1) {
            const bool summon=t.pattern++%3==2;
            e->setBck(summon?0xB:0xA,0,3.f,1.f);
            t.summons=0;t.pzStage=1;
        }
    } else if(e->mMoveMode==2) {
        auto& t=s_bossTactics[owner->id];
        if(t.pzStage!=2) {
            if((t.pattern-1)%3==2)e->field_0x7d8=((t.pattern/3)%2)?0:3;
            else e->field_0x7d8=5;
            t.pzStage=2;
        }
    } else s_bossTactics[owner->id].pzStage=0;
    e->current.angle.y=fopAcM_searchPlayerAngleY(e);
}

Enemy* phantom_child_owner(ActorId id) {
    const auto* child=tracked_enemy_arrow(id);
    auto* owner=child&&!child->retiring?tracked(child->parent):nullptr;
    return owner&&owner->profile==fpcNm_E_PZ_e?owner:nullptr;
}
HookAction before_boss_summon(ModContext*,void* args,void* result,void*) {
    auto* owner=phantom_child_owner(mods::arg<ActorId>(args,1));
    const auto profile=mods::arg<s16>(args,0);
    if(!owner||(profile!=fpcNm_E_YK_e&&profile!=fpcNm_E_S1_e))return HOOK_CONTINUE;
    *static_cast<ActorId*>(result)=kNone;
    if(owner->retiring||owner->credited||s_run.phase!=Phase::Running||!boss_pz_minion(*owner,profile))return HOOK_SKIP_ORIGINAL;
    auto slot=std::find_if(s_enemies.begin(),s_enemies.end(),[](const auto& e){return e.id==kNone;});
    if(slot==s_enemies.end())return HOOK_SKIP_ORIGINAL;
    const auto type=profile==fpcNm_E_YK_e?Species::ShadowKeese:Species::ShadowBeast;
    cXyz pos=*mods::arg<const cXyz*>(args,3);
    if(!dry_arena_floor(pos,500,2000))return HOOK_SKIP_ORIGINAL;
    if(type==Species::ShadowKeese)pos.y+=180;
    if(create_enemy_at(pos,0,unsigned(type),false)){*static_cast<ActorId*>(result)=slot->id;s_bossTactics[slot->id].summoner=owner->id;++s_bossTactics[owner->id].summons;}
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_boss_pz_bullet(ModContext*,void* args,void* result,void*) {
    if(!boss_script_scope())return HOOK_CONTINUE;
    auto* owner=phantom_child_owner(s_bossScopes.back().id);
    if(!owner||mods::arg<s16>(args,0)!=fpcNm_E_PZ_e||(mods::arg<u32>(args,1)&0xff)!=40)return HOOK_CONTINUE;
    const auto* sourceAngle=mods::arg<const csXyz*>(args,4);
    csXyz angle=sourceAngle?*sourceAngle:csXyz{};
    if(auto* portal=actor_by_id(s_bossScopes.back().id)) {
        const s16 offset=s16(angle.y-fopAcM_searchPlayerAngleY(portal));
        const bool narrow=s_bossTactics[owner->id].pattern%3==1;
        angle.y=boss_target_yaw(portal,boss_lead_target(12.f))+s16(narrow?offset/2:offset);
    }
    *static_cast<ActorId*>(result)=fopAcM_createChild(fpcNm_E_PZ_e,owner->id,mods::arg<u32>(args,1),
        mods::arg<const cXyz*>(args,2),mods::arg<int>(args,3),&angle,
        mods::arg<const cXyz*>(args,5),mods::arg<s8>(args,6),nullptr);
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_boss_tn_damage(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<daB_TN_c*>(args,0);auto* owner=encounter_owner(e);
    if(!owner)return HOOK_CONTINUE;
    if(owner->initialized&&e->health>0&&e->field_0x560>0&&e->mActionMode1<daB_TN_c::ACT_CHANGEDEMO) {
        const int part=std::clamp((e->field_0x560-e->health)*24/e->field_0x560,0,12);
        if(part>e->mNextBreakPart) {e->setBreakPart(part);e->mNextBreakPart=part;}
        if(part==12)e->setActionMode(daB_TN_c::ACT_CHANGEDEMO,0);
    }
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_boss_tn_change(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<daB_TN_c*>(args,0);if(!encounter_owner(e))return HOOK_CONTINUE;
    e->executeZakoChangeDemo();return HOOK_SKIP_ORIGINAL;
}
HookAction before_boss_tn_area(ModContext*,void* args,void* result,void*) {
    if(!encounter_owner(mods::arg<fopAc_ac_c*>(args,0)))return HOOK_CONTINUE;
    *static_cast<int*>(result)=0;return HOOK_SKIP_ORIGINAL;
}
HookAction before_boss_th_spin(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<e_th_class*>(args,0);if(auto* owner=encounter_owner(e))owner->encounterTarget=e->current.pos;
    return HOOK_CONTINUE;
}
void after_boss_th_spin(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<e_th_class*>(args,0);auto* owner=encounter_owner(e);
    if(!owner)return;
    // Native movement only follows the mansion corridor's Z axis. Rotate that
    // same step toward Link while retaining the native spin/throw timing.
    const float step=(e->current.pos-owner->encounterTarget).absXZ()*1.5f;
    e->current.pos.x=owner->encounterTarget.x+cM_ssin(e->mPlayerAngleY)*step;
    e->current.pos.z=owner->encounterTarget.z+cM_scos(e->mPlayerAngleY)*step;
}
