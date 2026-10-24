// Native princess Zelda, owned by the Survivors hub. Story Zeldas and their
// message flows never enter this path.
ActorId s_hubZelda = kNone;
bool s_hubZeldaRetiring = false, s_hubZeldaRewardPending = false;
unsigned s_hubZeldaRetry = 0;
mods::flow::Graph s_hubZeldaGraph;
mods::flow::RegisteredMessage s_hubZeldaMessage;
mods::flow::Event s_hubZeldaReward;
u16 s_hubZeldaStart = 0;
constexpr u16 kHubZeldaFlow = 3001; // Native common-message archive selection.
struct HubZeldaGift {
    ActorId id = kNone;
    bool queued = false, started = false, poseStarted = false, retiring = false;
    unsigned waitFrames = 0;
} s_hubZeldaGift;

bool owns_hub_zelda_gift_event() {
    return s_hubZeldaGift.id != kNone && dComIfGp_getEvent()->mPt2 == s_hubZeldaGift.id;
}
bool hub_zelda_gift_busy() { return s_hubZeldaGift.queued || s_hubZeldaGift.id != kNone; }
fopAc_ac_c* hub_zelda_gift_actor() {
    auto* actor = actor_by_id(s_hubZeldaGift.id);
    return actor && fopAcM_GetName(actor) == fpcNm_Demo_Item_e ? actor : nullptr;
}
void cleanup_hub_zelda_gift() {
    if (s_hubZeldaGift.id == kNone || fpcM_IsCreating(s_hubZeldaGift.id)) return;
    if (!hub_zelda_gift_actor()) {
        if (in_arena() && !transitioning() && owns_hub_zelda_gift_event()) dComIfGp_event_reset();
        s_hubZeldaGift = {};
    }
    else if (s_hubZeldaGift.retiring) svc_actor->delete_actor(mod_ctx, s_hubZeldaGift.id);
}
void retire_hub_zelda_gift() {
    if (in_arena() && !transitioning() && owns_hub_zelda_gift_event()) dComIfGp_event_reset();
    s_hubZeldaGift.queued = false;
    s_hubZeldaGift.retiring = true;
    cleanup_hub_zelda_gift();
}
void update_hub_zelda_gift() {
    cleanup_hub_zelda_gift();
    if (s_hubZeldaGift.retiring) return;
    if (s_hubZeldaGift.queued) {
        // Zelda's native talk action must close/reset its event first.
        if (!playable() || s_progressSaving || s_hubSaveRequested) return;
        auto* link = daAlink_getAlinkActorClass();
        if (!link) return;
        const ActorSpawnParams params{
            // Native demo item, no item bit, presentation-only. Commit the
            // sword once the owned event completes, as with Survivors chests.
            .parameters=0x00017f00u | dItemNo_SWORD_e, .argument=-1,
            .room_num=fopAcM_GetRoomNo(link),
            .position={link->current.pos.x,link->current.pos.y,link->current.pos.z},
            .angle={0,0,0}, .scale={1,1,1}, .create_function=nullptr,
        };
        s_hubZeldaGift.queued = false;
        ActorId id = kNone;
        if (create_standalone_actor(fpcNm_Demo_Item_e, params, id) == MOD_OK) s_hubZeldaGift.id = id;
        return;
    }
    if (s_hubZeldaGift.id == kNone || fpcM_IsCreating(s_hubZeldaGift.id)) return;
    auto* item = hub_zelda_gift_actor();
    const bool owned = owns_hub_zelda_gift_event();
    if (item && owned && item->eventInfo.checkCommandItem()) s_hubZeldaGift.started = true;
    if (s_hubZeldaGift.started) {
        if (owned && !dComIfGp_evmng_endCheck("DEFAULT_GETITEM")) return;
        if (owned) execItemGet(dItemNo_SWORD_e);
        retire_hub_zelda_gift();
        return;
    }
    if (!item) { retire_hub_zelda_gift(); return; }
    if (!playable() || s_progressSaving || s_hubSaveRequested) return;
    if (++s_hubZeldaGift.waitFrames > 300) { retire_hub_zelda_gift(); return; }
    dComIfGp_event_setGtItm(dItemNo_SWORD_e);
    item->eventInfo.onCondition(dEvtCnd_CANGETITEM_e);
    fopAcM_orderItemEvent(item, 0, 0);
}
HookAction before_hub_zelda_get(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    auto* item = hub_zelda_gift_actor();
    if (!link || !item || s_hubZeldaGift.retiring || !owns_hub_zelda_gift_event() ||
        !item->eventInfo.checkCommandItem() || fopAcM_GetID(link) != s_player) return HOOK_CONTINUE;
    s_hubZeldaGift.started = true;
    dComIfGp_event_setItemPartnerId(s_hubZeldaGift.id);
    dComIfGp_event_setGtItm(dItemNo_SWORD_e);
    link->mDemo.setParam0(0); link->mDemo.setParam1(0);
    return HOOK_CONTINUE;
}
void after_hub_zelda_get(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !s_hubZeldaGift.started || s_hubZeldaGift.poseStarted ||
        s_hubZeldaGift.retiring || !owns_hub_zelda_gift_event() ||
        fopAcM_GetID(link) != s_player || link->mProcID != daAlink_c::PROC_GET_ITEM) return;
    // Ordinary NPC gifts initialize GET_A_WAIT. Start the full raise-item
    // animation once, after any native sheathing action has actually finished.
    link->setSingleAnimeBase(daAlink_c::ANM_GET_A);
    link->mProcVar1.field_0x300a = 0;
    s_hubZeldaGift.poseStarted = true;
}

bool owned_hub_zelda(fopAc_ac_c* actor) {
    return actor && s_hubZelda != kNone && fopAcM_GetID(actor) == s_hubZelda &&
        fopAcM_GetName(actor) == fpcNm_NPC_ZELDA_e;
}
bool hub_zelda_enabled() {
    return survivors_hub_active() && !s_hubDeparture && !transitioning();
}
void cleanup_hub_zelda() {
    if (s_hubZelda == kNone || fpcM_IsCreating(s_hubZelda)) return;
    auto* actor = actor_by_id(s_hubZelda);
    if (!owned_hub_zelda(actor)) {
        s_hubZelda = kNone;
        s_hubZeldaRetiring = s_hubZeldaRewardPending = false;
    } else if (s_hubZeldaRetiring) {
        actor->attention_info.flags = 0;
        svc_actor->delete_actor(mod_ctx, s_hubZelda);
    }
}
void retire_hub_zelda() {
    retire_hub_zelda_gift();
    s_hubZeldaRetiring = true;
    s_hubZeldaRewardPending = false;
    s_hubZeldaRetry = 0;
    // Keep pending IDs until creation/deletion finishes. Never spawn a second
    // Zelda while the previous asynchronous request still owns an actor.
    cleanup_hub_zelda();
}
void hub_zelda_give_sword(ModContext*, const FlowEventContext* event, void*) {
    auto* actor = actor_by_id(s_hubZelda);
    if (!event || event->speaker_actor != actor || !owned_hub_zelda(actor) ||
        !hub_zelda_enabled() || s_hubZeldaRetiring || !s_hubZeldaRewardPending || hub_zelda_gift_busy()) return;
    s_hubZeldaRewardPending = false;
    s_hubZeldaGift = {};
    s_hubZeldaGift.queued = true;
}
ModResult initialize_hub_zelda() {
    s_hubZeldaMessage = register_survivors_dialog(mods::flow::MessageBuilder{}
        .box_kind(MESSAGE_BOX_TALK).talk_anim(0).face_anim(0)
        .text("It's dangerous to go alone! Take this."));
    if (!s_hubZeldaMessage) return s_hubZeldaMessage.result();
    s_hubZeldaReward = mods::flow::register_event("Survivors Zelda Ordon Sword", hub_zelda_give_sword);
    if (!s_hubZeldaReward) return s_hubZeldaReward.result();
    mods::flow::GraphBuilder graph{0};
    const auto text = graph.add_message(s_hubZeldaMessage.id());
    const auto reward = graph.add_event(s_hubZeldaReward.id(), {0,0,0,0});
    text.next(reward); reward.next(mods::flow::kEnd);
    s_hubZeldaGraph = graph.commit();
    if (!s_hubZeldaGraph) return s_hubZeldaGraph.result();
    s_hubZeldaStart = text.id();
    return MOD_OK;
}
HookAction before_hub_zelda_param(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<daNpc_Zelda_c*>(args, 0);
    if (!owned_hub_zelda(actor)) return HOOK_CONTINUE;
    // Set the actual action state before the first execute's wait/evtTalk,
    // including Create's internal Execute. A getType() detour alone left the
    // native type-0 seated idle and its "do not turn when talking" path active.
    actor->field_0xf80 = 1;
    actor->mAcch.ClrGrndNone();
    actor->mAcch.ClrWallNone();
    return HOOK_CONTINUE;
}
void after_hub_zelda_param(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<daNpc_Zelda_c*>(args, 0);
    if (owned_hub_zelda(actor) && (!hub_zelda_enabled() || s_hubZeldaRetiring ||
        !playable() || s_progressSaving || s_hubSaveRequested || hub_zelda_gift_busy())) actor->attention_info.flags = 0;
}
HookAction before_hub_zelda_flow(ModContext*, void* args, void* result, void*) {
    auto* actor = actor_by_id(s_hubZelda);
    if (!hub_zelda_enabled() || s_hubZeldaRetiring || !owned_hub_zelda(actor) || !s_hubZeldaStart ||
        mods::arg<dMsgFlow_c*>(args, 0) != &static_cast<daNpc_Zelda_c*>(actor)->mFlow ||
        mods::arg<u16>(args, 1) != kHubZeldaFlow) return HOOK_CONTINUE;
    // Retain native init(3001) to bind group 0, then select our private graph.
    *static_cast<u16*>(result) = s_hubZeldaStart;
    s_hubZeldaRewardPending = true;
    return HOOK_SKIP_ORIGINAL;
}
void update_hub_zelda() {
    if (!hub_zelda_enabled()) { retire_hub_zelda(); return; }
    update_hub_zelda_gift();
    cleanup_hub_zelda();
    if (s_hubZelda != kNone || !s_hubZeldaStart || !playable() ||
        s_progressSaving || s_hubSaveRequested) return;
    if (s_hubZeldaRetry) { --s_hubZeldaRetry; return; }
    s_hubZeldaRetry = 30;
    cXyz anchor;
    if (!survivors_pedestal_anchor(anchor)) return;
    cXyz probe = anchor + cXyz(-320, 100, 0);
    dBgS_ObjGndChk ground; ground.SetPos(&probe);
    const float height = dComIfG_Bgsp().GroundCross(&ground);
    if (!std::isfinite(height) || height <= -1.e8f || std::abs(height-anchor.y) > 50) return;
    const ActorSpawnParams params{
        .parameters=0xffffff01u, .argument=-1, .room_num=arenas[0].room,
        .position={probe.x,height,probe.z}, .angle={kHubZeldaFlow,0x4000,0},
        .scale={1,1,1}, .create_function=nullptr,
    };
    ActorId id = kNone;
    if (create_standalone_actor(fpcNm_NPC_ZELDA_e, params, id) == MOD_OK && id != kNone) {
        s_hubZelda = id;
        s_hubZeldaRetiring = s_hubZeldaRewardPending = false;
    }
}
