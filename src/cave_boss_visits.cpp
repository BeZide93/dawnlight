#include "cave_boss_visits.hpp"
#include "cave_boss_pool.hpp"
#include "clear_timer.hpp"
#include "config.hpp"
#include "save_state.hpp"
#include "service_imports.hpp"
#include "d/actor/d_a_npc_fairy.h"
#include "d/d_com_inf_game.h"
#include "d/d_save.h"
#include "d/d_msg_object.h"
#include "SSystem/SComponent/c_math.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include <array>
#include <cstring>

namespace dawnlight {
namespace {
DEFINE_HOOK(&daNpc_Fairy_c::evtTalk, CaveFairyTalk);
DEFINE_HOOK(&daNpc_Fairy_c::setParam, CaveFairyParams);
DEFINE_HOOK(&dSv_danBit_c::init, CaveDungeonInit);
CaveBossPool s_pool;
SaveObserverHandle s_observer = 0;
struct Visit {
    bool active = false, returning = false;
    s8 room = -1;
    s16 yaw = 0;
    int table = -1;
    cXyz position{0,0,0};
    dSv_memory_c memory{};
    dSv_danBit_c dungeon{};
    std::array<bool, 48> roomSwitches{};
} s_visit;

bool in_cave() {
    const char* stage = dComIfGp_getStartStageName();
    return save_state_boss_rush_active() && stage && std::strcmp(stage, "D_SB01") == 0;
}
void save_reset(ModContext*, uint32_t, void*) { reset_cave_boss_visits(); }

HookAction fairy_talk(ModContext*, void* args, void* result, void*) {
    auto* fairy = mods::arg<daNpc_Fairy_c*>(args, 0);
    if (!cave_boss_visits_enabled() || !in_cave() || !fairy || fairy->mStatus != 1 ||
        CaveBossPool::floor_index(fopAcM_GetRoomNo(fairy)) < 0) return HOOK_CONTINUE;
    // The native talk callback precedes AppearDemoCall; never start that event.
    dComIfGp_getEvent()->reset(fairy);
    dMsgObject_onKillMessageFlag();
    if (result) *static_cast<BOOL*>(result) = TRUE;
    auto* player = dComIfGp_getPlayer(0);
    if (!player || s_pool.cleared(fopAcM_GetRoomNo(fairy)) || !cave_boss_warp_available())
        return HOOK_SKIP_ORIGINAL;
    unsigned remaining = 0;
    for (unsigned i=0; i<CaveBossPool::count; ++i) if (!(s_pool.used & (1u << i))) ++remaining;
    const int boss = s_pool.choose(static_cast<unsigned>(cM_rndF(static_cast<float>(remaining))));
    if (boss < 0) return HOOK_SKIP_ORIGINAL;
    s_visit = {};
    s_visit.room = fopAcM_GetRoomNo(fairy);
    s_visit.position = player->current.pos;
    s_visit.yaw = player->shape_angle.y;
    s_visit.table = dStage_stagInfo_GetSaveTbl(dComIfGp_getStageStagInfo());
    auto* save = dComIfGs_getSaveInfo();
    s_visit.memory = save->getMemory();
    s_visit.dungeon = save->getDan();
    for (unsigned i=0; i<s_visit.roomSwitches.size(); ++i)
        s_visit.roomSwitches[i] = dComIfGs_isSwitch(192 + i, s_visit.room);
    if (warp_to_cave_boss(boss)) {
        s_visit.active = true;
        s_pool.reserve(boss);
        begin_cave_boss_timer(boss);
    } else s_visit = {};
    return HOOK_SKIP_ORIGINAL;
}
void fairy_params(ModContext*, void* args, void*, void*) {
    auto* fairy = mods::arg<daNpc_Fairy_c*>(args, 0);
    if (!in_cave() || !fairy) return;
    const int room = fopAcM_GetRoomNo(fairy);
    if (s_visit.returning && room == s_visit.room) {
        // Zones are recreated for the returned room. Restore by switch number,
        // never by a stale zone-array index from the previous stage.
        for (unsigned i=0; i<s_visit.roomSwitches.size(); ++i) {
            if (s_visit.roomSwitches[i]) dComIfGs_onSwitch(192 + i, room);
            else dComIfGs_offSwitch(192 + i, room);
        }
        s_visit.returning = false;
    }
    if (s_pool.cleared(room)) {
        // Native fairy dialogue normally sets this gate switch. Completing the
        // substitute boss must open it even if the option was switched off away.
        if (fairy->mSwitchBit >= 0 && fairy->mSwitchBit < 240)
            dComIfGs_onSwitch(fairy->mSwitchBit, room);
        if (cave_boss_visits_enabled()) fairy->attention_info.flags = 0;
    }
}
void dungeon_init(ModContext*, void* args, void*, void*) {
    auto* dungeon = mods::arg<dSv_danBit_c*>(args, 0);
    if (s_visit.returning && in_cave() && mods::arg<s8>(args, 1) == s_visit.table &&
        dungeon == &dComIfGs_getSaveInfo()->getDan()) {
        *dungeon = s_visit.dungeon;
        // Negative restart spawns normally retain zones. This is a different
        // stage, so discard boss-zone indices before Cave rooms are created.
        dComIfGp_roomControl_initZone();
    }
}
}

void reset_cave_boss_visits() { s_pool = {}; s_visit = {}; }
bool cave_boss_final_cleared() { return s_pool.cleared(49); }
bool return_from_cave_boss() {
    if (!s_visit.active) return false;
    s_visit.active = false;
    s_visit.returning = true;
    s_pool.complete(s_visit.room);
    // Boss preparation and combat must not overwrite the Cave's saved progress.
    dComIfGs_getSaveData()->getSave(s_visit.table) = s_visit.memory;
    end_cave_boss_timer();
    warp_back_to_cave(s_visit.position, s_visit.yaw, s_visit.room);
    return true;
}
ModResult initialize_cave_boss_visits(ModError* error) {
    auto result = svc_save->observe_saves(mod_ctx, save_reset, save_reset, nullptr, nullptr, &s_observer);
    if (result == MOD_OK) result = mods::hook_add_pre<CaveFairyTalk>(svc_hook, fairy_talk);
    if (result == MOD_OK) result = mods::hook_add_post<CaveFairyParams>(svc_hook, fairy_params);
    if (result == MOD_OK) result = mods::hook_add_post<CaveDungeonInit>(svc_hook, dungeon_init);
    return result == MOD_OK ? result : mods::set_error(error, result, "failed to install Cave fairy boss warps");
}
void shutdown_cave_boss_visits() {
    if (s_observer) svc_save->unobserve_saves(mod_ctx, s_observer);
    s_observer = 0;
    reset_cave_boss_visits();
    mods::hook_uninstall<CaveFairyTalk>(svc_hook);
    mods::hook_uninstall<CaveFairyParams>(svc_hook);
    mods::hook_uninstall<CaveDungeonInit>(svc_hook);
}
}
