#include "profile.hpp"
#include "timing.hpp"
#include "d/actor/d_a_npc_gra.h"
#include "d/d_com_inf_game.h"
#include "m_Do/m_Do_ext.h"

namespace dawnlight {
namespace {
// Expose typed member pointers, without casting an actor to a fabricated subclass.
struct GoronFields : daNpc_grA_c {
    using daNpcF_c::mAnm_p;
    using daNpcF_c::mBckAnm;
    using daNpcF_c::mBtpAnm;
    using daNpcF_c::mBtkAnm;
    using daNpcF_c::mBrkAnm;
    using daNpcF_c::mCurAngle;
    using daNpcF_c::mDamageTimer;
    using daNpcF_c::mExpressionMorf;
    using daNpcF_c::mTurnAmount;
    using daNpcF_c::mTurnStepNum;
    using daNpcF_c::mTurnStartAngle;
    using daNpcF_c::field_0x984;
    using daNpcF_c::field_0x992;
};

DEFINE_HOOK(&daNpc_grA_c::Execute, ExecuteHook);
DEFINE_HOOK(&daNpcF_c::turn, TurnHook);
DEFINE_HOOK(&daNpc_grA_c::setOtherObjMtx, CarriedModelHook);
DEFINE_HOOK(&cLib_chaseAngleS, AngleHook);

bool eligible(fopAc_ac_c* actor) {
    auto& goron = *static_cast<daNpc_grA_c*>(actor);
    return goron.*&GoronFields::mAnm_p != nullptr && !dComIfGp_event_runCheck() &&
           goron.mAction != &daNpc_grA_c::talk;
}

EnemySlowStep* goron_step() {
    auto* step = current_enemy_slow_step();
    return step != nullptr && step->actor != nullptr && step->profile->name == fpcNm_NPC_GRA_e
        ? step : nullptr;
}

void prepare(EnemySlowStep& step, bool) {
    auto& actor = *static_cast<daNpc_grA_c*>(step.actor);
    step.animations[0] = actor.*&GoronFields::mAnm_p;
    step.controllers = {(actor.*&GoronFields::mBckAnm).getFrameCtrl(),
                        (actor.*&GoronFields::mBtpAnm).getFrameCtrl(),
                        (actor.*&GoronFields::mBtkAnm).getFrameCtrl(),
                        (actor.*&GoronFields::mBrkAnm).getFrameCtrl()};
    auto& angles = actor.*&GoronFields::mCurAngle;
    auto& reaction = actor.*&GoronFields::field_0x984;
    step.chaseFloats = {&actor.speedF, &(actor.*&GoronFields::mExpressionMorf), &reaction[0], &reaction[2]};
    step.chaseAngles = {&actor.current.angle.y, &angles.y, &(actor.*&GoronFields::field_0x992)};
    step.values[0] = static_cast<float>(actor.*&GoronFields::mDamageTimer);
}

void after_execute(EnemySlowStep& step) {
    auto& actor = *static_cast<daNpc_grA_c*>(step.actor);
    auto& timer = actor.*&GoronFields::mDamageTimer;
    // NPC damage timeout is decremented AFTER main; don't change what main sees.
    if (!step.timerTick && step.values[0] > 0 && timer == static_cast<int>(step.values[0]) - 1) ++timer;
}

void before_move(EnemySlowStep& step) {
    auto& actor = *step.actor;
    // Gate attendants also move directly in main(), before standard movement.
    // setTagJump/setHomeJump relocate old.pos too and must remain instantaneous.
    if (actor.old.pos.x == step.originalOldPosition.x &&
        actor.old.pos.y == step.originalOldPosition.y && actor.old.pos.z == step.originalOldPosition.z) {
        actor.current.pos = step.originalPosition + (actor.current.pos - step.originalPosition) * step.scale;
    }
}

HookAction before_angle(ModContext*, void* args, void*, void*) {
    auto* step = goron_step();
    if (step != nullptr && mods::arg<s16*>(args, 0) == &step->actor->current.angle.y) {
        auto& rate = mods::arg_ref<s16>(args, 2);
        rate = static_cast<s16>(std::max(1L, std::lround(rate * step->scale)));
    }
    return HOOK_CONTINUE;
}

HookAction before_turn(ModContext*, void* args, void* retval, void*) {
    auto* step = goron_step();
    if (step == nullptr || mods::arg<daNpcF_c*>(args, 0) != step->actor) return HOOK_CONTINUE;
    auto& actor = *static_cast<daNpc_grA_c*>(step->actor);
    const auto target = mods::arg<s16>(args, 1);
    const auto rate = mods::arg<float>(args, 2);
    auto direction = mods::arg<int>(args, 3);
    auto& amount = actor.*&GoronFields::mTurnAmount;
    auto& steps = actor.*&GoronFields::mTurnStepNum;
    auto& start = actor.*&GoronFields::mTurnStartAngle;
    if (amount == 0) {
        steps = std::max(8.0f, static_cast<float>(static_cast<int>(
            std::fabs(cM_sht2d(static_cast<s16>(actor.current.angle.y - target))) / 180.0f * rate) + 1));
        start = actor.current.angle.y;
    }
    if (direction == 0) direction = static_cast<s16>(target - start) >= 0 ? 1 : -1;
    int angle = static_cast<u16>(target - start);
    if (direction < 0) angle = -static_cast<u16>(0xffff - angle);
    const auto offset = static_cast<int>(angle * cM_ssin(amount));
    amount += std::max(1L, std::lround(16384.0f / steps * step->scale));
    if (amount >= 0x4000) {
        actor.current.angle.y = target;
        amount = 0x4000;
    } else {
        actor.current.angle.y = static_cast<s16>(start + offset);
    }
    *static_cast<BOOL*>(retval) = actor.current.angle.y == target;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_carried_model(ModContext*, void* args, void*, void*) {
    auto* step = goron_step();
    auto* actor = mods::arg<daNpc_grA_c*>(args, 0);
    if (step == nullptr || actor != step->actor || actor->field_0x150C == 0 || actor->mType != 7) return HOOK_CONTINUE;
    // Native uses an integer-indexed height curve with period 31, not 32.
    constexpr float heights[]{182, 182, 186, 190, 198, 206, 206, 206, 202, 202, 198,
        194, 190, 186, 182, 182, 186, 186, 190, 198, 202, 206, 209, 206, 198,
        186, 202, 206, 202, 190, 182};
    const float frame = std::max(0.0f, (actor->*&GoronFields::mAnm_p)->getFrame());
    cXyz position = actor->current.pos;
    position.y += sample_looped_enemy_motion(heights, frame);
    mDoMtx_stack_c::transS(position);
    mDoMtx_stack_c::ZXYrotM(actor->*&GoronFields::mCurAngle);
    mDoMtx_stack_c::scaleM(actor->scale);
    mDoMtx_copy(mDoMtx_stack_c::get(), actor->field_0x14DC);
    return HOOK_SKIP_ORIGINAL;
}

ModResult install() {
    auto result = install_enemy_execute_hook<ExecuteHook>();
    if (result == MOD_OK) result = mods::hook::add_pre<TurnHook>(svc_hook, before_turn);
    if (result == MOD_OK) result = mods::hook::add_pre<AngleHook>(svc_hook, before_angle);
    if (result == MOD_OK) result = mods::hook::add_pre<CarriedModelHook>(svc_hook, before_carried_model);
    return result;
}
}

const EnemySlowProfile& goron_slow_profile() {
    static const EnemySlowProfile profile{
        fpcNm_NPC_GRA_e, eligible, prepare, before_move, nullptr, install, nullptr, nullptr, after_execute
    };
    return profile;
}
}
