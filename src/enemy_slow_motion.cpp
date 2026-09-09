#include "enemy_slow_motion.hpp"
#include "enemy_slow_motion/profile.hpp"
#include "enemy_slow_motion/timing.hpp"
#include "m_Do/m_Do_ext.h"

namespace dawnlight {
namespace {
DEFINE_HOOK(&fopAcM_posMoveF, MoveHook);
DEFINE_HOOK(&mDoExt_morf_c::frameUpdate, AnimationHook);
DEFINE_HOOK(&J3DFrameCtrl::checkPass, AnimationEventHook);

const std::array<const EnemySlowProfile*, 3> s_profiles{
    &darknut_slow_profile(), &bokoblin_slow_profile(), &mini_freezard_slow_profile()
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
};
std::array<ActorClock, 64> s_clocks{};

std::array<EnemySlowStep, 8> s_steps{};
std::size_t s_depth = 0;

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
    return false;
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
    return s_depth != 0 && s_depth <= s_steps.size() ? &s_steps[s_depth - 1] : nullptr;
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
        auto& clock = clock_for(actor);
        const bool timerTick = advance_enemy_timer(clock.fraction, step.scale);
        profile->prepare(step, timerTick);
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
    if (s_depth != 0 && --s_depth < s_steps.size()) s_steps[s_depth] = {};
}

ModResult initialize_enemy_slow_motion() {
    for (const auto* profile : s_profiles) {
        const ModResult result = profile->install();
        if (result != MOD_OK) return result;
    }
    ModResult result = mods::hook::add_pre<MoveHook>(svc_hook, before_move);
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
    s_animation = {};
    s_eventController = nullptr;
    for (const auto* profile : s_profiles) {
        if (profile->reset != nullptr) profile->reset();
    }
}
}  // namespace dawnlight
