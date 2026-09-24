#include "glider_reward.hpp"

#include "config.hpp"
#include "glider_visual.hpp"
#include "service_imports.hpp"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_demo_item.h"
#include "d/d_com_inf_game.h"
#include "d/d_item.h"
#include "d/d_meter2_info.h"
#include "d/d_msg_object.h"
#include "f_pc/f_pc_create_req.h"
#include "f_pc/f_pc_create_tag.h"
#include "f_pc/f_pc_layer.h"
#include "f_pc/f_pc_leaf.h"
#include "f_pc/f_pc_method.h"
#include "mods/svc/flow.hpp"
#include "mods/svc/hook.hpp"

#include <vector>

namespace dawnlight {
namespace {
DEFINE_HOOK(&daAlink_c::procCoGetItemInit, GliderRewardGetInit);
DEFINE_HOOK(&fpcLf_Draw, GliderRewardItemDraw);
#if defined(__APPLE__)
DEFINE_HOOK(&fpcMtd_Method, GliderRewardExecute);
#else
DEFINE_HOOK(&fpcMtd_Execute, GliderRewardExecute);
#endif

constexpr ActorId kNoActor = fpcM_ERROR_PROCESS_ID_e;
// Native demo-item flag 0x01 prevents execItemGet: no bombs, bag, first-item
// flags, or item-give observers are changed by this presentation-only reward.
constexpr u8 kPresentationOnly = 0x01;
constexpr int kOrderTimeout = 300;
struct RewardState {
    bool pending = false;
    bool retiring = false;
    bool started = false;
    bool poseStarted = false;
    ActorId item = kNoActor;
    ActorId player = kNoActor;
    int waitFrames = 0;
};
RewardState s_reward;
mods::flow::RegisteredMessage s_message;

// Never identify the prop merely by its bomb item number: real bomb pickups
// and their models/messages must keep their normal behavior.
daDitem_c* reward_actor() {
    if (s_reward.item == kNoActor || fpcM_IsCreating(s_reward.item)) return nullptr;
    auto* actor = fopAcM_SearchByID(s_reward.item);
    return actor && fopAcM_GetName(actor) == fpcNm_Demo_Item_e
        ? static_cast<daDitem_c*>(actor) : nullptr;
}

bool owns_event() {
    return s_reward.item != kNoActor && dComIfGp_getEvent()->mPt2 == s_reward.item;
}

bool safe_to_start(daAlink_c* link) {
    if (!link || link->checkWolf() || link->checkDeadHP() || link->checkEventRun() ||
        dComIfGp_event_runCheck() || dComIfGp_isEnableNextStage() ||
        dComIfGp_isPauseFlag() || dMeter2Info_getWindowStatus() != 0 ||
        dMeter2Info_getPauseStatus() != 0 || dMsgObject_isTalkNowCheck()) return false;
    switch (link->mProcID) {
    case daAlink_c::PROC_WAIT:
    case daAlink_c::PROC_MOVE:
    case daAlink_c::PROC_ATN_MOVE:
    case daAlink_c::PROC_TIRED_WAIT:
        return true;
    default:
        return false;
    }
}

void retire_prop() {
    clear_glider_reward_visual();
    if (s_reward.item != kNoActor && fpcM_IsCreating(s_reward.item)) {
        // Creation can still be loading its archive when a save is reset.
        for (auto* node = g_fpcCtTg_Queue.mpHead; node; node = NODE_GET_NEXT(node)) {
            auto* request = static_cast<create_request*>(reinterpret_cast<create_tag*>(node)->base.mpTagData);
            if (request && request->id == s_reward.item) {
                if (fpcCtRq_IsDoing(request) || !fpcCtRq_Cancel(request)) return;
                break;
            }
        }
        if (fpcM_IsCreating(s_reward.item)) return;
    }
    if (auto* actor = reward_actor(); actor && !fopAcM_delete(actor)) return;
    s_reward = {};
}

HookAction before_item_execute(ModContext*, void* args, void*, void*) {
    if (!s_reward.started || s_reward.retiring || !owns_event()) return HOOK_CONTINUE;
    auto* item = reward_actor();
    if (!item || mods::arg<void*>(args, 1) != item || !item->sub_method) return HOOK_CONTINUE;
    const auto* methods = reinterpret_cast<const process_method_class*>(item->sub_method);
#if defined(__APPLE__)
    const bool executing = mods::arg<process_method_func>(args, 0) == methods->execute_method;
#else
    const bool executing = mods::arg<const process_method_class*>(args, 0) == methods;
#endif
    // Normally a separate chest/pickup actor owns DEFAULT_GETITEM until the
    // camera finishes. Our prop also owns that event, so retain it until our
    // update resets the completed event. Never interfere with deletion hooks.
    if (executing && item->mAction == daDitem_c::ACTION_END_e)
        item->setAction(daDitem_c::ACTION_WAIT_LIGHT_END_e);
    return HOOK_CONTINUE;
}

HookAction before_get_init(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    auto* item = reward_actor();
    if (!link || !item || s_reward.retiring || !owns_event() ||
        !item->eventInfo.checkCommandItem() || fopAcM_GetID(link) != s_reward.player)
        return HOOK_CONTINUE;
    // DEFAULT_GETITEM uses the item partner. Supply our existing prop instead
    // of letting Link create an inventory-granting second demo item.
    s_reward.started = true;
    dComIfGp_event_setItemPartnerId(s_reward.item);
    dComIfGp_event_setGtItm(dItemNo_BOMB_5_e);
    link->mDemo.setParam0(0);
    link->mDemo.setParam1(0);
    return HOOK_CONTINUE;
}

void after_get_init(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !s_reward.started || s_reward.poseStarted || !owns_event() ||
        fopAcM_GetID(link) != s_reward.player || link->mProcID != daAlink_c::PROC_GET_ITEM)
        return; // An equipped item may first require the native unequip action.
    // Use the chest-reward lift, followed by the native holding pose. No chest
    // opening action or unrelated cutscene is forced into the NPC dialogue.
    link->setSingleAnimeBase(daAlink_c::ANM_GET_A);
    link->mProcVar1.field_0x300a = 0;
    link->field_0x32cc = s_message.id();
    s_reward.poseStarted = true;
}

HookAction before_item_draw(ModContext*, void* args, void* result, void*) {
    auto* process = mods::arg<leafdraw_class*>(args, 0);
    if (!process || s_reward.item == kNoActor || fopAcM_GetID(process) != s_reward.item)
        return HOOK_CONTINUE;
    auto* item = reward_actor();
    auto* link = daAlink_getAlinkActorClass();
    if (!s_reward.retiring && s_reward.started && item && item->chkDraw() &&
        !item->chkDead() && link && fopAcM_GetID(link) == s_reward.player &&
        link->mProcID == daAlink_c::PROC_GET_ITEM)
        queue_glider_reward_visual(link, s_reward.item);
    else
        clear_glider_reward_visual();
    if (result) *static_cast<int*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
}
} // namespace

ModResult initialize_glider_reward(ModError* error) {
    std::vector<mods::flow::MessageVariant> variants;
    for (auto language : {MESSAGE_LANGUAGE_ENGLISH, MESSAGE_LANGUAGE_GERMAN,
            MESSAGE_LANGUAGE_FRENCH, MESSAGE_LANGUAGE_SPANISH, MESSAGE_LANGUAGE_ITALIAN,
            MESSAGE_LANGUAGE_JAPANESE}) {
        const char* text = language == MESSAGE_LANGUAGE_GERMAN
            ? "Du hast den Gleiter erhalten!\nDrücke ZR in der Luft, um zu gleiten."
            : "You got the Glider!\nPress ZR in midair to glide.";
        variants.push_back(mods::flow::MessageBuilder{}
            .box_kind(MESSAGE_BOX_ITEM_GET).box_position(MESSAGE_POSITION_MIDDLE)
            .text(text).build(language));
    }
    s_message = mods::flow::register_message(0, variants);
    if (!s_message) return mods::set_error(error, s_message.result(),
        "failed to register Glider reward message");
    auto result = mods::hook::add_pre<GliderRewardGetInit>(svc_hook, before_get_init);
    if (result == MOD_OK) result = mods::hook::add_post<GliderRewardGetInit>(svc_hook, after_get_init);
    if (result == MOD_OK) result = mods::hook::add_pre<GliderRewardItemDraw>(svc_hook, before_item_draw);
    if (result == MOD_OK) result = mods::hook::add_pre<GliderRewardExecute>(svc_hook, before_item_execute);
    return result == MOD_OK ? MOD_OK : mods::set_error(error, result,
        "failed to hook Glider reward presentation");
}

void queue_glider_reward() {
    if (!s_reward.pending && s_reward.item == kNoActor) s_reward.pending = true;
}

void update_glider_reward() {
    if (s_reward.retiring) { retire_prop(); return; }
    if (!s_reward.pending && s_reward.item == kNoActor) return;
    if (auto* item = reward_actor(); item && owns_event() && item->eventInfo.checkCommandItem())
        s_reward.started = true;
    // Turning progression off cancels a pending reward. An accepted sequence
    // is allowed to finish normally so it cannot strand Link or the camera.
    if (!progression_system_enabled() && !s_reward.started) { cancel_glider_reward(); return; }
    auto* link = daAlink_getAlinkActorClass();
    if (s_reward.item == kNoActor) {
        if (!safe_to_start(link)) return;
        auto* layer = link->layer_tag.layer;
        if (!layer || layer == fpcLy_RootLayer() || fpcLy_IsDeletingMesg(layer)) return;
        // mod_update can run on the root layer. Room number alone does not
        // give the prop scene ownership: a root actor also gets a second draw
        // through root traversal, in addition to the play scene's actor queue.
        struct RestoreLayer {
            layer_class* previous;
            ~RestoreLayer() { fpcLy_SetCurrentLayer(previous); }
        } restore{fpcLy_CurrentLayer()};
        fpcLy_SetCurrentLayer(layer);
        s_reward.item = fopAcM_createDemoItem(&link->current.pos, dItemNo_BOMB_5_e, -1,
            nullptr, fopAcM_GetRoomNo(link), nullptr, kPresentationOnly);
        s_reward.player = fopAcM_GetID(link);
        s_reward.pending = false;
        return;
    }
    if (!link || dComIfGp_isEnableNextStage() || fopAcM_GetID(link) != s_reward.player) {
        cancel_glider_reward();
        return;
    }
    if (s_reward.started && !owns_event()) { cancel_glider_reward(); return; }
    if (s_reward.started && dComIfGp_evmng_endCheck("DEFAULT_GETITEM")) {
        dComIfGp_event_reset();
        cancel_glider_reward();
        return;
    }
    if (fpcM_IsCreating(s_reward.item)) {
        if (++s_reward.waitFrames > kOrderTimeout) cancel_glider_reward();
        return;
    }
    auto* item = reward_actor();
    if (!item) { cancel_glider_reward(); return; }
    if (s_reward.started) {
        // The prop fades/deletes natively. Never reset another actor's event.
        if (!owns_event() && !dComIfGp_event_runCheck()) cancel_glider_reward();
        return;
    }
    if (++s_reward.waitFrames > kOrderTimeout) { cancel_glider_reward(); return; }
    if (safe_to_start(link)) {
        item->eventInfo.onCondition(dEvtCnd_CANGETITEM_e);
        fopAcM_orderItemEvent(item, 0, 0);
    }
}

void cancel_glider_reward() {
    s_reward.pending = false;
    s_reward.retiring = true;
    // Do not reset the global event here: save observers run after another
    // save is installed, and it may already own an unrelated event.
    retire_prop();
}

void shutdown_glider_reward() {
    if (s_reward.started && owns_event()) dComIfGp_event_reset();
    cancel_glider_reward();
    s_message.reset();
}
} // namespace dawnlight
