#include "enemy_hard_mode.hpp"

#include "config.hpp"
#include "enemy_slow_motion/profile.hpp"

#include "m_Do/m_Do_ext.h"

#include "d/actor/d_a_b_gg.h"
#include "d/actor/d_a_b_tn.h"
#include "d/actor/d_a_e_ba.h"
#include "d/actor/d_a_e_bs.h"
#include "d/actor/d_a_e_bu.h"
#include "d/actor/d_a_e_cr.h"
#include "d/actor/d_a_e_db.h"
#include "d/actor/d_a_e_dd.h"
#include "d/actor/d_a_e_dn.h"
#include "d/actor/d_a_e_fb.h"
#include "d/actor/d_a_e_fs.h"
#include "d/actor/d_a_e_fz.h"
#include "d/actor/d_a_e_gb.h"
#include "d/actor/d_a_e_gi.h"
#include "d/actor/d_a_e_hb.h"
#include "d/actor/d_a_e_kk.h"
#include "d/actor/d_a_e_mf.h"
#include "d/actor/d_a_e_ms.h"
#include "d/actor/d_a_e_oc.h"
#include "d/actor/d_a_e_rd.h"
#include "d/actor/d_a_e_sf.h"
#include "d/actor/d_a_e_sh.h"
#include "d/actor/d_a_e_st.h"
#include "d/actor/d_a_e_tk2.h"
#include "d/actor/d_a_e_tt.h"
#include "d/actor/d_a_e_ww.h"
#include "d/actor/d_a_e_zs.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_name.h"

#include <array>
#include <cstdint>

namespace dawnlight {
namespace {

constexpr float kTurnScale = 1.25f;

struct CadenceClock {
    fopAc_ac_c* actor = nullptr;
    fpc_ProcID id = fpcM_ERROR_PROCESS_ID_e;
    std::uint8_t phase = 0;
    bool followUp = false;
};

std::array<CadenceClock, 64> s_cadenceClocks{};

CadenceClock& cadence_clock_for(fopAc_ac_c* actor) {
    for (auto& clock : s_cadenceClocks) {
        if (clock.actor == actor && clock.id == fopAcM_GetID(actor)) return clock;
    }
    for (auto& clock : s_cadenceClocks) {
        if (clock.actor == nullptr || fopAcM_SearchByID(clock.id) != clock.actor) {
            clock = {actor, fopAcM_GetID(actor),
                     static_cast<std::uint8_t>(fopAcM_GetID(actor) % 3), false};
            return clock;
        }
    }
    auto& clock = s_cadenceClocks[fopAcM_GetID(actor) % s_cadenceClocks.size()];
    clock = {actor, fopAcM_GetID(actor),
             static_cast<std::uint8_t>(fopAcM_GetID(actor) % 3), false};
    return clock;
}

template <typename T>
void shorten_timer(T& timer) {
    if (timer > 0) --timer;
}

void shorten_attack_interval(EnemySlowStep& step) {
    switch (step.profile->name) {
    case fpcNm_B_TN_e:
        {
            auto& actor = *static_cast<daB_TN_c*>(step.actor);
            shorten_timer(actor.mTimer3);
            if (actor.mActionMode1 == daB_TN_c::ACT_ATTACKH ||
                actor.mActionMode1 == daB_TN_c::ACT_ATTACKL ||
                actor.mActionMode1 == daB_TN_c::ACT_GUARDH ||
                actor.mActionMode1 == daB_TN_c::ACT_GUARDL) {
                shorten_timer(actor.mTimer1);
            }
        }
        break;
    case fpcNm_E_OC_e:
        {
            auto& actor = *static_cast<daE_OC_c*>(step.actor);
            shorten_timer(actor.field_0x6c0);
            shorten_timer(actor.field_0x6c2);
            shorten_timer(actor.field_0x6c4);
        }
        break;
    case fpcNm_B_GG_e:
        {
            auto& actor = *static_cast<daB_GG_c*>(step.actor);
            shorten_timer(actor.mTimers[0]);
            shorten_timer(actor.field_0x65a);
        }
        break;
    case fpcNm_E_KK_e:
        {
            auto& actor = *static_cast<daE_KK_c*>(step.actor);
            shorten_timer(actor.mTimer);
            shorten_timer(actor.field_0x672);
        }
        break;
    case fpcNm_E_WW_e:
        {
            auto& actor = *static_cast<daE_WW_c*>(step.actor);
            shorten_timer(actor.field_0x728);
            shorten_timer(actor.field_0x734);
        }
        break;
    case fpcNm_E_RD_e: {
        auto& actor = *reinterpret_cast<e_rd_class*>(step.actor);
        shorten_timer(actor.attack_timer);
        shorten_timer(actor.timer[0]);
        shorten_timer(actor.timer[2]);
        if (actor.bow_anm != nullptr) shorten_timer(actor.bow_shake_timer);
        break;
    }
    case fpcNm_E_DN_e: {
        auto& actor = *reinterpret_cast<e_dn_class*>(step.actor);
        shorten_timer(actor.timer[0]);
        shorten_timer(actor.timer[2]);
        shorten_timer(actor.unk_timer_1);
        break;
    }
    case fpcNm_E_MF_e: {
        auto& actor = *reinterpret_cast<e_mf_class*>(step.actor);
        shorten_timer(actor.field_0x6c0[0]);
        shorten_timer(actor.field_0x6c0[2]);
        shorten_timer(actor.field_0x6c0[3]);
        break;
    }
    case fpcNm_E_SF_e: {
        auto& actor = *reinterpret_cast<e_sf_class*>(step.actor);
        shorten_timer(actor.mTimers[0]);
        shorten_timer(actor.mTimers[2]);
        if (actor.mAction == 8 || actor.mAction == 9) shorten_timer(actor.mTimers[1]);
        break;
    }
    case fpcNm_E_FZ_e: {
        auto& actor = *static_cast<daE_FZ_c*>(step.actor);
        shorten_timer(actor.field_0x710);
        shorten_timer(actor.field_0x711);
        break;
    }
    case fpcNm_E_BA_e: {
        auto& actor = *reinterpret_cast<e_ba_class*>(step.actor);
        shorten_timer(actor.mTimer[0]);
        shorten_timer(actor.mTimer[1]);
        break;
    }
    case fpcNm_E_TT_e: {
        auto& actor = *static_cast<daE_TT_c*>(step.actor);
        shorten_timer(actor.mAttackTimer);
        if (actor.mAction == 0 || actor.mMode >= 4) shorten_timer(actor.mGenericTimer);
        break;
    }
    case fpcNm_E_GI_e:
        shorten_timer(static_cast<daE_GI_c*>(step.actor)->field_0x684);
        break;
    case fpcNm_E_ZS_e: {
        auto& actor = *static_cast<daE_ZS_c*>(step.actor);
        shorten_timer(actor.field_0x670);
        shorten_timer(actor.field_0x671);
        break;
    }
    case fpcNm_E_FB_e:
        {
            auto& actor = *static_cast<daE_FB_c*>(step.actor);
            shorten_timer(actor.field_0x680);
            if (actor.mActionMode == 1 && actor.mMoveMode >= 3) {
                shorten_timer(actor.field_0x69c);
            }
        }
        break;
    case fpcNm_E_BS_e: {
        auto& actor = *reinterpret_cast<e_bs_class*>(step.actor);
        shorten_timer(actor.timers[0]);
        shorten_timer(actor.timers[1]);
        shorten_timer(actor.timers[2]);
        break;
    }
    case fpcNm_E_BU_e: {
        auto& actor = *reinterpret_cast<e_bu_class*>(step.actor);
        shorten_timer(actor.timers[0]);
        shorten_timer(actor.timers[1]);
        break;
    }
    case fpcNm_E_MS_e: {
        auto& actor = *static_cast<e_ms_class*>(step.actor);
        shorten_timer(actor.mActionTimer[0]);
        shorten_timer(actor.mActionTimer[2]);
        break;
    }
    case fpcNm_E_FS_e: {
        auto& actor = *reinterpret_cast<e_fs_class*>(step.actor);
        if (actor.mAction == e_fs_class::ACT_WAIT || actor.mAction == e_fs_class::ACT_MOVE) {
            shorten_timer(actor.mTimer[0]);
        }
        break;
    }
    case fpcNm_E_CR_e: {
        auto& actor = *reinterpret_cast<e_cr_class*>(step.actor);
        shorten_timer(actor.timers[0]);
        shorten_timer(actor.timers[1]);
        shorten_timer(actor.timers[3]);
        break;
    }
    case fpcNm_E_SH_e: {
        auto& actor = *reinterpret_cast<e_sh_class*>(step.actor);
        if (actor.field_0x676 == 3 && actor.field_0x678 == 1) {
            shorten_timer(actor.field_0x698[0]);
        }
        break;
    }
    case fpcNm_E_TK2_e: {
        auto& actor = *static_cast<e_tk2_class*>(step.actor);
        if (actor.mAction == 2) shorten_timer(actor.mActionTimer[0]);
        break;
    }
    case fpcNm_E_DD_e: {
        auto& actor = *reinterpret_cast<e_dd_class*>(step.actor);
        if (actor.mAction == 4 && (actor.field_0x68c == 0 || actor.field_0x68c >= 4)) {
            shorten_timer(actor.field_0x6aa[0]);
        }
        break;
    }
    case fpcNm_E_ST_e: {
        auto& actor = *reinterpret_cast<e_st_class*>(step.actor);
        if (actor.mAction == 3 || actor.mAction == 0x0B ||
            actor.mAction == 0x0E || actor.mAction == 0x0F ||
            actor.mAction == 0x33) {
            shorten_timer(actor.mTimers[0]);
            if (actor.mAction == 0x33) shorten_timer(actor.mDefTimer);
        }
        break;
    }
    case fpcNm_E_HB_e:
        {
            auto& actor = *reinterpret_cast<e_hb_class*>(step.actor);
            shorten_timer(actor.timers[0]);
            shorten_timer(actor.timers[1]);
        }
        break;
    case fpcNm_E_GB_e:
        {
            auto& actor = *reinterpret_cast<e_gb_class*>(step.actor);
            shorten_timer(actor.timer[0]);
            shorten_timer(actor.timer[1]);
        }
        break;
    case fpcNm_E_DB_e:
        {
            auto& actor = *reinterpret_cast<e_db_class*>(step.actor);
            shorten_timer(actor.timers[0]);
            shorten_timer(actor.timers[1]);
        }
        break;
    default:
        break;
    }
}

}  // namespace

bool enemy_hard_mode_applies(int profileName) {
    if (!enemy_hard_mode_enabled()) return false;
    switch (profileName) {
    case fpcNm_B_TN_e:
    case fpcNm_E_OC_e:
    case fpcNm_B_GG_e:
    case fpcNm_E_KK_e:
    case fpcNm_E_WW_e:
    case fpcNm_E_RD_e:
    case fpcNm_E_DN_e:
    case fpcNm_E_MF_e:
    case fpcNm_E_SF_e:
    case fpcNm_E_FZ_e:
    case fpcNm_E_BA_e:
    case fpcNm_E_TT_e:
    case fpcNm_E_GI_e:
    case fpcNm_E_ZS_e:
    case fpcNm_E_FB_e:
    case fpcNm_E_BS_e:
    case fpcNm_E_BU_e:
    case fpcNm_E_MS_e:
    case fpcNm_E_FS_e:
    case fpcNm_E_CR_e:
    case fpcNm_E_SH_e:
    case fpcNm_E_TK2_e:
    case fpcNm_E_DD_e:
    case fpcNm_E_ST_e:
    case fpcNm_E_HB_e:
    case fpcNm_E_GB_e:
    case fpcNm_E_DB_e:
        return true;
    default:
        return false;
    }
}

float enemy_hard_mode_turn_scale(const EnemySlowStep& step) {
    if (step.profile == nullptr || !enemy_hard_mode_applies(step.profile->name)) return 1.0f;
    switch (step.profile->name) {
    case fpcNm_E_FZ_e:
    case fpcNm_E_BA_e:
    case fpcNm_E_BU_e:
    case fpcNm_E_MS_e:
    case fpcNm_E_SH_e:
    case fpcNm_E_DD_e:
    case fpcNm_E_MF_e:
        return 1.4f;
    case fpcNm_B_TN_e:
        return static_cast<daB_TN_c*>(step.actor)->mActionMode1 >= daB_TN_c::ACT_CHASEL
            ? 1.4f : kTurnScale;
    default:
        return kTurnScale;
    }
}

float enemy_hard_mode_chase_scale(const EnemySlowStep& step, const float* value) {
    if (step.actor == nullptr || step.profile == nullptr || value != &step.actor->speedF ||
        !enemy_hard_mode_applies(step.profile->name)) return 1.0f;
    switch (step.profile->name) {
    case fpcNm_E_FZ_e:
    case fpcNm_E_BA_e:
    case fpcNm_E_BU_e:
    case fpcNm_E_MS_e:
    case fpcNm_E_CR_e:
    case fpcNm_E_SH_e:
    case fpcNm_E_HB_e:
    case fpcNm_E_GB_e:
    case fpcNm_E_DB_e:
        return 1.4f;
    case fpcNm_B_TN_e:
        return static_cast<daB_TN_c*>(step.actor)->mActionMode1 >= daB_TN_c::ACT_CHASEL
            ? 1.3f : 1.15f;
    default:
        return 1.2f;
    }
}

void prepare_enemy_hard_mode(EnemySlowStep& step) {
    if (step.actor == nullptr || step.profile == nullptr || !step.timerTick ||
        !enemy_hard_mode_applies(step.profile->name)) {
        return;
    }

    auto& clock = cadence_clock_for(step.actor);
    clock.phase = static_cast<std::uint8_t>((clock.phase + 1) % 3);
    // Native logic removes three timer points over three frames. Applying one
    // additional decrement on two cadence phases raises that to five points
    // while preserving the per-actor staggering.
    if (clock.phase < 2) shorten_attack_interval(step);
}

void finish_enemy_hard_mode(EnemySlowStep& step) {
    if (step.actor == nullptr || step.profile == nullptr ||
        !enemy_hard_mode_applies(step.profile->name)) return;

    // A bounded second pounce makes some Stalhounds less predictable without
    // allowing an endless attack loop. Actor-ID parity also staggers packs.
    if (step.profile->name == fpcNm_E_SH_e) {
        auto& actor = *reinterpret_cast<e_sh_class*>(step.actor);
        auto& clock = cadence_clock_for(step.actor);
        if (step.action != 3 && actor.field_0x676 == 3) {
            clock.followUp = (fopAcM_GetID(step.actor) & 1U) != 0;
        } else if (step.action == 3 && step.subaction == 4 &&
                   actor.field_0x676 == 2 && clock.followUp) {
            actor.field_0x676 = 3;
            actor.field_0x678 = 0;
            clock.followUp = false;
        }
    }
}

void reset_enemy_hard_mode() {
    s_cadenceClocks = {};
}

}  // namespace dawnlight
