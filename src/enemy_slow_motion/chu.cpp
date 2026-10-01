#include "integration.hpp"
#include "d/actor/d_a_e_sm2.h"
#include "d/d_s_play.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&mDoExt_McaMorfSO::play, PlayHook);
e_sm2_class& chu(fopAc_ac_c* base) {
    static_assert(offsetof(e_sm2_class, enemy) == 0);
    return *reinterpret_cast<e_sm2_class*>(base);
}
bool eligible(fopAc_ac_c* base) {
    const auto& a = chu(base);
    // Ceiling placement and bottle-catch ownership retain the native fallback.
    return (a.isPiece ? a.pieceModelMorf : a.modelMorf) && a.action != ACTION_ROOF &&
        !(a.action == ACTION_FAIL && (a.mode == 2 || base->eventInfo.checkCommandCatch()));
}
void prepare(EnemySlowStep& step, bool tick) {
    auto& a = chu(step.actor);
    step.action = a.action;
    step.animations = {a.isPiece ? a.pieceModelMorf : a.modelMorf};
    step.directCollision = &a.acch;
    step.chaseFloats = {&step.actor->speedF, &a.size, &a.color_R, &a.color_G,
        &a.color_B, &a.color_alpha, &a.field_0x6ac, &a.field_0x6b0, &a.field_0x82c,
        &a.field_0x830, &a.field_0x838, &a.field_0x840.x, &a.field_0x840.y, &a.field_0x840.z};
    own_enemy_values(step.chaseFloats, a.field_0x6c8);
    step.chaseAngles = {&step.actor->current.angle.y, &step.actor->shape_angle.y,
        &step.actor->shape_angle.x, &a.mCurrentAngleYTargetStep, &a.field_0x84c.x, &a.field_0x84c.y};
    step.values[0] = a.counter;
    step.values[1] = a.combine_off_timer;
    step.values[2] = a.sizetype;
    step.values[3] = a.isPiece;
    step.values[4] = a.field_0x828;
    step.values[5] = a.field_0x83f;
    for (int i = 0; i < 8; ++i) {
        step.points[i] = a.field_0x708[i];
        step.points[i + 8] = a.jnt_pos[i];
    }
    if (!tick) {
        a.counter = static_cast<s16>(static_cast<u16>(a.counter) - 1);
        for (auto& timer : a.timers) hold_enemy_timer(timer);
        hold_enemy_timer(a.invulernabilityTimer);
        hold_enemy_timer(a.combine_off_timer);
        if (a.action != ACTION_WATER) hold_enemy_timer(a.field_0x6a9);
        hold_enemy_timer(a.field_0x6aa);
        hold_enemy_timer(a.field_0x83e);
        // counter is also the deformation phase. Hold it unchanged and gate
        // only the modulo-8 merge search, instead of feeding a fake odd phase.
        if ((static_cast<int>(step.values[0]) & 7) == 0 && step.values[1] == 0)
            a.combine_off_timer = 2; // native -- leaves 1, inhibiting this search
    }
}
void before_collision(EnemySlowStep& step) {
    auto& a = chu(step.actor);
    if (step.action != ACTION_WATER && a.action == ACTION_WATER) {
        // Preserve the exact water-surface snap rather than lerping from land.
        step.originalPosition.y = step.actor->current.pos.y - step.actor->speed.y;
    }
    slow_enemy_gravity(step, step.actor->gravity, -100.0f, true);
}
void relax_body(EnemySlowStep& step) {
    auto& a = chu(step.actor);
    if (a.isPiece || a.sizetype != step.values[2] || a.isPiece != step.values[3]) return;
    // Native dmcalc solves the body every execute. Interpolate the live chain
    // before modelCalc AND collision registration, preserving split/merge resets.
    // Joint zero is the freshly computed head/root, never an old world anchor.
    for (int i = 1; i < 8; ++i) {
        a.field_0x708[i] = step.points[i] + (a.field_0x708[i] - step.points[i]) * step.scale;
        a.jnt_pos[i] = step.points[i + 8] + (a.jnt_pos[i] - step.points[i + 8]) * step.scale;
        const cXyz delta = a.field_0x708[i] - a.field_0x708[i - 1];
        const auto nativeAngle = a.field_0x768[i];
        a.field_0x768[i].y = cM_atan2s(delta.x, delta.z);
        a.field_0x768[i].x = -cM_atan2s(delta.y, std::sqrt(delta.x * delta.x + delta.z * delta.z));
        // Keep the native visual lag relative to the body direction. jnt_pos[0]
        // is never updated by native dmcalc, so it cannot anchor this rotation.
        a.field_0x7f8[i].y += static_cast<s16>(a.field_0x768[i].y - nativeAngle.y);
        a.field_0x7f8[i].x += static_cast<s16>(a.field_0x768[i].x - nativeAngle.x);
    }
}
HookAction before_play(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (!step || step->profile->name != fpcNm_E_SM2_e) return HOOK_CONTINUE;
    auto& a = chu(step->actor);
    auto* morph = a.isPiece ? a.pieceModelMorf : a.modelMorf;
    if (mods::arg<mDoExt_McaMorfSO*>(args, 0) != morph) return HOOK_CONTINUE;
    relax_body(*step);
    if (!step->timerTick && a.field_0x83f == step->values[5] - 1) ++a.field_0x83f;
    if (a.isPiece) {
        // Pieces have procedural rotation/squash instead of a walking BCK.
        const float phase = a.counter + step->timerFraction;
        const s16 turn = static_cast<s16>(static_cast<int>(phase * 400.0f));
        const float pulse = 1.0f + (0.007f + NREG_F(16)) * a.timers[1] *
            cM_ssin(static_cast<s16>(static_cast<int>(phase * (ZREG_S(1) + 8000))));
        const float width = (0.5f / a.field_0x6b0) * pulse;
        mDoMtx_stack_c::transS(step->actor->current.pos);
        mDoMtx_stack_c::YrotM(step->actor->shape_angle.y - turn);
        mDoMtx_stack_c::scaleM((1.08f + KREG_F(7)) * width, a.field_0x6b0, width);
        mDoMtx_stack_c::YrotM(turn);
        morph->getModel()->setBaseTRMtx(mDoMtx_stack_c::get());
    }
    return HOOK_CONTINUE;
}
void after_execute(EnemySlowStep& step) {
    auto& a = chu(step.actor);
    if (!step.timerTick && (static_cast<int>(step.values[0]) & 7) == 0 &&
        step.values[1] == 0 && a.combine_off_timer == 1) a.combine_off_timer = 0;
    const s16 phase = slow_enemy_angle(static_cast<s16>(step.values[4]), a.field_0x828, step.scale);
    const s16 difference = phase - a.field_0x828;
    a.field_0x828 = phase;
    if (a.action != ACTION_FAIL)
        for (auto& angle : a.field_0x768) angle.z += difference;
}
ModResult install() { return mods::hook::add_pre<PlayHook>(svc_hook, before_play); }
}
const EnemySlowProfile& chu_slow_profile() {
    static const EnemySlowProfile profile{fpcNm_E_SM2_e, eligible, prepare, nullptr,
        nullptr, install, nullptr, before_collision, after_execute, true};
    return profile;
}
}
