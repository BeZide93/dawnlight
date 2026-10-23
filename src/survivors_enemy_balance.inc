// Encounter multipliers are applied once, at Link's damage boundary. Explicit
// boss values override the ordinary promoted-elite factor.
bool native_boss_profile(ProfileName p) {
    switch(p) {
    case fpcNm_B_TN_e:case fpcNm_B_GG_e:case fpcNm_B_DS_e:case fpcNm_E_ZH_e:
    case fpcNm_E_RDB_e:case fpcNm_E_VT_e:case fpcNm_E_YMB_e:case fpcNm_E_TH_e:
    case fpcNm_E_HZELDA_e:case fpcNm_B_GND_e:case fpcNm_E_PZ_e:return true;
    default:return false;
    }
}
unsigned enemy_attack_factor(const Enemy& e) {
    if(!e.elite&&!native_boss_profile(e.profile))return 1;
    switch(e.profile) {
    case fpcNm_B_GG_e:return e.elite?2:1; // Field replacement retains the giant Kargarok multiplier.
    case fpcNm_B_TN_e:return 2;
    case fpcNm_E_VT_e:case fpcNm_B_DS_e:case fpcNm_E_YC_e:case fpcNm_E_HZELDA_e:return 4;
    case fpcNm_E_YMB_e:case fpcNm_E_TH_e:return 3;
    case fpcNm_B_GND_e:case fpcNm_E_PZ_e:return 5;
    default:return e.elite&&!native_boss_profile(e.profile)?2:1;
    }
}
bool faster_boss(const Enemy& e) {return e.elite||native_boss_profile(e.profile);}
// Accelerate native attack-readiness/recovery clocks, never
// animation clocks, hit windows, charge durations or invulnerability timers.
void advance_elite_cooldowns(fopAc_ac_c* actor,const Enemy& enemy) {
    if(enemy.dormant)return;
    // Stalchildren are ordinary enemies too. Three extra decrements plus the
    // native tick quarter readiness and close-range idle waits, clamped at zero.
    if(enemy.profile==fpcNm_E_BS_e) {
        auto* e=reinterpret_cast<e_bs_class*>(actor);
        if(e->action==2) {
            e->timers[2]=std::max(0,int(e->timers[2])-3);
            if(e->mode==5)e->timers[0]=std::max(0,int(e->timers[0])-3);
        }
        return;
    }
    if(!enemy.elite||native_boss_profile(enemy.profile))return;
    const auto tick=[](auto& timer){if(timer>0)--timer;};
    switch(enemy.profile) {
    case fpcNm_E_DN_e:tick(reinterpret_cast<e_dn_class*>(actor)->timer[2]);break;
    case fpcNm_E_MF_e:tick(reinterpret_cast<e_mf_class*>(actor)->field_0x6c0[2]);break;
    case fpcNm_E_SF_e:{auto& timer=reinterpret_cast<e_sf_class*>(actor)->mTimers[2];
        timer=std::max(0,int(timer)-3);break;} // 4x native, twice the previous elite rate.
    case fpcNm_E_KR_e:tick(reinterpret_cast<e_kr_class*>(actor)->field_0x69c[5]);break;
    case fpcNm_E_MM_e:{auto* e=reinterpret_cast<e_mm_class*>(actor);if(e->action==1&&e->mode==5)tick(e->timers[0]);break;}
    case fpcNm_E_OC_e:{auto* e=reinterpret_cast<daE_OC_c*>(actor);if(e->mActionMode==3){tick(e->field_0x6c2);tick(e->field_0x6c0);}break;}
    case fpcNm_E_RD_e:{auto* e=reinterpret_cast<e_rd_class*>(actor);if(e->action==6&&e->mode==2)tick(e->timer[0]);break;}
    case fpcNm_E_SH_e:{auto* e=reinterpret_cast<e_sh_class*>(actor);if(e->field_0x676==3&&e->field_0x678==1)tick(e->field_0x698[0]);break;}
    case fpcNm_E_S1_e:{auto* e=reinterpret_cast<e_s1_class*>(actor);if(e->mAction==3&&e->mMode==1)tick(e->mTimers[0]);break;}
    case fpcNm_E_RDY_e:{auto* e=reinterpret_cast<e_rdy_class*>(actor);if((e->mAction==4&&(e->mMode==1||e->mMode==3||e->mMode==10))||(e->mAction==6&&e->mMode==2))tick(e->mTimer[0]);break;}
    case fpcNm_E_YC_e:tick(reinterpret_cast<e_yc_class*>(actor)->mTimer[2]);break;
    default:break;
    }
}
