#include "boss_rush.hpp"
#include "boss_rush_common.hpp"
#include "../save_state.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_overlap_mng.h"
#include <cstring>

bool is_in_boss_rush_chamber() {
    return dawnlight::save_state_boss_rush_active() && dawnlight::save_state_boss_rush_state() == 0 &&
        std::strcmp(dComIfGp_getStartStageName(), kBossRushChamberStage) == 0 &&
        dComIfGp_getStartStageRoomNo() == kBossRushChamberRoom;
}

bool boss_rush_scene_load_stable() {
    return is_in_boss_rush_chamber() && daAlink_getAlinkActorClass() &&
        !dComIfGp_isEnableNextStage() && !fopOvlpM_IsPeek();
}
size_t boss_rush_get_active_gallery_count() { return g_bossGalleryCount; }
size_t boss_rush_get_active_gallery_table_index(size_t slot) { return slot; }
size_t boss_rush_get_circle_slot_for_table_index(size_t index) { return index; }
