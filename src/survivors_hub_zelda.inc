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
    s_hubZeldaRetiring = true;
    s_hubZeldaRewardPending = false;
    s_hubZeldaRetry = 0;
    // Keep pending IDs until creation/deletion finishes. Never spawn a second
    // Zelda while the previous asynchronous request still owns an actor.
    cleanup_hub_zelda();
}
void hub_zelda_give_rupees(ModContext*, const FlowEventContext* event, void*) {
    auto* actor = actor_by_id(s_hubZelda);
    if (!event || event->speaker_actor != actor || !owned_hub_zelda(actor) ||
        !hub_zelda_enabled() || s_hubZeldaRetiring || !s_hubZeldaRewardPending) return;
    s_hubZeldaRewardPending = false;
    // Use the native wallet queue for its cap, HUD count-up and rupee sound.
    dComIfGp_setItemRupeeCount(10);
}
ModResult initialize_hub_zelda() {
    s_hubZeldaMessage = register_survivors_dialog(mods::flow::MessageBuilder{}
        .box_kind(MESSAGE_BOX_TALK).talk_anim(0).face_anim(0)
        .text("It's dangerous to go alone! Take this."));
    if (!s_hubZeldaMessage) return s_hubZeldaMessage.result();
    s_hubZeldaReward = mods::flow::register_event("Survivors Zelda rupees", hub_zelda_give_rupees);
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
HookAction before_hub_zelda_type(ModContext*, void* args, void* result, void*) {
    if (!owned_hub_zelda(mods::arg<daNpc_Zelda_c*>(args, 0))) return HOOK_CONTINUE;
    // The native type-1 path has ground collision, standing idle and player
    // facing. Type 0 is the suspended throne-room Zelda with no gravity.
    *static_cast<u8*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
}
void after_hub_zelda_param(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<daNpc_Zelda_c*>(args, 0);
    if (owned_hub_zelda(actor) && (!hub_zelda_enabled() || s_hubZeldaRetiring ||
        !playable() || s_progressSaving || s_hubSaveRequested)) actor->attention_info.flags = 0;
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
