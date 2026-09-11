#include "bullet_time.hpp"

#include "aim_hooks.hpp"
#include "config.hpp"
#include "enemy_slow_motion.hpp"
#include "service_imports.hpp"
#include "stamina.hpp"

#include "global.h"
#include "dusk/audio/MusicRateBuffer.h"
#include "dusk/simulation_accumulator.h"
#include "slow_motion/Controller.h"
#include "SSystem/SComponent/c_cc_d.h"
#include "SSystem/SComponent/c_cc_s.h"
#include "JSystem/J3DGraphAnimator/J3DModel.h"
#include "Z2AudioLib/Z2LinkMgr.h"
#include "d/actor/d_a_alink.h"
#if __has_include("dusk/gyro.h") && __has_include("dusk/settings.h")
#define DAWNLIGHT_HAS_GYRO_API 1
#include "dusk/gyro.h"
#include "dusk/settings.h"
#else
#define DAWNLIGHT_HAS_GYRO_API 0
#endif
#include "d/actor/d_a_arrow.h"
#include "d/d_cc_s.h"
#include "d/d_cc_uty.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2_draw.h"
#include "f_op/f_op_actor.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_manager.h"
#include "f_pc/f_pc_method.h"
#include "f_pc/f_pc_name.h"
#include "m_Do/m_Do_graphic.h"
#include "mods/service.hpp"
#include "mods/svc/gfx.h"
#include "mods/svc/hook.h"
#include "mods/svc/hook.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "../integration/dusklight_edges.inc"

namespace dawnlight {
namespace {

using Clock = std::chrono::steady_clock;

constexpr float kSimulationPeriod = 1.0f / 30.0f;
constexpr float kEnemyTimeScale = 0.1f;
constexpr float kFlurryLinkTimeScale = 0.25f;
constexpr float kArrowTimeScale = 0.2f;
constexpr float kLinkTimeScale = 0.1f;
constexpr auto kBulletTimeDuration = std::chrono::seconds(5);
constexpr auto kManualJumpTimeout = std::chrono::seconds(7);
constexpr auto kFlurryRushDuration = std::chrono::seconds(3);
constexpr float kPerfectDodgeMargin = 45.0f;
constexpr float kFlurryRushMeleeDistance = 120.0f;
constexpr u8 kFlurryRushActionStatus = BUTTON_STATUS_UNK_129;
constexpr auto kColliderCacheEntries = std::size_t{256};
constexpr auto kCollidersPerActor = std::size_t{64};
constexpr auto kHitActorEntries = std::size_t{32};
constexpr auto kArrowFlightEntries = std::size_t{8};
constexpr std::uint64_t kHitExecuteGraceFrames = 6;

#if (defined(__linux__) && !defined(__ANDROID__)) || defined(__APPLE__)
DEFINE_HOOK_SYMBOL("_ZL13fopAc_ExecutePv", int(void*), ActorExecuteHook);
#else
DEFINE_HOOK_SYMBOL("fopAc_Execute", int(void*), ActorExecuteHook);
#endif
#if defined(__APPLE__)
DEFINE_HOOK(&fpcMtd_Method, ProcessMethodHook);
#else
DEFINE_HOOK(&fpcMtd_Execute, ProcessExecuteHook);
#endif
DEFINE_HOOK(&cCcS::Set, ColliderSetHook);
DEFINE_HOOK(&daArrow_c::atHitCallBack, ArrowHitHook);
DEFINE_HOOK(&daAlink_c::allAnimePlay, LinkAllAnimePlayHook);
DEFINE_HOOK(&daAlink_c::posMove, LinkPosMoveHook);
DEFINE_HOOK(&daAlink_c::checkDamageAction, LinkDamageActionHook);
#if defined(_WIN32)
DEFINE_HOOK_SYMBOL(
    "?startLinkVoice@Z2CreatureLink@@QEAAPEAVZ2SoundHandlePool@@VJAISoundID@@C@Z",
    Z2SoundHandlePool*(Z2CreatureLink*, JAISoundID, s8), LinkVoiceStartHook);
#else
DEFINE_HOOK(&Z2CreatureLink::startLinkVoice, LinkVoiceStartHook);
#endif
DEFINE_HOOK(&cCcS::SetAtTgCommonHitInf, CommonAtTgHitHook);
DEFINE_HOOK(&cc_at_check, AtCheckHook);
DEFINE_HOOK(&at_power_check, FlurryAttackPowerHook);
DEFINE_HOOK(&dMeter2Draw_c::getActionString, FlurryActionStringHook);
DEFINE_HOOK(&J3DModel::calc, CombatModelCalcHook);
DEFINE_HOOK(&J3DModel::viewCalc, CombatModelViewCalcHook);
DEFINE_HOOK(&fpcM_DrawIterater, DrawIteraterHook);
DEFINE_HOOK_SYMBOL("dusk::audio::DspRender", void(void*), DspRenderHook);

struct ColliderCacheEntry {
    fopAc_ac_c* actor = nullptr;
    std::array<cCcD_Obj*, kCollidersPerActor> colliders{};
    std::uint64_t lastSeenFrame = 0;
    u16 actorId = 0;
    std::size_t count = 0;
};

struct HitActorEntry {
    fopAc_ac_c* actor = nullptr;
    std::uint64_t frame = 0;
};

struct ArrowFlightEntry {
    daArrow_c* arrow = nullptr;
    u16 actorId = 0;
};

struct LinkPositionStep {
    daAlink_c* link = nullptr;
    cXyz startPosition{};
    float gravity = 0.0f;
    float scale = 1.0f;
    bool active = false;
};

struct LinkAnimationRateStep {
    daAlink_c* link = nullptr;
    std::array<float, 3> underRates{};
    std::array<float, 3> upperRates{};
    bool active = false;
};

struct DeferredFlurryDamage {
    fopAc_ac_c* targetActor = nullptr;
    fpc_ProcID targetActorId = fpcM_ERROR_PROCESS_ID_e;
    cCcD_Obj* attackCollider = nullptr;
    cCcD_Obj* targetCollider = nullptr;
    cXyz hitPosition{};
    std::uint64_t lastAttackSerial = 0;
    bool setAttackHit = false;
    bool pending = false;
};

using MatrixPose = std::array<float, 12>;
using VectorPose = std::array<float, 3>;

struct QuaternionPose {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;
};

struct TransformPose {
    VectorPose translation{};
    VectorPose scale{1.0f, 1.0f, 1.0f};
    QuaternionPose rotation{};
};

struct SlowActorClockEntry {
    fopAc_ac_c* actor = nullptr;
    fpc_ProcID actorId = fpcM_ERROR_PROCESS_ID_e;
    std::uint64_t lastSeenFrame = 0;
    dusk::game_clock::SimulationAccumulator accumulator{kSimulationPeriod};
    int pendingTicks = 0;
};

constexpr int kAudioChannels = 2;
constexpr int kAudioSubframeSize = 0x50;

struct AudioOutputSubframe {
    std::array<std::array<float, kAudioSubframeSize>, kAudioChannels> channels{};
};

struct NativeAudioSource {
    std::array<float, kAudioSubframeSize * kAudioChannels> samples{};
    int position = 0;
    int count = 0;

    void reset() {
        position = 0;
        count = 0;
    }

    void render(float* output, int frames);
};

struct LinkVoiceRateEntry {
    Z2SoundHandlePool* handle = nullptr;
    JAISound* sound = nullptr;
    float basePitch = 1.0f;
    bool compensated = false;
};

struct ModelVisualState {
    J3DModel* model = nullptr;
    fopAc_ac_c* actor = nullptr;
    fpc_ProcID actorId = fpcM_ERROR_PROCESS_ID_e;
    std::uint64_t targetFrame = 0;
    std::uint64_t captureFrame = 0;
    std::uint64_t lastSeenFrame = 0;
    std::vector<MatrixPose> startJoints;
    std::vector<MatrixPose> targetJoints;
    std::vector<MatrixPose> backupJoints;
    std::vector<MatrixPose> startWeights;
    std::vector<MatrixPose> targetWeights;
    std::vector<MatrixPose> backupWeights;
    MatrixPose startBase{};
    MatrixPose targetBase{};
    MatrixPose backupBase{};
    VectorPose rootStartTangent{};
    bool initialized = false;
    bool viewApplied = false;
};

daAlink_c* s_manualJumpOwner = nullptr;
Clock::time_point s_manualJumpStarted{};
Clock::time_point s_bulletTimeStarted{};
daAlink_c* s_flurryRushOwner = nullptr;
fopAc_ac_c* s_flurryRushTarget = nullptr;
Clock::time_point s_flurryRushStarted{};
daAlink_c* s_dodgeOwner = nullptr;
u16 s_dodgeProc = daAlink_c::PROC_WAIT;
bool s_dodgeTriggered = false;
std::array<ColliderCacheEntry, kColliderCacheEntries> s_colliderCache{};
std::array<HitActorEntry, kHitActorEntries> s_hitActors{};
std::array<ArrowFlightEntry, kArrowFlightEntries> s_flyingArrows{};
std::array<fopAc_ac_c*, 8> s_actorExecuteStack{};
std::size_t s_actorExecuteDepth = 0;
std::array<SlowActorClockEntry, 256> s_actorClocks{};
std::uint64_t s_slowFrame = 0;
float s_previousGravity = 0.0f;
float s_previousMaxFallSpeed = 0.0f;
bool s_previousSpecialGravity = false;
bool s_bulletTimeActive = false;
#if DAWNLIGHT_HAS_GYRO_API
bool s_bulletTimeOwnsGyroKeepAlive = false;
using GetGyroKeepAliveFn = bool (*)();
using SetGyroKeepAliveFn = void (*)(bool);
using GetGyroAimDeltasFn = void (*)(float&, float&);
using GetSettingsFn = dusk::UserSettings& (*)();
GetGyroKeepAliveFn s_getGyroKeepAlive = nullptr;
SetGyroKeepAliveFn s_setGyroKeepAlive = nullptr;
GetGyroAimDeltasFn s_getGyroAimDeltas = nullptr;
GetSettingsFn s_getSettings = nullptr;
bool s_gyroKeepAliveSymbolsResolved = false;
#endif
bool s_flurryRushActive = false;
bool s_flurryLinkSlowed = false;
bool s_flurryMeleePositioned = false;
bool s_bulletTimeUsedForJump = false;
bool s_bowBulletTimePending = false;
std::uint64_t s_flurrySwordAttackSerial = 0;
u16 s_flurryLastSwordProc = daAlink_c::PROC_WAIT;
u8 s_flurryLastCutCount = 0;
bool s_flurrySwordAttackWasActive = false;
LinkPositionStep s_linkPositionStep{};
LinkAnimationRateStep s_linkAnimationRateStep{};
DeferredFlurryDamage s_deferredFlurryDamage{};
std::array<ModelVisualState, 128> s_modelVisualStates{};
cCcD_Obj* s_heldFlurryPowerCollider = nullptr;
fopAc_ac_c* s_heldFlurryTarget = nullptr;
std::uint32_t s_heldFlurryAttackCount = 0;
thread_local bool s_flurryDamageCheckActive = false;
thread_local bool s_flurryDamageScaled = false;
slow_motion::Controller s_enemySlowMotion;
slow_motion::Controller s_arrowSlowMotion;
slow_motion::Controller s_flurryLinkSlowMotion;
Clock::time_point s_lastPresentationSample{};
GfxStageHookHandle s_edgesHook = 0;
std::atomic<float> s_audioRate{1.0f};
thread_local dusk::audio::MusicRateBuffer s_musicRateBuffer;
thread_local NativeAudioSource s_nativeAudioSource;
thread_local bool s_audioSlowMotionActive = false;
std::array<LinkVoiceRateEntry, 8> s_linkVoiceRates{};

bool combat_slow_active() {
    return s_bulletTimeActive || s_flurryRushActive;
}

#if DAWNLIGHT_HAS_GYRO_API
void resolve_bullet_time_gyro_symbols() {
    if (s_gyroKeepAliveSymbolsResolved || svc_hook == nullptr ||
        svc_hook->resolve == nullptr)
    {
        return;
    }

    constexpr const char* getSymbol = "dusk::gyro::get_sensor_keep_alive";
    constexpr const char* setSymbol = "dusk::gyro::set_sensor_keep_alive";
    constexpr const char* deltasSymbol = "dusk::gyro::getAimDeltas";
    constexpr const char* settingsSymbol = "dusk::getSettings";

    void* address = nullptr;
    if (svc_hook->resolve(mod_ctx, getSymbol, &address, nullptr) == MOD_OK) {
        s_getGyroKeepAlive = reinterpret_cast<GetGyroKeepAliveFn>(address);
    }
    address = nullptr;
    if (svc_hook->resolve(mod_ctx, setSymbol, &address, nullptr) == MOD_OK) {
        s_setGyroKeepAlive = reinterpret_cast<SetGyroKeepAliveFn>(address);
    }
    address = nullptr;
    if (svc_hook->resolve(mod_ctx, deltasSymbol, &address, nullptr) == MOD_OK) {
        s_getGyroAimDeltas = reinterpret_cast<GetGyroAimDeltasFn>(address);
    }
    address = nullptr;
    if (svc_hook->resolve(mod_ctx, settingsSymbol, &address, nullptr) == MOD_OK) {
        s_getSettings = reinterpret_cast<GetSettingsFn>(address);
    }
    s_gyroKeepAliveSymbolsResolved = true;
}

bool bullet_time_gyro_enabled() {
    resolve_bullet_time_gyro_symbols();
    return s_getSettings != nullptr &&
           s_getSettings().game.enableGyroAim.getValue();
}

void sync_bullet_time_gyro_keep_alive() {
    resolve_bullet_time_gyro_symbols();
    if (s_getGyroKeepAlive == nullptr || s_setGyroKeepAlive == nullptr ||
        !bullet_time_gyro_enabled())
    {
        if (s_bulletTimeOwnsGyroKeepAlive && s_setGyroKeepAlive != nullptr) {
            s_setGyroKeepAlive(false);
            s_bulletTimeOwnsGyroKeepAlive = false;
        }
        return;
    }

    const bool sensorKeepAlive = s_getGyroKeepAlive();
    if (s_bulletTimeActive && !sensorKeepAlive) {
        s_setGyroKeepAlive(true);
        s_bulletTimeOwnsGyroKeepAlive = true;
    } else if (!s_bulletTimeActive && s_bulletTimeOwnsGyroKeepAlive) {
        s_setGyroKeepAlive(false);
        s_bulletTimeOwnsGyroKeepAlive = false;
    }
}

void apply_bullet_time_gyro_impl(daAlink_c* link) {
    if (link == nullptr || !bullet_time_gyro_enabled() ||
        s_getGyroAimDeltas == nullptr)
    {
        return;
    }

    float yaw = 0.0f;
    float pitch = 0.0f;
    s_getGyroAimDeltas(yaw, pitch);

    float scale = 1.0f;
    if (link->checkWolfEyeUp()) {
        scale *= 0.6f;
    }
    if (dComIfGp_checkPlayerStatus0(0, 0x200000)) {
        scale /= dComIfGp_getCameraZoomScale(link->field_0x317c);
    }

    link->shape_angle.y += cM_rad2s(yaw * scale);
    link->mBodyAngle.x = link->checkBodyAngleX(
        static_cast<s16>(link->mBodyAngle.x + cM_rad2s(pitch * scale)));
    link->field_0x310a = link->mBodyAngle.x;
    link->field_0x310c = link->shape_angle.y;
}
#else
void sync_bullet_time_gyro_keep_alive() {}

void apply_bullet_time_gyro_impl(daAlink_c*) {}
#endif

void after_flurry_action_string(ModContext*, void* args, void* retval, void*) {
    if (!s_flurryRushActive || retval == nullptr ||
        mods::arg<u8>(args, 1) != kFlurryRushActionStatus)
    {
        return;
    }

    static char prompt[] = "Flurry Rush";
    if (u8* drawType = mods::arg<u8*>(args, 3); drawType != nullptr) {
        *drawType = MESSAGE_DRAW_UI_ACTION;
    }
    *static_cast<char**>(retval) = prompt;
}

void NativeAudioSource::render(float* output, int frames) {
    while (frames > 0) {
        if (position >= count) {
            AudioOutputSubframe rendered{};
            DspRenderHook::g_orig(&rendered);
            for (int frame = 0; frame < kAudioSubframeSize; ++frame) {
                for (int channel = 0; channel < kAudioChannels; ++channel) {
                    samples[frame * kAudioChannels + channel] =
                        rendered.channels[channel][frame];
                }
            }
            position = 0;
            count = kAudioSubframeSize;
        }

        const int copyFrames = std::min(frames, count - position);
        std::copy_n(samples.data() + position * kAudioChannels,
                    copyFrames * kAudioChannels, output);
        output += copyFrames * kAudioChannels;
        position += copyFrames;
        frames -= copyFrames;
    }
}

void remember_link_voice(Z2SoundHandlePool* handle) {
    if (handle == nullptr || !*handle) {
        return;
    }

    LinkVoiceRateEntry* destination = &s_linkVoiceRates.front();
    for (LinkVoiceRateEntry& entry : s_linkVoiceRates) {
        if (entry.handle == handle || entry.handle == nullptr ||
            !*entry.handle || entry.handle->getSound() != entry.sound)
        {
            destination = &entry;
            break;
        }
    }

    JAISound* sound = handle->getSound();
    *destination = {
        .handle = handle,
        .sound = sound,
        .basePitch = sound->getAuxiliary().params_.mPitch,
    };
}

void update_link_voice_rates(float outputRate) {
    for (LinkVoiceRateEntry& entry : s_linkVoiceRates) {
        if (entry.handle == nullptr) {
            continue;
        }
        if (!*entry.handle || entry.handle->getSound() != entry.sound) {
            entry = {};
            continue;
        }

        if (outputRate < 0.999f) {
            entry.sound->getAuxiliary().movePitch(entry.basePitch / outputRate, 0);
            entry.compensated = true;
        } else if (entry.compensated) {
            entry.sound->getAuxiliary().movePitch(entry.basePitch, 0);
            entry.compensated = false;
        }
    }
}

void clear_link_voice_rates() {
    update_link_voice_rates(1.0f);
    s_linkVoiceRates = {};
}

void after_link_voice_start(ModContext*, void*, void* retval, void*) {
    if (retval != nullptr) {
        remember_link_voice(*static_cast<Z2SoundHandlePool**>(retval));
    }
}

void replace_dsp_render(ModContext*, void* args, void*, void*) {
    auto* output = static_cast<AudioOutputSubframe*>(mods::arg<void*>(args, 0));
    if (output == nullptr || DspRenderHook::g_orig == nullptr) {
        return;
    }

    const float rate = s_audioRate.load(std::memory_order_relaxed);
    if (rate >= 0.999f) {
        if (s_audioSlowMotionActive) {
            s_musicRateBuffer.reset();
            s_nativeAudioSource.reset();
            s_audioSlowMotionActive = false;
        }
        DspRenderHook::g_orig(output);
        return;
    }

    s_audioSlowMotionActive = true;
    std::array<float, kAudioSubframeSize * kAudioChannels> mixed{};
    s_musicRateBuffer.mix(mixed.data(), kAudioSubframeSize, rate,
        [](float* target, int renderFrames) {
            s_nativeAudioSource.render(target, renderFrames);
        });
    for (int frame = 0; frame < kAudioSubframeSize; ++frame) {
        for (int channel = 0; channel < kAudioChannels; ++channel) {
            output->channels[channel][frame] = mixed[frame * kAudioChannels + channel];
        }
    }
}

void sync_slow_motion_controllers() {
    if (combat_slow_active()) {
        s_enemySlowMotion.start(kEnemyTimeScale);
    } else if (s_enemySlowMotion.active()) {
        s_enemySlowMotion.stop();
    }

    if (s_bulletTimeActive) {
        s_arrowSlowMotion.start(kArrowTimeScale);
    } else if (s_arrowSlowMotion.active()) {
        s_arrowSlowMotion.stop();
    }

    if (s_flurryRushActive && s_flurryLinkSlowed) {
        s_flurryLinkSlowMotion.start(kFlurryLinkTimeScale);
    } else if (s_flurryLinkSlowMotion.active()) {
        s_flurryLinkSlowMotion.stop();
    }
}

void clear_model_visual_states() {
    for (ModelVisualState& state : s_modelVisualStates) {
        state = {};
    }
}

void clear_combat_time_caches() {
    s_colliderCache = {};
    s_hitActors = {};
    s_flyingArrows = {};
    s_actorClocks = {};
    s_slowFrame = 0;
    clear_model_visual_states();
}

void clear_deferred_flurry_damage() {
    s_deferredFlurryDamage = {};
}

void clear_held_flurry_damage() {
    s_heldFlurryPowerCollider = nullptr;
    s_heldFlurryTarget = nullptr;
    s_heldFlurryAttackCount = 0;
}

void release_deferred_flurry_damage() {
    const DeferredFlurryDamage deferred = s_deferredFlurryDamage;
    clear_deferred_flurry_damage();
    if (!deferred.pending || s_flurrySwordAttackSerial == 0 ||
        s_flurryRushOwner == nullptr ||
        daAlink_getAlinkActorClass() != s_flurryRushOwner ||
        fopAcM_SearchByID(deferred.targetActorId) != deferred.targetActor ||
        deferred.attackCollider == nullptr || deferred.targetCollider == nullptr ||
        deferred.attackCollider->GetAc() != s_flurryRushOwner ||
        deferred.targetCollider->GetAc() != deferred.targetActor)
    {
        return;
    }

    cCcD_Stts* attackStatus = deferred.attackCollider->GetStts();
    cCcD_Stts* targetStatus = deferred.targetCollider->GetStts();
    auto* attackInfo = static_cast<dCcD_GObjInf*>(
        deferred.attackCollider->GetGObjInf());
    auto* targetInfo = static_cast<dCcD_GObjInf*>(
        deferred.targetCollider->GetGObjInf());
    if (attackStatus == nullptr || targetStatus == nullptr ||
        attackInfo == nullptr || targetInfo == nullptr)
    {
        return;
    }

    clear_held_flurry_damage();
    if (deferred.setAttackHit) {
        deferred.attackCollider->SetAtHit(deferred.targetCollider);
    }
    deferred.targetCollider->SetTgHit(deferred.attackCollider);

    cXyz hitPosition = deferred.hitPosition;
    dComIfG_Ccsp()->SetAtTgGObjInf(
        deferred.setAttackHit, true,
        deferred.attackCollider, deferred.targetCollider,
        attackInfo, targetInfo,
        attackStatus, targetStatus,
        attackStatus->GetGStts(), targetStatus->GetGStts(),
        &hitPosition);
    s_heldFlurryPowerCollider = deferred.attackCollider;
    s_heldFlurryTarget = deferred.targetActor;
    s_heldFlurryAttackCount = static_cast<std::uint32_t>(s_flurrySwordAttackSerial);
}

void stop_bullet_time() {
    if (!s_bulletTimeActive) {
        return;
    }

    if (s_manualJumpOwner != nullptr &&
        daAlink_getAlinkActorClass() == s_manualJumpOwner)
    {
        s_manualJumpOwner->setSpecialGravity(
            s_previousGravity, s_previousMaxFallSpeed,
            s_previousSpecialGravity ? FALSE : TRUE);
    }
    s_bulletTimeActive = false;
    sync_bullet_time_gyro_keep_alive();
    sync_slow_motion_controllers();
    if (!combat_slow_active()) {
        clear_combat_time_caches();
    }
}

void stop_flurry_rush(bool releaseDamage = true) {
    if (!s_flurryRushActive) {
        s_flurryMeleePositioned = false;
        clear_deferred_flurry_damage();
        return;
    }

    if (releaseDamage) {
        release_deferred_flurry_damage();
    } else {
        clear_deferred_flurry_damage();
        clear_held_flurry_damage();
    }
    if (s_flurryRushOwner != nullptr &&
        dComIfGp_getAStatus() == kFlurryRushActionStatus)
    {
        s_flurryRushOwner->setBStatus(BUTTON_STATUS_NONE);
    }
    s_flurryRushActive = false;
    s_flurryRushOwner = nullptr;
    s_flurryRushTarget = nullptr;
    s_flurryLinkSlowed = false;
    s_flurryMeleePositioned = false;
    s_flurrySwordAttackSerial = 0;
    s_flurryLastSwordProc = daAlink_c::PROC_WAIT;
    s_flurryLastCutCount = 0;
    s_flurrySwordAttackWasActive = false;
    sync_slow_motion_controllers();
    if (!combat_slow_active()) {
        clear_combat_time_caches();
    }
}

void start_bullet_time(daAlink_c* link) {
    if (s_bulletTimeActive || s_bulletTimeUsedForJump || link == nullptr ||
        !stamina_available_for_bullet_time())
    {
        return;
    }

    stop_flurry_rush();
    s_previousGravity = link->gravity;
    s_previousMaxFallSpeed = link->maxFallSpeed;
    s_previousSpecialGravity =
        link->checkNoResetFlg3(daPy_py_c::FLG3_UNK_4000) != 0;
    clear_combat_time_caches();
    s_bulletTimeStarted = Clock::now();
    link->setSpecialGravity(s_previousGravity, s_previousMaxFallSpeed, FALSE);
    s_bulletTimeActive = true;
    s_bulletTimeUsedForJump = true;
    sync_bullet_time_gyro_keep_alive();
    sync_slow_motion_controllers();
}

bool actor_is_exempt(fopAc_ac_c* actor) {
    if (actor == nullptr) {
        return true;
    }

    switch (fopAcM_GetName(actor)) {
    case fpcNm_ALINK_e:
    case fpcNm_ARROW_e:
        return true;
    default:
        return false;
    }
}

ColliderCacheEntry* find_collider_entry(fopAc_ac_c* actor, bool create) {
    ColliderCacheEntry* oldest = &s_colliderCache.front();

    for (ColliderCacheEntry& entry : s_colliderCache) {
        if (entry.actor == actor) {
            if (entry.actorId != actor->setID) {
                entry = {};
                entry.actor = actor;
                entry.actorId = actor->setID;
            }
            return &entry;
        }
        if (entry.actor == nullptr && create) {
            entry.actor = actor;
            entry.actorId = actor->setID;
            return &entry;
        }
        if (entry.lastSeenFrame < oldest->lastSeenFrame) {
            oldest = &entry;
        }
    }

    if (!create) {
        return nullptr;
    }

    *oldest = {};
    oldest->actor = actor;
    oldest->actorId = actor->setID;
    return oldest;
}

float get_flurry_target_collider_radius(fopAc_ac_c* actor) {
    ColliderCacheEntry* entry = find_collider_entry(actor, false);
    if (entry == nullptr) {
        return 0.0f;
    }

    float radius = 0.0f;
    for (std::size_t i = 0; i < entry->count; ++i) {
        cCcD_Obj* collider = entry->colliders[i];
        if (collider == nullptr || !collider->ChkTgSet()) {
            continue;
        }

        cCcD_ShapeAttr* shape = collider->GetShapeAttr();
        if (shape == nullptr) {
            continue;
        }

        cCcD_ShapeAttr::Shape access{};
        shape->getShapeAccess(&access);
        if ((access._0 == 0 || access._0 == 1) && std::isfinite(access._10) &&
            access._10 > radius)
        {
            radius = access._10;
        }
    }
    return radius;
}

void remember_collider(cCcD_Obj* collider) {
    if (!combat_slow_active() || collider == nullptr) {
        return;
    }

    fopAc_ac_c* actor = collider->GetAc();
    const bool cacheSlowedLink = actor != nullptr &&
                                  fopAcM_GetName(actor) == fpcNm_ALINK_e &&
                                  s_flurryRushActive && s_flurryLinkSlowed &&
                                  actor == s_flurryRushOwner;
    if (actor_is_exempt(actor) && !cacheSlowedLink) {
        return;
    }

    ColliderCacheEntry* entry = find_collider_entry(actor, true);
    entry->lastSeenFrame = s_slowFrame;
    for (std::size_t i = 0; i < entry->count; ++i) {
        if (entry->colliders[i] == collider) {
            return;
        }
    }
    if (entry->count < entry->colliders.size()) {
        entry->colliders[entry->count++] = collider;
    }
}

void prime_actor_colliders(fopAc_ac_c* actor) {
    ColliderCacheEntry* entry = find_collider_entry(actor, false);
    if (entry == nullptr) {
        return;
    }

    for (std::size_t i = 0; i < entry->count; ++i) {
        if (entry->colliders[i] != nullptr) {
            dComIfG_Ccsp()->Set(entry->colliders[i]);
        }
    }
}

void suppress_flurry_link_hits(cCcD_Obj* collider) {
    if (!s_flurryRushActive || s_flurryRushOwner == nullptr || collider == nullptr ||
        collider->GetAc() != s_flurryRushOwner)
    {
        return;
    }

    for (dCcD_Cyl& body : s_flurryRushOwner->mTgCyls) {
        if (collider == &body) {
            body.OffTgSetBit();
            body.ResetTgHit();
            return;
        }
    }

    if (collider == &s_flurryRushOwner->mAtSph) {
        s_flurryRushOwner->mAtSph.OffTgSetBit();
        s_flurryRushOwner->mAtSph.ResetTgHit();
    }
}

void disable_flurry_link_targets(daAlink_c* link) {
    if (link == nullptr) {
        return;
    }

    for (dCcD_Cyl& body : link->mTgCyls) {
        body.OffTgSetBit();
        body.ResetTgHit();
    }
    link->mAtSph.OffTgSetBit();
    link->mAtSph.ResetTgHit();
}

bool actor_has_hit_grace(fopAc_ac_c* actor) {
    for (const HitActorEntry& entry : s_hitActors) {
        if (entry.actor == actor && s_slowFrame - entry.frame <= kHitExecuteGraceFrames) {
            return true;
        }
    }
    return false;
}

void mark_actor_hit(fopAc_ac_c* actor) {
    if (!combat_slow_active() || actor_is_exempt(actor)) {
        return;
    }

    HitActorEntry* oldest = &s_hitActors.front();
    for (HitActorEntry& entry : s_hitActors) {
        if (entry.actor == actor || entry.actor == nullptr) {
            entry.actor = actor;
            entry.frame = s_slowFrame;
            return;
        }
        if (entry.frame < oldest->frame) {
            oldest = &entry;
        }
    }
    oldest->actor = actor;
    oldest->frame = s_slowFrame;
}

bool arrow_flight_was_initialized(daArrow_c* arrow) {
    if (arrow == nullptr) {
        return false;
    }

    if (arrow->checkWait()) {
        for (ArrowFlightEntry& entry : s_flyingArrows) {
            if (entry.arrow == arrow) {
                entry = {};
            }
        }
        return false;
    }

    ArrowFlightEntry* freeEntry = nullptr;
    for (ArrowFlightEntry& entry : s_flyingArrows) {
        if (entry.arrow == arrow) {
            if (entry.actorId == arrow->setID) {
                return true;
            }
            entry = {};
        }
        if (entry.arrow == nullptr && freeEntry == nullptr) {
            freeEntry = &entry;
        }
    }

    if (freeEntry == nullptr) {
        freeEntry = &s_flyingArrows.front();
    }
    *freeEntry = {.arrow = arrow, .actorId = arrow->setID};
    return false;
}

bool flurry_dodge_active(const daAlink_c* link) {
    return link != nullptr &&
           (link->mProcID == daAlink_c::PROC_SIDESTEP ||
               link->mProcID == daAlink_c::PROC_BACK_JUMP);
}

bool flurry_link_slow_active(const daAlink_c* link) {
    return s_flurryRushActive && s_flurryLinkSlowed &&
           s_flurryRushOwner == link && flurry_dodge_active(link);
}

bool actor_uses_visual_slowdown(fopAc_ac_c* actor) {
    if (!combat_slow_active() || actor == nullptr) {
        return false;
    }

    if (fopAcM_GetName(actor) == fpcNm_ALINK_e || enemy_uses_continuous_slow(actor)) {
        return false;
    }

    return !actor_is_exempt(actor) && !actor_has_hit_grace(actor);
}

float actor_time_scale(fopAc_ac_c* actor) {
    if (!combat_slow_active() || actor == nullptr) {
        return 1.0f;
    }

    switch (fopAcM_GetName(actor)) {
    case fpcNm_ARROW_e:
        return s_bulletTimeActive ? s_arrowSlowMotion.time_scale() : 1.0f;
    case fpcNm_ALINK_e:
        return 1.0f;
    default:
        return actor_is_exempt(actor) ? 1.0f : s_enemySlowMotion.time_scale();
    }
}

SlowActorClockEntry* find_actor_clock(fopAc_ac_c* actor, bool create,
                                      bool initiallyDue = true) {
    SlowActorClockEntry* oldest = &s_actorClocks.front();
    for (SlowActorClockEntry& entry : s_actorClocks) {
        if (entry.actor == actor) {
            if (entry.actorId != fopAcM_GetID(actor)) {
                entry = {};
                entry.actor = actor;
                entry.actorId = fopAcM_GetID(actor);
                entry.accumulator.reset(initiallyDue ? kSimulationPeriod : 0.0f);
                entry.pendingTicks = initiallyDue ? 1 : 0;
            }
            entry.lastSeenFrame = s_slowFrame;
            return &entry;
        }
        if (entry.actor == nullptr && create) {
            entry.actor = actor;
            entry.actorId = fopAcM_GetID(actor);
            entry.lastSeenFrame = s_slowFrame;
            entry.accumulator.reset(initiallyDue ? kSimulationPeriod : 0.0f);
            entry.pendingTicks = initiallyDue ? 1 : 0;
            return &entry;
        }
        if (entry.lastSeenFrame < oldest->lastSeenFrame) {
            oldest = &entry;
        }
    }

    if (!create) {
        return nullptr;
    }

    *oldest = {};
    oldest->actor = actor;
    oldest->actorId = fopAcM_GetID(actor);
    oldest->lastSeenFrame = s_slowFrame;
    oldest->accumulator.reset(initiallyDue ? kSimulationPeriod : 0.0f);
    oldest->pendingTicks = initiallyDue ? 1 : 0;
    return oldest;
}

void reset_actor_clock(fopAc_ac_c* actor, bool initiallyDue) {
    if (actor == nullptr) {
        return;
    }
    SlowActorClockEntry* entry = find_actor_clock(actor, true, initiallyDue);
    entry->accumulator.reset(initiallyDue ? kSimulationPeriod : 0.0f);
    entry->pendingTicks = initiallyDue ? 1 : 0;
}

bool consume_actor_tick(fopAc_ac_c* actor) {
    SlowActorClockEntry* entry = find_actor_clock(actor, true);
    if (entry->pendingTicks <= 0) {
        return false;
    }
    --entry->pendingTicks;
    entry->accumulator.commit();
    return true;
}

float actor_visual_progress(fopAc_ac_c* actor) {
    SlowActorClockEntry* entry = find_actor_clock(actor, false);
    return entry == nullptr ? 1.0f : entry->accumulator.interpolation();
}

void update_slow_motion_presentation() {
    const Clock::time_point now = Clock::now();
    float dt = 1.0f / 60.0f;
    if (s_lastPresentationSample != Clock::time_point{}) {
        dt = std::chrono::duration<float>(now - s_lastPresentationSample).count();
    }
    s_lastPresentationSample = now;
    dt = std::clamp(dt, 0.0f, 0.05f);

    s_enemySlowMotion.update(dt);
    s_arrowSlowMotion.update(dt);
    s_flurryLinkSlowMotion.update(dt);
    const float audioRate = combat_slow_active()
                                ? s_enemySlowMotion.audio_rate()
                                : 1.0f;
    s_audioRate.store(audioRate, std::memory_order_relaxed);
    update_link_voice_rates(audioRate);

    for (SlowActorClockEntry& entry : s_actorClocks) {
        if (entry.actor == nullptr) {
            continue;
        }
        if (fopAcM_SearchByID(entry.actorId) != entry.actor) {
            entry = {};
            continue;
        }

        const float scale = actor_time_scale(entry.actor);
        if (scale >= 0.999f) {
            entry = {};
            continue;
        }
        if (entry.pendingTicks == 0) {
            entry.pendingTicks = entry.accumulator.advance(dt, scale, 1);
        }
    }
}

HookAction before_draw_iterater(ModContext*, void*, void*, void*) {
    update_slow_motion_presentation();
    return HOOK_CONTINUE;
}

void draw_slow_motion_edges(ModContext*, const GfxStageContext*, void*) {
    view_class* view = dComIfGd_getView();
    if (view != nullptr) {
        drawSlowMotionEdges(view, s_enemySlowMotion.edge_strength());
    }
}

void read_matrix(MtxP matrix, MatrixPose& pose) {
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            pose[row * 4 + column] = matrix[row][column];
        }
    }
}

void write_matrix(const MatrixPose& pose, MtxP matrix) {
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            matrix[row][column] = pose[row * 4 + column];
        }
    }
}

void write_interpolated_matrix(const MatrixPose& start, const MatrixPose& target,
                               float progress, MtxP matrix) {
    MatrixPose pose{};
    for (std::size_t i = 0; i < pose.size(); ++i) {
        pose[i] = start[i] + (target[i] - start[i]) * progress;
    }
    write_matrix(pose, matrix);
}

float vector_length(float x, float y, float z) {
    return std::sqrt(x * x + y * y + z * z);
}

bool decompose_transform(const MatrixPose& pose, TransformPose& transform) {
    constexpr float kMinimumScale = 0.0001f;
    constexpr float kMaximumAxisDot = 0.001f;

    float scaleX = vector_length(pose[0], pose[4], pose[8]);
    const float scaleY = vector_length(pose[1], pose[5], pose[9]);
    const float scaleZ = vector_length(pose[2], pose[6], pose[10]);
    if (scaleX < kMinimumScale || scaleY < kMinimumScale ||
        scaleZ < kMinimumScale)
    {
        return false;
    }

    float r00 = pose[0] / scaleX;
    float r10 = pose[4] / scaleX;
    float r20 = pose[8] / scaleX;
    const float r01 = pose[1] / scaleY;
    const float r11 = pose[5] / scaleY;
    const float r21 = pose[9] / scaleY;
    const float r02 = pose[2] / scaleZ;
    const float r12 = pose[6] / scaleZ;
    const float r22 = pose[10] / scaleZ;

    if (std::abs(r00 * r01 + r10 * r11 + r20 * r21) > kMaximumAxisDot ||
        std::abs(r00 * r02 + r10 * r12 + r20 * r22) > kMaximumAxisDot ||
        std::abs(r01 * r02 + r11 * r12 + r21 * r22) > kMaximumAxisDot)
    {
        return false;
    }

    const float determinant =
        r00 * (r11 * r22 - r12 * r21) -
        r01 * (r10 * r22 - r12 * r20) +
        r02 * (r10 * r21 - r11 * r20);
    if (determinant < 0.0f) {
        scaleX = -scaleX;
        r00 = -r00;
        r10 = -r10;
        r20 = -r20;
    }

    QuaternionPose rotation;
    const float trace = r00 + r11 + r22;
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        rotation.w = 0.25f * s;
        rotation.x = (r21 - r12) / s;
        rotation.y = (r02 - r20) / s;
        rotation.z = (r10 - r01) / s;
    } else if (r00 > r11 && r00 > r22) {
        const float s = std::sqrt(1.0f + r00 - r11 - r22) * 2.0f;
        rotation.w = (r21 - r12) / s;
        rotation.x = 0.25f * s;
        rotation.y = (r01 + r10) / s;
        rotation.z = (r02 + r20) / s;
    } else if (r11 > r22) {
        const float s = std::sqrt(1.0f + r11 - r00 - r22) * 2.0f;
        rotation.w = (r02 - r20) / s;
        rotation.x = (r01 + r10) / s;
        rotation.y = 0.25f * s;
        rotation.z = (r12 + r21) / s;
    } else {
        const float s = std::sqrt(1.0f + r22 - r00 - r11) * 2.0f;
        rotation.w = (r10 - r01) / s;
        rotation.x = (r02 + r20) / s;
        rotation.y = (r12 + r21) / s;
        rotation.z = 0.25f * s;
    }

    const float rotationLength = vector_length(
        rotation.x, rotation.y, rotation.z);
    const float quaternionLength =
        std::sqrt(rotationLength * rotationLength + rotation.w * rotation.w);
    if (quaternionLength < kMinimumScale) {
        return false;
    }
    rotation.x /= quaternionLength;
    rotation.y /= quaternionLength;
    rotation.z /= quaternionLength;
    rotation.w /= quaternionLength;

    transform.translation = {pose[3], pose[7], pose[11]};
    transform.scale = {scaleX, scaleY, scaleZ};
    transform.rotation = rotation;
    return true;
}

QuaternionPose interpolate_rotation(
    const QuaternionPose& start, const QuaternionPose& target, float progress)
{
    QuaternionPose end = target;
    const float dot = start.x * end.x + start.y * end.y +
                      start.z * end.z + start.w * end.w;
    if (dot < 0.0f) {
        end.x = -end.x;
        end.y = -end.y;
        end.z = -end.z;
        end.w = -end.w;
    }

    QuaternionPose result{
        start.x + (end.x - start.x) * progress,
        start.y + (end.y - start.y) * progress,
        start.z + (end.z - start.z) * progress,
        start.w + (end.w - start.w) * progress,
    };
    const float length = std::sqrt(result.x * result.x + result.y * result.y +
                                   result.z * result.z + result.w * result.w);
    if (length > 0.0001f) {
        result.x /= length;
        result.y /= length;
        result.z /= length;
        result.w /= length;
    }
    return result;
}

float interpolate_root_translation(float start, float target, float startTangent,
                                   float progress) {
    const float progress2 = progress * progress;
    const float progress3 = progress2 * progress;
    const float endTangent = target - start;
    const float value = (2.0f * progress3 - 3.0f * progress2 + 1.0f) * start +
                        (progress3 - 2.0f * progress2 + progress) * startTangent +
                        (-2.0f * progress3 + 3.0f * progress2) * target +
                        (progress3 - progress2) * endTangent;
    return std::clamp(value, std::min(start, target), std::max(start, target));
}

void compose_transform(const TransformPose& transform, MatrixPose& pose) {
    const QuaternionPose& q = transform.rotation;
    const float xx = q.x * q.x;
    const float yy = q.y * q.y;
    const float zz = q.z * q.z;
    const float xy = q.x * q.y;
    const float xz = q.x * q.z;
    const float yz = q.y * q.z;
    const float xw = q.x * q.w;
    const float yw = q.y * q.w;
    const float zw = q.z * q.w;

    pose[0] = (1.0f - 2.0f * (yy + zz)) * transform.scale[0];
    pose[4] = (2.0f * (xy + zw)) * transform.scale[0];
    pose[8] = (2.0f * (xz - yw)) * transform.scale[0];
    pose[1] = (2.0f * (xy - zw)) * transform.scale[1];
    pose[5] = (1.0f - 2.0f * (xx + zz)) * transform.scale[1];
    pose[9] = (2.0f * (yz + xw)) * transform.scale[1];
    pose[2] = (2.0f * (xz + yw)) * transform.scale[2];
    pose[6] = (2.0f * (yz - xw)) * transform.scale[2];
    pose[10] = (1.0f - 2.0f * (xx + yy)) * transform.scale[2];
    pose[3] = transform.translation[0];
    pose[7] = transform.translation[1];
    pose[11] = transform.translation[2];
}

void write_interpolated_transform(const MatrixPose& start, const MatrixPose& target,
                                  float progress, MtxP matrix,
                                  const VectorPose* rootStartTangent = nullptr) {
    TransformPose startTransform;
    TransformPose targetTransform;
    if (!decompose_transform(start, startTransform) ||
        !decompose_transform(target, targetTransform))
    {
        write_interpolated_matrix(start, target, progress, matrix);
        return;
    }

    TransformPose result;
    for (std::size_t i = 0; i < result.translation.size(); ++i) {
        result.translation[i] = rootStartTangent == nullptr ?
            startTransform.translation[i] +
                (targetTransform.translation[i] - startTransform.translation[i]) * progress :
            interpolate_root_translation(startTransform.translation[i],
                targetTransform.translation[i], (*rootStartTangent)[i], progress);
        result.scale[i] = startTransform.scale[i] +
                          (targetTransform.scale[i] - startTransform.scale[i]) * progress;
    }
    result.rotation = interpolate_rotation(
        startTransform.rotation, targetTransform.rotation, progress);

    MatrixPose pose{};
    compose_transform(result, pose);
    write_matrix(pose, matrix);
}

ModelVisualState* find_model_visual_state(J3DModel* model, bool create,
                                          fopAc_ac_c* actor = nullptr) {
    ModelVisualState* oldest = &s_modelVisualStates.front();
    for (ModelVisualState& state : s_modelVisualStates) {
        if (state.model == model) {
            if (actor != nullptr &&
                (state.actor != actor || state.actorId != fopAcM_GetID(actor)))
            {
                state = {};
                state.model = model;
                state.actor = actor;
                state.actorId = fopAcM_GetID(actor);
            }
            return &state;
        }
        if (state.model == nullptr && create) {
            state.model = model;
            state.actor = actor;
            state.actorId = actor == nullptr ? fpcM_ERROR_PROCESS_ID_e
                                             : fopAcM_GetID(actor);
            return &state;
        }
        if (state.lastSeenFrame < oldest->lastSeenFrame) {
            oldest = &state;
        }
    }

    if (!create) {
        return nullptr;
    }

    *oldest = {};
    oldest->model = model;
    oldest->actor = actor;
    oldest->actorId = actor == nullptr ? fpcM_ERROR_PROCESS_ID_e
                                       : fopAcM_GetID(actor);
    return oldest;
}

bool prepare_model_pose_buffers(ModelVisualState& state) {
    J3DModelData* modelData = state.model == nullptr
                                  ? nullptr
                                  : state.model->getModelData();
    if (modelData == nullptr) {
        return false;
    }

    const std::size_t jointCount = modelData->getJointNum();
    const std::size_t weightCount = modelData->getWEvlpMtxNum();
    state.startJoints.resize(jointCount);
    state.targetJoints.resize(jointCount);
    state.backupJoints.resize(jointCount);
    state.startWeights.resize(weightCount);
    state.targetWeights.resize(weightCount);
    state.backupWeights.resize(weightCount);
    return true;
}

void read_model_pose(J3DModel* model, std::vector<MatrixPose>& joints,
                     std::vector<MatrixPose>& weights) {
    for (std::size_t i = 0; i < joints.size(); ++i) {
        read_matrix(model->getAnmMtx(static_cast<int>(i)), joints[i]);
    }
    for (std::size_t i = 0; i < weights.size(); ++i) {
        read_matrix(model->getWeightAnmMtx(static_cast<int>(i)), weights[i]);
    }
}

void write_interpolated_model_pose(ModelVisualState& state, float progress) {
    read_matrix(state.model->getBaseTRMtx(), state.backupBase);
    read_model_pose(state.model, state.backupJoints, state.backupWeights);
    write_interpolated_transform(state.startBase, state.targetBase, progress,
                                 state.model->getBaseTRMtx(),
                                 &state.rootStartTangent);
    for (std::size_t i = 0; i < state.startJoints.size(); ++i) {
        write_interpolated_transform(state.startJoints[i], state.targetJoints[i],
                                     progress,
                                     state.model->getAnmMtx(static_cast<int>(i)));
    }
    for (std::size_t i = 0; i < state.startWeights.size(); ++i) {
        write_interpolated_matrix(state.startWeights[i], state.targetWeights[i],
                                  progress,
                                  state.model->getWeightAnmMtx(static_cast<int>(i)));
    }
    state.viewApplied = true;
}

void restore_model_pose(ModelVisualState& state) {
    write_matrix(state.backupBase, state.model->getBaseTRMtx());
    for (std::size_t i = 0; i < state.backupJoints.size(); ++i) {
        write_matrix(state.backupJoints[i],
                     state.model->getAnmMtx(static_cast<int>(i)));
    }
    for (std::size_t i = 0; i < state.backupWeights.size(); ++i) {
        write_matrix(state.backupWeights[i],
                     state.model->getWeightAnmMtx(static_cast<int>(i)));
    }
    state.viewApplied = false;
}

bool should_skip_actor(fopAc_ac_c* actor) {
    if (!combat_slow_active() || actor == nullptr) {
        return false;
    }

    if (fopAcM_GetName(actor) == fpcNm_ARROW_e) {
        if (!s_bulletTimeActive) {
            return false;
        }
        if (!arrow_flight_was_initialized(static_cast<daArrow_c*>(actor))) {
            reset_actor_clock(actor, false);
            return false;
        }
        return !consume_actor_tick(actor);
    }

    if (fopAcM_GetName(actor) == fpcNm_ALINK_e || enemy_uses_continuous_slow(actor)) {
        return false;
    }

    return !actor_is_exempt(actor) && !actor_has_hit_grace(actor) &&
           !consume_actor_tick(actor);
}

void update_dodge_attempt(daAlink_c* link) {
    if (!flurry_dodge_active(link)) {
        s_dodgeOwner = nullptr;
        s_dodgeProc = daAlink_c::PROC_WAIT;
        s_dodgeTriggered = false;
        return;
    }

    if (s_dodgeOwner != link || s_dodgeProc != link->mProcID) {
        s_dodgeOwner = link;
        s_dodgeProc = link->mProcID;
        s_dodgeTriggered = false;
    }
}

float axis_gap(float minA, float maxA, float minB, float maxB) {
    if (maxA < minB) {
        return minB - maxA;
    }
    if (maxB < minA) {
        return minA - maxB;
    }
    return 0.0f;
}

bool collider_near_link(cCcD_Obj* attack, daAlink_c* link) {
    cCcD_ShapeAttr* attackShape = attack->GetShapeAttr();
    if (attackShape == nullptr) {
        return false;
    }
    attackShape->CalcAabBox();
    const cM3dGAab& attackBounds = attackShape->GetWorkAab();
    const float marginSquared = kPerfectDodgeMargin * kPerfectDodgeMargin;

    for (dCcD_Cyl& body : link->mTgCyls) {
        if (!body.ChkTgSet() || (attack->GetAtGrp() & body.GetTgGrp()) == 0 ||
            (attack->GetAtType() & body.GetTgType()) == 0)
        {
            continue;
        }

        cCcD_ShapeAttr* bodyShape = body.GetShapeAttr();
        if (bodyShape == nullptr) {
            continue;
        }
        bodyShape->CalcAabBox();
        const cM3dGAab& bodyBounds = bodyShape->GetWorkAab();
        const float x = axis_gap(attackBounds.GetMinX(), attackBounds.GetMaxX(),
                                 bodyBounds.GetMinX(), bodyBounds.GetMaxX());
        const float y = axis_gap(attackBounds.GetMinY(), attackBounds.GetMaxY(),
                                 bodyBounds.GetMinY(), bodyBounds.GetMaxY());
        const float z = axis_gap(attackBounds.GetMinZ(), attackBounds.GetMaxZ(),
                                 bodyBounds.GetMinZ(), bodyBounds.GetMaxZ());
        if (x * x + y * y + z * z <= marginSquared) {
            return true;
        }
    }
    return false;
}

void try_start_flurry_rush(cCcD_Obj* attack) {
    if (attack == nullptr || !attack->ChkAtSet() || attack->GetAtType() == AT_TYPE_0 ||
        s_bulletTimeActive || s_flurryRushActive || !flurry_rush_enabled())
    {
        return;
    }

    daAlink_c* link = daAlink_getAlinkActorClass();
    update_dodge_attempt(link);
    if (!flurry_dodge_active(link) || s_dodgeTriggered || !link->checkAttentionLock()) {
        return;
    }

    fopAc_ac_c* target = link->mTargetedActor;
    fopAc_ac_c* attacker = attack->GetAc();
    if (target == nullptr || attacker == nullptr || attacker == link ||
        !daAlink_c::checkEnemyGroup(target) ||
        (attacker != target && !daAlink_c::checkEnemyGroup(attacker)) ||
        !collider_near_link(attack, link))
    {
        return;
    }
    if (!consume_flurry_rush_stamina()) {
        s_dodgeTriggered = true;
        return;
    }

    clear_combat_time_caches();
    s_flurryRushOwner = link;
    s_flurryRushTarget = target;
    s_flurryRushStarted = Clock::now();
    s_flurryRushActive = true;
    s_flurryLinkSlowed = true;
    s_flurryMeleePositioned = false;
    s_dodgeTriggered = true;
    s_flurrySwordAttackSerial = 0;
    s_flurryLastSwordProc = link->mProcID;
    s_flurryLastCutCount = link->getCutCount();
    s_flurrySwordAttackWasActive = false;
    clear_deferred_flurry_damage();
    disable_flurry_link_targets(link);
    sync_slow_motion_controllers();
}

bool sword_attack_active(const daAlink_c* link) {
    if (link == nullptr || link->mEquipItem != 0x103) {
        return false;
    }

    switch (link->mProcID) {
    case daAlink_c::PROC_CUT_NORMAL:
    case daAlink_c::PROC_CUT_FINISH:
    case daAlink_c::PROC_CUT_FINISH_JUMP_UP:
    case daAlink_c::PROC_CUT_REVERSE:
    case daAlink_c::PROC_CUT_JUMP:
    case daAlink_c::PROC_CUT_TURN:
    case daAlink_c::PROC_CUT_DOWN:
    case daAlink_c::PROC_CUT_HEAD:
    case daAlink_c::PROC_CUT_LARGE_JUMP:
        return true;
    default:
        return false;
    }
}

void track_flurry_sword_attack(daAlink_c* link) {
    if (!s_flurryRushActive || link == nullptr || link != s_flurryRushOwner) {
        return;
    }

    const bool attackActive = sword_attack_active(link);
    const u8 cutCount = link->getCutCount();
    const u16 swordProc = link->mProcID;
    const bool newAttack = attackActive &&
                           (!s_flurrySwordAttackWasActive ||
                               cutCount != s_flurryLastCutCount ||
                               swordProc != s_flurryLastSwordProc);
    if (newAttack) {
        const int attackPower = swordProc == daAlink_c::PROC_CUT_TURN
                                    ? link->mAtSph.GetAtAtp()
                                    : link->mAtCps[0].GetAtAtp();
        if (attackPower > 0) {
            ++s_flurrySwordAttackSerial;
            if (s_flurrySwordAttackSerial == 0) {
                ++s_flurrySwordAttackSerial;
            }
        }
    }

    s_flurrySwordAttackWasActive = attackActive;
    s_flurryLastCutCount = cutCount;
    s_flurryLastSwordProc = swordProc;
}

void move_link_to_flurry_target(daAlink_c* link) {
    if (link == nullptr || link->mTargetedActor == nullptr ||
        link->mTargetedActor != s_flurryRushTarget)
    {
        return;
    }

    const cXyz targetPosition = s_flurryRushTarget->current.pos;
    const float deltaX = link->current.pos.x - targetPosition.x;
    const float deltaZ = link->current.pos.z - targetPosition.z;
    const float meleeDistance =
        kFlurryRushMeleeDistance + get_flurry_target_collider_radius(s_flurryRushTarget);
    const float distance = cXyz(deltaX, 0.0f, deltaZ).abs();
    if (distance <= meleeDistance || distance < 0.001f) {
        return;
    }

    const float scale = meleeDistance / distance;
    cXyz destination(
        targetPosition.x + deltaX * scale,
        link->current.pos.y,
        targetPosition.z + deltaZ * scale);
    link->current.pos = destination;
    link->old.pos = destination;
    link->mCcStts.ClrCcMove();
}

void update_flurry_link_attack(fopAc_ac_c* actor) {
    if (!s_flurryRushActive || s_flurryMeleePositioned ||
        actor != s_flurryRushOwner)
    {
        return;
    }

    auto* link = static_cast<daAlink_c*>(actor);
    if (!sword_attack_active(link)) {
        return;
    }

    if (s_flurryLinkSlowed) {
        s_flurryLinkSlowed = false;
        s_flurryRushStarted = Clock::now();
        sync_slow_motion_controllers();
    }
    s_flurryMeleePositioned = true;
    move_link_to_flurry_target(link);
}

void update_flurry_link_landing(fopAc_ac_c* actor) {
    if (!s_flurryRushActive || !s_flurryLinkSlowed || actor != s_flurryRushOwner) {
        return;
    }

    auto* link = static_cast<daAlink_c*>(actor);
    if (link->mProcID != daAlink_c::PROC_BACK_JUMP_LAND &&
        link->mProcID != daAlink_c::PROC_SIDESTEP_LAND)
    {
        return;
    }

    s_flurryLinkSlowed = false;
    s_flurryRushStarted = Clock::now();
    sync_slow_motion_controllers();
}

void apply_bullet_time_fall(daAlink_c* link) {
    if (!s_bulletTimeActive || link == nullptr) {
        return;
    }

    link->setSpecialGravity(s_previousGravity, s_previousMaxFallSpeed, FALSE);
    if (link->speed.y < s_previousMaxFallSpeed) {
        link->speed.y = s_previousMaxFallSpeed;
    }
}

HookAction before_link_pos_move(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    s_linkPositionStep = {};
    float scale = 1.0f;
    if (bullet_time_active_for(link)) {
        scale = kLinkTimeScale;
    } else if (flurry_link_slow_active(link)) {
        scale = s_flurryLinkSlowMotion.time_scale();
    }
    if (scale >= 0.999f) {
        return HOOK_CONTINUE;
    }

    s_linkPositionStep = {
        .link = link,
        .startPosition = link->current.pos,
        .gravity = link->gravity,
        .scale = scale,
        .active = true,
    };
    link->gravity *= scale;
    return HOOK_CONTINUE;
}

void after_link_pos_move(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!s_linkPositionStep.active || s_linkPositionStep.link != link) {
        return;
    }

    link->current.pos.x = s_linkPositionStep.startPosition.x +
                          (link->current.pos.x - s_linkPositionStep.startPosition.x) *
                              s_linkPositionStep.scale;
    link->current.pos.y = s_linkPositionStep.startPosition.y +
                          (link->current.pos.y - s_linkPositionStep.startPosition.y) *
                              s_linkPositionStep.scale;
    link->current.pos.z = s_linkPositionStep.startPosition.z +
                          (link->current.pos.z - s_linkPositionStep.startPosition.z) *
                              s_linkPositionStep.scale;
    link->gravity = s_linkPositionStep.gravity;
    s_linkPositionStep = {};
}

HookAction before_link_all_anime_play(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    s_linkAnimationRateStep = {};
    if (!flurry_link_slow_active(link)) {
        return HOOK_CONTINUE;
    }

    const float scale = s_flurryLinkSlowMotion.time_scale();
    if (scale >= 0.999f) {
        return HOOK_CONTINUE;
    }

    s_linkAnimationRateStep.link = link;
    s_linkAnimationRateStep.active = true;
    for (std::size_t i = 0; i < 3; ++i) {
        s_linkAnimationRateStep.underRates[i] = link->mUnderFrameCtrl[i].getRate();
        s_linkAnimationRateStep.upperRates[i] = link->mUpperFrameCtrl[i].getRate();
        link->mUnderFrameCtrl[i].setRate(
            s_linkAnimationRateStep.underRates[i] * scale);
        link->mUpperFrameCtrl[i].setRate(
            s_linkAnimationRateStep.upperRates[i] * scale);
    }
    return HOOK_CONTINUE;
}

void after_link_all_anime_play(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!s_linkAnimationRateStep.active || s_linkAnimationRateStep.link != link) {
        return;
    }

    for (std::size_t i = 0; i < 3; ++i) {
        if (link->mUnderFrameCtrl[i].getRate() != 0.0f) {
            link->mUnderFrameCtrl[i].setRate(s_linkAnimationRateStep.underRates[i]);
        }
        if (link->mUpperFrameCtrl[i].getRate() != 0.0f) {
            link->mUpperFrameCtrl[i].setRate(s_linkAnimationRateStep.upperRates[i]);
        }
    }
    s_linkAnimationRateStep = {};
}

HookAction before_actor_execute(ModContext*, void* args, void*, void*) {
    if (s_actorExecuteDepth < s_actorExecuteStack.size()) {
        s_actorExecuteStack[s_actorExecuteDepth++] =
            static_cast<fopAc_ac_c*>(mods::arg<void*>(args, 0));
    }
    return HOOK_CONTINUE;
}

void after_actor_execute(ModContext*, void* args, void*, void*) {
    if (s_actorExecuteDepth == 0) {
        return;
    }

    auto* actor = static_cast<fopAc_ac_c*>(mods::arg<void*>(args, 0));
    if (actor == s_flurryRushOwner) {
        track_flurry_sword_attack(static_cast<daAlink_c*>(actor));
    }
    update_flurry_link_attack(actor);
    update_flurry_link_landing(actor);
    if (s_flurryRushActive && actor == s_flurryRushOwner) {
        dComIfGp_setAStatus(kFlurryRushActionStatus, BUTTON_STATUS_FLAG_EMPHASIS);
    }
    if (s_actorExecuteStack[s_actorExecuteDepth - 1] == actor) {
        s_actorExecuteStack[--s_actorExecuteDepth] = nullptr;
    }
}

HookAction before_combat_model_calc(ModContext*, void* args, void*, void*) {
    if (s_actorExecuteDepth == 0) {
        return HOOK_CONTINUE;
    }

    auto* model = mods::arg<J3DModel*>(args, 0);
    fopAc_ac_c* actor = s_actorExecuteStack[s_actorExecuteDepth - 1];
    if (model == nullptr || actor == nullptr) {
        return HOOK_CONTINUE;
    }

    if (!actor_uses_visual_slowdown(actor)) {
        if (ModelVisualState* state = find_model_visual_state(model, false);
            state != nullptr)
        {
            *state = {};
        }
        return HOOK_CONTINUE;
    }

    ModelVisualState* state = find_model_visual_state(model, true, actor);
    if (!prepare_model_pose_buffers(*state)) {
        *state = {};
        return HOOK_CONTINUE;
    }

    if (!state->initialized) {
        read_matrix(model->getBaseTRMtx(), state->startBase);
        state->targetBase = state->startBase;
        read_model_pose(model, state->startJoints, state->startWeights);
        state->targetJoints = state->startJoints;
        state->targetWeights = state->startWeights;
        state->initialized = true;
    } else if (state->captureFrame != s_slowFrame) {
        state->rootStartTangent = {
            state->targetBase[3] - state->startBase[3],
            state->targetBase[7] - state->startBase[7],
            state->targetBase[11] - state->startBase[11],
        };
        state->startBase = state->targetBase;
        state->startJoints = state->targetJoints;
        state->startWeights = state->targetWeights;
    }

    read_matrix(model->getBaseTRMtx(), state->targetBase);
    state->captureFrame = s_slowFrame;
    state->lastSeenFrame = s_slowFrame;
    return HOOK_CONTINUE;
}

void after_combat_model_calc(ModContext*, void* args, void*, void*) {
    if (s_actorExecuteDepth == 0) {
        return;
    }

    auto* model = mods::arg<J3DModel*>(args, 0);
    fopAc_ac_c* actor = s_actorExecuteStack[s_actorExecuteDepth - 1];
    ModelVisualState* state = find_model_visual_state(model, false);
    if (state == nullptr || !state->initialized || state->actor != actor ||
        !actor_uses_visual_slowdown(actor))
    {
        return;
    }

    read_matrix(model->getBaseTRMtx(), state->targetBase);
    read_model_pose(model, state->targetJoints, state->targetWeights);
    state->targetFrame = s_slowFrame;
    state->lastSeenFrame = s_slowFrame;
}

HookAction before_combat_model_view_calc(ModContext*, void* args, void*, void*) {
    auto* model = mods::arg<J3DModel*>(args, 0);
    ModelVisualState* state = find_model_visual_state(model, false);
    if (state == nullptr || !state->initialized || state->viewApplied) {
        return HOOK_CONTINUE;
    }

    if (state->actor == nullptr ||
        fopAcM_SearchByID(state->actorId) != state->actor ||
        !actor_uses_visual_slowdown(state->actor))
    {
        *state = {};
        return HOOK_CONTINUE;
    }

    J3DModelData* modelData = model->getModelData();
    if (modelData == nullptr ||
        state->startJoints.size() != modelData->getJointNum() ||
        state->startWeights.size() != modelData->getWEvlpMtxNum())
    {
        *state = {};
        return HOOK_CONTINUE;
    }

    const float progress = actor_visual_progress(state->actor);
    write_interpolated_model_pose(*state, progress);
    return HOOK_CONTINUE;
}

void after_combat_model_view_calc(ModContext*, void* args, void*, void*) {
    auto* model = mods::arg<J3DModel*>(args, 0);
    ModelVisualState* state = find_model_visual_state(model, false);
    if (state != nullptr && state->viewApplied) {
        restore_model_pose(*state);
    }
}

HookAction before_process_execute(ModContext*, void* args, void* retval, void*) {
    if (s_actorExecuteDepth == 0) {
        return HOOK_CONTINUE;
    }

    auto* actor = s_actorExecuteStack[s_actorExecuteDepth - 1];
    if (mods::arg<void*>(args, 1) != actor || !should_skip_actor(actor)) {
        return HOOK_CONTINUE;
    }

    prime_actor_colliders(actor);
    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

#if defined(__APPLE__)
HookAction before_process_method(ModContext*, void* args, void* retval, void*) {
    if (s_actorExecuteDepth == 0) {
        return HOOK_CONTINUE;
    }

    auto* actor = s_actorExecuteStack[s_actorExecuteDepth - 1];
    const auto method = mods::arg<process_method_func>(args, 0);
    const auto* methods = actor == nullptr
                              ? nullptr
                              : reinterpret_cast<const process_method_class*>(actor->sub_method);
    if (actor == nullptr || actor->sub_method == nullptr ||
        mods::arg<void*>(args, 1) != actor ||
        method != methods->execute_method ||
        !should_skip_actor(actor)) {
        return HOOK_CONTINUE;
    }

    prime_actor_colliders(actor);
    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}
#endif

HookAction before_collider_set(ModContext*, void* args, void*, void*) {
    cCcD_Obj* collider = mods::arg<cCcD_Obj*>(args, 1);
    try_start_flurry_rush(collider);
    suppress_flurry_link_hits(collider);
    remember_collider(collider);
    return HOOK_CONTINUE;
}

HookAction before_link_damage_action(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!s_flurryRushActive || link == nullptr || link != s_flurryRushOwner) {
        return HOOK_CONTINUE;
    }

    disable_flurry_link_targets(link);
    *static_cast<BOOL*>(retval) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_arrow_hit(ModContext*, void* args, void*, void*) {
    mark_actor_hit(mods::arg<fopAc_ac_c*>(args, 2));
    return HOOK_CONTINUE;
}

HookAction before_at_check(ModContext*, void* args, void*, void*) {
    auto* enemy = mods::arg<fopAc_ac_c*>(args, 0);
    auto* atInfo = mods::arg<dCcU_AtInfo*>(args, 1);
    s_flurryDamageCheckActive = atInfo != nullptr &&
                                enemy == s_heldFlurryTarget &&
                                atInfo->mpCollider == s_heldFlurryPowerCollider &&
                                s_heldFlurryAttackCount > 0;
    s_flurryDamageScaled = false;
    return HOOK_CONTINUE;
}

void after_flurry_attack_power(ModContext*, void* args, void*, void*) {
    auto* atInfo = mods::arg<dCcU_AtInfo*>(args, 0);
    if (!s_flurryDamageCheckActive || s_flurryDamageScaled || atInfo == nullptr ||
        atInfo->mpCollider != s_heldFlurryPowerCollider || atInfo->mAttackPower == 0)
    {
        return;
    }

    const u16 baseDamage = atInfo->mAttackPower;
    atInfo->mAttackPower = static_cast<u16>(std::min<std::uint32_t>(
        static_cast<std::uint32_t>(baseDamage) * s_heldFlurryAttackCount,
        0xFFFFU));
    s_flurryDamageScaled = true;
}

void after_at_check(ModContext*, void*, void*, void*) {
    if (s_flurryDamageCheckActive) {
        clear_held_flurry_damage();
    }
    s_flurryDamageCheckActive = false;
    s_flurryDamageScaled = false;
}

bool is_flurry_sword_collider(cCcD_Obj* collider) {
    if (s_flurryRushOwner == nullptr || collider == nullptr) {
        return false;
    }

    for (dCcD_Cps& sword : s_flurryRushOwner->mAtCps) {
        if (collider == &sword) {
            return true;
        }
    }
    return collider == &s_flurryRushOwner->mAtSph;
}

void preserve_flurry_attack_hit(cCcD_Obj* attack, cCcD_Obj* target,
                                cXyz* hitPosition) {
    attack->SetAtHit(target);
    auto* attackInfo = static_cast<dCcD_GObjInf*>(attack->GetGObjInf());
    cCcD_Stts* targetStatus = target->GetStts();
    if (attackInfo == nullptr || targetStatus == nullptr) {
        return;
    }

    attackInfo->SetAtHitApid(targetStatus->GetApid());
    attackInfo->SetAtHitPos(*hitPosition);
}

HookAction before_common_at_tg_hit(ModContext*, void* args, void*, void*) {
    if (!s_flurryRushActive || s_flurrySwordAttackSerial == 0) {
        return HOOK_CONTINUE;
    }

    auto* attack = mods::arg<cCcD_Obj*>(args, 1);
    auto* target = mods::arg<cCcD_Obj*>(args, 2);
    if (attack == nullptr || target == nullptr ||
        attack->GetAc() != s_flurryRushOwner ||
        !is_flurry_sword_collider(attack))
    {
        return HOOK_CONTINUE;
    }

    fopAc_ac_c* targetActor = target->GetAc();
    cXyz* hitPosition = mods::arg<cXyz*>(args, 3);
    const u8 damage = attack->GetAtAtp();
    if (targetActor == nullptr || targetActor != s_flurryRushTarget ||
        !daAlink_c::checkEnemyGroup(targetActor) || hitPosition == nullptr || damage == 0)
    {
        return HOOK_CONTINUE;
    }

    preserve_flurry_attack_hit(attack, target, hitPosition);
    if (s_deferredFlurryDamage.pending &&
        s_deferredFlurryDamage.lastAttackSerial == s_flurrySwordAttackSerial)
    {
        return HOOK_SKIP_ORIGINAL;
    }

    s_deferredFlurryDamage.targetActor = targetActor;
    s_deferredFlurryDamage.targetActorId = fopAcM_GetID(targetActor);
    s_deferredFlurryDamage.attackCollider = attack;
    s_deferredFlurryDamage.targetCollider = target;
    s_deferredFlurryDamage.hitPosition = *hitPosition;
    s_deferredFlurryDamage.lastAttackSerial = s_flurrySwordAttackSerial;
    s_deferredFlurryDamage.setAttackHit = true;
    s_deferredFlurryDamage.pending = true;

    return HOOK_SKIP_ORIGINAL;
}

bool manual_jump_is_airborne(daAlink_c* link) {
    return link != nullptr && link->mProcID == daAlink_c::PROC_AUTO_JUMP &&
           !link->mLinkAcch.ChkGroundHit();
}

bool selected_bow_button_requested(daAlink_c* link) {
    if (link == nullptr) {
        return false;
    }

    const u32 buttons = static_cast<u32>(link->mItemTrigger) |
                        static_cast<u32>(link->mItemButton);
    constexpr std::array<u8, 3> slots = {
        SELECT_ITEM_X,
        SELECT_ITEM_Y,
        SELECT_ITEM_DOWN,
    };
    for (const u8 slot : slots) {
        if ((buttons & (1u << slot)) != 0 &&
            daPy_py_c::checkBowItem(dComIfGp_getSelectItem(slot)))
        {
            return true;
        }
    }
    return false;
}

bool bow_aim_requested(daAlink_c* link) {
    return link != nullptr && daPy_py_c::checkBowItem(link->mEquipItem) &&
           link->checkReadyItem() &&
           (selected_bow_button_requested(link) || link->itemTrigger() || link->itemButton());
}

void equip_selected_bow_for_bullet_time(daAlink_c* link) {
    if (link == nullptr || daPy_py_c::checkBowItem(link->mEquipItem)) {
        return;
    }

    const u32 buttons = static_cast<u32>(link->mItemTrigger) |
                        static_cast<u32>(link->mItemButton);
    constexpr std::array<u8, 3> slots = {
        SELECT_ITEM_X,
        SELECT_ITEM_Y,
        SELECT_ITEM_DOWN,
    };
    for (const u8 slot : slots) {
        if ((buttons & (1u << slot)) == 0) {
            continue;
        }
        const u16 item = dComIfGp_getSelectItem(slot);
        if (!daPy_py_c::checkBowItem(item)) {
            continue;
        }
        link->mSelectItemId = slot;
        link->itemEquip(item);
        return;
    }
}

void prepare_bow_aim(daAlink_c* link) {
    if (link == nullptr || !daPy_py_c::checkBowItem(link->mEquipItem) ||
        !link->checkReadyItem())
    {
        return;
    }

    if (!link->checkBowAnime()) {
        prepare_bullet_time_bow_aim(link);
        link->setBowReadyAnime();
        link->mItemMode = 0;
    }
    link->setBowOrSlingStatus();
}

}  // namespace

void apply_bullet_time_gyro(daAlink_c* link) {
    apply_bullet_time_gyro_impl(link);
}

float enemy_slow_motion_scale(fopAc_ac_c* actor) {
    return combat_slow_active() && !actor_has_hit_grace(actor)
        ? s_enemySlowMotion.time_scale() : 1.0f;
}

ModResult initialize_bullet_time(ModError* error) {
    ModResult result = initialize_enemy_slow_motion();
    if (result == MOD_OK) {
        result = mods::hook::add_pre<ActorExecuteHook>(svc_hook, before_actor_execute);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<ActorExecuteHook>(svc_hook, after_actor_execute);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<CombatModelCalcHook>(
            svc_hook, before_combat_model_calc);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<CombatModelCalcHook>(
            svc_hook, after_combat_model_calc);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<CombatModelViewCalcHook>(
            svc_hook, before_combat_model_view_calc);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<CombatModelViewCalcHook>(
            svc_hook, after_combat_model_view_calc);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<DrawIteraterHook>(
            svc_hook, before_draw_iterater);
    }
    if (result == MOD_OK) {
        result = mods::hook::replace<DspRenderHook>(
            svc_hook, replace_dsp_render);
    }
    if (result == MOD_OK) {
#if defined(__APPLE__)
        result = mods::hook::add_pre<ProcessMethodHook>(svc_hook, before_process_method);
#else
        result = mods::hook::add_pre<ProcessExecuteHook>(svc_hook, before_process_execute);
#endif
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<ColliderSetHook>(svc_hook, before_collider_set);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<ArrowHitHook>(svc_hook, before_arrow_hit);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<LinkAllAnimePlayHook>(
            svc_hook, before_link_all_anime_play);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<LinkAllAnimePlayHook>(
            svc_hook, after_link_all_anime_play);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<LinkPosMoveHook>(svc_hook, before_link_pos_move);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<LinkPosMoveHook>(svc_hook, after_link_pos_move);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<LinkDamageActionHook>(
            svc_hook, before_link_damage_action);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<LinkVoiceStartHook>(
            svc_hook, after_link_voice_start);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<CommonAtTgHitHook>(
            svc_hook, before_common_at_tg_hit);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_pre<AtCheckHook>(svc_hook, before_at_check);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<AtCheckHook>(svc_hook, after_at_check);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<FlurryAttackPowerHook>(
            svc_hook, after_flurry_attack_power);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<FlurryActionStringHook>(
            svc_hook, after_flurry_action_string);
    }
    if (result == MOD_OK) {
        GfxStageHookDesc desc = GFX_STAGE_HOOK_DESC_INIT;
        desc.callback = draw_slow_motion_edges;
        result = svc_gfx->register_stage_hook(
            mod_ctx, GFX_STAGE_FRAME_BEFORE_HUD, &desc, &s_edgesHook);
    }
    if (result != MOD_OK) {
        return mods::set_error(error, result,
                               "failed to install Dawnlight Bullet Time hooks");
    }
    return MOD_OK;
}

void mark_manual_jump_started(daAlink_c* link) {
    stop_flurry_rush();
    stop_bullet_time();
    s_manualJumpOwner = link;
    s_manualJumpStarted = Clock::now();
    s_bulletTimeUsedForJump = false;
    s_bowBulletTimePending = false;
}

void clear_manual_jump(daAlink_c* link) {
    if (link != nullptr && s_manualJumpOwner != link) {
        return;
    }

    stop_bullet_time();
    s_manualJumpOwner = nullptr;
    s_bulletTimeUsedForJump = false;
    s_bowBulletTimePending = false;
}

void update_bullet_time_before_jump(daAlink_c* link) {
    if (link == nullptr || s_manualJumpOwner != link) {
        return;
    }

    if (!bullet_time_enabled() || !r_jump_enabled() || !manual_jump_is_airborne(link) ||
        Clock::now() - s_manualJumpStarted >= kManualJumpTimeout)
    {
        clear_manual_jump(link);
        return;
    }

    if (s_bulletTimeActive &&
        (link->doTrigger() || Clock::now() - s_bulletTimeStarted >= kBulletTimeDuration))
    {
        stop_bullet_time();
    }

    if (!s_bulletTimeActive && !s_bulletTimeUsedForJump) {
        if (!s_bowBulletTimePending && selected_bow_button_requested(link) &&
            !daPy_py_c::checkBowItem(link->mEquipItem))
        {
            equip_selected_bow_for_bullet_time(link);
            s_bowBulletTimePending = true;
        }

        if ((s_bowBulletTimePending || bow_aim_requested(link)) &&
            daPy_py_c::checkBowItem(link->mEquipItem) && link->checkReadyItem())
        {
            start_bullet_time(link);
            s_bowBulletTimePending = false;
        }
    }

    if (s_bulletTimeActive) {
        link->setDoStatus(BUTTON_STATUS_BACK);
        apply_bullet_time_fall(link);
        prepare_bow_aim(link);
    }
}

void update_bullet_time_after_jump(daAlink_c* link) {
    if (!bullet_time_active_for(link)) {
        return;
    }

    if (!manual_jump_is_airborne(link)) {
        clear_manual_jump(link);
        return;
    }

    link->setDoStatus(BUTTON_STATUS_BACK);
    apply_bullet_time_fall(link);
    if (daPy_py_c::checkBowItem(link->mEquipItem) &&
        !update_bullet_time_bow_aim(link) && link->setBodyAngleToCamera())
    {
        link->setBowSight();
    }
}

void slow_bullet_time_jump_speed_change(daAlink_c* link, float previousNormalSpeed) {
    if (!bullet_time_active_for(link)) {
        return;
    }

    link->mNormalSpeed = previousNormalSpeed +
                         (link->mNormalSpeed - previousNormalSpeed) * kLinkTimeScale;
}

bool bullet_time_active_for(const daAlink_c* link) {
    return s_bulletTimeActive && s_manualJumpOwner == link;
}

void bullet_time_tick() {
    daAlink_c* currentLink = daAlink_getAlinkActorClass();
    sync_bullet_time_gyro_keep_alive();
    update_dodge_attempt(currentLink);

    if (s_flurryRushActive) {
        const bool targetStillLocked = currentLink == s_flurryRushOwner &&
                                       currentLink != nullptr &&
                                       currentLink->checkAttentionLock() &&
                                       currentLink->mTargetedActor == s_flurryRushTarget &&
                                       s_flurryRushTarget != nullptr;
        const bool timedOut = !s_flurryLinkSlowed &&
                              Clock::now() - s_flurryRushStarted >= kFlurryRushDuration;
        if (!flurry_rush_enabled() || !targetStillLocked || timedOut) {
            stop_flurry_rush();
        }
    }

    if (s_manualJumpOwner != nullptr) {
        if (currentLink != s_manualJumpOwner || !bullet_time_enabled() || !r_jump_enabled()) {
            clear_manual_jump(nullptr);
        } else {
            const auto now = Clock::now();
            if (!manual_jump_is_airborne(currentLink) ||
                now - s_manualJumpStarted >= kManualJumpTimeout)
            {
                clear_manual_jump(currentLink);
            } else if (s_bulletTimeActive &&
                       now - s_bulletTimeStarted >= kBulletTimeDuration)
            {
                stop_bullet_time();
            }
        }
    }

    if (!update_stamina(s_bulletTimeActive) && s_bulletTimeActive) {
        stop_bullet_time();
    }

    if (combat_slow_active()) {
        ++s_slowFrame;
    }
}

void shutdown_bullet_time() {
    reset_enemy_slow_motion();
    stop_flurry_rush(false);
    clear_manual_jump(nullptr);
    s_dodgeOwner = nullptr;
    s_dodgeProc = daAlink_c::PROC_WAIT;
    s_dodgeTriggered = false;
    s_bulletTimeActive = false;
    sync_bullet_time_gyro_keep_alive();
    s_linkPositionStep = {};
    s_linkAnimationRateStep = {};
    s_actorExecuteStack = {};
    s_actorExecuteDepth = 0;
    s_enemySlowMotion.reset();
    s_arrowSlowMotion.reset();
    s_flurryLinkSlowMotion.reset();
    s_lastPresentationSample = {};
    s_audioRate.store(1.0f, std::memory_order_relaxed);
    clear_link_voice_rates();
    if (s_edgesHook != 0 && svc_gfx != nullptr) {
        svc_gfx->unregister_stage_hook(mod_ctx, s_edgesHook);
        s_edgesHook = 0;
    }
    clear_model_visual_states();
}

}  // namespace dawnlight
