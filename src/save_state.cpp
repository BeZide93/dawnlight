#include "save_state.hpp"

#include "service_imports.hpp"

#include "d/d_com_inf_game.h"
#include "d/d_save.h"
#include "mods/service.hpp"
#include "mods/svc/game_mode.h"
#include "mods/svc/log.h"
#include "mods/svc/save.h"

#include <array>
#include <cstring>

namespace dawnlight {
namespace {

constexpr char kDefaultStateBlob[] = "state";
constexpr char kBossRushStateBlob[] = "bossrush-state";
constexpr char kBossRushGameModeId[] = "bossrush";
constexpr uint8_t kStateVersion = 1;
constexpr uint8_t kFlagIntroSkipped = 1 << 0;
constexpr uint8_t kFlagBossRush = 1 << 1;
constexpr size_t kBossDefeatedMaskSize = 3;
constexpr size_t kSerializedStateSize = 9;

// Legacy Dawnlight metadata embedded in dSv_save_c. It is read only for one-time migration.
constexpr size_t kLegacyReserveOffset = 0x8F0;
constexpr size_t kLegacyNgPlusCountOffset = 8;
constexpr size_t kLegacyIntroSkipOffset = 16;
constexpr size_t kLegacyBossRushOffset = 32;
constexpr size_t kLegacyBossRushIndexOffset = 40;
constexpr size_t kLegacyBossRushLoopOffset = 41;
constexpr size_t kLegacyBossRushStageOffset = 42;
constexpr size_t kLegacyBossDefeatedMagicOffset = 43;
constexpr size_t kLegacyBossDefeatedMaskOffset = 51;
constexpr char kLegacyNgPlusMagic[] = "DUSKNGP1";
constexpr char kLegacyIntroSkipMagic[] = "DUSKSKP1";
constexpr char kLegacyBossRushMagic[] = "DUSKBR1";
constexpr char kLegacyBossDefeatedMagic[] = "DUSKBRD1";

struct DawnlightSaveState {
    bool available = false;
    bool bossRushContext = false;
    uint8_t flags = 0;
    uint8_t ngPlusCount = 0;
    uint8_t bossRushIndex = 0;
    uint8_t bossRushLoop = 0;
    uint8_t bossRushState = 0;
    std::array<uint8_t, kBossDefeatedMaskSize> defeated = {};
};

DawnlightSaveState s_state;
SaveObserverHandle s_saveObserver = 0;

bool boss_rush_context_active() {
    bool active = false;
    return svc_game_mode != nullptr &&
           svc_game_mode->is_active(mod_ctx, kBossRushGameModeId, &active) == MOD_OK && active;
}

const char* current_blob_name() {
    return s_state.bossRushContext ? kBossRushStateBlob : kDefaultStateBlob;
}

bool flag_set(uint8_t flag) {
    return (s_state.flags & flag) != 0;
}

void set_flag(uint8_t flag, bool enabled) {
    if (enabled) {
        s_state.flags |= flag;
    } else {
        s_state.flags &= static_cast<uint8_t>(~flag);
    }
}

bool state_has_data() {
    if (s_state.bossRushContext) {
        return flag_set(kFlagBossRush);
    }
    return flag_set(kFlagIntroSkipped) || s_state.ngPlusCount != 0;
}

std::array<uint8_t, kSerializedStateSize> serialize_state() {
    return {{kStateVersion, s_state.flags, s_state.ngPlusCount, s_state.bossRushIndex,
        s_state.bossRushLoop, s_state.bossRushState, s_state.defeated[0], s_state.defeated[1],
        s_state.defeated[2]}};
}

bool deserialize_state(const std::array<uint8_t, kSerializedStateSize>& data) {
    if (data[0] != kStateVersion) {
        return false;
    }
    s_state.flags = data[1];
    s_state.ngPlusCount = data[2];
    s_state.bossRushIndex = data[3];
    s_state.bossRushLoop = data[4];
    s_state.bossRushState = data[5];
    s_state.defeated = {{data[6], data[7], data[8]}};
    return true;
}

void persist_state() {
    if (!s_state.available || svc_save == nullptr) {
        return;
    }

    if (!state_has_data()) {
        const ModResult result = svc_save->delete_blob(mod_ctx, current_blob_name());
        if (result != MOD_OK && result != MOD_INVALID_ARGUMENT && result != MOD_UNAVAILABLE) {
            svc_log->warn(mod_ctx, "Dawnlight Save: failed to remove empty state blob");
        }
        return;
    }

    const auto data = serialize_state();
    if (svc_save->set_blob(mod_ctx, current_blob_name(), data.data(), data.size()) != MOD_OK) {
        svc_log->warn(mod_ctx, "Dawnlight Save: failed to persist state blob");
    }
}

bool legacy_marker_matches(const uint8_t* reserve, size_t offset, const char* marker,
    size_t markerSize) {
    return std::memcmp(reserve + offset, marker, markerSize) == 0;
}

void migrate_legacy_state(bool loadedBlob) {
    dSv_save_c* save = dComIfGs_getSaveData();
    if (save == nullptr) {
        return;
    }

    uint8_t* reserve = reinterpret_cast<uint8_t*>(save) + kLegacyReserveOffset;
    const bool legacyNgPlus = legacy_marker_matches(
        reserve, 0, kLegacyNgPlusMagic, sizeof(kLegacyNgPlusMagic) - 1);
    const bool legacyIntro = legacy_marker_matches(reserve, kLegacyIntroSkipOffset,
        kLegacyIntroSkipMagic, sizeof(kLegacyIntroSkipMagic) - 1);
    const bool legacyBoss = legacy_marker_matches(reserve, kLegacyBossRushOffset,
        kLegacyBossRushMagic, sizeof(kLegacyBossRushMagic) - 1);
    const bool legacyDefeated = legacy_marker_matches(reserve, kLegacyBossDefeatedMagicOffset,
        kLegacyBossDefeatedMagic, sizeof(kLegacyBossDefeatedMagic) - 1);

    if (!loadedBlob) {
        if (s_state.bossRushContext && legacyBoss) {
            set_flag(kFlagBossRush, true);
            s_state.bossRushIndex = reserve[kLegacyBossRushIndexOffset];
            s_state.bossRushLoop = reserve[kLegacyBossRushLoopOffset];
            s_state.bossRushState = reserve[kLegacyBossRushStageOffset];
            if (legacyDefeated) {
                std::memcpy(s_state.defeated.data(), reserve + kLegacyBossDefeatedMaskOffset,
                    s_state.defeated.size());
            }
        } else if (!s_state.bossRushContext) {
            if (legacyIntro) {
                set_flag(kFlagIntroSkipped, true);
            }
            if (legacyNgPlus) {
                s_state.ngPlusCount = reserve[kLegacyNgPlusCountOffset] == 0
                    ? 1
                    : reserve[kLegacyNgPlusCountOffset];
            }
        }
    }

    bool cleaned = false;
    if (legacyNgPlus) {
        std::memset(reserve, 0, sizeof(kLegacyNgPlusMagic) - 1);
        reserve[kLegacyNgPlusCountOffset] = 0;
        cleaned = true;
    }
    if (legacyIntro) {
        std::memset(reserve + kLegacyIntroSkipOffset, 0, sizeof(kLegacyIntroSkipMagic) - 1);
        cleaned = true;
    }
    if (legacyBoss || legacyDefeated) {
        std::memset(reserve + kLegacyBossRushOffset, 0,
            kLegacyBossDefeatedMaskOffset + kBossDefeatedMaskSize - kLegacyBossRushOffset);
        cleaned = true;
    }

    if (!loadedBlob && (legacyNgPlus || legacyIntro || legacyBoss)) {
        persist_state();
    }
    if (cleaned) {
        svc_log->info(mod_ctx,
            "Dawnlight Save: migrated legacy metadata out of the vanilla save data");
    }
}

void load_current_state() {
    s_state = {};
    s_state.available = true;
    s_state.bossRushContext = boss_rush_context_active();
    if (s_state.bossRushContext) {
        set_flag(kFlagBossRush, true);
    }

    std::array<uint8_t, kSerializedStateSize> data{};
    size_t size = data.size();
    const ModResult result =
        svc_save->get_blob(mod_ctx, current_blob_name(), data.data(), &size);
    const bool loadedBlob = result == MOD_OK && size == data.size() && deserialize_state(data);
    if (result == MOD_OK && !loadedBlob) {
        svc_log->warn(mod_ctx, "Dawnlight Save: ignoring an incompatible state blob");
    }
    if (s_state.bossRushContext) {
        set_flag(kFlagBossRush, true);
    } else {
        set_flag(kFlagBossRush, false);
        s_state.bossRushIndex = 0;
        s_state.bossRushLoop = 0;
        s_state.bossRushState = 0;
        s_state.defeated.fill(0);
    }
    migrate_legacy_state(loadedBlob);
}

void on_new_save(ModContext*, uint32_t, void*) {
    s_state = {};
    s_state.available = true;
    s_state.bossRushContext = boss_rush_context_active();
    set_flag(kFlagBossRush, s_state.bossRushContext);
}

void on_save_loaded(ModContext*, uint32_t, void*) {
    load_current_state();
}

bool state_available_for_current_context() {
    if (!s_state.available || s_state.bossRushContext != boss_rush_context_active()) {
        return false;
    }
    return true;
}

}  // namespace

ModResult initialize_save_state(ModError* error) {
    const ModResult result = svc_save->observe_saves(
        mod_ctx, on_new_save, on_save_loaded, nullptr, nullptr, &s_saveObserver);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to observe Dawnlight save state");
    }
    return MOD_OK;
}

void shutdown_save_state() {
    if (s_saveObserver != 0 && svc_save != nullptr) {
        svc_save->unobserve_saves(mod_ctx, s_saveObserver);
    }
    s_saveObserver = 0;
    s_state = {};
}

bool save_state_is_new_game_plus() {
    return state_available_for_current_context() && s_state.ngPlusCount != 0;
}

unsigned save_state_new_game_plus_count() {
    return save_state_is_new_game_plus() ? s_state.ngPlusCount : 0;
}

bool save_state_intro_skipped() {
    return state_available_for_current_context() && flag_set(kFlagIntroSkipped);
}

void save_state_set_intro_skipped(bool enabled) {
    if (!state_available_for_current_context()) return;
    set_flag(kFlagIntroSkipped, enabled);
    persist_state();
}

bool save_state_boss_rush_active() {
    return state_available_for_current_context() && flag_set(kFlagBossRush);
}

void save_state_set_boss_rush_active(bool enabled) {
    if (!state_available_for_current_context()) return;
    set_flag(kFlagBossRush, enabled);
    if (!enabled) {
        s_state.bossRushIndex = 0;
        s_state.bossRushLoop = 0;
        s_state.bossRushState = 0;
        s_state.defeated.fill(0);
    }
    persist_state();
}

uint8_t save_state_boss_rush_index() {
    return save_state_boss_rush_active() ? s_state.bossRushIndex : 0;
}

void save_state_set_boss_rush_index(uint8_t index) {
    if (!save_state_boss_rush_active()) return;
    s_state.bossRushIndex = index;
    persist_state();
}

uint8_t save_state_boss_rush_loop() {
    return save_state_boss_rush_active() ? s_state.bossRushLoop : 0;
}

void save_state_set_boss_rush_loop(uint8_t loop) {
    if (!save_state_boss_rush_active()) return;
    s_state.bossRushLoop = loop;
    persist_state();
}

void save_state_increment_boss_rush_loop() {
    if (!save_state_boss_rush_active() || s_state.bossRushLoop == 0xff) return;
    ++s_state.bossRushLoop;
    persist_state();
}

uint8_t save_state_boss_rush_state() {
    return save_state_boss_rush_active() ? s_state.bossRushState : 0;
}

void save_state_set_boss_rush_state(uint8_t state) {
    if (!save_state_boss_rush_active()) return;
    s_state.bossRushState = state;
    persist_state();
}

bool save_state_boss_defeated(uint8_t index) {
    return save_state_boss_rush_active() && index < s_state.defeated.size() * 8 &&
           (s_state.defeated[index / 8] & (1 << (index % 8))) != 0;
}

void save_state_mark_boss_defeated(uint8_t index) {
    if (!save_state_boss_rush_active() || index >= s_state.defeated.size() * 8) return;
    s_state.defeated[index / 8] |= static_cast<uint8_t>(1 << (index % 8));
    persist_state();
}

void save_state_clear_boss_defeated() {
    if (!save_state_boss_rush_active()) return;
    s_state.defeated.fill(0);
    persist_state();
}

}  // namespace dawnlight
