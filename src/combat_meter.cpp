#include "combat_meter.hpp"

#include "hud_layout.hpp"

#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter_HIO.h"
#include "d/d_meter2_draw.h"
#include "d/d_pane_class.h"

#include <algorithm>
#include <cmath>

namespace dawnlight {
namespace {

// The meter panes include transparent edge padding, so slightly overlap their
// bounds to produce the intended visible spacing after HUD scaling.
constexpr float kHeartPaneOverlap = 6.0f;
constexpr float kRowPaneOverlap = 6.0f;
constexpr float kAdditionalRowLiftPixels = 5.0f;

struct PaneState {
    CPaneMgr* pane = nullptr;
    float posX = 0.0f;
    float posY = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float alpha = 1.0f;
    bool visible = false;
};

PaneState save_pane(CPaneMgr* pane) {
    if (pane == nullptr || pane->getPanePtr() == nullptr) {
        return {};
    }
    return {
        pane,
        pane->getPosX(),
        pane->getPosY(),
        pane->getTranslateX(),
        pane->getTranslateY(),
        pane->getSizeX(),
        pane->getSizeY(),
        pane->getScaleX(),
        pane->getScaleY(),
        pane->getAlphaRate(),
        pane->getPanePtr()->isVisible(),
    };
}

void restore_pane(const PaneState& state) {
    if (state.pane == nullptr || state.pane->getPanePtr() == nullptr) {
        return;
    }
    state.pane->move(state.posX, state.posY);
    state.pane->translate(state.x, state.y);
    state.pane->resize(state.width, state.height);
    state.pane->scale(state.scaleX, state.scaleY);
    state.pane->setAlphaRate(state.alpha);
    if (state.visible) {
        state.pane->getPanePtr()->show();
    } else {
        state.pane->getPanePtr()->hide();
    }
}

struct ScreenBounds {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    bool valid = false;
};

ScreenBounds pane_screen_bounds(CPaneMgr* pane) {
    if (pane == nullptr || pane->getPanePtr() == nullptr) {
        return {};
    }

    Mtx matrix;
    const Vec first = pane->getGlobalVtx(&matrix, 0, false, 0);
    const Vec last = pane->getGlobalVtx(&matrix, 3, false, 0);
    return {
        .left = std::min(first.x, last.x),
        .top = std::min(first.y, last.y),
        .right = std::max(first.x, last.x),
        .bottom = std::max(first.y, last.y),
        .valid = true,
    };
}

void include_bounds(ScreenBounds& result, const ScreenBounds& bounds) {
    if (!bounds.valid) {
        return;
    }
    if (!result.valid) {
        result = bounds;
        return;
    }
    result.left = std::min(result.left, bounds.left);
    result.top = std::min(result.top, bounds.top);
    result.right = std::max(result.right, bounds.right);
    result.bottom = std::max(result.bottom, bounds.bottom);
}

ScreenBounds full_heart_bounds(dMeter2Draw_c* meter) {
    ScreenBounds result;
    for (CPaneMgr* heart : meter->mpLifeParts) {
        if (heart == nullptr || heart->getPanePtr() == nullptr) {
            continue;
        }
        include_bounds(result, pane_screen_bounds(heart));
    }
    return result;
}

ScreenBounds untransformed_heart_bounds(dMeter2Draw_c* meter) {
    const PaneState lifeParent = save_pane(meter->mpLifeParent);
    const DuskModHudTransform heartsTransform = hud_layout_hearts_transform();
    const float heartsScale = std::max(heartsTransform.scale, 0.001f);

    meter->mpLifeParent->translate(
        lifeParent.x - heartsTransform.offset_x, lifeParent.y - heartsTransform.offset_y);
    meter->mpLifeParent->scale(
        lifeParent.scaleX / heartsScale, lifeParent.scaleY / heartsScale);

    J2DPane* secondRow = meter->getMainScreenPtr() != nullptr ?
        meter->getMainScreenPtr()->search(MULTI_CHAR('heart_un')) : nullptr;
    float secondRowX = 0.0f;
    float secondRowY = 0.0f;
    const bool undoHealthBar = secondRow != nullptr && custom_hud_layout_enabled() &&
                               hud_custom_health_bar_enabled() &&
                               meter->mpLifeParts[0] != nullptr &&
                               meter->mpLifeParts[1] != nullptr &&
                               meter->mpLifeParts[9] != nullptr &&
                               meter->mpLifeParts[10] != nullptr;
    if (undoHealthBar) {
        secondRowX = secondRow->getTranslateX();
        secondRowY = secondRow->getTranslateY();
        const float spacingX = meter->mpLifeParts[1]->getInitGlobalCenterPosX() -
                               meter->mpLifeParts[0]->getInitGlobalCenterPosX();
        const float offsetX = meter->mpLifeParts[9]->getInitGlobalCenterPosX() + spacingX -
                              meter->mpLifeParts[10]->getInitGlobalCenterPosX();
        const float offsetY = meter->mpLifeParts[0]->getInitGlobalCenterPosY() -
                              meter->mpLifeParts[10]->getInitGlobalCenterPosY();
        secondRow->translate(secondRowX - offsetX, secondRowY - offsetY);
    }

    const ScreenBounds result = full_heart_bounds(meter);
    if (undoHealthBar) {
        secondRow->translate(secondRowX, secondRowY);
    }
    restore_pane(lifeParent);
    return result;
}

DuskModHudTransform combat_meter_transform(CombatMeterStyle style) {
    return style == CombatMeterStyle::Stamina ? hud_layout_stamina_bar_transform() :
                                                hud_layout_fierce_deity_bar_transform();
}

ScreenBounds combat_meter_bounds(dMeter2Draw_c* meter) {
    ScreenBounds result;
    include_bounds(result, pane_screen_bounds(meter->mpMagicBase));
    include_bounds(result, pane_screen_bounds(meter->mpMagicFrameL));
    include_bounds(result, pane_screen_bounds(meter->mpMagicMeter));
    include_bounds(result, pane_screen_bounds(meter->mpMagicFrameR));
    return result;
}

}  // namespace

void draw_combat_meter(
    dMeter2Draw_c* meter, float percentage, CombatMeterStyle style, int row)
{
    if (meter == nullptr || meter->mpKanteraScreen == nullptr ||
        meter->mpMagicParent == nullptr || meter->mpMagicBase == nullptr ||
        meter->mpMagicFrameL == nullptr || meter->mpMagicMeter == nullptr ||
        meter->mpMagicFrameR == nullptr || meter->mpLifeParent == nullptr)
    {
        return;
    }

    const PaneState parent = save_pane(meter->mpMagicParent);
    const PaneState base = save_pane(meter->mpMagicBase);
    const PaneState frameL = save_pane(meter->mpMagicFrameL);
    const PaneState fill = save_pane(meter->mpMagicMeter);
    const PaneState frameR = save_pane(meter->mpMagicFrameR);
    auto* fillPicture = static_cast<J2DPicture*>(meter->mpMagicMeter->getPanePtr());
    const JUtility::TColor oldBlack = fillPicture->getBlack();
    const JUtility::TColor oldWhite = fillPicture->getWhite();

    const float fullWidth = meter->mpMagicMeter->getInitSizeX();
    const float frameSpan =
        meter->mpMagicFrameR->getInitPosX() - meter->mpMagicFrameL->getInitPosX();

    meter->mpMagicParent->getPanePtr()->show();
    meter->mpMagicBase->getPanePtr()->show();
    meter->mpMagicFrameL->getPanePtr()->show();
    meter->mpMagicMeter->getPanePtr()->show();
    meter->mpMagicFrameR->getPanePtr()->show();
    meter->mpMagicParent->setAlphaRate(1.0f);
    meter->mpMagicBase->setAlphaRate(1.0f);
    meter->mpMagicFrameL->setAlphaRate(1.0f);
    meter->mpMagicMeter->setAlphaRate(1.0f);
    meter->mpMagicFrameR->setAlphaRate(1.0f);

    if (style == CombatMeterStyle::Stamina) {
        meter->mpMagicMeter->setBlackWhite(
            JUtility::TColor(100, 255, 100, 255), JUtility::TColor(0, 210, 0, 255));
    } else {
        meter->mpMagicMeter->setBlackWhite(
            JUtility::TColor(255, 100, 100, 255), JUtility::TColor(210, 0, 0, 255));
    }

    const float fillRatio = std::clamp(percentage, 0.0f, 100.0f) / 100.0f;
    meter->mpMagicMeter->resize(
        fullWidth * fillRatio, meter->mpMagicMeter->getInitSizeY());
    meter->mpMagicFrameR->move(frameSpan + meter->mpMagicFrameL->getInitPosX(),
        meter->mpMagicFrameL->getInitPosY());
    meter->mpMagicBase->resize(
        meter->mpMagicBase->getInitSizeX(), meter->mpMagicBase->getInitSizeY());

    const DuskModHudTransform meterTransform = combat_meter_transform(style);
    const DuskModHudTransform heartsTransform = hud_layout_hearts_transform();
    const float heartsScale = std::max(heartsTransform.scale, 0.001f);
    const float lifeBaseScale = std::max(g_drawHIO.mLifeParentScale, 0.001f);
    const float relativeScale = g_drawHIO.mMagicMeterScale / lifeBaseScale;
    meter->mpMagicParent->scale(
        meter->mpLifeParent->getScaleX() / heartsScale * relativeScale * meterTransform.scale,
        meter->mpLifeParent->getScaleY() / heartsScale * relativeScale * meterTransform.scale);
    meter->mpMagicParent->paneTrans(parent.x, parent.y);

    const ScreenBounds hearts = untransformed_heart_bounds(meter);
    const ScreenBounds frame = pane_screen_bounds(meter->mpMagicFrameL);
    const ScreenBounds meterBounds = combat_meter_bounds(meter);
    if (hearts.valid && frame.valid && meterBounds.valid) {
        const float lifeScaleY = std::abs(meter->mpLifeParent->getScaleY()) / heartsScale;
        const float meterScaleY = std::abs(meter->mpMagicParent->getScaleY());
        const float gap = -kHeartPaneOverlap * lifeScaleY;
        const float rowStep =
            meterBounds.bottom - meterBounds.top - kRowPaneOverlap * meterScaleY;
        const int normalizedRow = std::max(row, 0);
        const float targetTop = hearts.bottom + gap + normalizedRow * rowStep -
                                normalizedRow * kAdditionalRowLiftPixels;
        meter->mpMagicParent->paneTrans(
            parent.x + hearts.left - frame.left + meterTransform.offset_x,
            parent.y + targetTop - frame.top + meterTransform.offset_y);
    }

    J2DGrafContext* graf = dComIfGp_getCurrentGrafPort();
    meter->mpKanteraScreen->draw(0.0f, 0.0f, graf);

    fillPicture->setBlackWhite(oldBlack, oldWhite);
    restore_pane(frameR);
    restore_pane(fill);
    restore_pane(frameL);
    restore_pane(base);
    restore_pane(parent);
}

}  // namespace dawnlight
