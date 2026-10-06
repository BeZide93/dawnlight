#include "collection_service.hpp"
#include "config.hpp"
#include "service_imports.hpp"
#include "twilit_essentials/collection.h"
#include "d/d_com_inf_game.h"
#include "mods/service.hpp"
#include <cstring>

namespace dawnlight {
namespace {
using CollectionService = TwilitEssentialsCollectionService;
const CollectionService* s_provider = nullptr;
TwilitEssentialsCollectionSlotHandle s_slot = 0;
uint64_t s_lifecycle = 0;
SecondSword s_sword = SecondSword::Ordon;
uint32_t s_backing = dItemNo_NONE_e;
unsigned s_retry = 0;
bool s_ready = false;
bool s_warnedAlias = false;
bool s_aliasUnsupported = false;

const CollectionService* live_service() {
    const void* service = nullptr;
    if (!svc_host || !svc_host->get_service ||
        svc_host->get_service(mod_ctx, TWILIT_ESSENTIALS_COLLECTION_SERVICE_ID,
            TWILIT_ESSENTIALS_COLLECTION_SERVICE_MAJOR, 0, &service) != MOD_OK) return nullptr;
    const auto* api = static_cast<const CollectionService*>(service);
    if (!SERVICE_HAS(api, CollectionService, refresh) || api->header.major_version != 1 ||
        !api->add_slot || !api->remove_slot || !api->is_equipped || !api->refresh) return nullptr;
    return api;
}

bool owned_shield(uint32_t item) {
    return (item == dItemNo_WOOD_SHIELD_e || item == dItemNo_SHIELD_e || item == dItemNo_HYLIA_SHIELD_e) &&
        dComIfGs_isItemFirstBit(static_cast<u8>(item));
}

uint32_t unlocked(void*) {
    return dual_wield_enabled() && second_sword() == s_sword && owned_shield(s_backing);
}

void provider_detached(ModContext*, ModContext*, const char* id, ModLifecycleEvent event, void*) {
    if (event != MOD_LIFECYCLE_DETACHED || !id || std::strcmp(id, "com.dusklight.twilit_essentials")) return;
    // The provider has already shut down. Do not call its old function table,
    // even if a later load reuses the same address and handle numbers.
    s_provider = nullptr; s_slot = 0; s_retry = 0; s_ready = false; s_warnedAlias = false; s_aliasUnsupported = false;
}

bool remove_slot() {
    if (!s_slot) return true;
    if (s_provider != live_service()) { s_slot = 0; s_ready = false; return true; }
    if (s_provider->remove_slot(mod_ctx, s_slot) != MOD_OK) return false;
    s_slot = 0; s_ready = false;
    return true;
}
}

ModResult initialize_collection_service(ModError* error) {
    const auto result = svc_host->watch_mod_lifecycle(mod_ctx, provider_detached, nullptr, &s_lifecycle);
    if (result != MOD_OK) return mods::set_error(error, result, "Dual Wield: collection service lifecycle");
    return MOD_OK;
}

bool collection_service_active() {
    // A failed removal still owns a provider slot. Suppress the legacy slot
    // until removal succeeds, but do not treat the pending slot as equipped.
    return s_slot && s_provider == live_service();
}

bool collection_service_equipped() {
    uint32_t equipped = 0;
    return collection_service_active() && s_ready && unlocked(nullptr) &&
        s_provider->is_equipped(mod_ctx, s_slot, &equipped) == MOD_OK && equipped != 0;
}

void update_collection_service() {
    const auto* api = live_service();
    if (api != s_provider) {
        s_slot = 0; s_provider = api; s_retry = 0; s_ready = false; s_warnedAlias = false; s_aliasUnsupported = false;
    }
    if (!api) return;
    if (s_retry) { --s_retry; return; }
    const auto sword = second_sword();
    const auto backing = dComIfGs_getSelectEquipShield();
    const bool enabled = dual_wield_enabled();
    const bool equipped = collection_service_equipped();
    const bool replace = s_slot && (!s_ready || !enabled || sword != s_sword ||
        !owned_shield(s_backing) || (!equipped && owned_shield(backing) && backing != s_backing));
    if (replace) {
        if (!remove_slot()) { s_retry = 60; return; }
        api->refresh(mod_ctx);
    }
    if (s_slot || s_aliasUnsupported || !enabled || !owned_shield(backing)) return;
    // The collection archive is a game resource, unavailable during mod init.
    // Use its file ID so TE can reuse native/replaced artwork without shipping
    // a duplicate texture or borrowing a pointer into the menu's heap.
    auto* archive = dComIfGp_getCollectResArchive();
    const auto assets = second_sword_assets(sword);
    auto* icon = archive ? archive->findNameResource(assets.icon) : nullptr;
    if (!icon) return;
    TwilitEssentialsCollectionSlotDesc desc = TWILIT_ESSENTIALS_COLLECTION_SLOT_DESC_INIT;
    desc.kind = TWILIT_ESSENTIALS_COLLECTION_SHIELD;
    desc.column = 0; // Service-defined automatic placement; never overwrite a TE slot.
    desc.base_item = backing; // Only an already-owned, equipped shield; never a sword item in the shield row.
    desc.name = assets.name;
    desc.description = "Equip a second sword for Dual Wield. Select a shield to return to shield combat.";
    desc.icon_arc_file_id = icon->getFileID();
    // Dawnlight owns both sword models and combat poses. TE owns this menu slot.
    desc.is_unlocked = unlocked;
    s_sword = sword; s_backing = backing;
    TwilitEssentialsCollectionSlotHandle handle = 0;
    if (api->add_slot(mod_ctx, &desc, &handle) != MOD_OK || !handle) {
        s_retry = 60;
        return;
    }
    s_slot = handle;
    uint32_t initialEquipped = 0;
    if (api->is_equipped(mod_ctx, s_slot, &initialEquipped) != MOD_OK || initialEquipped) {
        // The demo does not specify independent selection for model-less slots.
        // An alias of the backing shield reports equipped immediately. Never
        // silently activate Dual Wield just because a normal shield is worn.
        // The same ambiguous result can occur on provider-side save restoration;
        // retain the legacy backend until a fresh independent slot is available.
        s_aliasUnsupported = initialEquipped != 0;
        remove_slot();
        api->refresh(mod_ctx);
        if (!s_warnedAlias && svc_log)
            svc_log->warn(mod_ctx, "Dual Wield: TE collection slot has no confirmed independent selection; keeping the legacy menu");
        s_warnedAlias = true; s_retry = 60;
        return;
    }
    s_ready = true;
    api->refresh(mod_ctx);
}

void shutdown_collection_service() {
    if (!remove_slot() && svc_log)
        svc_log->warn(mod_ctx, "Dual Wield: TE did not remove its collection slot; provider must release caller state on detach");
    if (s_lifecycle) svc_host->unwatch_mod_lifecycle(mod_ctx, s_lifecycle);
    s_lifecycle = 0; s_provider = nullptr; s_slot = 0; s_retry = 0; s_ready = false; s_warnedAlias = false; s_aliasUnsupported = false;
}
}
