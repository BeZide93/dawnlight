#include "cave_randomizer.hpp"

#include "config.hpp"
#include "cave_randomizer_ground.hpp"
#include "service_imports.hpp"
#include "SSystem/SComponent/c_math.h"
#include "SSystem/SComponent/c_phase.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_create_req.h"
#include "mods/svc/hook.hpp"

#include <array>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <vector>

// Public symbol, but no SDK declaration. Only the create_request base (the
// first member) is inspected; never mirror the private request's layout.
struct standard_create_request_class;
int fpcSCtRq_phase_Load(standard_create_request_class*);

namespace dawnlight {
namespace {
DEFINE_HOOK(&dStage_dt_c_roomReLoader, RoomActorsHook);
DEFINE_HOOK(&fpcSCtRq_Request, RequestHook);
DEFINE_HOOK(&fpcSCtRq_phase_Load, LoadHook);
DEFINE_HOOK(&fpcBs_Create, AllocateHook);

struct Enemy {
    s16 profile;
    u32 parameters;
    float height;
    s8 argument = -1;
    s16 angleZ = 0;
};

// Standalone variants only: no paths, appearance/death switches, opening demos,
// ceiling anchors, splitters or enemy generators.
constexpr Enemy kEnemies[] = {
    {fpcNm_E_OC_e, 0x00ff0000, 0.0f},   // Bokoblin
    {fpcNm_E_FZ_e, 0x00000000, 0.0f},   // Mini Freezard
    {fpcNm_E_BA_e, 0xffff0f01, 200.0f}, // Flying Keese
    {fpcNm_E_BA_e, 0xffff1f01, 200.0f}, // Fire Keese
    {fpcNm_E_BA_e, 0xffff2f01, 200.0f}, // Ice Keese
    {fpcNm_E_TT_e, 0x000000ff, 0.0f},   // Red Tektite
    {fpcNm_E_GI_e, 0x00ff01ff, 0.0f},   // Awake Gibdo
    {fpcNm_E_KK_e, 0x00ff0000, 0.0f},   // Chilfos
    {fpcNm_E_BS_e, 0x00ffff00, 0.0f},   // Stalchild
    {fpcNm_E_BU_e, 0xffff0f00, 200.0f}, // Bubble
    {fpcNm_E_BU_e, 0xffff1f00, 200.0f}, // Fire Bubble
    {fpcNm_E_BU_e, 0xffff2f00, 200.0f}, // Ice Bubble
    {fpcNm_E_MS_e, 0xffff0000, 0.0f},   // Rat
    {fpcNm_E_RD_e, 0xff000100, 0.0f},   // Club Bulblin
    {fpcNm_E_RD_e, 0xff000200, 0.0f},   // Bow Bulblin
    {fpcNm_B_GG_e, 0xffff0002, 200.0f}, // Regular Aeralfos without boss demo
    {fpcNm_E_CR_e, 0x00000000, 0.0f},   // Bomskit
    {fpcNm_E_DB_e, 0xff000000, 0.0f},   // Ground Deku Baba
    {fpcNm_E_MM_e, 0x0000ff00, 0.0f},   // Helmasaur
    {fpcNm_E_MM_e, 0x0000ff00, 0.0f, 1}, // Helmasaurus (actor argument variant)
    {fpcNm_E_KR_e, 0xffffff00, 200.0f, -1, 0xff}, // Flying Kargarok, no path/switch
    {fpcNm_E_FS_e, 0x00000000, 0.0f},   // Standalone Puppet
    {fpcNm_E_DN_e, 0xff000000, 0.0f},   // Lizalfos
    {fpcNm_E_DD_e, 0xffff0000, 0.0f},   // Dodongo
    {fpcNm_E_MF_e, 0xff000000, 0.0f},   // Dynalfos
    {fpcNm_E_ST_e, 0xff000002, 0.0f},   // Ground Skulltula
    {fpcNm_E_SF_e, 0x0000ff00, 0.0f, -1, 0xff}, // Standing Stalfos, no switch
    {fpcNm_B_TN_e, 0x000001ff, 0.0f},   // Darknut without intro
};

struct RoomLoad {
    void* args;
    int room;
    bool enabled;
};
struct PendingEnemy {
    cXyz authored;
    cXyz ground;
    s16 profile;
    s8 room;
    unsigned attempts = 0;
    bool ready = false;
};
std::vector<RoomLoad> s_roomLoads;
std::unordered_map<fpc_ProcID, PendingEnemy> s_pending;

bool in_cave() {
    const char* stage = dComIfGp_getStartStageName();
    return stage != nullptr && std::strcmp(stage, "D_SB01") == 0;
}

bool is_enemy_profile(s16 name) {
    const auto* profile = fpcPf_Get(name);
    // Check both inheritance levels before reading actor-only profile fields.
    if (profile == nullptr || profile->methods != &g_fpcLf_Method.base) return false;
    const auto* leaf = reinterpret_cast<const leaf_process_profile_definition*>(profile);
    if (leaf->sub_method != &g_fopAc_Method.base) return false;
    return reinterpret_cast<const actor_process_profile_definition*>(profile)->group == fopAc_ENEMY_e;
}

HookAction before_room(ModContext*, void* args, void*, void*) {
    const int room = mods::arg<int>(args, 2);
    const bool enabled = in_cave() && cave_randomizer_enabled() && room >= 0 && room < 64;
    s_roomLoads.push_back({args, room, enabled});
    return HOOK_CONTINUE;
}

void after_room(ModContext*, void* args, void*, void*) {
    if (!s_roomLoads.empty() && s_roomLoads.back().args == args) s_roomLoads.pop_back();
}

void after_request(ModContext*, void* args, void* retval, void*) {
    if (s_roomLoads.empty() || !s_roomLoads.back().enabled || retval == nullptr) return;
    const auto name = mods::arg<s16>(args, 1);
    if (!is_enemy_profile(name)) return;
    const auto* append = mods::arg<fopAcM_prm_class*>(args, 4);
    const auto id = *static_cast<fpc_ProcID*>(retval);
    if (append == nullptr || id == fpcM_ERROR_PROCESS_ID_e ||
        append->room_no != s_roomLoads.back().room ||
        append->parent_id != fpcM_ERROR_PROCESS_ID_e ||
        mods::arg<stdCreateFunc>(args, 2) != nullptr) return;

    // A Wolfos pack coordinator is not one enemy. Its AI casts children to
    // daE_WW_c, so replacing those children would corrupt memory. Preserve
    // such generators; directly placed combat wolves are ordinary slots.
    const u32 parameters = append->base.parameters;
    if (name == fpcNm_E_WW_e &&
        (((parameters >> 24) & 15) == 0 || ((parameters >> 24) & 15) == 15)) return;

    s_pending.emplace(id, PendingEnemy{append->base.position, {}, name, append->room_no});
}

bool find_ground(PendingEnemy& enemy) {
    const auto ground = cave::find_ground(
        {enemy.authored.x, enemy.authored.y, enemy.authored.z}, enemy.room,
        [](const cave::Point& point) {
            cXyz probe(point.x, point.y, point.z);
            dBgS_ObjGndChk check;
            check.SetPos(&probe);
            const float y = dComIfG_Bgsp().GroundCross(&check);
            if (!std::isfinite(y) || y == -1.0e9f) return cave::Surface{};
            return cave::Surface{true, y, dComIfG_Bgsp().GetRoomId(check)};
        });
    if (!ground) return false;
    enemy.ground.set(ground->x, ground->y, ground->z);
    return true;
}

HookAction before_load(ModContext*, void* args, void* retval, void*) {
    // standard_create_request_class starts with this public base.
    const auto* request = mods::arg<create_request*>(args, 0);
    if (request == nullptr) return HOOK_CONTINUE;
    const auto it = s_pending.find(request->id);
    if (it == s_pending.end()) return HOOK_CONTINUE;
    auto& enemy = it->second;
    if (!in_cave() || request->is_cancel) {
        s_pending.erase(it);
        return HOOK_CONTINUE;
    }
    if (enemy.ready || find_ground(enemy)) {
        enemy.ready = true;
        return HOOK_CONTINUE;
    }
    if (++enemy.attempts >= 120) {
        // Keep the original request completely untouched when no safe floor
        // exists. Never delete a slot or leave a room permanently loading.
        svc_log->warn(mod_ctx, "Cave randomizer: no safe floor; keeping native enemy");
        s_pending.erase(it);
        return HOOK_CONTINUE;
    }
    *static_cast<int*>(retval) = cPhs_INIT_e;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_allocate(ModContext*, void* args, void*, void*) {
    const auto it = s_pending.find(mods::arg<fpc_ProcID>(args, 1));
    if (it == s_pending.end()) return HOOK_CONTINUE;
    const auto enemy = it->second;
    s_pending.erase(it);
    auto* append = mods::arg<fopAcM_prm_class*>(args, 2);
    if (!enemy.ready || !in_cave() || append == nullptr ||
        mods::arg<s16>(args, 0) != enemy.profile || append->room_no != enemy.room) return HOOK_CONTINUE;

    std::array<unsigned, std::size(kEnemies)> choices{};
    unsigned count = 0;
    for (unsigned i = 0; i < std::size(kEnemies); ++i) {
        if (kEnemies[i].profile == enemy.profile) continue;
        choices[count++] = i;
    }
    unsigned pick = static_cast<unsigned>(cM_rndF(static_cast<float>(count)));
    if (pick >= count) pick = count - 1;
    const auto& replacement = kEnemies[choices[pick]];

    // Change the profile BEFORE allocation, never reinterpret an existing
    // enemy as a different-sized actor. Dusklight's TARGET_PC module loader is
    // a no-op (cDyl_LinkASync); actual enemy assets load in the new actor Create.
    mods::arg_ref<s16>(args, 0) = replacement.profile;
    append->base.parameters = replacement.parameters;
    cXyz position = enemy.ground;
    position.y += replacement.height;
    append->base.position = position;
    const csXyz oldAngle = append->base.angle;
    // Stalfos and Kargarok encode a switch in Z; Helmasaurus uses argument 1.
    append->base.angle = csXyz(0, oldAngle.y, replacement.angleZ);
    append->scale = {10, 10, 10};
    append->argument = replacement.argument;
    // Preserve setID, parent, room and the original creation request/layer.
    // The native cleared-room gate therefore still sees one enemy per slot.
    return HOOK_CONTINUE;
}
} // namespace

void update_cave_randomizer() {
    for (auto it = s_pending.begin(); it != s_pending.end();) {
        if (!in_cave() || !fpcM_IsCreating(it->first)) it = s_pending.erase(it);
        else ++it;
    }
}

void shutdown_cave_randomizer() {
    mods::hook::uninstall<AllocateHook>(svc_hook);
    mods::hook::uninstall<LoadHook>(svc_hook);
    mods::hook::uninstall<RequestHook>(svc_hook);
    mods::hook::uninstall<RoomActorsHook>(svc_hook);
    s_pending.clear();
    s_roomLoads.clear();
}

ModResult install_cave_randomizer(ModError* error) {
    ModResult result = mods::hook::add_pre<RoomActorsHook>(svc_hook, before_room);
    if (result == MOD_OK) result = mods::hook::add_post<RoomActorsHook>(svc_hook, after_room);
    if (result == MOD_OK) result = mods::hook::add_post<RequestHook>(svc_hook, after_request);
    if (result == MOD_OK) result = mods::hook::add_pre<LoadHook>(svc_hook, before_load);
    if (result == MOD_OK) result = mods::hook::add_pre<AllocateHook>(svc_hook, before_allocate);
    if (result != MOD_OK) {
        shutdown_cave_randomizer();
        return mods::set_error(error, result, "failed to install Cave of Ordeals randomizer");
    }
    return MOD_OK;
}
} // namespace dawnlight
