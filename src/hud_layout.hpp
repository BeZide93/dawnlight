#pragma once

#include "config.hpp"

#include <cstdint>

namespace dawnlight {

struct DuskModHudTransform {
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    float scale = 1.0f;
    std::uint32_t flags = 0;
    int parent_mode = 0;
    int slide_direction = 0;
};

struct DuskModHudButtonLayout {
    float item_scale = 1.0f;
    float item_offset_x = 0.0f;
    float item_offset_y = 0.0f;
    float ammo_scale = 1.0f;
    float ammo_offset_x = 0.0f;
    float ammo_offset_y = 0.0f;
    float text_scale = 1.0f;
    float text_offset_x = 0.0f;
    float text_offset_y = 0.0f;
    int item_anchor = 0;
    int default_item_anchor = 0;
    int text_anchor = 0;
    std::uint32_t style_flags = 0;
};

constexpr int kHudItemAnchorLeft = 0;
constexpr int kHudItemAnchorRight = 1;
constexpr int kHudItemAnchorTop = 2;
constexpr int kHudItemAnchorBottom = 3;
constexpr int kHudTextAnchorLeft = 0;
constexpr int kHudTextAnchorRight = 1;
constexpr int kHudParentIndependent = 1;
constexpr int kHudSlideLeftToRight = 1;
constexpr int kHudSlideRightToLeft = 2;

inline float hud_percent_scale(const int percent) {
    return static_cast<float>(percent) / 100.0f;
}

inline DuskModHudTransform hud_custom_element_transform(HudElement element) {
    return {
        .offset_x = static_cast<float>(hud_custom_element_x(element)),
        .offset_y = static_cast<float>(hud_custom_element_y(element)),
        .scale = hud_percent_scale(hud_custom_element_scale_percent(element)),
    };
}

inline DuskModHudButtonLayout hud_custom_button_layout(
    HudButton button, const int defaultItemAnchor) {
    return {
        .item_scale = hud_percent_scale(hud_custom_button_item_scale_percent(button)),
        .item_offset_x = static_cast<float>(hud_custom_button_item_offset_x(button)),
        .item_offset_y = static_cast<float>(hud_custom_button_item_offset_y(button)),
        .ammo_scale = hud_percent_scale(hud_custom_button_ammo_scale_percent(button)),
        .ammo_offset_x = static_cast<float>(hud_custom_button_ammo_offset_x(button)),
        .ammo_offset_y = static_cast<float>(hud_custom_button_ammo_offset_y(button)),
        .text_scale = hud_percent_scale(hud_custom_button_text_scale_percent(button)),
        .text_offset_x = static_cast<float>(hud_custom_button_text_offset_x(button)),
        .text_offset_y = static_cast<float>(hud_custom_button_text_offset_y(button)),
        .item_anchor = hud_custom_button_item_anchor(button),
        .default_item_anchor = defaultItemAnchor,
        .text_anchor = hud_custom_button_text_anchor(button),
    };
}

inline DuskModHudTransform hud_layout_a_transform() {
    switch (hud_layout()) {
    case HudLayout::XBox:
        return {
            .offset_x = -35.0f,
            .offset_y = 25.0f,
            .scale = 1.0f,
        };
    case HudLayout::WiiU:
        return {
            .offset_x = -5.0f,
            .offset_y = -3.0f,
            .scale = 1.0f,
        };
    case HudLayout::Dawnlight:
        return {
            .offset_x = -135.0f,
            .offset_y = 25.0f,
            .scale = 1.0f,
        };
    case HudLayout::Custom:
        return hud_custom_element_transform(HudElement::A);
    case HudLayout::GameCube:
    default:
        return {};
    }
}

inline DuskModHudButtonLayout hud_layout_a_button_layout() {
    if (custom_hud_layout_enabled()) {
        return hud_custom_button_layout(HudButton::A, kHudItemAnchorRight);
    }
    if (hud_layout() == HudLayout::WiiU) {
        return {
            .text_scale = 1.0f,
            .text_offset_x = 200.0f,
            .text_offset_y = 0.0f,
            .text_anchor = kHudTextAnchorRight,
        };
    }
    if (hud_layout() == HudLayout::XBox) {
        return {
            .text_scale = 1.0f,
            .text_offset_x = 200.0f,
            .text_offset_y = 0.0f,
            .text_anchor = kHudTextAnchorRight,
        };
    }
    if (hardcoded_hud_layout_enabled()) {
        return {
            .text_scale = 1.0f,
            .text_offset_x = 200.0f,
            .text_offset_y = 0.0f,
            .text_anchor = kHudTextAnchorRight,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_b_transform() {
    switch (hud_layout()) {
    case HudLayout::XBox:
        return {
            .offset_x = 20.0f,
            .offset_y = -27.0f,
            .scale = 1.49f,
        };
    case HudLayout::WiiU:
        return {
            .offset_x = -11.0f,
            .offset_y = 5.0f,
            .scale = 1.5f,
        };
    case HudLayout::Dawnlight:
        return {
            .offset_x = -80.0f,
            .offset_y = -27.0f,
            .scale = 1.5f,
        };
    case HudLayout::Custom:
        return hud_custom_element_transform(HudElement::B);
    case HudLayout::GameCube:
    default:
        return {};
    }
}

inline DuskModHudButtonLayout hud_layout_b_button_layout() {
    if (custom_hud_layout_enabled()) {
        return hud_custom_button_layout(HudButton::B, kHudItemAnchorRight);
    }
    if (hud_layout() == HudLayout::WiiU) {
        return {
            .item_scale = 0.5f,
            .item_offset_x = 30.0f,
            .item_offset_y = 0.0f,
            .text_scale = 0.5f,
            .text_offset_x = 160.0f,
            .text_offset_y = 0.0f,
            .item_anchor = kHudItemAnchorTop,
            .default_item_anchor = kHudItemAnchorRight,
            .text_anchor = kHudTextAnchorRight,
        };
    }
    if (hud_layout() == HudLayout::XBox) {
        return {
            .item_scale = 0.5f,
            .item_offset_x = 30.0f,
            .item_offset_y = 0.0f,
            .text_scale = 0.5f,
            .text_offset_x = 160.0f,
            .text_offset_y = 0.0f,
            .item_anchor = kHudItemAnchorTop,
            .default_item_anchor = kHudItemAnchorRight,
            .text_anchor = kHudTextAnchorRight,
        };
    }
    if (hardcoded_hud_layout_enabled()) {
        return {
            .item_scale = 0.5f,
            .item_offset_x = 30.0f,
            .item_offset_y = 0.0f,
            .text_scale = 0.5f,
            .text_offset_x = 160.0f,
            .text_offset_y = 0.0f,
            .item_anchor = kHudItemAnchorTop,
            .default_item_anchor = kHudItemAnchorRight,
            .text_anchor = kHudTextAnchorRight,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_x_transform() {
    switch (hud_layout()) {
    case HudLayout::XBox:
        return {
            .offset_x = -102.0f,
            .offset_y = -1.0f,
            .scale = 1.7000000476837158f,
        };
    case HudLayout::WiiU:
        return {
            .offset_x = -73.0f,
            .offset_y = -35.0f,
            .scale = 1.7000000476837158f,
        };
    case HudLayout::Dawnlight:
        return {
            .offset_x = -202.0f,
            .offset_y = -1.0f,
            .scale = 1.7000000476837158f,
        };
    case HudLayout::Custom:
        return hud_custom_element_transform(HudElement::X);
    case HudLayout::GameCube:
    default:
        return {};
    }
}

inline DuskModHudButtonLayout hud_layout_x_button_layout() {
    switch (hud_layout()) {
    case HudLayout::Custom:
        return hud_custom_button_layout(HudButton::X, kHudItemAnchorRight);
    case HudLayout::WiiU:
        return {
            .item_scale = 0.5f,
            .item_offset_x = -10.0f,
            .item_offset_y = -5.0f,
            .ammo_scale = 1.0f,
            .ammo_offset_x = 0.0f,
            .ammo_offset_y = 0.0f,
            .text_scale = 0.5f,
            .text_offset_x = 30.0f,
            .text_offset_y = -15.0f,
            .item_anchor = kHudItemAnchorTop,
            .default_item_anchor = kHudItemAnchorRight,
            .text_anchor = kHudTextAnchorLeft,
        };
    case HudLayout::XBox:
        return {
            .item_scale = 0.5f,
            .item_offset_x = -15.0f,
            .item_offset_y = -15.0f,
            .ammo_scale = 1.0f,
            .ammo_offset_x = 0.0f,
            .ammo_offset_y = 0.0f,
            .text_scale = 0.5f,
            .text_offset_x = 100.0f,
            .text_offset_y = -15.0f,
            .item_anchor = kHudItemAnchorLeft,
            .default_item_anchor = kHudItemAnchorRight,
            .text_anchor = kHudTextAnchorRight,
        };
    case HudLayout::Dawnlight:
        return {
            .item_scale = 0.5f,
            .item_offset_x = 0.0f,
            .item_offset_y = 0.0f,
            .ammo_scale = 1.0f,
            .ammo_offset_x = 0.0f,
            .ammo_offset_y = 0.0f,
            .text_scale = 0.5f,
            .text_offset_x = 100.0f,
            .text_offset_y = -15.0f,
            .item_anchor = kHudItemAnchorLeft,
            .default_item_anchor = kHudItemAnchorRight,
            .text_anchor = kHudTextAnchorRight,
        };
    case HudLayout::GameCube:
    default:
        return {};
    }
}

inline DuskModHudTransform hud_layout_y_transform() {
    switch (hud_layout()) {
    case HudLayout::XBox:
        return {
            .offset_x = -22.0f,
            .offset_y = 0.0f,
            .scale = 1.7000000476837158f,
        };
    case HudLayout::WiiU:
        return {
            .offset_x = -52.0f,
            .offset_y = 32.0f,
            .scale = 1.7000000476837158f,
        };
    case HudLayout::Dawnlight:
        return {
            .offset_x = -122.0f,
            .offset_y = 0.0f,
            .scale = 1.7000000476837158f,
        };
    case HudLayout::Custom:
        return hud_custom_element_transform(HudElement::Y);
    case HudLayout::GameCube:
    default:
        return {};
    }
}

inline DuskModHudButtonLayout hud_layout_y_button_layout() {
    if (custom_hud_layout_enabled()) {
        return hud_custom_button_layout(HudButton::Y, kHudItemAnchorLeft);
    }
    if (hud_layout() == HudLayout::WiiU) {
        return {
            .item_scale = 0.5f,
            .item_offset_x = 20.0f,
            .item_offset_y = 6.0f,
            .ammo_scale = 1.0f,
            .ammo_offset_x = 0.0f,
            .ammo_offset_y = 0.0f,
            .text_scale = 0.5f,
            .text_offset_x = 65.0f,
            .text_offset_y = 0.0f,
            .item_anchor = kHudItemAnchorLeft,
            .default_item_anchor = kHudItemAnchorLeft,
            .text_anchor = kHudTextAnchorLeft,
        };
    }
    if (hud_layout() == HudLayout::XBox) {
        return {
            .item_scale = 0.5f,
            .item_offset_x = 0.0f,
            .item_offset_y = 0.0f,
            .ammo_scale = 1.0f,
            .ammo_offset_x = 0.0f,
            .ammo_offset_y = 0.0f,
            .text_scale = 0.5f,
            .text_offset_x = 150.0f,
            .text_offset_y = 0.0f,
            .item_anchor = kHudItemAnchorTop,
            .default_item_anchor = kHudItemAnchorLeft,
            .text_anchor = kHudTextAnchorRight,
        };
    }
    if (hardcoded_hud_layout_enabled()) {
        return {
            .item_scale = 0.5f,
            .item_offset_x = 0.0f,
            .item_offset_y = 0.0f,
            .ammo_scale = 1.0f,
            .ammo_offset_x = 0.0f,
            .ammo_offset_y = 0.0f,
            .text_scale = 0.5f,
            .text_offset_x = 150.0f,
            .text_offset_y = 0.0f,
            .item_anchor = kHudItemAnchorTop,
            .default_item_anchor = kHudItemAnchorLeft,
            .text_anchor = kHudTextAnchorRight,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_z_transform() {
    switch (hud_layout()) {
    case HudLayout::XBox:
    case HudLayout::WiiU:
        return {
            .offset_x = 0.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    case HudLayout::Dawnlight:
        return {
            .offset_x = -100.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    case HudLayout::Custom:
        return hud_custom_element_transform(HudElement::Z);
    case HudLayout::GameCube:
    default:
        return {};
    }
}

inline DuskModHudTransform hud_layout_dpad_transform() {
    switch (hud_layout()) {
    case HudLayout::XBox:
    case HudLayout::WiiU:
        return {
            .offset_x = 0.0f,
            .offset_y = -280.0f,
            .scale = 1.0f,
            .parent_mode = kHudParentIndependent,
        };
    case HudLayout::Dawnlight:
        return {
            .offset_x = 0.0f,
            .offset_y = -15.0f,
            .scale = 1.0f,
            .parent_mode = kHudParentIndependent,
        };
    case HudLayout::Custom: {
        DuskModHudTransform transform = hud_custom_element_transform(HudElement::DPad);
        transform.parent_mode =
            hud_custom_dpad_follows_minimap() ? 0 : kHudParentIndependent;
        return transform;
    }
    case HudLayout::GameCube:
    default:
        return {};
    }
}

inline DuskModHudTransform hud_layout_dpad_items_text_transform() {
    return hud_layout() == HudLayout::Custom ?
               hud_custom_element_transform(HudElement::DPadItemsText) :
               DuskModHudTransform{};
}

inline DuskModHudTransform hud_layout_dpad_map_text_transform() {
    return hud_layout() == HudLayout::Custom ?
               hud_custom_element_transform(HudElement::DPadMapText) :
               DuskModHudTransform{};
}

inline DuskModHudTransform hud_layout_midna_transform() {
    if (hud_layout() == HudLayout::Custom) {
        return hud_custom_element_transform(HudElement::Midna);
    }
    if (hud_layout() == HudLayout::XBox || hud_layout() == HudLayout::WiiU) {
        return {
            .offset_x = -6.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudButtonLayout hud_layout_z_button_layout() {
    if (custom_hud_layout_enabled()) {
        return hud_custom_button_layout(HudButton::Z, kHudItemAnchorRight);
    }
    if (hud_layout() == HudLayout::WiiU) {
        return {
            .item_scale = 1.0f,
            .item_offset_x = 0.0f,
            .item_offset_y = 0.0f,
            .ammo_scale = 0.8f,
            .ammo_offset_x = 0.0f,
            .ammo_offset_y = -15.0f,
            .text_scale = 1.0f,
            .text_offset_x = 0.0f,
            .text_offset_y = 0.0f,
            .item_anchor = kHudItemAnchorRight,
            .default_item_anchor = kHudItemAnchorRight,
            .text_anchor = kHudTextAnchorLeft,
        };
    }
    if (hud_layout() == HudLayout::XBox) {
        return {
            .item_scale = 1.0f,
            .item_offset_x = 0.0f,
            .item_offset_y = 0.0f,
            .ammo_scale = 0.7f,
            .ammo_offset_x = 0.0f,
            .ammo_offset_y = -15.0f,
            .text_scale = 1.0f,
            .text_offset_x = 0.0f,
            .text_offset_y = 0.0f,
            .item_anchor = kHudItemAnchorRight,
            .default_item_anchor = kHudItemAnchorRight,
            .text_anchor = kHudTextAnchorLeft,
        };
    }
    if (hardcoded_hud_layout_enabled()) {
        return {
            .item_scale = 1.0f,
            .item_offset_x = 0.0f,
            .item_offset_y = 0.0f,
            .ammo_scale = 0.7f,
            .ammo_offset_x = 0.0f,
            .ammo_offset_y = -15.0f,
            .text_scale = 1.0f,
            .text_offset_x = 0.0f,
            .text_offset_y = 0.0f,
            .item_anchor = kHudItemAnchorRight,
            .default_item_anchor = kHudItemAnchorRight,
            .text_anchor = kHudTextAnchorLeft,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_backing_transform() {
    if (hud_layout() == HudLayout::Custom) {
        return hud_custom_element_transform(HudElement::ButtonBacking);
    }
    if (hardcoded_hud_layout_enabled()) {
        return {
            .offset_x = -100.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_hearts_transform() {
    if (hud_layout() == HudLayout::Custom) {
        return hud_custom_element_transform(HudElement::Hearts);
    }
    if (hud_layout() == HudLayout::Dawnlight) {
        return {
            .offset_x = 100.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_rupees_transform() {
    if (hud_layout() == HudLayout::Custom) {
        return hud_custom_element_transform(HudElement::Rupees);
    }
    if (hud_layout() == HudLayout::Dawnlight) {
        return {
            .offset_x = 40.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_keys_transform() {
    if (hud_layout() == HudLayout::Custom) {
        return hud_custom_element_transform(HudElement::Keys);
    }
    return {};
}

inline DuskModHudTransform hud_layout_oil_transform() {
    if (hud_layout() == HudLayout::Custom) {
        return hud_custom_element_transform(HudElement::Oil);
    }
    if (hud_layout() == HudLayout::Dawnlight) {
        return {
            .offset_x = 100.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_oxygen_transform() {
    if (hud_layout() == HudLayout::Custom) {
        return hud_custom_element_transform(HudElement::Oxygen);
    }
    if (hud_layout() == HudLayout::Dawnlight) {
        return {
            .offset_x = 100.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_minimap_transform() {
    switch (hud_layout()) {
    case HudLayout::XBox:
    case HudLayout::WiiU:
        return {
            .offset_x = 0.0f,
            .offset_y = 50.0f,
            .scale = 0.699999988079071f,
            .slide_direction = kHudSlideLeftToRight,
        };
    case HudLayout::Dawnlight:
        return {
            .offset_x = 730.0f,
            .offset_y = -190.0f,
            .scale = 0.699999988079071f,
            .slide_direction = kHudSlideRightToLeft,
        };
    case HudLayout::Custom: {
        DuskModHudTransform transform = hud_custom_element_transform(HudElement::Minimap);
        transform.slide_direction = hud_custom_minimap_slide_direction() == 0 ?
                                        kHudSlideLeftToRight :
                                        kHudSlideRightToLeft;
        return transform;
    }
    case HudLayout::GameCube:
    default:
        return {};
    }
}

}  // namespace dawnlight
