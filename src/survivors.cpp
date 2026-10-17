#include "survivors.hpp"
#include "survivors_rules.hpp"
#include "enemy_spawner.hpp"
#include "save_state.hpp"
#include "service_imports.hpp"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "SSystem/SComponent/c_math.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_demo_item.h"
#include "d/actor/d_a_e_ba.h"
#include "d/actor/d_a_e_oc.h"
#include "d/actor/d_a_e_ms.h"
#include "d/d_com_inf_game.h"
#include "d/d_debug_viewer.h"
#include "d/d_gameover.h"
#include "d/d_item.h"
#include "d/d_meter2_draw.h"
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
#include "res/Object/AlAnm.h"
#include "d/d_particle_name.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace dawnlight {
namespace {
using namespace survivors;
constexpr char kMode[] = "hyrule_survivors";
// Shared empty-room resource alias; its admission gates are also enabled for this mode.
constexpr char kStage[] = "D_DLBR0";
constexpr char kBlob[] = "hyrule-survivors-records-v1";
constexpr ActorId kNone = fpcM_ERROR_PROCESS_ID_e;
DEFINE_HOOK(&daAlink_c::execute, SurvivorsPlayer);
DEFINE_HOOK(&daAlink_c::setMatrix, SurvivorsRangedAim);
DEFINE_HOOK(&daAlink_c::checkUpperItemActionBow, SurvivorsBowAction);
DEFINE_HOOK(&daAlink_c::checkNextActionBow, SurvivorsBowNext);
DEFINE_HOOK(&daAlink_c::cancelUpperItemReadyAnime, SurvivorsRangedCancel);
DEFINE_HOOK(&daAlink_c::procCoGetItemInit, SurvivorsGetItem);
DEFINE_HOOK(&fpcLf_Draw, SurvivorsTreasureDraw);
DEFINE_HOOK(&daAlink_c::draw, SurvivorsDraw);
DEFINE_HOOK(&daAlink_c::procCoDeadInit, SurvivorsDeath);
DEFINE_HOOK(&daAlink_c::checkDoCutAction, SurvivorsManualAttack);
DEFINE_HOOK(&daAlink_c::checkCutAction, SurvivorsSwordAttack);
DEFINE_HOOK(&daAlink_c::procCutTurn, SurvivorsSpinMove);
DEFINE_HOOK(&daAlink_c::procCutNormal, SurvivorsNormalMove);
DEFINE_HOOK(&daAlink_c::procCutFinish, SurvivorsFinishMove);
DEFINE_HOOK(&daAlink_c::checkCutLargeTurnState, SurvivorsLargeSpin);
DEFINE_HOOK(&fopAcM_orderDoorEvent, SurvivorsDoor);
DEFINE_HOOK(&at_power_check, SurvivorsPower);
DEFINE_HOOK(&dMeter2Draw_c::draw, SurvivorsHud);
DEFINE_HOOK(&dComIfGs_gameStart, SurvivorsGameStart);
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
Run s_run;
Records s_records;
UiWindowHandle s_window = 0;
ActorId s_player = kNone;
fpc_ProcID s_savePrompt = kNone;
unsigned s_spawnCooldown = 0, s_spawnFailures = 0, s_warmup = 0;
enum class SwordAction { None, Normal, Spin };
SwordAction s_swordAction = SwordAction::None;
unsigned s_meleeCooldown = 0;
unsigned s_comboStep = 0, s_comboTimeout = 0;
bool s_autoSwordCall = false;
bool s_wavePending = false, s_allowDrop = false;
std::array<unsigned, 12> s_cooldowns{};
void clear_loot();
void cancel_treasure_demo();
int potion_slot();
void cancel_ranged();
bool queue_ranged(daAlink_c*, Upgrade);
std::array<unsigned, 3> s_choices{0, 1, 2};
struct ProcessFrame { void* args; ActorId id; };
std::vector<ProcessFrame> s_processFrames;

struct Enemy {
    ActorId id = kNone;
    ProfileName profile = 0;
    bool initialized = false, credited = false, retiring = false, elite = false;
    unsigned age = 0;
};
std::array<Enemy, enemy_capacity> s_enemies;
struct Orb { cXyz position{}; unsigned value = 0; };
std::array<Orb, orb_capacity> s_orbs;
struct Attack {
    bool active = false;
    unsigned life = 0, weapon = 0, age = 0;
    bool returning = false, exploding = false;
    ActorId target = kNone;
    unsigned hops = 0, visitedCount = 0;
    std::array<ActorId, 6> visited{};
    float travel = 0, limit = 0;
    cXyz position{}, velocity{};
    float radius = 0;
    dCcD_Stts status{};
    dCcD_Sph collider{};
    std::array<u32, 2> emitters{};
};
// Stable addresses: collision server retains pointers until its next frame.
std::array<Attack, projectile_capacity> s_attacks;
const dCcD_SrcSph kAttackSource = {
    {{0, {{AT_TYPE_NORMAL_SWORD, 3, 0x1a}, {0, 0}, 0}},
     {dCcD_SE_SWORD, 3, dCcG_At_Spl_UNK_1, dCcD_MTRL_NONE, {1}},
     {dCcD_SE_NONE, 0, 0, dCcD_MTRL_NONE, {0}}, {0}},
    {{{0, 0, 0}, 50}},
};

bool in_arena() {
    return s_active && s_gameStarted && std::strcmp(dComIfGp_getStartStageName(), kStage) == 0 &&
        dComIfGp_getStartStageRoomNo() == 51;
}
bool ui_visible() {
    bool value = false;
    svc_ui->is_any_document_visible(mod_ctx, &value);
    return value;
}
bool transitioning() { return dComIfGp_isEnableNextStage() || fopOvlpM_IsPeek(); }
bool playable() {
    return in_arena() && !transitioning() && !dComIfGp_isPauseFlag() &&
        !dScnPly_c::isPause() && !dComIfGp_event_runCheck() && !ui_visible() &&
        dMeter2Info_getGameOverType() == 0;
}
fopAc_ac_c* actor_by_id(ActorId id) {
    fopAc_ac_c* actor = nullptr;
    if (id != kNone && !fpcM_IsCreating(id)) fopAcM_SearchByID(id, &actor);
    return actor;
}
Enemy* tracked(ActorId id) {
    for (auto& e : s_enemies) if (e.id != kNone && e.id == id) return &e;
    return nullptr;
}
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
    cancel_ranged();
    for (auto& a : s_attacks) stop_attack(a);
}
void close_window() {
    const auto handle = s_window; s_window = 0;
    if (handle) svc_ui->window_close(mod_ctx, handle);
}
void retire_enemies() {
    for (auto& e : s_enemies) if (e.id != kNone) e.retiring = true;
}
void cleanup_enemies() {
    for (auto& e : s_enemies) {
        if (e.id == kNone) continue;
        auto* actor = actor_by_id(e.id);
        if (!actor && !fpcM_IsCreating(e.id)) {
            if (e.elite && !e.credited) s_run.elite_spawned = false;
            e = {}; continue;
        }
        // Pending creation is not completion or a kill. Keep its ID until the scene
        // destroys it or the actor is ready to accept the deferred delete request.
        if (e.retiring && actor) svc_actor->delete_actor(mod_ctx, e.id);
    }
}
bool own_collider(const cCcD_Obj* collider) {
    if (!collider) return false;
    for (const auto& a : s_attacks) if (collider == &a.collider) return true;
    return false;
}
void detach_attack_references() {
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
        }
        if (info && own_collider(info->mpCollider)) info->mpCollider = nullptr;
    }
}
void reset_runtime() {
    s_saveReady = false; s_gameStarted = false;
    close_window(); clear_attacks(); clear_loot(); detach_attack_references(); retire_enemies();
    if (s_savePrompt != kNone) {
        d_GameOver_Delete(s_savePrompt); s_savePrompt = kNone;
        dComIfGp_setGameoverStatus(0);
    }
    s_run = {}; s_orbs = {}; s_cooldowns = {};
    s_player = kNone; s_loaded = false; s_resultsStored = false;
    s_storeFailed = false; s_restart = false; s_saveRequested = false;
    s_spawnCooldown = s_spawnFailures = s_warmup = 0;
    s_swordAction = SwordAction::None; s_meleeCooldown = 0; s_wavePending = false;
    s_comboStep = s_comboTimeout = 0;
}
void prepare_save() {
    save_state_set_boss_rush_active(false);
    save_state_set_intro_skipped(false);
    auto* save = dComIfGs_getSaveData();
    save->getPlayer().getPlayerReturnPlace().set(kStage, 51, 0);
    dComIfGs_onEventBit(dSv_event_flag_c::F_0250);
    dComIfGs_onEventBit(dSv_event_flag_c::F_0302);
    dComIfGs_onEventBit(dSv_event_flag_c::M_010);
    dComIfGs_setTransformStatus(TF_STATUS_HUMAN);
    dComIfGs_onItemFirstBit(dItemNo_WEAR_KOKIRI_e);
    dComIfGs_setCollectClothes(KOKIRI_CLOTHES_FLAG);
    dComIfGs_setSelectEquipClothes(dItemNo_WEAR_KOKIRI_e);
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
}
void load_records() {
    if (s_loaded) return;
    s_records = {};
    auto bytes = s_records.encode(); size_t size = bytes.size();
    const auto result = svc_save->get_blob(mod_ctx, kBlob, bytes.data(), &size);
    if (result == MOD_OK) s_records.decode(std::span(bytes.data(), std::min(size, bytes.size())));
    s_loaded = true;
}
void finish(bool won) {
    if (s_run.phase != Phase::Running && s_run.phase != Phase::Upgrade) return;
    s_run.end(won); clear_attacks(); retire_enemies();
    // Keep native Game Over from stealing the result screen. A replay reloads the
    // scene, so no dead Link action or temporary combat state survives it.
    dComIfGs_setLife(dComIfGs_getMaxLifeGauge());
    if (!s_resultsStored) {
        load_records(); s_records.finish(s_run);
        const auto bytes = s_records.encode();
        s_storeFailed = svc_save->set_blob(mod_ctx, kBlob, bytes.data(), bytes.size()) != MOD_OK;
        s_resultsStored = true;
    }
}
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
}
void drop_reward(fopAc_ac_c* actor) {
    const auto drop = s_run.roll_drop();
    if (drop == Run::Drop::None) return;
    const int item = drop == Run::Drop::Heart ? dItemNo_HEART_e :
        drop == Run::Drop::BlueRupee ? dItemNo_BLUE_RUPEE_e : dItemNo_GREEN_RUPEE_e;
    // Native item physics, pickup effects and currency/health handling. Ordinary
    // enemy drop hooks remain blocked, so a death cannot generate two rewards.
    struct Scope { Scope(){s_allowDrop=true;} ~Scope(){s_allowDrop=false;} } scope;
    fopAcM_createItem(&actor->current.pos, item, -1, 51, nullptr, nullptr, 0);
}
void credit_death(Enemy& e, fopAc_ac_c* actor) {
    if (e.credited || e.retiring || !e.initialized || s_run.phase != Phase::Running ||
        !in_arena() || transitioning()) return;
    bool dead = actor->health <= 0;
    if (e.profile == fpcNm_E_BA_e)
        dead = dead || reinterpret_cast<e_ba_class*>(actor)->mIsDying;
    if (e.profile == fpcNm_E_OC_e)
        dead = dead || static_cast<daE_OC_c*>(actor)->getActionMode() == 9;
    if (!dead) return;
    e.credited = true; ++s_run.kills;
    if (!e.elite) drop_reward(actor);
    add_orb(actor->current.pos, e.elite ? 30 : e.profile == fpcNm_E_OC_e ? 3 : 1);
    if (e.elite) finish(true);
}
void update_orbs(daAlink_c* link) {
    const float range = 150.0f + 65.0f * s_run.rank(Upgrade::Magnet);
    for (auto& orb : s_orbs) {
        if (!orb.value) continue;
        const auto target = link->current.pos + cXyz(0, 30, 0);
        cXyz delta = target - orb.position;
        const float distance = delta.abs();
        if (distance < 60) { s_run.xp += orb.value; orb = {}; }
        else if (distance < range) orb.position += delta * (std::min(35.0f, distance) / distance);
    }
}
bool spawn_enemy(daAlink_c* link, bool elite) {
    auto slot = std::find_if(s_enemies.begin(), s_enemies.end(), [](const auto& e) { return e.id == kNone; });
    if (slot == s_enemies.end()) return false;
    const unsigned type = elite ? 0 : s_run.next_random() % (s_run.ticks < 900 ? 2 : 3);
    constexpr std::array<ProfileName, 3> profiles{fpcNm_E_OC_e, fpcNm_E_BA_e, fpcNm_E_MS_e};
    constexpr std::array<u32, 3> parameters{0x00ff0000, 0xffff0f01, 0xffff0000};
    for (unsigned attempt = 0; attempt < 12; ++attempt) {
        const auto yaw = static_cast<s16>(s_run.next_random());
        const float radius = 600.0f + float(s_run.next_random() % 450);
        cXyz pos = link->current.pos + cXyz(cM_ssin(yaw) * radius, 0, cM_scos(yaw) * radius);
        cXyz probe = pos; probe.y += 400;
        dBgS_ObjGndChk ground; ground.SetPos(&probe);
        const float floor = dComIfG_Bgsp().GroundCross(&ground);
        if (!std::isfinite(floor) || floor < -1.0e8f || std::abs(floor - link->current.pos.y) > 180) continue;
        pos.y = floor + (type == 1 ? 180 : 0);
        cXyz from = link->current.pos + cXyz(0, 80, 0), to = pos + cXyz(0, 80, 0);
        dBgS_LinChk wall; wall.Set(&from, &to, link);
        if (dComIfG_Bgsp().LineCross(&wall)) continue;
        ActorId id = kNone;
        const float scale = elite ? 1.4f : 1.0f;
        const ActorSpawnParams params{
            .parameters = parameters[type], .argument = -1, .room_num = 51,
            .position = {pos.x, pos.y, pos.z},
            .angle = {0, static_cast<s16>(yaw + 0x8000), 0},
            .scale = {scale, scale, scale}, .create_function = nullptr,
        };
        if (create_standalone_actor(profiles[type], params, id) != MOD_OK) return false;
        *slot = {id, profiles[type], false, false, false, elite, 0};
        return true;
    }
    return false;
}
void update_spawns(daAlink_c* link) {
    unsigned population = 0;
    for (auto& e : s_enemies) {
        if (e.id == kNone) continue;
        ++population;
        auto* actor = actor_by_id(e.id);
        if (!actor || !e.initialized) continue;
        ++e.age;
        // Cleanup is never a reward. A lost elite can be attempted again.
        if (!e.retiring && (actor->current.pos.absXZ(link->current.pos) > 2500 || e.age > 5400)) {
            e.retiring = true;
            if (e.elite && !e.credited) s_run.elite_spawned = false;
        }
    }
    if (s_spawnCooldown) { --s_spawnCooldown; return; }
    s_spawnCooldown = s_run.spawn_interval();
    if (s_run.ticks >= duration) {
        if (!s_run.elite_spawned && population < enemy_capacity) {
            s_run.elite_spawned = spawn_enemy(link, true);
            if (!s_run.elite_spawned) ++s_spawnFailures;
        }
    } else if (population < s_run.target_population()) {
        if (!spawn_enemy(link, false)) ++s_spawnFailures;
    }
}
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
void launch(daAlink_c* link, unsigned weapon, cXyz direction) {
    auto slot = std::find_if(s_attacks.begin(), s_attacks.begin() + 8, [](const auto& a) { return !a.active; });
    if (slot == s_attacks.begin() + 8) return;
    auto& a = *slot;
    a.weapon = weapon; a.life = weapon == 0 ? 35 : 3;
    a.position = link->current.pos + cXyz(0, 65, 0);
    a.velocity = weapon == 0 ? direction * 42.0f : cXyz(0, 0, 0);
    a.radius = s_run.radius(weapon == 0 ? 65.0f : 130.0f);
    a.status.Init(120, 0xff, link);
    a.collider.Set(kAttackSource); a.collider.SetStts(&a.status);
    a.collider.SetAtType(AT_TYPE_NORMAL_SWORD);
    a.collider.SetAtAtp(static_cast<u8>(s_run.attack_power(weapon)));
    // LIGHT material hardcodes 100 damage in cc_at_check; aura color is visual.
    a.collider.SetAtMtrl(dCcD_MTRL_NONE);
    a.collider.OnAtNoConHit(); a.collider.ResetAtHit(); a.collider.OnAtSetBit();
    a.active = true;
}
void update_wave_effects(Attack& a, daAlink_c* link) {
    // Same moving-emitter approach as Great Spin Projectile, using only the
    // two small normal-spin effects, never the six large-spin emitters.
    constexpr std::array<u16, 2> effects{ID_ZI_J_KAITENGIRI_A, ID_ZI_J_KAITENGIRI_B};
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
                                  &rotation, nullptr, 0xff, nullptr, -1, nullptr, nullptr, nullptr);
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
    const auto direction = target_direction(link);
    launch(link, 0, direction);
    if (s_run.ranks[0] >= 3) {
        launch(link, 0, cXyz(direction.x * 0.94f + direction.z * 0.34f,
            direction.y, direction.z * 0.94f - direction.x * 0.34f));
        if (s_run.ranks[0] >= 5) launch(link, 0, cXyz(direction.x * 0.94f - direction.z * 0.34f,
            direction.y, direction.z * 0.94f + direction.x * 0.34f));
    }
}
bool normal_sword_proc(daAlink_c* link) {
    return link->mProcID == daAlink_c::PROC_CUT_NORMAL || link->mProcID == daAlink_c::PROC_CUT_FINISH;
}
bool begin_sword_combo(daAlink_c* link) {
    // Native recovery can clear its counter before our cooldown expires. Keep
    // the automatic sequence across that gap, but let native code pick each
    // directional/lock-on/hit-dependent swing and the fourth-hit finisher.
    link->mComboCutCount = s_comboStep;
    struct Scope { Scope(){s_autoSwordCall=true;} ~Scope(){s_autoSwordCall=false;} } scope;
    if (!link->checkCutAction() || !normal_sword_proc(link)) return false;
    s_comboStep = link->mComboCutCount % 4;
    s_comboTimeout = 2 * ticks_per_second;
    return true;
}
void update_sword(daAlink_c* link) {
    if (s_meleeCooldown) --s_meleeCooldown;
    if (s_comboTimeout && !--s_comboTimeout) s_comboStep = 0;
    bool continuingSwing = false;
    if (s_swordAction == SwordAction::Normal) {
        if (normal_sword_proc(link)) {
            if (s_wavePending && link->mUnderFrameCtrl[0].getFrame() >= link->field_0x3478) {
                release_sword_wave(link); s_wavePending = false;
            }
            // Never cut off the damage window, and let the final hit recover.
            if (!s_comboStep || s_meleeCooldown || link->mUnderFrameCtrl[0].getFrame() <=
                std::max(link->field_0x347c, link->field_0x3484)) return;
            continuingSwing = true;
        } else {
            s_wavePending = false; // Interrupted attacks cannot fire late from a damage/dodge action.
            s_swordAction = SwordAction::None;
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
    const bool spin = !continuingSwing && s_run.ranks[1] && !s_cooldowns[1] && enemy_in_range(link, s_run.radius(450));
    const bool wave = s_run.ranks[0] && !s_cooldowns[0] && enemy_in_range(link, 1600);
    if (!spin && !wave && (s_meleeCooldown || !enemy_in_range(link, s_run.radius(280)))) return;
    if (!link->checkItemSwordEquip()) {
        if (link->mEquipItem != dItemNo_NONE_e) return;
        if (link->field_0x2fde != 0x103) link->swordEquip(0);
        return; // Let native equip animation finish before replacing the body action.
    }
    const auto direction = target_direction(link);
    link->shape_angle.y = cM_atan2s(direction.x, direction.z);
    link->current.angle.y = link->shape_angle.y;
    if (spin) {
        if (link->procCutTurnInit(1, 1)) {
            s_swordAction = SwordAction::Spin;
            s_comboStep = s_comboTimeout = 0;
            link->field_0x3478 = s_run.radius(link->field_0x3478);
            s_cooldowns[1] = s_run.cooldown(100);
        }
    } else if (begin_sword_combo(link)) {
        s_swordAction = SwordAction::Normal; s_wavePending = wave;
        s_meleeCooldown = s_run.cooldown(30);
        if (wave) s_cooldowns[0] = s_run.cooldown(40);
    }
}
void after_sword_move(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !playable() || s_run.phase != Phase::Running || fopAcM_GetID(link) != s_player) return;
    const bool spin = s_swordAction == SwordAction::Spin && link->mProcID == daAlink_c::PROC_CUT_TURN;
    const bool normal = s_swordAction == SwordAction::Normal && normal_sword_proc(link);
    if ((!spin && !normal) || !link->mLinkAcch.ChkGroundHit()) return;
    // Runs before native posMove/ground/wall resolution, retaining collision.
    // Normal swings and finishers decelerate/lunge too, not only spin attacks.
    link->current.angle.y = link->mMoveAngle;
    link->mNormalSpeed = link->mpHIO->mMove.m.mMaxSpeed * link->mStickValue;
}
HookAction before_large_spin(ModContext*, void*, void* result, void*) {
    if (!in_arena()) return HOOK_CONTINUE;
    *static_cast<BOOL*>(result) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}
#include "survivors_loot.inc"
#include "survivors_ranged.inc"
#include "survivors_treasure.inc"

void update_weapons(daAlink_c* link) {
    for (auto& cooldown : s_cooldowns) if (cooldown) --cooldown;
    for (auto& a : s_attacks) {
        if (!a.active) continue;
        a.status.Move();
        if (a.weapon >= unsigned(Upgrade::Bow)) { update_item_attack(a, link); continue; }
        if (!a.life-- || a.collider.ChkAtShieldHit()) { stop_attack(a); continue; }
        const cXyz next = a.position + a.velocity;
        dBgS_LinChk wall; wall.Set(&a.position, &next, link);
        if (a.weapon == 0 && dComIfG_Bgsp().LineCross(&wall)) { stop_attack(a); continue; }
        a.position = next;
        if (a.weapon == 0) update_wave_effects(a, link);
        a.collider.SetC(a.position); a.collider.SetR(a.radius);
        a.collider.SetAtVec(a.velocity);
        dComIfG_Ccsp()->Set(&a.collider);
    }
    update_ranged(link);
    fire_items(link);
    if (!s_ranged.active) update_sword(link);
    if (s_run.ranks[2] && !s_cooldowns[2]) {
        launch(link, 2, cXyz(0, 0, 0));
        s_cooldowns[2] = s_run.cooldown(65);
    }
}
bool native_sword_collider(const cCcD_Obj* collider) {
    auto* link = daAlink_getAlinkActorClass();
    if (!in_arena() || !link || fopAcM_GetID(link) != s_player) return false;
    if (s_swordAction == SwordAction::Spin && collider == &link->mAtSph) return true;
    if (s_swordAction == SwordAction::Normal)
        for (const auto& capsule : link->mAtCps) if (collider == &capsule) return true;
    return false;
}
void after_power(ModContext*, void* args, void*, void*) {
    if (!s_active) return;
    auto* info = mods::arg<dCcU_AtInfo*>(args, 0);
    if (!info || !info->mpCollider) return;
    if (native_sword_collider(info->mpCollider)) {
        info->mAttackPower = s_swordAction == SwordAction::Spin ? s_run.attack_power(1) :
            3 + s_run.rank(Upgrade::Power) + s_run.sword_bonus();
        return;
    }
    for (const auto& attack : s_attacks) {
        if (info->mpCollider == &attack.collider) {
            // The native table maps 3 to 30 and >=4 to up to 200 damage.
            // Keep native collision, defense, damage reactions and death handling,
            // but use a linear power value for our own weapons only.
            info->mAttackPower = info->mpCollider->GetAtAtp();
            return;
        }
    }
}
void after_player(ModContext*, void* args, void*, void*) {
    if (!s_active || s_run.phase != Phase::Running || !playable()) return;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || fopAcM_GetID(link) != s_player) return;
    if (dComIfGs_getLife() == 0) { finish(false); return; }
    s_run.tick(true);
    update_spawns(link); update_weapons(link); update_orbs(link);
    if (s_run.level_up()) clear_attacks();
    else update_chests(link);
}
HookAction before_death(ModContext*, void*, void* result, void*) {
    if (!in_arena() || (s_run.phase != Phase::Running && s_run.phase != Phase::Upgrade)) return HOOK_CONTINUE;
    finish(false);
    *static_cast<int*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
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
bool pause_run_actor(bool enemy) {
    if (!in_arena() || s_run.phase == Phase::Loading) return false;
    if (ui_visible() || dComIfGp_isPauseFlag()) return true;
    // Native Get Item needs Link's Execute; enemies and the run clock stay paused.
    return s_run.phase != Phase::Running && (enemy || s_run.phase != Phase::Treasure);
}
HookAction before_process(ModContext*, void* args, void* result, void*) {
    auto* process = mods::arg<void*>(args, 1);
    if (!process || !fopAcM_IsActor(process)) return HOOK_CONTINUE;
    auto* actor = static_cast<fopAc_ac_c*>(process);
    const auto id = fopAcM_GetID(actor);
    auto* enemy = tracked(id);
    retain_treasure_actor(actor, args);
    if (!enemy && (!in_arena() || id != s_player)) return HOOK_CONTINUE;
    const auto* methods = reinterpret_cast<const process_method_class*>(actor->sub_method);
    if (!methods) return HOOK_CONTINUE;
    const auto method = mods::arg<process_method_func>(args, 0);
    if (method == methods->delete_method) {
        // Never award XP here: cancellation, unloading and despawning also delete.
        if (enemy) {
            if (enemy->elite && !enemy->credited) s_run.elite_spawned = false;
            *enemy = {};
        }
        return HOOK_CONTINUE;
    }
    if (method != methods->execute_method) return HOOK_CONTINUE;
    if (enemy) s_processFrames.push_back({args, id});
    if (enemy && !enemy->initialized) {
        enemy->initialized = true;
        if (enemy->profile == fpcNm_E_OC_e) {
            actor->field_0x560 = actor->health = enemy->elite ? 120 :
                static_cast<s16>(8 + std::min(24u, s_run.ticks / 900));
        }
    }
    if (enemy && !enemy->retiring && !enemy->credited && s_run.phase == Phase::Running && playable()) {
        // Keep mobile enemies engaged without altering their native damage/death states.
        if (enemy->profile == fpcNm_E_MS_e) {
            auto* rat = static_cast<e_ms_class*>(actor);
            if (rat->mAction == 0 && rat->health > 0) { rat->mAction = 1; rat->mMode = 0; }
        } else if (enemy->profile == fpcNm_E_OC_e) {
            auto* bokoblin = static_cast<daE_OC_c*>(actor);
            if (bokoblin->getActionMode() <= 2) bokoblin->setActionMode(3, 0);
        }
    }
    if ((enemy && enemy->retiring) || pause_run_actor(enemy != nullptr)) {
        *static_cast<int*>(result) = 1;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
void after_process(ModContext*, void* args, void*, void*) {
    // Only matched Execute callbacks may inspect an actor. Delete callbacks can
    // return after freeing it, and nested methods must not consume another frame.
    if (s_processFrames.empty() || s_processFrames.back().args != args) return;
    const auto frame = s_processFrames.back(); s_processFrames.pop_back();
    if (!in_arena() || s_run.phase != Phase::Running || transitioning()) return;
    auto* enemy = tracked(frame.id);
    auto* actor = actor_by_id(frame.id);
    if (enemy && actor) credit_death(*enemy, actor);
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
    if (in_arena() && !s_restart && s_run.phase != Phase::Loading) return HOOK_SKIP_ORIGINAL;
    dComIfGs_setRestartRoomParam(0);
    mods::arg_ref<const char*>(args, 0) = kStage;
    mods::arg_ref<s16>(args, 1) = 0;
    mods::arg_ref<s8>(args, 2) = 51;
    mods::arg_ref<s8>(args, 3) = 0;
    return HOOK_CONTINUE;
}
void after_draw(ModContext*, void*, void*, void*) {
    if (!in_arena() || transitioning()) return;
    for (auto& orb : s_orbs) {
        if (!orb.value) continue;
        cXyz size(10, 16, 10); csXyz angle(0, static_cast<s16>(s_run.ticks * 300), 0);
        dDbVw_drawCubeOpa(orb.position, size, angle, GXColor{90, 235, 255, 255});
    }
    draw_loot();
    draw_ranged();
    for (auto& a : s_attacks) {
        if (!a.active || a.weapon != 2) continue;
        const GXColor color{255, 235, 160, 180};
        // Only Light Aura uses a ring; Sword Wave uses native spin particles.
        for (unsigned i = 0; i < 24; ++i) {
            const s16 p = static_cast<s16>(i * 65536 / 24);
            const s16 q = static_cast<s16>((i + 1) * 65536 / 24);
            cXyz from = a.position + cXyz(cM_ssin(p) * a.radius, -35, cM_scos(p) * a.radius);
            cXyz to = a.position + cXyz(cM_ssin(q) * a.radius, -35, cM_scos(q) * a.radius);
            dDbVw_drawLineXlu(from, to, color, 1, 8);
        }
    }
}
void label(const char* text, float x, float y, float size) {
    auto* font = mDoExt_getMesgFont(); if (!font) return;
    J2DPrint shadow(font, JUtility::TColor(0,0,0,230), JUtility::TColor(0,0,0,230));
    shadow.setFontSize(size, size);
    shadow.printReturn(text, 550, size + 8, HBIND_CENTER, VBIND_TOP, x - 274, y + 1, 255);
    J2DPrint print(font, JUtility::TColor(180,245,255,255), JUtility::TColor(255,240,195,255));
    print.setFontSize(size, size);
    print.printReturn(text, 550, size + 8, HBIND_CENTER, VBIND_TOP, x - 275, y, 255);
}
void draw_hud(ModContext*, void*, void*, void*) {
    if (!in_arena() || transitioning() || ui_visible() || s_run.phase == Phase::Loading) return;
    auto* graf = dComIfGp_getCurrentGrafPort(); if (!graf) return;
    graf->setup2D();
    const float x = mDoGph_gInf_c::getMinXF() + mDoGph_gInf_c::getWidthF() * 0.5f;
    char text[180];
    std::snprintf(text, sizeof(text), "%02u:%02u / 10:00   Level %u   Kills %u",
        s_run.ticks / 1800, s_run.ticks / 30 % 60, s_run.level, s_run.kills);
    label(text, x, 40, 17);
    std::snprintf(text, sizeof(text), "XP %u / %u   Wave %u   Spin %u   Aura %u",
        s_run.xp, s_run.required_xp(), s_run.ranks[0], s_run.ranks[1], s_run.ranks[2]);
    label(text, x, 62, 14);
    // J2DPrint enables direct TEX0 attributes. fillBox emits only position and
    // color, so reset the vertex layout before its quads or the GX FIFO reads
    // subsequent command bytes as missing texture coordinates.
    graf->setup2D();
    const float fraction = std::min(1.0f, float(s_run.xp) / float(s_run.required_xp()));
    const auto oldTL = graf->mColorTL, oldTR = graf->mColorTR;
    const auto oldBR = graf->mColorBR, oldBL = graf->mColorBL;
    graf->setColor(JUtility::TColor(12, 25, 32, 210));
    graf->fillBox(JGeometry::TBox2<f32>(x - 160, 82, x + 160, 88));
    graf->setColor(JUtility::TColor(80, 220, 245, 240));
    graf->fillBox(JGeometry::TBox2<f32>(x - 160, 82, x - 160 + 320 * fraction, 88));
    graf->setColor(oldTL, oldTR, oldBR, oldBL);
    std::snprintf(text, sizeof(text), "Bow %u | Boomerang %u | Bombs %u | Ball %u",
        s_run.rank(Upgrade::Bow), s_run.rank(Upgrade::Boomerang), s_run.rank(Upgrade::Bombs),
        s_run.rank(Upgrade::Ball));
    label(text, x, 96, 12);
    if (s_run.chest_rewards_left(potion_slot() >= 0))
        std::snprintf(text, sizeof(text), "Next chest: %u rupees", s_run.chest_cost());
    else std::snprintf(text, sizeof(text), "No chest rewards available");
    label(text, x, 114, 14);
    if (s_run.ticks >= duration && s_run.phase == Phase::Running)
        label("Defeat the Elite Bokoblin!", x, 136, 16);
    else if (s_spawnFailures >= 10)
        label("Waiting for a clear enemy spawn position", x, 136, 13);
    graf->setup2D();
}
void select_upgrade(ModContext*, void* data) {
    const unsigned choice = *static_cast<unsigned*>(data);
    if (s_run.phase != Phase::Upgrade || choice >= 3) return;
    const auto selected = s_run.offers[choice];
    if (!s_run.choose(choice)) return;
    if (selected == Upgrade::Heart) dComIfGs_setMaxLife(15 + 5 * s_run.rank(Upgrade::Heart));
    if (selected == Upgrade::Rupees)
        dComIfGs_setRupee(std::min<unsigned>(dComIfGs_getRupeeMax(), dComIfGs_getRupee() + 20));
    close_window();
    s_run.level_up(); // Preserve queued levels; do not resume combat between choices.
}
void start_run(ModContext*, void*) {
    if (s_run.phase != Phase::Ready) return;
    close_window(); clear_attacks(); clear_loot(); prepare_save();
    dMeter2Info_setSword(dItemNo_SWORD_e, false); // Refresh live equipment after a previous run.
    const auto seed = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    s_run.start(seed); s_chestDelay += s_run.next_random() % (10 * ticks_per_second + 1);
    s_resultsStored = false; s_cooldowns = {}; s_spawnCooldown = 30;
    s_swordAction = SwordAction::None; s_meleeCooldown = 0; s_wavePending = false;
    s_comboStep = s_comboTimeout = 0;
}
void replay(ModContext*, void*) { close_window(); s_restart = true; }
void save_results(ModContext*, void*) { close_window(); s_saveRequested = true; }
ModResult button(UiElementHandle pane, const char* text, UiPressedFn callback, void* data = nullptr,
                 const char* help = nullptr) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.label = text; desc.on_pressed = callback; desc.user_data = data; desc.help_rml = help;
    return svc_ui->pane_add_control(mod_ctx, pane, &desc, nullptr);
}
ModResult build_window(ModContext*, UiWindowHandle, UiElementHandle left, UiElementHandle right,
                       void*, ModError*) {
    if (s_run.phase == Phase::Upgrade) {
        svc_ui->pane_add_text(mod_ctx, right, "Combat is paused. Choose one upgrade. Closing this window keeps the same choices.", nullptr);
        for (unsigned i = 0; i < 3; ++i) {
            const auto u = s_run.offers[i]; const unsigned index = static_cast<unsigned>(u);
            char text[80];
            if (u == Upgrade::Rupees) std::snprintf(text, sizeof(text), "%s (+20)", names[index]);
            else std::snprintf(text, sizeof(text), "%s - Level %u", names[index], s_run.rank(u) + 1);
            const auto result = button(left, text, select_upgrade, &s_choices[i], descriptions[index]);
            if (result != MOD_OK) return result;
        }
        return MOD_OK;
    }
    char text[240];
    if (s_run.phase == Phase::Ready) {
        svc_ui->pane_add_text(mod_ctx, left, "Survive for 10 minutes, then defeat the Elite Bokoblin.", nullptr);
        svc_ui->pane_add_text(mod_ctx, right,
            "Move and dodge while your weapons attack automatically. Collect the blue crystals to level up. "
            "Normal sword strikes are automatic. Unlock Sword Wave, Whirling Blade and Light Aura during the run. Enemies may drop hearts and rupees. "
            "Face a treasure chest and press Open to receive an automatic item weapon. You need enough rupees to interact. Prices start at 20 rupees and rise by 20 after every purchase. "
            "Chest weapons can reach Level 5 through duplicate finds and level-up choices. Chests can also contain the Master Sword or a bottle of Red Potion.", nullptr);
        std::snprintf(text, sizeof(text), "Completed runs: %u | Wins: %u | Best kills: %u | Best level: %u",
            s_records.runs, s_records.wins, s_records.best_kills, s_records.best_level);
        svc_ui->pane_add_text(mod_ctx, left, text, nullptr);
        return button(left, "Start run", start_run);
    }
    std::snprintf(text, sizeof(text), "%s - %02u:%02u | %u kills | Level %u",
        s_run.phase == Phase::Victory ? "Victory" : "Defeat", s_run.ticks / 1800,
        s_run.ticks / 30 % 60, s_run.kills, s_run.level);
    svc_ui->pane_add_text(mod_ctx, left, text, nullptr);
    std::snprintf(text, sizeof(text), "Sword Wave %u | Whirling Blade %u | Light Aura %u",
        s_run.ranks[0], s_run.ranks[1], s_run.ranks[2]);
    svc_ui->pane_add_text(mod_ctx, left, text, nullptr);
    std::snprintf(text, sizeof(text), "Bow %u | Boomerang %u | Bombs %u | Ball and Chain %u | Chests %u",
        s_run.rank(Upgrade::Bow), s_run.rank(Upgrade::Boomerang), s_run.rank(Upgrade::Bombs),
        s_run.rank(Upgrade::Ball), s_run.chests_opened);
    svc_ui->pane_add_text(mod_ctx, left, text, nullptr);
    svc_ui->pane_add_text(mod_ctx, right, s_storeFailed ?
        "Records could not be stored. Retry saving before leaving this mode." :
        "Save records before quitting. The native save screen writes your Survivors records to this mode's separate save file. "
        "Runs are not resumed mid-combat; loading returns to the start screen.", nullptr);
    auto result = button(left, "Save records", save_results);
    if (result == MOD_OK) result = button(left, "New run", replay);
    return result;
}
void open_window() {
    if (s_window || ui_visible()) return;
    UiTabDesc tab = UI_TAB_DESC_INIT;
    tab.title = s_run.phase == Phase::Upgrade ? "Choose an upgrade" : "Hyrule Survivors";
    tab.build = build_window;
    UiWindowDesc desc = UI_WINDOW_DESC_INIT;
    desc.tabs = &tab; desc.tab_count = 1;
    desc.on_closed = [](ModContext*, UiWindowHandle handle, void*) {
        if (s_window == handle) s_window = 0;
    };
    if (svc_ui->window_push(mod_ctx, &desc, &s_window) != MOD_OK)
        svc_log->warn(mod_ctx, "Hyrule Survivors: could not open run window");
}
ModResult activated(void*, ModError*) { s_active = true; reset_runtime(); return MOD_OK; }
ModResult deactivated(void*, ModError*) { s_active = false; reset_runtime(); return MOD_OK; }
ModResult new_save(void*, ModError*) {
    reset_runtime(); s_records = {}; prepare_save();
    s_saveReady = true;
    const auto bytes = s_records.encode();
    svc_save->set_blob(mod_ctx, kBlob, bytes.data(), bytes.size());
    return MOD_OK;
}
ModResult loaded(void*, ModError*) {
    reset_runtime(); prepare_save(); s_saveReady = true; return MOD_OK;
}
ModResult reset(void*, ModError*) { reset_runtime(); return MOD_OK; }
} // namespace

bool survivors_active() { return s_active; }
bool survivors_arena_active() { return in_arena(); }
void update_survivors() {
    cleanup_enemies();
    if (s_treasure.retiring) retire_treasure_actor();
    if (!s_active || !s_gameStarted) return;
    if (s_savePrompt != kNone) {
        if (d_GameOver_CheckDelete(s_savePrompt) && dComIfGp_getGameoverStatus() == 1) {
            d_GameOver_Delete(s_savePrompt); dComIfGp_setGameoverStatus(0);
            s_savePrompt = kNone;
        }
        return;
    }
    if (!in_arena() || transitioning()) { clear_attacks(); clear_loot(); s_player = kNone; s_warmup = 0; return; }
    auto* link = daAlink_getAlinkActorClass();
    if (!link || !dComIfGp_getStageStagInfo()) return;
    if (s_restart) {
        // Always use a scene reload: destroys old actors, pending spawns, collision,
        // damage animations and native drop actors before another run starts.
        retire_enemies(); clear_attacks(); clear_loot();
        dComIfGp_setNextStage(kStage, 0, 51, 0);
        s_run = {}; s_orbs = {}; s_restart = false; s_player = kNone;
        s_resultsStored = false; s_spawnFailures = 0;
        return;
    }
    if (s_player != fopAcM_GetID(link)) {
        s_player = fopAcM_GetID(link); s_run.phase = Phase::Loading; s_warmup = 0;
    }
    if (s_run.phase == Phase::Loading) {
        if (!playable() || ++s_warmup < 15) return;
        load_records(); s_run.phase = Phase::Ready;
    }
    if (s_run.phase == Phase::Treasure) update_treasure_demo(link);
    if (s_saveRequested) {
        if (ui_visible() || dComIfGp_event_runCheck() || dMeter2Info_getGameOverType()) return;
        const auto bytes = s_records.encode();
        s_storeFailed = svc_save->set_blob(mod_ctx, kBlob, bytes.data(), bytes.size()) != MOD_OK;
        if (s_storeFailed) { s_saveRequested = false; return; }
        dComIfGs_getSaveData()->getPlayer().getPlayerReturnPlace().set(kStage, 51, 0);
        s_savePrompt = d_GameOver_Create(1);
        s_saveRequested = s_savePrompt == kNone;
        return;
    }
    if (s_run.phase == Phase::Ready || s_run.phase == Phase::Upgrade ||
        s_run.phase == Phase::Victory || s_run.phase == Phase::Defeat) open_window();
}
ModResult initialize_survivors(ModError* error) {
    ModResult result = MOD_OK;
#define PRE(H, F) if (result == MOD_OK) result = mods::hook_add_pre<H>(svc_hook, F)
#define POST(H, F) if (result == MOD_OK) result = mods::hook_add_post<H>(svc_hook, F)
    PRE(SurvivorsProcess, before_process); POST(SurvivorsProcess, after_process);
    POST(SurvivorsPlayer, after_player); POST(SurvivorsDraw, after_draw);
    PRE(SurvivorsPlayer, before_chest_action);
    PRE(SurvivorsRangedAim, before_ranged_aim);
    PRE(SurvivorsBowAction, before_ranged_native);
    PRE(SurvivorsBowNext, before_ranged_native);
    PRE(SurvivorsRangedCancel, before_ranged_native);
    PRE(SurvivorsTreasureDraw, before_treasure_draw);
    PRE(SurvivorsGetItem, before_treasure_get); POST(SurvivorsGetItem, after_treasure_get);
    PRE(SurvivorsDeath, before_death); PRE(SurvivorsManualAttack, before_cut);
    PRE(SurvivorsSwordAttack, before_sword_attack); PRE(SurvivorsDoor, before_cut);
    POST(SurvivorsPower, after_power);
    POST(SurvivorsSpinMove, after_sword_move); PRE(SurvivorsLargeSpin, before_large_spin);
    POST(SurvivorsNormalMove, after_sword_move); POST(SurvivorsFinishMove, after_sword_move);
    PRE(SurvivorsStage, before_stage); POST(SurvivorsHud, draw_hud);
    PRE(SurvivorsGameStart, before_game_start); PRE(SurvivorsOpening, before_opening);
    PRE(SurvivorsDrops, before_drop); PRE(SurvivorsDirectDrops, before_drop);
#undef PRE
#undef POST
    if (result != MOD_OK) return mods::set_error(error, result, "failed to install Hyrule Survivors hooks");
    const GameModeDesc mode{
        .struct_size = sizeof(GameModeDesc), .game_mode_id = kMode,
        .full_name = "Hyrule Survivors", .save_name = "gczelda2-survivors", .user_data = nullptr,
        .on_activated = activated, .on_deactivated = deactivated, .on_play = activated,
        .on_save_loaded = loaded, .on_new_save = new_save, .on_new_save_select = nullptr,
        .on_game_reset = reset, .on_tick = nullptr,
    };
    result = svc_game_mode->register_game_mode(mod_ctx, &mode);
    if (result != MOD_OK) return mods::set_error(error, result, "failed to register Hyrule Survivors");
    s_registered = true;
    return MOD_OK;
}
void shutdown_survivors() {
    s_active = false; reset_runtime(); cleanup_enemies();
    if (s_savePrompt != kNone) { d_GameOver_Delete(s_savePrompt); s_savePrompt = kNone; }
    if (s_registered) svc_game_mode->unregister_game_mode(mod_ctx, kMode);
    s_registered = false;
    // Loader removes callbacks after shutdown. No native actor profile points at
    // this module; outstanding deletes belong to the scene and remain safe.
}
} // namespace dawnlight
