// Typed access to native body targets; never reinterpret one species as another.
struct ExtendedBody {
    std::array<dCcD_GObjInf*,8> targets{};
    dCcU_AtInfo* info=nullptr;
    dCcD_Stts* status=nullptr;
    mDoExt_McaMorfSO* morph=nullptr;
};
ExtendedBody extended_body(fopAc_ac_c* actor,ProfileName profile) {
#define BODY(P,T,Targets,Info,Status,Morph) case P: { auto* e=reinterpret_cast<T*>(actor);return {Targets,Info,Status,Morph}; }
    switch(profile) {
    BODY(fpcNm_E_WB_e,e_wb_class,(std::array<dCcD_GObjInf*,8>{&e->cc_sph[0],&e->cc_sph[1],&e->cc_sph[2],&e->cc_sph[3],&e->cc_sph[4],&e->cc_sph[5],&e->cc_sph[6]}),&e->at_info,&e->Stts,e->anm_p)
    BODY(fpcNm_E_TT_e,daE_TT_c,(std::array<dCcD_GObjInf*,8>{&e->mSphere}),&e->mAtInfo,&e->mStts,e->mpMorfSO)
    BODY(fpcNm_E_GE_e,daE_GE_c,(std::array<dCcD_GObjInf*,8>{&e->mSphere,&e->mSphere2}),&e->mAtInfo,&e->mStts,e->mpMorfSO)
    BODY(fpcNm_E_KR_e,e_kr_class,(std::array<dCcD_GObjInf*,8>{&e->mSphere2,&e->mSphere3}),&e->mAtInfo,&e->mStts,e->mpMorf)
    BODY(fpcNm_E_DN_e,e_dn_class,(std::array<dCcD_GObjInf*,8>{&e->cc_sph[0],&e->cc_sph[1],&e->cc_sph[2],&e->tate_sph}),&e->at_info,&e->stts,e->anm_p)
    case fpcNm_E_MM_e: {
        auto* e=reinterpret_cast<e_mm_class*>(actor);
        ExtendedBody body{{&e->ccSph},&e->atInfo,&e->ccStts,e->modelMorf};
        auto* shell=actor_by_id(e->armorChildId);
        if(shell&&fopAcM_GetName(shell)==fpcNm_E_MM_MT_e) {
            auto* armor=reinterpret_cast<e_mm_mt_class*>(shell);
            body.targets[1]=&armor->m_sphere;body.targets[2]=&armor->m_sph;
        }
        return body;
    }
    BODY(fpcNm_E_CR_e,e_cr_class,(std::array<dCcD_GObjInf*,8>{&e->ccSph}),&e->atInfo,&e->ccStts,e->modelMorf)
    BODY(fpcNm_E_BS_e,e_bs_class,(std::array<dCcD_GObjInf*,8>{&e->ccSph[0],&e->ccSph[1]}),&e->atInfo,&e->ccStts,e->modelMorf)
    BODY(fpcNm_E_GI_e,daE_GI_c,(std::array<dCcD_GObjInf*,8>{&e->mCcSph[0],&e->mCcSph[1]}),&e->mAtInfo,&e->mCcStts,e->mpModelMorf)
    BODY(fpcNm_E_SF_e,e_sf_class,(std::array<dCcD_GObjInf*,8>{&e->mCcSphs[0],&e->mCcSphs[1],&e->mCcSphs[2],&e->mTateSph}),&e->mAtInfo,&e->mStts,e->mpModelMorf)
    BODY(fpcNm_E_FS_e,e_fs_class,(std::array<dCcD_GObjInf*,8>{&e->mCcCyl}),&e->mAtInfo,&e->mCcStts,e->mpMorf)
    BODY(fpcNm_E_BU_e,e_bu_class,(std::array<dCcD_GObjInf*,8>{&e->ccSph}),&e->atInfo,&e->ccStts,e->modelMorf)
    BODY(fpcNm_E_SM2_e,e_sm2_class,(std::array<dCcD_GObjInf*,8>{&e->ccSph[0],&e->ccSph[1],&e->ccSph[2],&e->ccSph[3]}),&e->atInfo,&e->ccStts,e->modelMorf)
    BODY(fpcNm_E_MF_e,e_mf_class,(std::array<dCcD_GObjInf*,8>{&e->field_0xa7c[0],&e->field_0xa7c[1],&e->field_0xa7c[2],&e->field_0xf5c}),&e->mAtInfo,&e->mStts,e->mpModelMorf)
    BODY(fpcNm_B_GG_e,daB_GG_c,(std::array<dCcD_GObjInf*,8>{&e->mCcCyl,&e->mCcHookCyl,&e->mCcShieldSph}),&e->mAtInfo,&e->mCcStts,e->mpModelMorf)
    BODY(fpcNm_B_TN_e,daB_TN_c,(std::array<dCcD_GObjInf*,8>{&e->mSphA[0],&e->mSphA[1],&e->mSphA[2],&e->mSphB[0],&e->mSphB[1],&e->mSphB[2],&e->mSphC}),&e->mAtInfo,&e->mStts1,e->mpModelMorf1)
    BODY(fpcNm_E_DD_e,e_dd_class,(std::array<dCcD_GObjInf*,8>{&e->field_0xa74[0],&e->field_0xa74[1],&e->field_0xa74[2]}),&e->mAtInfo,&e->mStts,e->mpModelMorf)
    BODY(fpcNm_E_SH_e,e_sh_class,(std::array<dCcD_GObjInf*,8>{&e->mSphArr[0],&e->mSphArr[1]}),&e->mAtInfo,&e->mStts,e->mAnm_p)
    BODY(fpcNm_E_RDY_e,e_rdy_class,(std::array<dCcD_GObjInf*,8>{&e->mCcSph[0],&e->mCcSph[1],&e->mCcSph[2]}),&e->mAtInfo,&e->mCcStts,e->mpMorf)
    BODY(fpcNm_E_S1_e,e_s1_class,(std::array<dCcD_GObjInf*,8>{&e->mCcSph[0],&e->mCcSph[1],&e->mCcSph[2]}),&e->mAtInfo,&e->mCcStts,e->mpMorf)
    BODY(fpcNm_E_YK_e,e_yk_class,(std::array<dCcD_GObjInf*,8>{&e->mCollisionSphere}),&e->mAtColliderInfo,&e->mCollisionStatus,e->mpMorfSO)
    BODY(fpcNm_E_YC_e,e_yc_class,(std::array<dCcD_GObjInf*,8>{&e->mCcSph}),&e->mAtInfo,&e->mCcStts,e->mpMorf)
    BODY(fpcNm_B_DS_e,daB_DS_c,(std::array<dCcD_GObjInf*,8>{&e->mHeadSph[0],&e->mHeadSph[1],&e->mHeadSph[2],&e->mHeadSph[3],&e->mHeadSph[4]}),&e->mAtInfo,&e->mCcStts,e->mpMorf)
    BODY(fpcNm_E_ZH_e,daE_ZH_c,(std::array<dCcD_GObjInf*,8>{&e->field_0xa08,&e->field_0xb40}),&e->mAtInfo,&e->mStts,e->mpModelMorf)
    BODY(fpcNm_E_RDB_e,e_rdb_class,(std::array<dCcD_GObjInf*,8>{&e->field_0x944[0],&e->field_0x944[1],&e->field_0x944[2]}),&e->mAtInfo,&e->mStts,e->mpModelMorf)
    BODY(fpcNm_E_VT_e,daE_VA_c,(std::array<dCcD_GObjInf*,8>{&e->mBodyCyls[0],&e->mBodyCyls[1],&e->mBodyCyls[2],&e->mNeckSph}),&e->mAtInfo,&e->mBodyCcStts,e->mpMorf)
    BODY(fpcNm_E_YMB_e,daE_YMB_c,(std::array<dCcD_GObjInf*,8>{&e->field_0x82c[0],&e->field_0x82c[1],&e->field_0xa9c}),&e->mAtInfo,&e->mStts,e->mpModelMorf)
    BODY(fpcNm_E_PZ_e,daE_PZ_c,(std::array<dCcD_GObjInf*,8>{&e->mCylCollider}),&e->mAtInfo,&e->mColliderStts,e->mpModelMorf)
    BODY(fpcNm_E_TH_e,e_th_class,(std::array<dCcD_GObjInf*,8>{&e->mCcSph[0],&e->mCcSph[1],&e->mCcSph[2]}),&e->mAtInfo,&e->mCcStts,e->mpModelMorf)
    BODY(fpcNm_E_HZELDA_e,e_hzelda_class,(std::array<dCcD_GObjInf*,8>{&e->mSphCc[0],&e->mSphCc[1],&e->mSphCc[2]}),&e->mAtInfo,&e->field_0x958,e->mpModelMorf)
    BODY(fpcNm_B_GND_e,b_gnd_class,(std::array<dCcD_GObjInf*,8>{&e->mGndSph[0],&e->mGndSph[1],&e->mGndSph[2]}),&e->mAtInfo,&e->mGndCcStts,e->mpModelMorf)
    default:return {};
    }
#undef BODY
}
void maintain_helmasaur_armor(Enemy& enemy,fopAc_ac_c* actor) {
    if(enemy.profile!=fpcNm_E_MM_e||enemy.retiring||enemy.credited||actor->health<=0)return;
    auto* helmasaur=reinterpret_cast<e_mm_class*>(actor);
    helmasaur->field_0x672=1;
    helmasaur->field_0xb99&=~4; // Never enter the native detached-shell path.
    const auto id=helmasaur->armorChildId;
    if(actor_by_id(id)||fpcM_IsCreating(id))return;
    // Failed child creation is retried at most once per playable second.
    if(enemy.age%30==0)
        helmasaur->armorChildId=fopAcM_createChild(fpcNm_E_MM_MT_e,enemy.id,0,&actor->current.pos,
            fopAcM_GetRoomNo(actor),&actor->shape_angle,&actor->scale,actor->argument,nullptr);
}
void initialize_extended_enemy(Enemy& enemy,fopAc_ac_c* actor) {
    const auto& native=native_roster[enemy.species];
    if(enemy.elite&&elite_scale(Species(enemy.species))>1.f) {
        const float size=elite_scale(Species(enemy.species));actor->scale=cXyz(size,size,size);
    }
    // Dedicated bosses may exceed the ordinary elite baseline (Stallord's head
    // arrives at 20 minutes, when a single upgraded critical can exceed 120 HP).
    actor->health=actor->field_0x560=s16(s_run.spawn_enemy_health(native.hp,enemy.elite,s_map,enemy.eliteSlot));
    // Only these doubled elites omit actor scale in their native model matrix.
    if (enemy.elite && (enemy.profile==fpcNm_E_TT_e || enemy.profile==fpcNm_E_GI_e ||
        enemy.profile==fpcNm_E_FS_e || enemy.profile==fpcNm_E_BU_e ||
        enemy.profile==fpcNm_E_DD_e || enemy.profile==fpcNm_E_SH_e || enemy.profile==fpcNm_E_YC_e)) {
        if(auto* morph=extended_body(actor,enemy.profile).morph) morph->getModel()->setBaseScale(actor->scale);
    }
    if (enemy.profile==fpcNm_E_GI_e) reinterpret_cast<daE_GI_c*>(actor)->setBck(12,2,3.f,1.f);
}
void engage_extended_enemy(fopAc_ac_c* actor,const Enemy& enemy) {
    if (!extended_enemy(enemy.profile) || enemy.stagger) return;
    auto* link=daAlink_getAlinkActorClass();if(!link)return;
    const s16 yaw=cM_atan2s(link->current.pos.x-actor->current.pos.x,link->current.pos.z-actor->current.pos.z);
    switch(enemy.profile) {
    case fpcNm_E_TT_e: {auto* e=reinterpret_cast<daE_TT_c*>(actor);e->field_0x6d4=0;if(e->mAction==0||e->mAction==5)e->setActionMode(1,0);break;}
    case fpcNm_E_DN_e: {auto* e=reinterpret_cast<e_dn_class*>(actor);e->pl_range=1.e9f;if(e->action==0){e->action=3;e->mode=0;}break;}
    case fpcNm_E_MF_e: {auto* e=reinterpret_cast<e_mf_class*>(actor);e->field_0x6b8=1.e9f;if(e->mAction==0){e->mAction=3;e->field_0x5b4=0;}break;}
    case fpcNm_E_SF_e: {auto* e=reinterpret_cast<e_sf_class*>(actor);e->mRecognizeDist=1.e9f;if(e->mAction==0){e->mAction=3;e->mActionPhase=0;}break;}
    case fpcNm_E_BS_e: {auto* e=reinterpret_cast<e_bs_class*>(actor);e->pl_recognize_dist=1.e9f;if(e->action==1){e->action=2;e->mode=0;}break;}
    case fpcNm_E_MM_e: {auto* e=reinterpret_cast<e_mm_class*>(actor);if(e->action==0){e->action=1;e->mode=0;}break;}
    case fpcNm_E_GE_e: {auto* e=reinterpret_cast<daE_GE_c*>(actor);actor->home.pos.x=link->current.pos.x;actor->home.pos.z=link->current.pos.z;if(e->mActionMode==0||e->mActionMode==6)e->setActionMode(2);break;}
    case fpcNm_E_KR_e: {auto* e=reinterpret_cast<e_kr_class*>(actor);if(e->mCurAction==0||e->mCurAction==4||e->mCurAction==7){e->mCurAction=3;e->field_0x672=0;}break;}
    case fpcNm_E_CR_e: {auto* e=reinterpret_cast<e_cr_class*>(actor);e->angle_target=yaw;e->timers[1]=2;break;} // Chase rather than flee; retain egg drops.
    case fpcNm_E_GI_e: {auto* e=reinterpret_cast<daE_GI_c*>(actor);if(e->mActionMode<=1){e->mActionMode=2;e->mMoveMode=1;}break;}
    case fpcNm_E_FS_e: {auto* e=reinterpret_cast<e_fs_class*>(actor);if(e->mAction==e_fs_class::ACT_WAIT){e->mAction=e_fs_class::ACT_MOVE;e->mMode=0;}break;}
    case fpcNm_E_BU_e: {auto* e=reinterpret_cast<e_bu_class*>(actor);e->pl_search_range=1.e9f;if(e->action==0||e->action==5||e->action==6){e->action=1;e->mode=0;}break;}
    case fpcNm_E_SM2_e: {auto* e=reinterpret_cast<e_sm2_class*>(actor);e->combine_off_timer=30000;if(e->action==ACTION_COMBINE){e->action=ACTION_ATTACK;e->mode=0;}break;}
    case fpcNm_B_TN_e: {auto* e=reinterpret_cast<daB_TN_c*>(actor);if(e->mActionMode1<=2)e->setActionMode(daB_TN_c::ACT_CHASEH,0);break;}
    case fpcNm_E_DD_e: {auto* e=reinterpret_cast<e_dd_class*>(actor);if(e->mAction==0){e->mAction=3;e->field_0x68c=0;}break;}
    case fpcNm_E_SH_e: {auto* e=reinterpret_cast<e_sh_class*>(actor);e->field_0x6b8=e->field_0x6bc=1.e9f;break;}
    case fpcNm_E_RDY_e: {auto* e=reinterpret_cast<e_rdy_class*>(actor);e->mPlayerCheckDist=1.e9f;if(e->mAction==0){e->mAction=e->mWeaponType==1?3:5;e->mMode=0;}break;}
    case fpcNm_E_S1_e: {auto* e=reinterpret_cast<e_s1_class*>(actor);e->mSearchRange=1.e9f;e->mGroupID=0xff;if(e->mAction==0){e->mAction=2;e->mMode=0;}break;}
    case fpcNm_E_YK_e: {auto* e=reinterpret_cast<e_yk_class*>(actor);e->mPlayerTrigger=1.e9f;if(e->mAction==0||e->mAction==4||e->mAction==5||e->mAction==6){e->mAction=1;e->mActionPhase=0;}break;}
    default:break;
    }
}
// Conservative locomotion states: jumps, lunges, charges and attack flights are
// never multiplied by Enemy Speed. New species retain native attack timing.
bool extended_locomotion(fopAc_ac_c* actor,ProfileName p) {
    switch(p) {
    case fpcNm_E_DN_e:return reinterpret_cast<e_dn_class*>(actor)->action==3;
    case fpcNm_E_MF_e:return reinterpret_cast<e_mf_class*>(actor)->mAction==3;
    case fpcNm_E_SF_e:return reinterpret_cast<e_sf_class*>(actor)->mAction==3;
    case fpcNm_E_BS_e:return reinterpret_cast<e_bs_class*>(actor)->action==2;
    case fpcNm_E_GI_e:return reinterpret_cast<daE_GI_c*>(actor)->mActionMode==2;
    case fpcNm_E_FS_e:return reinterpret_cast<e_fs_class*>(actor)->mAction==e_fs_class::ACT_MOVE;
    case fpcNm_E_CR_e:return reinterpret_cast<e_cr_class*>(actor)->action==0;
    case fpcNm_B_TN_e:return reinterpret_cast<daB_TN_c*>(actor)->mActionMode1==daB_TN_c::ACT_CHASEH;
    case fpcNm_E_DD_e:return reinterpret_cast<e_dd_class*>(actor)->mAction==3;
    case fpcNm_E_SH_e:return reinterpret_cast<e_sh_class*>(actor)->field_0x676==2;
    case fpcNm_E_RDY_e:{const auto a=reinterpret_cast<e_rdy_class*>(actor)->mAction;return a==3||a==5;}
    case fpcNm_E_S1_e:return reinterpret_cast<e_s1_class*>(actor)->mAction==2;
    default:return false;
    }
}
// Native lunges and jumps may deliberately use a heading different from the
// current player direction (including Darknut's backwards/sideways movement).
// Let their own steering/target-lock windows run before and after floor correction.
bool committed_enemy_motion(fopAc_ac_c* actor,ProfileName profile) {
    switch(profile) {
    case fpcNm_E_MM_e:return reinterpret_cast<e_mm_class*>(actor)->action==1;
    case fpcNm_E_TT_e: {
        const auto* e=reinterpret_cast<daE_TT_c*>(actor);
        return (e->mAction==1 && e->mMode>=6 && e->mMode<=8) ||
            (e->mAction==2 && e->mMode>=2) || (e->mAction==6 && e->mMode>=12);
    }
    case fpcNm_E_GE_e: {
        const auto* e=reinterpret_cast<daE_GE_c*>(actor);return e->mActionMode==2 && e->mMode>=2;
    }
    case fpcNm_E_KR_e:return reinterpret_cast<e_kr_class*>(actor)->mCurAction==3;
    case fpcNm_E_YK_e: {
        const auto* e=reinterpret_cast<e_yk_class*>(actor);return e->mAction==ACT_ATTACK && e->mActionPhase>=2;
    }
    case fpcNm_E_SH_e: {
        // The pounce tell already owns its heading. Phase 1 still follows Link
        // so native alignment can complete before the boss aim lock starts.
        const auto* e=reinterpret_cast<e_sh_class*>(actor);return e->field_0x676==3 && e->field_0x678>=2;
    }
    case fpcNm_E_S1_e: {
        const auto* e=reinterpret_cast<e_s1_class*>(actor);return e->mAction==3&&e->mMode==2;
    }
    case fpcNm_E_SM2_e:return reinterpret_cast<e_sm2_class*>(actor)->action==ACTION_ATTACK;
    case fpcNm_E_DN_e: {
        const auto action=reinterpret_cast<e_dn_class*>(actor)->action;
        return action==5 || action==6 || action==8 || action==25;
    }
    case fpcNm_E_MF_e: {
        const auto action=reinterpret_cast<e_mf_class*>(actor)->mAction;
        return action==5 || action==6 || action==8 || action==25;
    }
    case fpcNm_B_TN_e: {
        const auto action=reinterpret_cast<daB_TN_c*>(actor)->mActionMode1;
        return action!=3 && action!=6 && action!=12; // Preserve unarmored footwork, lunges and armor-break jump.
    }
    default:return false;
    }
}
void face_extended_enemy(fopAc_ac_c* actor,const Enemy& enemy) {
    if(!extended_enemy(enemy.profile)||enemy.stagger || enemy.profile==fpcNm_B_DS_e ||
       enemy.profile==fpcNm_E_ZH_e || enemy.profile==fpcNm_E_YC_e ||
       enemy.species>=unsigned(Species::KingBulblin))return;
    if(committed_enemy_motion(actor,enemy.profile))return;
    auto* link=daAlink_getAlinkActorClass();if(!link)return;
    const cXyz delta=link->current.pos-actor->current.pos;
    if(delta.absXZ()<1||delta.absXZ()>400*std::max(1.f,actor->scale.x)||std::abs(delta.y)>250*std::max(1.f,actor->scale.y))return;
    const s16 yaw=cM_atan2s(delta.x,delta.z);
    actor->current.angle.y=actor->shape_angle.y=yaw;
    if(enemy.profile==fpcNm_E_CR_e)reinterpret_cast<e_cr_class*>(actor)->angle_target=yaw;
    if(enemy.profile==fpcNm_E_FS_e)reinterpret_cast<e_fs_class*>(actor)->mTargetAngleY=yaw;
    if(enemy.profile==fpcNm_E_SM2_e)reinterpret_cast<e_sm2_class*>(actor)->mCurrentAngleYTarget=yaw;
    if(enemy.profile==fpcNm_E_KR_e)reinterpret_cast<e_kr_class*>(actor)->field_0x6c0=yaw;
}
