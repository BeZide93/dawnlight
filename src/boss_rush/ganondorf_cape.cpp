// Adapted from BeZide93/dawnlight-twilit-essentials, commit 805fac000711135ee2d85e086ab6100d767717bd.
#include "ganondorf_cape.hpp"
#include "boss_rush.hpp"
#include "boss_rush_common.hpp"
#include "boss_rush_models.hpp"

#include "mods/svc/hook.hpp"
#include "mods/svc/actor.h"
#include "mods/svc/log.h"

#include "d/d_com_inf_game.h"
#include "d/actor/d_a_alink.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_name.h"
#include "SSystem/SComponent/c_math.h"
#include "SSystem/SComponent/c_phase.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "d/actor/d_a_b_gnd.h"

#define private public
#define protected public
#include "d/actor/d_a_mant.h"
#undef private
#undef protected

extern const ActorService* svc_actor;

namespace {

constexpr bool kGanondorfCapeEnabled = true;

constexpr f32 kShoulderHalfWidth = 45.0f;
constexpr f32 kShoulderHeight    = 175.0f;
constexpr f32 kShoulderBack      = -14.0f;

constexpr f32 kSegLen    = 26.0f;
constexpr f32 kRearDrift = 0.45f;

ModContext*       s_ctx = nullptr;


bool s_hookInstalled = false;
bool s_shuttingDown  = false;

bool s_capeWanted = false;
cXyz s_gndPos;
s16  s_gndYaw   = 0;
f32  s_gndScale = 1.0f;

ActorId s_mantId    = 0;
bool    s_mantAlive = false;

ProfileName  s_dummyProfile = 0;
ActorHandle  s_dummyHandle  = 0;
bool         s_dummyRegistered = false;
ActorId      s_dummyId    = 0;
bool         s_dummyAlive = false;
bool         s_realSimActive = false;

int dummy_create(void*)    { return cPhs_COMPLEATE_e; }
int dummy_execute(void*)   { return 1; }
int dummy_draw(void*)      { return 1; }
int dummy_is_delete(void*) { return 1; }
int dummy_delete(void*)    { return 1; }

void ensure_dummy_profile() {
    if (s_dummyRegistered || svc_actor == nullptr) return;
    ActorProfileDesc d{};
    std::strcpy(const_cast<char*>(d.name), "DawnGndCape");
    d.priority_group   = 4;
    d.process_size     = sizeof(b_gnd_class);
    d.draw_priority    = 0;
    d.status           = 0x40000u | 0x4000u;
    d.group            = 1;
    d.cull_type        = 0;
    d.create_function     = dummy_create;
    d.delete_function     = dummy_delete;
    d.execute_function    = dummy_execute;
    d.is_delete_function  = dummy_is_delete;
    d.draw_function       = dummy_draw;
    if (svc_actor->register_actor(s_ctx, &d, &s_dummyProfile, &s_dummyHandle) == MOD_OK) {
        s_dummyRegistered = true;
    }
}

void build_anchor_frame(const cXyz& a, const cXyz& b, Mtx out) {
    cXyz ax = b - a;
    if (!ax.normalizeRS()) ax = cXyz::BaseX;
    cXyz helper = (std::fabs(ax.y) > 0.95f) ? cXyz::BaseZ : cXyz::BaseY;
    cXyz az = ax.getCrossProduct(helper);
    if (!az.normalizeRS()) az = cXyz::BaseZ;
    cXyz ay = az.getCrossProduct(ax);
    if (!ay.normalizeRS()) ay = cXyz::BaseY;
    const cXyz cen = a + (b - a) * 0.5f;
    const cXyz cols[3] = {ax, ay, az};
    const f32  t[3]    = {cen.x, cen.y, cen.z};
    for (int r = 0; r < 3; ++r) {
        out[r][0] = (&cols[0].x)[r];
        out[r][1] = (&cols[1].x)[r];
        out[r][2] = (&cols[2].x)[r];
        out[r][3] = t[r];
    }
}

void shoulder_anchors(cXyz& outA, cXyz& outB) {
    if (boss_rush_get_ganondorf_cape_anchors(outA, outB)) {
        return;
    }
    const f32 rad = cM_s2rad(s_gndYaw);
    const f32 sy = std::sin(rad);
    const f32 cy = std::cos(rad);
    auto toWorld = [&](f32 lx, f32 ly, f32 lz) {
        cXyz w;
        w.x = s_gndPos.x + (lx * cy + lz * sy) * s_gndScale;
        w.y = s_gndPos.y + ly * s_gndScale;
        w.z = s_gndPos.z + (-lx * sy + lz * cy) * s_gndScale;
        return w;
    };
    outA = toWorld(+kShoulderHalfWidth, kShoulderHeight, kShoulderBack);
    outB = toWorld(-kShoulderHalfWidth, kShoulderHeight, kShoulderBack);
}

void static_drape(mant_class* m) {
    cXyz anchorA, anchorB;
    shoulder_anchors(anchorA, anchorB);
    m->field_0x3928[0] = anchorA;
    m->field_0x3928[1] = anchorB;
    m->current.pos = s_gndPos;
    m->field_0x3940 = s_gndPos;

    cXyz along = anchorA - anchorB;
    cXyz back(along.z, 0.0f, -along.x);
    if (!back.normalizeRS()) back.set(0.0f, 0.0f, -1.0f);
    const f32 seg = kSegLen * s_gndScale;

    for (int i = 0; i < 13; ++i) {
        const f32 u = static_cast<f32>(i) / 12.0f;
        cXyz p = anchorB + (anchorA - anchorB) * u;
        m->field_0x25a8[i].field_0x0[0] = p;
        for (int j = 1; j < 13; ++j) {
            const f32 fj = static_cast<f32>(j) / 12.0f;
            p += back * (seg * kRearDrift * fj);
            p.y -= seg * (0.75f + 0.25f * fj);
            m->field_0x25a8[i].field_0x0[j] = p;
        }
    }
    for (int buf = 0; buf < 2; ++buf) {
        cXyz* dst = &m->field_0x0570.mPos[buf][0];
        for (int i = 0; i < 13; ++i)
            for (int j = 0; j < 13; ++j)
                dst[i + j * 13] = m->field_0x25a8[i].field_0x0[12 - j];
    }
    build_anchor_frame(m->field_0x3928[0], m->field_0x3928[1], m->field_0x0570.mMtx);
    build_anchor_frame(m->field_0x3928[0], m->field_0x3928[1], m->field_0x0570.mMtx2);
}

bool is_our_mant(mant_class* m) {
    return m != nullptr && s_capeWanted && is_in_boss_rush_chamber() &&
           (s_mantId == 0 || fopAcM_GetID(m) == s_mantId);
}

HookAction on_mant_execute_pre(ModContext*, void* args, void*, void*) {
    mant_class* m = mods::arg<mant_class*>(args, 0);
    if (!is_our_mant(m)) {
        return HOOK_CONTINUE;
    }

    if (s_shuttingDown) {
        fopAcM_delete(m);
        return HOOK_SKIP_ORIGINAL;
    }

    fopAc_ac_c* dummy =
        (s_dummyId != 0) ? fopAcM_SearchByID(s_dummyId) : nullptr;

    if (dummy != nullptr) {
        b_gnd_class* fake = reinterpret_cast<b_gnd_class*>(dummy);
        fake->mDrawHorse = 0;
        fake->mpModelMorf = nullptr;
        fake->field_0x1fb8.set(0.0f, 0.0f, 0.0f);

        cXyz anchorA, anchorB;
        shoulder_anchors(anchorA, anchorB);
        m->field_0x3928[0] = anchorA;
        m->field_0x3928[1] = anchorB;
        m->current.pos = s_gndPos;
        m->parentActorID = fopAcM_GetID(dummy);

        s_realSimActive = true;
        return HOOK_CONTINUE;
    }

    s_realSimActive = false;
    m->parentActorID = 0;
    static_drape(m);
    m->field_0x0570.field_0x74 ^= 1;
    return HOOK_SKIP_ORIGINAL;
}

HookAction on_mant_draw_pre(ModContext*, void*, void*, void*) {
    if (!s_capeWanted) {
        return HOOK_CONTINUE;
    }
    if (s_shuttingDown || daAlink_getAlinkActorClass() == nullptr) {
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

}

#if (defined(__linux__) && !defined(__ANDROID__)) || defined(__APPLE__)
DEFINE_HOOK_SYMBOL("_ZL14daMant_ExecuteP10mant_class", int(mant_class*), MantExecuteHook);
DEFINE_HOOK_SYMBOL("_ZL11daMant_DrawP10mant_class", int(mant_class*), MantDrawHook);
#else
DEFINE_HOOK_SYMBOL("daMant_Execute", int(mant_class*), MantExecuteHook);
DEFINE_HOOK_SYMBOL("daMant_Draw", int(mant_class*), MantDrawHook);
#endif

ModResult init_ganondorf_cape(const HookService* hook_svc, const LogService*,
                              ModContext* mod_ctx) {
    if (s_hookInstalled) return MOD_OK;
    s_ctx = mod_ctx;
    s_shuttingDown = false;
    if (!kGanondorfCapeEnabled || hook_svc == nullptr) {
        return MOD_OK;
    }
    ModResult result = mods::hook::add_pre<MantExecuteHook>(hook_svc, on_mant_execute_pre);
    if (result != MOD_OK) return result;
    result = mods::hook::add_pre<MantDrawHook>(hook_svc, on_mant_draw_pre);
    if (result != MOD_OK) {
        mods::hook::uninstall<MantExecuteHook>(hook_svc);
        return result;
    }
    s_hookInstalled = true;
    return MOD_OK;
}

void update_ganondorf_cape(bool statueVisible, const cXyz& gndPos, s16 gndYaw, f32 gndScale) {
    if (!kGanondorfCapeEnabled || !s_hookInstalled || svc_actor == nullptr) {
        return;
    }

    s_capeWanted = statueVisible && is_in_boss_rush_chamber();
    s_gndPos   = gndPos;
    s_gndYaw   = gndYaw;
    s_gndScale = gndScale;

    if (s_mantAlive && (s_mantId == 0 || (fopAcM_SearchByID(s_mantId) == nullptr && !fpcM_IsCreating(s_mantId)))) {
        s_mantAlive = false; s_mantId = 0;
    }
    if (s_dummyAlive && (s_dummyId == 0 || (fopAcM_SearchByID(s_dummyId) == nullptr && !fpcM_IsCreating(s_dummyId)))) {
        s_dummyAlive = false; s_dummyId = 0;
        if (s_realSimActive) { s_realSimActive = false; }
    }

    const bool safeToSpawn = boss_rush_scene_load_stable() &&
                             daAlink_getAlinkActorClass() != nullptr;

    if (s_capeWanted && safeToSpawn) {
        ensure_dummy_profile();

        if (s_dummyRegistered && !s_dummyAlive) {
            ActorSpawnParams sp{};
            sp.room_num = static_cast<int8_t>(kBossRushChamberRoom);
            sp.position = {gndPos.x, gndPos.y, gndPos.z};
            sp.scale = {1.0f, 1.0f, 1.0f};
            ActorId id{};
            if (svc_actor->create_actor(s_ctx, s_dummyProfile, &sp, &id) == MOD_OK) {
                s_dummyId = id; s_dummyAlive = true;
                        }
        }
        if (!s_mantAlive) {
            ActorSpawnParams sp{};
            sp.room_num = static_cast<int8_t>(kBossRushChamberRoom);
            sp.position = {gndPos.x, gndPos.y, gndPos.z};
            sp.angle = {0, gndYaw, 0};
            sp.scale = {1.0f, 1.0f, 1.0f};
            ActorId id{};
            if (svc_actor->create_actor(s_ctx, fpcNm_MANT_e, &sp, &id) == MOD_OK) {
                s_mantId = id; s_mantAlive = true;
                        }
        }
    } else if (!s_capeWanted && (s_mantAlive || s_dummyAlive)) {
        if (s_mantAlive && !fpcM_IsCreating(s_mantId) && svc_actor->delete_actor(s_ctx, s_mantId) == MOD_OK) { s_mantAlive = false; s_mantId = 0; }
        if (s_dummyAlive && !s_mantAlive && !fpcM_IsCreating(s_dummyId) && svc_actor->delete_actor(s_ctx, s_dummyId) == MOD_OK) { s_dummyAlive = false; s_dummyId = 0; }
        s_realSimActive = false;
    }
}

void shutdown_ganondorf_cape(const HookService* hook_svc) {
    s_shuttingDown = true;
    if (s_mantId  != 0) fopAcM_delete(s_mantId);
    if (s_dummyId != 0) fopAcM_delete(s_dummyId);
    if (s_dummyRegistered && svc_actor != nullptr) {
        svc_actor->unregister_actor(s_ctx, s_dummyHandle);
    }
    s_mantId = 0;  s_mantAlive = false;
    s_dummyId = 0; s_dummyAlive = false;
    s_dummyRegistered = false;
    s_realSimActive = false;
    s_capeWanted = false;
    mods::hook::uninstall<MantExecuteHook>(hook_svc);
    mods::hook::uninstall<MantDrawHook>(hook_svc);
    s_hookInstalled = false;
}
