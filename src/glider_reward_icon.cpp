#include "glider_reward_icon.hpp"
#include "glider_reward.hpp"
#include "service_imports.hpp"
#include "d/d_msg_scrn_item.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "JSystem/JUtility/JUTTexture.h"
#include "mods/svc/hook.hpp"
#include "mods/svc/resource.h"

#include <array>
#include <cstddef>
#include <cstring>

IMPORT_SERVICE(ResourceService, svc_resource);

namespace dawnlight {
namespace {
DEFINE_HOOK(&dMsgScrnItem_c::drawSelf, GliderRewardIconDraw);
constexpr unsigned kIconSize = 128;
struct alignas(32) IconTexture {
    ResTIMG header{};
    alignas(32) std::array<u8, kIconSize * kIconSize * 4> pixels{};
};
IconTexture s_icon;
bool s_ready = false;
unsigned s_drawDepth = 0;
struct IconDrawState {
    dMsgScrnItem_c* screen = nullptr;
    J2DPicture* panes[3]{};
    const ResTIMG* texture = nullptr;
    JUtility::TColor black, white;
    f32 width = 0, height = 0;
    int itemIndex = 0;
    bool mirror = false;
};
IconDrawState s_draw;

HookAction before_icon_draw(ModContext*, void* args, void*, void*) {
    if (++s_drawDepth != 1 || !s_ready || !glider_reward_message_active()) return HOOK_CONTINUE;
    auto* screen = mods::arg<dMsgScrnItem_c*>(args, 0);
    auto* pane = screen ? screen->mpItemPane[0] : nullptr;
    auto* texture = pane ? pane->getTexture(0) : nullptr;
    if (!texture || !texture->getTexInfo()) return HOOK_CONTINUE;
    s_draw.screen = screen;
    for (int i = 0; i < 3; ++i) s_draw.panes[i] = screen->mpItemPane[i];
    s_draw.texture = texture->getTexInfo();
    s_draw.black = pane->getBlack();
    s_draw.white = pane->getWhite();
    s_draw.width = screen->field_0x178;
    s_draw.height = screen->field_0x17c;
    s_draw.itemIndex = screen->mItemIndex;
    s_draw.mirror = screen->field_0x19e;
    pane->changeTexture(&s_icon.header, 0);
    pane->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(255, 255, 255, 255));
    // The custom message has no native item index. Use the normal 48px layout
    // footprint, independently of the icon's higher texture resolution, and
    // keep the native position, animation, alpha, HUD scaling and aspect ratio.
    screen->mItemIndex = -1;
    screen->field_0x178 = screen->field_0x170;
    screen->field_0x17c = screen->field_0x174;
    screen->field_0x19e = false;
    screen->mpItemPane[1] = screen->mpItemPane[2] = nullptr;
    return HOOK_CONTINUE;
}

void after_icon_draw(ModContext*, void*, void*, void*) {
    if (!s_drawDepth || --s_drawDepth != 0 || !s_draw.screen) return;
    auto* screen = s_draw.screen;
    auto* pane = s_draw.panes[0];
    pane->changeTexture(s_draw.texture, 0);
    pane->setBlackWhite(s_draw.black, s_draw.white);
    for (int i = 0; i < 3; ++i) screen->mpItemPane[i] = s_draw.panes[i];
    screen->field_0x178 = s_draw.width;
    screen->field_0x17c = s_draw.height;
    screen->mItemIndex = s_draw.itemIndex;
    screen->field_0x19e = s_draw.mirror;
    // Nothing in the native screen owns or retains our texture after drawing.
    // Its destructor and all unrelated item messages keep their original data.
    s_draw = {};
}
} // namespace

ModResult initialize_glider_reward_icon(ModError* error) {
    ResourceBuffer buffer = RESOURCE_BUFFER_INIT;
    const auto loaded = svc_resource->load(mod_ctx, "glider-item-icon.rgba8", &buffer);
    if (loaded != MOD_OK) return mods::set_error(error, loaded, "failed to load Glider item icon");
    const bool valid = buffer.data && buffer.size == s_icon.pixels.size();
    if (valid) std::memcpy(s_icon.pixels.data(), buffer.data, buffer.size);
    svc_resource->free(mod_ctx, &buffer);
    if (!valid) return mods::set_error(error, MOD_ERROR, "invalid Glider item icon size");
    auto& header = s_icon.header;
    header = {};
    header.format = GX_TF_RGBA8;
    header.alphaEnabled = 1;
    header.width = header.height = kIconSize;
    header.wrapS = header.wrapT = GX_CLAMP;
    header.minFilter = header.magFilter = GX_LINEAR;
    header.mipmapCount = 1;
    header.imageOffset = offsetof(IconTexture, pixels);
    auto result = mods::hook::add_pre<GliderRewardIconDraw>(svc_hook, before_icon_draw);
    if (result == MOD_OK) result = mods::hook::add_post<GliderRewardIconDraw>(svc_hook, after_icon_draw);
    s_ready = result == MOD_OK;
    return s_ready ? MOD_OK : mods::set_error(error, result, "failed to hook Glider item icon");
}

void shutdown_glider_reward_icon() {
    s_ready = false;
}
} // namespace dawnlight
