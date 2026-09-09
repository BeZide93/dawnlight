#include "profile.hpp"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_ww.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daE_WW_c::execute, ExecuteHook);

bool eligible(fopAc_ac_c* base) {
    const auto& actor = *static_cast<daE_WW_c*>(base);
    // The invisible pack coordinator does not animate or move like a wolf.
    return actor.mpModelMorf != nullptr && actor.mAction != 0;
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto& actor = *static_cast<daE_WW_c*>(step.actor);
    step.animations = {actor.mpModelMorf};
    step.chaseFloats = {&actor.speedF};
    step.chaseAngles = {&actor.current.angle.y, &actor.shape_angle.x,
        &actor.shape_angle.y, &actor.field_0x674.x, &actor.field_0x674.z,
        &actor.field_0x67a, &actor.field_0x67c, &actor.field_0x6ce};
    if (!timerTick) {
        hold_enemy_timer(actor.field_0x724);
        hold_enemy_timer(actor.field_0x728);
        hold_enemy_timer(actor.field_0x72c);
        hold_enemy_timer(actor.field_0x730);
        hold_enemy_timer(actor.field_0x734);
        hold_enemy_timer(actor.field_0x738);
        hold_enemy_timer(actor.field_0x73c);
        hold_enemy_timer(actor.field_0x740);
    }
}
}

const EnemySlowProfile& white_wolfos_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_E_WW_e, eligible, prepare, nullptr, nullptr,
        install_enemy_execute_hook<ExecuteHook>, nullptr
    };
    return profile;
}
}
