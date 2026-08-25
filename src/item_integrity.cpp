#include "config.hpp"
#include "service_imports.hpp"

#include "global.h"
#include "d/d_com_inf_game.h"
#include "d/d_item.h"
#include "d/d_save.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

namespace dawnlight {
namespace {

DEFINE_HOOK(&dSv_player_item_c::setItem, SetItem);
DEFINE_HOOK(&dSv_player_item_c::getItem, GetItem);
DEFINE_HOOK(&dSv_player_item_c::setBottleItemIn, SetBottleItemIn);
DEFINE_HOOK(&dSv_player_item_c::setEquipBottleItemIn, SetEquipBottleItemIn);

thread_local bool s_repairingMixPairs = false;

bool valid_slot(u8 slot) {
    return slot < MAX_ITEM_SLOTS;
}

bool valid_mix_pair(const dSv_player_item_c* items, u8 selectSlot, u8 mixSlot) {
    if (mixSlot == dItemNo_NONE_e) {
        return true;
    }
    if (items == nullptr || !valid_slot(selectSlot) || !valid_slot(mixSlot)) {
        return false;
    }

    const u8 selected = items->mItems[selectSlot];
    const u8 mixed = items->mItems[mixSlot];
    if (selected == dItemNo_BOW_e || mixed == dItemNo_BOW_e) {
        const u8 other = selected == dItemNo_BOW_e ? mixed : selected;
        return other == dItemNo_NORMAL_BOMB_e || other == dItemNo_WATER_BOMB_e ||
               other == dItemNo_POKE_BOMB_e || other == dItemNo_HAWK_EYE_e;
    }
    if (selected == dItemNo_FISHING_ROD_1_e || mixed == dItemNo_FISHING_ROD_1_e) {
        const u8 other = selected == dItemNo_FISHING_ROD_1_e ? mixed : selected;
        return other == dItemNo_BEE_CHILD_e || other == dItemNo_WORM_e ||
               other == dItemNo_ZORAS_JEWEL_e;
    }
    return false;
}

void repair_mix_pairs(dSv_player_item_c* items) {
    if (!item_integrity_fixes_enabled() || items == nullptr || s_repairingMixPairs) {
        return;
    }
    s_repairingMixPairs = true;
    for (int i = 0; i < SELECT_ITEM_NUM; ++i) {
        const u8 selected = dComIfGs_getSelectItemIndex(i);
        const u8 mixed = dComIfGs_getMixItemIndex(i);
        if (!valid_mix_pair(items, selected, mixed)) {
            dComIfGs_setMixItemIndex(i, dItemNo_NONE_e);
            dComIfGp_setSelectItem(i);
        }
    }
    s_repairingMixPairs = false;
}

void on_set_item_post(ModContext*, void* args, void*, void*) {
    repair_mix_pairs(mods::arg<dSv_player_item_c*>(args, 0));
}

HookAction on_get_item_pre(ModContext*, void* args, void*, void*) {
    repair_mix_pairs(const_cast<dSv_player_item_c*>(
        mods::arg<const dSv_player_item_c*>(args, 0)));
    return HOOK_CONTINUE;
}

thread_local int s_bottleIndex = -1;

HookAction on_set_bottle_pre(ModContext*, void* args, void*, void*) {
    s_bottleIndex = -1;
    if (!item_integrity_fixes_enabled()) {
        return HOOK_CONTINUE;
    }
    auto* items = mods::arg<dSv_player_item_c*>(args, 0);
    const u8 current = mods::arg<u8>(args, 1);
    for (int i = 0; i < dSv_player_item_c::BOTTLE_MAX; ++i) {
        if (items->mItems[SLOT_11 + i] == current) {
            s_bottleIndex = i;
            break;
        }
    }
    return HOOK_CONTINUE;
}

void on_set_bottle_post(ModContext*, void* args, void*, void*) {
    const u8 replacement = mods::arg<u8>(args, 2);
    if (s_bottleIndex >= 0 && replacement != dItemNo_BEE_CHILD_e) {
        dComIfGs_setBottleNum(static_cast<u8>(s_bottleIndex), 0);
    }
    s_bottleIndex = -1;
}

HookAction on_set_equipped_bottle_pre(ModContext*, void* args, void*, void*) {
    s_bottleIndex = -1;
    if (!item_integrity_fixes_enabled()) {
        return HOOK_CONTINUE;
    }
    const u8 selectIndex = mods::arg<u8>(args, 1);
    const u8 slot = dComIfGs_getSelectItemIndex(selectIndex);
    if (slot >= SLOT_11 && slot < SLOT_15) {
        s_bottleIndex = slot - SLOT_11;
    }
    return HOOK_CONTINUE;
}

ModResult check_hook(ModResult result, ModError* error) {
    return result == MOD_OK ? MOD_OK :
        mods::set_error(error, result, "failed to install Dawnlight item integrity hooks");
}

}  // namespace

ModResult install_item_integrity_hooks(ModError* error) {
    ModResult result = mods::hook_add_post<SetItem>(svc_hook, on_set_item_post);
    if (result == MOD_OK) {
        result = mods::hook_add_pre<GetItem>(svc_hook, on_get_item_pre);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<SetBottleItemIn>(svc_hook, on_set_bottle_pre);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<SetBottleItemIn>(svc_hook, on_set_bottle_post);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<SetEquipBottleItemIn>(svc_hook, on_set_equipped_bottle_pre);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<SetEquipBottleItemIn>(svc_hook, on_set_bottle_post);
    }
    return check_hook(result, error);
}

}  // namespace dawnlight
