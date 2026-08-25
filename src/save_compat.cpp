#include "save_compat.hpp"

#include "config.hpp"
#include "service_imports.hpp"

#include "d/d_com_inf_game.h"
#include "d/d_item.h"
#include "d/d_save.h"
#include "d/d_stage.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

#include <cstring>

namespace dawnlight {
namespace {

DEFINE_HOOK(&dSv_info_c::card_to_memory, CardToMemory);
DEFINE_HOOK(&item_func_SWORD, ItemFuncSword);
DEFINE_HOOK(&item_func_WOOD_STICK, ItemFuncWoodStick);
DEFINE_HOOK(&item_func_WOOD_SHIELD, ItemFuncWoodShield);
DEFINE_HOOK(&item_getcheck_func_MASTER_SWORD, ItemGetCheckMasterSword);
DEFINE_HOOK(&item_getcheck_func_WEAR_KOKIRI, ItemGetCheckWearKokiri);
DEFINE_HOOK(&item_getcheck_func_HVY_BOOTS, ItemGetCheckHeavyBoots);
DEFINE_HOOK(&item_getcheck_func_KANTERA, ItemGetCheckLantern);
DEFINE_HOOK(&item_getcheck_func_PACHINKO, ItemGetCheckSlingshot);

constexpr size_t kReserveOffset = 0x8F0;
constexpr size_t kNgPlusCountOffset = 8;
constexpr size_t kIntroSkipOffset = 16;
constexpr char kNgPlusMagic[] = "DUSKNGP1";
constexpr char kIntroSkipMagic[] = "DUSKSKP1";

const u8* reserve_bytes(const dSv_save_c* save) {
    return save == nullptr ? nullptr : reinterpret_cast<const u8*>(save) + kReserveOffset;
}

void repair_faron_tears(dSv_save_c* save) {
    if (save == nullptr || !is_intro_skipped(save)) {
        return;
    }

    static constexpr u8 kFaronTearTboxes[] = {
        0, 1, 4, 5, 6, 8, 9, 11, 12, 13, 14, 17, 18, 20, 21, 23,
    };
    dSv_memBit_c& faron = save->getSave(dStage_SaveTbl_FARON).getBit();
    for (u8 tbox : kFaronTearTboxes) {
        faron.onTbox(tbox);
    }
}

void repair_ordon_gear(dSv_save_c* save) {
    if (save == nullptr || !is_new_game_plus(save) || is_intro_skipped(save)) {
        return;
    }

    dSv_player_c& player = save->getPlayer();
    dSv_player_status_a_c& status = player.getPlayerStatusA();
    dSv_memBit_c& ordon = save->getSave(dStage_SaveTbl_ORDON).getBit();
    dSv_event_c& event = save->getEvent();

    const bool swordQuestIncomplete = !event.isEventBit(dSv_event_flag_c::F_0363) &&
                                      !ordon.isSwitch(24);
    if (swordQuestIncomplete &&
        (player.getGetItem().isFirstBit(dItemNo_SWORD_e) ||
            player.getCollect().isCollect(COLLECT_SWORD, COLLECT_ORDON_SWORD)))
    {
        player.getGetItem().offFirstBit(dItemNo_SWORD_e);
        player.getCollect().offCollect(COLLECT_SWORD, COLLECT_ORDON_SWORD);
        if (status.getSelectEquip(COLLECT_SWORD) == dItemNo_SWORD_e) {
            status.setSelectEquip(COLLECT_SWORD,
                player.getGetItem().isFirstBit(dItemNo_MASTER_SWORD_e) ? dItemNo_MASTER_SWORD_e
                                                                       : dItemNo_NONE_e);
        }
    }

    const bool shieldQuestIncomplete = !event.isEventBit(dSv_event_flag_c::M_072) &&
                                       !ordon.isSwitch(26);
    if (shieldQuestIncomplete &&
        (player.getGetItem().isFirstBit(dItemNo_WOOD_SHIELD_e) ||
            player.getCollect().isCollect(COLLECT_SHIELD, COLLECT_WOODEN_SHIELD)))
    {
        player.getGetItem().offFirstBit(dItemNo_WOOD_SHIELD_e);
        player.getCollect().offCollect(COLLECT_SHIELD, COLLECT_WOODEN_SHIELD);
        if (status.getSelectEquip(COLLECT_SHIELD) == dItemNo_WOOD_SHIELD_e) {
            status.setSelectEquip(COLLECT_SHIELD,
                player.getGetItem().isFirstBit(dItemNo_HYLIA_SHIELD_e) ? dItemNo_HYLIA_SHIELD_e
                                                                       : dItemNo_NONE_e);
        }
    }
}

void on_card_to_memory_post(ModContext*, void*, void*, void*) {
    if (!save_compatibility_enabled()) {
        return;
    }
    dSv_save_c* save = dComIfGs_getSaveData();
    repair_ordon_gear(save);
    repair_faron_tears(save);
}

void restore_master_sword_post(ModContext*, void*, void*, void*) {
    dSv_save_c* save = dComIfGs_getSaveData();
    if (is_new_game_plus(save) && dComIfGs_isItemFirstBit(dItemNo_MASTER_SWORD_e)) {
        dComIfGs_setSelectEquipSword(dItemNo_MASTER_SWORD_e);
    }
}

void restore_hylian_shield_post(ModContext*, void*, void*, void*) {
    dSv_save_c* save = dComIfGs_getSaveData();
    if (is_new_game_plus(save) && dComIfGs_isItemFirstBit(dItemNo_HYLIA_SHIELD_e)) {
        dComIfGs_setSelectEquipShield(dItemNo_HYLIA_SHIELD_e);
    }
}

void hide_early_ngplus_item_post(ModContext*, void*, void* retval, void* eventBitPtr) {
    auto* result = static_cast<int*>(retval);
    const auto eventBit = static_cast<u16>(reinterpret_cast<uintptr_t>(eventBitPtr));
    if (result != nullptr && is_new_game_plus(dComIfGs_getSaveData()) &&
        !dComIfGs_isEventBit(eventBit))
    {
        *result = FALSE;
    }
}

template <class Entry>
ModResult add_post(HookPostFn callback, void* userdata = nullptr) {
    HookOptions options = HOOK_OPTIONS_INIT;
    options.userdata = userdata;
    return mods::hook_add_post<Entry>(svc_hook, callback, &options);
}

}  // namespace

bool is_new_game_plus(const dSv_save_c* save) {
    const u8* reserve = reserve_bytes(save);
    return reserve != nullptr && std::memcmp(reserve, kNgPlusMagic, sizeof(kNgPlusMagic) - 1) == 0;
}

bool is_intro_skipped(const dSv_save_c* save) {
    const u8* reserve = reserve_bytes(save);
    return reserve != nullptr &&
           std::memcmp(reserve + kIntroSkipOffset, kIntroSkipMagic, sizeof(kIntroSkipMagic) - 1) == 0;
}

unsigned new_game_plus_count(const dSv_save_c* save) {
    if (!is_new_game_plus(save)) {
        return 0;
    }
    const unsigned count = reserve_bytes(save)[kNgPlusCountOffset];
    return count == 0 ? 1 : count;
}

ModResult install_save_compat_hooks(ModError* error) {
    ModResult result = mods::hook_add_post<CardToMemory>(svc_hook, on_card_to_memory_post);
    if (result == MOD_OK) {
        result = mods::hook_add_post<ItemFuncSword>(svc_hook, restore_master_sword_post);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<ItemFuncWoodStick>(svc_hook, restore_master_sword_post);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<ItemFuncWoodShield>(svc_hook, restore_hylian_shield_post);
    }
    if (result == MOD_OK) {
        result = add_post<ItemGetCheckMasterSword>(hide_early_ngplus_item_post,
            reinterpret_cast<void*>(dSv_event_flag_c::F_0264));
    }
    if (result == MOD_OK) {
        result = add_post<ItemGetCheckWearKokiri>(hide_early_ngplus_item_post,
            reinterpret_cast<void*>(dSv_event_flag_c::M_019));
    }
    if (result == MOD_OK) {
        result = add_post<ItemGetCheckHeavyBoots>(hide_early_ngplus_item_post,
            reinterpret_cast<void*>(dSv_event_flag_c::F_0232));
    }
    if (result == MOD_OK) {
        result = add_post<ItemGetCheckLantern>(hide_early_ngplus_item_post,
            reinterpret_cast<void*>(dSv_event_flag_c::M_095));
    }
    if (result == MOD_OK) {
        result = add_post<ItemGetCheckSlingshot>(hide_early_ngplus_item_post,
            reinterpret_cast<void*>(dSv_event_flag_c::F_0600));
    }
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight save compatibility hooks");
    }
    return MOD_OK;
}

}  // namespace dawnlight
