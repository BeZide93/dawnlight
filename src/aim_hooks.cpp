#include "config.hpp"
#include "service_imports.hpp"

#include "global.h"
#include "d/actor/d_a_alink.h"
#include "d/d_camera.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_camera_mng.h"
#include "m_Do/m_Do_controller_pad.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "SSystem/SComponent/c_math.h"

#include <RmlUi/Core.h>
#include <SDL3/SDL_touch.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "dusk/ui/controls.hpp"
#include "dusk/ui/document.hpp"

#define private public
#include "dusk/ui/touch_controls.hpp"
#undef private

namespace dawnlight {
namespace {

DEFINE_HOOK(&daAlink_c::procBowSubject, BowSubjectHook);
DEFINE_HOOK(&daAlink_c::procBoomerangSubject, BoomerangSubjectHook);
DEFINE_HOOK(&daAlink_c::procHookshotSubject, HookshotSubjectHook);
DEFINE_HOOK(&daAlink_c::procIronBallSubject, IronBallSubjectHook);
DEFINE_HOOK(&daAlink_c::procCopyRodSubject, CopyRodSubjectHook);
DEFINE_HOOK(&daAlink_c::execute, PlayerExecuteHook);
DEFINE_HOOK(&dCamera_c::Run, CameraRunHook);
DEFINE_HOOK(&dCamera_c::nextMode, CameraNextModeHook);
DEFINE_HOOK(&dCamera_c::nextType, CameraNextTypeHook);
#if defined(__ANDROID__)
#define DAWNLIGHT_TOUCH_SYNC_STATE_SYMBOL "_ZN4dusk2ui13TouchControls16sync_touch_stateEv"
#define DAWNLIGHT_TOUCH_HANDLE_DOWN_SYMBOL "_ZN4dusk2ui13TouchControls17handle_touch_downERN3Rml5EventE"
#elif defined(__linux__) || defined(__APPLE__)
#define DAWNLIGHT_TOUCH_SYNC_STATE_SYMBOL "_ZN4dusk2ui13TouchControls16sync_touch_stateEv"
#define DAWNLIGHT_TOUCH_HANDLE_DOWN_SYMBOL "_ZN4dusk2ui13TouchControls17handle_touch_downERN3Rml5EventE"
#else
#define DAWNLIGHT_TOUCH_SYNC_STATE_SYMBOL "dusk::ui::TouchControls::sync_touch_state"
#define DAWNLIGHT_TOUCH_HANDLE_DOWN_SYMBOL "dusk::ui::TouchControls::handle_touch_down"
#endif

DEFINE_HOOK_SYMBOL(DAWNLIGHT_TOUCH_SYNC_STATE_SYMBOL,
    void(dusk::ui::TouchControls*), TouchSyncStateHook);
DEFINE_HOOK_SYMBOL(DAWNLIGHT_TOUCH_HANDLE_DOWN_SYMBOL,
    void(dusk::ui::TouchControls*, Rml::Event*), TouchHandleDownHook);

constexpr float kTouchStickRadiusDp = 62.0f;
constexpr float kTouchStickKnobRadiusDp = 24.0f;
constexpr float kTouchAnalogZoneTopDp = 92.0f;
constexpr float kTouchAnalogZoneBottomDp = 30.0f;
constexpr float kTouchLeftZoneWidth = 0.46f;
constexpr float kTouchRightZoneStart = 0.52f;

#if defined(__ANDROID__)
constexpr const char* kTouchEventIdSymbol = "_ZN4dusk2ui14touch_event_idERKN3Rml5EventE";
constexpr const char* kTouchEventPositionSymbol =
    "_ZN4dusk2ui20touch_event_positionERKN3Rml5EventE";
constexpr const char* kTouchDpScaleSymbol = "_ZN4dusk2ui14touch_dp_scaleEPN3Rml7ContextE";
constexpr const char* kRmlContextSymbol = "_ZN6aurora5rmlui11get_contextEv";
constexpr const char* kRmlContextDimensionsSymbol = "_ZNK3Rml7Context13GetDimensionsEv";
constexpr const char* kRmlSetClassSymbol =
    "_ZN3Rml7Element8SetClassERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEb";
constexpr const char* kRmlSetPropertySymbol =
    "_ZN3Rml7Element11SetPropertyERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEES9_";
#elif defined(__linux__)
constexpr const char* kTouchEventIdSymbol = "_ZN4dusk2ui14touch_event_idERKN3Rml5EventE";
constexpr const char* kTouchEventPositionSymbol =
    "_ZN4dusk2ui20touch_event_positionERKN3Rml5EventE";
constexpr const char* kTouchDpScaleSymbol = "_ZN4dusk2ui14touch_dp_scaleEPN3Rml7ContextE";
constexpr const char* kRmlContextSymbol = "_ZN6aurora5rmlui11get_contextEv";
constexpr const char* kRmlContextDimensionsSymbol = "_ZNK3Rml7Context13GetDimensionsEv";
constexpr const char* kRmlSetClassSymbol =
    "_ZN3Rml7Element8SetClassERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEb";
constexpr const char* kRmlSetPropertySymbol =
    "_ZN3Rml7Element11SetPropertyERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES8_";
#elif defined(__APPLE__)
constexpr const char* kTouchEventIdSymbol = "_ZN4dusk2ui14touch_event_idERKN3Rml5EventE";
constexpr const char* kTouchEventPositionSymbol =
    "_ZN4dusk2ui20touch_event_positionERKN3Rml5EventE";
constexpr const char* kTouchDpScaleSymbol = "_ZN4dusk2ui14touch_dp_scaleEPN3Rml7ContextE";
constexpr const char* kRmlContextSymbol = "_ZN6aurora5rmlui11get_contextEv";
constexpr const char* kRmlContextDimensionsSymbol = "_ZNK3Rml7Context13GetDimensionsEv";
constexpr const char* kRmlSetClassSymbol =
    "_ZN3Rml7Element8SetClassERKNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEb";
constexpr const char* kRmlSetPropertySymbol =
    "_ZN3Rml7Element11SetPropertyERKNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEES9_";
#else
constexpr const char* kTouchEventIdSymbol = "dusk::ui::touch_event_id";
constexpr const char* kTouchEventPositionSymbol = "dusk::ui::touch_event_position";
constexpr const char* kTouchDpScaleSymbol = "dusk::ui::touch_dp_scale";
constexpr const char* kRmlContextSymbol = "aurora::rmlui::get_context";
constexpr const char* kRmlContextDimensionsSymbol = "Rml::Context::GetDimensions";
constexpr const char* kRmlSetClassSymbol = "Rml::Element::SetClass";
constexpr const char* kRmlSetPropertySymbol = "Rml::Element::SetProperty";
#endif

enum class AimItem {
    Bow,
    Boomerang,
    Hookshot,
    IronBall,
    CopyRod,
};

using TouchControls = dusk::ui::TouchControls;
using TouchEventIdFn = SDL_FingerID (*)(const Rml::Event&) noexcept;
using TouchEventPositionFn = Rml::Vector2f (*)(const Rml::Event&) noexcept;
using TouchDpScaleFn = float (*)(Rml::Context*) noexcept;
using RmlContextFn = Rml::Context* (*)();
using RmlContextDimensionsFn = Rml::Vector2i (*)(const Rml::Context*);
using RmlElementSetClassFn = void (*)(Rml::Element*, const Rml::String&, bool);
using RmlElementSetPropertyFn =
    bool (*)(Rml::Element*, const Rml::String&, const Rml::String&);

TouchEventIdFn s_touchEventId = nullptr;
TouchEventPositionFn s_touchEventPosition = nullptr;
TouchDpScaleFn s_touchDpScale = nullptr;
RmlContextFn s_rmlContext = nullptr;
RmlContextDimensionsFn s_rmlContextDimensions = nullptr;
RmlElementSetClassFn s_rmlSetClass = nullptr;
RmlElementSetPropertyFn s_rmlSetProperty = nullptr;

struct SavedTouchMove {
    TouchControls* controls = nullptr;
    TouchControls::StickTouch move;
    SDL_FingerID cameraId = 0;
    bool active = false;
    bool cameraWasActive = false;
};

SavedTouchMove s_savedTouchMove;
bool s_customCinemaSightActive = false;

bool use_custom_aim_movement() {
    return aim_mode() != AimMode::Vanilla || aim_movement_enabled();
}

bool use_third_person_camera() {
    return aim_mode() == AimMode::ThirdPerson;
}

bool use_cinema_camera() {
    return aim_mode() == AimMode::Cinema;
}

void remember_custom_cinema_sight() {
    if (use_cinema_camera()) {
        s_customCinemaSightActive = true;
    }
}

bool use_scope_suppress_camera() {
    return aim_mode() == AimMode::ThirdPerson || aim_mode() == AimMode::Cinema;
}

bool is_hawkeye_bow(daAlink_c* link) {
    return link != nullptr && link->mEquipItem == dItemNo_HAWK_ARROW_e;
}

bool hawkeye_active() noexcept {
    return dCamera_c::isAimActive() && dComIfGp_checkPlayerStatus0(0, 0x200000);
}

bool touch_aim_movement_enabled() {
    return aim_movement_enabled() && dCamera_c::isAimActive() && !hawkeye_active();
}

bool player_in_supported_aim_status(const u32 pad) {
    return dComIfGp_checkPlayerStatus0(pad, 0x1040) ||
           dComIfGp_checkPlayerStatus0(pad, 0x80000) ||
           dComIfGp_checkPlayerStatus0(pad, 0x80) ||
           dComIfGp_checkPlayerStatus0(pad, 0x4000) ||
           dComIfGp_checkPlayerStatus0(pad, 0x400);
}

template <typename Fn>
ModResult resolve_required_symbol(const char* symbol, Fn& out) {
    void* resolved = nullptr;
    const ModResult result = svc_hook->resolve(mod_ctx, symbol, &resolved, nullptr);
    if (result == MOD_OK) {
        out = reinterpret_cast<Fn>(resolved);
    }
    return result;
}

template <typename Fn>
void resolve_optional_symbol(const char* symbol, Fn& out) {
    void* resolved = nullptr;
    if (svc_hook->resolve(mod_ctx, symbol, &resolved, nullptr) == MOD_OK) {
        out = reinterpret_cast<Fn>(resolved);
    }
}

ModResult resolve_touch_aim_symbols() {
    if (s_touchEventId != nullptr) {
        return MOD_OK;
    }

    ModResult result = resolve_required_symbol(kTouchEventIdSymbol, s_touchEventId);
    if (result == MOD_OK) {
        result = resolve_required_symbol(kTouchEventPositionSymbol, s_touchEventPosition);
    }
    if (result == MOD_OK) {
        result = resolve_required_symbol(kTouchDpScaleSymbol, s_touchDpScale);
    }
    if (result == MOD_OK) {
        result = resolve_required_symbol(kRmlContextSymbol, s_rmlContext);
    }
    if (result == MOD_OK) {
        result = resolve_required_symbol(kRmlContextDimensionsSymbol, s_rmlContextDimensions);
    }
    resolve_optional_symbol(kRmlSetClassSymbol, s_rmlSetClass);
    resolve_optional_symbol(kRmlSetPropertySymbol, s_rmlSetProperty);
    return result;
}

float touch_dp_scale(Rml::Context* context) {
    return s_touchDpScale != nullptr ? std::max(s_touchDpScale(context), 1.0f) : 1.0f;
}

void set_touch_element_class(Rml::Element* element, const char* className, bool active) {
    if (element == nullptr || s_rmlSetClass == nullptr) {
        return;
    }

    const std::string classNameString = className;
    s_rmlSetClass(element, classNameString, active);
}

void set_touch_element_px(Rml::Element* element, const char* property, float value) {
    if (element == nullptr || s_rmlSetProperty == nullptr) {
        return;
    }

    char valueString[32] = {};
    std::snprintf(valueString, sizeof(valueString), "%.3fpx", value);
    const std::string propertyString = property;
    const std::string propertyValue = valueString;
    s_rmlSetProperty(element, propertyString, propertyValue);
}

Rml::Vector2f clamped_touch_stick_delta(
    const Rml::Vector2f start, const Rml::Vector2f current, const float radius) {
    Rml::Vector2f delta = current - start;
    const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (length > radius && radius > 0.0f) {
        delta *= radius / length;
    }
    return delta;
}

void sync_touch_move_stick_visual(TouchControls* controls) {
    if (controls == nullptr || !controls->mMoveTouch.active || controls->mControlStick == nullptr) {
        return;
    }

    Rml::Context* context = s_rmlContext != nullptr ? s_rmlContext() : nullptr;
    const float scale = touch_dp_scale(context);
    const float stickRadius = kTouchStickRadiusDp * scale;
    if (stickRadius <= 0.0f) {
        return;
    }

    const Rml::Vector2f delta = clamped_touch_stick_delta(
        controls->mMoveTouch.start, controls->mMoveTouch.current, stickRadius);
    const float knobRadius = kTouchStickKnobRadiusDp * scale;

    set_touch_element_class(controls->mControlStick, "active", true);
    set_touch_element_px(
        controls->mControlStick, "left", controls->mMoveTouch.start.x - stickRadius);
    set_touch_element_px(
        controls->mControlStick, "top", controls->mMoveTouch.start.y - stickRadius);
    set_touch_element_px(
        controls->mControlKnob, "left", stickRadius + delta.x - knobRadius);
    set_touch_element_px(
        controls->mControlKnob, "top", stickRadius + delta.y - knobRadius);
}

bool touch_context_bounds(Rml::Vector2i& dimensions, float& scale) {
    Rml::Context* context = s_rmlContext != nullptr ? s_rmlContext() : nullptr;
    if (context == nullptr || s_rmlContextDimensions == nullptr) {
        return false;
    }

    dimensions = s_rmlContextDimensions(context);
    scale = touch_dp_scale(context);
    return dimensions.x > 0 && dimensions.y > 0;
}

BOOL face_camera_view_yaw(daAlink_c* link) {
    if (link == nullptr) {
        return FALSE;
    }

    auto* camera = dComIfGp_getCamera(link->field_0x317c);
    if (camera == nullptr) {
        return FALSE;
    }

    const cXyz direction = *fopCamM_GetCenter_p(camera) - *fopCamM_GetEye_p(camera);
    const f32 horizontal = JMAFastSqrt(SQUARE(direction.x) + SQUARE(direction.z));
    if (horizontal <= 0.001f) {
        return FALSE;
    }

    link->shape_angle.y = cM_atan2s(direction.x, direction.z);
    link->field_0x310c = link->shape_angle.y;
    return TRUE;
}

BOOL aim_with_c_stick(daAlink_c* link) {
    const f32 stickValue = link->mStickValue;
    const f32 moveValue = link->mMoveValue;
    const s16 stickAngle = link->mStickAngle;
    const s16 moveAngle = link->mMoveAngle;

    const f32 cStickValue = cLib_minMaxLimit<f32>(mDoCPd_c::getSubStickValue(PAD_1), 0.0f, 1.0f);
    if (cStickValue <= 0.05f) {
        link->mStickValue = 0.0f;
        link->mMoveValue = 0.0f;
    } else {
        link->mStickValue = cStickValue;
        link->mMoveValue = cStickValue;
        link->mStickAngle = mDoCPd_c::getSubStickAngle(PAD_1) + 0x8000;
        link->mMoveAngle = link->mStickAngle +
            dCam_getControledAngleY(dComIfGp_getCamera(link->field_0x317c));
    }

    const BOOL result = link->setBodyAngleToCamera();
    link->mStickValue = stickValue;
    link->mMoveValue = moveValue;
    link->mStickAngle = stickAngle;
    link->mMoveAngle = moveAngle;
    return result;
}

void update_move_animation(daAlink_c* link, u8 waitDirection, bool rightWait, bool ironBall) {
    f32 morph = -1.0f;
    if (link->checkZeroSpeedF()) {
        link->onModeFlg(1);
        if (link->field_0x2f98 != waitDirection) {
            link->field_0x2f98 = waitDirection;
        }
        if (rightWait) {
            link->current.angle.y = link->shape_angle.y - 0x4000;
        }
    } else {
        link->offModeFlg(1);
    }

    if (ironBall && link->checkModeFlg(1)) {
        link->setIronBallBaseAnime();
    } else {
        link->setBlendAtnMoveAnime(morph);
    }
}

void draw_iron_ball_sight(daAlink_c* link) {
    cXyz position;
    link->checkSightLine(10000.0f, &position);
    link->mSight.setPos(&position);
    link->mSight.onDrawFlg();
    link->mSight.offLockFlg();
    remember_custom_cinema_sight();
}

void draw_camera_center_sight(daAlink_c* link) {
    auto* camera = link != nullptr ? dComIfGp_getCamera(link->field_0x317c) : nullptr;
    if (camera == nullptr) {
        return;
    }

    cXyz* eye = fopCamM_GetEye_p(camera);
    cXyz* center = fopCamM_GetCenter_p(camera);
    cXyz direction = *center - *eye;
    if (direction.abs() <= 0.001f) {
        return;
    }

    direction.normalize();
    cXyz position = *eye + direction * 10000.0f;
    link->mArrowLinChk.Set(eye, &position, link);
    if (dComIfG_Bgsp().LineCross(&link->mArrowLinChk)) {
        position = link->mArrowLinChk.GetCross();
    }

    link->mSight.setPos(&position);
    link->mSight.onDrawFlg();
    link->mSight.offLockFlg();
    remember_custom_cinema_sight();
}

void draw_subject_sight(daAlink_c* link, AimItem item) {
    switch (item) {
    case AimItem::Bow:
        if (link->mEquipItem != dItemNo_HAWK_ARROW_e) {
            link->setBowSight();
            link->mSight.onDrawFlg();
            remember_custom_cinema_sight();
        }
        break;
    case AimItem::Boomerang:
        link->setBoomerangSight();
        link->mSight.onDrawFlg();
        remember_custom_cinema_sight();
        break;
    case AimItem::Hookshot:
        link->setHookshotSight();
        link->mSight.onDrawFlg();
        remember_custom_cinema_sight();
        break;
    case AimItem::IronBall:
        draw_iron_ball_sight(link);
        break;
    case AimItem::CopyRod:
        link->setCopyRodSight();
        link->mSight.onDrawFlg();
        remember_custom_cinema_sight();
        break;
    }
}

bool should_keep_cinema_bow_sight(daAlink_c* link) {
    return link != nullptr && use_cinema_camera() && link->mEquipItem != dItemNo_HAWK_ARROW_e;
}

void keep_cinema_bow_sight(daAlink_c* link) {
    if (should_keep_cinema_bow_sight(link)) {
        aim_with_c_stick(link);
        draw_camera_center_sight(link);
    }
}

void normalize_forward_aim_speed(daAlink_c* link) {
    if (link == nullptr || !use_custom_aim_movement() || link->mTargetedActor != nullptr ||
        !link->checkInputOnR())
    {
        return;
    }

    const s16 moveDelta = link->mMoveAngle - link->shape_angle.y;
    if (cM_scos(moveDelta) < 0.99f || std::fabs(cM_ssin(moveDelta)) > 0.02f) {
        return;
    }

    const f32 targetSpeed = link->mMaxSpeed * link->mStickValue * link->mStickValue;
    if (link->mNormalSpeed >= 0.0f && link->mNormalSpeed < targetSpeed) {
        link->mNormalSpeed = targetSpeed;
    }
}

bool update_subject_aim(daAlink_c* link, AimItem item) {
    if (link == nullptr) {
        return false;
    }

    const bool hawkeyeBow = item == AimItem::Bow && is_hawkeye_bow(link);
    if (hawkeyeBow || !use_custom_aim_movement()) {
        return false;
    }

    if (use_cinema_camera()) {
        face_camera_view_yaw(link);
    }

    const s16 shapeYaw = link->shape_angle.y;
    link->setSpeedAndAngleAtn();
    link->shape_angle.y = shapeYaw;

    switch (item) {
    case AimItem::Bow:
        update_move_animation(link, 3, true, false);
        if (aim_with_c_stick(link)) {
            draw_subject_sight(link, item);
        } else if (should_keep_cinema_bow_sight(link)) {
            draw_camera_center_sight(link);
        }
        break;
    case AimItem::Boomerang:
        update_move_animation(link, 3, false, false);
        if (aim_with_c_stick(link)) {
            draw_subject_sight(link, item);
        }
        break;
    case AimItem::Hookshot:
        if (link->checkHookshotWait()) {
            update_move_animation(link, 2, false, false);
            if (aim_with_c_stick(link)) {
                draw_subject_sight(link, item);
            }
        }
        break;
    case AimItem::IronBall:
        if (link->checkIronBallPreSwingAnime()) {
            link->mNormalSpeed = 0.0f;
        }
        update_move_animation(link, 2, false, true);
        if (link->itemButton() && link->mItemVar0.field_0x3018 == 2 && aim_with_c_stick(link)) {
            draw_iron_ball_sight(link);
        }
        break;
    case AimItem::CopyRod:
        update_move_animation(link, 3, false, false);
        if (aim_with_c_stick(link)) {
            draw_subject_sight(link, item);
        }
        break;
    }
    normalize_forward_aim_speed(link);
    return true;
}

HookAction before_touch_sync_state(ModContext*, void* args, void*, void*) {
    s_savedTouchMove = {};
    auto* controls = mods::arg<TouchControls*>(args, 0);
    if (!touch_aim_movement_enabled() || controls == nullptr || !controls->mMoveTouch.active) {
        return HOOK_CONTINUE;
    }

    s_savedTouchMove = {
        .controls = controls,
        .move = controls->mMoveTouch,
        .cameraId = controls->mCameraTouch.id,
        .active = true,
        .cameraWasActive = controls->mCameraTouch.active,
    };
    return HOOK_CONTINUE;
}

void after_touch_sync_state(ModContext*, void* args, void*, void*) {
    auto* controls = mods::arg<TouchControls*>(args, 0);
    const bool restore =
        s_savedTouchMove.active && s_savedTouchMove.controls == controls &&
        touch_aim_movement_enabled();
    if (!restore) {
        s_savedTouchMove = {};
        return;
    }

    if (!controls->mMoveTouch.active) {
        controls->mMoveTouch = s_savedTouchMove.move;
    }
    if (!s_savedTouchMove.cameraWasActive && controls->mCameraTouch.active &&
        controls->mCameraTouch.id == s_savedTouchMove.move.id)
    {
        controls->mCameraTouch = {};
    }

    sync_touch_move_stick_visual(controls);
    s_savedTouchMove = {};
}

HookAction before_touch_handle_down(ModContext*, void* args, void*, void*) {
    auto* controls = mods::arg<TouchControls*>(args, 0);
    auto* event = mods::arg<Rml::Event*>(args, 1);
    if (!touch_aim_movement_enabled() || controls == nullptr || event == nullptr ||
        controls->mWasSuppressed || s_touchEventId == nullptr || s_touchEventPosition == nullptr)
    {
        return HOOK_CONTINUE;
    }

    Rml::Vector2i dimensions;
    float scale = 1.0f;
    if (!touch_context_bounds(dimensions, scale)) {
        return HOOK_CONTINUE;
    }

    const Rml::Vector2f position = s_touchEventPosition(*event);
    const float width = static_cast<float>(dimensions.x);
    const bool inLeftZone = position.x < width * kTouchLeftZoneWidth;
    const bool inCameraZone = position.x > width * kTouchRightZoneStart;
    if (inCameraZone) {
        return HOOK_CONTINUE;
    }

    const float top = controls->mSafeInsets.top + kTouchAnalogZoneTopDp * scale;
    const float bottom =
        static_cast<float>(dimensions.y) - controls->mSafeInsets.bottom -
        kTouchAnalogZoneBottomDp * scale;
    const bool inAnalogZone = position.y >= top && position.y <= bottom;
    if (!inLeftZone || !inAnalogZone) {
        return HOOK_SKIP_ORIGINAL;
    }

    if (!controls->mMoveTouch.active) {
        controls->mMoveTouch = {
            .id = s_touchEventId(*event),
            .start = position,
            .current = position,
            .active = true,
        };
        sync_touch_move_stick_visual(controls);
    }
    return HOOK_SKIP_ORIGINAL;
}

HookAction replace_bow_subject(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    link->setDoStatus(BUTTON_STATUS_BACK);
    if (!link->checkNextAction(0) &&
        (!update_subject_aim(link, AimItem::Bow) && link->setBodyAngleToCamera()))
    {
        link->setBowSight();
    }
    keep_cinema_bow_sight(link);
    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

HookAction replace_boomerang_subject(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link->checkItemActorPointer()) {
        *static_cast<int*>(retval) = 1;
        return HOOK_SKIP_ORIGINAL;
    }

    if (link->checkBoomerangReadyAnime()) {
        link->setDoStatus(BUTTON_STATUS_BACK);
    }
    link->setShapeAngleToAtnActor(0);

    if (!link->checkNextAction(0)) {
        if (!update_subject_aim(link, AimItem::Boomerang) && link->setBodyAngleToCamera()) {
            link->setBoomerangSight();
        }
    } else {
        link->mSight.offDrawFlg();
    }

    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

HookAction replace_hookshot_subject(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link->checkHookshotWait()) {
        link->setDoStatus(BUTTON_STATUS_BACK);
    }

    link->setShapeAngleToAtnActor(0);
    link->mSight.offDrawFlg();

    if (!link->checkNextAction(0)) {
        if (link->checkHookshotWait()) {
            if (!update_subject_aim(link, AimItem::Hookshot) && link->setBodyAngleToCamera()) {
                link->setHookshotSight();
            }
            dComIfGp_clearPlayerStatus0(0, 0x40000);
        } else {
            dComIfGp_setPlayerStatus0(0, 0x40000);
        }
    }

    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

HookAction replace_iron_ball_subject(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link->checkIronBallAnime()) {
        link->setDoStatus(BUTTON_STATUS_BACK);
    }
    link->setShapeAngleToAtnActor(0);

    if (!link->checkNextAction(0) && link->itemButton() && link->mItemVar0.field_0x3018 == 2) {
        if (!update_subject_aim(link, AimItem::IronBall)) {
            link->setBodyAngleToCamera();
        }
    }

    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

HookAction replace_copy_rod_subject(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link->checkItemActorPointer()) {
        *static_cast<int*>(retval) = 1;
        return HOOK_SKIP_ORIGINAL;
    }

    if (link->checkCopyRodReadyAnime()) {
        link->setDoStatus(BUTTON_STATUS_BACK);
    }
    link->setShapeAngleToAtnActor(0);

    if (!link->checkNextAction(0)) {
        if (!update_subject_aim(link, AimItem::CopyRod) && link->setBodyAngleToCamera()) {
            link->setCopyRodSight();
        }
    } else {
        link->mSight.offDrawFlg();
    }

    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

bool player_in_supported_aim_state(dCamera_c* camera) {
    auto* link = daAlink_getAlinkActorClass();
    if (link == nullptr || camera == nullptr) {
        return false;
    }

    return player_in_supported_aim_status(camera->mPadID);
}

void after_camera_run(ModContext*, void* args, void*, void*) {
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    if (camera == nullptr || !use_cinema_camera() || !player_in_supported_aim_state(camera) ||
        is_hawkeye_bow(daAlink_getAlinkActorClass()))
    {
        return;
    }

    const int zoomPercent = cinema_zoom_percent();
    if (zoomPercent == 100) {
        return;
    }

    const float zoom = std::clamp(static_cast<float>(zoomPercent) / 100.0f, 0.25f, 4.0f);
    camera->mFovy = std::clamp(camera->mFovy / zoom, 10.0f, 120.0f);
}

void after_camera_next_mode(ModContext*, void* args, void* retval, void*) {
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    auto* result = static_cast<s32*>(retval);
    if (camera == nullptr || result == nullptr || !use_scope_suppress_camera() ||
        !player_in_supported_aim_state(camera))
    {
        return;
    }
    if (is_hawkeye_bow(daAlink_getAlinkActorClass())) {
        return;
    }

    if (*result == 7 || *result == 8) {
        *result = use_third_person_camera() ? 0 : 8;
    }
}

void after_camera_next_type(ModContext*, void* args, void* retval, void*) {
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    auto* result = static_cast<s32*>(retval);
    if (camera == nullptr || result == nullptr || !use_scope_suppress_camera() ||
        !player_in_supported_aim_state(camera))
    {
        return;
    }
    if (is_hawkeye_bow(daAlink_getAlinkActorClass())) {
        return;
    }

    const int scopeType = camera->GetCameraTypeFromCameraName("Scope");
    if (*result == scopeType) {
        *result = camera->mMapToolType;
    }
}

void after_player_execute(ModContext*, void* args, void*, void*) {
    if (!s_customCinemaSightActive) {
        return;
    }

    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link != nullptr && use_cinema_camera() && dCamera_c::isAimActive() &&
        player_in_supported_aim_status(0))
    {
        return;
    }

    if (link != nullptr) {
        link->mSight.offDrawFlg();
        link->mSight.offLockFlg();
    }
    s_customCinemaSightActive = false;
}

ModResult add_aim_hooks(ModError* error, ModResult result) {
    if (result == MOD_OK) {
        result = mods::hook_add_pre<BoomerangSubjectHook>(svc_hook, replace_boomerang_subject);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<HookshotSubjectHook>(svc_hook, replace_hookshot_subject);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<IronBallSubjectHook>(svc_hook, replace_iron_ball_subject);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<CopyRodSubjectHook>(svc_hook, replace_copy_rod_subject);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<CameraNextModeHook>(svc_hook, after_camera_next_mode);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<CameraNextTypeHook>(svc_hook, after_camera_next_type);
    }
    if (result == MOD_OK) {
        ModResult touchResult = resolve_touch_aim_symbols();
#if defined(__ANDROID__)
        result = touchResult;
#else
        if (touchResult != MOD_OK) {
            s_touchEventId = nullptr;
            s_touchEventPosition = nullptr;
            s_touchDpScale = nullptr;
            s_rmlContext = nullptr;
            s_rmlContextDimensions = nullptr;
            s_rmlSetClass = nullptr;
            s_rmlSetProperty = nullptr;
        }
#endif
    }
    if (result == MOD_OK) {
        if (s_touchEventId != nullptr) {
            result = mods::hook_add_pre<TouchSyncStateHook>(svc_hook, before_touch_sync_state);
        }
    }
    if (result == MOD_OK) {
        if (s_touchEventId != nullptr) {
            result = mods::hook_add_post<TouchSyncStateHook>(svc_hook, after_touch_sync_state);
        }
    }
    if (result == MOD_OK) {
        if (s_touchEventId != nullptr) {
            result = mods::hook_add_pre<TouchHandleDownHook>(svc_hook, before_touch_handle_down);
        }
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<PlayerExecuteHook>(svc_hook, after_player_execute);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<CameraRunHook>(svc_hook, after_camera_run);
    }
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight aim hooks");
    }
    return MOD_OK;
}

}  // namespace

ModResult install_aim_hooks(ModError* error) {
    return add_aim_hooks(error, mods::hook_add_pre<BowSubjectHook>(svc_hook, replace_bow_subject));
}

}  // namespace dawnlight
