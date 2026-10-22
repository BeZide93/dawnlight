// New roster species share Survivors HP and hit processing. Their native AI,
// attack colliders, models and movement remain active, but puzzle-specific HP
// resets, armor checks, resurrection and splitting cannot override run damage.
bool roster_owned(fopAc_ac_c* actor) {
    const auto* enemy=actor?tracked(fopAcM_GetID(actor)):nullptr;
    return in_arena() && enemy && extended_enemy(enemy->profile);
}
HookAction before_roster_native_damage(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<fopAc_ac_c*>(args,0);
    if(!roster_owned(actor))return HOOK_CONTINUE;
    const auto* enemy=tracked(fopAcM_GetID(actor));
    if((enemy->profile==fpcNm_E_DN_e||enemy->profile==fpcNm_E_SF_e) && enemy->elite && enemy->initialized &&
       !enemy->retiring && !enemy->credited && !enemy->dormant && !enemy->stagger &&
       actor->health>0 && s_run.phase==Phase::Running && !transitioning()) {
        auto* link=daAlink_getAlinkActorClass();
        // Native action() refreshes its player distance before damage_check(),
        // then uses it for the fight-run spacing and attack selection. Express
        // that decision in model units so the doubled body cannot push Link
        // away before reaching the unscaled combat thresholds. Recompute
        // from world positions, never divide a previous normalized value.
        if(link && actor->scale.x>1.f) {
            const float distance=(link->current.pos-actor->current.pos).absXZ()/actor->scale.x;
            if(enemy->profile==fpcNm_E_DN_e) {
                auto* lizalfos=reinterpret_cast<e_dn_class*>(actor);
                if(lizalfos->action==3)lizalfos->pl_dir=distance;
            } else {
                auto* stalfos=reinterpret_cast<e_sf_class*>(actor);
                if(stalfos->mAction==3)stalfos->mPlayerDistanceXZ=distance;
            }
        }
    }
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_cry(ModContext*,void* args,void* result,void*) {
    auto* actor=mods::arg<daE_GI_c*>(args,0);
    if(!roster_owned(actor))return HOOK_CONTINUE;
    auto* enemy=tracked(fopAcM_GetID(actor));
    *static_cast<bool*>(result)=false;
    if(!enemy->initialized||enemy->retiring||enemy->credited||enemy->dormant||enemy->stagger||
       actor->health<=0||s_run.phase!=Phase::Running||transitioning()||s_run.gibdo_scream_cooldown)
        return HOOK_SKIP_ORIGINAL;
    // Keep native range/LOS checks, animation, button-mashing and Link's scream
    // pose. Avoid the native global/camera owner: run deaths bypass its cleanup.
    actor->mPlayerStunTimer=30;actor->mCryTimer=80;actor->mPushButtonCount=0;
    actor->speedF=0;actor->setBck(10,0,5.f,1.f);actor->field_0x6a0=3;
    s_run.gibdo_scream_cooldown=180;
    *static_cast<bool*>(result)=true;return HOOK_SKIP_ORIGINAL;
}
void allow_roster_target(dCcD_GObjInf* target) {
    target->OnTgSetBit();target->SetTgType(0xffffffff);target->OffTgShield();target->OffTgShieldFrontRange();
}
void finish_extended_targets(const Enemy* enemy,fopAc_ac_c* actor) {
    if(!enemy||!actor||!enemy->initialized||enemy->retiring||enemy->credited||
       actor->health<=0||!extended_enemy(enemy->profile))return;
    // Dodongo and some armor paths change shield flags after registering CC.
    for(auto* target:extended_body(actor,enemy->profile).targets)if(target)allow_roster_target(target);
    if(enemy->profile==fpcNm_E_MM_e) {
        auto* helmasaur=reinterpret_cast<e_mm_class*>(actor);
        // Native Execute omits this body collider while it believes armor is
        // attached. Keep HP reachable even while its child is still loading.
        if(helmasaur->field_0x672&&!actor_by_id(helmasaur->armorChildId)) {
            auto center=actor->current.pos;center.y+=40.f*actor->scale.x;
            helmasaur->ccSph.SetC(center);helmasaur->ccSph.SetR(40.f*actor->scale.x);
            helmasaur->ccSph.OffAtSetBit();dComIfG_Ccsp()->Set(&helmasaur->ccSph);
        }
    }
}
HookAction before_roster_targets(ModContext*,void* args,void*,void*) {
    auto* object=mods::arg<cCcD_Obj*>(args,1);if(!object||!in_arena())return HOOK_CONTINUE;
    auto* actor=object->GetAc();if(!actor)return HOOK_CONTINUE;
    // Route shell contacts to the owner's HP; retain the native shell model.
    if(fopAcM_GetName(actor)==fpcNm_E_MM_MT_e && tracked(actor->parentActorID)) {
        actor=actor_by_id(actor->parentActorID);if(!actor)return HOOK_CONTINUE;
    }
    auto* enemy=tracked(fopAcM_GetID(actor));
    if(!enemy||!extended_enemy(enemy->profile)||!enemy->initialized)return HOOK_CONTINUE;
    const auto body=extended_body(actor,enemy->profile);
    for(auto* target:body.targets) if(target==object) {
        if(enemy->credited||enemy->retiring||actor->health<=0) {target->OffTgSetBit();target->OffAtSetBit();break;}
        allow_roster_target(target);
        // Some native bosses hide body spheres far outside the map during
        // puzzle invulnerability. Keep their real animated body as the target.
        if(enemy->profile==fpcNm_E_TH_e||enemy->profile==fpcNm_B_GND_e||enemy->profile==fpcNm_E_HZELDA_e) {
            const unsigned index=unsigned(std::find(body.targets.begin(),body.targets.end(),target)-body.targets.begin());
            constexpr unsigned ordinaryJoints[]{4,2,1};
            constexpr unsigned hammerJoints[]{4,1,2};
            constexpr unsigned zeldaJoints[]{4,32,37};
            const auto joint=enemy->profile==fpcNm_E_TH_e?hammerJoints[index]:
                enemy->profile==fpcNm_E_HZELDA_e?zeldaJoints[index]:ordinaryJoints[index];
            const auto matrix=body.morph->getModel()->getAnmMtx(joint);
            static_cast<dCcD_Sph*>(target)->SetC(cXyz(matrix[0][3],matrix[1][3],matrix[2][3]));
        }
        // Absolute radii: these native paths omit actor scale, some only set R
        // at creation. Never multiply the previous frame's radius.
        const float scale=enemy->elite?elite_scale(Species(enemy->species)):1.f;
        if(enemy->profile==fpcNm_E_TT_e) {
            auto* e=reinterpret_cast<daE_TT_c*>(actor);e->mSphere.SetR((e->mAction==6?80.f:55.f)*scale);
        } else if(enemy->profile==fpcNm_E_GI_e) static_cast<dCcD_Sph*>(target)->SetR(55.f*scale);
        else if(enemy->profile==fpcNm_E_BU_e) static_cast<dCcD_Sph*>(target)->SetR(45.f*scale);
        else if(enemy->profile==fpcNm_E_KR_e) {
            auto* e=reinterpret_cast<e_kr_class*>(actor);
            e->mSphere2.SetR(62.5f*scale);e->mSphere3.SetR(65.f*scale);
        } else if(enemy->profile==fpcNm_E_FS_e) {
            auto* e=reinterpret_cast<e_fs_class*>(actor);e->mCcCyl.SetR(80.f*scale);e->mCcCyl.SetH(230.f*scale);
        } else if(enemy->profile==fpcNm_E_DN_e) {
            auto* e=reinterpret_cast<e_dn_class*>(actor);
            e->cc_sph[0].SetR(50.f*scale);e->cc_sph[1].SetR(45.f*scale);e->cc_sph[2].SetR(45.f*scale);
        } else if(enemy->profile==fpcNm_E_DD_e) {
            auto* e=reinterpret_cast<e_dd_class*>(actor);
            e->field_0xa74[0].SetR(60.f*scale);e->field_0xa74[1].SetR(75.f*scale);e->field_0xa74[2].SetR(50.f*scale);
        } else if(enemy->profile==fpcNm_E_SH_e) {
            auto* e=reinterpret_cast<e_sh_class*>(actor);e->mSphArr[0].SetR(30.f*scale);e->mSphArr[1].SetR(40.f*scale);
        } else if(enemy->profile==fpcNm_E_RDY_e) {
            auto* e=reinterpret_cast<e_rdy_class*>(actor);for(auto& sphere:e->mCcSph)sphere.SetR(35.f*scale);
        } else if(enemy->profile==fpcNm_E_SF_e) {
            // Native Stalfos uses a 1.2 base size; joint centers already scale.
            auto* e=reinterpret_cast<e_sf_class*>(actor);
            e->mCcSphs[0].SetR(36.f*scale);
            e->mCcSphs[1].SetR(42.f*scale);e->mCcSphs[2].SetR(42.f*scale);
            e->mTateSph.SetR(60.f*scale);
        } else if(enemy->profile==fpcNm_E_S1_e) {
            auto* e=reinterpret_cast<e_s1_class*>(actor);
            e->mCcSph[0].SetR(35.f*scale);e->mCcSph[1].SetR(60.f*scale);e->mCcSph[2].SetR(50.f*scale);
        } else if(enemy->profile==fpcNm_E_YC_e) reinterpret_cast<e_yc_class*>(actor)->mCcSph.SetR(80.f*scale);
        break;
    }
    if(enemy->elite) {
        const float scale=elite_scale(Species(enemy->species));
        if(enemy->profile==fpcNm_E_SH_e) {
            auto* e=reinterpret_cast<e_sh_class*>(actor);if(object==&e->mSph2)e->mSph2.SetR(35.f*scale);
        } else if(enemy->profile==fpcNm_E_SF_e) {
            auto* e=reinterpret_cast<e_sf_class*>(actor);if(object==&e->mAtSph)e->mAtSph.SetR(60.f*scale);
        } else if(enemy->profile==fpcNm_E_S1_e) {
            auto* e=reinterpret_cast<e_s1_class*>(actor);if(object==&e->mCcAtSph)e->mCcAtSph.SetR(40.f*scale);
        } else if(enemy->profile==fpcNm_E_RDY_e) {
            auto* e=reinterpret_cast<e_rdy_class*>(actor);if(object==&e->mAtSph && e->mAtSph.GetR()>0)e->mAtSph.SetR(50.f*scale);
        }
    }
    // Frozen heavy-hit stagger must not leave a damaging attack in the scene.
    if(enemy->stagger)object->OffAtSetBit();
    return HOOK_CONTINUE;
}
bool roster_attack(const cCcD_Obj* attack) {
    if(!attack)return false;
    auto* link=daAlink_getAlinkActorClass();
    if(native_sword_collider(attack)||effect_collider(attack)||(link&&attack==&link->mGuardAtCps))return true;
    for(const auto& a:s_attacks)if(attack==&a.collider)return true;
    return false;
}
// The shell may execute before its parent. Preserve one contact for the shared
// parent damage pass and clear native contacts before armor/puzzle code sees them.
HookAction before_roster_shell(ModContext*,void* args,void*,void*) {
    auto* shell=mods::arg<e_mm_mt_class*>(args,0);
    auto* actor=reinterpret_cast<fopAc_ac_c*>(shell);
    auto* parent=actor_by_id(actor->parentActorID);
    if(!roster_owned(parent))return HOOK_CONTINUE;
    auto* enemy=tracked(actor->parentActorID);
    if(enemy->profile!=fpcNm_E_MM_e||enemy->retiring||enemy->credited||parent->health<=0)return HOOK_CONTINUE;
    auto* helmasaur=reinterpret_cast<e_mm_class*>(parent);
    helmasaur->field_0x672=1;helmasaur->field_0xb99&=~4;
    shell->m_action=0;shell->m_mode=0;
    for(auto* target:{&shell->m_sphere,&shell->m_sph}) {
        if(target->ChkTgHit() && !enemy->childHit && roster_attack(target->GetTgHitObj()))
            enemy->childHit=target->GetTgHitObj();
        target->ResetTgHit();
    }
    return HOOK_CONTINUE;
}
HookAction before_roster_search(ModContext*,void* args,void* result,void*) {
    auto* actor=mods::arg<fopAc_ac_c*>(args,0);
    if(!arena_aggro(actor,fpcNm_E_TT_e))return HOOK_CONTINUE;
    *static_cast<bool*>(result)=true;return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_bubble_search(ModContext*,void* args,void* result,void*) {
    auto* actor=mods::arg<e_bu_class*>(args,0);
    const float range=mods::arg<float>(args,1);
    if(!arena_aggro(reinterpret_cast<fopAc_ac_c*>(actor),fpcNm_E_BU_e) ||
       (range!=2000.f && range!=actor->pl_search_range))return HOOK_CONTINUE;
    *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
}
bool phantom_projectile_charge(fopAc_ac_c* actor,const Enemy& enemy) {
    if (enemy.profile!=fpcNm_E_PZ_e || !actor || actor->health<=0) return false;
    const auto* phantom=reinterpret_cast<const daE_PZ_c*>(actor);
    return phantom->mActionMode==daE_PZ_c::ACTION_ATTACK_e && phantom->mMoveMode>=0 && phantom->mMoveMode<=2;
}
bool update_extended_enemy(fopAc_ac_c* actor,Enemy& enemy) {
    maintain_helmasaur_armor(enemy,actor);
    auto body=extended_body(actor,enemy.profile);
    if(!body.targets[0])return true;
    enemy.hitCooldown=0; // Distinct run attacks have no shared damage lockout.
    if(body.status)body.status->Move();
    auto& effect=effect_for(enemy);
    cCcD_Obj* attack=enemy.childHit;enemy.childHit=nullptr;
    if(attack && s_run.dual_wield && effect.meleeSerial==s_meleeSerial &&
       s_swordAction==SwordAction::Normal && native_sword_collider(attack))attack=nullptr;
    for(auto* target:body.targets) if(target&&target->ChkTgHit()) {
        auto* hit=target->GetTgHitObj();
        const bool duplicate=s_run.dual_wield&&effect.meleeSerial==s_meleeSerial&&
            s_swordAction==SwordAction::Normal&&native_sword_collider(hit);
        if(!attack&&!duplicate&&roster_attack(hit))attack=hit;
    }
    const int ball=ball_contact(actor,attack);
    if(ball==0)attack=nullptr;
    if(ball==1)enemy.hitCooldown=0;
    // Consume status damage only when it can be applied; real hits take priority.
    if(!attack&&(effect.pending||effect.fixedPending)&&!enemy.hitCooldown) {
        auto* link=daAlink_getAlinkActorClass();
        if(link&&fopAcM_GetID(link)==s_player) {
            effect.attribution.consume();
            effect.damage=effect.pending;effect.pending=0;effect.injected=true;
            effect.fixedDamage=effect.fixedPending;effect.fixedPending=0;
            effect.critical=effect.pendingCritical;effect.pendingCritical=false;
            effect.stagger=effect.pendingStagger;effect.pendingStagger=false;
            effect.status.Init(120,0xff,link);effect.collider.Set(kAttackSource);
            effect.collider.SetStts(&effect.status);effect.collider.SetAtType(AT_TYPE_ARROW);
            effect.collider.SetAtAtp(1);effect.collider.SetAtMtrl(dCcD_MTRL_NONE);
            attack=&effect.collider;
        }
    }
    if(attack&&!enemy.hitCooldown&&actor->health>0) {
        dCcU_AtInfo info=body.info?*body.info:dCcU_AtInfo{};
        info.mpCollider=attack;info.mPowerType=0;
        const int before=actor->health;
        cc_at_check(actor,&info); // Existing power, material, Crit and status hooks run once.
        if(actor->health<before) {
            enemy.hitCooldown=0;
            boss_received_damage(enemy,actor->health);
            if(enemy.profile==fpcNm_E_GI_e) {
                auto* gibdo=reinterpret_cast<daE_GI_c*>(actor);
                gibdo->mPlayerStunTimer=0;gibdo->mCryTimer=20;
            }
            if(causes_stagger(attack) && !phantom_projectile_charge(actor,enemy) && boss_accept_stagger(enemy))enemy.stagger=8;
        }
        effect.injected=false;effect.damage=0;effect.fixedDamage=0;effect.critical=false;effect.stagger=false;
    }
    for(auto* target:body.targets)if(target)target->ResetTgHit();
    if(actor->health<=0) {
        credit_death(enemy,actor); // Credit before requesting deletion; never credit unloads.
        for(auto* target:body.targets)if(target){target->OffAtSetBit();target->OffTgSetBit();}
        fopAcM_createDisappear(actor,&actor->current.pos,15,0,0xff);
        enemy.retiring=true;svc_actor->delete_actor(mod_ctx,enemy.id);
        return false;
    }
    if(phantom_projectile_charge(actor,enemy)) {
        enemy.stagger=0;
        auto* phantom=reinterpret_cast<daE_PZ_c*>(actor);
        // These are independent of the damage cylinder and Survivors stagger.
        // Native damage_check can warp to WAIT/11 and replace the casting
        // animation, which makes the charging child ball destroy itself.
        // Clear at the actor-execute boundary as well as skipping the native
        // damage hook, so inlined native reaction paths cannot cancel a cast.
        phantom->mWarpCylCollider.ResetTgHit();
        phantom->mBombArrowHit=false;
    }
    if(enemy.stagger) {
        --enemy.stagger;
        // Re-register body targets during the short stagger; movement/AI and
        // attack colliders pause, while new hits and burn ticks remain possible.
        for(auto* target:body.targets)if(target){target->OffAtSetBit();dComIfG_Ccsp()->Set(target);}
        return false;
    }
    return true;
}
