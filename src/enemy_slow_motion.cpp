#include "enemy_slow_motion.hpp"
#include "enemy_slow_motion/profile.hpp"
#include "enemy_slow_motion/scope.hpp"
#include "enemy_slow_motion/timing.hpp"
#include "m_Do/m_Do_ext.h"
#include "f_pc/f_pc_method.h"
#include "d/d_bg_s_acch.h"
#include "SSystem/SComponent/c_lib.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&fopAcM_posMoveF, MoveHook);
DEFINE_HOOK(&mDoExt_morf_c::frameUpdate, AnimationHook);
DEFINE_HOOK(&J3DFrameCtrl::checkPass, AnimationEventHook);
DEFINE_HOOK(&fpcMtd_Method, ProcessMethodHook);
// MSVC cannot emit the SDK's constinit metadata for this virtual member pointer.
DEFINE_HOOK_SYMBOL("dBgS_Acch::CrrPos", void(dBgS_Acch*, dBgS&), CollisionHook);
DEFINE_HOOK(&cLib_addCalc2, ChaseTargetHook);
DEFINE_HOOK(&cLib_addCalc0, ChaseZeroHook);
DEFINE_HOOK(&cLib_chaseF, ChaseLinearHook);
DEFINE_HOOK(&cLib_addCalcAngleS2, ChaseAngleHook);
DEFINE_HOOK(&cLib_addCalcAngleS, ChaseAngleMinHook);
DEFINE_HOOK(&cLib_chaseS, ChaseShortHook);
DEFINE_HOOK(&J3DFrameCtrl::update, ControllerUpdateHook);

const std::array<const EnemySlowProfile*, 20> s_profiles{
    &darknut_slow_profile(), &bokoblin_slow_profile(), &mini_freezard_slow_profile(),
    &keese_slow_profile(), &tektite_slow_profile(), &gibdo_slow_profile(),
    &goron_slow_profile(), &staltroop_slow_profile(), &aeralfos_slow_profile(), &chilfos_slow_profile(),
    &freezard_slow_profile(), &stalchild_slow_profile(), &bubble_slow_profile(),
    &rat_slow_profile(), &white_wolfos_slow_profile(), &puppet_slow_profile(),
    &bomskit_slow_profile(), &stalhound_slow_profile(), &fire_toadpoli_slow_profile(),
    &bulblin_slow_profile()
};

const EnemySlowProfile* find_profile(fopAc_ac_c* actor) {
    if (actor == nullptr) return nullptr;
    const auto name = fopAcM_GetName(actor);
    for (const auto* profile : s_profiles) {
        if (profile->name == name) return profile;
    }
    return nullptr;
}

struct ActorClock {
    fopAc_ac_c* actor = nullptr;
    fpc_ProcID id = fpcM_ERROR_PROCESS_ID_e;
    float fraction = 0.0f;
    J3DAnmTransform* animation = nullptr;
    int eventFrame = -1;
};
std::array<ActorClock, 64> s_clocks{};

std::array<EnemySlowStep, 8> s_steps{};
std::size_t s_depth = 0;
std::array<bool, 64> s_processFrames{};
std::size_t s_processDepth = 0;

ActorClock& clock_for(fopAc_ac_c* actor) {
    for (auto& clock : s_clocks) {
        if (clock.actor == actor && clock.id == fopAcM_GetID(actor)) {
            return clock;
        }
    }
    for (auto& clock : s_clocks) {
        if (clock.actor == nullptr || fopAcM_SearchByID(clock.id) != clock.actor) {
            clock = {actor, fopAcM_GetID(actor), 0.0f};
            return clock;
        }
    }
    // This affects timer phase only; it never borrows another actor's state.
    auto& clock = s_clocks[fopAcM_GetID(actor) % s_clocks.size()];
    clock = {actor, fopAcM_GetID(actor), 0.0f};
    return clock;
}


HookAction before_move(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (step == nullptr || step->actor == nullptr ||
        mods::arg<fopAc_ac_c*>(args, 0) != step->actor) {
        return HOOK_CONTINUE;
    }
    auto* actor = step->actor;
    if (step->profile->beforeMove != nullptr) step->profile->beforeMove(*step);
    step->position = actor->current.pos;
    step->gravity = actor->gravity;
    step->moving = true;
    actor->gravity *= step->scale;
    return HOOK_CONTINUE;
}

HookAction before_collision(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (step == nullptr || step->actor == nullptr || step->directCollision == nullptr ||
        mods::arg<dBgS_Acch*>(args, 0) != step->directCollision) return HOOK_CONTINUE;
    if (step->profile->beforeCollision != nullptr) step->profile->beforeCollision(*step);
    if (step->directCollision == nullptr) return HOOK_CONTINUE;
    auto* actor = step->actor;
    // Some actors temporarily shift BOTH current and old positions for their
    // collision origin. Preserve this offset; scale only the integrated motion.
    const cXyz offset = actor->old.pos - step->originalOldPosition;
    actor->current.pos.x = slow_enemy_position_axis(step->originalPosition.x, actor->current.pos.x, offset.x, step->scale);
    actor->current.pos.y = slow_enemy_position_axis(step->originalPosition.y, actor->current.pos.y, offset.y, step->scale);
    actor->current.pos.z = slow_enemy_position_axis(step->originalPosition.z, actor->current.pos.z, offset.z, step->scale);
    step->directCollision = nullptr;
    return HOOK_CONTINUE;
}

bool owns_chase_float(const EnemySlowStep* step, float* value) {
    if (step == nullptr || step->actor == nullptr) return false;
    for (auto* owned : step->chaseFloats) if (owned == value) return true;
    return false;
}

HookAction before_chase_target(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (owns_chase_float(step, mods::arg<float*>(args, 0))) {
        mods::arg_ref<float>(args, 2) *= step->scale;
        mods::arg_ref<float>(args, 3) *= step->scale;
    }
    return HOOK_CONTINUE;
}

HookAction before_chase_zero(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (owns_chase_float(step, mods::arg<float*>(args, 0))) {
        mods::arg_ref<float>(args, 1) *= step->scale;
        mods::arg_ref<float>(args, 2) *= step->scale;
    }
    return HOOK_CONTINUE;
}

HookAction before_chase_linear(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (owns_chase_float(step, mods::arg<float*>(args, 0))) {
        mods::arg_ref<float>(args, 2) *= step->scale;
    }
    return HOOK_CONTINUE;
}

HookAction before_chase_angle(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (step == nullptr || step->actor == nullptr) return HOOK_CONTINUE;
    for (auto* owned : step->chaseAngles) {
        if (owned == mods::arg<s16*>(args, 0)) {
            auto& divisor = mods::arg_ref<s16>(args, 2);
            auto& maximum = mods::arg_ref<s16>(args, 3);
            if (divisor > 0) divisor = static_cast<s16>(std::min(32767.0f, std::ceil(divisor / step->scale)));
            if (maximum > 0) maximum = static_cast<s16>(std::max(1L, std::lround(maximum * step->scale)));
            break;
        }
    }
    return HOOK_CONTINUE;
}

bool owns_chase_angle(const EnemySlowStep* step, s16* value) {
    if (step == nullptr || step->actor == nullptr) return false;
    for (auto* owned : step->chaseAngles) if (owned == value) return true;
    return false;
}

HookAction before_chase_angle_min(ModContext* ctx, void* args, void* ret, void* data) {
    auto* step = current_enemy_slow_step();
    if (owns_chase_angle(step, mods::arg<s16*>(args, 0))) {
        auto& minimum = mods::arg_ref<s16>(args, 4);
        if (minimum > 0) minimum = static_cast<s16>(std::max(1L, std::lround(minimum * step->scale)));
        return before_chase_angle(ctx, args, ret, data);
    }
    return HOOK_CONTINUE;
}

HookAction before_chase_short(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (owns_chase_angle(step, mods::arg<s16*>(args, 0))) {
        auto& maximum = mods::arg_ref<s16>(args, 2);
        if (maximum > 0) maximum = static_cast<s16>(std::max(1L, std::lround(maximum * step->scale)));
    }
    return HOOK_CONTINUE;
}

HookAction before_process_method(ModContext* ctx, void* args, void* retval, void*) {
    bool entered = false;
    void* process = mods::arg<void*>(args, 1);
    if (s_processDepth < s_processFrames.size() && process != nullptr && fopAcM_IsActor(process)) {
        auto* actor = static_cast<fopAc_ac_c*>(process);
        const auto* profile = find_profile(actor);
        const auto* methods = reinterpret_cast<const process_method_class*>(actor->sub_method);
        if (profile != nullptr && profile->processExecute && methods != nullptr &&
            mods::arg<process_method_func>(args, 0) == methods->execute_method) {
            void* actorArgument = actor;
            void* executeArgs[]{&actorArgument};
            before_enemy_slow_execute(ctx, executeArgs, retval, nullptr);
            entered = true;
        }
    }
    if (s_processDepth < s_processFrames.size()) s_processFrames[s_processDepth] = entered;
    ++s_processDepth;
    return HOOK_CONTINUE;
}

void after_process_method(ModContext* ctx, void*, void*, void*) {
    if (s_processDepth != 0 && --s_processDepth < s_processFrames.size()) {
        if (s_processFrames[s_processDepth]) after_enemy_slow_execute(ctx, nullptr, nullptr, nullptr);
        s_processFrames[s_processDepth] = false;
    }
}

void after_move(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (step == nullptr || !step->moving || mods::arg<fopAc_ac_c*>(args, 0) != step->actor) {
        return;
    }
    auto* actor = step->actor;
    actor->current.pos = step->position + (actor->current.pos - step->position) * step->scale;
    actor->gravity = step->gravity;
    step->moving = false;
    // Vanilla performs mAcch.CrrPos next, against the slowed position.
}


bool owns_animation(const EnemySlowStep* step, const mDoExt_morf_c* animation) {
    if (step == nullptr || step->actor == nullptr || animation == nullptr) return false;
    for (auto* owned : step->animations) if (owned == animation) return true;
    return false;
}

bool owns_controller(const EnemySlowStep* step, const J3DFrameCtrl* controller) {
    for (auto* owned : step->animations) {
        if (owned != nullptr && &owned->mFrameCtrl == controller) return true;
    }
    for (auto* owned : step->controllers) if (owned == controller) return true;
    return false;
}

J3DFrameCtrl* s_updatedController = nullptr;
float s_controllerScale = 1.0f;
HookAction before_controller_update(ModContext*, void* args, void*, void*) {
    auto* step = current_enemy_slow_step();
    if (step == nullptr || step->actor == nullptr) return HOOK_CONTINUE;
    auto* controller = mods::arg<J3DFrameCtrl*>(args, 0);
    // Morph controllers already pass through frameUpdate; these are only the
    // separately registered expression/material controllers.
    for (auto* owned : step->controllers) {
        if (owned != nullptr && owned == controller) {
            s_updatedController = controller;
            s_controllerScale = step->scale;
            controller->setRate(controller->getRate() * step->scale);
            break;
        }
    }
    return HOOK_CONTINUE;
}

void after_controller_update(ModContext*, void* args, void*, void*) {
    auto* controller = mods::arg<J3DFrameCtrl*>(args, 0);
    if (s_updatedController == controller) {
        controller->setRate(controller->getRate() / s_controllerScale);
        s_updatedController = nullptr;
    }
}

struct AnimationStep {
    mDoExt_morf_c* animation = nullptr;
    float rate = 0.0f;
    float morph = 0.0f;
    float morphStep = 0.0f;
    float scale = 1.0f;
};
AnimationStep s_animation{};

HookAction before_animation(ModContext*, void* args, void*, void*) {
    auto* animation = mods::arg<mDoExt_morf_c*>(args, 0);
    auto* step = current_enemy_slow_step();
    if (owns_animation(step, animation)) {
        s_animation = {animation, animation->getPlaySpeed(), animation->mCurMorf,
                       animation->mMorfStep, step->scale};
        animation->setPlaySpeed(s_animation.rate * step->scale);
    }
    return HOOK_CONTINUE;
}

void after_animation(ModContext*, void* args, void*, void*) {
    auto* animation = mods::arg<mDoExt_morf_c*>(args, 0);
    if (s_animation.animation != animation) {
        return;
    }
    if (animation->getPlaySpeed() != 0.0f) {
        // Preserve a direction reversal in ping-pong animations.
        animation->setPlaySpeed(animation->getPlaySpeed() / s_animation.scale);
    }
    animation->mCurMorf = s_animation.morph +
        (animation->mCurMorf - s_animation.morph) * s_animation.scale;
    animation->mMorfStep = s_animation.morphStep +
        (animation->mMorfStep - s_animation.morphStep) * s_animation.scale;
    s_animation = {};
}

J3DFrameCtrl* s_eventController = nullptr;
float s_eventRate = 0.0f;
HookAction before_animation_event(ModContext*, void* args, void*, void*) {
    auto* controller = mods::arg<J3DFrameCtrl*>(args, 0);
    auto* step = current_enemy_slow_step();
    if (step != nullptr && step->actor != nullptr &&
        owns_controller(step, controller)) {
        // checkPass looks forward by rate, not backward at the last update.
        // Without this, a single weapon event repeats on every slow frame.
        s_eventController = controller;
        s_eventRate = controller->getRate();
        controller->setRate(s_eventRate * step->scale);
    }
    return HOOK_CONTINUE;
}

void after_animation_event(ModContext*, void* args, void*, void*) {
    auto* controller = mods::arg<J3DFrameCtrl*>(args, 0);
    if (s_eventController == controller) {
        controller->setRate(s_eventRate);
        s_eventController = nullptr;
    }
}

}  // namespace

EnemySlowStep* current_enemy_slow_step() {
    return active_enemy_scope(s_steps, s_depth);
}

bool enemy_uses_continuous_slow(fopAc_ac_c* actor) {
    const auto* profile = find_profile(actor);
    return profile != nullptr && enemy_slow_motion_scale(actor) < 0.999f &&
           profile->eligible(actor);
}

HookAction before_enemy_slow_execute(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<fopAc_ac_c*>(args, 0);
    const auto* profile = find_profile(actor);
    EnemySlowStep step{};
    if (enemy_uses_continuous_slow(actor) && s_depth < s_steps.size()) {
        step.actor = actor;
        step.profile = profile;
        step.scale = enemy_slow_motion_scale(actor);
        step.facing = actor->shape_angle.y;
        step.speed = actor->speedF;
        step.originalPosition = actor->current.pos;
        step.originalOldPosition = actor->old.pos;
        step.originalSpeed = actor->speed;
        step.originalAngles = actor->current.angle;
        step.originalShapeAngles = actor->shape_angle;
        auto& clock = clock_for(actor);
        const bool timerTick = advance_enemy_timer(clock.fraction, step.scale);
        step.timerTick = timerTick;
        step.timerFraction = clock.fraction;
        profile->prepare(step, timerTick);
        if (auto* animation = step.animations[0]; animation != nullptr) {
            const int frame = static_cast<int>(animation->getFrame());
            step.freshAnimationFrame = clock.animation != animation->getAnm() || clock.eventFrame != frame;
            clock.animation = animation->getAnm();
            clock.eventFrame = frame;
        }
    } else {
        for (auto& clock : s_clocks) {
            if (clock.actor == actor) clock = {};
        }
    }
    if (profile != nullptr && profile->observeExecute != nullptr) {
        profile->observeExecute(actor, step.scale);
    }
    if (s_depth < s_steps.size()) s_steps[s_depth] = step;
    ++s_depth;
    return HOOK_CONTINUE;
}

void after_enemy_slow_execute(ModContext*, void*, void*, void*) {
    if (auto* step = current_enemy_slow_step(); step != nullptr && step->actor != nullptr &&
        step->profile->afterExecute != nullptr) step->profile->afterExecute(*step);
    if (s_depth != 0 && --s_depth < s_steps.size()) s_steps[s_depth] = {};
}

ModResult initialize_enemy_slow_motion() {
    for (const auto* profile : s_profiles) {
        if (profile->install == nullptr) continue;
        const ModResult result = profile->install();
        if (result != MOD_OK) return result;
    }
    ModResult result = mods::hook::add_pre<MoveHook>(svc_hook, before_move);
    if (result == MOD_OK) result = mods::hook::add_pre<ProcessMethodHook>(svc_hook, before_process_method);
    if (result == MOD_OK) result = mods::hook::add_post<ProcessMethodHook>(svc_hook, after_process_method);
    if (result == MOD_OK) result = mods::hook::add_pre<CollisionHook>(svc_hook, before_collision);
    if (result == MOD_OK) result = mods::hook::add_pre<ChaseTargetHook>(svc_hook, before_chase_target);
    if (result == MOD_OK) result = mods::hook::add_pre<ChaseZeroHook>(svc_hook, before_chase_zero);
    if (result == MOD_OK) result = mods::hook::add_pre<ChaseLinearHook>(svc_hook, before_chase_linear);
    if (result == MOD_OK) result = mods::hook::add_pre<ChaseAngleHook>(svc_hook, before_chase_angle);
    if (result == MOD_OK) result = mods::hook::add_pre<ChaseAngleMinHook>(svc_hook, before_chase_angle_min);
    if (result == MOD_OK) result = mods::hook::add_pre<ChaseShortHook>(svc_hook, before_chase_short);
    if (result == MOD_OK) result = mods::hook::add_pre<ControllerUpdateHook>(svc_hook, before_controller_update);
    if (result == MOD_OK) result = mods::hook::add_post<ControllerUpdateHook>(svc_hook, after_controller_update);
    if (result == MOD_OK) result = mods::hook::add_post<MoveHook>(svc_hook, after_move);
    if (result == MOD_OK) result = mods::hook::add_pre<AnimationHook>(svc_hook, before_animation);
    if (result == MOD_OK) result = mods::hook::add_post<AnimationHook>(svc_hook, after_animation);
    if (result == MOD_OK) result = mods::hook::add_pre<AnimationEventHook>(svc_hook, before_animation_event);
    if (result == MOD_OK) result = mods::hook::add_post<AnimationEventHook>(svc_hook, after_animation_event);
    return result;
}

void reset_enemy_slow_motion() {
    s_clocks = {};
    s_steps = {};
    s_depth = 0;
    s_processFrames = {};
    s_processDepth = 0;
    s_animation = {};
    s_eventController = nullptr;
    s_updatedController = nullptr;
    for (const auto* profile : s_profiles) {
        if (profile->reset != nullptr) profile->reset();
    }
}
}  // namespace dawnlight
