#include "profile.hpp"
#include "d/d_com_inf_game.h"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_e_fz.h"
#include "f_pc/f_pc_name.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daE_FZ_c::execute, ExecuteHook);

bool eligible(fopAc_ac_c* base) {
    auto* actor = static_cast<daE_FZ_c*>(base);
    // Blizzeta controls orbiting children directly; the iron-ball-gated type
    // can return before decrementing its timers. Neither uses this profile.
    return actor->mpModel != nullptr && actor->field_0x714 != 2 &&
        actor->field_0x714 != 3 && actor->mpBlizzetaActor == nullptr &&
        actor->mActionMode != ACT_ROLLMOVE;
}

void prepare(EnemySlowStep& step, bool timerTick) {
    auto* actor = static_cast<daE_FZ_c*>(step.actor);
    step.action = actor->mActionMode;
    step.subaction = actor->mActionPhase;
    step.waist = actor->current.angle.y;
    if (timerTick) return;
    hold_enemy_timer(actor->field_0x710);
    hold_enemy_timer(actor->field_0x711);
    hold_enemy_timer(actor->field_0x712);
}

void before_move(EnemySlowStep& step) {
    auto* actor = static_cast<daE_FZ_c*>(step.actor);
    if (actor->mActionMode != step.action || actor->mActionPhase != step.subaction) return;
    actor->shape_angle.y = slow_enemy_angle(step.facing, actor->shape_angle.y, step.scale);
    // A spinning ice body does not move in its visual facing direction.
    // Preserve wall reflection impulses instead of blending through the wall.
    if (!actor->mObjAcch.ChkWallHit()) {
        actor->current.angle.y = slow_enemy_angle(step.waist, actor->current.angle.y, step.scale);
    }
    actor->speedF = step.speed + (actor->speedF - step.speed) * step.scale;
}

ModResult install() { return install_enemy_execute_hook<ExecuteHook>(); }
}

const EnemySlowProfile& mini_freezard_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_FZ_e, eligible, prepare, before_move,
                                          nullptr, install, nullptr};
    return profile;
}
}
