// Only active guard actions block. Armor and ordinary chase remain vulnerable.
bool boss_blocks_from(const Enemy& enemy,fopAc_ac_c* actor,const cXyz& source) {
    if(!actor || actor->health<=0)return false;
    bool guarding=false;
    if(enemy.profile==fpcNm_B_TN_e) {
        const auto action=reinterpret_cast<daB_TN_c*>(actor)->mActionMode1;
        guarding=action==daB_TN_c::ACT_GUARDH || action==daB_TN_c::ACT_GUARDL;
    } else if(enemy.profile==fpcNm_B_GND_e)
        guarding=reinterpret_cast<b_gnd_class*>(actor)->mActionMode==12; // native ACTION_DEFENCE
    else if(enemy.profile==fpcNm_E_SF_e&&enemy.elite&&!enemy.dormant&&!enemy.stagger) {
        const auto* stalfos=reinterpret_cast<e_sf_class*>(actor);
        guarding=stalfos->arg3!=0xff&&stalfos->mAction==7&&
            ((stalfos->mActionPhase==1&&stalfos->mpModelMorf->getFrame()>=2.f)||stalfos->mActionPhase==2);
    }
    if(enemy.profile==fpcNm_E_MF_e&&enemy.elite&&!enemy.dormant&&!enemy.stagger) {
        const auto* dynalfos=reinterpret_cast<e_mf_class*>(actor);
        guarding=dynalfos->mAction==7&&((dynalfos->field_0x5b4==6&&
            dynalfos->mpModelMorf->getFrame()>=2.f)||dynalfos->field_0x5b4==7);
    }
    if(!guarding)return false;
    const auto delta=source-actor->current.pos;
    const s16 angle=s16(cM_atan2s(delta.x,delta.z)-actor->shape_angle.y);
    return std::abs(int(angle))<=0x4000;
}
bool block_boss_contact(const Enemy& enemy,fopAc_ac_c* actor,cCcD_Obj* attack,cCcD_Obj* target) {
    auto* link=daAlink_getAlinkActorClass();if(!link)return false;
    bool owned=native_sword_collider(attack)||attack==&link->mGuardAtCps;
    cXyz source=link->current.pos;
    for(const auto& projectile:s_attacks)if(projectile.active && attack==&projectile.collider) {
        owned=true;source=projectile.position;
        // Use the approach side if the projectile has already crossed the center.
        if(projectile.velocity.absXZ()>0.01f)source=actor->current.pos-projectile.velocity;
        break;
    }
    if(!owned || !boss_blocks_from(enemy,actor,source))return false;
    attack->SetAtHit(target);
    auto* info=static_cast<dCcD_GObjInf*>(attack->GetGObjInf());
    info->SetAtHitApid(fopAcM_GetID(target->GetAc()));info->OnAtShieldHit();
    return true; // No pending damage, burn, score, lifesteal or knockback.
}
