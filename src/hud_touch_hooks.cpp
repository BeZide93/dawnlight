#include "config.hpp"
#include "fierce_deity.hpp"
#include "touch_buttons.hpp"
#include "hud_layout.hpp"
#include "save_state.hpp"
#include "service_imports.hpp"

#include "global.h"
#include "Z2AudioLib/Z2AudioMgr.h"
#include "Z2AudioLib/Z2SeMgr.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_kantera_icon_meter.h"
#include "d/d_item.h"
#include "d/d_item_data.h"
#include "d/d_meter_button.h"
#include "d/d_meter_HIO.h"
#include "d/d_meter2_info.h"
#include "d/d_menu_fmap.h"
#include "d/d_menu_window.h"
#include "d/d_menu_item_explain.h"
#include "d/d_pane_class.h"
#include "d/d_msg_object.h"
#include "JSystem/J2DGraph/J2DPane.h"
#include "JSystem/J2DGraph/J2DScreen.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#define private public
#include "d/d_meter2.h"
#include "d/d_menu_ring.h"
#include "d/d_meter_map.h"
#include "d/d_meter2_draw.h"
#undef private
#include "m_Do/m_Do_controller_pad.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

#include "dusk/config_var.hpp"

#if defined(__ANDROID__)
#define private public
#include "dusk/ui/touch_controls.hpp"
#undef private
#endif

namespace Rml {
using String = std::string;
class Element;
}  // namespace Rml

#if !defined(__ANDROID__)
namespace dusk::ui {
class TouchControls;

// Keep this ABI-facing declaration local to the mod. The standalone mod SDK
// intentionally does not expose Dusklight's private UI headers.
enum class Control {
    A,
    B,
    X,
    Y,
    Z,
    L,
    R,
    FIRST_PERSON,
    ITEMS,
    COLLECTIONS,
    MAP,
    SKIP,
    DPAD_UP,
    DPAD_DOWN,
    DPAD_LEFT,
    DPAD_RIGHT,
    COUNT,
};
}  // namespace dusk::ui
#endif

namespace dawnlight {

bool gale_shortcut_priority_active(const daAlink_c* link);

namespace {

DEFINE_HOOK(&dMeter2_c::_delete, MeterDeleteHook);
DEFINE_HOOK(&dMeter2Draw_c::draw, MeterDrawHook);
DEFINE_HOOK_SYMBOL("dMeter2Draw_c::drawKantera",
    void(dMeter2Draw_c*, s32, f32, f32, f32), MeterDrawKanteraHook);
DEFINE_HOOK_SYMBOL("dMeter2Draw_c::drawOxygen",
    void(dMeter2Draw_c*, s32, f32, f32, f32), MeterDrawOxygenHook);
DEFINE_HOOK(&dMeter2Draw_c::drawKanteraScreen, MeterGaugeScreenHook);
DEFINE_HOOK(&J2DScreen::draw, ScreenDrawHook);

DEFINE_HOOK(&dMeter2_c::moveButtonCross, MeterMoveButtonCrossHook);

DEFINE_HOOK(&dMeterMap_c::draw, MeterMapDrawHook);
DEFINE_HOOK(static_cast<void (J2DPicture::*)(f32, f32, f32, f32, bool, bool, bool)>(
    &J2DPicture::draw), MinimapPictureDrawHook);
DEFINE_HOOK(&daAlink_c::midnaTalkTrigger, MidnaTalkTriggerHook);
DEFINE_HOOK(&daAlink_c::handleQuickTransform, NativeQuickTransformHook);
DEFINE_HOOK(&daAlink_c::handleWolfHowl, NativeWolfHowlHook);
DEFINE_HOOK(&mDoCPd_c::read, PadReadHook);

#if defined(__ANDROID__)
DEFINE_HOOK(&PADSetVirtualStatus, PadSetVirtualStatusHook);
DEFINE_HOOK(&PADClearVirtualStatus, PadClearVirtualStatusHook);
DEFINE_HOOK(&dMenu_Fmap_c::_move, TouchFmapMoveHook);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui20set_control_overrideENS0_7ControlENS0_15ControlOverrideE",
    void(dusk::ui::Control, dusk::ui::ControlOverride), TouchSetControlOverrideHook);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui13TouchControls17sync_visual_stateEv",
    void(dusk::ui::TouchControls*), TouchSyncVisualStateHook);

DEFINE_HOOK_SYMBOL("_ZN4dusk2ui13TouchControls19set_control_pressedENS0_7ControlEb",
    void(dusk::ui::TouchControls*, dusk::ui::Control, bool), TouchSetControlPressedHook);

DEFINE_HOOK_SYMBOL("_ZN4dusk2ui17midna_icon_sourceEv", std::string(), MidnaIconSourceHook);

DEFINE_HOOK_SYMBOL("_ZN4dusk2ui25update_midna_icon_textureEP7J2DPane",
    void(J2DPane*), UpdateMidnaIconTextureHook);
#endif

bool s_dawnlightTouchUiSessionEnabled = false;
#if defined(__ANDROID__)
dusk::ui::ControlOverride s_hostLTouchOverride = dusk::ui::ControlOverride::Default;
bool s_mapLTouchOverrideActive = false;
bool s_touchMapLRawHeld = false;
bool s_touchMapLHeld = false;
bool s_touchMapLPressed = false;
u32 s_touchMapZHeldOriginal = 0;
u32 s_touchMapZPressedOriginal = 0;
bool s_touchMapPortalInputActive = false;
#endif

bool dawnlight_touch_ui_active() {
    return s_dawnlightTouchUiSessionEnabled;
}

struct HudPaneTransformState {
    J2DPane* pane = nullptr;
    f32 offsetX = 0.0f;
    f32 offsetY = 0.0f;
    f32 scale = 1.0f;
    f32 appliedX = 0.0f;
    f32 appliedY = 0.0f;
    f32 appliedScaleX = 1.0f;
    f32 appliedScaleY = 1.0f;
    u8 originalTextFlags = 0;
    bool hasOriginalTextFlags = false;
    bool active = false;
};

struct HudTextBoxFlagState {
    J2DTextBox* textBox = nullptr;
    u8 originalFlags = 0;
    bool active = false;
};

enum class HudPaneSlot : std::size_t {
    ButtonA,
    TextA,
    ButtonB,
    ItemB,
    LightB,
    TextB,
    ButtonX,
    ItemX,
    ItemXSecondary,
    LightX,
    TextX,
    ButtonY,
    ItemY,
    ItemYSecondary,
    LightY,
    TextY,
    Backing,
    DPad,
    Midna,
    DPadItemsText,
    DPadMapText,
    Hearts,
    HealthBar,
    HealthBarCurrentHeart,
    RupeeIcon,
    Rupee0,
    Rupee1,
    Rupee2,
    Keys,
    TearsOfLight,
    Count,
};

std::array<HudPaneTransformState, static_cast<std::size_t>(HudPaneSlot::Count)>
    s_wiiUHudPaneTransforms;
std::array<std::array<HudTextBoxFlagState, 5>, static_cast<std::size_t>(HudPaneSlot::Count)>
    s_hudTextBoxFlags;
std::array<dMeter2Draw_c::item_params, 2> s_xyAmmoOriginalParams = {};
std::array<bool, 2> s_xyAmmoOriginalValid = {};



struct HudPaneVisibilityState {
    J2DPane* pane = nullptr;
    bool wasVisible = false;
    bool active = false;
};

std::array<HudPaneVisibilityState, 4> s_dpadArrowVisibility;
std::array<HudPaneVisibilityState, 4> s_dpadShadowVisibility;

std::array<HudPaneVisibilityState, 4> s_hdHudGlyphVisibility;
void* s_hdHudGlyphDraw = nullptr;
using HudGetConfigVarFn = dusk::config::ConfigVarBase* (*)(std::string_view);
HudGetConfigVarFn s_hdHudGetConfigVar = nullptr;
bool s_hdHudConfigLookupAttempted = false;

struct RoundButtonOverlayState {
    J2DPicture* picture = nullptr;
    std::array<JUtility::TColor, 4> corners;
};
std::array<RoundButtonOverlayState, 32> s_roundXYOverlays;
std::size_t s_roundXYOverlayCount = 0;
J2DScreen* s_roundXYOverlayScreen = nullptr;

struct GaugeDrawState {
    dMeter2Draw_c* meter = nullptr;
    J2DPane* pane = nullptr;
    u8 type = 0;
    f32 x = 0.0f;
    f32 y = 0.0f;
};

GaugeDrawState s_gaugeDraw;

struct MinimapTransformState {
    dMeterMap_c* map = nullptr;
    f32 drawPosY = 0.0f;
    f32 sizeW = 0.0f;
    f32 sizeH = 0.0f;
    bool active = false;
};

MinimapTransformState s_wiiUMinimapTransform;

struct RoundPictureState {
    J2DPicture* picture = nullptr;
    ResTIMG const* textures[2] = {};
    u8 textureCount = 0;
    f32 left = 0.0f;
    f32 top = 0.0f;
    f32 width = 0.0f;
    f32 height = 0.0f;
    bool active = false;
};

std::array<RoundPictureState, 32> s_roundPictureStates;
dMeter2Draw_c* s_roundHudMeter = nullptr;
dMeter2Draw_c* s_hudLayoutMeter = nullptr;
J2DScreen* s_hudLayoutScreen = nullptr;

bool touch_menu_or_pause_context();

u8 hud_texture_item(u8 itemNo) {
    return itemNo == dItemNo_LIGHT_ARROW_e ? dItemNo_BOW_e : itemNo;
}

bool cutscene_skip_touch_visible() {
    auto* event = dComIfGp_getEvent();
    return event != nullptr && event->mEventStatus == 1 && event->mSkipFunc != nullptr &&
           !event->chkFlag2(2);
}

bool touch_midna_controls_suppressed() {
    return dComIfGp_event_runCheck() ||
           (dComIfGp_getMsgObjectClass() != nullptr && dMsgObject_isTalkNowCheck()) ||
           touch_menu_or_pause_context();
}

bool boss_rush_save_active() {
    return save_state_boss_rush_active();
}

bool midna_touch_available() {
    return boss_rush_save_active() ||
           (dComIfGs_isEventBit(dSv_event_flag_c::M_067) &&
            !dComIfGs_isEventBit(dSv_event_flag_c::F_0800));
}

bool midna_touch_button_ready() {
    return dawnlight_touch_ui_active() && !cutscene_skip_touch_visible() &&
           !touch_midna_controls_suppressed() && midna_touch_available() &&
           dComIfGp_getLinkPlayer() != nullptr;
}

#if defined(__ANDROID__)
std::string true_midna_icon_source() {
    if (MidnaIconSourceHook::g_orig == nullptr) {
        return {};
    }
    return MidnaIconSourceHook::g_orig();
}

template <typename Fn>
ModResult resolve_touch_symbol(const char* name, Fn& out) {
    void* resolved = nullptr;
    const ModResult result = svc_hook->resolve(mod_ctx, name, &resolved, nullptr);
    if (result == MOD_OK) {
        out = reinterpret_cast<Fn>(resolved);
    }
    return result;
}

#include "touch_midna_capture.inc"
#endif

u8 hud_layout_item(u8 itemNo) {
    return itemNo == dItemNo_HAWK_ARROW_e ? dItemNo_BOW_e : hud_texture_item(itemNo);
}

u8 clamp_hud_alpha(const f32 alpha) {
    if (alpha <= 0.0f) {
        return 0;
    }
    if (alpha >= 255.0f) {
        return 255;
    }
    return static_cast<u8>(alpha);
}

void hide_pane_tree(J2DPane* pane) {
    if (pane == nullptr) {
        return;
    }

    pane->hide();
    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        hide_pane_tree(child);
    }
}

void show_pane_tree(J2DPane* pane) {
    if (pane == nullptr) {
        return;
    }

    pane->show();
    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        show_pane_tree(child);
    }
}

void set_pane_tree_alpha_visible(J2DPane* pane, const bool visible, const u8 alpha) {
    if (pane == nullptr) {
        return;
    }

    pane->setAlpha(alpha);
    if (visible) {
        pane->show();
    } else {
        pane->hide();
    }

    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        set_pane_tree_alpha_visible(child, visible, alpha);
    }
}

void set_pane_influenced_alpha_tree(J2DPane* pane, const bool influenced) {
    if (pane == nullptr) {
        return;
    }

    pane->setInfluencedAlpha(influenced, true);
    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        set_pane_influenced_alpha_tree(child, influenced);
    }
}

void show_pane_parents(J2DPane* pane) {
    for (J2DPane* parent = pane; parent != nullptr; parent = parent->getParentPane()) {
        parent->show();
    }
}

J2DPicture* as_picture(J2DPane* pane) {
    if (pane == nullptr || pane->getTypeID() != 18) {
        return nullptr;
    }

    return static_cast<J2DPicture*>(pane);
}

J2DPicture* first_picture_pane(J2DPane* pane) {
    if (J2DPicture* picture = as_picture(pane)) {
        return picture;
    }

    if (pane == nullptr) {
        return nullptr;
    }

    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        if (J2DPicture* picture = first_picture_pane(child)) {
            return picture;
        }
    }

    return nullptr;
}

ResTIMG const* round_hud_button_texture(CPaneMgr* roundSource) {
    auto* archive = dComIfGp_getMain2DArchive();
    if (archive != nullptr) {
        auto* texture = static_cast<ResTIMG const*>(
            archive->getResource('TIMG', "tt_zelda_button_ab_maru.bti"));
        if (texture != nullptr) {
            return texture;
        }
    }

    if (roundSource == nullptr) {
        return nullptr;
    }

    J2DPicture* source = first_picture_pane(roundSource->getPanePtr());
    if (source == nullptr || source->getTexture(0) == nullptr) {
        return nullptr;
    }

    return source->getTexture(0)->getTexInfo();
}

void resize_pane_around_center(J2DPane* pane, const f32 width, const f32 height) {
    JGeometry::TBox2<f32> bounds = pane->getBounds();
    const f32 centerX = bounds.i.x + bounds.getWidth() * 0.5f;
    const f32 centerY = bounds.i.y + bounds.getHeight() * 0.5f;

    pane->resize(width, height);
    pane->move(centerX - width * 0.5f, centerY - height * 0.5f);
}

void make_hud_button_picture_square(J2DPicture* picture) {
    const f32 width = picture->getWidth();
    const f32 height = picture->getHeight();
    if (width <= 0.0f || height <= 0.0f || std::fabs(width - height) < 0.01f) {
        return;
    }

    const f32 size = width < height ? width : height;
    resize_pane_around_center(picture, size, size);
}

RoundPictureState* round_picture_state(J2DPicture* picture) {
    for (auto& state : s_roundPictureStates) {
        if (state.active && state.picture == picture) {
            return &state;
        }
    }

    for (auto& state : s_roundPictureStates) {
        if (!state.active) {
            return &state;
        }
    }

    return nullptr;
}

void capture_round_picture_state(RoundPictureState& state, J2DPicture* picture) {
    if (state.active) {
        return;
    }

    const JGeometry::TBox2<f32> bounds = picture->getBounds();
    state.picture = picture;
    state.textureCount = std::min<u8>(picture->getTextureCount(), 2);
    for (u8 i = 0; i < state.textureCount; ++i) {
        auto* texture = picture->getTexture(i);
        state.textures[i] = texture != nullptr ? texture->getTexInfo() : nullptr;
    }
    state.left = bounds.i.x;
    state.top = bounds.i.y;
    state.width = bounds.getWidth();
    state.height = bounds.getHeight();
    state.active = true;
}

void restore_round_picture_state(RoundPictureState& state) {
    if (state.active && state.picture != nullptr) {
        for (u8 i = 0; i < state.textureCount; ++i) {
            if (state.textures[i] != nullptr) {
                state.picture->changeTexture(state.textures[i], i);
            }
        }
        if (state.picture->getTexture(0) != nullptr) {
            state.picture->setTexCoord(state.picture->getTexture(0), BIND15, MIRROR0, false);
        }
        state.picture->resize(state.width, state.height);
        state.picture->move(state.left, state.top);
    }
    state = {};
}

void restore_round_button_pictures() {
    for (auto& state : s_roundPictureStates) {
        restore_round_picture_state(state);
    }
}

bool apply_round_hud_picture(J2DPicture* picture, ResTIMG const* texture) {
    if (picture == nullptr || texture == nullptr) {
        return false;
    }

    RoundPictureState* state = round_picture_state(picture);
    if (state == nullptr) {
        return false;
    }
    capture_round_picture_state(*state, picture);

    const u8 textureCount = picture->getTextureCount();
    for (u8 i = 0; i < textureCount; ++i) {
        picture->changeTexture(texture, i);
    }
    if (picture->getTexture(0) != nullptr) {
        picture->setTexCoord(picture->getTexture(0), BIND15, MIRROR0, false);
    }
    make_hud_button_picture_square(picture);
    return true;
}

void apply_round_hud_button_base(CPaneMgr* button, ResTIMG const* texture) {
    if (button == nullptr) {
        return;
    }

    apply_round_hud_picture(first_picture_pane(button->getPanePtr()), texture);
}

void apply_round_hud_button_layers(J2DPane* pane, ResTIMG const* texture) {
    if (pane == nullptr || texture == nullptr) {
        return;
    }

    apply_round_hud_picture(as_picture(pane), texture);

    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        apply_round_hud_button_layers(child, texture);
    }
}

void apply_round_xy_buttons(dMeter2Draw_c* meter) {
    if (meter == nullptr) {
        restore_round_button_pictures();
        s_roundHudMeter = nullptr;
        return;
    }

    if (s_roundHudMeter != meter) {
        s_roundPictureStates = {};
        s_roundHudMeter = meter;
    }

    if (!round_xy_buttons_enabled()) {
        restore_round_button_pictures();
        return;
    }

    ResTIMG const* texture = round_hud_button_texture(meter->mpButtonA);
    if (texture == nullptr) {
        return;
    }

    apply_round_hud_button_base(meter->mpButtonXY[0], texture);
    apply_round_hud_button_base(meter->mpButtonXY[1], texture);
    if (meter->mpLightXY[0] != nullptr) {
        apply_round_hud_button_layers(meter->mpLightXY[0]->getPanePtr(), texture);
    }
    if (meter->mpLightXY[1] != nullptr) {
        apply_round_hud_button_layers(meter->mpLightXY[1]->getPanePtr(), texture);
    }
}

bool nearly_equal(const f32 lhs, const f32 rhs) {
    return std::fabs(lhs - rhs) < 0.01f;
}

struct HudLayoutOffset {
    f32 x = 0.0f;
    f32 y = 0.0f;
};

HudLayoutOffset hud_item_anchor_position(const int anchor, const f32 defaultX,
    const f32 defaultY) {
    const f32 distance = std::fabs(defaultX) > 14.0f ? std::fabs(defaultX) : 14.0f;

    switch (anchor) {
    case kHudItemAnchorLeft:
        return {-distance, defaultY};
    case kHudItemAnchorTop:
        return {0.0f, defaultY - distance};
    case kHudItemAnchorBottom:
        return {0.0f, defaultY + distance};
    case kHudItemAnchorRight:
    default:
        return {distance, defaultY};
    }
}

HudLayoutOffset hud_item_anchor_delta(const DuskModHudButtonLayout& layout,
    const f32 defaultX, const f32 defaultY) {
    const HudLayoutOffset selected =
        hud_item_anchor_position(layout.item_anchor, defaultX, defaultY);
    const HudLayoutOffset fallback =
        hud_item_anchor_position(layout.default_item_anchor, defaultX, defaultY);
    return {
        .x = selected.x - fallback.x,
        .y = selected.y - fallback.y,
    };
}

f32 hud_item_scale(const DuskModHudTransform& transform,
    const DuskModHudButtonLayout& layout) {
    const f32 itemScale = layout.item_scale > 0.0f ? layout.item_scale : 1.0f;
    return transform.scale * itemScale;
}

f32 hud_text_scale(const DuskModHudTransform& transform,
    const DuskModHudButtonLayout& layout) {
    const f32 textScale = layout.text_scale > 0.0f ? layout.text_scale : 1.0f;
    return transform.scale * textScale;
}

f32 hud_ammo_scale(const DuskModHudTransform& transform,
    const DuskModHudButtonLayout& layout) {
    const f32 itemScale = layout.item_scale > 0.0f ? layout.item_scale : 1.0f;
    const f32 ammoScale = layout.ammo_scale > 0.0f ? layout.ammo_scale : 1.0f;
    return transform.scale * itemScale * ammoScale;
}

DuskModHudTransform hud_layout_xy_transform(const int slot) {
    return slot == dMeter2Draw_c::SELECT_Y_e ? hud_layout_y_transform() :
                                               hud_layout_x_transform();
}

DuskModHudButtonLayout hud_layout_xy_button_layout(const int slot) {
    return slot == dMeter2Draw_c::SELECT_Y_e ? hud_layout_y_button_layout() :
                                               hud_layout_x_button_layout();
}

int hud_button_b_item_variant(dMeter2Draw_c* meter) {
    if (meter == nullptr) {
        return 0;
    }

    switch (meter->mButtonBItem) {
    case dItemNo_LURE_ROD_e:
        return 2;
    case dItemNo_WOOD_STICK_e:
    case dItemNo_SWORD_e:
    case dItemNo_MASTER_SWORD_e:
    case dItemNo_LIGHT_SWORD_e:
        return 1;
    default:
        return 0;
    }
}

HudPaneTransformState& hud_pane_state(const HudPaneSlot slot) {
    return s_wiiUHudPaneTransforms[static_cast<std::size_t>(slot)];
}

void remove_applied_hud_pane_transform(HudPaneTransformState& state, J2DPane* pane) {
    if (!state.active || state.pane != pane || pane == nullptr) {
        return;
    }

    if (nearly_equal(pane->getTranslateX(), state.appliedX) &&
        nearly_equal(pane->getTranslateY(), state.appliedY))
    {
        pane->translate(pane->getTranslateX() - state.offsetX,
            pane->getTranslateY() - state.offsetY);
    }
    if (nearly_equal(pane->getScaleX(), state.appliedScaleX) &&
        nearly_equal(pane->getScaleY(), state.appliedScaleY))
    {
        const f32 appliedScale = state.scale > 0.0f ? state.scale : 1.0f;
        pane->scale(pane->getScaleX() / appliedScale,
            pane->getScaleY() / appliedScale);
    }
}

void restore_applied_hud_pane_transform(const HudPaneSlot slot) {
    HudPaneTransformState& state = hud_pane_state(slot);
    remove_applied_hud_pane_transform(state, state.pane);
    state = {};
}

void restore_hud_layout_base() {
    for (std::size_t i = 0; i < static_cast<std::size_t>(HudPaneSlot::Count); ++i) {
        restore_applied_hud_pane_transform(static_cast<HudPaneSlot>(i));
    }
}

void clear_shared_hud_layout_cache() {
    s_wiiUHudPaneTransforms = {};
    s_hudTextBoxFlags = {};
    s_dpadArrowVisibility = {};
    s_dpadShadowVisibility = {};
    s_hdHudGlyphVisibility = {};
    s_hdHudGlyphDraw = nullptr;
    s_xyAmmoOriginalParams = {};
    s_xyAmmoOriginalValid = {};
    s_gaugeDraw = {};
    s_roundXYOverlayCount = 0;
    s_roundXYOverlayScreen = nullptr;
    s_roundPictureStates = {};
    s_roundHudMeter = nullptr;
    s_hudLayoutMeter = nullptr;
    s_hudLayoutScreen = nullptr;
}

J2DPane* pane_ptr(CPaneMgrAlpha* pane) {
    return pane != nullptr ? pane->getPanePtr() : nullptr;
}

J2DTextBox* text_box_ptr(CPaneMgr* pane) {
    J2DPane* j2dPane = pane_ptr(pane);
    if (j2dPane == nullptr || j2dPane->getTypeID() != 19) {
        return nullptr;
    }
    return static_cast<J2DTextBox*>(j2dPane);
}

void set_text_box_h_binding(J2DTextBox* textBox, const J2DTextBoxHBinding binding) {
    if (textBox == nullptr) {
        return;
    }
    textBox->mFlags = (textBox->mFlags & ~0x0C) | ((static_cast<u8>(binding) & 0x03) << 2);
}

J2DTextBoxHBinding hud_text_anchor_binding(const int textAnchor) {
    return textAnchor == kHudTextAnchorRight ? HBIND_LEFT : HBIND_RIGHT;
}

void apply_hud_text_box_binding(const HudPaneSlot slot, const std::size_t index,
    CPaneMgr* pane, const bool enabled, const int textAnchor) {
    if (index >= s_hudTextBoxFlags[static_cast<std::size_t>(slot)].size()) {
        return;
    }

    HudTextBoxFlagState& state =
        s_hudTextBoxFlags[static_cast<std::size_t>(slot)][index];
    J2DTextBox* textBox = text_box_ptr(pane);
    if (textBox == nullptr) {
        state = {};
        return;
    }

    // GameCube copies keep the native/HD HUD alignment, including centered
    // HD action labels. Position and scale edits still apply independently.
    if (!enabled || textAnchor == kHudTextAnchorOriginal) {
        if (state.active && state.textBox == textBox) {
            textBox->mFlags = (textBox->mFlags & ~0x0C) | (state.originalFlags & 0x0C);
        }
        state = {};
        return;
    }

    if (!state.active || state.textBox != textBox) {
        state = {
            .textBox = textBox,
            .originalFlags = textBox->mFlags,
            .active = true,
        };
    }

    set_text_box_h_binding(textBox, hud_text_anchor_binding(textAnchor));
}

void restore_hud_text_box_bindings() {
    for (auto& group : s_hudTextBoxFlags) {
        for (auto& state : group) {
            if (state.active && state.textBox != nullptr) {
                state.textBox->mFlags = (state.textBox->mFlags & ~0x0C) |
                    (state.originalFlags & 0x0C);
            }
            state = {};
        }
    }
}

void apply_hud_text_box_group_binding(const HudPaneSlot slot, CPaneMgr* const* panes,
    const std::size_t count, const bool enabled, const int textAnchor) {
    for (std::size_t i = 0; i < count; ++i) {
        apply_hud_text_box_binding(slot, i, panes[i], enabled, textAnchor);
    }
}

void apply_hud_xy_text_box_group_binding(const HudPaneSlot slot, CPaneMgr* panes[5][3],
    const std::size_t xySlot, const bool enabled, const int textAnchor) {
    for (std::size_t i = 0; i < 5; ++i) {
        apply_hud_text_box_binding(slot, i, panes[i][xySlot], enabled, textAnchor);
    }
}

void apply_hud_pane_transform(HudPaneTransformState& state, J2DPane* pane, const bool enabled,
    const f32 offsetX, const f32 offsetY, const f32 scale) {
    if (pane == nullptr || scale <= 0.0f) {
        state = {};
        return;
    }

    remove_applied_hud_pane_transform(state, pane);

    if (!enabled) {
        state = {};
        return;
    }

    const u8 originalTextFlags = state.originalTextFlags;
    const bool hasOriginalTextFlags = state.hasOriginalTextFlags;
    const f32 baseX = pane->getTranslateX();
    const f32 baseY = pane->getTranslateY();
    const f32 baseScaleX = pane->getScaleX();
    const f32 baseScaleY = pane->getScaleY();
    pane->translate(baseX + offsetX, baseY + offsetY);
    pane->scale(baseScaleX * scale, baseScaleY * scale);

    state = {
        .pane = pane,
        .offsetX = offsetX,
        .offsetY = offsetY,
        .scale = scale,
        .appliedX = pane->getTranslateX(),
        .appliedY = pane->getTranslateY(),
        .appliedScaleX = pane->getScaleX(),
        .appliedScaleY = pane->getScaleY(),
        .originalTextFlags = originalTextFlags,
        .hasOriginalTextFlags = hasOriginalTextFlags,
        .active = true,
    };
}

void apply_hud_pane_transform(const HudPaneSlot slot, CPaneMgr* pane, const bool enabled,
    const f32 offsetX, const f32 offsetY, const f32 scale) {
    apply_hud_pane_transform(hud_pane_state(slot), pane_ptr(pane), enabled, offsetX, offsetY,
        scale);
}

void apply_hud_pane_transform(const HudPaneSlot slot, CPaneMgrAlpha* pane, const bool enabled,
    const f32 offsetX, const f32 offsetY, const f32 scale) {
    apply_hud_pane_transform(hud_pane_state(slot), pane_ptr(pane), enabled, offsetX, offsetY,
        scale);
}

// HD HUD reparents the secondary item picture (bottle glass / bow) beside
// the primary picture. Native children already inherit the editor transform.
void apply_hud_item_secondary_transform(const HudPaneSlot slot, J2DPane* primary,
    J2DPane* secondary, const bool enabled, const f32 offsetX, const f32 offsetY,
    const f32 scale) {
    const bool sibling = primary != nullptr && secondary != nullptr &&
        primary->getParentPane() != nullptr &&
        secondary->getParentPane() == primary->getParentPane();
    apply_hud_pane_transform(hud_pane_state(slot), secondary,
        enabled && sibling, offsetX, offsetY, scale);
}

void apply_hud_text_pane_transform(const HudPaneSlot slot, CPaneMgr* pane, const bool enabled,
    const f32 offsetX, const f32 offsetY, const f32 scale, const int) {
    HudPaneTransformState& state = hud_pane_state(slot);
    apply_hud_pane_transform(state, pane_ptr(pane), enabled, offsetX, offsetY, scale);
}

void apply_dpad_text_layout(dMeter2Draw_c* meter) {
    if (meter == nullptr) {
        return;
    }

    const bool enabled = custom_hud_layout_enabled();
    const DuskModHudTransform itemsTransform = hud_layout_dpad_items_text_transform();
    apply_hud_pane_transform(HudPaneSlot::DPadItemsText, meter->mpTextI, enabled,
        itemsTransform.offset_x, itemsTransform.offset_y, itemsTransform.scale);
    const DuskModHudTransform mapTransform = hud_layout_dpad_map_text_transform();
    apply_hud_pane_transform(HudPaneSlot::DPadMapText, meter->mpTextM, enabled,
        mapTransform.offset_x, mapTransform.offset_y, mapTransform.scale);
}

void apply_hud_pane_hidden_state(
    HudPaneVisibilityState& state, J2DPane* pane, const bool hidden) {
    if (pane == nullptr) {
        state = {};
        return;
    }

    if (hidden) {
        if (!state.active || state.pane != pane) {
            state = {.pane = pane, .wasVisible = pane->isVisible(), .active = true};
        }
        pane->hide();
        return;
    }

    if (state.active && state.pane == pane) {
        if (state.wasVisible) {
            pane->show();
        } else {
            pane->hide();
        }
    }
    state = {};
}

template <std::size_t N>
void apply_hud_pane_group_hidden_state(J2DScreen* screen, const std::array<u64, N>& tags,
    std::array<HudPaneVisibilityState, N>& states, const bool hidden) {
    for (std::size_t i = 0; i < N; ++i) {
        apply_hud_pane_hidden_state(
            states[i], screen != nullptr ? screen->search(tags[i]) : nullptr, hidden);
    }
}

void apply_dpad_detail_visibility(dMeter2Draw_c* meter) {
    constexpr std::array<u64, 4> arrowTags = {
        MULTI_CHAR('yaji_00'), MULTI_CHAR('yaji_01'),
        MULTI_CHAR('yaji_02'), MULTI_CHAR('yaji_03'),
    };
    constexpr std::array<u64, 4> shadowTags = {
        MULTI_CHAR('ju_ring1'), MULTI_CHAR('ju_ring2'),
        MULTI_CHAR('ju_ring3'), MULTI_CHAR('ju_ring4'),
    };

    J2DScreen* screen = meter != nullptr ? meter->mpScreen : nullptr;
    const bool custom = custom_hud_layout_enabled();
    apply_hud_pane_group_hidden_state(
        screen, arrowTags, s_dpadArrowVisibility, custom && hud_custom_dpad_hide_arrows());
    apply_hud_pane_group_hidden_state(
        screen, shadowTags, s_dpadShadowVisibility, custom && hud_custom_dpad_hide_shadows());
}

void apply_health_bar_layout(dMeter2Draw_c* meter) {
    if (meter == nullptr || meter->getMainScreenPtr() == nullptr ||
        meter->mpLifeParts[0] == nullptr || meter->mpLifeParts[1] == nullptr ||
        meter->mpLifeParts[9] == nullptr || meter->mpLifeParts[10] == nullptr)
    {
        return;
    }

    J2DPane* secondRow = meter->getMainScreenPtr()->search(MULTI_CHAR('heart_un'));
    const f32 spacingX = meter->mpLifeParts[1]->getInitGlobalCenterPosX() -
                         meter->mpLifeParts[0]->getInitGlobalCenterPosX();
    const f32 offsetX = meter->mpLifeParts[9]->getInitGlobalCenterPosX() + spacingX -
                        meter->mpLifeParts[10]->getInitGlobalCenterPosX();
    const f32 offsetY = meter->mpLifeParts[0]->getInitGlobalCenterPosY() -
                        meter->mpLifeParts[10]->getInitGlobalCenterPosY();
    const bool enabled = custom_hud_layout_enabled() && hud_custom_health_bar_enabled();
    apply_hud_pane_transform(hud_pane_state(HudPaneSlot::HealthBar), secondRow,
        enabled, offsetX, offsetY, 1.0f);

    const u16 drawnLife = dComIfGp_getItemNowLife();
    const int currentHeart = drawnLife == 0 ? -1 : (static_cast<int>(drawnLife) - 1) / 4;
    apply_hud_pane_transform(HudPaneSlot::HealthBarCurrentHeart, meter->mpBigHeart,
        enabled && currentHeart >= 10, offsetX, offsetY, 1.0f);
}

void apply_midna_hud_layout(dMeter2Draw_c* meter) {
    if (meter == nullptr) {
        return;
    }
    // Apply to the final native/HD/Dawnlight Midna anchor independently of who
    // styles Midna. The regular restore pass prevents accumulation.
    const DuskModHudTransform midnaTransform = hud_layout_midna_transform();
    apply_hud_pane_transform(HudPaneSlot::Midna, meter->mpButtonMidona,
        hardcoded_hud_layout_enabled(), midnaTransform.offset_x, midnaTransform.offset_y,
        midnaTransform.scale);
}

void apply_tears_of_light_hud_layout(dMeter2Draw_c* meter) {
    if (meter == nullptr) {
        return;
    }
    // Transform the complete vessel after native presentation / HUD mods.
    // Leave its visibility, collection progress and animation state untouched.
    const DuskModHudTransform transform = hud_layout_tears_of_light_transform();
    apply_hud_pane_transform(HudPaneSlot::TearsOfLight, meter->mpLightDropParent,
        hardcoded_hud_layout_enabled(), transform.offset_x, transform.offset_y, transform.scale);
}

void apply_wii_u_hud_layout(dMeter2Draw_c* meter) {
    if (meter == nullptr) {
        return;
    }

    const bool enabled = hardcoded_hud_layout_enabled();

    const DuskModHudTransform aTransform = hud_layout_a_transform();
    const DuskModHudButtonLayout aLayout = hud_layout_a_button_layout();
    apply_hud_pane_transform(HudPaneSlot::ButtonA, meter->mpButtonA, enabled,
        aTransform.offset_x, aTransform.offset_y, aTransform.scale);
    apply_hud_text_pane_transform(HudPaneSlot::TextA, meter->mpTextA, enabled,
        aTransform.offset_x + aLayout.text_offset_x,
        aTransform.offset_y + aLayout.text_offset_y, hud_text_scale(aTransform, aLayout),
        aLayout.text_anchor);
    apply_hud_text_box_group_binding(
        HudPaneSlot::TextA, meter->mpAText, 5, enabled, aLayout.text_anchor);

    const DuskModHudTransform bTransform = hud_layout_b_transform();
    const DuskModHudButtonLayout bLayout = hud_layout_b_button_layout();
    const int bVariant = hud_button_b_item_variant(meter);
    const HudLayoutOffset bItemAnchor =
        hud_item_anchor_delta(bLayout, g_drawHIO.mButtonBItemPosX[bVariant],
            g_drawHIO.mButtonBItemPosY[bVariant]);
    const f32 bItemOffsetX = bTransform.offset_x + bLayout.item_offset_x + bItemAnchor.x;
    const f32 bItemOffsetY = bTransform.offset_y + bLayout.item_offset_y + bItemAnchor.y;
    apply_hud_pane_transform(HudPaneSlot::ButtonB, meter->mpButtonB, enabled,
        bTransform.offset_x, bTransform.offset_y, bTransform.scale);
    apply_hud_pane_transform(HudPaneSlot::ItemB, meter->mpItemB, enabled, bItemOffsetX,
        bItemOffsetY, hud_item_scale(bTransform, bLayout));
    apply_hud_pane_transform(HudPaneSlot::LightB, meter->mpLightB, enabled, bItemOffsetX,
        bItemOffsetY, hud_item_scale(bTransform, bLayout));
    apply_hud_text_pane_transform(HudPaneSlot::TextB, meter->mpTextB, enabled,
        bTransform.offset_x + bLayout.text_offset_x,
        bTransform.offset_y + bLayout.text_offset_y, hud_text_scale(bTransform, bLayout),
        bLayout.text_anchor);
    apply_hud_text_box_group_binding(
        HudPaneSlot::TextB, meter->mpBText, 5, enabled, bLayout.text_anchor);

    const DuskModHudTransform xTransform = hud_layout_x_transform();
    const DuskModHudButtonLayout xLayout = hud_layout_x_button_layout();
    const HudLayoutOffset xItemAnchor =
        hud_item_anchor_delta(xLayout, g_drawHIO.mButtonXItemBasePosX[0],
            g_drawHIO.mButtonXItemBasePosY[0]);
    const f32 xItemOffsetX = xTransform.offset_x + xLayout.item_offset_x + xItemAnchor.x;
    const f32 xItemOffsetY = xTransform.offset_y + xLayout.item_offset_y + xItemAnchor.y;
    apply_hud_pane_transform(HudPaneSlot::ButtonX, meter->mpButtonXY[0], enabled,
        xTransform.offset_x, xTransform.offset_y, xTransform.scale);
    apply_hud_pane_transform(HudPaneSlot::ItemX, meter->mpItemXY[0], enabled, xItemOffsetX,
        xItemOffsetY, hud_item_scale(xTransform, xLayout));
    apply_hud_pane_transform(HudPaneSlot::LightX, meter->mpLightXY[0], enabled, xItemOffsetX,
        xItemOffsetY, hud_item_scale(xTransform, xLayout));
    apply_hud_text_pane_transform(HudPaneSlot::TextX, meter->mpTextXY[0], enabled,
        xTransform.offset_x + xLayout.text_offset_x,
        xTransform.offset_y + xLayout.text_offset_y, hud_text_scale(xTransform, xLayout),
        xLayout.text_anchor);
    apply_hud_xy_text_box_group_binding(
        HudPaneSlot::TextX, meter->mpXYText, 0, enabled, xLayout.text_anchor);

    const DuskModHudTransform yTransform = hud_layout_y_transform();
    const DuskModHudButtonLayout yLayout = hud_layout_y_button_layout();
    const HudLayoutOffset yItemAnchor =
        hud_item_anchor_delta(yLayout, g_drawHIO.mButtonYItemBasePosX[0],
            g_drawHIO.mButtonYItemBasePosY[0]);
    const f32 yItemOffsetX = yTransform.offset_x + yLayout.item_offset_x + yItemAnchor.x;
    const f32 yItemOffsetY = yTransform.offset_y + yLayout.item_offset_y + yItemAnchor.y;
    apply_hud_pane_transform(HudPaneSlot::ButtonY, meter->mpButtonXY[1], enabled,
        yTransform.offset_x, yTransform.offset_y, yTransform.scale);
    apply_hud_pane_transform(HudPaneSlot::ItemY, meter->mpItemXY[1], enabled, yItemOffsetX,
        yItemOffsetY, hud_item_scale(yTransform, yLayout));
    apply_hud_pane_transform(HudPaneSlot::LightY, meter->mpLightXY[1], enabled, yItemOffsetX,
        yItemOffsetY, hud_item_scale(yTransform, yLayout));
    apply_hud_text_pane_transform(HudPaneSlot::TextY, meter->mpTextXY[1], enabled,
        yTransform.offset_x + yLayout.text_offset_x,
        yTransform.offset_y + yLayout.text_offset_y, hud_text_scale(yTransform, yLayout),
        yLayout.text_anchor);
    apply_hud_xy_text_box_group_binding(
        HudPaneSlot::TextY, meter->mpXYText, 1, enabled, yLayout.text_anchor);

    apply_hud_item_secondary_transform(HudPaneSlot::ItemXSecondary,
        pane_ptr(meter->mpItemXY[0]), meter->mpItemXYPane[0], enabled,
        xItemOffsetX, xItemOffsetY, hud_item_scale(xTransform, xLayout));
    apply_hud_item_secondary_transform(HudPaneSlot::ItemYSecondary,
        pane_ptr(meter->mpItemXY[1]), meter->mpItemXYPane[1], enabled,
        yItemOffsetX, yItemOffsetY, hud_item_scale(yTransform, yLayout));
    apply_midna_hud_layout(meter);
    apply_tears_of_light_hud_layout(meter);

    const DuskModHudTransform backingTransform = hud_layout_backing_transform();
    apply_hud_pane_transform(HudPaneSlot::Backing, meter->mpUzu, enabled,
        backingTransform.offset_x, backingTransform.offset_y, backingTransform.scale);
    const DuskModHudTransform dpadTransform = hud_layout_dpad_transform();
    apply_hud_pane_transform(HudPaneSlot::DPad, meter->mpButtonCrossParent, enabled,
        dpadTransform.offset_x, dpadTransform.offset_y, dpadTransform.scale);
    apply_dpad_text_layout(meter);
    apply_dpad_detail_visibility(meter);
    const DuskModHudTransform heartsTransform = hud_layout_hearts_transform();
    apply_hud_pane_transform(HudPaneSlot::Hearts, meter->mpLifeParent, enabled,
        heartsTransform.offset_x, heartsTransform.offset_y, heartsTransform.scale);
    apply_health_bar_layout(meter);
    const DuskModHudTransform rupeesTransform = hud_layout_rupees_transform();
    J2DPane* rupeeIcon = meter->mpScreen != nullptr ?
        meter->mpScreen->search(MULTI_CHAR('rupi')) : nullptr;
    const bool usesSharedRupeeParent = rupeeIcon != nullptr &&
        meter->mpRupeeKeyParent != nullptr &&
        rupeeIcon->getParentPane() == meter->mpRupeeKeyParent->getPanePtr();
    apply_hud_pane_transform(hud_pane_state(HudPaneSlot::RupeeIcon), rupeeIcon,
        enabled && usesSharedRupeeParent, rupeesTransform.offset_x,
        rupeesTransform.offset_y, rupeesTransform.scale);
    apply_hud_pane_transform(HudPaneSlot::Rupee0, meter->mpRupeeParent[0],
        enabled && !usesSharedRupeeParent,
        rupeesTransform.offset_x, rupeesTransform.offset_y, rupeesTransform.scale);
    apply_hud_pane_transform(HudPaneSlot::Rupee1, meter->mpRupeeParent[1],
        enabled && !usesSharedRupeeParent,
        rupeesTransform.offset_x, rupeesTransform.offset_y, rupeesTransform.scale);
    apply_hud_pane_transform(HudPaneSlot::Rupee2, meter->mpRupeeParent[2],
        enabled && !usesSharedRupeeParent,
        rupeesTransform.offset_x, rupeesTransform.offset_y, rupeesTransform.scale);
    const DuskModHudTransform keysTransform = hud_layout_keys_transform();
    apply_hud_pane_transform(HudPaneSlot::Keys, meter->mpKeyParent, enabled,
        keysTransform.offset_x, keysTransform.offset_y, keysTransform.scale);
}

void apply_xy_ammo_layout(dMeter2Draw_c* meter) {
    s_xyAmmoOriginalValid.fill(false);
    if (!hardcoded_hud_layout_enabled() || meter == nullptr) {
        return;
    }

    for (int slot = dMeter2Draw_c::SELECT_X_e; slot <= dMeter2Draw_c::SELECT_Y_e; ++slot) {
        if (meter->mpItemXY[slot] == nullptr) {
            continue;
        }

        s_xyAmmoOriginalParams[slot] = meter->mItemParams[slot];
        s_xyAmmoOriginalValid[slot] = true;

        const DuskModHudTransform transform = hud_layout_xy_transform(slot);
        const DuskModHudButtonLayout layout = hud_layout_xy_button_layout(slot);
        meter->mItemParams[slot].num_pos_x += layout.ammo_offset_x;
        meter->mItemParams[slot].num_pos_y += layout.ammo_offset_y;
        meter->mItemParams[slot].num_scale *= hud_ammo_scale(transform, layout);
    }
}

void restore_xy_ammo_layout(dMeter2Draw_c* meter) {
    if (meter != nullptr) {
        for (int slot = dMeter2Draw_c::SELECT_X_e; slot <= dMeter2Draw_c::SELECT_Y_e; ++slot) {
            if (s_xyAmmoOriginalValid[slot]) {
                meter->mItemParams[slot] = s_xyAmmoOriginalParams[slot];
            }
        }
    }
    s_xyAmmoOriginalValid.fill(false);
}


void apply_hud_backing_visibility(dMeter2Draw_c* meter) {
    if (meter == nullptr || meter->mpUzu == nullptr || meter->mpButtonParent == nullptr) {
        return;
    }

    if (!hud_button_backing_visible()) {
        meter->mpUzu->setAlphaRate(0.0f);
        return;
    }

    meter->mpUzu->setAlphaRate(
        meter->mButtonBaseAlpha * meter->mpButtonParent->getAlphaRate());
}

void apply_wii_u_minimap_layout(dMeterMap_c* map) {
    if (!hardcoded_hud_layout_enabled() || map == nullptr) {
        return;
    }

    s_wiiUMinimapTransform = {
        .map = map,
        .drawPosY = map->mDrawPosY,
        .sizeW = map->mSizeW,
        .sizeH = map->mSizeH,
        .active = true,
    };

    const DuskModHudTransform transform = hud_layout_minimap_transform();
    map->mDrawPosY += transform.offset_y;
    map->mSizeW *= transform.scale;
    map->mSizeH *= transform.scale;
}

void restore_wii_u_minimap_layout(dMeterMap_c* map) {
    if (!s_wiiUMinimapTransform.active || s_wiiUMinimapTransform.map != map) {
        s_wiiUMinimapTransform = {};
        return;
    }

    map->mDrawPosY = s_wiiUMinimapTransform.drawPosY;
    map->mSizeW = s_wiiUMinimapTransform.sizeW;
    map->mSizeH = s_wiiUMinimapTransform.sizeH;
    s_wiiUMinimapTransform = {};
}

HookAction before_minimap_picture_draw(ModContext*, void* args, void*, void*) {
    const auto& state = s_wiiUMinimapTransform;
    if (!state.active || state.map == nullptr ||
        mods::arg<J2DPicture*>(args, 0) != state.map->mMapJ2DPicture)
    {
        return HOOK_CONTINUE;
    }

    // draw() advances presentation interpolation and then recomputes X. Edit
    // only the final picture argument, leaving native slide state/targets intact.
    const DuskModHudTransform transform = hud_layout_minimap_transform();
    f32& x = mods::arg_ref<f32>(args, 1);
    x += transform.offset_x;
    if (transform.slide_direction == kHudSlideRightToLeft) {
        // Native X includes the slide twice (directly and via the edge getter).
        // Reflect that displacement about the fully open position.
        x -= 4.0f * state.map->mSlidePositionOffset;
    }
    return HOOK_CONTINUE;
}

bool touch_menu_or_pause_context() {
    const u8 windowStatus = dMeter2Info_getWindowStatus();
    return windowStatus != 0 || dMeter2Info_getPauseStatus() != 0 || dComIfGp_isPauseFlag() ||
           dComIfGp_event_runCheck() || dMeter2Info_isShopTalkFlag() ||
           dMsgObject_isTalkNowCheck();
}

HookAction before_meter_delete(ModContext*, void*, void*, void*) {
    clear_shared_hud_layout_cache();
    return HOOK_CONTINUE;
}

HookAction before_meter_draw_restore_hud(ModContext*, void* args, void*, void*) {
    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
    J2DScreen* screen = meter != nullptr ? meter->mpScreen : nullptr;
    if (meter != s_hudLayoutMeter || screen != s_hudLayoutScreen) {
        clear_shared_hud_layout_cache();
        s_hudLayoutMeter = meter;
        s_hudLayoutScreen = screen;
        return HOOK_CONTINUE;
    }

    // Restore before native/HD presentation snapshots the next frame. This
    // also lets Original follow live HUD changes after an explicit anchor.
    restore_hud_text_box_bindings();
    restore_hud_layout_base();
    return HOOK_CONTINUE;
}

bool twilight_hd_hud_active() {
    if (!s_hdHudConfigLookupAttempted) {
        s_hdHudConfigLookupAttempted = true;
        void* address = nullptr;
        if (svc_hook != nullptr && svc_hook->resolve != nullptr &&
            svc_hook->resolve(mod_ctx, "dusk::config::GetConfigVar", &address, nullptr) == MOD_OK)
        {
            s_hdHudGetConfigVar = reinterpret_cast<HudGetConfigVarFn>(address);
        }
    }
    // Look up the variable each draw; another mod can unregister/re-register it.
    // HUD styling is independent of HD HUD's optional third-item-slot setting.
    const auto* enabled = s_hdHudGetConfigVar ?
        s_hdHudGetConfigVar("mod.org_twilight_hd__hud.enabled") : nullptr;
    return enabled != nullptr &&
        static_cast<const dusk::config::ConfigVar<bool>*>(enabled)->getValue();
}

HookAction before_hd_hud_glyph_draw(ModContext*, void* args, void*, void*) {
    auto* screen = mods::arg<J2DScreen*>(args, 0);
    if (s_hdHudGlyphDraw != nullptr || s_hudLayoutMeter == nullptr || screen == nullptr ||
        screen != s_hudLayoutScreen || !twilight_hd_hud_active())
    {
        return HOOK_CONTINUE;
    }

    s_hdHudGlyphDraw = args;
    // HD HUD's discs include their own letters. Its archive still contains the
    // stock letter pictures, displaced to the right. Moving the complete button
    // groups left brings these obsolete pictures back on screen unless hidden.
    const std::array<CPaneMgr*, 4> glyphs = {s_hudLayoutMeter->mpBTextA,
        s_hudLayoutMeter->mpBTextB, s_hudLayoutMeter->mpBTextXY[0],
        s_hudLayoutMeter->mpBTextXY[1]};
    for (std::size_t i = 0; i < glyphs.size(); ++i) {
        auto* pane = pane_ptr(glyphs[i]);
        if (pane == nullptr) continue;
        s_hdHudGlyphVisibility[i] = {pane, pane->isVisible(), true};
        pane->hide();
    }
    return HOOK_CONTINUE;
}

void after_hd_hud_glyph_draw(ModContext*, void* args, void*, void*) {
    // Unrelated/nested draws (including draws skipped by another mod) cannot
    // restore the outer HUD's letters early.
    if (s_hdHudGlyphDraw == nullptr || s_hdHudGlyphDraw != args) return;
    for (auto& state : s_hdHudGlyphVisibility) {
        if (state.active) {
            if (state.wasVisible) state.pane->show();
            else state.pane->hide();
        }
        state = {};
    }
    s_hdHudGlyphDraw = nullptr;
}

void suppress_round_button_overlays(J2DPane* pane, J2DPicture* base, J2DPane* glyph) {
    if (pane == nullptr || pane == glyph) return;

    if (auto* picture = as_picture(pane);
        picture != nullptr && picture != base && s_roundXYOverlayCount < s_roundXYOverlays.size())
    {
        auto& state = s_roundXYOverlays[s_roundXYOverlayCount++];
        state.picture = picture;
        std::array<JUtility::TColor, 4> transparent;
        for (std::size_t i = 0; i < state.corners.size(); ++i) {
            state.corners[i] = picture->corner(i);
            transparent[i] = state.corners[i];
            transparent[i].a = 0;
        }
        // Vertex alpha suppresses this artwork without hiding a letter or
        // replacement button nested beneath the picture in the pane tree.
        picture->setCornerColor(transparent[0], transparent[1], transparent[2], transparent[3]);
    }
    for (auto* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        suppress_round_button_overlays(child, base, glyph);
    }
}

HookAction before_round_xy_screen_draw(ModContext*, void* args, void*, void*) {
    auto* screen = mods::arg<J2DScreen*>(args, 0);
    if (!round_xy_buttons_enabled() || s_hudLayoutMeter == nullptr || screen == nullptr ||
        screen != s_hudLayoutScreen)
    {
        return HOOK_CONTINUE;
    }

    s_roundXYOverlayCount = 0;
    s_roundXYOverlayScreen = screen;
    for (std::size_t i = 0; i < 2; ++i) {
        auto* button = s_hudLayoutMeter->mpButtonXY[i];
        auto* label = s_hudLayoutMeter->mpBTextXY[i];
        auto* root = button != nullptr ? button->getPanePtr() : nullptr;
        auto* base = first_picture_pane(root);
        auto* glyph = label != nullptr ? label->getPanePtr() : nullptr;
        // Do not alter an incomplete group if its base or letter is absent.
        if (base != nullptr && glyph != nullptr) {
            suppress_round_button_overlays(root, base, glyph);
        }
    }
    return HOOK_CONTINUE;
}

void after_round_xy_screen_draw(ModContext*, void* args, void*, void*) {
    if (mods::arg<J2DScreen*>(args, 0) != s_roundXYOverlayScreen) return;
    for (std::size_t i = 0; i < s_roundXYOverlayCount; ++i) {
        const auto& state = s_roundXYOverlays[i];
        state.picture->setCornerColor(
            state.corners[0], state.corners[1], state.corners[2], state.corners[3]);
    }
    s_roundXYOverlayCount = 0;
    s_roundXYOverlayScreen = nullptr;
}

HookAction before_meter_draw(ModContext*, void* args, void*, void*) {
    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);

    apply_round_xy_buttons(meter);
    return HOOK_CONTINUE;
}

void after_meter_draw_restore_xy_ammo(ModContext*, void* args, void*, void*) {
    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
    // Unwind X/Y before normal-priority HUD mods restore their own snapshots.
    restore_xy_ammo_layout(meter);
}

HookAction before_meter_screen_draw_restore_hud(ModContext*, void* args, void*, void*) {
    auto* screen = mods::arg<J2DScreen*>(args, 0);
    if (s_hudLayoutMeter != nullptr && screen == s_hudLayoutScreen) {
        // Presentation and other HUD mods can update the same panes between
        // dMeter2Draw_c::draw and the final screen draw. Give them the native
        // base instead of a transform that already contains Dawnlight's layout.
        restore_xy_ammo_layout(s_hudLayoutMeter);
        restore_hud_layout_base();
    }
    return HOOK_CONTINUE;
}

HookAction before_meter_screen_draw_apply_hud(ModContext*, void* args, void*, void*) {
    auto* screen = mods::arg<J2DScreen*>(args, 0);
    if (s_hudLayoutMeter != nullptr && screen == s_hudLayoutScreen) {
        // Apply after Dusklight's presentation pass and normal-priority HUD
        // mods, immediately before the gameplay HUD is drawn.
        apply_wii_u_hud_layout(s_hudLayoutMeter);
        // Other HUD mods must snapshot the unedited ammo values first. Applying
        // at MeterDraw entry makes their post-hook retain our offset every frame.
        apply_xy_ammo_layout(s_hudLayoutMeter);
        apply_hud_backing_visibility(s_hudLayoutMeter);
    }
    return HOOK_CONTINUE;
}

void after_meter_draw_kantera(ModContext*, void* args, void*, void*) {
    if (hardcoded_hud_layout_enabled()) {
        auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
        if (meter != nullptr) {
            const DuskModHudTransform transform = hud_layout_oil_transform();
            meter->field_0x5cc[1] *= transform.scale;
            meter->field_0x5d8[1] *= transform.scale;
        }
    }
}

void after_meter_draw_oxygen(ModContext*, void* args, void*, void*) {
    if (hardcoded_hud_layout_enabled()) {
        auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
        if (meter != nullptr) {
            const DuskModHudTransform transform = hud_layout_oxygen_transform();
            meter->field_0x5cc[2] *= transform.scale;
            meter->field_0x5d8[2] *= transform.scale;
        }
    }
}

HookAction before_meter_gauge_screen(ModContext*, void* args, void*, void*) {
    s_gaugeDraw = {};
    const u8 type = mods::arg<u8>(args, 1);
    if (type == 1 || type == 2) {
        s_gaugeDraw.meter = mods::arg<dMeter2Draw_c*>(args, 0);
        s_gaugeDraw.type = type;
    }
    return HOOK_CONTINUE;
}

HookAction before_gauge_screen_draw(ModContext*, void* args, void*, void*) {
    auto* screen = mods::arg<J2DScreen*>(args, 0);
    auto* meter = s_gaugeDraw.meter;
    if (!hardcoded_hud_layout_enabled() || meter == nullptr ||
        screen != meter->mpKanteraScreen || meter->mpMagicParent == nullptr ||
        s_gaugeDraw.pane != nullptr)
    {
        return HOOK_CONTINUE;
    }

    J2DPane* pane = meter->mpMagicParent->getPanePtr();
    if (pane == nullptr) {
        return HOOK_CONTINUE;
    }

    const DuskModHudTransform transform = s_gaugeDraw.type == 1 ?
        hud_layout_oil_transform() : hud_layout_oxygen_transform();
    s_gaugeDraw.pane = pane;
    s_gaugeDraw.x = pane->getTranslateX();
    s_gaugeDraw.y = pane->getTranslateY();
    pane->translate(s_gaugeDraw.x + transform.offset_x,
        s_gaugeDraw.y + transform.offset_y);
    return HOOK_CONTINUE;
}

void after_gauge_screen_draw(ModContext*, void*, void*, void*) {
    if (s_gaugeDraw.pane != nullptr) {
        s_gaugeDraw.pane->translate(s_gaugeDraw.x, s_gaugeDraw.y);
        s_gaugeDraw.pane = nullptr;
    }
}

void after_meter_gauge_screen(ModContext*, void*, void*, void*) {
    s_gaugeDraw = {};
}

void after_meter_move_button_cross(ModContext*, void* args, void*, void*) {
    auto* meter = mods::arg<dMeter2_c*>(args, 0);
    if (!hardcoded_hud_layout_enabled() || meter == nullptr || meter->mpMeterDraw == nullptr) {
        return;
    }

    const DuskModHudTransform dpadTransform = hud_layout_dpad_transform();
    if (dpadTransform.parent_mode != kHudParentIndependent) {
        return;
    }

    meter->field_0x1b4 = 0;
    meter->mPresentationTargets.crossX = meter->mButtonCrossOFFPosX;
    meter->mPresentationTargets.crossY = meter->mButtonCrossOFFPosY;
}

HookAction before_meter_map_draw(ModContext*, void* args, void*, void*) {
    apply_wii_u_minimap_layout(mods::arg<dMeterMap_c*>(args, 0));
    return HOOK_CONTINUE;
}

void after_meter_map_draw(ModContext*, void* args, void*, void*) {
    restore_wii_u_minimap_layout(mods::arg<dMeterMap_c*>(args, 0));
}

HookAction before_midna_talk_trigger(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<const daAlink_c*>(args, 0);
    if (link == nullptr) {
        return HOOK_CONTINUE;
    }

    const bool touchTriggered = consume_midna_touch_press();

    if (dawnlight_touch_ui_active() && touchTriggered) {
        *static_cast<BOOL*>(retval) = TRUE;
        return HOOK_SKIP_ORIGINAL;
    }

    return HOOK_CONTINUE;
}

#include "native_shortcut_compat.inc"

#if defined(__ANDROID__)
bool map_l_touch_override_needed() {
    const auto windowStatus = dMeter2Info_getWindowStatus();
    // dMw_c uses 4 for the field map and 5 for the dungeon map, including
    // their opening/closing animations. Other menus keep the host behavior.
    return dawnlight_touch_ui_active() && (windowStatus == 4 || windowStatus == 5);
}

void sync_map_l_touch_override() {
    if (TouchSetControlOverrideHook::g_orig == nullptr) {
        return;
    }

    const bool mapActive = map_l_touch_override_needed();
    if (mapActive || s_mapLTouchOverrideActive) {
        // Action keeps L visible and uses momentary input instead of target
        // locking. Apply before sync_visual_state can hide/release the button.
        TouchSetControlOverrideHook::g_orig(dusk::ui::Control::L,
            mapActive ? dusk::ui::ControlOverride::Action : s_hostLTouchOverride);
    }
    s_mapLTouchOverrideActive = mapActive;
}

HookAction before_touch_set_control_override(ModContext*, void* args, void*, void*) {
    if (mods::arg<dusk::ui::Control>(args, 0) == dusk::ui::Control::L) {
        auto& requested = mods::arg_ref<dusk::ui::ControlOverride>(args, 1);
        // Remember the latest host/other-mod request so closing the map does
        // not clear an L action that belongs to another menu.
        s_hostLTouchOverride = requested;
        s_mapLTouchOverrideActive = map_l_touch_override_needed();
        if (s_mapLTouchOverrideActive) {
            requested = dusk::ui::ControlOverride::Action;
        }
    }
    return HOOK_CONTINUE;
}

HookAction before_touch_sync_visual_state(ModContext*, void*, void*, void*) {
    sync_map_l_touch_override();
    return HOOK_CONTINUE;
}

HookAction before_pad_set_virtual_status(ModContext*, void* args, void*, void*) {
    if (!dawnlight_touch_ui_active() || mods::arg<u32>(args, 0) != PAD_1) {
        return HOOK_CONTINUE;
    }

    auto* status = const_cast<PADStatus*>(mods::arg<const PADStatus*>(args, 1));
    s_touchMapLRawHeld = status != nullptr && (status->button & PAD_TRIGGER_L) != 0;
    return HOOK_CONTINUE;
}

HookAction before_pad_clear_virtual_status(ModContext*, void* args, void*, void*) {
    if (mods::arg<u32>(args, 0) == PAD_1) {
        s_touchMapLRawHeld = false;
    }
    return HOOK_CONTINUE;
}

void observe_map_touch_input(ModContext*, void*, void*, void*) {
    // Observe the host's accepted input before TPHD's Fixed mode rebuilds L/R
    // from physical triggers. Requiring both sources respects input blocking
    // and prevents a physical L press from becoming a second portal shortcut.
    const auto& pad = mDoCPd_c::getCpadInfo(PAD_1);
    const bool held = dawnlight_touch_ui_active() && dMeter2Info_getWindowStatus() == 4 &&
        s_touchMapLRawHeld && (pad.mButtonFlags & PAD_TRIGGER_L) != 0;
    s_touchMapLPressed = held && !s_touchMapLHeld;
    s_touchMapLHeld = held;
}

HookAction before_touch_fmap_move(ModContext*, void*, void*, void*) {
    if (!dawnlight_touch_ui_active() || dMeter2Info_getWindowStatus() != 4) {
        return HOOK_CONTINUE;
    }

    // TPHD maps only physical L to native Z and clears other Z input here.
    // Add touch L after that mapping, scoped to the field-map update only.
    auto& pad = mDoCPd_c::getCpadInfo(PAD_1);
    s_touchMapZHeldOriginal = pad.mButtonFlags & PAD_TRIGGER_Z;
    s_touchMapZPressedOriginal = pad.mPressedButtonFlags & PAD_TRIGGER_Z;
    s_touchMapPortalInputActive = true;
    if (s_touchMapLHeld) pad.mButtonFlags |= PAD_TRIGGER_Z;
    if (s_touchMapLPressed) pad.mPressedButtonFlags |= PAD_TRIGGER_Z;
    s_touchMapLPressed = false;
    return HOOK_CONTINUE;
}

void after_touch_fmap_move(ModContext*, void*, void*, void*) {
    if (!s_touchMapPortalInputActive) return;
    auto& pad = mDoCPd_c::getCpadInfo(PAD_1);
    pad.mButtonFlags = (pad.mButtonFlags & ~PAD_TRIGGER_Z) | s_touchMapZHeldOriginal;
    pad.mPressedButtonFlags =
        (pad.mPressedButtonFlags & ~PAD_TRIGGER_Z) | s_touchMapZPressedOriginal;
    s_touchMapPortalInputActive = false;
}

#endif

HookAction before_touch_set_control_pressed(ModContext*, void* args, void*, void*) {
    const auto control = mods::arg<dusk::ui::Control>(args, 1);
    const bool pressed = mods::arg<bool>(args, 2);
    if (control == dusk::ui::Control::R) fierce_deity_touch_button(PAD_TRIGGER_R, pressed);
    if (control == dusk::ui::Control::A) fierce_deity_touch_button(PAD_BUTTON_A, pressed);
    if (control == dusk::ui::Control::Z) {
        fierce_deity_touch_button(PAD_TRIGGER_Z, pressed);
        return HOOK_CONTINUE;
    }
    return HOOK_CONTINUE;
}

ModResult add_hook(ModResult result, ModError* error) {
    return result == MOD_OK ? MOD_OK :
        mods::set_error(error, result, "failed to install Dawnlight HUD and touch hooks");
}

}  // namespace

bool midna_touch_button_available() { return midna_touch_button_ready(); }
std::string midna_touch_button_icon() {
#if defined(__ANDROID__)
    return true_midna_icon_source();
#else
    return {};
#endif
}

ModResult install_hud_touch_hooks(ModError* error) {
#if defined(__ANDROID__)
    s_dawnlightTouchUiSessionEnabled = dawnlight_touch_ui_enabled();
#else
    s_dawnlightTouchUiSessionEnabled = false;
#endif

    HookOptions sharedHudRestoreOptions = HOOK_OPTIONS_INIT;
    sharedHudRestoreOptions.priority = 100;
    HookOptions sharedHudApplyOptions = HOOK_OPTIONS_INIT;
    sharedHudApplyOptions.priority = -100;
    HookOptions minimapPreOptions = HOOK_OPTIONS_INIT;
    minimapPreOptions.priority = -100;
    HookOptions minimapPostOptions = HOOK_OPTIONS_INIT;
    minimapPostOptions.priority = 100;
    HookOptions hdGlyphPreOptions = HOOK_OPTIONS_INIT;
    hdGlyphPreOptions.priority = -101;
    HookOptions hdGlyphPostOptions = HOOK_OPTIONS_INIT;
    hdGlyphPostOptions.priority = 101;
    HookOptions finalGaugePreOptions = HOOK_OPTIONS_INIT;
    finalGaugePreOptions.priority = -100;
    HookOptions finalGaugePostOptions = HOOK_OPTIONS_INIT;
    finalGaugePostOptions.priority = 100;
    HookOptions touchInputObserveOptions = HOOK_OPTIONS_INIT;
    touchInputObserveOptions.priority = 100;
    HookOptions touchInputApplyOptions = HOOK_OPTIONS_INIT;
    touchInputApplyOptions.priority = -100;

    ModResult result = MOD_OK;

    if (result == MOD_OK) {
        result = mods::hook_add_pre<MeterDeleteHook>(svc_hook, before_meter_delete);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<MeterDrawHook>(
            svc_hook, before_meter_draw_restore_hud, &sharedHudRestoreOptions);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<MeterDrawHook>(
            svc_hook, before_meter_draw, &sharedHudApplyOptions);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterDrawHook>(
            svc_hook, after_meter_draw_restore_xy_ammo, &sharedHudRestoreOptions);
    }

    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterDrawKanteraHook>(svc_hook, after_meter_draw_kantera);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterDrawOxygenHook>(svc_hook, after_meter_draw_oxygen);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<MeterGaugeScreenHook>(svc_hook, before_meter_gauge_screen);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<ScreenDrawHook>(
            svc_hook, before_meter_screen_draw_restore_hud, &sharedHudRestoreOptions);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<ScreenDrawHook>(
            svc_hook, before_meter_screen_draw_apply_hud, &sharedHudApplyOptions);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<ScreenDrawHook>(
            svc_hook, before_round_xy_screen_draw, &sharedHudApplyOptions);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<ScreenDrawHook>(
            svc_hook, before_hd_hud_glyph_draw, &hdGlyphPreOptions);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<ScreenDrawHook>(
            svc_hook, after_hd_hud_glyph_draw, &hdGlyphPostOptions);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<ScreenDrawHook>(
            svc_hook, after_round_xy_screen_draw, &sharedHudRestoreOptions);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<ScreenDrawHook>(
            svc_hook, before_gauge_screen_draw, &finalGaugePreOptions);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<ScreenDrawHook>(
            svc_hook, after_gauge_screen_draw, &finalGaugePostOptions);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterGaugeScreenHook>(svc_hook, after_meter_gauge_screen);
    }

    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterMoveButtonCrossHook>(
            svc_hook, after_meter_move_button_cross, &sharedHudApplyOptions);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<MeterMapDrawHook>(
            svc_hook, before_meter_map_draw, &minimapPreOptions);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterMapDrawHook>(
            svc_hook, after_meter_map_draw, &minimapPostOptions);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<MinimapPictureDrawHook>(
            svc_hook, before_minimap_picture_draw, &minimapPreOptions);
    }

    if (result == MOD_OK && s_dawnlightTouchUiSessionEnabled) {
        result = mods::hook_add_pre<MidnaTalkTriggerHook>(
            svc_hook, before_midna_talk_trigger, &touchInputObserveOptions);
    }

    if (result == MOD_OK) {
        result = mods::hook_add_pre<NativeQuickTransformHook>(svc_hook, before_native_shortcut);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<NativeQuickTransformHook>(svc_hook, after_native_shortcut);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<NativeWolfHowlHook>(svc_hook, before_native_shortcut);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<NativeWolfHowlHook>(svc_hook, after_native_shortcut);
    }
#if defined(__ANDROID__)
    if (result == MOD_OK && s_dawnlightTouchUiSessionEnabled) {
        // Optional lookup: avoid depending on UserSettings offsets in forks.
        (void)resolve_touch_symbol("dusk::config::GetConfigVar", s_touchGetConfigVar);
    }
    if (result == MOD_OK && s_dawnlightTouchUiSessionEnabled) {
        result = mods::hook_add_pre<PadClearVirtualStatusHook>(
            svc_hook, before_pad_clear_virtual_status, &touchInputObserveOptions);
    }
    if (result == MOD_OK && s_dawnlightTouchUiSessionEnabled) {
        result = mods::hook_add_post<PadReadHook>(
            svc_hook, observe_map_touch_input, &touchInputObserveOptions);
    }
    if (result == MOD_OK && s_dawnlightTouchUiSessionEnabled) {
        result = mods::hook_add_pre<TouchFmapMoveHook>(
            svc_hook, before_touch_fmap_move, &touchInputApplyOptions);
    }
    if (result == MOD_OK && s_dawnlightTouchUiSessionEnabled) {
        result = mods::hook_add_post<TouchFmapMoveHook>(
            svc_hook, after_touch_fmap_move, &touchInputObserveOptions);
    }
    if (result == MOD_OK && s_dawnlightTouchUiSessionEnabled) {
        result = mods::hook_add_pre<TouchSetControlOverrideHook>(
            svc_hook, before_touch_set_control_override);
    }
    if (result == MOD_OK && s_dawnlightTouchUiSessionEnabled) {
        result = mods::hook_add_pre<TouchSyncVisualStateHook>(
            svc_hook, before_touch_sync_visual_state);
    }
    if (result == MOD_OK && s_dawnlightTouchUiSessionEnabled) {
        result = mods::hook_add_pre<PadSetVirtualStatusHook>(
            svc_hook, before_pad_set_virtual_status, &touchInputObserveOptions);
    }

    if (result == MOD_OK && s_dawnlightTouchUiSessionEnabled) {
        result = mods::hook::install<MidnaIconSourceHook>(svc_hook);
    }
    if (result == MOD_OK && s_dawnlightTouchUiSessionEnabled) {
        result = mods::hook_add_pre<UpdateMidnaIconTextureHook>(svc_hook, before_midna_icon_capture);
    }
    if (result == MOD_OK && s_dawnlightTouchUiSessionEnabled) {
        result = mods::hook_add_post<UpdateMidnaIconTextureHook>(svc_hook, after_midna_icon_capture);
    }
    if (result == MOD_OK) {
        // Fierce Deity shortcuts also need native touch input when our custom
        // touch layout is off. This observer never changes the host UI input.
        result = mods::hook_add_pre<TouchSetControlPressedHook>(
            svc_hook, before_touch_set_control_pressed);
    }
#endif

    const ModResult hookResult = add_hook(result, error);
    if (hookResult == MOD_OK && svc_log != nullptr) {
        svc_log->info(mod_ctx, s_dawnlightTouchUiSessionEnabled ?
            "Dawnlight Touch UI enabled; touch hooks installed" :
            "Dawnlight Touch UI disabled; touch hooks skipped");
    }
    return hookResult;
}

void shutdown_hud_touch_hooks() {
    s_hdHudGetConfigVar = nullptr;
    s_hdHudConfigLookupAttempted = false;
    clear_shared_hud_layout_cache();
    s_dawnlightTouchUiSessionEnabled = false;
    s_dpadArrowVisibility = {};
    s_dpadShadowVisibility = {};
    after_native_shortcut(nullptr, nullptr, nullptr, nullptr);
#if defined(__ANDROID__)
    s_touchGetConfigVar = nullptr;
    sync_map_l_touch_override();
    after_touch_fmap_move(nullptr, nullptr, nullptr, nullptr);
    s_touchMapLRawHeld = false;
    s_touchMapLHeld = false;
    s_touchMapLPressed = false;
    s_hostLTouchOverride = dusk::ui::ControlOverride::Default;
    s_mapLTouchOverrideActive = false;
#endif
}

}  // namespace dawnlight
