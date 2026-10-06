#pragma once

#include <mods/service.hpp>

#define TWILIT_ESSENTIALS_COLLECTION_SERVICE_ID "com.dusklight.twilit_essentials.collection"
#define TWILIT_ESSENTIALS_COLLECTION_SERVICE_MAJOR 1u
#define TWILIT_ESSENTIALS_COLLECTION_SERVICE_MINOR 0u

enum TwilitEssentialsCollectionKind : uint32_t {
    TWILIT_ESSENTIALS_COLLECTION_SWORD = 0,
    TWILIT_ESSENTIALS_COLLECTION_SHIELD = 1,
    TWILIT_ESSENTIALS_COLLECTION_TUNIC = 2,
};

typedef uint64_t TwilitEssentialsCollectionSlotHandle;

struct TwilitEssentialsCollectionSlotDesc {
    uint32_t struct_size;
    uint32_t kind;
    uint32_t column;
    uint32_t base_item;
    const char* name;
    const char* description;
    const char* icon;
    uint32_t icon_arc_file_id;
    const char* model_arc;
    uint32_t model_file_id;
    uint32_t sheath_file_id;
    float offset[3];
    float rotation[3];
    float scale;
    uint32_t pad_color;
    uint32_t iron_boots_hide_feet;
    uint32_t (*is_unlocked)(void* user_data);
    void* user_data;
};

#define TWILIT_ESSENTIALS_COLLECTION_SLOT_DESC_INIT                                      \
    {sizeof(TwilitEssentialsCollectionSlotDesc), 0, 0, 0xFFu, nullptr, nullptr, nullptr, \
        0xFFFFu, nullptr, 0, 0xFFFFu, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f,      \
        0xFFFFFFFFu, 1, nullptr, nullptr}

struct TwilitEssentialsCollectionService {
    ServiceHeader header;
    ModResult (*add_slot)(ModContext* caller, const TwilitEssentialsCollectionSlotDesc* desc,
        TwilitEssentialsCollectionSlotHandle* out_handle);
    ModResult (*remove_slot)(ModContext* caller, TwilitEssentialsCollectionSlotHandle handle);
    ModResult (*is_equipped)(ModContext* caller, TwilitEssentialsCollectionSlotHandle handle,
        uint32_t* out_equipped);
    ModResult (*refresh)(ModContext* caller);
};

MOD_DECLARE_SERVICE(TwilitEssentialsCollectionService, svc_te_collection,
    TWILIT_ESSENTIALS_COLLECTION_SERVICE_ID, TWILIT_ESSENTIALS_COLLECTION_SERVICE_MAJOR,
    TWILIT_ESSENTIALS_COLLECTION_SERVICE_MINOR);
