#include "config.hpp"
#include "save_compat.hpp"
#include "service_imports.hpp"

#include "global.h"
#include "SSystem/SComponent/c_phase.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_actor.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

#include <algorithm>
#include <cstdint>

namespace dawnlight {
namespace {

DEFINE_HOOK_SYMBOL("fopAc_Create", int(void*), ActorCreate);

s16 scale_health(s16 health, int percent) {
    if (health <= 0 || percent == 100) {
        return health;
    }
    const int scaled = (static_cast<int>(health) * percent + 99) / 100;
    return static_cast<s16>(std::min(scaled, 0x7fff));
}

void on_actor_create_post(ModContext*, void* args, void* retval, void*) {
    if (args == nullptr || retval == nullptr || *static_cast<int*>(retval) != cPhs_COMPLEATE_e) {
        return;
    }

    auto* actor = static_cast<fopAc_ac_c*>(mods::arg<void*>(args, 0));
    if (actor == nullptr || actor->group != fopAc_ENEMY_e) {
        return;
    }

    int percent = health_scale_percent();
    if (automatic_ngplus_health_scaling()) {
        const unsigned count = std::min(new_game_plus_count(dComIfGs_getSaveData()), 9u);
        if (count != 0) {
            percent = std::max(percent, 200 + static_cast<int>(count) * 10);
        }
    }
    actor->health = scale_health(actor->health, percent);
    actor->field_0x560 = scale_health(actor->field_0x560, percent);
}

}  // namespace

ModResult install_enemy_scaling_hooks(ModError* error) {
    const ModResult result = mods::hook_add_post<ActorCreate>(svc_hook, on_actor_create_post);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight enemy scaling hook");
    }
    return MOD_OK;
}

}  // namespace dawnlight
