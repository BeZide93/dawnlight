struct NativeSpecies { ProfileName profile; u32 parameters; s8 argument; float height; unsigned hp; };
constexpr std::array<NativeSpecies,species_count> native_roster{{
    {fpcNm_E_OC_e,0x00ff0000,-1,0,8}, {fpcNm_E_BA_e,0xffff0f01,-1,180,4},
    {fpcNm_E_MS_e,0xffff0000,-1,0,4}, {fpcNm_E_RD_e,0xff000100,-1,0,8},
    {fpcNm_E_RD_e,0xff000200,-1,0,8}, {fpcNm_E_TT_e,0x000000ff,-1,0,8},
    {fpcNm_E_DN_e,0xff000000,-1,0,16}, {fpcNm_E_MM_e,0x0000ff00,-1,0,12},
    {fpcNm_E_MM_e,0x0000ff00,1,0,24}, {fpcNm_E_GE_e,0x00ffff01,-1,180,6},
    {fpcNm_E_KR_e,0xffffff00,-1,220,12}, {fpcNm_E_CR_e,0,-1,0,10},
    {fpcNm_E_BS_e,0x00ffff00,-1,0,8},
    {fpcNm_E_GI_e,0x00ff01ff,-1,0,20}, {fpcNm_E_SF_e,0x0000ff00,-1,0,24},
    {fpcNm_E_FS_e,0,-1,0,8}, {fpcNm_E_BU_e,0xffff0f00,-1,180,6},
    {fpcNm_E_BU_e,0xffff1f00,-1,180,10}, {fpcNm_E_SM2_e,0xffff0060,-1,0,12},
    {fpcNm_E_MF_e,0xff000000,-1,0,24}, {fpcNm_B_GG_e,0xffff0002,-1,300,1200},
    {fpcNm_B_TN_e,0x000001ff,-1,0,1200},
    {fpcNm_E_DD_e,0xffff0000,-1,0,16}, {fpcNm_E_SH_e,0x0014ff00,-1,0,10},
    {fpcNm_E_RDY_e,0xff000107,-1,0,8}, {fpcNm_E_RDY_e,0xff000207,-1,0,8},
    {fpcNm_E_S1_e,0xffffffF0,-1,0,20}, {fpcNm_E_YK_e,0xffff0f01,-1,180,4},
    {fpcNm_E_YC_e,0,-1,260,12}, {fpcNm_B_DS_e,0xffffff02,-1,300,1200},
    {fpcNm_E_ZH_e,0xffffff00,-1,240,1200},
    {fpcNm_E_RDB_e,0xffffff00,-1,0,1200}, {fpcNm_E_VT_e,0xffffffff,-1,0,24},
    {fpcNm_E_YMB_e,0xffffffff,-1,300,24}, {fpcNm_E_PZ_e,0xffffff01,-1,0,1200},
    {fpcNm_E_TH_e,0xffffff00,-1,0,24}, {fpcNm_E_HZELDA_e,0xffffff00,-1,300,24},
    {fpcNm_B_GND_e,0xffffff00,-1,0,1200},
    {fpcNm_E_WB_e,0xffff0000,-1,0,8}
}};
bool extended_enemy(ProfileName profile) {
    return std::any_of(native_roster.begin()+5,native_roster.end(),
        [profile](const auto& native){return native.profile==profile;});
}
// Suppress only native puzzle/instant-kill damage rules for run-owned variants.
#define ROSTER_HIT(Name, Symbol, Type) DEFINE_HOOK_SYMBOL(Symbol,void(Type*),Name)
ROSTER_HIT(RosterTT,"daE_TT_c::damage_check",daE_TT_c);
ROSTER_HIT(RosterDN,"src/d/actor/d_a_e_dn.cpp#damage_check",e_dn_class);
ROSTER_HIT(RosterMM,"src/d/actor/d_a_e_mm.cpp#damage_check",e_mm_class);
ROSTER_HIT(RosterMMArmor,"src/d/actor/d_a_e_mm.cpp#damage_checkMetOn",e_mm_class);
ROSTER_HIT(RosterGE,"daE_GE_c::damage_check",daE_GE_c);
ROSTER_HIT(RosterKR,"src/d/actor/d_a_e_kr.cpp#damage_check",e_kr_class);
ROSTER_HIT(RosterCR,"src/d/actor/d_a_e_cr.cpp#damage_check",e_cr_class);
ROSTER_HIT(RosterBS,"src/d/actor/d_a_e_bs.cpp#damage_check",e_bs_class);
ROSTER_HIT(RosterGI,"daE_GI_c::damage_check",daE_GI_c);
ROSTER_HIT(RosterSF,"src/d/actor/d_a_e_sf.cpp#damage_check",e_sf_class);
ROSTER_HIT(RosterFS,"src/d/actor/d_a_e_fs.cpp#damage_check",e_fs_class);
ROSTER_HIT(RosterBU,"src/d/actor/d_a_e_bu.cpp#damage_check",e_bu_class);
ROSTER_HIT(RosterSM,"src/d/actor/d_a_e_sm2.cpp#damage_check",e_sm2_class);
ROSTER_HIT(RosterMF,"src/d/actor/d_a_e_mf.cpp#damage_check",e_mf_class);
ROSTER_HIT(RosterWB,"src/d/actor/d_a_e_wb.cpp#damage_check",e_wb_class);
ROSTER_HIT(RosterGG,"daB_GG_c::At_Check",daB_GG_c);
ROSTER_HIT(RosterTN,"daB_TN_c::damage_check",daB_TN_c);
ROSTER_HIT(RosterDD,"src/d/actor/d_a_e_dd.cpp#damage_check",e_dd_class);
ROSTER_HIT(RosterSH,"src/d/actor/d_a_e_sh.cpp#damage_check",e_sh_class);
ROSTER_HIT(RosterRDY,"src/d/actor/d_a_e_rdy.cpp#damage_check",e_rdy_class);
ROSTER_HIT(RosterS1,"src/d/actor/d_a_e_s1.cpp#damage_check",e_s1_class);
ROSTER_HIT(RosterYK,"src/d/actor/d_a_e_yk.cpp#damage_check",e_yk_class);
ROSTER_HIT(RosterRDB,"src/d/actor/d_a_e_rdb.cpp#damage_check",e_rdb_class);
ROSTER_HIT(RosterVA,"daE_VA_c::damage_check",daE_VA_c);
ROSTER_HIT(RosterYMB,"daE_YMB_c::damage_check",daE_YMB_c);
ROSTER_HIT(RosterPZ,"daE_PZ_c::damage_check",daE_PZ_c);
ROSTER_HIT(RosterTH,"src/d/actor/d_a_e_th.cpp#damage_check",e_th_class);
ROSTER_HIT(RosterGND,"src/d/actor/d_a_b_gnd.cpp#damage_check",b_gnd_class);
ROSTER_HIT(RosterYC,"src/d/actor/d_a_e_yc.cpp#damage_check",e_yc_class);
ROSTER_HIT(RosterHeadDamage,"daB_DS_c::mBattle2_spinner_damage_check",daB_DS_c);
#undef ROSTER_HIT
DEFINE_HOOK_SYMBOL("cCcS::Set",void(cCcS*,cCcD_Obj*),RosterTargets);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_sf.cpp#action",void(e_sf_class*),RosterSFAction);
DEFINE_HOOK_SYMBOL("daE_GI_c::setCryStop",bool(daE_GI_c*),RosterGibdoCry);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_mm_mt.cpp#action",void(e_mm_mt_class*),RosterShell);
DEFINE_HOOK_SYMBOL("daE_TT_c::checkPlayerSearch",bool(daE_TT_c*),RosterTTSearch);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_bu.cpp#pl_check",int(e_bu_class*,float,s16),RosterBubbleSearch);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_dd.cpp#pl_check",int(e_dd_class*,float,s16),RosterDDSearch);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_rdy.cpp#pl_check",int(e_rdy_class*,float,s16),RosterRDYSearch);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_rdy.cpp#action",void(e_rdy_class*),RosterRDYAction);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_yc.cpp#action",void(e_yc_class*),RosterYCAction);
DEFINE_HOOK_SYMBOL("daB_DS_c::mBattle2Action",void(daB_DS_c*),RosterHeadAction);
DEFINE_HOOK_SYMBOL("daE_ZH_c::action",void(daE_ZH_c*),RosterHandAction);
DEFINE_HOOK_SYMBOL("daE_ZH_c::cc_set",void(daE_ZH_c*),RosterHandCollision);
DEFINE_HOOK_SYMBOL("daB_DS_c::create",cPhs_Step(daB_DS_c*),RosterHeadCreate);
DEFINE_HOOK(static_cast<s32 (*)()>(&dComIfGs_isStageBossEnemy),RosterHeadCleared);
DEFINE_HOOK_SYMBOL("dComIfGs_onZoneSwitch",void(int,int),RosterHeadZone);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_yk.cpp#pl_check",int(e_yk_class*,float,s16),RosterYKSearch);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_s1.cpp#pl_check",int(e_s1_class*,float),RosterS1Search);

DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_rdb.cpp#action",void(e_rdb_class*),BossRDBAction);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_th.cpp#action",void(e_th_class*),BossTHAction);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_b_gnd.cpp#daB_GND_Execute",int(b_gnd_class*),BossGNDExecute);
DEFINE_HOOK_SYMBOL("daE_VA_c::action",void(daE_VA_c*),BossVAAction);
DEFINE_HOOK_SYMBOL("daE_YMB_c::action",void(daE_YMB_c*),BossYMBAction);
DEFINE_HOOK_SYMBOL("daE_YMB_c::checkWaterPos",void(daE_YMB_c*),BossYMBWater);
DEFINE_HOOK_SYMBOL("daE_YMB_c::checkGroundPos",void(daE_YMB_c*),BossYMBGround);
DEFINE_HOOK_SYMBOL("daE_YMB_c::setCameraSwitch",void(daE_YMB_c*),BossYMBCamera);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_hzelda.cpp#action",void(e_hzelda_class*),BossZeldaAction);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_hzelda.cpp#e_hzelda_attack_b",void(e_hzelda_class*),BossZeldaTriangle);
DEFINE_HOOK_SYMBOL("daE_PZ_c::action",void(daE_PZ_c*),BossPZAction);
DEFINE_HOOK_SYMBOL("daE_PZ_c::executeAttack",void(daE_PZ_c*),BossPZAttack);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_rdb.cpp#demo_camera",void(e_rdb_class*),BossRDBCamera);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_th.cpp#demo_camera",void(e_th_class*),BossTHCamera);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_hzelda.cpp#demo_camera",void(e_hzelda_class*),BossZeldaCamera);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_b_gnd.cpp#demo_camera",void(b_gnd_class*),BossGNDCamera);
DEFINE_HOOK_SYMBOL("dComIfGs_onSwitch",void(int,int),BossSwitchOn);
DEFINE_HOOK_SYMBOL("dComIfGs_offSwitch",void(int,int),BossSwitchOff);
DEFINE_HOOK_SYMBOL("dComIfGs_onOneZoneSwitch",void(int,int),BossZoneOn);
DEFINE_HOOK_SYMBOL("dComIfGs_offOneZoneSwitch",void(int,int),BossZoneOff);
DEFINE_HOOK_SYMBOL("dComIfGs_onSaveDunSwitch",void(int),BossSaveSwitch);
DEFINE_HOOK_SYMBOL("dKy_custom_colset",void(u8,u8,float),BossLighting);
DEFINE_HOOK(static_cast<fpc_ProcID (*)(s16,u32,const cXyz*,int,const csXyz*,const cXyz*,s8,u32,u8)>(&fopAcM_create),BossPZBulletCreate);
DEFINE_HOOK_SYMBOL("daB_DS_c::mBattle2MoveFSet",bool(daB_DS_c*),RosterHeadMove);
DEFINE_HOOK_SYMBOL("daB_DS_c::mFuwafuwaSet",void(daB_DS_c*,bool),RosterHeadHover);
DEFINE_HOOK_SYMBOL("daB_DS_c::mCreateTrap",void(daB_DS_c*,bool),RosterHeadTrap);
DEFINE_HOOK_SYMBOL("daB_TN_c::executeChangeDemo",void(daB_TN_c*),BossTNChange);
DEFINE_HOOK_SYMBOL("daB_TN_c::checkMoveArea",int(daB_TN_c*),BossTNArea);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_th.cpp#e_th_spin_B",void(e_th_class*),BossTHSpin);

DEFINE_HOOK_SYMBOL("daB_TN_c::calcSwordMoveA",void(daB_TN_c*),BossTNThrow);
DEFINE_HOOK_SYMBOL("daB_TN_c::cc_set",void(daB_TN_c*),BossTNThrowCollision);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_rdy.cpp#e_rdy_yc_ride",void(e_rdy_class*),BossCarrierRide);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_b_gnd.cpp#b_gnd_g_attack",BOOL(b_gnd_class*),BossGNDCharge);

DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_mf.cpp#action",void(e_mf_class*),RosterMFAction);

DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_wb.cpp#action",void(e_wb_class*),BossBullboAction);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_rd.cpp#daE_RD_Execute",int(e_rd_class*),BossBullboRider);
