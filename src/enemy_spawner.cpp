#include "enemy_spawner.hpp"

#include "service_imports.hpp"

#include "SSystem/SComponent/c_math.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_e_tk2.h"
#include "d/actor/d_a_e_zs.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_method.h"
#include "f_pc/f_pc_name.h"
#include "mods/hook.hpp"

#include <array>
#include <unordered_set>

namespace dawnlight {
namespace {

constexpr std::array<ProfileName, kEnemySpawnerProfileLabels.size()> kEnemySpawnerProfiles{
    fpcNm_B_TN_e,  // Darknut
    fpcNm_E_OC_e,  // Bokoblin
    fpcNm_E_FZ_e,  // Mini Freezard
    fpcNm_E_BA_e,  // Keese
    fpcNm_E_TT_e,  // Tektite
    fpcNm_E_GI_e,  // Gibdo
    fpcNm_NPC_GRA_e,  // Goron
    fpcNm_E_ZS_e,  // Staltroop
    fpcNm_B_GG_e,  // Aeralfos
    fpcNm_E_KK_e,  // Chilfos
    fpcNm_E_FB_e,  // Freezard
    fpcNm_E_BS_e,  // Stalchild
    fpcNm_E_BU_e,  // Bubble
    fpcNm_E_MS_e,  // Rat
    fpcNm_E_WW_e,  // White Wolfos
    fpcNm_E_FS_e,  // Puppet
    fpcNm_E_CR_e,  // Bomskit
    fpcNm_E_SH_e,  // Stalhound
    fpcNm_E_TK2_e,  // Fire Toadpoli
    fpcNm_E_RD_e,  // Bulblin
    fpcNm_E_DN_e,  // Lizalfos
    fpcNm_E_DD_e,  // Dodongo
    fpcNm_E_MF_e,  // Dynalfos
    fpcNm_E_ST_e,  // Skulltula
    fpcNm_E_HB_e,  // Baba Serpent
    fpcNm_E_GB_e,  // Big Baba
    fpcNm_E_DB_e,  // Deku Baba
    fpcNm_E_SF_e,  // Stalfos
};

// Darknut treats type 0 as its room-opening demo. Type 1 is the ordinary waitable
// variant; 0xff means that no defeated-state switch is attached to the test actor.
constexpr std::array<u32, kEnemySpawnerProfiles.size()> kEnemySpawnerParameters{
    0x000001ff,  // Darknut
    0x00ff0000,  // Bokoblin: no defeated switch
    0,           // Mini Freezard
    0xffff0f01,  // Keese: flying, default detection radius, no path/switch
    0x000000ff,  // Tektite: red, no defeated switch
    0x00ff01ff,  // Gibdo: awake, no activation/defeated switches
    0,           // Goron
    0,           // Staltroop
    0xffff0002,  // Aeralfos: regular flying enemy, no arena demo/switches
    0x00ff0000,  // Chilfos: normal body, no defeated switch
    0x00ffff00,  // Freezard: no switches
    0x00ffff00,  // Stalchild: normal, default detection radius, no switch
    0xffff0f00,  // Bubble: no path/switch, default detection radius
    0xffff0000,  // Rat: no activation/defeated switches
    0x03000000,  // White Wolfos: individual, not the invisible pack controller
    0,           // Puppet
    0,           // Bomskit
    0x00ffff00,  // Stalhound: default detection/leash radii (night only)
    0,           // Fire Toadpoli
    0xff000100,  // Bulblin: club, no defeated switch
    0xff000000,  // Lizalfos: no defeated switch
    0xffff0000,  // Dodongo: no path or defeated switch
    0xff000000,  // Dynalfos: no defeated switch
    0xff000002,  // Skulltula: ground variant, no defeated switch
    0xff000000,  // Baba Serpent: no defeated switch
    0x00ffff00,  // Big Baba: no defeated/intro switches
    0xff000000,  // Deku Baba: no defeated switch
    0x0000ff00,  // Stalfos: normal standing variant, default detection radius
};

DEFINE_HOOK(&fpcMtd_Method, SpawnerProcessHook);
DEFINE_HOOK(&daE_ZS_c::executeAppear, StaltroopAppearHook);
DEFINE_HOOK(&daE_ZS_c::executeWait, StaltroopWaitHook);

std::unordered_set<ActorId> s_testActors;
bool s_processHookInstalled = false;
bool s_appearHookInstalled = false;
bool s_waitHookInstalled = false;

bool is_test_actor(fopAc_ac_c* actor) {
    return actor != nullptr && s_testActors.contains(fopAcM_GetID(actor));
}

HookAction before_process(ModContext*, void* args, void*, void*) {
    auto* process = mods::arg<void*>(args, 1);
    if (process == nullptr || !fopAcM_IsActor(process)) return HOOK_CONTINUE;
    auto* actor = static_cast<fopAc_ac_c*>(process);
    if (!is_test_actor(actor)) return HOOK_CONTINUE;
    const auto* methods = reinterpret_cast<const process_method_class*>(actor->sub_method);
    if (methods == nullptr) return HOOK_CONTINUE;
    const auto method = mods::arg<process_method_func>(args, 0);
    if (method == methods->delete_method) {
        s_testActors.erase(fopAcM_GetID(actor));
    } else if (method == methods->execute_method) {
        if (fopAcM_GetName(actor) == fpcNm_E_TK2_e) {
            // Vanilla anchors to lava after this timer expires. Test actors use
            // the ground chosen at spawn, including in rooms without lava.
            static_cast<e_tk2_class*>(actor)->mActionTimer[3] = 2;
        } else if (fopAcM_GetName(actor) == fpcNm_E_ZS_e) {
            // Prevent the arena-owner check from retiring this standalone actor.
            static_cast<daE_ZS_c*>(actor)->field_0x671 = 2;
        }
    }
    return HOOK_CONTINUE;
}

HookAction before_staltroop_appear(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<daE_ZS_c*>(args, 0);
    if (is_test_actor(actor) && actor->mMode == 0) actor->mMode = 1;
    return HOOK_CONTINUE;
}

HookAction before_staltroop_wait(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<daE_ZS_c*>(args, 0);
    if (!is_test_actor(actor)) return HOOK_CONTINUE;
    if (actor->mMode == 0) {
        actor->setBck(7, 2, 3.0f, 1.0f);
        actor->mMode = 1;
    }
    // The original idle retires the actor when Stallord is absent. Damage,
    // collision, appearance and death still run through the original actor.
    cLib_addCalcAngleS2(&actor->current.angle.y, fopAcM_searchPlayerAngleY(actor), 8, 0x1000);
    actor->shape_angle.y = actor->current.angle.y;
    return HOOK_SKIP_ORIGINAL;
}

ModResult install_test_hooks() {
    if (!s_processHookInstalled) {
        const auto result = mods::hook::add_pre<SpawnerProcessHook>(svc_hook, before_process);
        if (result != MOD_OK) return result;
        s_processHookInstalled = true;
    }
    if (!s_appearHookInstalled) {
        const auto result = mods::hook::add_pre<StaltroopAppearHook>(svc_hook, before_staltroop_appear);
        if (result != MOD_OK) return result;
        s_appearHookInstalled = true;
    }
    if (!s_waitHookInstalled) {
        const auto result = mods::hook::add_pre<StaltroopWaitHook>(svc_hook, before_staltroop_wait);
        if (result != MOD_OK) return result;
        s_waitHookInstalled = true;
    }
    return MOD_OK;
}

}  // namespace

ModResult spawn_enemy_for_testing(int profileIndex) {
    if (profileIndex < 0 ||
        static_cast<std::size_t>(profileIndex) >= kEnemySpawnerProfiles.size()) {
        return MOD_INVALID_ARGUMENT;
    }
    if (svc_actor == nullptr || svc_actor->create_actor == nullptr) return MOD_UNAVAILABLE;

    auto* link = daAlink_getAlinkActorClass();
    if (link == nullptr) return MOD_UNAVAILABLE;

    const auto hookResult = install_test_hooks();
    if (hookResult != MOD_OK) return hookResult;

    constexpr float kSpawnDistance = 300.0f;
    const s16 facing = link->shape_angle.y;
    cXyz position{
        link->current.pos.x + cM_ssin(facing) * kSpawnDistance,
        link->current.pos.y,
        link->current.pos.z + cM_scos(facing) * kSpawnDistance,
    };
    cXyz probe = position;
    probe.y += 500.0f;
    dBgS_ObjGndChk ground;
    ground.SetPos(&probe);
    const float groundY = dComIfG_Bgsp().GroundCross(&ground);
    if (groundY == -1.0e9f) return MOD_UNAVAILABLE;
    position.y = groundY;
    const auto profile = kEnemySpawnerProfiles[static_cast<std::size_t>(profileIndex)];
    if (profile == fpcNm_E_BA_e || profile == fpcNm_E_BU_e || profile == fpcNm_B_GG_e) {
        position.y += 200.0f;
    }
    // Stalfos encodes its defeated switch in angle.z and clears it in Create.
    const s16 angleZ = profile == fpcNm_E_SF_e ? 0xff : 0;
    const csXyz angle{0, static_cast<s16>(facing + 0x8000), angleZ};
    const ActorSpawnParams params{
        .parameters = kEnemySpawnerParameters[static_cast<std::size_t>(profileIndex)],
        .argument = -1,
        .room_num = fopAcM_GetRoomNo(link),
        .position = {position.x, position.y, position.z},
        .angle = {angle.x, angle.y, angle.z},
        .scale = {1.0f, 1.0f, 1.0f},
        .create_function = nullptr,
    };

    ActorId actorId = fpcM_ERROR_PROCESS_ID_e;
    const auto result = svc_actor->create_actor(mod_ctx, profile, &params, &actorId);
    if (result == MOD_OK) s_testActors.insert(actorId);
    return result;
}

}  // namespace dawnlight
