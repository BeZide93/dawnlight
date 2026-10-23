#include "fierce_deity_equipment_scope.hpp"
#include "platform_compat.hpp"
#include "Z2AudioLib/Z2AudioMgr.h"
#include "survivors.hpp"
#include "survivors_enemy_hud.hpp"
#include "survivors_integrity.hpp"
#include "d/d_door_param2.h"
#include "config.hpp"
#include "heroes_shade_encounter.hpp"
#include "aim_hooks.hpp"
#include "bullet_time.hpp"
#include "combat_meter.hpp"
#include "c/c_damagereaction.h"
#include "process_actor.hpp"
#include "actor_slot_index.hpp"
#include "survivors_rules.hpp"
#include "survivors_hud_layout.hpp"
#include "survivors_multistrike.hpp"
#include "survivors_enemy_hits.hpp"
#include "survivors_boss_tactics.hpp"
#include "survivors_field_boss_rules.hpp"
#include "d/d_msg_scrn_explain.h"
#include "d/d_msg_scrn_3select.h"
#include "d/d_lib.h"
#include "d/d_cc_uty.h"
#include "dusk/menu_pointer.h"
#include "survivors_projectiles.hpp"
#include "survivors_roster.hpp"
#include "survivors_reachable.hpp"
#include "survivors_items.hpp"
#include "survivors_maps.hpp"
#include "survivors_barriers.hpp"
#include "ganondorf_barrier.hpp"
#include "survivors_locomotion.hpp"
#include "dual_wield_animation.hpp"
#include "enemy_spawner.hpp"
#include "bow_fire_resource.hpp"
#include "save_state.hpp"
#include "gale_counter.hpp"
#include "jump_abilities.hpp"
#include "m_Do/m_Do_MemCard.h"
#include "service_imports.hpp"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/flow.hpp"
#include "dusk/ui/nav_types.hpp"
#include "d/d_menu_save.h"
#include "d/d_file_sel_info.h"
#include "d/d_menu_window.h"
#include "d/d_meter_map.h"
#include "d/d_menu_ring.h"
#include "d/d_menu_collect.h"
#include "d/d_pane_class.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "d/d_menu_item_explain.h"
#include "d/d_map.h"
#include "d/d_camera.h"
#include "d/d_msg_object.h"
#include "d/d_msg_class.h"
#include "JSystem/JMessage/control.h"
#include "JSystem/JParticle/JPAResource.h"
#include "JSystem/JParticle/JPABaseShape.h"
#include "SSystem/SComponent/c_math.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_midna.h"
#include "d/actor/d_a_demo_item.h"
#include "d/actor/d_a_e_ba.h"
#include "d/actor/d_a_e_oc.h"
#include "d/actor/d_a_obj_carry.h"
#include "d/actor/d_a_obj_bosswarp.h"
#include "d/actor/d_a_obj_item.h"
#include "d/actor/d_a_obj_life_container.h"
#include "d/d_bg_s_wtr_chk.h"
#include "m_Do/m_Do_lib.h"
#include "d/actor/d_a_e_ms.h"
#include "d/actor/d_a_e_rd.h"
#include "d/actor/d_a_e_wb.h"
#include "d/actor/d_a_e_dd.h"
#include "d/actor/d_a_e_sh.h"
#include "d/actor/d_a_e_rdy.h"
#include "d/actor/d_a_e_s1.h"
#include "d/actor/d_a_e_yk.h"
#include "d/actor/d_a_e_yc.h"
#include "d/actor/d_a_b_ds.h"
#include "d/actor/d_a_e_zh.h"
#include "d/actor/d_a_e_rdb.h"
#include "d/actor/d_a_e_vt.h"
#include "d/actor/d_a_e_ymb.h"
#include "d/actor/d_a_e_pz.h"
#include "d/actor/d_a_e_th.h"
#include "d/actor/d_a_e_th_ball.h"
#include "d/actor/d_a_e_hzelda.h"
#include "d/actor/d_a_b_gnd.h"
#include "d/actor/d_a_mant.h"

#include "d/actor/d_a_e_tt.h"
#include "d/actor/d_a_e_dn.h"
#include "d/actor/d_a_e_mm.h"
#include "d/actor/d_a_e_mm_mt.h"
#include "d/actor/d_a_e_ge.h"
#include "d/actor/d_a_e_kr.h"
#include "d/actor/d_a_e_cr.h"
#include "d/actor/d_a_e_bs.h"
#include "d/actor/d_a_e_gi.h"
#include "d/actor/d_a_e_sf.h"
#include "d/actor/d_a_e_fs.h"
#include "d/actor/d_a_e_bu.h"
#include "d/actor/d_a_e_sm2.h"
#include "d/actor/d_a_e_mf.h"
#include "d/actor/d_a_b_gg.h"
#include "d/actor/d_a_b_tn.h"

#include "d/d_com_inf_game.h"
#include "d/d_path.h"
#include "d/actor/d_a_suspend.h"
#include "d/actor/d_a_obj_lv8Lift.h"
#include "d/d_debug_viewer.h"
#include "d/d_gameover.h"
#include "res/Object/E_fm.h"
#include "d/d_item.h"
#include "d/d_meter2_draw.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "d/d_meter2_info.h"
#include "d/d_s_play.h"
#include "f_op/f_op_actor_mng.h"
#include "f_op/f_op_overlap_mng.h"
#include "f_pc/f_pc_method.h"
#include "f_pc/f_pc_create_req.h"
#include "f_pc/f_pc_create_tag.h"
#include "f_pc/f_pc_layer.h"
#include "f_pc/f_pc_leaf.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_graphic.h"
#include "m_Do/m_Do_Reset.h"
#include "m_Do/m_Do_controller_pad.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "JSystem/J2DGraph/J2DPrint.h"
#include "JSystem/JParticle/JPAEmitter.h"
#include "JSystem/JKernel/JKRSolidHeap.h"
#include "JSystem/J3DGraphAnimator/J3DModel.h"
#include "JSystem/J3DGraphAnimator/J3DJoint.h"
#include "JSystem/J3DGraphBase/J3DMaterial.h"
#include "JSystem/J3DGraphBase/J3DDrawBuffer.h"
#include "res/Object/AlAnm.h"
#include "d/d_particle_name.h"
#include "d/d_bg_w.h"
#include "f_pc/f_pc_profile.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "d/actor/d_a_bg.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <memory>
#include <span>
#include <optional>
#include <unordered_map>

namespace dawnlight {
namespace {
using namespace survivors;
#include "survivors_roster_native.inc"
constexpr char kMode[] = "hyrule_survivors";
unsigned s_map = 0, s_requestedMap = 0;
const Arena& arena() { return arenas[s_map]; }
// Keep the original key so the versioned payload can migrate existing saves.
constexpr char kBlob[] = "hyrule-survivors-records-v1";
constexpr ActorId kNone = fpcM_ERROR_PROCESS_ID_e;
DEFINE_HOOK(&daAlink_c::execute, SurvivorsPlayer);
// Symbol form avoids MSVC constinit failure for this multiply-inherited class.
DEFINE_HOOK_SYMBOL("dBgS_Acch::CrrPos", void(dBgS_Acch*, dBgS&), SurvivorsEnemySpeed);
DEFINE_HOOK(&daAlink_c::setDamagePoint, SurvivorsEnemyDamage);
DEFINE_HOOK(&daAlink_c::commonBottleDrink, SurvivorsPotionDrink);
DEFINE_HOOK(&dComIfGp_setItemLifeCount, SurvivorsPotionHeal);
DEFINE_HOOK(&dComIfGp_setItemRupeeCount, SurvivorsFateRupees);
DEFINE_HOOK(&daAlink_c::getMainBckData, SurvivorsFateSprint);
DEFINE_HOOK(&JPABaseEmitter::deleteAllParticle, SurvivorsFairyParticles);
DEFINE_HOOK(&dCamera_c::Run, SurvivorsCamera);
DEFINE_HOOK(&daAlink_c::orderZTalk, SurvivorsMidnaOrder);
DEFINE_HOOK(&dMsgFlow_c::getInitNodeIndex, SurvivorsMidnaFlow);
DEFINE_HOOK(&daAlink_c::checkSceneChange, SurvivorsArenaExit);
DEFINE_HOOK(&daAlink_c::checkRestartRoom, SurvivorsArenaRestart);
DEFINE_HOOK_SYMBOL("src/d/d_stage.cpp#dStage_actorCreate", void(stage_actor_data_class*, fopAcM_prm_class*, size_t), SurvivorsArenaActor);
DEFINE_HOOK(static_cast<void(*)(fopAc_ac_c*)>(&daSus_c::check), SurvivorsPalaceFerrySuspend);
DEFINE_HOOK(&daL8Lift_c::moveLift, SurvivorsPalaceFerryMove);
DEFINE_HOOK(&daAlink_c::checkDamageAction, SurvivorsBlock);
DEFINE_HOOK(&daAlink_c::setShieldGuard, SurvivorsGuard);
DEFINE_HOOK_SYMBOL("cCcS::SetCoCommonHitInf",void(cCcS*,cCcD_Obj*,cXyz*,cCcD_Obj*,cXyz*,f32),SurvivorsEnemyContact);
DEFINE_HOOK(&daE_OC_c::damage_check, SurvivorsBokoblinHit);
DEFINE_HOOK_SYMBOL("cCcS::SetAtTgCommonHitInf",void(cCcS*,cCcD_Obj*,cCcD_Obj*,cXyz*),SurvivorsConcurrentHit);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_rd.cpp#damage_check", void(e_rd_class*), SurvivorsBulblinHit);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_rd.cpp#pl_check", BOOL(e_rd_class*, f32, s16), SurvivorsBulblinAggro);
DEFINE_HOOK(&fopAcM_createChild, SurvivorsEnemyArrow);
DEFINE_HOOK(&daE_OC_c::cc_set, SurvivorsBokoblinTargets);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_ms.cpp#e_ms_damage", void(e_ms_class*), SurvivorsRatRecovery);
DEFINE_HOOK(&daE_OC_c::searchPlayer, SurvivorsBokoblinAggro);
DEFINE_HOOK(&daE_OC_c::executeWait, SurvivorsBokoblinWait);
DEFINE_HOOK(&fopAcM_searchActorDistance, SurvivorsEliteDistance);
DEFINE_HOOK(&fopAcM_searchActorAngleY, SurvivorsBossAim);
DEFINE_HOOK(&daObjCarry_c::obj_break, SurvivorsPotBreak);
DEFINE_HOOK(&daItem_c::itemGetNextExecute, SurvivorsKeyPickup);
DEFINE_HOOK(&renderingAmap_c::afterDrawPath, SurvivorsEliteMinimap);
DEFINE_HOOK(&dFile_info_c::setSaveData, SurvivorsSaveSlotInfo);
DEFINE_HOOK(&dDlst_FileInfo_c::draw, SurvivorsSaveSlotDraw);
// SDK v2.0.0's typed trampoline cannot marshal const references. The native
// const Vec& uses the same pointer ABI; preserve it through a named hook.
DEFINE_HOOK_SYMBOL("renderingPlusDoorAndCursor_c::drawCursor",
    void(renderingPlusDoorAndCursor_c*, const Vec*, s16, int, f32), SurvivorsMinimapCursor);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_ba.cpp#pl_check", BOOL(e_ba_class*, f32, s16), SurvivorsKeeseAggro);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_ms.cpp#pl_check", int(e_ms_class*, f32), SurvivorsRatAggro);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_ba.cpp#damage_check", void(e_ba_class*), SurvivorsKeeseHit);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_ms.cpp#damage_check", void(e_ms_class*), SurvivorsRatHit);
DEFINE_HOOK(&daAlink_c::setMatrix, SurvivorsRangedAim);
DEFINE_HOOK(&daAlink_c::checkUpperItemActionBow, SurvivorsBowAction);
DEFINE_HOOK(&daAlink_c::checkNextActionBow, SurvivorsBowNext);
DEFINE_HOOK(&daAlink_c::checkUpperItemActionIronBall, SurvivorsBallAction);
DEFINE_HOOK(&daAlink_c::checkNextActionIronBall, SurvivorsBallNext);
DEFINE_HOOK(&dMenu_Collect2D_c::isSkillIconVisible, SurvivorsScrollVisible);
DEFINE_HOOK(&dMenu_Collect2D_c::screenSet, SurvivorsGearScreen);
DEFINE_HOOK(&dMenu_Collect2D_c::_draw, SurvivorsGearDraw);
DEFINE_HOOK(&dMenu_Collect2D_c::wait_proc, SurvivorsCollectionWait);
DEFINE_HOOK(&dMenu_Collect2D_c::pointerActivateCurrent, SurvivorsCollectionClick);
DEFINE_HOOK(&dMenu_Collect2D_c::_delete, SurvivorsCollectionDelete);
DEFINE_HOOK(&dMeter2Info_c::readItemTexture, SurvivorsScrollTexture);
DEFINE_HOOK(&dMenu_Ring_c::setActiveCursor, SurvivorsRingCursor);
DEFINE_HOOK(&dMenu_Ring_c::setItem, SurvivorsRingAssign);
DEFINE_HOOK(&dMenu_Ring_c::setMixItem, SurvivorsRingMix);
// The two status setters begin with unrelocatable short jumps in Windows
// v2.0.3. Guard the shared item-cache consumer and Link's execution instead.
DEFINE_HOOK(&dComIfGp_setSelectItem, SurvivorsBind);
DEFINE_HOOK(&dMenu_ItemExplain_c::openExplain, SurvivorsExplain);
DEFINE_HOOK(&JMessage::TControl::update, SurvivorsTreasureText);
DEFINE_HOOK(&daAlink_c::checkUpperGrabItemThrow, SurvivorsBombThrow);
DEFINE_HOOK(&daAlink_c::cancelUpperItemReadyAnime, SurvivorsRangedCancel);
DEFINE_HOOK(&daAlink_c::procCoGetItemInit, SurvivorsGetItem);
DEFINE_HOOK(&fpcLf_Draw, SurvivorsTreasureDraw);
DEFINE_HOOK(&daAlink_c::draw, SurvivorsDraw);
DEFINE_HOOK(&daAlink_c::procCoDeadInit, SurvivorsDeath);
DEFINE_HOOK(&dGameover_c::_execute, SurvivorsGameOver);
DEFINE_HOOK(&daAlink_c::checkDoCutAction, SurvivorsManualAttack);
// Select the non-const overload explicitly. MSVC crashes while materializing
// its cast member pointer; the qualified display name also matches the const
// virtual query, so use the exact platform ABI symbol for this one hook.
#ifdef _WIN32
DEFINE_HOOK_SYMBOL("?checkCutTurnCharge@daAlink_c@@QEAAXXZ", void(daAlink_c*), SurvivorsManualCharge);
#else
DEFINE_HOOK_SYMBOL("_ZN9daAlink_c18checkCutTurnChargeEv", void(daAlink_c*), SurvivorsManualCharge);
#endif
DEFINE_HOOK(&daAlink_c::procCutTurnChargeInit, SurvivorsChargeInit);
DEFINE_HOOK(&daAlink_c::procCutTurnMoveInit, SurvivorsChargeMoveInit);
DEFINE_HOOK(&daAlink_c::procCutTurnInit, SurvivorsNativeSpin);
DEFINE_HOOK(&daAlink_c::setCutJumpSpeed, SurvivorsJumpTarget);
DEFINE_HOOK(&daAlink_c::checkCutAction, SurvivorsSwordAttack);
DEFINE_HOOK(&daAlink_c::setSwordAtCollision, SurvivorsSwordCollision);
DEFINE_HOOK(&cBgS::LineCross, SurvivorsEnemyNavigationLine);
DEFINE_HOOK(&daAlink_c::transAnimeProc, SurvivorsMovement);
DEFINE_HOOK(&daAlink_c::modelCalc, SurvivorsLocomotionModel);
DEFINE_HOOK(&daAlink_c::posMove, SurvivorsPosition);
DEFINE_HOOK(&daAlink_c::checkAutoJumpAction, SurvivorsLedgeJump);
DEFINE_HOOK(&daAlink_c::procCutTurn, SurvivorsSpinMove);
DEFINE_HOOK(&daAlink_c::setCutTurnEffect, SurvivorsSpinEffect);
DEFINE_HOOK(&daAlink_c::procCutLargeJump, SurvivorsJumpStrike);
DEFINE_HOOK(&daAlink_c::procCutLargeJumpLand, SurvivorsJumpStrikeLand);
DEFINE_HOOK(&daAlink_c::setCutLargeJumpLandEffect, SurvivorsJumpStrikeEffect);
DEFINE_HOOK(&daAlink_c::procCutNormal, SurvivorsNormalMove);
DEFINE_HOOK(&daAlink_c::procCutFinish, SurvivorsFinishMove);
DEFINE_HOOK(&daAlink_c::checkCutLargeTurnState, SurvivorsLargeSpin);
DEFINE_HOOK(&fopAcM_orderDoorEvent, SurvivorsDoor);
DEFINE_HOOK(&at_power_check, SurvivorsPower);
DEFINE_HOOK(&cc_at_check, SurvivorsHitMaterial);
DEFINE_HOOK(&daAlink_c::checkLightSwordMtrl, SurvivorsLightSword);
DEFINE_HOOK(&dMeter2Draw_c::draw, SurvivorsHud);
DEFINE_HOOK(&dComIfGs_gameStart, SurvivorsGameStart);
DEFINE_HOOK(&mDoCPd_c::read, SurvivorsPad);
DEFINE_HOOK(&daObjBossWarp_c::execute, SurvivorsHordePortal);
DEFINE_HOOK(&dCamera_c::setMapToolData, SurvivorsCameraType);
DEFINE_HOOK(&dMenu_save_c::_open, SurvivorsSaveOpen);
DEFINE_HOOK(&dMenu_save_c::_delete, SurvivorsSaveDelete);
DEFINE_HOOK(&dMsgScrn3Select_c::pointerMove, SurvivorsSavePointer);
DEFINE_HOOK(&dMeterMap_c::meter_map_move, SurvivorsSaveMapMove);
DEFINE_HOOK(&dMeterMap_c::keyCheck, SurvivorsSaveMapKeys);
DEFINE_HOOK(&dMw_c::key_wait_proc, SurvivorsSaveMenuKeys);
DEFINE_HOOK(&dComIfG_changeOpeningScene, SurvivorsOpening);
DEFINE_HOOK(&fpcMtd_Method, SurvivorsProcess);
DEFINE_HOOK(&fopAcM_createItemFromTable, SurvivorsDrops);
DEFINE_HOOK(&fopAcM_createItem, SurvivorsDirectDrops);
DEFINE_HOOK(static_cast<void (*)(const char*, s16, s8, s8, f32, u32, int, s8, s16, int, int)>(
    &dComIfGp_setNextStage), SurvivorsStage);

bool s_active = false, s_registered = false, s_loaded = false, s_restart = false;
// Selecting a mode starts the title flow, not a game. A slot callback prepares
// the save; only gameStart commits it after names and optional TV settings.
bool s_saveReady = false, s_gameStarted = false;
bool s_resultsStored = false, s_storeFailed = false, s_saveRequested = false;
unsigned s_saveStartTicks = 0;
Run s_run;
RunOptions s_options;
#include "survivors_hud_config.inc"
UiDialogHandle s_debugNotice=0;
void close_debug_notice() {
    const auto handle=s_debugNotice;s_debugNotice=0;
    if(handle)svc_ui->dialog_close(mod_ctx,handle);
}
enum class HubMenu { None, Talents, Run };
bool s_hub=true, s_hubDeparture=false, s_hubDoorStarted=false, s_arrivingRun=false;
bool s_hubSaveRequested=false, s_allowHubDoor=false;
HubMenu s_hubMenu=HubMenu::None;
ActorId s_hubDoor=kNone;
unsigned s_hubDoorTicks=0;
u16 s_hubDoorPriority=0, s_hubDoorFlags=0;
void open_window();
void load_option_config();
void store_option_config();
bool settings_editable();
void hub_menu_closed() {
    if(s_hubMenu==HubMenu::None)return;
    s_hubMenu=HubMenu::None;
    if(s_active&&s_gameStarted&&s_hub&&s_run.phase==Phase::Ready)s_hubSaveRequested=true;
}
Records s_records;
bool map_available(unsigned map) {
    return map<arenas.size() && (s_records.map_unlocked(map) || (debug_enabled && debug_session_enabled && s_options.debug.unlock_all_maps));
}
bool s_progressSaving=false, s_progressSaved=false;
#include "survivors_gear_runtime.inc"
UiWindowHandle s_refreshChoices=0;
// Opaque host documents: no private RmlUi/Window layouts are copied.
#if !DAWNLIGHT_IOS
DEFINE_HOOK_SYMBOL("dusk::ui::Window::handle_nav_command",
                   bool(void*, void*, dusk::ui::NavCommand), SurvivorsMenuWindow);
DEFINE_HOOK_SYMBOL("dusk::ui::Modal::handle_nav_command",
                   bool(void*, void*, dusk::ui::NavCommand), SurvivorsMenuModal);
#endif
void* s_menuDocument = nullptr;
void* s_talentDocument = nullptr;
void remember_menu_document(bool talent);
UiWindowHandle s_window = 0, s_powerupsWindow = 0, s_hubClosingWindow=0;
UiDialogHandle s_talentDialog=0;
bool s_powerupsRequested = false;
dMenu_Collect2D_c* s_powerupsMenu = nullptr;
void close_powerup_window();
void close_score_windows();
void close_high_score(ModContext*,void*);
void request_delete_score(ModContext*,void*);
ActorId s_player = kNone;
fpc_ProcID s_savePrompt = kNone;
bool s_victory=false, s_deathPending=false, s_deathDismiss=false;
unsigned s_victoryTicks=0;
fpc_ProcID s_deathGameOver=kNone;
void begin_victory(const cXyz&);
void begin_defeat();
void reset_ending();
bool update_ending(daAlink_c*);
void update_victory();
void draw_victory_wave();
void prepare_victory_wave();
unsigned s_spawnCooldown = 0, s_spawnFailures = 0, s_warmup = 0;
unsigned s_midnaRequestFrames = 0;
bool s_midnaTalking = false, s_midnaEndPending = false, s_midnaRestartPending = false;
bool midna_dialog_busy() { return s_midnaRequestFrames || s_midnaTalking || s_midnaEndPending || s_midnaRestartPending; }
void reset_midna_dialog() { s_midnaRequestFrames=0; s_midnaTalking=s_midnaEndPending=s_midnaRestartPending=false; }
enum class SwordAction { None, Normal, Spin, Jump };
SwordAction s_swordAction = SwordAction::None;
unsigned s_meleeCooldown = 0;
MultiStrikeState s_multiStrike;
void clear_lightning();
void update_lightning(daAlink_c*);
void draw_lightning();
void trigger_lightning(fopAc_ac_c*);
unsigned s_comboStep = 0, s_comboTimeout = 0;
bool s_autoSwordCall = false, s_trackMelee = false;
ActorId s_meleeTarget = kNone;
bool s_wavePending = false, s_allowDrop = false;
bool s_waveManual = false, s_manualSwing = false;
std::array<unsigned, unsigned(Upgrade::Rupees)> s_cooldowns{};
void clear_loot();
void clear_locomotion();
void clear_attacks();
void clear_sword_aim();
void restore_sword_aim_camera(dCamera_c*);
bool update_sword_aim_camera(dCamera_c*);
void resolve_manual_projectiles(dCamera_c*);
void draw_hub_labels();
void interrupt_auto_actions(daAlink_c*);
void finish(bool won);
void continue_endless(ModContext*,void*);
void replay(ModContext*, void*);
void restart_run(ModContext*, void*);
void clear_enemy_effects();
bool effect_collider(const cCcD_Obj*);
bool native_sword_collider(const cCcD_Obj*);
void clear_jump_effects();
void new_melee_swing();
void cancel_treasure_demo();
int potion_slot();
unsigned potion_capacity();
unsigned chest_keys();
void set_chest_keys(unsigned);
void cancel_ranged();
bool prepare_treasure_message(ChestReward, const Run&, const Run&, Rarity);
unsigned treasure_message_id();
void sync_survivors_inventory();
bool queue_ranged(daAlink_c*, Upgrade);
struct Enemy;
void queue_knockback(Enemy&, fopAc_ac_c*, bool, DamageSource = DamageSource::Other);
bool ranged_pose_active();
bool boss_blocks_from(const Enemy&,fopAc_ac_c*,const cXyz&);
void stop_survivors_music();
bool loot_floor_position(cXyz&);
void update_knockback(Enemy&, fopAc_ac_c*);
void initialize_extended_enemy(Enemy&,fopAc_ac_c*);
void drop_elite_chest(Enemy&, fopAc_ac_c*);
std::array<unsigned, 3> s_choices{0, 1, 2};
struct ProcessFrame { void* args; ActorId id; bool executed=false; };
std::vector<ProcessFrame> s_processFrames;

struct Enemy {
    ActorId id = kNone;
    ProfileName profile = 0;
    bool initialized = false, credited = false, retiring = false, elite = false;
    unsigned age = 0, endlessMinute = 0;
    int eliteSlot = -1;
    bool dormant = false, idleAnimation = false;
    cXyz lootPosition{}, spawnPosition{};
    unsigned species=0, hitCooldown=0, stagger=0;
    cCcD_Obj* childHit=nullptr;
    bool swordKill=false;
    bool encounterReady=false;
    unsigned encounterTick=0;
    unsigned specialCooldown=0, summonCooldown=240, throwLife=0;
    bool extraAttackReady=false;
    dCcD_Stts extraStatus{};
    dCcD_Sph extraAttack{};
    cXyz encounterTarget{};
    unsigned arrowVolley=0;
    std::array<ActorId,2> bullboRiders{kNone,kNone};
    std::array<unsigned,3> bullboTargetsPending{};
    AeralfosCycle aeralfosCycle;
    KingWaveClock kingWaveClock;
};
static_assert(enemy_capacity <= 512, "Update enemy HUD slot capacity with the roster");
std::array<Enemy, enemy_capacity> s_enemies;
ActorSlotIndex<ActorId> s_enemyIndex;
struct Orb { cXyz position{}; unsigned value = 0; bool summoned = false, merged = false; };
std::array<Orb, orb_capacity> s_orbs;
struct Attack {
    bool active = false;
    unsigned life = 0, weapon = 0, age = 0;
    bool returning = false, exploding = false, greatSpin = false;
    bool hurricane=false, bombArrow=false, detonated=false, aimPending=false, manualFlight=false;
    float aimSpread=0;
    bool chainSpin=false;
    unsigned chainSpinTick=0;
    float chainSpinRadius=0,chainSpinAngle=0;
    J3DModel* bombModel=nullptr;
    JKRSolidHeap* bombHeap=nullptr;
    float effectBaseRadius = 260.f;
    ActorId target = kNone;
    unsigned hops = 0, visitedCount = 0;
    std::array<ActorId, 13> visited{};
    std::array<ActorId, enemy_capacity> ballVictims{};
    unsigned ballVictimCount = 0, arrowTargets = 1;
    JKRSolidHeap* modelHeap = nullptr;
    J3DModel* model = nullptr;
    J3DModel* wind = nullptr;
    unsigned bowVolleySlot = 20;
    uint64_t bowVolley = 0;
    float travel = 0, limit = 0;
    cXyz position{}, velocity{};
    float radius = 0;
    dCcD_Stts status{};
    dCcD_Sph collider{};
    std::array<u32, 9> emitters{};
};
// Stable addresses: collision server retains pointers until its next frame.
std::deque<Attack> s_attacks(initial_projectile_slots);
const dCcD_SrcSph kAttackSource = {
    {{0, {{AT_TYPE_NORMAL_SWORD, 3, 0x1a}, {0, 0}, 0}},
     {dCcD_SE_SWORD, 3, dCcG_At_Spl_UNK_1, dCcD_MTRL_NONE, {1}},
     {dCcD_SE_NONE, 0, 0, dCcD_MTRL_NONE, {0}}, {0}},
    {{{0, 0, 0}, 50}},
};

bool in_arena() {
    return s_active && s_gameStarted && arena_matches(s_map, dComIfGp_getStartStageName(), dComIfGp_getStartStageRoomNo());
}
#include "survivors_save_modal.inc"
#include "survivors_save_pointer.inc"
bool ui_visible() {
    if(save_modal_active())return true;
    bool value = false;
    svc_ui->is_any_document_visible(mod_ctx, &value);
    return value;
}
bool transitioning() { return dComIfGp_isEnableNextStage() || fopOvlpM_IsPeek(); }
bool playable() {
    return in_arena() && !midna_dialog_busy() && !transitioning() && !dComIfGp_isPauseFlag() &&
        !dScnPly_c::isPause() && !dComIfGp_event_runCheck() && !ui_visible() &&
        dMeter2Info_getGameOverType() == 0;
}
fopAc_ac_c* actor_by_id(ActorId id) {
    fopAc_ac_c* actor = nullptr;
    if (id != kNone && !fpcM_IsCreating(id)) fopAcM_SearchByID(id, &actor);
    return actor;
}
Enemy* tracked(ActorId id) {
    return id == kNone ? nullptr : s_enemyIndex.find(s_enemies, id);
}
#include "survivors_enemy_types.inc"
#include "survivors_enemy_balance.inc"
#include "survivors_boss_tactics_state.inc"
#include "survivors_enemy_arrows.inc"
#include "survivors_enemy_hits.inc"
#include "survivors_zelda_orbs.inc"
#include "survivors_king_waves.inc"
#include "survivors_collision.inc"
#include "survivors_elite_positions.inc"
#include "survivors_palace_ferry.inc"
void clear_reachable_floor();
#include "survivors_maps.inc"
#include "survivors_extended_ai.inc"
#include "survivors_aggro.inc"
#include "survivors_navigation.inc"
#include "survivors_enemy_facing.inc"
DamageAmounts hit_damage_sources(const cCcD_Obj* collider);
bool sword_kill_hit(const cCcD_Obj* collider);
#include "survivors_feedback.inc"
#include "survivors_ground.inc"
#include "survivors_reachable.inc"
#include "survivors_pickups.inc"
#include "survivors_key_visual.inc"
#include "survivors_player_damage.inc"
#include "survivors_options.inc"
#include "survivors_controls.inc"
#include "survivors_potions.inc"
void stop_attack(Attack& a) {
    for (auto& id : a.emitters) {
        if (id) {
            if (auto* emitter = dComIfGp_particle_getEmitter(id)) {
                emitter->stopCreateParticle();
                emitter->quitImmortalEmitter();
                emitter->becomeInvalidEmitter();
            }
            id = 0;
        }
    }
    a.active = false; a.life = 0;
    a.collider.OffAtSetBit(); a.collider.ResetAtHit();
}
void clear_attacks() {
    s_multiStrike={};clear_lightning();
    clear_sword_aim();
    clear_jump_effects();
    s_meleeTarget = kNone; s_trackMelee = false;
    cancel_ranged();
    for (auto& a : s_attacks) stop_attack(a);
}
void close_window() {
    close_hud_editor();
    close_debug_notice();
    close_score_windows();
    s_menuDocument = s_talentDocument = nullptr;
    const auto dialog=s_talentDialog;s_talentDialog=0;
    if(dialog)svc_ui->dialog_close(mod_ctx,dialog);
    const auto handle = s_window; s_window = 0;
    if (handle) {
        if(s_hubDeparture)s_hubClosingWindow=handle;
        if(s_refreshChoices!=handle)hub_menu_closed();
        svc_ui->window_close(mod_ctx, handle);
    }
}
#include "survivors_menu_handoff.inc"
void retire_horde_portals();
void cleanup_horde_portals();
void retire_pots();
void cleanup_pots();
void detach_pot_attacks();
#include "survivors_enemy_archives.inc"
#include "survivors_boss_ring_terrain.inc"
#include "survivors_boss_ring.inc"
void retire_enemies() {
    release_enemy_archives();
    retire_boss_ring();
    clear_zelda_light_orbs();clear_king_waves();
    retire_horde_portals();
    retire_pots(); retire_pickups(); retire_enemy_arrows();
    clear_enemy_effects();
    for (auto& e : s_enemies) if (e.id != kNone) e.retiring = true;
}
void cleanup_enemies() {
    cleanup_palace_ferries();
    cleanup_horde_portals();
    cleanup_boss_ring();
    cleanup_enemy_arrows();
    for (auto& e : s_enemies) {
        if (e.id == kNone) continue;
        auto* actor = actor_by_id(e.id);
        if(e.retiring || e.credited || !actor)enemy_hud::forget(&e-s_enemies.data());
        if(e.retiring||e.credited||(actor&&actor->health<=0)){clear_zelda_light_orbs(e.id);clear_king_waves(e.id);}
        if (!actor && !fpcM_IsCreating(e.id)) {
            clear_zelda_light_orbs(e.id);clear_king_waves(e.id);
            s_enemyIndex.erase(e.id); s_bossTactics.erase(e.id);
            e = {}; continue;
        }
        // Pending creation is not completion or a kill. Keep its ID until the scene
        // destroys it or the actor is ready to accept the deferred delete request.
        if (e.retiring && actor) svc_actor->delete_actor(mod_ctx, e.id);
    }
}
bool own_collider(const cCcD_Obj* collider) {
    if (!collider) return false;
    if (effect_collider(collider)) return true;
    for (const auto& a : s_attacks) if (collider == &a.collider) return true;
    for (const auto& orb : s_zeldaLightOrbs) if (collider == &orb.collider) return true;
    return false;
}
#include "survivors_contacts.inc"
void detach_attack_references() {
    detach_pot_attacks();
    // The collision server also keeps last-frame pointers for debug drawing.
    // Null only our entries; never reset another actor/mod's collision state.
    if (auto* server = dComIfG_Ccsp()) {
        for (auto*& obj : server->mpObjAt) if (own_collider(obj)) obj = nullptr;
        for (auto*& obj : server->mpObjTg) if (own_collider(obj)) obj = nullptr;
        for (auto*& obj : server->mpObjCo) if (own_collider(obj)) obj = nullptr;
        for (auto*& obj : server->mpObj) if (own_collider(obj)) obj = nullptr;
    }
    const auto clear = [](dCcD_Sph& collider) {
        if (own_collider(collider.GetTgHitObj())) collider.ResetTgHit();
    };
    for (const auto& e : s_enemies) {
        auto* actor = actor_by_id(e.id);
        if (!actor || !e.initialized) continue;
        dCcU_AtInfo* info = nullptr;
        if (e.profile == fpcNm_E_OC_e) {
            auto* native = static_cast<daE_OC_c*>(actor);
            for (auto& collider : native->mSphs_cc) clear(collider);
            info = &native->mAtInfo;
        } else if (e.profile == fpcNm_E_BA_e) {
            auto* native = reinterpret_cast<e_ba_class*>(actor);
            clear(native->mSph); info = &native->mAtInfo;
        } else if (e.profile == fpcNm_E_MS_e) {
            auto* native = static_cast<e_ms_class*>(actor);
            clear(native->mSph1); clear(native->mSph2); info = &native->mAtInfo;
        } else if (e.profile == fpcNm_E_RD_e) {
            auto* native = reinterpret_cast<e_rd_class*>(actor);
            for (auto& collider : native->cc_sph) clear(collider);
            info = &native->at_info;
        }
        if (info && own_collider(info->mpCollider)) info->mpCollider = nullptr;
    }
}
void reset_runtime() {
    clear_player_contacts();
    restore_gear_collection();close_gear_window();s_gear={};s_gearWritable=true;clear_gear_notices();
    clear_zelda_light_orbs();clear_king_waves();
    s_enemyHitSources.clear(); s_enemyContactFrames.clear(); s_bossTactics.clear();
    stop_survivors_music();
    end_survivors_integrity();
    reset_ending();
    s_hubMenu=HubMenu::None; s_hubSaveRequested=false; s_hubClosingWindow=0;
    s_hub=true; s_hubDeparture=s_hubDoorStarted=s_arrivingRun=s_allowHubDoor=false;
    s_hubDoor=kNone; s_hubDoorTicks=0;
    update_survivors_equipment_chest(false,{},0);
    update_survivors_pedestal(false);
    s_options.debug.clamp();
    reset_midna_dialog();
    s_saveReady = false; s_gameStarted = false;
    s_map = s_requestedMap = 0; clear_arena_geometry();
    close_powerup_window(); close_window(); clear_attacks(); clear_loot(); detach_attack_references(); retire_enemies();
    if (s_savePrompt != kNone) {
        d_GameOver_Delete(s_savePrompt); s_savePrompt = kNone;
        dComIfGp_setGameoverStatus(0);
    }
    clear_combat_feedback();
    enemy_hud::reset();
    s_run = {}; s_orbs = {}; s_cooldowns = {};
    s_player = kNone; s_loaded = false; s_resultsStored = false;
    s_storeFailed = false; s_restart = false; s_saveRequested = false; s_saveStartTicks = 0;
    s_progressSaving=s_progressSaved=false;
    s_spawnCooldown = s_spawnFailures = s_warmup = 0;
    s_swordAction = SwordAction::None; s_meleeCooldown = 0; s_wavePending = false;
    s_comboStep = s_comboTimeout = 0;
}
void prepare_save() {
    save_state_set_boss_rush_active(false);
    save_state_set_intro_skipped(false);
    auto* save = dComIfGs_getSaveData();
    save->getPlayer().getPlayerReturnPlace().set(arena().stage, arena().room, arena().point);
    dComIfGs_onEventBit(dSv_event_flag_c::F_0250);
    dComIfGs_onEventBit(dSv_event_flag_c::F_0302);
    dComIfGs_onEventBit(dSv_event_flag_c::M_010);
    dComIfGs_onEventBit(dSv_event_flag_c::M_067); // Native Midna riding/call and touch icon.
    for (int province=0; province<3; ++province) dComIfGs_onDarkClearLV(province);
    dComIfGs_setTransformStatus(TF_STATUS_HUMAN);
    dComIfGs_onItemFirstBit(dItemNo_WEAR_KOKIRI_e);
    dComIfGs_setCollectClothes(KOKIRI_CLOTHES_FLAG);
    dComIfGs_setSelectEquipClothes(dItemNo_WEAR_KOKIRI_e);
    dComIfGs_offCollectSword(COLLECT_LIGHT_SWORD);
    dComIfGs_offItemFirstBit(dItemNo_LIGHT_SWORD_e);
    for (int shield = COLLECT_WOODEN_SHIELD; shield <= COLLECT_HYLIAN_SHIELD; ++shield) dComIfGs_offCollectShield(shield);
    dComIfGs_offCollectSword(COLLECT_MASTER_SWORD);
    dComIfGs_offItemFirstBit(dItemNo_MASTER_SWORD_e);
    dComIfGs_onItemFirstBit(dItemNo_SWORD_e);
    dComIfGs_setCollectSword(COLLECT_ORDON_SWORD);
    dComIfGs_setSelectEquipSword(dItemNo_SWORD_e);
    dComIfGs_setSelectEquipShield(dItemNo_NONE_e);
    dComIfGs_setBButtonItemKey(dItemNo_SWORD_e);
    // Save callbacks run inside file select, before Link exists. The live
    // dComIfGs_setSelectItemIndex hook in HD HUD checks Link's wolf state for
    // slot 2. Initialize the data directly, including the play cache, instead.
    // Clear assignments first so setItem cannot refresh an assigned slot.
    auto& status = save->getPlayer().getPlayerStatusA();
    for (int i = 0; i < MAX_SELECT_ITEM; ++i) {
        status.setSelectItemIndex(i, dItemNo_NONE_e);
        status.setMixItemIndex(i, dItemNo_NONE_e);
        g_dComIfG_gameInfo.play.setSelectItem(i, dItemNo_NONE_e);
    }
    for (int i = 0; i < MAX_ITEM_SLOTS; ++i) dComIfGs_setItem(i, dItemNo_NONE_e);
    dComIfGs_setWalletSize(2); // Giant wallet for escalating chest prices.
    dComIfGs_setMaxLife(15); // Three hearts; max life uses fifths, gauge uses quarters.
    dComIfGs_setLife(12);
    dComIfGs_setRupee(0);
    set_chest_keys(0); // Keys belong to this run, never a previous arena/save.
}
void load_records() {
    if (s_loaded) return;
    s_records = {};
    auto bytes = s_records.encode(); size_t size = bytes.size();
    const auto result = svc_save->get_blob(mod_ctx, kBlob, bytes.data(), &size);
    if (result == MOD_OK) s_records.decode(std::span(bytes.data(), std::min(size, bytes.size())));
    load_gear();
    s_loaded = true;
}
#include "survivors_progress_save.inc"
void finish(bool won) {
    if (s_resultsStored || (s_run.phase != Phase::Running && s_run.phase != Phase::Upgrade &&
        s_run.phase != Phase::TreasureChoice && s_run.phase != Phase::Treasure &&
        !(s_deathPending && s_run.phase==Phase::Defeat))) return;
    update_survivors_integrity(s_run.modded);
    end_survivors_integrity();
    s_run.end(won); clear_attacks(); retire_enemies();
    s_victory=s_deathPending=s_deathDismiss=false;
    // Only restore health after the native death presentation (or a voluntary
    // end). The scene reload owns recovery of Link's dead action and camera.
    dComIfGs_setLife(dComIfGs_getMaxLifeGauge());
    load_records(); s_records.finish(s_run,s_map); s_progressSaved=false;
    const auto bytes = s_records.encode();
    s_storeFailed = svc_save->set_blob(mod_ctx, kBlob, bytes.data(), bytes.size()) != MOD_OK;
    s_resultsStored = true;
}
#include "survivors_ending.inc"
#include "survivors_midna.inc"
void add_orb(cXyz position, unsigned value) {
    position.y -= 20;
    cXyz probe = position; probe.y += 500;
    dBgS_ObjGndChk ground; ground.SetPos(&probe);
    const float floor = dComIfG_Bgsp().GroundCross(&ground);
    if (std::isfinite(floor) && floor > -1.0e8f) position.y = floor + 25;
    Orb* nearest = &s_orbs[0]; float distance = 1.0e20f;
    for (auto& orb : s_orbs) {
        if (!orb.value) { orb = {position, value}; return; }
        const float d = orb.position.abs2(position);
        if (d < distance) { distance = d; nearest = &orb; }
    }
    // Preserve XP at the cap without spawning more game actors or particles.
    nearest->value += value;
    nearest->merged = true;
}
void drop_reward(fopAc_ac_c* actor) {
    const auto drop = s_run.roll_drop();
    if (drop == Run::Drop::None) return;
    constexpr std::array<int, unsigned(Run::Drop::Count)> items{-1, dItemNo_HEART_e,
        dItemNo_GREEN_RUPEE_e, dItemNo_BLUE_RUPEE_e, dItemNo_YELLOW_RUPEE_e,
        dItemNo_RED_RUPEE_e, dItemNo_PURPLE_RUPEE_e, dItemNo_ORANGE_RUPEE_e, dItemNo_SILVER_RUPEE_e};
    const int item = items[unsigned(drop)];
    // Native item physics, pickup effects and currency/health handling. Ordinary
    // enemy drop hooks remain blocked, so a death cannot generate two rewards.
    create_run_pickup(actor->current.pos, item);
}
void credit_death(Enemy& e, fopAc_ac_c* actor) {
    if (s_victory || e.credited || e.retiring || !e.initialized || s_run.phase != Phase::Running ||
        !in_arena() || transitioning()) return;
    bool dead = actor->health <= 0;
    if (e.profile == fpcNm_E_BA_e)
        dead = dead || reinterpret_cast<e_ba_class*>(actor)->mIsDying;
    if (e.profile == fpcNm_E_OC_e)
        dead = dead || static_cast<daE_OC_c*>(actor)->getActionMode() == 9;
    if (!dead) return;
    e.credited = true; ++s_run.kills;
    if(e.elite)drop_boss_gear();
    if (s_run.life_steal_active() && e.swordKill && dComIfGs_getLife() > 0)
        dComIfGs_setLife(std::min<unsigned>(dComIfGs_getMaxLifeGauge(),dComIfGs_getLife()+1));
    if(e.elite) {
        if(e.eliteSlot>=0 && unsigned(e.eliteSlot)<timed_elite_count)++s_run.timed_elites;
        else ++s_run.guard_elites;
    }
    if (e.elite || s_run.normal_kill_drops()) {
        if (!e.elite) {
            drop_reward(actor);
            if (s_run.roll_exp_magnet()) queue_exp_magnet(e,actor);
        }
        add_orb(actor->current.pos, e.elite ? 30 : (e.profile == fpcNm_E_OC_e || e.profile == fpcNm_E_RD_e) ? 3 : 1);
    }
    if (e.elite) { drop_elite_chest(e, actor); queue_exp_magnet(e, actor); }
    // The scheduled final boss ends combat even if an earlier elite survived.
    // Its own kill, damage and loot have already been credited exactly once.
    if (!s_run.endless && e.elite && e.eliteSlot==int(timed_elite_count)-1) begin_victory(actor->current.pos);
}
void update_orbs(daAlink_c* link) {
    const float range = magnet_range();
    for (auto& orb : s_orbs) {
        if (!orb.value) continue;
        const auto target = link->current.pos + cXyz(0, 30, 0);
        cXyz delta = target - orb.position;
        const float distance = delta.abs();
        if (distance < 60) { s_run.xp += s_run.gain_xp(orb.value); orb = {}; }
        else if (orb.summoned || distance < range) {
            const float speed = orb.summoned ? std::max(35.f, distance * 0.08f) : 35.f;
            orb.position += delta * (std::min(speed, distance) / distance);
        }
    }
}
bool create_enemy_at(cXyz pos, s16 yaw, unsigned type, bool elite, int eliteSlot = -1, bool dormant = false) {
    auto slot = std::find_if(s_enemies.begin(), s_enemies.end(), [](const auto& e) { return e.id == kNone; });
    if (slot == s_enemies.end()) return false;
    if (type >= native_roster.size()) return false;
    const auto& native=native_roster[type];
    ActorId id = kNone;
    const float scale = elite ? elite_scale(Species(type)) : 1.0f;
    const ActorSpawnParams params{
        .parameters = native.parameters, .argument = native.argument, .room_num = arena().room,
        .position = {pos.x, pos.y, pos.z},
        .angle = {static_cast<s16>(native.profile==fpcNm_E_RDY_e ? 0xff : 0), static_cast<s16>(yaw + 0x8000), static_cast<s16>(native.profile==fpcNm_E_KR_e ? 0xff : 0)},
        .scale = {scale, scale, scale}, .create_function = nullptr,
    };
    if (create_standalone_actor(native.profile, params, id) != MOD_OK) return false;
    *slot = {id, native.profile, false, false, false, elite, 0};
    s_enemyIndex.add(id, std::size_t(slot - s_enemies.begin()));
    slot->species=type;
    slot->eliteSlot = eliteSlot; slot->dormant = dormant; slot->spawnPosition = slot->lootPosition = pos;
    // Callers add flight height before creation; never cache that altitude as loot ground.
    if (!loot_floor_position(slot->lootPosition)) slot->lootPosition.y-=native.height;
    return true;
}
bool spawn_enemy(daAlink_c* link, bool elite, int eliteSlot = -1) {
    auto slot = std::find_if(s_enemies.begin(), s_enemies.end(), [](const auto& e) { return e.id == kNone; });
    if (slot == s_enemies.end()) return false;
    std::array<unsigned,species_count> counts{};
    for (const auto& e:s_enemies) if (e.id!=kNone && !e.elite && !e.retiring && !e.credited && e.species<species_count)
        ++counts[e.species];
    const unsigned groupRoll=elite?0:s_run.next_random(),pickRoll=elite?0:s_run.next_random();
    const unsigned type=elite ? unsigned(eliteSlot>=0 && unsigned(eliteSlot)<timed_elite_count ?
        map_roster(s_map).elites[eliteSlot] : Species::Bokoblin) :
        unsigned(choose_species(s_map,s_run.ticks/ticks_per_second,s_run.horde_remaining!=0,
            s_run.options.horde,counts,groupRoll,pickRoll));
    if (type>=native_roster.size()) return false;
    for (unsigned attempt = 0; attempt < 12; ++attempt) {
        const auto yaw = static_cast<s16>(s_run.next_random());
        const float radius = 600.0f + float(s_run.next_random() % 450);
        cXyz pos = link->current.pos + cXyz(cM_ssin(yaw) * radius, 0, cM_scos(yaw) * radius);
        if (!walkable_spawn_path(link->current.pos, pos, elite ? 100.f : 40.f)) continue;
        pos.y += native_roster[type].height;
        return create_enemy_at(pos, yaw, type, elite, eliteSlot);
    }
    return false;
}
#include "survivors_elites.inc"
#include "survivors_pots.inc"
#include "survivors_save_slot.inc"
void update_spawns(daAlink_c* link) {
    update_boss_watchdogs(link);
    unsigned population = 0;
    for (auto& e : s_enemies) {
        if (e.id == kNone) continue;
        if (!e.elite && !e.retiring && !e.credited) ++population;
        auto* actor = actor_by_id(e.id);
        if (!actor || !e.initialized) continue;
        ++e.age;
        // Guards and scheduled elites persist until killed; cleanup grants no loot.
        // Do not let ordinary enemies stranded in the old area occupy the
        // population budget for three minutes after Link has moved away.
        if (!e.elite && !e.retiring && !e.credited && (e.age>5400 ||
            (e.age>10*ticks_per_second && actor->current.pos.absXZ(link->current.pos)>6000.f))) {
            e.retiring=true;--population; // Existing cleanup deletes without kill/drop credit.
        }
    }
    if ((s_run.endless || s_run.ticks <= s_run.run_duration()) && s_run.ticks % s_run.boss_interval() == 0) s_spawnCooldown = 0;
    for (const auto& encounter : s_eliteEncounters)
        if (encounter.defeated && encounter.respawnAt == s_run.ticks) s_spawnCooldown = 0;
    if (s_spawnCooldown) { --s_spawnCooldown; return; }
    s_spawnCooldown = s_run.spawn_interval();
    update_elite_spawns(link);
    if(active_boss_encounter()){s_spawnFailures=0;return;}
    if ((!s_run.endless && s_run.ticks >= s_run.run_duration()) || population>=s_run.target_population()) { s_spawnFailures=0; return; }
    const unsigned batch=s_run.spawn_batch();
    for (unsigned i=0; i<batch && population<s_run.target_population(); ++i) {
        if (spawn_enemy(link,false)) { ++population; s_spawnFailures=0; }
        else { ++s_spawnFailures; break; }
    }
}
#include "survivors_manual_attack.inc"
cXyz target_direction(daAlink_c* link) {
    float nearest = 1600 * 1600;
    cXyz result(cM_ssin(link->shape_angle.y), 0, cM_scos(link->shape_angle.y));
    const cXyz origin = link->current.pos + cXyz(0, 70, 0);
    for (const auto& e : s_enemies) {
        if (!e.initialized || e.retiring || e.credited) continue;
        auto* actor = actor_by_id(e.id);
        if (!actor) continue;
        cXyz target = actor->eyePos;
        cXyz delta = target - origin;
        const float distance = delta.abs2();
        if (distance < 1 || distance >= nearest) continue;
        dBgS_LinChk wall; wall.Set(&origin, &target, link);
        if (dComIfG_Bgsp().LineCross(&wall)) continue;
        nearest = distance; result = delta * (1.0f / std::sqrt(distance));
    }
    return result;
}
#include "survivors_spin_effects.inc"
void launch(daAlink_c* link, unsigned weapon, cXyz direction, bool manual, float spread) {
    auto& a = acquire_projectile(s_attacks, weapon, 0, 8);
    a.weapon = weapon; a.life = 35; a.ballVictimCount = 0;
    a.aimPending=manual;a.aimSpread=spread;
    a.position = link->current.pos + cXyz(0, 65, 0);
    a.velocity = direction * 42.0f;
    a.greatSpin = s_run.great_spin(Upgrade::Wave);
    a.radius = s_run.wave_radius();
    const auto& spin = link->mpHIO->mCut.mCutTurn.m;
    a.effectBaseRadius = a.greatSpin ? spin.mLargeAttackRadius : spin.mAttackRadius;
    a.status.Init(120, 0xff, link);
    a.collider.Set(kAttackSource); a.collider.SetStts(&a.status);
    a.collider.SetAtType(AT_TYPE_NORMAL_SWORD);
    a.collider.SetAtAtp(static_cast<u8>(s_run.attack_power(weapon)));
    // Keep the native material neutral so linear Survivors damage applies.
    a.collider.SetAtMtrl(dCcD_MTRL_NONE);
    a.collider.OnAtNoConHit(); a.collider.ResetAtHit(); a.collider.OnAtSetBit();
    a.active = true;
}
void update_wave_effects(Attack& a, daAlink_c* link) {
    // Snapshot the launch tier: an upgrade never changes an in-flight collider.
    const auto effects = survivor_spin_effects(a.greatSpin);
    const float size = a.radius / std::max(1.f, a.effectBaseRadius);
    const cXyz scale(size, size, size);
    const csXyz rotation(-cM_atan2s(a.velocity.y, a.velocity.absXZ()),
                         cM_atan2s(a.velocity.x, a.velocity.z), 0);
    for (unsigned i = 0; i < effects.size(); ++i) {
        auto& id = a.emitters[i];
        auto* emitter = id ? dComIfGp_particle_getEmitter(id) : nullptr;
        if (id && (!emitter || (emitter->mMaxFrame > 0 && emitter->getAge() >= emitter->mMaxFrame))) {
            if (emitter) {
                emitter->stopCreateParticle(); emitter->quitImmortalEmitter();
                emitter->becomeInvalidEmitter();
            }
            id = 0;
        }
        id = dComIfGp_particle_set(id, effects[i], &a.position, &link->tevStr,
                                  &rotation, &scale, 0xff, nullptr, -1, nullptr, nullptr, nullptr);
        if (id) {
            dComIfGp_particle_levelEmitterOnEventMove(id);
            if (auto* current = dComIfGp_particle_getEmitter(id)) current->setGlobalTranslation(a.position);
        }
    }
}
bool sword_action_available(daAlink_c* link) {
    if (!link->mLinkAcch.ChkGroundHit() || link->checkWolf() || !link->mSwordModel) return false;
    switch (link->mProcID) {
    case daAlink_c::PROC_WAIT: case daAlink_c::PROC_MOVE:
    case daAlink_c::PROC_ATN_MOVE: case daAlink_c::PROC_ATN_ACTOR_WAIT:
    case daAlink_c::PROC_ATN_ACTOR_MOVE: case daAlink_c::PROC_WAIT_TURN:
    case daAlink_c::PROC_MOVE_TURN: return true;
    default: return false; // Never interrupt damage, dodge, jumping or item actions.
    }
}
bool enemy_in_range(daAlink_c* link, float range) {
    for (const auto& e : s_enemies) {
        if (!e.initialized || e.retiring || e.credited) continue;
        if (auto* actor = actor_by_id(e.id); actor && actor->current.pos.abs(link->current.pos) < range)
            return true;
    }
    return false;
}
void release_sword_wave(daAlink_c* link) {
    const bool manual=s_run.options.manual_enabled(Upgrade::Wave)&&(s_waveManual||manual_skill(link,Upgrade::Wave));
    const auto direction = manual ? manual_direction(link) : target_direction(link);
    for (unsigned i=0; i<s_run.ranged_shots(); ++i) {
        const float angle=manual ? (i==0?0.f:float((i+1)/2)*(i%2?-1.f:1.f)*.20943951f) : s_run.wave_spread(i);
        const float sine=std::sin(angle), cosine=std::cos(angle);
        launch(link,0,cXyz(direction.x*cosine+direction.z*sine,direction.y,
                          direction.z*cosine-direction.x*sine),manual,angle);
    }
}
bool normal_sword_proc(daAlink_c* link) {
    return link->mProcID == daAlink_c::PROC_CUT_NORMAL || link->mProcID == daAlink_c::PROC_CUT_FINISH;
}
#include "survivors_targeting.inc"
#include "survivors_melee.inc"
bool begin_sword_combo(daAlink_c* link) {
    // Native recovery can clear its counter before our cooldown expires. Keep
    // the automatic sequence across that gap, but let native code pick each
    // directional/lock-on/hit-dependent swing and the fourth-hit finisher.
    s_manualSwing=manual_sword(link);
    link->mComboCutCount = s_comboStep;
    struct Scope { Scope(){s_autoSwordCall=true;} ~Scope(){s_autoSwordCall=false;} } scope;
    if (!link->checkCutAction() || !normal_sword_proc(link)) return false;
    s_comboStep = link->mComboCutCount % 4;
    s_comboTimeout = 2 * ticks_per_second;
    return true;
}
#include "survivors_wave_cast.inc"
#include "survivors_jump_strike.inc"
void sync_spin_radius(daAlink_c* link) {
    // Native normal/light particles are authored for the final native radius.
    // Match their geometry and particle size to the current expanding hitbox,
    // including Reach. Absolute scales avoid compounding on reused emitters.
    const float radius = s_run.melee_area_radius(link->field_0x347c);
    const float nativeRadius = link->field_0x3478;
    if (!std::isfinite(radius) || !std::isfinite(nativeRadius) || radius <= 0 || nativeRadius <= 0) return;
    link->mAtSph.SetR(radius);
    const float scale = radius / nativeRadius;
    const cXyz effectScale(scale, scale, scale);
    for (auto id : link->field_0x3204)
        if (auto* emitter = id ? dComIfGp_particle_getEmitter(id) : nullptr)
            emitter->setGlobalScale(effectScale);
}
void after_spin_effect(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !playable() || s_run.phase != Phase::Running || fopAcM_GetID(link) != s_player ||
        s_swordAction != SwordAction::Spin || link->mProcID != daAlink_c::PROC_CUT_TURN) return;
    // Runs after native creation/update, so newly created normal and Light Sword
    // emitters receive the same radius in their very first rendered frame.
    sync_spin_radius(link);
}
bool begin_spin_attack(daAlink_c* link) {
    // A due ability may interrupt an ordinary cut, but never another ability,
    // equipment transition, damage, dodge or airborne action.
    if (ranged_pose_active() || link->checkEquipAnime() ||
        (!sword_action_available(link) && !(s_swordAction==SwordAction::Normal && !s_wavePending && normal_sword_proc(link))) ||
        !link->mLinkAcch.ChkGroundHit() || link->checkWolf() || !link->mSwordModel) return false;
    if (!link->checkItemSwordEquip()) {
        if (link->mEquipItem==dItemNo_NONE_e) link->swordEquip(0);
        return false;
    }
    const auto previous=s_swordAction;
    s_swordAction=SwordAction::Spin;
    struct Scope { Scope(){s_autoSwordCall=true;} ~Scope(){s_autoSwordCall=false;} } scope;
    if (!link->procCutTurnInit(1,1)) {s_swordAction=previous;return false;}
    s_trackMelee=s_wavePending=false;s_meleeTarget=kNone;
    s_comboStep=s_comboTimeout=0;
    new_melee_swing();
    // Use the perk tier, independently of the equipped sword's Light flag.
    const auto& spin=link->mpHIO->mCut.mCutTurn.m;
    link->field_0x3478=s_run.great_spin(Upgrade::Spin)?spin.mLargeAttackRadius:spin.mAttackRadius;
    return true;
}
void update_sword(daAlink_c* link) {
    if (s_meleeCooldown) --s_meleeCooldown;
    if (s_comboTimeout && !--s_comboTimeout) s_comboStep = 0;
    if (ranged_pose_active()) return; // Ordinary cuts always yield presentation to abilities.
    bool continuingSwing = false;
    if (s_swordAction == SwordAction::Jump) return; // Owned traversal finishes before the scheduler resumes.
    if (s_swordAction == SwordAction::Normal) {
        if (normal_sword_proc(link)) {
            // Never cut off the damage window, and let the final hit recover.
            if ((!s_comboStep && !s_multiStrike.count(StrikeSeries::Sword)) ||
                (s_meleeCooldown && !s_multiStrike.count(StrikeSeries::Sword)) || link->mUnderFrameCtrl[0].getFrame() <=
                std::max(link->field_0x347c, link->field_0x3484)) return;
            continuingSwing = true;
        } else {
            s_wavePending = false; // Interrupted attacks cannot fire late from a damage/dodge action.
            s_swordAction = SwordAction::None; s_trackMelee = false; s_meleeTarget = kNone;
        }
    } else if (s_swordAction == SwordAction::Spin) {
        if (link->mProcID == daAlink_c::PROC_CUT_TURN) return;
        s_swordAction = SwordAction::None;
    }
    if ((!continuingSwing && !sword_action_available(link)) ||
        !link->mLinkAcch.ChkGroundHit() || link->checkWolf() || !link->mSwordModel) {
        s_comboStep = s_comboTimeout = 0;
        return;
    }
    if ((s_meleeCooldown && !s_multiStrike.count(StrikeSeries::Sword)) || (!manual_sword(link) && !melee_target(link))) return;
    if (!link->checkItemSwordEquip()) {
        if (link->mEquipItem != dItemNo_NONE_e) return;
        // The pending item ID can survive an interrupted upper animation.
        // Only an actually running equip animation means the draw is pending.
        if (!link->checkEquipAnime()) link->swordEquip(0);
        return; // Let native equip animation finish before replacing the body action.
    }
    // Other abilities run independently; this scheduler owns only the native
    // normal combo and its occasional Jump Strike movement.
    if (!s_multiStrike.count(StrikeSeries::Sword) && try_jump_strike(link)) return;
    cXyz targetPoint;
    auto* target = melee_target(link, &targetPoint);
    if (!target && !manual_sword(link)) return;
    const auto direction = manual_sword(link) ? manual_direction(link) : targetPoint - link->current.pos;
    link->shape_angle.y = cM_atan2s(direction.x, direction.z);
    link->current.angle.y = link->shape_angle.y;
    if (begin_sword_combo(link)) {
        if(s_multiStrike.count(StrikeSeries::Sword))s_multiStrike.consume(StrikeSeries::Sword);
        else s_multiStrike.start(StrikeSeries::Sword,s_run.multi_strikes());
        s_swordAction = SwordAction::Normal; s_wavePending = false; s_trackMelee = true;
        new_melee_swing();
        link->mUnderFrameCtrl[0].setRate(link->mUnderFrameCtrl[0].getRate()*s_run.melee_speed());
        link->mUpperFrameCtrl[0].setRate(link->mUpperFrameCtrl[0].getRate()*s_run.melee_speed());
        s_meleeCooldown = s_run.melee_cooldown();
    }
}
void after_sword_move(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !playable() || s_run.phase != Phase::Running || fopAcM_GetID(link) != s_player) return;
    const bool spin = s_swordAction == SwordAction::Spin && link->mProcID == daAlink_c::PROC_CUT_TURN;
    const bool normal = s_swordAction == SwordAction::Normal && normal_sword_proc(link);
    // Scale the actual collision radius each frame, not only its target. The
    // native expansion window is too short to reach a scaled target at high Reach.
    if (spin) sync_spin_radius(link);
    if ((!spin && !normal) || !link->mLinkAcch.ChkGroundHit()) return;
    // Runs before native posMove/ground/wall resolution, retaining collision.
    // Normal swings and finishers decelerate/lunge too, not only spin attacks.
    link->current.angle.y = link->mMoveAngle;
    link->mNormalSpeed = link->mpHIO->mMove.m.mMaxSpeed * link->mStickValue;
}
HookAction before_large_spin(ModContext*, void*, void* result, void*) {
    if (!in_arena()) return HOOK_CONTINUE;
    *static_cast<BOOL*>(result) = s_swordAction==SwordAction::Spin && s_run.great_spin(Upgrade::Spin);
    return HOOK_SKIP_ORIGINAL;
}
#include "survivors_loot.inc"
#include "survivors_fate.inc"
#include "survivors_horde_portals.inc"
#include "survivors_minimap.inc"
#include "survivors_ranged.inc"
#include "survivors_keys.inc"
#include "survivors_treasure.inc"
#include "survivors_inventory.inc"
#include "survivors_black_flame.inc"
#include "survivors_status.inc"
#include "survivors_lightning.inc"
#include "survivors_knockback.inc"
void new_melee_swing() { ++s_meleeSerial; }
#include "survivors_combat_pause.inc"
#include "survivors_defense.inc"
#include "survivors_boss_guard.inc"
#include "survivors_concurrent_hits.inc"
#include "survivors_bulblin.inc"
#include "survivors_movement.inc"
#include "survivors_locomotion.inc"
#include "survivors_ledge_jump.inc"

#include "survivors_parallel.inc"

void update_weapons(daAlink_c* link) {
    for (auto& a : s_attacks) {
        a.aimPending=false;
        if (!a.active) continue;
        a.status.Move();
        if (a.weapon >= unsigned(Upgrade::Bow) && a.weapon != unsigned(Upgrade::Shockwave)) { update_item_attack(a, link); continue; }
        if (!a.life-- || a.collider.ChkAtShieldHit()) { stop_attack(a); continue; }
        if (a.weapon == unsigned(Upgrade::Spin) || a.weapon == unsigned(Upgrade::ShieldAttack))
            update_parallel_pulse(a, link);
        const cXyz next = a.position + a.velocity;
        dBgS_LinChk wall; wall.Set(&a.position, &next, link);
        if ((a.weapon == 0 || a.weapon == unsigned(Upgrade::Shockwave)) && dComIfG_Bgsp().LineCross(&wall)) { stop_attack(a); continue; }
        a.position = next;
        if (a.weapon == 0 || a.weapon == unsigned(Upgrade::Shockwave)) update_wave_effects(a, link);
        a.collider.SetC(a.position); a.collider.SetR(a.radius);
        a.collider.SetAtVec(a.velocity);
        dComIfG_Ccsp()->Set(&a.collider);
    }
    const bool jumping = update_jump_strike(link);
    if (!jumping && auto_attacks_suspended(link)) { suspend_auto_attacks(link); return; }
    for (auto& cooldown : s_cooldowns) if (cooldown) --cooldown;
    update_lightning(link);
    update_ranged(link); // Cosmetic upper-body pose; never gates a weapon.
    update_parallel_skills(link);
    fire_items(link); // Every due ranged weapon may release in this frame.
    if (!jumping) update_sword(link);

}
bool native_sword_collider(const cCcD_Obj* collider) {
    auto* link = daAlink_getAlinkActorClass();
    if (!in_arena() || !link || fopAcM_GetID(link) != s_player) return false;
    if (s_swordAction == SwordAction::Spin && collider == &link->mAtSph) return true;
    if (s_swordAction == SwordAction::Jump && jump_strike_pose(link) && collider == &link->mAtSph) return true;
    if (s_swordAction == SwordAction::Normal || (s_swordAction == SwordAction::Jump && jump_strike_pose(link)))
        for (const auto& capsule : link->mAtCps) if (collider == &capsule) return true;
    return false;
}
void after_power(ModContext*, void* args, void*, void*) {
    if (!s_active) return;
    auto* info = mods::arg<dCcU_AtInfo*>(args, 0);
    if (!info || !info->mpCollider) return;
    if (reuse_bulblin_power(info)) {
        const auto* hit=bulblin_power_hit(info);mark_damage_critical(info,hit->critical);
        if(hit->basePower)mark_damage_split(info,hit->basePower,hit->scaledPower);
        return;
    }
    const auto scaled_power=[&](unsigned base) {
        const auto scaled=s_run.dark_power(base);mark_damage_split(info,base,scaled);
        if(auto* hit=bulblin_power_hit(info)){hit->basePower=base;hit->scaledPower=scaled;}
        return s_run.player_damage(scaled);
    };
    const auto critical_power = [&](unsigned base) {
        const auto sources=hit_damage_sources(info->mpCollider);
        bool melee=false;for(unsigned i=0;i<sources.size();++i)if(sources[i]&&melee_critical_source(DamageSource(i)))melee=true;
        const bool bow=sources[unsigned(DamageSource::Bow)]!=0;
        bool critical=false; const auto power=s_run.critical_power(base,&critical,melee,bow);
        if (auto* hit=bulblin_power_hit(info)) hit->critical=critical;
        mark_damage_critical(info,critical);
        return power;
    };
    const BulblinPowerCache cache{info};
    for (const auto& effect : s_effects) if (info->mpCollider == &effect.collider) {
        info->mHitType = HIT_TYPE_LINK_HEAVY_ATTACK;
        if (auto* hit=bulblin_power_hit(info)) hit->critical=effect.critical;
        mark_damage_critical(info,effect.critical);
        info->mAttackPower = s_run.player_damage(s_run.dark_power(effect.damage) + effect.fixedDamage); return;
    }
    auto* link = daAlink_getAlinkActorClass();
    if (in_arena() && link && info->mpCollider == &link->mGuardAtCps) {
        info->mHitType = HIT_TYPE_LINK_HEAVY_ATTACK; // Rat stun must not reset elite HP to one.
        info->mAttackPower = scaled_power(critical_power(s_run.attack_power(unsigned(Upgrade::ShieldAttack)))); return;
    }
    if (native_sword_collider(info->mpCollider)) {
        info->mHitType = HIT_TYPE_LINK_HEAVY_ATTACK; // Keep cc_at_check from doubling the linear value for Master Sword.
        const auto base = s_swordAction == SwordAction::Jump && info->mpCollider == &link->mAtSph ?
            s_run.jump_splash_power() : s_swordAction == SwordAction::Spin ? s_run.attack_power(1) : s_run.melee_power();
        info->mAttackPower = scaled_power(critical_power(base));
        return;
    }
    for (const auto& attack : s_attacks) {
        if (info->mpCollider == &attack.collider) {
            // The native table maps 3 to 30 and >=4 to up to 200 damage.
            // Keep native collision, defense, damage reactions and death handling,
            // but use a linear power value for our own weapons only.
            info->mHitType = HIT_TYPE_LINK_HEAVY_ATTACK;
            info->mAttackPower = scaled_power(critical_power(info->mpCollider->GetAtAtp()));
            return;
        }
    }
}
void after_player(ModContext*, void* args, void*, void*) {
    if (!s_active || s_run.phase != Phase::Running || !playable()) return;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || fopAcM_GetID(link) != s_player) return;
    if (dComIfGs_getLife() == 0 && !s_victory) { begin_defeat(); return; }
    update_combat_feedback();
    update_gear_notice();
    chest_spawning_allowed(); // Also retire exhausted chests during victory loot time.
    update_palace_ferries();
    if (s_victory) {
        update_victory();
        update_exp_magnets(link); update_orbs(link);
        s_run.maximum_hearts = dComIfGs_getMaxLife() / 5;
        if (s_run.level_up()) { if (!s_run.greed()) dComIfGs_setLife(dComIfGs_getMaxLifeGauge()); clear_attacks(); }
        // Existing chests remain interactable; do not spawn new ones after victory.
        // Do not interrupt a final chest/level-up on the expiration frame.
        if (s_victoryTicks>=10*ticks_per_second && s_run.phase==Phase::Running) finish(true);
        return;
    }
    enemy_hud::tick();
    s_run.tick(true);
    if (s_run.life_regen_due() && dComIfGs_getLife()<dComIfGs_getMaxLifeGauge())
        dComIfGs_setLife(std::min<unsigned>(dComIfGs_getMaxLifeGauge(),dComIfGs_getLife()+1));
    if (const unsigned rupees=s_run.auto_rupees())
        dComIfGs_setRupee(std::min<unsigned>(dComIfGs_getRupeeMax(),dComIfGs_getRupee()+s_run.gain_rupees(rupees)));
    update_enemy_effects();
    update_reachable_floor();
    update_horde_portals(link);
    update_spawns(link); update_pots(link); update_exp_magnets(link); update_weapons(link); update_orbs(link);
    s_run.maximum_hearts = dComIfGs_getMaxLife() / 5;
    if (s_run.level_up()) {
        if (!s_run.greed()) dComIfGs_setLife(dComIfGs_getMaxLifeGauge());
        clear_attacks();
    }
    else update_chests(link);
}
HookAction before_death(ModContext*, void*, void* result, void*) {
    if (!in_arena()) return HOOK_CONTINUE;
    if (s_victory) { // Victory is settled; environmental damage cannot revoke it.
        dComIfGs_setLife(std::max<unsigned>(1,dComIfGs_getLife()));
        *static_cast<int*>(result)=1; return HOOK_SKIP_ORIGINAL;
    }
    begin_defeat();
    return HOOK_CONTINUE; // Link creates and animates the native Game Over screen.
}
HookAction before_cut(ModContext*, void*, void* result, void*) {
    if (!in_arena()) return HOOK_CONTINUE;
    *static_cast<BOOL*>(result) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_sword_attack(ModContext* ctx, void* args, void* result, void* user) {
    if (s_autoSwordCall) return HOOK_CONTINUE;
    return before_cut(ctx, args, result, user);
}
#include "survivors_extended_damage.inc"
#include "survivors_encounters.inc"
#include "survivors_bullbo.inc"
#include "survivors_aeralfos.inc"
#include "survivors_elite_scale.inc"
#include "survivors_bosses.inc"
#include "survivors_boss_tactics.inc"
#include "survivors_stalfos.inc"
#include "survivors_dynalfos.inc"
bool pause_run_actor(bool enemy) {
    if(save_modal_active())return true;
    if (!in_arena() || s_run.phase == Phase::Loading) return false;
    if (enemy && (s_victory || s_deathPending || midna_dialog_busy() || dComIfGp_event_runCheck())) return true;
    if (s_deathPending && !enemy) return false; // Link must finish PROC_DEAD.
    if (ui_visible() || dComIfGp_isPauseFlag()) return true;
    // Native Get Item and the hub door handshake need Link's Execute. In the
    // hub, beforeProc clears CANDOOR every frame; Link must rebuild it even
    // while a requested departure waits for the native event manager.
    return s_run.phase != Phase::Running && (enemy || (s_run.phase != Phase::Treasure && !(s_hub && s_run.phase==Phase::Ready)));
}
HookAction before_process(ModContext*, void* args, void* result, void*) {
    auto* process = mods::arg<void*>(args, 1);
    if (!process || !is_actor_process(process)) return HOOK_CONTINUE;
    auto* actor = static_cast<fopAc_ac_c*>(process);
    const auto id = fopAcM_GetID(actor);
    if(id==s_bossRing.id)return process_boss_ring(actor,args,result);
    auto* enemy = tracked(id);
    auto* pot = tracked_pot(id);
    auto* pickup = tracked_pickup(id);
    auto* arrow = tracked_enemy_arrow(id);
    retain_treasure_actor(actor, args);
    if (in_arena()) {
        const auto* nativeMethods = reinterpret_cast<const process_method_class*>(actor->sub_method);
        if (nativeMethods && mods::arg<process_method_func>(args, 0) == nativeMethods->execute_method) {
            collect_arena_mesh(actor);
            // A moving floor must pause with its rider during upgrade/settings
            // menus. Leave native creation, drawing and unloading untouched.
            if (auto* ferry=tracked_palace_ferry(id); ferry && (ferry->retiring || pause_run_actor(false))) {
                *static_cast<int*>(result)=1;
                return HOOK_SKIP_ORIGINAL;
            }
        }
    }
    if (!enemy && !pot && !pickup && !arrow && (!in_arena() || id != s_player)) return HOOK_CONTINUE;
    const auto* methods = reinterpret_cast<const process_method_class*>(actor->sub_method);
    if (!methods) return HOOK_CONTINUE;
    const auto method = mods::arg<process_method_func>(args, 0);
    if (method == methods->delete_method) {
        clear_zelda_light_orbs(id);clear_king_waves(id);
        s_bossTactics.erase(id);
        if (pickup) { s_pickupIndex.erase(id); pickup->id=kNone; }
        if (arrow) { s_enemyArrowIndex.erase(id); arrow->id=kNone; }
        // Never award XP here: cancellation, unloading and despawning also delete.
        if (enemy) {
            enemy_hud::forget(enemy-s_enemies.data());
            s_enemyIndex.erase(id);
            *enemy = {};
        }
        if (pot) {
            s_potIndex.erase(id);
            pot->id=kNone; pot->initialized=false;
            if (pot->retiring) *pot={};
        }
        return HOOK_CONTINUE;
    }
    if (method != methods->execute_method) return HOOK_CONTINUE;
    if (arrow && (arrow->retiring || pause_run_actor(true))) {
        *static_cast<int*>(result)=1; return HOOK_SKIP_ORIGINAL;
    }
    if (pickup) {
        // Waiting pickups normally freeze with enemies during menus. The
        // victory sweep freezes enemies only; existing loot must still execute.
        const bool waiting=pickup_waiting(*pickup,actor) && !(s_victory && s_run.phase==Phase::Running);
        if (pickup->retiring || pause_run_actor(waiting)) {
            *static_cast<int*>(result)=1; return HOOK_SKIP_ORIGINAL;
        }
        move_pickup(*pickup,actor);
        if (pickup->retiring) { *static_cast<int*>(result)=1; return HOOK_SKIP_ORIGINAL; }
    }
    if (pot) pot->initialized=true;
    if (enemy || arrow) s_processFrames.push_back({args, id});
    if (enemy) initialize_enemy(enemy,actor);
    if (enemy && enemy->dormant && enemy->profile == fpcNm_E_OC_e) {
        auto* guard = static_cast<daE_OC_c*>(actor);
        guard->mMoveRange = 0; guard->field_0x6e3 = 1; // No social alert/idle search.
        if (guard->getActionMode() <= 3) guard->setActionMode(0, 0);
    }
    if (enemy) { engage_enemy(actor, *enemy); track_enemy_facing(actor); }
    if ((enemy && enemy->retiring) || (pot && pot->retiring) || pause_run_actor(enemy != nullptr || pot != nullptr)) {
        *static_cast<int*>(result) = 1;
        return HOOK_SKIP_ORIGINAL;
    }
    if(enemy)enemy_hud::observe(enemy-s_enemies.data(),actor,enemy->initialized&&!enemy->retiring&&!enemy->credited&&!enemy->dormant);
    if(enemy && boss_tactics_owned(*enemy))s_bossTactics[id].stagger.tick();
    if (enemy && extended_enemy(enemy->profile) && !update_extended_enemy(actor,*enemy)) {
        *static_cast<int*>(result)=1; return HOOK_SKIP_ORIGINAL;
    }
    if(arrow)update_boss_projectile(actor,false);
    if(enemy) {
        before_boss_tactics(*enemy,actor);
        advance_elite_cooldowns(actor,*enemy);
        if(enemy->specialCooldown)--enemy->specialCooldown;
        if(enemy->summonCooldown)--enemy->summonCooldown;
    }
    if(enemy || arrow)s_processFrames.back().executed=true;
    return HOOK_CONTINUE;
}
void after_process(ModContext*, void* args, void*, void*) {
    // Only matched Execute callbacks may inspect an actor. Delete callbacks can
    // return after freeing it, and nested methods must not consume another frame.
    if (s_processFrames.empty() || s_processFrames.back().args != args) return;
    const auto frame = s_processFrames.back(); s_processFrames.pop_back();
    if (!in_arena() || s_victory || s_run.phase != Phase::Running || transitioning()) return;
    auto* enemy = tracked(frame.id);
    auto* actor = actor_by_id(frame.id);
    if(frame.executed && actor && !enemy && active_enemy_arrow(frame.id))update_boss_projectile(actor,true);
    scale_elite_colliders(enemy,actor);
    finish_extended_targets(enemy,actor);
    if (enemy && actor) {
        if(frame.executed)after_boss_tactics(*enemy,actor);
        confine_arena_actor(actor, enemy->elite ? 80.f : 35.f);
        if (enemy->elite && actor->health > 0 && enemy->age % 15 == 0) {
            cXyz floor = actor->current.pos;
            if (loot_floor_position(floor)) enemy->lootPosition = floor;
        }
        // Extended AI can credit/retire a lethal hit before returning here.
        // Observe that final HP loss while the pre-Execute baseline still exists.
        enemy_hud::observe(enemy-s_enemies.data(),actor,enemy->initialized&&!enemy->dormant&&
            ((!enemy->retiring&&!enemy->credited)||actor->health<=0));
        credit_death(*enemy, actor);
    }
}
HookAction before_drop(ModContext*, void*, void* result, void*) {
    if (!in_arena() || s_allowDrop) return HOOK_CONTINUE;
    *static_cast<fpc_ProcID*>(result) = kNone;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_game_start(ModContext*, void*, void*, void*) {
    if (s_active && s_saveReady && !mDoRst::isReset() && !mDoRst::isReturnToMenu())
        s_gameStarted = true;
    return HOOK_CONTINUE;
}
HookAction before_opening(ModContext*, void*, void*, void*) {
    if (s_active) reset_runtime();
    return HOOK_CONTINUE;
}
HookAction before_stage(ModContext*, void* args, void*, void*) {
    if (!s_active || !s_gameStarted || mDoRst::isReset() || mDoRst::isReturnToMenu()) return HOOK_CONTINUE;
    // gameStart and the subsequent new-file F_SP108 override must both target
    // the arena. The earlier opening/title request must remain untouched.
    if(s_hub && s_hubDoorStarted && in_arena()) {
        s_map=s_requestedMap; s_hub=false; s_arrivingRun=true;
        s_hubDeparture=s_hubDoorStarted=false; s_hubDoor=kNone;
        clear_arena_geometry();
        update_survivors_equipment_chest(false,{},0);
        update_survivors_pedestal(false);
    } else if (in_arena() && !s_restart && s_run.phase != Phase::Loading) return HOOK_SKIP_ORIGINAL;
    dComIfGs_setRestartRoomParam(0);
    mods::arg_ref<const char*>(args, 0) = arena().stage;
    mods::arg_ref<s16>(args, 1) = arena().point;
    mods::arg_ref<s8>(args, 2) = arena().room;
    mods::arg_ref<s8>(args, 3) = 0;
    return HOOK_CONTINUE;
}
void after_draw(ModContext*, void*, void*, void*) {
    if (!in_arena() || transitioning()) return;
    for (auto& orb : s_orbs) {
        if (!orb.value) continue;
        cXyz size(10, 16, 10); csXyz angle(0, static_cast<s16>(s_run.ticks * 300), 0);
        dDbVw_drawCubeOpa(orb.position, size, angle, orb.merged ? GXColor{255,70,70,255} : GXColor{90,235,255,255});
    }
    for (const auto& drop:s_expMagnets) {
        if (!drop.active) continue;
        cXyz size(25,40,25); csXyz angle(0,static_cast<s16>(s_run.ticks*300),0);
        cXyz pos=drop.position+cXyz(0,8*cM_ssin(s16(s_run.ticks*700)),0);
        dDbVw_drawCubeOpa(pos,size,angle,GXColor{255,210,45,255});
    }
    draw_lightning();
    draw_loot();
    draw_victory_wave();
    draw_ranged();
    draw_arena_barriers();

}
void label(const char* text, float x, float y, float size, float width=550) {
    auto* font = mDoExt_getMesgFont(); if (!font) return;
    J2DPrint shadow(font, JUtility::TColor(0,0,0,230), JUtility::TColor(0,0,0,230));
    shadow.setFontSize(size, size);
    shadow.printReturn(text, width, size + 8, HBIND_CENTER, VBIND_TOP, x - width*.5f + 1, y + 1, 255);
    J2DPrint print(font, JUtility::TColor(180,245,255,255), JUtility::TColor(255,240,195,255));
    print.setFontSize(size, size);
    print.printReturn(text, width, size + 8, HBIND_CENTER, VBIND_TOP, x - width*.5f, y, 255);
}
#include "survivors_prices.inc"
#include "survivors_feedback_draw.inc"
#include "survivors_boss_hud.inc"
#include "survivors_chest_hud.inc"
#include "survivors_gear_notice.inc"
void draw_hud(ModContext*, void* args, void*, void*) {
    // Native Collection, ring/map submenus and their opening/closing transitions
    // are not UI-service documents. Suppress every Survivors overlay together.
    if(save_modal_active() || dMeter2Info_getWindowStatus()!=0 || dMeter2Info_getPauseStatus()!=0) return;
    const bool preview=s_hudEditorWindow!=0;
    if(transitioning() || (!preview && (dComIfGp_isPauseFlag() || dScnPly_c::isPause())))return;
    if(!preview && survivors_hub_active()) {draw_hub_labels();return;}
    if (!preview && (!in_arena() || ui_visible() || s_run.phase == Phase::Loading || s_run.phase == Phase::Ready)) return;
    auto* graf = dComIfGp_getCurrentGrafPort(); if (!graf) return;
    graf->setup2D();
    if (!preview && (s_deathPending || s_run.phase==Phase::Defeat)) return;
    if(!preview) {draw_chest_prices();draw_horde_portal_labels();draw_combat_feedback();}
    const float x = mDoGph_gInf_c::getMinXF() + mDoGph_gInf_c::getWidthF() * 0.5f;
    if(preview || s_gearNoticeTicks)draw_gear_notice(preview);
    if (!preview && s_victory) { draw_victory_hud(x); graf->setup2D(); return; }
    if(!preview)enemy_hud::draw();
    char text[180];
    draw_run_rank_hud(x,preview);
    const auto drawText=[&](HudPart part,float size,float offset=0.f) {
        const auto p=hud_position(part,x);
        label(text,p.x,p.y+offset*p.scale,size*p.scale,550*p.scale);
    };
    std::snprintf(text,sizeof(text),"Level %u",s_run.level);drawText(HudPart::Level,14);
    std::snprintf(text,sizeof(text),"Score %u",s_run.score());drawText(HudPart::Score,14);
    std::snprintf(text,sizeof(text),"XP %u / %u",s_run.xp,s_run.required_xp());drawText(HudPart::ExpText,14);
    // Text enables direct TEX0; reset GX before submitting untextured quads.
    graf->setup2D();
    const float fraction = preview ? .6f : std::min(1.0f, float(s_run.xp) / float(s_run.required_xp()));
    const auto xp=hud_position(HudPart::ExpBar,x);
    const auto oldTL = graf->mColorTL, oldTR = graf->mColorTR;
    const auto oldBR = graf->mColorBR, oldBL = graf->mColorBL;
    graf->setColor(JUtility::TColor(12, 25, 32, 210));
    graf->fillBox(JGeometry::TBox2<f32>(xp.x-160*xp.scale,xp.y,xp.x+160*xp.scale,xp.y+6*xp.scale));
    graf->setColor(JUtility::TColor(80, 220, 245, 240));
    graf->fillBox(JGeometry::TBox2<f32>(xp.x-160*xp.scale,xp.y,xp.x+(-160+320*fraction)*xp.scale,xp.y+6*xp.scale));
    graf->setColor(oldTL, oldTR, oldBR, oldBL);
    const float bossOffset = draw_boss_hud(mods::arg<dMeter2Draw_c*>(args, 0), x, preview) ? 54.0f : 0.0f;
    if (preview || (!s_chestsExhausted && s_run.chest_rewards_left(potion_slot() >= 0)))
        draw_next_chest(mods::arg<dMeter2Draw_c*>(args, 0), x, bossOffset);
    else {
        std::snprintf(text, sizeof(text), "No chest rewards available");
        drawText(HudPart::Info,14,bossOffset);
    }
    float effectOffset=22+bossOffset;
    const bool fateTooltips=show_fate_tooltips();
    for (unsigned i=0;i<unsigned(FateEffect::Count);++i) {
        const auto remaining=s_run.fate_ticks(FateEffect(i));
        if (!remaining) continue;
        std::snprintf(text,sizeof(text),"%s  %us",fate_names[i],(remaining+ticks_per_second-1)/ticks_per_second);
        drawText(HudPart::Info,17,effectOffset);
        if(fateTooltips) {
            std::snprintf(text,sizeof(text),"%s",fate_tooltips[i]);
            drawText(HudPart::Info,13,effectOffset+18);
        }
        effectOffset+=fateTooltips?38:22;
    }
    if (s_spawnFailures >= 10) {
        std::snprintf(text,sizeof(text),"Waiting for a clear enemy spawn position");
        drawText(HudPart::Info,13,effectOffset);
    }
    graf->setup2D();
}

void select_upgrade(ModContext*, void* data) {
    const unsigned choice = *static_cast<unsigned*>(data);
    if (s_run.phase != Phase::Upgrade || choice >= 3) return;
    const auto selected = s_run.offers[choice];
    s_run.maximum_hearts = dComIfGs_getMaxLife() / 5;
    const unsigned oldHearts=s_run.rank(Upgrade::Heart);
    const unsigned bonus = s_run.upgrade_rupees(selected, s_run.offer_rarities[choice]);
    if (!s_run.choose(choice)) return;
    if (selected==Upgrade::MasterSword) {
        if(auto* link=daAlink_getAlinkActorClass())apply_treasure_inventory(link,ChestReward::MasterSword,s_run,0);
        sync_survivors_inventory();
    }
    if (selected == Upgrade::Heart)
        dComIfGs_setMaxLife(std::min(100u,unsigned(dComIfGs_getMaxLife())+5*(s_run.rank(Upgrade::Heart)-oldHearts)));
    // Include newly gained heart containers and each queued level's choice.
    if (!s_run.greed()) dComIfGs_setLife(dComIfGs_getMaxLifeGauge());
    if (bonus) dComIfGs_setRupee(std::min<unsigned>(dComIfGs_getRupeeMax(), dComIfGs_getRupee() + s_run.gain_rupees(bonus)));
    close_window();
    s_run.level_up(); // Preserve queued levels; do not resume combat between choices.
}
void begin_run() {
    if (s_run.phase != Phase::Ready || !s_arenaMeshReady || s_restart || s_progressSaving) return;
    clear_zelda_light_orbs();clear_king_waves();
    s_enemyHitSources.clear(); s_enemyContactFrames.clear(); s_bossTactics.clear();
    reset_ending();
    load_option_config();
    close_window(); clear_attacks(); clear_loot(); prepare_save();
    dMeter2Info_setSword(dItemNo_SWORD_e, false); // Refresh live equipment after a previous run.
    const auto seed = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    begin_enemy_archives();
    reset_elite_encounters();
    s_bossHudTarget = kNone;
    clear_combat_feedback();
    enemy_hud::reset();
    s_run.start(seed, s_options, s_records.talents);s_run.equip_gear(s_gear);clear_gear_notices(); reset_gale_charges();
    begin_survivors_integrity(s_run.options.allow_cheats,s_run.modded);
    dComIfGs_setMaxLife(s_run.maximum_hearts*5); dComIfGs_setLife(s_run.maximum_hearts*4);
    s_chestDelay += s_run.next_random() % (10 * ticks_per_second + 1);
    s_resultsStored = false; s_cooldowns = {}; s_spawnCooldown = 30;
    s_swordAction = SwordAction::None; s_meleeCooldown = 0; s_wavePending = false;
    s_comboStep = s_comboTimeout = 0;
    begin_starting_item_choice();
}
void start_run(ModContext*, void*) {
    if(!s_hub || s_run.phase!=Phase::Ready || s_hubDeparture || s_progressSaving ||
       s_restart || !s_arenaMeshReady || !map_available(s_requestedMap) || !actor_by_id(s_hubDoor))return;
    s_hubDeparture=true;s_hubDoorStarted=false;s_hubDoorTicks=0;
    close_window(); // Save and fully hide the UI before ordering the native door event.
}
void continue_endless(ModContext*,void*) {
    if(s_run.phase!=Phase::Victory || s_run.endless || !s_resultsStored || s_restart ||
       s_saveRequested || s_savePrompt!=kNone || s_progressSaving || transitioning())return;
    // Retry staging the victory checkpoint before resuming. Never double-award it.
    const auto bytes=s_records.encode();
    s_storeFailed=svc_save->set_blob(mod_ctx,kBlob,bytes.data(),bytes.size())!=MOD_OK;
    if(s_storeFailed){s_refreshChoices=s_window;close_window();return;} // Rebuild with the save-error message.
    if(!s_run.continue_endless())return;
    begin_enemy_archives();
    close_window();reset_ending();
    s_resultsStored=false;s_spawnCooldown=0;s_spawnFailures=0;
    reset_elite_encounters();
    for(unsigned i=0;i<timed_elite_count;++i)
        s_eliteEncounters[i].respawnAt=(s_run.endless_start/elite_interval+i+1)*elite_interval;
    const bool wasModded=s_run.modded;
    begin_survivors_integrity(s_run.options.allow_cheats,s_run.modded);
    s_run.modded=s_run.modded||wasModded;
}
// Results return to the hub; Midna's restart bypasses it and starts on arrival.
void replay(ModContext*, void*) { close_window(); s_requestedMap = s_map; s_arrivingRun=false; s_restart = true; }
void restart_run(ModContext*, void*) { replay(nullptr,nullptr); s_arrivingRun=true; }
void reload_run_scene() {
    end_survivors_integrity();
    reset_ending();
    // Always reload to destroy old actors, pending spawns, collision and native
    // damage state. Keep direct restart intent until the destination is ready.
    retire_enemies(); clear_attacks(); clear_loot(); clear_arena_geometry();
    s_hub=!s_arrivingRun; s_hubMenu=HubMenu::None; s_hubDeparture=s_hubDoorStarted=false;
    s_hubDoor=kNone; s_hubSaveRequested=false;
    s_map=s_arrivingRun?s_requestedMap:0;
    prepare_save();
    dComIfGp_setNextStage(arena().stage, arena().point, arena().room, 0);
    s_run = {}; s_orbs = {}; s_restart = false; s_player = kNone;
    s_resultsStored = false; s_spawnFailures = 0;
}
#include "survivors_post_run_save.inc"
void save_results(ModContext*, void*) {
    if(s_saveRequested || s_savePrompt!=kNone)return;
    s_storeFailed=false; s_saveStartTicks=0;
    close_window(); s_saveRequested=true;
    svc_log->info(mod_ctx,"Survivors post-run save: queued");
}
ModResult button(UiElementHandle pane, const char* text, UiPressedFn callback, void* data = nullptr,
                 const char* help = nullptr) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.label = text; desc.on_pressed = callback; desc.user_data = data; desc.help_rml = help;
    return svc_ui->pane_add_control(mod_ctx, pane, &desc, nullptr);
}
#include "survivors_choice_ui.inc"
#include "survivors_choice_actions.inc"
#include "survivors_chest_ui.inc"
#include "survivors_powerups.inc"
#include "survivors_talents.inc"
#include "survivors_results.inc"
#include "survivors_scores_ui.inc"
#include "survivors_hud_editor.inc"
#include "survivors_maps_ui.inc"
#include "survivors_debug_ui.inc"
#include "survivors_settings_config.inc"
#include "survivors_gear_ui.inc"
#include "survivors_gear_collection.inc"
#include "survivors_hub.inc"

ModResult build_window(ModContext*, UiWindowHandle, UiElementHandle left, UiElementHandle right,
                       void*, ModError*) {
    if (s_run.phase == Phase::TreasureChoice) return build_treasure_choices(left, right);
    if (s_run.phase == Phase::Upgrade) return build_upgrade_choices(left, right);
    if (s_run.phase == Phase::Ready) return build_arena_choices(left, right);
    return build_results(left,right);
}
void open_window() {
    if (s_window || ui_visible() || (s_run.phase==Phase::Ready && s_hubMenu==HubMenu::None)) return;
    s_options.debug.clamp();s_run.options.debug.clamp();
    UiTabDesc tabs[5] = {UI_TAB_DESC_INIT, UI_TAB_DESC_INIT, UI_TAB_DESC_INIT, UI_TAB_DESC_INIT, UI_TAB_DESC_INIT};
    auto& tab=tabs[0];
    tab.title = s_run.phase == Phase::TreasureChoice ? (s_run.starting_item_pending ? "Choose a starting item" : "Choose a treasure") :
        s_run.phase == Phase::Upgrade ? "Choose an upgrade" : "Hyrule Survivors";
    if(s_run.phase==Phase::Victory || s_run.phase==Phase::Defeat)tab.title="Results";
    tab.build = build_window;
    UiWindowDesc desc = UI_WINDOW_DESC_INIT;
    desc.tabs = tabs; desc.tab_count = 1;
    if (s_run.phase==Phase::Ready) {
        load_option_config();
        if(s_hubMenu==HubMenu::Talents) {
            tab.title="Talents";tab.build=build_talents;tab.update=update_talent_tab;
            tabs[1].title="Settings";tabs[1].build=build_run_settings;
            desc.tab_count=2;
        } else {
            tab.title="Maps";tab.update=update_arena_tab;
            tabs[1].title="Difficulty";tabs[1].build=build_difficulty;
            tabs[2].title="Settings";tabs[2].build=build_run_settings;
            desc.tab_count=3;
            if(debug_enabled){tabs[3].title="Debug";tabs[3].build=build_debug;tabs[3].update=update_debug_tab;desc.tab_count=4;}
        }
    }
    if (s_run.phase == Phase::TreasureChoice || s_run.phase == Phase::Upgrade) desc.rcss = kTreasureStyle;
    if (s_run.phase == Phase::Ready) desc.rcss = kArenaStyle;
    if (s_run.phase == Phase::Victory || s_run.phase == Phase::Defeat) desc.rcss = kResultsStyle;
    desc.on_closed = [](ModContext*, UiWindowHandle handle, void*) {
        if(s_hubClosingWindow==handle)s_hubClosingWindow=0;
        if (s_window == handle) { close_debug_notice(); s_window = 0; s_menuDocument = nullptr; hub_menu_closed(); }
        if (s_refreshChoices==handle) { s_refreshChoices=0; return; }
        if (s_run.phase == Phase::TreasureChoice && s_chosenTreasure == ChestReward::Count)
            dismiss_treasure_choice();
    };
    if (svc_ui->window_push(mod_ctx, &desc, &s_window) != MOD_OK)
        svc_log->warn(mod_ctx, "Hyrule Survivors: could not open run window");
    else if (s_run.phase == Phase::Ready) remember_menu_document(false);
}
ModResult activated(void*, ModError*) { s_active = true; reset_runtime(); return MOD_OK; }
ModResult deactivated(void*, ModError*) { s_active = false; reset_runtime(); return MOD_OK; }
ModResult new_save(void*, ModError*) {
    reset_runtime(); s_records = {}; prepare_save();
    if(!store_gear(GearInventory{}))return MOD_UNAVAILABLE;
    s_saveReady = true;
    const auto bytes = s_records.encode();
    svc_save->set_blob(mod_ctx, kBlob, bytes.data(), bytes.size());
    return MOD_OK;
}
ModResult loaded(void*, ModError*) {
    reset_runtime(); prepare_save(); s_saveReady = true; return MOD_OK;
}
ModResult reset(void*, ModError*) { reset_runtime(); return MOD_OK; }
#include "survivors_mode.inc"
#include "survivors_music.inc"
} // namespace

ModResult build_survivors_settings(ModContext* ctx,UiWindowHandle window,UiElementHandle left,UiElementHandle right,void* data,ModError* error) {
    load_option_config();
    return build_run_settings(ctx,window,left,right,data,error);
}
bool survivors_hub_active() {return s_hub && in_arena() && s_run.phase==Phase::Ready && !s_restart;}
void survivors_open_talents() {
    if(!survivors_hub_active() || !playable() || s_hubDeparture || s_progressSaving)return;
    s_hubMenu=HubMenu::Talents;open_window();
}
bool survivors_active() { return s_active; }
bool survivors_switch_attack_lock() {
    return s_run.phase==Phase::Running && s_run.options.switch_attack_lock && playable();
}
bool survivors_mobility_override(ModeSetting setting,int64_t& value) {
    return talent_mode_override(s_run.talents,setting,value);
}
float survivors_sprint_multiplier() { return s_run.talents.sprint_multiplier(); }
bool survivors_dual_wield() {
    return in_arena() && s_run.owns(ChestReward::OrdonSword) && !shield_pose(daAlink_getAlinkActorClass());
}
bool survivors_arena_active() { return in_arena(); }
bool survivors_save_screen_active() { return save_modal_active(); }
bool survivors_dark_link_active() {
    return in_arena() && s_run.dark_active() && (s_run.phase == Phase::Running ||
        s_run.phase == Phase::Upgrade || s_run.phase == Phase::TreasureChoice || s_run.phase == Phase::Treasure);
}
void update_survivors() {
    enemy_hud::refresh();
    update_survivors_registration();
    update_survivors_music();
    update_survivors_integrity(s_run.modded);
    update_enemy_archives();
    cleanup_enemies();
    update_boss_ring();
    cleanup_pots(); cleanup_pickups();
    if (s_treasure.retiring) retire_treasure_actor();
    if (!s_active || !s_gameStarted) return;
    enforce_survivors_bindings();
    if(update_hub_save())return;
    if(update_post_run_save())return;
    if (!in_arena() || transitioning()) { enemy_hud::reset(); update_survivors_equipment_chest(false,{},0); close_gear_window(); reset_ending(); reset_midna_dialog(); close_powerup_window(); clear_attacks(); clear_loot(); clear_enemy_effects(); s_player = kNone; s_warmup = 0; return; }
    auto* link = daAlink_getAlinkActorClass();
    if (!link || !dComIfGp_getStageStagInfo()) return;
    if (s_restart) {
        reload_run_scene();
        return;
    }
    if (s_player != fopAcM_GetID(link)) {
        reset_midna_dialog();
        s_player = fopAcM_GetID(link); s_run.phase = Phase::Loading; s_warmup = 0;
        s_arenaSafe = s_arenaSpawn = false;
    }
    if (fpcM_IsCreating(s_player)) return;
    if (update_ending(link)) return;
    if (!s_hub && !s_resultsStored) prepare_victory_wave();
    update_midna_dialog();
    if (s_restart) return; // Reload next tick without briefly opening the results UI.
    update_arena_safety(link);
    if (s_run.phase == Phase::Loading) {
        // Never freeze a falling or partially initialized Link in the preview.
        if (!playable() || !arena_grounded(link)) { s_warmup=0; return; }
        if (++s_warmup < 15) return;
        load_records(); s_run.phase = Phase::Ready;
        if(s_arrivingRun && s_arenaMeshReady) {s_arrivingRun=false;begin_run();}
    }
    if(s_arrivingRun && s_run.phase==Phase::Ready && s_arenaMeshReady && playable()) {
        s_arrivingRun=false; begin_run();
    }
    if(s_hub)update_hub(link);
    open_powerup_window();
    if (s_run.phase == Phase::TreasureChoice) update_treasure_choice(link);
    if (s_run.phase == Phase::Treasure) update_treasure_demo(link);
    if ((s_run.phase == Phase::TreasureChoice && s_chosenTreasure == ChestReward::Count) ||
        s_run.phase == Phase::Ready || s_run.phase == Phase::Upgrade ||
        s_run.phase == Phase::Victory || s_run.phase == Phase::Defeat) open_window();
}
ModResult initialize_survivors(ModError* error) {
    ModResult result = initialize_survivors_integrity(error);
    if(result!=MOD_OK)return result; // Preserve the initializer's specific error.
    result=initialize_save_pointer(error);
    if(result!=MOD_OK)return result;
    result=initialize_option_config();
    if(result!=MOD_OK)return mods::set_error(error,result,"failed to initialize Hyrule Survivors settings");
    result=enemy_hud::initialize();
    if(result!=MOD_OK)return mods::set_error(error,result,"failed to initialize Survivors enemy HUD settings");
    result=initialize_hud_config();
    if(result!=MOD_OK)return mods::set_error(error,result,"failed to initialize Survivors HUD settings");
    result=initialize_midna_dialog();
    if(result!=MOD_OK)return mods::set_error(error,result,"failed to initialize Hyrule Survivors Midna dialog");
    initialize_menu_handoff();
#define PRE(H, F) do { result = mods::hook_add_pre<H>(svc_hook, F); \
    if(result!=MOD_OK)return mods::set_error(error,result,"failed to install Hyrule Survivors pre-hook " #H); } while(false)
#define POST(H, F) do { result = mods::hook_add_post<H>(svc_hook, F); \
    if(result!=MOD_OK)return mods::set_error(error,result,"failed to install Hyrule Survivors post-hook " #H); } while(false)
    // Resource retention is optional: an unsupported host detour must never
    // prevent Dawnlight or Survivors from loading with normal native resources.
    s_enemyArchivesAvailable=mods::hook_add_post<SurvivorsArchiveReady>(svc_hook,after_enemy_archive_ready)==MOD_OK;
    if(!s_enemyArchivesAvailable)svc_log->warn(mod_ctx,
        "Survivors archive retention unavailable; using native resource loading");
    PRE(SurvivorsArenaActor, before_arena_stage_actor);
    PRE(SurvivorsPalaceFerrySuspend, before_palace_ferry_suspend);
    PRE(SurvivorsPalaceFerryMove, before_palace_ferry_move);
#include "survivors_roster_hooks.inc"
    PRE(SurvivorsEnemySpeed, before_enemy_speed); POST(SurvivorsEnemySpeed, after_enemy_facing);
    PRE(SurvivorsEnemySpeed,before_boss_path); // Final position constraints after all movement boosts.
    PRE(SurvivorsEnemySpeed,before_dynalfos_path);
    PRE(SurvivorsEnemyDamage, before_enemy_damage);
    PRE(SurvivorsPotionDrink, before_potion_drink); POST(SurvivorsPotionDrink, after_potion_drink);
    PRE(SurvivorsPotionHeal, before_potion_heal);
    PRE(SurvivorsFateRupees, before_fate_rupees);
    PRE(SurvivorsFateSprint, before_fate_sprint);
    PRE(SurvivorsFairyParticles, before_fate_fairy_particles);
    PRE(SurvivorsPad, before_survivors_pad); POST(SurvivorsPad, after_survivors_pad);
    POST(SurvivorsCameraType, after_survivors_camera_tools);
    PRE(SurvivorsSaveOpen,before_save_screen_open); PRE(SurvivorsSaveDelete,before_save_screen_delete);
    PRE(SurvivorsSavePointer,before_save_pointer); POST(SurvivorsSavePointer,after_save_pointer);
    PRE(SurvivorsSaveMapMove,before_save_gameplay_menu); PRE(SurvivorsSaveMapKeys,before_save_gameplay_menu);
    PRE(SurvivorsSaveMenuKeys,before_save_gameplay_menu);
    PRE(SurvivorsCamera, before_run_camera); POST(SurvivorsCamera, after_run_camera);
    PRE(SurvivorsArenaExit, before_arena_exit);
    PRE(SurvivorsArenaRestart, before_arena_restart);
    POST(SurvivorsBossAim,after_boss_aim);
    PRE(SurvivorsProcess, before_contact_owner_delete);
    PRE(SurvivorsProcess, before_process); POST(SurvivorsProcess, after_process);
    POST(SurvivorsPlayer, after_player); POST(SurvivorsDraw, after_draw);
    PRE(SurvivorsPlayer, before_player_contacts);
    PRE(SurvivorsPlayer, before_player_bindings);
    PRE(SurvivorsPlayer, before_chest_action);
    PRE(SurvivorsPlayer, before_horde_portal_action);
    PRE(SurvivorsHordePortal, before_horde_portal_execute);
    PRE(SurvivorsPlayer, before_midna_call);
    PRE(SurvivorsMidnaOrder, before_midna_order);
    PRE(SurvivorsMidnaFlow, before_midna_flow);
    PRE(SurvivorsEnemyContact,before_enemy_contact);
    PRE(SurvivorsBlock, before_player_contacts);
    PRE(SurvivorsBlock, before_shield_block); POST(SurvivorsBlock, after_player_damage); PRE(SurvivorsGuard, before_survivors_guard);
    PRE(SurvivorsBulblinHit, before_bulblin_hit); POST(SurvivorsBulblinHit, after_bulblin_hit);
    PRE(SurvivorsBulblinAggro, before_bulblin_aggro);
    PRE(SurvivorsEnemyArrow, before_enemy_arrow); POST(SurvivorsEnemyArrow, after_enemy_arrow);
    PRE(SurvivorsConcurrentHit,before_boss_attack_safety);
    PRE(SurvivorsConcurrentHit, before_enemy_attack_contact); POST(SurvivorsConcurrentHit, after_enemy_attack_contact);
    PRE(SurvivorsConcurrentHit, before_concurrent_hit);
    POST(SurvivorsConcurrentHit, after_player_contact);
    PRE(SurvivorsProcess, before_enemy_attack_process);
    POST(SurvivorsEnemyArrow, after_enemy_attack_spawn);
    // Post callbacks run in registration order. Boss aim/volley setup needs
    // the child's ownership and attack budget established by the two above.
    POST(SurvivorsEnemyArrow, after_boss_child_spawn);
    PRE(SurvivorsBokoblinHit, before_enemy_hit); POST(SurvivorsBokoblinHit, after_enemy_hit);
    PRE(SurvivorsBokoblinTargets, before_bokoblin_targets);
    PRE(SurvivorsRatRecovery, before_rat_recovery);
    POST(SurvivorsRatRecovery, after_rat_recovery);
    PRE(SurvivorsBokoblinAggro, before_bokoblin_aggro);
    PRE(SurvivorsBokoblinWait, before_bokoblin_wait);
    PRE(SurvivorsPotBreak, before_pot_break);
    PRE(SurvivorsKeyPickup, before_key_pickup);
    PRE(SurvivorsEliteMinimap, before_survivors_minimap);
    POST(SurvivorsSaveSlotInfo, after_survivors_save_slot);
    POST(SurvivorsSaveSlotDraw, after_survivors_save_slot_draw);
    POST(SurvivorsEliteMinimap, after_elite_minimap);
    PRE(SurvivorsMinimapCursor, before_survivors_minimap_cursor);
    PRE(SurvivorsKeeseAggro, before_keese_aggro);
    PRE(SurvivorsRatAggro, before_rat_aggro);
    PRE(SurvivorsKeeseHit, before_enemy_hit); POST(SurvivorsKeeseHit, after_enemy_hit);
    PRE(SurvivorsRatHit, before_enemy_hit); POST(SurvivorsRatHit, after_enemy_hit);
    PRE(SurvivorsMovement, before_auto_movement); POST(SurvivorsMovement, after_auto_movement);
    PRE(SurvivorsLedgeJump, before_survivors_ledge_jump); POST(SurvivorsLedgeJump, after_survivors_ledge_jump);
    POST(SurvivorsPosition, after_arena_position);
    POST(SurvivorsPosition, after_auto_position);
    POST(SurvivorsPosition, after_locomotion_position);
    PRE(SurvivorsLocomotionModel, before_locomotion_model); POST(SurvivorsLocomotionModel, after_locomotion_model);
    PRE(SurvivorsRangedAim, before_ranged_aim);
    PRE(SurvivorsBowAction, before_ranged_native);
    PRE(SurvivorsBowNext, before_ranged_native);
    PRE(SurvivorsBallAction, before_ranged_native);
    PRE(SurvivorsBallNext, before_ranged_native);
    POST(SurvivorsScrollVisible, after_powerup_scroll_visible);
    POST(SurvivorsGearScreen, after_gear_collection_screen);
    PRE(SurvivorsGearDraw, before_gear_collection_draw);
    POST(SurvivorsGearDraw, after_gear_collection_draw);
    POST(SurvivorsCollectionWait, after_gear_collection_action);
    POST(SurvivorsCollectionClick, after_gear_collection_action);
    PRE(SurvivorsCollectionDelete, before_gear_collection_delete);
    PRE(SurvivorsCollectionWait, before_powerup_collection_wait);
    POST(SurvivorsCollectionWait, after_powerup_collection_action);
    POST(SurvivorsCollectionClick, after_powerup_collection_action);
    PRE(SurvivorsCollectionDelete, before_powerup_collection_delete);
    PRE(SurvivorsScrollTexture, before_powerup_icon);
    PRE(SurvivorsRingCursor, before_ring_assignment);
    PRE(SurvivorsRingAssign, before_ring_assignment);
    PRE(SurvivorsRingMix, before_ring_assignment);
    PRE(SurvivorsBind, before_item_binding);
    POST(SurvivorsExplain, after_item_explain);
    PRE(SurvivorsTreasureText, before_treasure_text_update);
    PRE(SurvivorsBombThrow, before_ranged_native);
    PRE(SurvivorsRangedCancel, before_ranged_native);
    PRE(SurvivorsTreasureDraw, before_key_draw);
    PRE(SurvivorsTreasureDraw, before_treasure_draw);
    PRE(SurvivorsGetItem, before_treasure_get); POST(SurvivorsGetItem, after_treasure_get);
    PRE(SurvivorsGameOver, before_game_over);
    PRE(SurvivorsDeath, before_death); PRE(SurvivorsManualAttack, before_cut);
    PRE(SurvivorsManualCharge, before_manual_charge);
    PRE(SurvivorsChargeInit, before_cut); PRE(SurvivorsChargeMoveInit, before_cut);
    PRE(SurvivorsNativeSpin, before_sword_attack); PRE(SurvivorsJumpTarget, before_jump_target);
    PRE(SurvivorsSwordAttack, before_sword_attack); PRE(SurvivorsDoor, before_hub_door);
    POST(SurvivorsPower, after_power);
    POST(SurvivorsSwordCollision, after_sword_collision);
    PRE(SurvivorsEnemyNavigationLine, before_enemy_navigation_line);
    PRE(SurvivorsHitMaterial, before_hit_material); POST(SurvivorsHitMaterial, after_hit_material);
    POST(SurvivorsLightSword, after_light_sword);
    POST(SurvivorsJumpStrike, after_jump_strike); POST(SurvivorsJumpStrikeLand, after_jump_strike);
    PRE(SurvivorsJumpStrikeEffect, before_jump_strike_effect);
    POST(SurvivorsSpinMove, after_sword_move); POST(SurvivorsSpinEffect, after_spin_effect);
    PRE(SurvivorsLargeSpin, before_large_spin);
    POST(SurvivorsNormalMove, after_sword_move); POST(SurvivorsFinishMove, after_sword_move);
    PRE(SurvivorsStage, before_stage); POST(SurvivorsHud, draw_hud);
    PRE(SurvivorsGameStart, before_game_start); PRE(SurvivorsOpening, before_opening);
    PRE(SurvivorsDrops, before_drop); PRE(SurvivorsDirectDrops, before_drop);
#undef PRE
#undef POST
    return survivors_enabled()?register_survivors_mode(error):MOD_OK;
}
void shutdown_survivors() {
    enemy_hud::shutdown();
    shutdown_boss_ring();
    stop_survivors_music();
    shutdown_survivors_integrity();
    before_survivors_pad(nullptr,nullptr,nullptr,nullptr);
    s_active = false; reset_runtime(); cleanup_enemies(); cleanup_pots(); cleanup_pickups();
    s_midnaGraph.reset(); s_midnaPrompt.reset(); s_midnaOptions.reset(); s_midnaStart=0;
    s_treasureMessage.reset(); s_itemNameMessage.reset(); s_itemDescriptionMessage.reset();
    if (s_savePrompt != kNone) { d_GameOver_Delete(s_savePrompt); s_savePrompt = kNone; }
    if (s_registered) svc_game_mode->unregister_game_mode(mod_ctx, kMode);
    s_registered = false;
    s_nativeSaveScreens.clear();
    // Loader removes callbacks after shutdown. No native actor profile points at
    // this module; outstanding deletes belong to the scene and remain safe.
}
} // namespace dawnlight
