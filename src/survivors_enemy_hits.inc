// Reuse the existing process, child-create and collision hooks. No additional
// host symbols or global player invulnerability are needed.
struct EnemyHitSource {
    EnemyAttackCycle cycle;
    EnemyAttackUsePtr projectile, thrownSword, riders, magic, spin;
    unsigned throwCooldown = 0;
    bool summoning = false;
};
std::unordered_map<ActorId, EnemyHitSource> s_enemyHitSources;
bool limited_enemy_attacks(const Enemy* enemy) {
    return enemy && (enemy->elite || native_boss_profile(enemy->profile));
}
void observe_enemy_attack(Enemy& enemy, fopAc_ac_c* actor) {
    if (!limited_enemy_attacks(&enemy) || !actor) return;
    auto& source = s_enemyHitSources[enemy.id];
    int action = 0, mode = 0, key = 0;
    switch (enemy.profile) {
    case fpcNm_E_OC_e: {
        auto* e = static_cast<daE_OC_c*>(actor); action=e->mActionMode;mode=e->mOcState;
        if (action==4) key=action;
        break;
    }
    case fpcNm_E_DN_e: {
        auto* e=reinterpret_cast<e_dn_class*>(actor);action=e->action;mode=e->mode;
        if (action==4||action==5||action==6||action==8) key=action;
        break;
    }
    case fpcNm_E_MF_e: {
        auto* e=reinterpret_cast<e_mf_class*>(actor);action=e->mAction;mode=e->field_0x5b4;
        if (action==5||action==6||action==8) key=action;
        break;
    }
    case fpcNm_E_MM_e: {
        auto* e=reinterpret_cast<e_mm_class*>(actor);action=e->action;mode=e->mode;
        if (action==1) key=action;
        break;
    }
    case fpcNm_E_RD_e: {
        auto* e=reinterpret_cast<e_rd_class*>(actor);action=e->action;mode=e->mode;
        if (action==4||action==6||action==7) key=action;
        break;
    }
    case fpcNm_E_RDY_e: {
        auto* e=reinterpret_cast<e_rdy_class*>(actor);action=e->mAction;mode=e->mMode;
        if (action==4||action==6) key=action;
        break;
    }
    case fpcNm_E_KR_e: {
        auto* e=reinterpret_cast<e_kr_class*>(actor);action=e->mCurAction;mode=e->field_0x672;
        if (action==3) key=action;
        break;
    }
    case fpcNm_E_SH_e: {
        auto* e=reinterpret_cast<e_sh_class*>(actor);action=e->field_0x676;mode=e->field_0x678;
        if (action==3) key=action;
        break;
    }
    case fpcNm_E_SF_e: {
        auto* e=reinterpret_cast<e_sf_class*>(actor);action=e->mAction;mode=e->mActionPhase;
        if (action==4||action==5) key=action;
        break;
    }
    case fpcNm_E_S1_e: {
        auto* e=reinterpret_cast<e_s1_class*>(actor);action=e->mAction;mode=e->mMode;
        if (action==3) key=action;
        break;
    }
    case fpcNm_B_TN_e: {
        auto* e=static_cast<daB_TN_c*>(actor);action=e->mActionMode1;mode=e->mActionMode2;
        if (action==4||action==5||action==10||action==11) key=action;
        if (enemy.specialCooldown>source.throwCooldown) source.thrownSword=new_enemy_attack_use();
        source.throwCooldown=enemy.specialCooldown;
        break;
    }
    case fpcNm_E_RDB_e: {
        auto* e=reinterpret_cast<e_rdb_class*>(actor);action=e->mAction;mode=e->mMode;
        if (action==3||action==4) key=action;
        break;
    }
    case fpcNm_E_VT_e: {
        auto* e=reinterpret_cast<daE_VA_c*>(actor);action=e->mAction;mode=e->mMode;
        if (action==daE_VA_c::ACTION_OPACI_ATTACK_e||action==daE_VA_c::ACTION_OPACI_FLY_e) key=action;
        break;
    }
    case fpcNm_B_DS_e: {
        auto* e=reinterpret_cast<daB_DS_c*>(actor);action=e->mAction;mode=e->mMode;
        if (action==daB_DS_c::ACT_B2_F_MOVE) key=action+1;
        break;
    }
    case fpcNm_E_YMB_e: {
        auto* e=reinterpret_cast<daE_YMB_c*>(actor);action=e->mAction;mode=e->mMode;
        if (action==2) key=action;
        break;
    }
    case fpcNm_E_YC_e: {
        auto* e=reinterpret_cast<e_yc_class*>(actor);action=e->mAction;mode=e->mMode;
        if (action==e_yc_class::ACT_ATTACK) key=action+1;
        break;
    }
    case fpcNm_E_PZ_e: {
        auto* e=reinterpret_cast<daE_PZ_c*>(actor);action=e->mActionMode;mode=e->mMoveMode;
        if (action==daE_PZ_c::ACTION_ATTACK_e) key=action+1;
        break;
    }
    case fpcNm_E_TH_e: {
        auto* e=reinterpret_cast<e_th_class*>(actor);action=e->mAction;mode=e->mMode;
        // Body contacts share a spin use; the rotating ball has its own cadence.
        // One throw includes its return journey.
        if (action==1||action==2) key=1;
        if (action==3||action==4) key=2;
        break;
    }
    case fpcNm_E_HZELDA_e: {
        auto* e=reinterpret_cast<e_hzelda_class*>(actor);action=e->mAction;mode=e->mMode;
        if (action>=1&&action<=3) key=action;
        break;
    }
    case fpcNm_B_GND_e: {
        auto* e=reinterpret_cast<b_gnd_class*>(actor);action=e->mActionMode;mode=e->mMoveMode;
        if (action==11) key=1+mode/5; // Native A/B/C/E attack sequences, not animation loops.
        if (enemy.summonCooldown) source.summoning=false;
        break;
    }
    case fpcNm_E_ZH_e:
        // Portable hand: lock target, descend, impact and rise in one cycle.
        action=1;key=1+int(enemy.encounterTick/180);mode=1;break;
    case fpcNm_B_GG_e: {
        auto* e=reinterpret_cast<daB_GG_c*>(actor);action=e->mAction;mode=e->mMode;
        if(e->mSubAction==2)key=action+1;
        break;
    }
    // Other roster types can also be promoted to elites. Their native action
    // transitions/re-entry delimit attacks; continuous contact is one use.
#define HIT_STATE(P,T,A,M) case P: {auto* e=reinterpret_cast<T*>(actor);action=e->A;mode=e->M;key=action+1;break;}
    HIT_STATE(fpcNm_E_BA_e,e_ba_class,mAction,mMode)
    HIT_STATE(fpcNm_E_MS_e,e_ms_class,mAction,mMode)
    HIT_STATE(fpcNm_E_TT_e,daE_TT_c,mAction,mMode)
    HIT_STATE(fpcNm_E_GE_e,daE_GE_c,mActionMode,mMode)
    HIT_STATE(fpcNm_E_CR_e,e_cr_class,action,mode)
    HIT_STATE(fpcNm_E_BS_e,e_bs_class,action,mode)
    HIT_STATE(fpcNm_E_GI_e,daE_GI_c,mActionMode,mMoveMode)
    HIT_STATE(fpcNm_E_FS_e,e_fs_class,mAction,mMode)
    HIT_STATE(fpcNm_E_BU_e,e_bu_class,action,mode)
    HIT_STATE(fpcNm_E_SM2_e,e_sm2_class,action,mode)
    HIT_STATE(fpcNm_E_DD_e,e_dd_class,mAction,field_0x68c)
    HIT_STATE(fpcNm_E_YK_e,e_yk_class,mAction,mActionPhase)
#undef HIT_STATE
    default:break;
    }
    source.cycle.observe(key,action,mode);
    if(enemy.profile==fpcNm_E_VT_e&&action==daE_VA_c::ACTION_OPACI_FLY_e)
        source.magic=source.cycle.current();
}
EnemyAttackUsePtr enemy_attack_use(cCcD_Obj* attack) {
    auto* actor=attack?attack->GetAc():nullptr;if(!actor)return {};
    const auto id=fopAcM_GetID(actor);
    auto child=s_enemyHitSources.find(id);
    if(child!=s_enemyHitSources.end()&&child->second.projectile)return child->second.projectile;
    auto* owner=enemy_child_root(id);
    if(!limited_enemy_attacks(owner))return {};
    auto* root=id==owner->id?actor:actor_by_id(owner->id);
    if(!root)return {};
    observe_enemy_attack(*owner,root);
    auto& source=s_enemyHitSources[owner->id];
    if(owner->profile==fpcNm_E_HZELDA_e) {
        if(auto use=zelda_light_orb_use(attack))return use;
        auto* zelda=reinterpret_cast<e_hzelda_class*>(root);
        if(attack==&zelda->mBallSphAt&&source.magic)return source.magic;
    }
    bool repeatingSpin=false;
    if(owner->profile==fpcNm_E_RDB_e) {
        auto* king=reinterpret_cast<e_rdb_class*>(root);
        repeatingSpin=king->mAction==4 && king->mMode==2 && attack==&king->field_0xe64;
    } else if(owner->profile==fpcNm_E_TH_e && id!=owner->id &&
              fopAcM_GetName(actor)==fpcNm_E_TH_BALL_e) {
        auto* ball=reinterpret_cast<e_th_ball_class*>(actor);
        repeatingSpin=ball->mAction==1 && ball->mMode>=1 && ball->mMode<=3 && attack==&ball->mAtSph;
    }
    // One accepted spin contact per half-second of playable run time, per
    // owner and shared across Link's colliders. Re-entry cannot reset the clock.
    if(repeatingSpin) {
        if(!source.spin || (source.spin->contacted &&
           s_run.ticks-source.spin->contactTick>=ticks_per_second/2))
            source.spin=new_enemy_attack_use();
        return source.spin;
    }
    if(owner->profile==fpcNm_E_VT_e) {
        auto* sword=reinterpret_cast<daE_VA_c*>(root);
        if(attack==&sword->mMagicSphs[0]||attack==&sword->mMagicSphs[1]) {
            if(!source.magic)source.magic=new_enemy_attack_use();
            return source.magic;
        }
    }
    if(owner->profile==fpcNm_B_TN_e&&attack==&owner->extraAttack) {
        if(!source.thrownSword)source.thrownSword=new_enemy_attack_use();
        return source.thrownSword;
    }
    return source.cycle.current();
}
void after_enemy_attack_spawn(ModContext*,void* args,void* result,void*) {
    const auto id=*static_cast<ActorId*>(result);
    if(id==kNone||!active_enemy_arrow(id))return;
    const auto parent=mods::arg<ActorId>(args,1);
    auto* owner=enemy_child_root(parent);if(!limited_enemy_attacks(owner))return;
    const auto profile=mods::arg<s16>(args,0);
    if(structural_enemy_child(profile))return; // Shell/ball follow the owner's current attack.
    if(auto* actor=actor_by_id(owner->id))observe_enemy_attack(*owner,actor);
    EnemyAttackUsePtr use;
    if(profile==fpcNm_E_ARROW_e)use=new_enemy_attack_use(); // Each native bow shot is a new attack.
    else if(profile==fpcNm_E_FK_e) {
        auto& source=s_enemyHitSources[owner->id];
        if(!source.summoning){source.riders=new_enemy_attack_use();source.summoning=true;}
        use=source.riders;
    } else {
        const auto inherited=s_enemyHitSources.find(parent);
        use=inherited!=s_enemyHitSources.end()&&inherited->second.projectile?
            inherited->second.projectile:s_enemyHitSources[owner->id].cycle.current();
    }
    s_enemyHitSources[id].projectile=std::move(use);
}
// Observe wind-up/idle boundaries even when no attack collider is enabled.
// Children retain shared cast ownership across a later cast or owner deletion.
HookAction before_enemy_attack_process(ModContext*,void* args,void*,void*) {
    auto* process=mods::arg<void*>(args,1);
    if(!process||!is_actor_process(process))return HOOK_CONTINUE;
    auto* actor=static_cast<fopAc_ac_c*>(process);
    const auto* methods=reinterpret_cast<const process_method_class*>(actor->sub_method);
    if(!methods)return HOOK_CONTINUE;
    const auto method=mods::arg<process_method_func>(args,0);
    const auto id=fopAcM_GetID(actor);
    if(method==methods->delete_method)s_enemyHitSources.erase(id);
    else if(method==methods->execute_method&&in_arena()&&s_run.phase==Phase::Running) {
        if(auto* enemy=tracked(id);enemy&&enemy->initialized&&!enemy->retiring&&!enemy->credited&&playable())
            observe_enemy_attack(*enemy,actor);
    }
    return HOOK_CONTINUE;
}
struct EnemyContactFrame {void* args;cCcD_Obj* attack;cCcD_Obj* target;EnemyAttackUsePtr use;};
std::vector<EnemyContactFrame> s_enemyContactFrames;
HookAction before_enemy_attack_contact(ModContext*,void* args,void*,void*) {
    auto* attack=mods::arg<cCcD_Obj*>(args,1);
    auto* target=mods::arg<cCcD_Obj*>(args,2);
    auto* link=daAlink_getAlinkActorClass();
    if(!attack||!target||!link||target->GetAc()!=link||fopAcM_GetID(link)!=s_player||
       !in_arena()||s_run.phase!=Phase::Running||!playable()||!attack->GetAtAtp())return HOOK_CONTINUE;
    auto use=enemy_attack_use(attack);if(!use)return HOOK_CONTINUE;
    if(use->contacted)return HOOK_SKIP_ORIGINAL;
    s_enemyContactFrames.push_back({args,attack,target,std::move(use)});
    return HOOK_CONTINUE;
}
void after_enemy_attack_contact(ModContext*,void* args,void*,void*) {
    if(s_enemyContactFrames.empty()||s_enemyContactFrames.back().args!=args)return;
    auto frame=std::move(s_enemyContactFrames.back());s_enemyContactFrames.pop_back();
    // Commit only a native accepted contact. The first block/dodge remains
    // effective for the whole attack instead of rerolling on every frame.
    if(frame.target->ChkTgHit()&&frame.target->GetTgHitObj()==frame.attack) {
        frame.use->contacted=true;
        frame.use->contactTick=s_run.ticks;
        boss_record_contact(frame.attack);
    }
}
