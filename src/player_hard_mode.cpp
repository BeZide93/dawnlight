#include "player_hard_mode.hpp"

#include "config.hpp"
#include "service_imports.hpp"

#include "d/actor/d_a_alink.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

namespace dawnlight {
namespace {

DEFINE_HOOK(&daAlink_c::checkDamageAction, PlayerHardModeDamageActionHook);

struct DamageActionSnapshot {
    daAlink_c* link = nullptr;
    s16 timer = 0;
};

thread_local DamageActionSnapshot s_damageActionSnapshot{};
thread_local daAlink_c* s_knockdownTimerOwner = nullptr;

bool is_knockdown_process(u16 process) {
    switch (process) {
    case daAlink_c::PROC_LARGE_DAMAGE_UP:
    case daAlink_c::PROC_LAND_DAMAGE:
    case daAlink_c::PROC_WOLF_LARGE_DAMAGE_UP:
    case daAlink_c::PROC_WOLF_LAND_DAMAGE:
    case daAlink_c::PROC_LARGE_DAMAGE:
    case daAlink_c::PROC_LARGE_DAMAGE_WALL:
        return true;
    default:
        return false;
    }
}

HookAction before_damage_action(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    s_damageActionSnapshot = {
        link, static_cast<s16>(link != nullptr ? link->mDamageTimer : 0)};
    if (link != nullptr && link->mDamageTimer == 0 && s_knockdownTimerOwner == link) {
        s_knockdownTimerOwner = nullptr;
    }
    return HOOK_CONTINUE;
}

void after_damage_action(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const auto snapshot = s_damageActionSnapshot;
    s_damageActionSnapshot = {};

    if (!remove_normal_hit_invulnerability_enabled() || link == nullptr ||
        snapshot.link != link || link->mDamageTimer <= snapshot.timer)
    {
        return;
    }

    if (is_knockdown_process(link->mProcID)) {
        s_knockdownTimerOwner = link;
        return;
    }

    if (s_knockdownTimerOwner != link) {
        link->mDamageTimer = 0;
    }
}

}  // namespace

ModResult install_player_hard_mode_hooks(ModError* error) {
    ModResult result = mods::hook::add_pre<PlayerHardModeDamageActionHook>(
        svc_hook, before_damage_action);
    if (result == MOD_OK) {
        result = mods::hook::add_post<PlayerHardModeDamageActionHook>(
            svc_hook, after_damage_action);
    }
    if (result != MOD_OK) {
        return mods::set_error(
            error, result, "failed to install Dawnlight player Hard Mode hooks");
    }
    return MOD_OK;
}

}  // namespace dawnlight
