#include "boss.hpp"
#include "d/actor/d_a_e_hzelda.h"
#include "d/actor/d_a_alink.h"
#include "Z2AudioLib/Z2AudioMgr.h"
#include "SSystem/SComponent/c_lib.h"

namespace dawnlight {
namespace {
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_hzelda.cpp#action", void(e_hzelda_class*), ActionHook);
e_hzelda_class& boss(fopAc_ac_c* actor) {
    return *static_cast<e_hzelda_class*>(actor);
}
bool eligible(fopAc_ac_c* actor) {
    const auto& a = boss(actor);
    return a.mpModelMorf && a.mDemoMode == 0 && !dComIfGp_event_runCheck();
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = boss(step.actor);
    step.animations = {a.mpModelMorf};
    own_boss_controller(step, a.mpTriangleAtBrk);
    own_boss_controller(step, a.mpTriangleAtBtk);
    step.directCollision = &a.mAcch;
    step.chaseFloats = {&a.speedF, &a.field_0x6c4, &a.field_0x6cc, &a.field_0x6e8, &a.mArmLRotY, &a.mArmRRotY, &a.mBodyRotY, &a.mBodyRotZ, &a.mDodgeMove, &a.mMoveStep, &a.mSwordColorIntensity};
    step.chaseAngles = {&a.current.angle.x, &a.current.angle.y, &a.mHeadRotX, &a.mHeadRotZ, &a.shape_angle.x, &a.shape_angle.y, &a.shape_angle.z};
    hold_boss_timer_array(tick, a.mTimers);
    hold_boss_timers(tick, a.field_0x6b4);
}
void after_action(ModContext*, void* args, void*, void*) {
    auto* a = mods::arg<e_hzelda_class*>(args, 0);
    auto* step = boss_step(a, fpcNm_E_HZELDA_e);
    if (!step || !eligible(a)) return;
    // Ball movement occurs later in execute. Restore the velocity only if the
    // native sword/bottle reflection did not replace it with a new impulse.
    step->points[0] = a->mBallMove;
    step->points[1] = a->mBallMove * step->scale;
    a->mBallMove = step->points[1];
    step->values[0] = 1;
    // The native triangle block increments an integer and tests equality at
    // 2/100. Run that block below on the actor clock, with smooth material frames.
    step->values[1] = a->mDrawTriangleAt;
    a->mDrawTriangleAt = 0;
}
void after_execute(EnemySlowStep& step) {
    auto& a = boss(step.actor);
    if (!step.values[0]) return;
    if (a.mBallMove == step.points[1]) a.mBallMove = step.points[0];
    a.mDrawTriangleAt = static_cast<s16>(step.values[1]);
    if (!a.mDrawTriangleAt) return;

    const int previous = a.mTriangleAnmFrame;
    a.mTriangleAnmFrame = advance_boss_triangle(previous, step.timerTick);
    const float frame = std::min(210.0f, a.mTriangleAnmFrame +
        (a.mTriangleAnmFrame < 100 ? 2.0f : 1.0f) * step.timerFraction);
    mDoMtx_stack_c::transS(a.mTrianglePos);
    mDoMtx_stack_c::YrotM(a.mTriangleRotY + 0x8000);
    mDoMtx_stack_c::scaleM(a.mTriangleSize, a.mTriangleSize, a.mTriangleSize);
    a.mpTriangleAtModel->setBaseTRMtx(mDoMtx_stack_c::get());
    a.mpTriangleAtBrk->setFrame(frame);
    a.mpTriangleAtBtk->setFrame(frame);
    if (previous < 2 && a.mTriangleAnmFrame >= 2)
        Z2GetAudioMgr()->seStart(Z2SE_EN_HZE_ATK_B_LIGHT, &a.mTrianglePos,
            0, 0, 1, 1, -1, -1, 0);
    if (previous < 100 && a.mTriangleAnmFrame >= 100) {
        Z2GetAudioMgr()->seStart(Z2SE_EN_HZE_ATK_B_LIGHTWALL, &a.mTrianglePos,
            0, 0, 1, 1, -1, -1, 0);
        a.mSound.startCreatureVoice(Z2SE_EN_HZE_V_ATK_B_LIGHTWALL, -1);
        csXyz rotation(0, a.mTriangleRotY + 0x8000, 0);
        cXyz size(a.mTriangleSize, a.mTriangleSize, a.mTriangleSize);
        for (u16 id : {0x8945, 0x8946, 0x8947, 0x8948, 0x8949})
            dComIfGp_particle_set(id, &a.mTrianglePos, &rotation, &size);
    }
    if (frame > 105 && frame < 135) {
        auto* player = daPy_getPlayerActorClass();
        cXyz offset = player->current.pos - a.mTrianglePos;
        offset.y = 0;
        cXyz rotated;
        cMtx_YrotS(*calc_mtx, -a.mTriangleRotY);
        MtxPosition(&offset, &rotated);
        float angle = std::abs(57.295f * cM_atan2f(rotated.x, rotated.z));
        if (angle >= 60 && angle <= 120) angle = 120 - angle;
        else if (angle >= 120 && angle <= 180) angle -= 120;
        const float limit = 50 * a.mTriangleSize / cM_fcos(M_PI * (angle / 180));
        if (std::sqrt(rotated.x * rotated.x + rotated.z * rotated.z) < limit) {
            a.mTriAtSph.SetC(player->current.pos);
            a.mTriAtSph.SetR(5);
            dComIfG_Ccsp()->Set(&a.mTriAtSph);
            if (step.timerTick) a.mSound.startCreatureVoice(Z2SE_EN_HZE_V_LAUGH, -1);
        }
    }
}
void before_collision(EnemySlowStep& step) {
    if (!eligible(step.actor)) step.directCollision = nullptr;
}
ModResult install() { return mods::hook::add_post<ActionHook>(svc_hook, after_action); }
}
const EnemySlowProfile& puppet_zelda_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_HZELDA_e, eligible, prepare,
        nullptr, nullptr, install, nullptr, before_collision, after_execute, true};
    return profile;
}
}
