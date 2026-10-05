#pragma once
#include "service_imports.hpp"
#include "f_op/f_op_actor_mng.h"
#include "mods/svc/hook.hpp"
namespace dawnlight {
struct EnemyHardModeStep {
    fopAc_ac_c* actor = nullptr;
    int profileName = -1;
    int action = 0;
    int subaction = 0;
    bool chaseSpeed = false;
    bool chaseCurrentYaw = false;
    bool chaseShapeYaw = false;
};
EnemyHardModeStep* current_enemy_hard_mode_step();
HookAction before_enemy_hard_mode_execute(ModContext*, void*, void*, void*);
void after_enemy_hard_mode_execute(ModContext*, void*, void*, void*);
template <typename Hook>
ModResult install_enemy_hard_mode_execute_hook() {
    auto result = mods::hook::add_pre<Hook>(svc_hook, before_enemy_hard_mode_execute);
    if (result == MOD_OK) result = mods::hook::add_post<Hook>(svc_hook, after_enemy_hard_mode_execute);
    return result;
}
ModResult initialize_enemy_hard_mode_runtime();
void reset_enemy_hard_mode_runtime();
}
