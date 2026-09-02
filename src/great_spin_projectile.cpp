#include "great_spin_projectile.hpp"

#include "config.hpp"
#include "service_imports.hpp"

#include "JSystem/JParticle/JPAEmitter.h"
#include "d/actor/d_a_alink.h"
#include "d/d_bg_s_lin_chk.h"
#include "d/d_cc_d.h"
#include "d/d_com_inf_game.h"
#include "d/d_particle_name.h"
#include "f_op/f_op_camera_mng.h"
#include "m_Do/m_Do_mtx.h"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/hook.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace dawnlight {
namespace {

DEFINE_HOOK(&daAlink_c::execute, GreatSpinPlayerExecuteHook);

constexpr float kProjectileSpeed = 40.0f;
constexpr float kProjectileRange = 2400.0f;
constexpr float kColliderHalfWidth = 150.0f;
constexpr float kColliderRadius = 55.0f;

constexpr std::array<u16, 6> kGreatSpinEffects = {
    ID_ZI_J_KAITENGIRID_A,
    ID_ZI_J_KAITENGIRID_B,
    ID_ZI_J_KAITENGIRID_C,
    ID_ZI_J_KAITENGIRID_D,
    ID_ZI_J_KAITENGIRID_E,
    ID_ZI_J_KAITENGIRID_F,
};

const std::array<JGeometry::TVec3<s16>, 6> kLeftEffectRotations = {
    JGeometry::TVec3<s16>(-0x8000, 0x2000, 0x093e),
    JGeometry::TVec3<s16>(-0x8000, 0x2000, 0x093e),
    JGeometry::TVec3<s16>(-0x8000, 0x2aaa, 0x093e),
    JGeometry::TVec3<s16>(-0x8000, 0x2aaa, 0x093e),
    JGeometry::TVec3<s16>(-0x8000, 0x2aaa, 0x093e),
    JGeometry::TVec3<s16>(-0x8000, 0x2aaa, 0x093e),
};

const std::array<JGeometry::TVec3<f32>, 6> kLeftEffectTranslations = {
    JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
    JGeometry::TVec3<f32>(0.0f, 35.0f, 0.0f),
    JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
    JGeometry::TVec3<f32>(0.0f, 45.0f, 0.0f),
    JGeometry::TVec3<f32>(0.0f, 30.0f, 0.0f),
    JGeometry::TVec3<f32>(0.0f, 50.0f, 0.0f),
};

const dCcD_SrcCps kProjectileColliderSource = {
    {
        {0, {{AT_TYPE_NORMAL_SWORD, 4, 0x1a}, {0, 0}, 0}},
        {dCcD_SE_SWORD, 3, dCcG_At_Spl_UNK_1, dCcD_MTRL_NONE, {1}},
        {dCcD_SE_NONE, 0, 0, dCcD_MTRL_NONE, {0}},
        {0},
    },
    {{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, kColliderRadius}},
};

struct GreatSpinProjectile {
    daAlink_c* owner = nullptr;
    std::uint16_t ownerId = 0;
    cXyz position{};
    cXyz direction{};
    s16 rotationX = 0;
    s16 rotationY = 0;
    float distance = 0.0f;
    float collisionStartDistance = 0.0f;
    bool active = false;
    bool leftSpin = false;
    dCcD_Stts colliderStatus{};
    dCcD_Cps collider{};
    std::array<u32, kGreatSpinEffects.size()> emitters{};
};

GreatSpinProjectile s_projectile{};
daAlink_c* s_spinOwner = nullptr;
std::uint16_t s_spinOwnerId = 0;
bool s_launchedForCurrentSpin = false;

bool is_large_spin(const daAlink_c* link) {
    if (link == nullptr) {
        return false;
    }
    const u8 cutType = link->getCutType();
    return cutType == daPy_py_c::CUT_TYPE_LARGE_TURN_LEFT ||
           cutType == daPy_py_c::CUT_TYPE_LARGE_TURN_RIGHT;
}

void stop_projectile_effects() {
    for (u32& emitterId : s_projectile.emitters) {
        if (emitterId == 0) {
            continue;
        }
        if (JPABaseEmitter* emitter = dComIfGp_particle_getEmitter(emitterId);
            emitter != nullptr)
        {
            emitter->stopCreateParticle();
            emitter->quitImmortalEmitter();
            emitter->becomeInvalidEmitter();
        }
        emitterId = 0;
    }
}

void clear_projectile() {
    stop_projectile_effects();
    s_projectile.collider.OffAtSetBit();
    s_projectile.collider.ResetAtHit();
    s_projectile.owner = nullptr;
    s_projectile.ownerId = 0;
    s_projectile.distance = 0.0f;
    s_projectile.collisionStartDistance = 0.0f;
    s_projectile.active = false;
}

bool emitter_cycle_finished(JPABaseEmitter* emitter) {
    return emitter == nullptr ||
           (emitter->mMaxFrame > 0 && emitter->getAge() >= emitter->mMaxFrame);
}

void update_projectile_effects() {
    const csXyz rotation(s_projectile.rotationX, s_projectile.rotationY, 0);
    for (std::size_t i = 0; i < kGreatSpinEffects.size(); ++i) {
        u32& emitterId = s_projectile.emitters[i];
        JPABaseEmitter* emitter =
            emitterId != 0 ? dComIfGp_particle_getEmitter(emitterId) : nullptr;
        if (emitterId != 0 && emitter_cycle_finished(emitter)) {
            if (emitter != nullptr) {
                emitter->stopCreateParticle();
                emitter->becomeInvalidEmitter();
            }
            emitterId = 0;
        }

        emitterId = dComIfGp_particle_set(
            emitterId, kGreatSpinEffects[i], &s_projectile.position,
            &s_projectile.owner->tevStr, &rotation, nullptr, 0xff, nullptr, -1,
            nullptr, nullptr, nullptr);
        if (emitterId == 0) {
            continue;
        }

        dComIfGp_particle_levelEmitterOnEventMove(emitterId);
        emitter = dComIfGp_particle_getEmitter(emitterId);
        if (emitter == nullptr) {
            continue;
        }
        emitter->setGlobalTranslation(s_projectile.position);
        if (s_projectile.leftSpin) {
            emitter->setLocalRotation(kLeftEffectRotations[i]);
            if (kLeftEffectTranslations[i].y > 1.0f) {
                emitter->setLocalTranslation(kLeftEffectTranslations[i]);
            }
        }
    }
}

void initialize_projectile_collider(daAlink_c* link) {
    s_projectile.colliderStatus.Init(120, 0xff, link);
    s_projectile.collider.Set(kProjectileColliderSource);
    s_projectile.collider.SetStts(&s_projectile.colliderStatus);
    s_projectile.collider.SetAtType(daAlink_c::getSwordAtType());
    s_projectile.collider.SetAtAtp(4);
    s_projectile.collider.SetAtSpl(dCcG_At_Spl_UNK_1);
    s_projectile.collider.SetAtHitMark(3);
    s_projectile.collider.SetAtSe(dCcD_SE_SWORD);
    s_projectile.collider.SetAtMtrl(
        link->checkNoResetFlg3(daPy_py_c::FLG3_UNK_100000) ? dCcD_MTRL_LIGHT :
                                                              dCcD_MTRL_NONE);
    s_projectile.collider.OnAtNoConHit();
    s_projectile.collider.OnAtSetBit();
    s_projectile.collider.ResetAtHit();
}

bool set_projectile_direction_from(cXyz direction) {
    const f32 length = direction.abs();
    if (length <= 0.001f) {
        return false;
    }

    direction.x /= length;
    direction.y /= length;
    direction.z /= length;
    const f32 horizontal =
        JMAFastSqrt(SQUARE(direction.x) + SQUARE(direction.z));
    if (horizontal > 0.001f) {
        s_projectile.rotationY = cM_atan2s(direction.x, direction.z);
    }

    s_projectile.direction = direction;
    s_projectile.rotationX = -cM_atan2s(direction.y, horizontal);
    return true;
}

void set_projectile_direction(daAlink_c* link) {
    s_projectile.rotationX = 0;
    s_projectile.rotationY = link->shape_angle.y;
    s_projectile.direction.set(
        cM_ssin(s_projectile.rotationY), 0.0f, cM_scos(s_projectile.rotationY));

    if (link->mTargetedActor != nullptr) {
        set_projectile_direction_from(
            link->mTargetedActor->eyePos - s_projectile.position);
        return;
    }

    auto* camera = dComIfGp_getCamera(link->field_0x317c);
    if (camera != nullptr) {
        set_projectile_direction_from(
            *fopCamM_GetCenter_p(camera) - *fopCamM_GetEye_p(camera));
    }
}

void launch_projectile(daAlink_c* link) {
    clear_projectile();

    s_projectile.owner = link;
    s_projectile.ownerId = link->setID;
    mDoMtx_multVecZero(link->getLinkBackBone1Matrix(), &s_projectile.position);
    set_projectile_direction(link);
    s_projectile.leftSpin =
        link->getCutType() == daPy_py_c::CUT_TYPE_LARGE_TURN_LEFT;
    s_projectile.distance = 0.0f;
    s_projectile.collisionStartDistance = link->field_0x3478 + kColliderRadius;
    s_projectile.active = true;

    initialize_projectile_collider(link);
    update_projectile_effects();
}

bool projectile_owner_is_current(daAlink_c* link) {
    return s_projectile.owner == link && link != nullptr &&
           s_projectile.ownerId == link->setID;
}

void update_projectile(daAlink_c* link) {
    if (!s_projectile.active) {
        return;
    }
    if (!projectile_owner_is_current(link)) {
        clear_projectile();
        return;
    }
    s_projectile.colliderStatus.Move();
    if (s_projectile.collider.ChkAtShieldHit()) {
        clear_projectile();
        return;
    }

    cXyz nextPosition = s_projectile.position + s_projectile.direction * kProjectileSpeed;
    dBgS_LinChk wallCheck;
    wallCheck.Set(&s_projectile.position, &nextPosition, link);
    if (dComIfG_Bgsp().LineCross(&wallCheck)) {
        clear_projectile();
        return;
    }

    s_projectile.position = nextPosition;
    s_projectile.distance += kProjectileSpeed;
    if (s_projectile.distance > kProjectileRange) {
        clear_projectile();
        return;
    }

    update_projectile_effects();
    if (s_projectile.distance < s_projectile.collisionStartDistance) {
        return;
    }

    const f32 horizontal = JMAFastSqrt(
        SQUARE(s_projectile.direction.x) + SQUARE(s_projectile.direction.z));
    const cXyz right = horizontal > 0.001f ?
        cXyz(s_projectile.direction.z / horizontal, 0.0f,
             -s_projectile.direction.x / horizontal) :
        cXyz(cM_scos(s_projectile.rotationY), 0.0f,
             -cM_ssin(s_projectile.rotationY));
    const cXyz start = s_projectile.position - right * kColliderHalfWidth;
    const cXyz end = s_projectile.position + right * kColliderHalfWidth;
    static_cast<cM3dGCps*>(&s_projectile.collider)->Set(start, end, kColliderRadius);
    cXyz attackVector = s_projectile.direction * kProjectileSpeed;
    s_projectile.collider.SetAtVec(attackVector);
    dComIfG_Ccsp()->Set(&s_projectile.collider);
}

void update_spin_launch(daAlink_c* link) {
    if (s_spinOwner != link || s_spinOwnerId != link->setID) {
        s_spinOwner = link;
        s_spinOwnerId = link->setID;
        s_launchedForCurrentSpin = false;
    }

    if (link->mProcID != daAlink_c::PROC_CUT_TURN) {
        s_launchedForCurrentSpin = false;
        return;
    }
    if (s_launchedForCurrentSpin || !is_large_spin(link)) {
        return;
    }

    const float frame = link->mUnderFrameCtrl[0].getFrame();
    if (frame >= link->field_0x3484 && frame < link->field_0x3488) {
        s_launchedForCurrentSpin = true;
        launch_projectile(link);
    }
}

void after_player_execute(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr || !great_spin_projectile_enabled() ||
        link->checkSceneChangeAreaStart())
    {
        clear_projectile();
        s_spinOwner = link;
        s_spinOwnerId = link != nullptr ? link->setID : 0;
        s_launchedForCurrentSpin = false;
        return;
    }

    update_projectile(link);
    update_spin_launch(link);
}

}  // namespace

ModResult initialize_great_spin_projectile(ModError* error) {
    const ModResult result =
        mods::hook::add_post<GreatSpinPlayerExecuteHook>(svc_hook, after_player_execute);
    if (result != MOD_OK) {
        return mods::set_error(
            error, result, "failed to install Dawnlight Great Spin projectile hook");
    }
    return MOD_OK;
}

void shutdown_great_spin_projectile() {
    clear_projectile();
    s_spinOwner = nullptr;
    s_spinOwnerId = 0;
    s_launchedForCurrentSpin = false;
}

}  // namespace dawnlight
