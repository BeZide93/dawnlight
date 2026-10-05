#include "enemy_hard_mode_runtime.hpp"
#include "enemy_hard_mode.hpp"
#include "f_pc/f_pc_method.h"
#include "f_pc/f_pc_name.h"
#include "m_Do/m_Do_ext.h"
#include "JSystem/JParticle/JPAEmitter.h"
#include "d/actor/d_a_e_ai.h"
#include <array>
#include <cmath>
#include "SSystem/SComponent/c_lib.h"
#include "d/actor/d_a_b_gg.h"
#include "d/actor/d_a_b_tn.h"
#include "d/actor/d_a_e_ba.h"
#include "d/actor/d_a_e_bs.h"
#include "d/actor/d_a_e_bu.h"
#include "d/actor/d_a_e_cr.h"
#include "d/actor/d_a_e_db.h"
#include "d/actor/d_a_e_db_leaf.h"
#include "d/actor/d_a_e_dd.h"
#include "d/actor/d_a_e_dn.h"
#include "d/actor/d_a_e_fb.h"
#include "d/actor/d_a_e_fs.h"
#include "d/actor/d_a_e_fz.h"
#include "d/actor/d_a_e_gb.h"
#include "d/actor/d_a_e_gi.h"
#include "d/actor/d_a_e_hb.h"
#include "d/actor/d_a_e_hb_leaf.h"
#include "d/actor/d_a_e_kk.h"
#include "d/actor/d_a_e_mf.h"
#include "d/actor/d_a_e_ms.h"
#include "d/actor/d_a_e_oc.h"
#include "d/actor/d_a_e_rd.h"
#include "d/actor/d_a_e_sf.h"
#include "d/actor/d_a_e_sh.h"
#include "d/actor/d_a_e_st.h"
#include "d/actor/d_a_e_tk.h"
#include "d/actor/d_a_e_tk2.h"
#include "d/actor/d_a_e_tk_ball.h"
#include "d/actor/d_a_e_tt.h"
#include "d/actor/d_a_e_ww.h"
#include "d/actor/d_a_e_zs.h"
#include "d/d_bg_s_acch.h"
#include "d/d_cc_d.h"
#include "d/d_com_inf_game.h"
#include "d/d_item.h"
#include "d/d_s_play.h"
#include <algorithm>
namespace dawnlight {
namespace {
namespace darknut {
DEFINE_HOOK(&daB_TN_c::execute, ExecuteHook);
bool eligible(fopAc_ac_c* actor) {
    auto* darknut = static_cast<daB_TN_c*>(actor);
    // Scripted camera sequences contain exact integer timer events and scene
    // placement. Keep their vanilla scheduling instead of running them faster.
    if (darknut->mActionMode1 == daB_TN_c::ACT_ROOMDEMO ||
        darknut->mActionMode1 == daB_TN_c::ACT_OPENING ||
        darknut->mActionMode1 == daB_TN_c::ACT_CHANGEDEMO ||
        darknut->mActionMode1 == daB_TN_c::ACT_ENDING) {
        return false;
    }
    return darknut->mpModelMorf1 != nullptr && darknut->mpModelMorf2 != nullptr;
}
void snapshot(EnemyHardModeStep& step) {
    auto* actor = static_cast<daB_TN_c*>(step.actor);
    step.action = actor->mActionMode1;
    step.subaction = actor->mActionMode2;
}
ModResult install() { return install_enemy_hard_mode_execute_hook<ExecuteHook>(); }

}
namespace bokoblin {
DEFINE_HOOK(&daE_OC_c::execute, ExecuteHook);
constexpr int kFall = 14;
constexpr int kDeath = 9;
bool eligible(fopAc_ac_c* base) {
    auto* actor = static_cast<daE_OC_c*>(base);
    return actor->mpMorf != nullptr && actor->field_0x6c8 == 0 &&
        (actor->mActionMode < kDeath || actor->mActionMode >= kFall);
}
void snapshot(EnemyHardModeStep& step) {
    auto* actor = static_cast<daE_OC_c*>(step.actor);
    step.action = actor->mActionMode;
    step.subaction = actor->mOcState;
}
ModResult install() { return install_enemy_hard_mode_execute_hook<ExecuteHook>(); }

}
namespace aeralfos {
DEFINE_HOOK(&daB_GG_c::Execute, ExecuteHook);
bool eligible(fopAc_ac_c* actor) {
    auto& enemy = *static_cast<daB_GG_c*>(actor);
    // Demo/camera/death paths also manipulate Link, detached parts and scene state.
    return enemy.mpModelMorf != nullptr && enemy.mAction <= 2 && enemy.mSubAction != 4 &&
           enemy.mCamMode == 0 && enemy.field_0x5b1 != 1;
}
void snapshot(EnemyHardModeStep& step) {
    auto& actor = *static_cast<daB_GG_c*>(step.actor);
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
ModResult install() { return install_enemy_hard_mode_execute_hook<ExecuteHook>(); }

}
namespace chilfos {
DEFINE_HOOK(&daE_KK_c::execute, ExecuteHook);
bool eligible(fopAc_ac_c* actor) {
    auto& enemy = *static_cast<daE_KK_c*>(actor);
    return enemy.mpWeaponMorfSO != nullptr && (enemy.field_0x679 == 1 || enemy.mpMorfSO != nullptr) &&
        (enemy.field_0x679 != 2 || checkItemGet(dItemNo_IRONBALL_e, 1));
}
void snapshot(EnemyHardModeStep& step) {
    auto& actor = *static_cast<daE_KK_c*>(step.actor);
    step.action = actor.mActionMode;
    step.subaction = actor.mMoveMode;
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
ModResult install() { return install_enemy_hard_mode_execute_hook<ExecuteHook>(); }

}
namespace white_wolfos {
DEFINE_HOOK(&daE_WW_c::execute, ExecuteHook);
bool eligible(fopAc_ac_c* base) {
    const auto& actor = *static_cast<daE_WW_c*>(base);
    // The invisible pack coordinator does not animate or move like a wolf.
    return actor.mpModelMorf != nullptr && actor.mAction != 0;
}
void snapshot(EnemyHardModeStep& step) {
    auto& actor = *static_cast<daE_WW_c*>(step.actor);
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
ModResult install() { return install_enemy_hard_mode_execute_hook<ExecuteHook>(); }

}
namespace bulblin {
bool eligible(fopAc_ac_c* base) {
    const auto& a = *reinterpret_cast<e_rd_class*>(base);
    // Mounted King Bulblin and his scripted camera/boar paths are separate actors.
    return a.anm_p != nullptr && a.ride_mode == 0 && a.actor_set == 0 &&
        a.demo_mode == 0 && a.arg2 != 11 && a.field_0xaf0 == 0 &&
        a.action != 29 && !(a.action >= 40 && a.action <= 47) &&
        !dComIfGp_event_runCheck();
}
void snapshot(EnemyHardModeStep& step) {
    auto& a = *reinterpret_cast<e_rd_class*>(step.actor);
    step.action = a.action;
    step.subaction = a.mode;
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
}
namespace lizalfos {
bool eligible(fopAc_ac_c* base) {
    const auto& a = *reinterpret_cast<e_dn_class*>(base);
    return a.anm_p != nullptr && a.status == 0 && a.unk_timer_5 == 0 &&
        a.action != 24 && a.action != 60 && !dComIfGp_event_runCheck();
}
void snapshot(EnemyHardModeStep& step) {
    auto& a = *reinterpret_cast<e_dn_class*>(step.actor);
    step.action = a.action;
    step.subaction = a.mode;
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
}
namespace dynalfos {
bool eligible(fopAc_ac_c* base) {
    const auto& a = *reinterpret_cast<e_mf_class*>(base);
    return a.mpModelMorf != nullptr && a.field_0x728 == 0 && a.field_0x820 == 0 &&
        a.mAction != 24 && !dComIfGp_event_runCheck();
}
void snapshot(EnemyHardModeStep& step) {
    auto& a = *reinterpret_cast<e_mf_class*>(step.actor);
    step.action = a.mAction;
    step.subaction = a.field_0x5b4;
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
}
namespace stalfos {
constexpr s16 kSitWait = 33;
e_sf_class& stalfos(fopAc_ac_c* actor) {
    static_assert(offsetof(e_sf_class, actor) == 0);
    return *reinterpret_cast<e_sf_class*>(actor);
}
bool eligible(fopAc_ac_c* actor) {
    const auto& a = stalfos(actor);
    return a.mpModelMorf != nullptr && a.mDemoMode == 0 &&
        a.mAction != kSitWait && !dComIfGp_event_runCheck();
}
void snapshot(EnemyHardModeStep& step) {
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
}
namespace mini_freezard {
DEFINE_HOOK(&daE_FZ_c::execute, ExecuteHook);
bool eligible(fopAc_ac_c* base) {
    auto* actor = static_cast<daE_FZ_c*>(base);
    // Blizzeta controls orbiting children directly; the iron-ball-gated type
    // can return before decrementing its timers. Neither uses this profile.
    return actor->mpModel != nullptr && actor->field_0x714 != 2 &&
        actor->field_0x714 != 3 && actor->mpBlizzetaActor == nullptr &&
        actor->mActionMode != ACT_ROLLMOVE;
}
void snapshot(EnemyHardModeStep& step) {
    auto* actor = static_cast<daE_FZ_c*>(step.actor);
    step.action = actor->mActionMode;
    step.subaction = actor->mActionPhase;
}
ModResult install() { return install_enemy_hard_mode_execute_hook<ExecuteHook>(); }

}
namespace keese {
bool attached(const e_ba_class* actor) {
    return actor->mAction == e_ba_class::ACT_WIND ||
        (actor->mAction == e_ba_class::ACT_WOLFBITE && actor->mMode < 2);
}
e_ba_class* keese(fopAc_ac_c* base) {
    // The SDK declares this actor using composition, with mEnemy at offset zero.
    static_assert(offsetof(e_ba_class, mEnemy) == 0);
    return reinterpret_cast<e_ba_class*>(base);
}
bool eligible(fopAc_ac_c* base) {
    return keese(base)->mpMorf != nullptr && !attached(keese(base));
}
void snapshot(EnemyHardModeStep& step) {
    auto* actor = keese(step.actor);
    step.action = actor->mAction;
    step.subaction = actor->mMode;
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
}
namespace tektite {
DEFINE_HOOK(&daE_TT_c::execute, ExecuteHook);
bool eligible(fopAc_ac_c* actor) {
    return static_cast<daE_TT_c*>(actor)->mpMorfSO != nullptr;
}
void snapshot(EnemyHardModeStep& step) {
    auto& actor = *static_cast<daE_TT_c*>(step.actor);
    step.chaseSpeed = true;
    step.chaseShapeYaw = true;
}
ModResult install() { return install_enemy_hard_mode_execute_hook<ExecuteHook>(); }

}
namespace gibdo {
DEFINE_HOOK(&daE_GI_c::execute, ExecuteHook);
bool eligible(fopAc_ac_c* actor) {
    return static_cast<daE_GI_c*>(actor)->mpModelMorf != nullptr;
}
void snapshot(EnemyHardModeStep& step) {
    auto& actor = *static_cast<daE_GI_c*>(step.actor);
    step.chaseSpeed = true;
    step.chaseShapeYaw = true;
}
ModResult install() { return install_enemy_hard_mode_execute_hook<ExecuteHook>(); }

}
namespace stalchild {
e_bs_class& stalchild(fopAc_ac_c* actor) {
    static_assert(offsetof(e_bs_class, enemy) == 0);
    return *reinterpret_cast<e_bs_class*>(actor);
}
bool eligible(fopAc_ac_c* actor) {
    return stalchild(actor).modelMorf != nullptr;
}
void snapshot(EnemyHardModeStep& step) {
    auto& actor = stalchild(step.actor);
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
}
namespace freezard {
DEFINE_HOOK(&daE_FB_c::execute, ExecuteHook);
bool eligible(fopAc_ac_c* actor) {
    auto& enemy = *static_cast<daE_FB_c*>(actor);
    return enemy.mpMorf != nullptr || enemy.mType == 10 || enemy.mType == 11;
}
void snapshot(EnemyHardModeStep& step) {
    auto& actor = *static_cast<daE_FB_c*>(step.actor);
    step.chaseShapeYaw = true;
}
ModResult install() { return install_enemy_hard_mode_execute_hook<ExecuteHook>(); }

}
namespace bubble {
e_bu_class& bubble(fopAc_ac_c* actor) {
    static_assert(offsetof(e_bu_class, enemy) == 0);
    return *reinterpret_cast<e_bu_class*>(actor);
}
bool eligible(fopAc_ac_c* actor) {
    return bubble(actor).modelMorf != nullptr && !fopAcM_checkHookCarryNow(actor);
}
void snapshot(EnemyHardModeStep& step) {
    auto& actor = bubble(step.actor);
    step.action = actor.action;
    step.subaction = actor.mode;
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
}
namespace fire_toadpoli {
template <typename T>
bool eligible(fopAc_ac_c* base) {
    return static_cast<T*>(base)->mpMorf != nullptr;
}
template <typename T>
void snapshot(EnemyHardModeStep& step) {
    auto& actor = *static_cast<T*>(step.actor);
    step.action = actor.mAction;
    step.subaction = actor.mMode;
    step.chaseShapeYaw = true;
}
}
namespace rat {
bool eligible(fopAc_ac_c* base) {
    const auto& actor = *static_cast<e_ms_class*>(base);
    return actor.mpModelMorf != nullptr && actor.mAction != 5;
}
void snapshot(EnemyHardModeStep& step) {
    auto& actor = *static_cast<e_ms_class*>(step.actor);
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
}
namespace bomskit {
e_cr_class& bomskit(fopAc_ac_c* base) {
    static_assert(offsetof(e_cr_class, enemy) == 0);
    return *reinterpret_cast<e_cr_class*>(base);
}
bool eligible(fopAc_ac_c* base) {
    return bomskit(base).modelMorf != nullptr;
}
void snapshot(EnemyHardModeStep& step) {
    auto& actor = bomskit(step.actor);
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
}
namespace stalhound {
e_sh_class& stalhound(fopAc_ac_c* base) {
    static_assert(offsetof(e_sh_class, enemy) == 0);
    return *reinterpret_cast<e_sh_class*>(base);
}
bool eligible(fopAc_ac_c* base) {
    return stalhound(base).mAnm_p != nullptr;
}
void snapshot(EnemyHardModeStep& step) {
    auto& actor = stalhound(step.actor);
    step.action = actor.field_0x676;
    step.subaction = actor.field_0x678;
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
}
namespace dodongo {
bool eligible(fopAc_ac_c* base) {
    return reinterpret_cast<e_dd_class*>(base)->mpModelMorf != nullptr &&
        !dComIfGp_event_runCheck();
}
void snapshot(EnemyHardModeStep& step) {
    auto& a = *reinterpret_cast<e_dd_class*>(step.actor);
    step.action = a.mAction;
    step.subaction = a.field_0x68c;
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
}
namespace skulltula {
bool eligible(fopAc_ac_c* base) {
    auto& a = *reinterpret_cast<e_st_class*>(base);
    return a.mpModelMorf != nullptr && a.mAction != 21 &&
        !(a.mAction == 15 && (a.mActionPhase == 3 || a.mActionPhase == 4)) &&
        !dComIfGp_event_runCheck();
}
void snapshot(EnemyHardModeStep& step) {
    auto& a = *reinterpret_cast<e_st_class*>(step.actor);
    step.action = a.mAction;
    step.subaction = a.mActionPhase;
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
}
}
namespace baba_serpent {
bool eligible(fopAc_ac_c* base) {
    auto& a = *reinterpret_cast<e_hb_class*>(base);
    return a.modelMorf != nullptr && a.field_0x850 == 0 && a.enemy.health != 1000 &&
        a.field_0x852 == 0 && !dComIfGp_event_runCheck();
}
void snapshot(EnemyHardModeStep& step) {
    auto& a = *reinterpret_cast<e_hb_class*>(step.actor);
    step.action = a.action;
    step.subaction = a.mode;
    step.chaseShapeYaw = true;
}
}
namespace deku_baba {
bool eligible(fopAc_ac_c* base) {
    auto& a = *reinterpret_cast<e_db_class*>(base);
    return a.modelMorf != nullptr && a.enemy.health != 1000 && a.field_0x851 == 0 &&
        !(a.action == 20 && (a.mode == 20 || a.mode == 50)) && !dComIfGp_event_runCheck();
}
void snapshot(EnemyHardModeStep& step) {
    auto& a = *reinterpret_cast<e_db_class*>(step.actor);
    step.action = a.action;
    step.subaction = a.mode;
    step.chaseSpeed = true;
    step.chaseShapeYaw = true;
}
}
namespace staltroop {
DEFINE_HOOK(&daE_ZS_c::execute, ExecuteHook);
bool eligible(fopAc_ac_c* actor) {
    return static_cast<daE_ZS_c*>(actor)->mpMorf != nullptr;
}
void snapshot(EnemyHardModeStep& step) {
    auto& actor = *static_cast<daE_ZS_c*>(step.actor);
    step.action = actor.mAction;
    step.subaction = actor.mMode;
    step.chaseCurrentYaw = true;
}
ModResult install() { return install_enemy_hard_mode_execute_hook<ExecuteHook>(); }

}
namespace puppet {
e_fs_class& puppet(fopAc_ac_c* base) {
    static_assert(offsetof(e_fs_class, mEnemy) == 0);
    return *reinterpret_cast<e_fs_class*>(base);
}
bool eligible(fopAc_ac_c* base) {
    const auto& actor = puppet(base);
    return actor.mpMorf != nullptr && actor.mAction != e_fs_class::ACT_DEMOWAIT &&
        !(actor.mAction == e_fs_class::ACT_APPEAR && actor.mMode < 2);
}
void snapshot(EnemyHardModeStep& step) {
    auto& actor = puppet(step.actor);
    step.action = actor.mAction;
    step.subaction = actor.mMode;
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
}
}
namespace big_baba {
bool eligible(fopAc_ac_c* base) {
    auto& a = *reinterpret_cast<e_gb_class*>(base);
    return a.anmP != nullptr && a.flowerAnmP != nullptr && a.demoMode == 0 &&
        a.headAction < 5 && a.flowerAction != 10 && !dComIfGp_event_runCheck();
}
void snapshot(EnemyHardModeStep& step) {
    auto& a = *reinterpret_cast<e_gb_class*>(step.actor);
    step.action = a.headAction;
    step.subaction = a.mode;
    step.chaseSpeed = true;
    step.chaseCurrentYaw = true;
    step.chaseShapeYaw = true;
}
}

namespace fire_toadpoli {
DEFINE_HOOK(&fopAcM_createChild, CreateChildHook);

struct ShotClock {
    fpc_ProcID id = fpcM_ERROR_PROCESS_ID_e;
    u8 shots = 0;
    fpc_ProcID primaryBall = fpcM_ERROR_PROCESS_ID_e;
    std::array<fpc_ProcID, 2> spreadBalls{
        fpcM_ERROR_PROCESS_ID_e, fpcM_ERROR_PROCESS_ID_e};
    bool leadShot = false;
    bool primaryLaunched = false;
};

std::array<ShotClock, 16> s_shotClocks{};
ShotClock* s_creatingShot = nullptr;
bool s_spawningSpread = false;

ShotClock& shot_clock(fopAc_ac_c* actor) {
    const auto id = fopAcM_GetID(actor);
    auto& clock = s_shotClocks[id % s_shotClocks.size()];
    if (clock.id != id) {
        clock = {};
        clock.id = id;
    }
    return clock;
}

bool is_toadpoli_profile(s16 name) {
    return name == fpcNm_E_TK_e || name == fpcNm_E_TK2_e;
}

HookAction before_create_child(ModContext*, void* args, void* retval, void*) {
    if (s_spawningSpread) return HOOK_CONTINUE;
    auto* step = current_enemy_hard_mode_step();

    const bool isBall = step != nullptr && step->actor != nullptr &&
        is_toadpoli_profile(step->profileName) &&
        mods::arg<s16>(args, 0) == fpcNm_E_TK_BALL_e &&
        mods::arg<fpc_ProcID>(args, 1) == fopAcM_GetID(step->actor);
    if (isBall && enemy_hard_mode_applies(step->profileName)) {
        auto& clock = shot_clock(step->actor);
        ++clock.shots;
        const bool isFire = step->profileName == fpcNm_E_TK2_e;
        if (!isFire && (clock.shots & 1U) != 0) return HOOK_CONTINUE;

        clock.primaryBall = fpcM_ERROR_PROCESS_ID_e;
        clock.spreadBalls = {fpcM_ERROR_PROCESS_ID_e, fpcM_ERROR_PROCESS_ID_e};
        clock.leadShot = isFire && (clock.shots & 1U) == 0;
        clock.primaryLaunched = false;
        s_creatingShot = &clock;
    }
    return HOOK_CONTINUE;
}
void after_create_child(ModContext*, void* args, void* retval, void*) {
    if (s_spawningSpread) return;

    auto* shot = s_creatingShot;
    s_creatingShot = nullptr;
    if (shot == nullptr || mods::arg<s16>(args, 0) != fpcNm_E_TK_BALL_e ||
        *static_cast<fpc_ProcID*>(retval) == fpcM_ERROR_PROCESS_ID_e) {
        return;
    }

    shot->primaryBall = *static_cast<fpc_ProcID*>(retval);

    s_spawningSpread = true;
    for (std::size_t i = 0; i < shot->spreadBalls.size(); ++i) {
        shot->spreadBalls[i] = fopAcM_createChild(
            mods::arg<s16>(args, 0), mods::arg<fpc_ProcID>(args, 1),
            mods::arg<u32>(args, 2), mods::arg<const cXyz*>(args, 3),
            mods::arg<int>(args, 4), mods::arg<const csXyz*>(args, 5),
            mods::arg<const cXyz*>(args, 6),
            mods::arg<s8>(args, 7), mods::arg<createFunc>(args, 8));
    }
    s_spawningSpread = false;
}
bool launch_ball(fpc_ProcID id, fopAc_ac_c& parent, bool leadShot, s16 yawOffset) {
    auto* ball = static_cast<e_tk_ball_class*>(fopAcM_SearchByID(id));
    auto* player = dComIfGp_getPlayer(0);
    if (ball == nullptr || ball->mpModel == nullptr || player == nullptr) return false;

    ball->current.pos = parent.eyePos;
    ball->old.pos = ball->current.pos;
    ball->home.pos = ball->current.pos;

    cXyz target = player->eyePos;
    target.y -= 20.0f;
    if (leadShot) target += player->speed * 10.0f;
    cXyz direction = target - ball->current.pos;
    f32 distance = direction.abs();
    if (distance < 0.001f) {
        direction.set(0.0f, 0.0f, 1.0f);
        distance = 1.0f;
    }

    direction *= 50.0f / distance;
    const f32 sinYaw = cM_ssin(yawOffset);
    const f32 cosYaw = cM_scos(yawOffset);
    const f32 speedX = direction.x * cosYaw + direction.z * sinYaw;
    const f32 speedZ = direction.z * cosYaw - direction.x * sinYaw;
    direction.x = speedX;
    direction.z = speedZ;

    ball->speed = direction;
    ball->current.angle.y = cM_atan2s(direction.x, direction.z);
    ball->current.angle.x = -cM_atan2s(
        direction.y, JMAFastSqrt(direction.x * direction.x + direction.z * direction.z));
    ball->mInitalPosition = ball->current.pos;
    ball->mInitalDistance = distance < 10.0f ? 10.0f : distance;
    ball->mArcHeight = 0.0f;
    ball->mAction = 0;
    ball->mMode = 1;
    ball->mActionTimer[0] = 100;
    ball->mActionTimer[1] = 0;
    ball->mAtSph.OnAtVsPlayerBit();
    ball->mAtSph.OffAtVsEnemyBit();
    ball->mAtSph.StartCAt(ball->current.pos);
    ball->mPreviousPosition = ball->current.pos;
    ball->mSuspended = false;
    return true;
}
void after_execute(EnemyHardModeStep& step) {
    auto& parent = *step.actor;
    auto& shot = shot_clock(step.actor);
    if (shot.primaryBall == fpcM_ERROR_PROCESS_ID_e) return;

    auto* primary = static_cast<e_tk_ball_class*>(fopAcM_SearchByID(shot.primaryBall));
    if (primary == nullptr || primary->mpModel == nullptr || primary->mSuspended) return;

    if (!shot.primaryLaunched) {
        shot.primaryLaunched = launch_ball(shot.primaryBall, parent, shot.leadShot, 0);
    }

    static constexpr std::array<s16, 2> offsets{-0x900, 0x900};
    for (std::size_t i = 0; i < shot.spreadBalls.size(); ++i) {
        if (shot.spreadBalls[i] != fpcM_ERROR_PROCESS_ID_e &&
            launch_ball(shot.spreadBalls[i], parent, shot.leadShot, offsets[i])) {
            shot.spreadBalls[i] = fpcM_ERROR_PROCESS_ID_e;
        }
    }

    if (shot.primaryLaunched &&
        shot.spreadBalls[0] == fpcM_ERROR_PROCESS_ID_e &&
        shot.spreadBalls[1] == fpcM_ERROR_PROCESS_ID_e) {
        shot.primaryBall = fpcM_ERROR_PROCESS_ID_e;
        shot.primaryLaunched = false;
    }
}
ModResult install() {
    auto result = mods::hook::add_pre<CreateChildHook>(svc_hook, before_create_child);
    if (result == MOD_OK) result = mods::hook::add_post<CreateChildHook>(svc_hook, after_create_child);
    return result;
}
void reset() {
    s_shotClocks = {};
    s_creatingShot = nullptr;
    s_spawningSpread = false;
}
}
namespace bulblin {
DEFINE_HOOK(&fopAcM_createChild, ArrowHook);

struct ShotClock {
    fpc_ProcID id = fpcM_ERROR_PROCESS_ID_e;
    u8 shots = 0;
};

std::array<ShotClock, 16> s_shotClocks{};
bool s_spawningSpread = false;

ShotClock& shot_clock(fopAc_ac_c* actor) {
    const auto id = fopAcM_GetID(actor);
    auto& clock = s_shotClocks[id % s_shotClocks.size()];
    if (clock.id != id) clock = {id, 0};
    return clock;
}

void after_arrow(ModContext*, void* args, void* retval, void*) {
    if (s_spawningSpread || mods::arg<s16>(args, 0) != fpcNm_E_ARROW_e ||
        *static_cast<fpc_ProcID*>(retval) == fpcM_ERROR_PROCESS_ID_e) {
        return;
    }

    auto* step = current_enemy_hard_mode_step();
    if (step == nullptr || step->actor == nullptr ||
        step->profileName != fpcNm_E_RD_e ||
        mods::arg<fpc_ProcID>(args, 1) != fopAcM_GetID(step->actor) ||
        !enemy_hard_mode_applies(fpcNm_E_RD_e)) {
        return;
    }

    auto& clock = shot_clock(step->actor);
    if (++clock.shots % 2 != 0) return;

    const auto* source = mods::arg<const csXyz*>(args, 5);
    if (source == nullptr) return;

    s_spawningSpread = true;
    for (const s16 offset : {-0x900, 0x900}) {
        csXyz angle = *source;
        angle.y = static_cast<s16>(angle.y + offset);
        fopAcM_createChild(
            mods::arg<s16>(args, 0), mods::arg<fpc_ProcID>(args, 1),
            mods::arg<u32>(args, 2), mods::arg<const cXyz*>(args, 3),
            mods::arg<int>(args, 4), &angle, mods::arg<const cXyz*>(args, 6),
            mods::arg<s8>(args, 7), mods::arg<createFunc>(args, 8));
    }
    s_spawningSpread = false;
}
ModResult install() {
    return mods::hook::add_post<ArrowHook>(svc_hook, after_arrow);
}
void reset() {
    s_shotClocks = {};
    s_spawningSpread = false;
}
}
namespace chilfos {
DEFINE_HOOK(&fopAcM_createChild, SpearHook);
csXyz s_spearAngle{};
int s_spearCount = 0;

void* count_spears(void* process, void*) {
    if (fopAcM_IsActor(process) && fopAcM_GetName(process) == fpcNm_E_KK_e &&
        fopAcM_GetParam(process) == 0xFF0001) ++s_spearCount;
    return nullptr;
}

EnemyHardModeStep* chilfos_step() {
    auto* step = current_enemy_hard_mode_step();
    return step != nullptr && step->actor != nullptr && step->profileName == fpcNm_E_KK_e ? step : nullptr;
}

HookAction before_spear(ModContext*, void* args, void* retval, void*) {
    auto* step = chilfos_step();
    const bool isSpear = step != nullptr &&
        mods::arg<s16>(args, 0) == fpcNm_E_KK_e &&
        mods::arg<fpc_ProcID>(args, 1) == fopAcM_GetID(step->actor) &&
        mods::arg<u32>(args, 2) == 0xFF0001;

    if (isSpear && enemy_hard_mode_applies(fpcNm_E_KK_e)) {
        s_spearCount = 0;
        fpcM_Search(count_spears, nullptr);
        if (s_spearCount >= 3) {
            *static_cast<fpc_ProcID*>(retval) = fpcM_ERROR_PROCESS_ID_e;
            return HOOK_SKIP_ORIGINAL;
        }

        const auto* source = mods::arg<const csXyz*>(args, 5);
        s_spearAngle = source != nullptr ? *source : step->actor->shape_angle;
        auto* player = dComIfGp_getPlayer(0);
        if (player != nullptr) {
            const cXyz target = player->current.pos + player->speed * 8.0f;
            const cXyz delta = target - step->actor->current.pos;
            s_spearAngle.y = cM_atan2s(delta.x, delta.z);
        }
        mods::arg_ref<const csXyz*>(args, 5) = &s_spearAngle;
    }
    return HOOK_CONTINUE;
}
}
DEFINE_HOOK(&e_ai_class::e_ai_damage, ArmosDamageHook);
HookAction before_damage(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<e_ai_class*>(args, 0);
    if (a->field_0x692 != 0 || a->m_mode != 1 || a->m_timers[1] != 0)
        return HOOK_CONTINUE;

    // Native e_ai_damage unconditionally dereferences the emitter returned for
    // this scene-specific flash. It may be absent in a spawner/randomizer room,
    // or allocation may fail. Start the same flash/countdown safely, then let
    // native movement, emitter tracking, explosion, switch and deletion run.
    a->m_sound.startCreatureSound(Z2SE_EN_AI_FLASH, 0, -1);
    a->mpEmitter = dComIfGp_particle_set(0x81ED, &a->current.pos, &a->tevStr,
                                       &a->shape_angle, nullptr);
    if (a->mpEmitter) a->mpEmitter->becomeImmortalEmitter();
    a->m_timers[1] = 1000; // Bypass only the unsafe native flash-start block.
    a->m_timers[2] = 56;
    return HOOK_CONTINUE;
}
struct HardModeProfile {
    int name;
    bool (*eligible)(fopAc_ac_c*);
    void (*snapshot)(EnemyHardModeStep&);
    ModResult (*install)();
};
const HardModeProfile kProfiles[]{
    {fpcNm_B_TN_e, darknut::eligible, darknut::snapshot, darknut::install},
    {fpcNm_E_OC_e, bokoblin::eligible, bokoblin::snapshot, bokoblin::install},
    {fpcNm_B_GG_e, aeralfos::eligible, aeralfos::snapshot, aeralfos::install},
    {fpcNm_E_KK_e, chilfos::eligible, chilfos::snapshot, chilfos::install},
    {fpcNm_E_WW_e, white_wolfos::eligible, white_wolfos::snapshot, white_wolfos::install},
    {fpcNm_E_RD_e, bulblin::eligible, bulblin::snapshot, nullptr},
    {fpcNm_E_DN_e, lizalfos::eligible, lizalfos::snapshot, nullptr},
    {fpcNm_E_MF_e, dynalfos::eligible, dynalfos::snapshot, nullptr},
    {fpcNm_E_SF_e, stalfos::eligible, stalfos::snapshot, nullptr},
    {fpcNm_E_FZ_e, mini_freezard::eligible, mini_freezard::snapshot, mini_freezard::install},
    {fpcNm_E_BA_e, keese::eligible, keese::snapshot, nullptr},
    {fpcNm_E_TT_e, tektite::eligible, tektite::snapshot, tektite::install},
    {fpcNm_E_GI_e, gibdo::eligible, gibdo::snapshot, gibdo::install},
    {fpcNm_E_BS_e, stalchild::eligible, stalchild::snapshot, nullptr},
    {fpcNm_E_FB_e, freezard::eligible, freezard::snapshot, freezard::install},
    {fpcNm_E_BU_e, bubble::eligible, bubble::snapshot, nullptr},
    {fpcNm_E_TK_e, fire_toadpoli::eligible<e_tk_class>, fire_toadpoli::snapshot<e_tk_class>, nullptr},
    {fpcNm_E_TK2_e, fire_toadpoli::eligible<e_tk2_class>, fire_toadpoli::snapshot<e_tk2_class>, nullptr},
    {fpcNm_E_MS_e, rat::eligible, rat::snapshot, nullptr},
    {fpcNm_E_CR_e, bomskit::eligible, bomskit::snapshot, nullptr},
    {fpcNm_E_SH_e, stalhound::eligible, stalhound::snapshot, nullptr},
    {fpcNm_E_DD_e, dodongo::eligible, dodongo::snapshot, nullptr},
    {fpcNm_E_ST_e, skulltula::eligible, skulltula::snapshot, nullptr},
    {fpcNm_E_HB_e, baba_serpent::eligible, baba_serpent::snapshot, nullptr},
    {fpcNm_E_DB_e, deku_baba::eligible, deku_baba::snapshot, nullptr},
    {fpcNm_E_ZS_e, staltroop::eligible, staltroop::snapshot, staltroop::install},
    {fpcNm_E_FS_e, puppet::eligible, puppet::snapshot, nullptr},
    {fpcNm_E_GB_e, big_baba::eligible, big_baba::snapshot, nullptr}
};
const HardModeProfile* find_profile(fopAc_ac_c* actor) {
    if (!actor) return nullptr;
    for (const auto& profile : kProfiles) if (profile.name == fopAcM_GetName(actor)) return &profile;
    return nullptr;
}
std::array<EnemyHardModeStep, 8> s_steps{};
size_t s_depth = 0;
std::array<bool, 64> s_processFrames{};
size_t s_processDepth = 0;
DEFINE_HOOK(&fpcMtd_Method, ProcessMethodHook);
DEFINE_HOOK(&cLib_addCalc2, ChaseTargetHook);
DEFINE_HOOK(&cLib_addCalc, ChaseTargetMinHook);
DEFINE_HOOK(&cLib_addCalc0, ChaseZeroHook);
DEFINE_HOOK(&cLib_chaseF, ChaseLinearHook);
DEFINE_HOOK(&cLib_addCalcAngleS2, ChaseAngleHook);
DEFINE_HOOK(&cLib_addCalcAngleS, ChaseAngleMinHook);
DEFINE_HOOK(&cLib_chaseS, ChaseShortHook);
DEFINE_HOOK(&cLib_chaseAngleS, ChaseAngleLinearHook);
HookAction before_process_method(ModContext* ctx, void* args, void* retval, void*) {
    bool entered = false;
    void* process = mods::arg<void*>(args, 1);
    if (s_processDepth < s_processFrames.size() && process != nullptr && fopAcM_IsActor(process)) {
        auto* actor = static_cast<fopAc_ac_c*>(process);
        const auto* profile = find_profile(actor);
        const auto* methods = reinterpret_cast<const process_method_class*>(actor->sub_method);
        if (methods != nullptr &&
            mods::arg<process_method_func>(args, 0) == methods->execute_method) {
            void* actorArgument = actor;
            void* executeArgs[]{&actorArgument};
            if (profile != nullptr && profile->install == nullptr) {
                before_enemy_hard_mode_execute(ctx, executeArgs, retval, nullptr);
            } else {
                // A nested unrelated actor masks its parent. Typed actor hooks
                // push their own scope inside this placeholder, avoiding double ticks.
                if (s_depth < s_steps.size()) s_steps[s_depth] = {};
                ++s_depth;
            }
            entered = true;
        }
    }
    if (s_processDepth < s_processFrames.size()) s_processFrames[s_processDepth] = entered;
    ++s_processDepth;
    return HOOK_CONTINUE;
}
void after_process_method(ModContext* ctx, void*, void*, void*) {
    if (s_processDepth != 0 && --s_processDepth < s_processFrames.size()) {
        if (s_processFrames[s_processDepth]) after_enemy_hard_mode_execute(ctx, nullptr, nullptr, nullptr);
        s_processFrames[s_processDepth] = false;
    }
}
bool owns_chase_float(const EnemyHardModeStep* step, const float* value) {
    return step && step->chaseSpeed && value == &step->actor->speedF;
}
bool owns_chase_angle(const EnemyHardModeStep* step, const s16* value) {
    return step && ((step->chaseCurrentYaw && value == &step->actor->current.angle.y) ||
        (step->chaseShapeYaw && value == &step->actor->shape_angle.y));
}
HookAction before_chase_target(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_hard_mode_step();
    if (owns_chase_float(step, mods::arg<float*>(args, 0))) {
        const float hardScale = enemy_hard_mode_chase_scale(*step, mods::arg<float*>(args, 0));
        mods::arg_ref<float>(args, 2) *= hardScale;
        mods::arg_ref<float>(args, 3) *= hardScale;
    }
    return HOOK_CONTINUE;
}
HookAction before_chase_target_min(ModContext* ctx, void* args, void* result, void* user) {
    auto* step = current_enemy_hard_mode_step();
    if (owns_chase_float(step, mods::arg<float*>(args, 0)))
        mods::arg_ref<float>(args, 4) *= enemy_hard_mode_chase_scale(*step, mods::arg<float*>(args, 0));
    return before_chase_target(ctx, args, result, user);
}
HookAction before_chase_zero(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_hard_mode_step();
    if (owns_chase_float(step, mods::arg<float*>(args, 0))) {
        const float hardScale = enemy_hard_mode_chase_scale(*step, mods::arg<float*>(args, 0));
        mods::arg_ref<float>(args, 1) *= hardScale;
        mods::arg_ref<float>(args, 2) *= hardScale;
    }
    return HOOK_CONTINUE;
}
HookAction before_chase_linear(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_hard_mode_step();
    if (owns_chase_float(step, mods::arg<float*>(args, 0))) {
        mods::arg_ref<float>(args, 2) *= enemy_hard_mode_chase_scale(*step, mods::arg<float*>(args, 0));
    }
    return HOOK_CONTINUE;
}
float hard_mode_turn_scale(const EnemyHardModeStep& step, const s16* value) {
    if (value != &step.actor->current.angle.y && value != &step.actor->shape_angle.y) {
        return 1.0f;
    }
    return enemy_hard_mode_turn_scale(step);
}
HookAction before_chase_angle(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_hard_mode_step();
    if (step == nullptr || step->actor == nullptr) return HOOK_CONTINUE;
    for (auto* owned : {&step->actor->current.angle.y, &step->actor->shape_angle.y}) {
        if (owned == mods::arg<s16*>(args, 0) && owns_chase_angle(step, owned)) {
            const float hardScale = hard_mode_turn_scale(*step, owned);
            auto& divisor = mods::arg_ref<s16>(args, 2);
            auto& maximum = mods::arg_ref<s16>(args, 3);
            if (divisor > 0) {
                divisor = static_cast<s16>(std::clamp(
                    std::lround(divisor / hardScale), 1L, 32767L));
            }
            if (maximum > 0) {
                maximum = static_cast<s16>(std::clamp(
                    std::lround(maximum * hardScale), 1L, 32767L));
            }
            break;
        }
    }
    return HOOK_CONTINUE;
}
HookAction before_chase_angle_min(ModContext* ctx, void* args, void* ret, void* data) {
    auto* step = current_enemy_hard_mode_step();
    if (owns_chase_angle(step, mods::arg<s16*>(args, 0))) {
        auto& minimum = mods::arg_ref<s16>(args, 4);
        if (minimum > 0) {
            minimum = static_cast<s16>(std::clamp(
                std::lround(minimum *
                    hard_mode_turn_scale(*step, mods::arg<s16*>(args, 0))),
                1L, 32767L));
        }
        return before_chase_angle(ctx, args, ret, data);
    }
    return HOOK_CONTINUE;
}
HookAction before_chase_short(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_hard_mode_step();
    if (owns_chase_angle(step, mods::arg<s16*>(args, 0))) {
        auto& maximum = mods::arg_ref<s16>(args, 2);
        if (maximum > 0) {
            maximum = static_cast<s16>(std::clamp(
                std::lround(maximum *
                    hard_mode_turn_scale(*step, mods::arg<s16*>(args, 0))),
                1L, 32767L));
        }
    }
    return HOOK_CONTINUE;
}
} // namespace
EnemyHardModeStep* current_enemy_hard_mode_step() {
    if (s_depth == 0 || s_depth > s_steps.size()) return nullptr;
    auto& step = s_steps[s_depth - 1];
    return step.actor ? &step : nullptr;
}
HookAction before_enemy_hard_mode_execute(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<fopAc_ac_c*>(args, 0);
    const auto* profile = find_profile(actor);
    EnemyHardModeStep step{};
    if (profile && enemy_hard_mode_applies(profile->name) && profile->eligible(actor) && s_depth < s_steps.size()) {
        step.actor = actor;
        step.profileName = profile->name;
        profile->snapshot(step);
        prepare_enemy_hard_mode(step);
    }
    if (s_depth < s_steps.size()) s_steps[s_depth] = step;
    ++s_depth;
    return HOOK_CONTINUE;
}
void after_enemy_hard_mode_execute(ModContext*, void*, void*, void*) {
    if (auto* step = current_enemy_hard_mode_step()) {
        if (fire_toadpoli::is_toadpoli_profile(step->profileName)) fire_toadpoli::after_execute(*step);
        finish_enemy_hard_mode(*step);
    }
    if (s_depth && --s_depth < s_steps.size()) s_steps[s_depth] = {};
}
ModResult initialize_enemy_hard_mode_runtime() {
    for (const auto& profile : kProfiles) {
        if (!profile.install) continue;
        const auto result = profile.install();
        if (result != MOD_OK) return result;
    }
    auto result = fire_toadpoli::install();
    if (result == MOD_OK) result = bulblin::install();
    if (result == MOD_OK) result = mods::hook::add_pre<chilfos::SpearHook>(svc_hook, chilfos::before_spear);
    if (result == MOD_OK) result = mods::hook::add_pre<ArmosDamageHook>(svc_hook, before_damage);
    if (result == MOD_OK) result = install_enemy_hard_mode_hooks();
    if (result == MOD_OK) result = mods::hook::add_pre<ProcessMethodHook>(svc_hook, before_process_method);
    if (result == MOD_OK) result = mods::hook::add_post<ProcessMethodHook>(svc_hook, after_process_method);
    if (result == MOD_OK) result = mods::hook::add_pre<ChaseTargetHook>(svc_hook, before_chase_target);
    if (result == MOD_OK) result = mods::hook::add_pre<ChaseTargetMinHook>(svc_hook, before_chase_target_min);
    if (result == MOD_OK) result = mods::hook::add_pre<ChaseZeroHook>(svc_hook, before_chase_zero);
    if (result == MOD_OK) result = mods::hook::add_pre<ChaseLinearHook>(svc_hook, before_chase_linear);
    if (result == MOD_OK) result = mods::hook::add_pre<ChaseAngleHook>(svc_hook, before_chase_angle);
    if (result == MOD_OK) result = mods::hook::add_pre<ChaseAngleMinHook>(svc_hook, before_chase_angle_min);
    if (result == MOD_OK) result = mods::hook::add_pre<ChaseShortHook>(svc_hook, before_chase_short);
    if (result == MOD_OK) result = mods::hook::add_pre<ChaseAngleLinearHook>(svc_hook, before_chase_short);
    return result;
}
void reset_enemy_hard_mode_runtime() {
    s_steps = {};
    s_depth = 0;
    s_processFrames = {};
    s_processDepth = 0;
    fire_toadpoli::reset();
    bulblin::reset();
    reset_enemy_hard_mode();
}
} // namespace dawnlight
