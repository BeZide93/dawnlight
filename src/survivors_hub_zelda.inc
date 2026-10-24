// Native princess Zelda, owned by the Survivors hub. Story Zeldas and their
// message flows never enter this path.
ActorId s_hubZelda = kNone;
bool s_hubZeldaRetiring = false, s_hubZeldaRewardPending = false;
unsigned s_hubZeldaRetry = 0;
mods::flow::Graph s_hubZeldaGraph;
mods::flow::RegisteredMessage s_hubZeldaMessage, s_hubZeldaCallMessage;
mods::flow::RegisteredMessage s_hubZeldaAfterGiftMessage, s_hubZeldaTalentsMessage;
mods::flow::Event s_hubZeldaReward, s_hubZeldaTalentsEvent;
u16 s_hubZeldaStart = 0, s_hubZeldaCallStart = 0, s_hubZeldaAfterGiftStart = 0, s_hubZeldaTalentsStart = 0;
enum class HubZeldaTalk { None, Reminder, AfterGift, Talents };
HubZeldaTalk s_hubZeldaPendingTalk = HubZeldaTalk::None, s_hubZeldaActiveTalk = HubZeldaTalk::None;
bool s_hubZeldaTalentsPending = false;
unsigned s_hubZeldaCallFrames = 0;
constexpr u16 kHubZeldaFlow = 3001; // Native common-message archive selection.
struct HubZeldaGift {
    ActorId id = kNone;
    bool queued = false, started = false, poseStarted = false, retiring = false;
    unsigned waitFrames = 0;
    ActorId poseOwner = kNone;
    J3DModel* handSword = nullptr;
    bool bladeWasHidden = false;
} s_hubZeldaGift;

bool owns_hub_zelda_gift_event() {
    return s_hubZeldaGift.id != kNone && dComIfGp_getEvent()->mPt2 == s_hubZeldaGift.id;
}
bool hub_zelda_gift_busy() { return s_hubZeldaGift.queued || s_hubZeldaGift.id != kNone; }
fopAc_ac_c* hub_zelda_gift_actor() {
    auto* actor = actor_by_id(s_hubZeldaGift.id);
    return actor && fopAcM_GetName(actor) == fpcNm_Demo_Item_e ? actor : nullptr;
}
// Reuse the correctly oriented Ordon model, which Link loads even before
// obtaining it, rather than the differently authored spinning Demo_Item mesh.
bool hub_zelda_gift_pose(daAlink_c* link) {
    return link && s_hubZeldaGift.poseStarted && !s_hubZeldaGift.retiring &&
        owns_hub_zelda_gift_event() && fopAcM_GetID(link) == s_hubZeldaGift.poseOwner &&
        link->mProcID == daAlink_c::PROC_GET_ITEM &&
        link->checkUnderMove0BckNoArc(daAlink_c::ANM_GET_MASTER_SWORD);
}
void restore_hub_zelda_sword() {
    auto* link = daAlink_getAlinkActorClass();
    // Do not dereference a model from a deleted/replaced player on scene exit.
    if (s_hubZeldaGift.handSword && link && fopAcM_GetID(link) == s_hubZeldaGift.poseOwner &&
        link->mpSwAModel == s_hubZeldaGift.handSword) {
        auto* shape = link->mpSwAModel->getModelData()->getMaterialNodePointer(0)->getShape();
        if (s_hubZeldaGift.bladeWasHidden) shape->hide(); else shape->show();
    }
    s_hubZeldaGift.handSword = nullptr;
}
void cleanup_hub_zelda_gift() {
    if (s_hubZeldaGift.id == kNone || fpcM_IsCreating(s_hubZeldaGift.id)) return;
    if (!hub_zelda_gift_actor()) {
        if (in_arena() && !transitioning() && owns_hub_zelda_gift_event()) dComIfGp_event_reset();
        restore_hub_zelda_sword();
        s_hubZeldaGift = {};
    }
    else if (s_hubZeldaGift.retiring) svc_actor->delete_actor(mod_ctx, s_hubZeldaGift.id);
}
void retire_hub_zelda_gift() {
    restore_hub_zelda_sword();
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
        if (owned) {
            execItemGet(dItemNo_SWORD_e);
            s_hubZeldaPendingTalk = HubZeldaTalk::AfterGift;
            s_hubZeldaCallFrames = 0;
            s_hubSaveRequested = true; // Persist the first-get bit after the event closes.
        }
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
    // GETSWL is the overhead, closed-grip pose also used for Copy Rod revival.
    // Wait for native sheathing to finish before starting it, exactly once.
    link->setSingleAnimeBase(daAlink_c::ANM_GET_MASTER_SWORD);
    link->mProcVar1.field_0x300a = 0;
    s_hubZeldaGift.poseStarted = true;
    s_hubZeldaGift.poseOwner = fopAcM_GetID(link);
}

HookAction before_hub_zelda_get_tick(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!hub_zelda_gift_pose(link)) return HOOK_CONTINUE;
    auto& lower = link->mUnderFrameCtrl[0];
    if (lower.getEndFlg() || lower.getFrame() >= lower.getEnd() - 1.0f) {
        // Keep the final raised pose. Allow the native message/cut-end branch
        // to run, but not its transition back to the chest-height GET_A_WAIT.
        for (auto* ctrl : {&lower, &link->mUpperFrameCtrl[0]}) {
            ctrl->setFrame(ctrl->getEnd() - 1.0f);
            ctrl->setRate(0.0f);
            // checkAnmEnd marks a zero-rate controller ended each call.
            // Clear both flags each tick so it keeps returning false.
            ctrl->offEndFlg();
        }
        link->mProcVar1.field_0x300a = 1;
    }
    return HOOK_CONTINUE;
}
void after_hub_zelda_item_matrix(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!hub_zelda_gift_pose(link) || !link->mpSwAModel) {
        restore_hub_zelda_sword();
        return;
    }
    auto* sword = link->mpSwAModel;
    auto* shape = sword->getModelData()->getMaterialNodePointer(0)->getShape();
    if (!s_hubZeldaGift.handSword) {
        s_hubZeldaGift.handSword = sword;
        s_hubZeldaGift.bladeWasHidden = shape->checkFlag(J3DShpFlag_Visible);
    }
    shape->show();
    // Use the native sword grip joint, not the wrist (getLeftHandMatrix).
    // Its local offset/orientation aligns the hilt with the closed fingers.
    sword->setBaseTRMtx(link->getLeftItemMatrix());
    link->modelCalc(sword);
}
HookAction before_hub_zelda_gift_draw(ModContext*, void* args, void* result, void*) {
    auto* process = mods::arg<leafdraw_class*>(args, 0);
    if (!process || fopAcM_GetID(process) != s_hubZeldaGift.id ||
        !s_hubZeldaGift.handSword || !hub_zelda_gift_pose(daAlink_getAlinkActorClass())) return HOOK_CONTINUE;
    auto* link = daAlink_getAlinkActorClass();
    if (link->mSwordModel != s_hubZeldaGift.handSword || !link->checkSwordDraw()) {
        // Before the first grant the native equipment renderer hides swords.
        // Draw only the presentation model, without granting/equipping early.
        auto* item = hub_zelda_gift_actor();
        if (!item) return HOOK_CONTINUE;
        dKy_tevstr_c tev{}; g_env_light.settingTevStruct(0, &item->current.pos, &tev);
        g_env_light.setLightTevColorType_MAJI(s_hubZeldaGift.handSword, &tev);
        mDoExt_modelUpdateDL(s_hubZeldaGift.handSword);
    }
    // Only suppress this event's floating display. Its actor still executes
    // the native item message, fanfare and event completion normally.
    *static_cast<int*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
}

bool owned_hub_zelda(fopAc_ac_c* actor) {
    return actor && s_hubZelda != kNone && fopAcM_GetID(actor) == s_hubZelda &&
        fopAcM_GetName(actor) == fpcNm_NPC_ZELDA_e;
}
bool hub_zelda_enabled() {
    return survivors_hub_active() && !s_hubDeparture && !transitioning();
}
void clear_hub_zelda_call() {
    s_hubZeldaPendingTalk = s_hubZeldaActiveTalk = HubZeldaTalk::None;
    s_hubZeldaTalentsPending = false;
    s_hubZeldaCallFrames = 0;
}
void request_hub_zelda_call() {
    if (!hub_zelda_enabled() || s_hubZeldaRetiring || hub_zelda_gift_busy() ||
        s_hubZeldaPendingTalk != HubZeldaTalk::None || s_hubZeldaActiveTalk != HubZeldaTalk::None ||
        dComIfGs_isItemFirstBit(dItemNo_SWORD_e) ||
        !owned_hub_zelda(actor_by_id(s_hubZelda))) return;
    s_hubZeldaRewardPending = false;
    s_hubZeldaPendingTalk = HubZeldaTalk::Reminder;
    s_hubZeldaCallFrames = 0;
}
void update_hub_zelda_call() {
    if (s_hubZeldaPendingTalk == HubZeldaTalk::None && s_hubZeldaActiveTalk == HubZeldaTalk::None) return;
    auto* actor = actor_by_id(s_hubZelda);
    if (!owned_hub_zelda(actor)) {clear_hub_zelda_call(); return;}
    if (s_hubZeldaActiveTalk != HubZeldaTalk::None) {
        if (dComIfGp_event_runCheck()) {
            if (dComIfGp_getEvent()->mPt2 != s_hubZelda) clear_hub_zelda_call();
            return; // Native Zelda still owns input, text and the talk camera.
        }
        if (s_hubZeldaTalentsPending) {
            if (dMsgObject_isTalkNowCheck() || !playable() || s_progressSaving || s_hubSaveRequested) return;
            clear_hub_zelda_call();
            survivors_open_talents(); // Only after the native conversation closes.
        } else clear_hub_zelda_call();
        return;
    }
    // Finish item actor cleanup and any pending native save before ordering
    // the follow-up conversation. An interrupted gift never queues this path.
    if (hub_zelda_gift_busy() || !playable() || s_progressSaving || s_hubSaveRequested) return;
    const bool received = dComIfGs_isItemFirstBit(dItemNo_SWORD_e);
    if (++s_hubZeldaCallFrames > 90 ||
        (s_hubZeldaPendingTalk == HubZeldaTalk::Reminder ? received : !received)) {
        clear_hub_zelda_call(); return;
    }
    auto* link = daAlink_getAlinkActorClass();
    if (!link || !link->eventInfo.chkCondition(dEvtCnd_CANTALK_e) ||
        !actor->eventInfo.chkCondition(dEvtCnd_CANTALK_e)) return;
    fopAcM_orderTalkEvent(link, actor, 0, 0);
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
    if (s_hubZeldaActiveTalk != HubZeldaTalk::None && in_arena() && !transitioning() &&
        dComIfGp_getEvent()->mPt2 == s_hubZelda) dComIfGp_event_reset();
    clear_hub_zelda_call();
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
        !hub_zelda_enabled() || s_hubZeldaRetiring || !s_hubZeldaRewardPending ||
        dComIfGs_isItemFirstBit(dItemNo_SWORD_e) || hub_zelda_gift_busy()) return;
    s_hubZeldaRewardPending = false;
    s_hubZeldaGift = {};
    s_hubZeldaGift.queued = true;
}
void hub_zelda_open_talents(ModContext*, const FlowEventContext* event, void*) {
    auto* actor = actor_by_id(s_hubZelda);
    if (!event || event->speaker_actor != actor || !owned_hub_zelda(actor) ||
        !hub_zelda_enabled() || s_hubZeldaRetiring || s_hubZeldaActiveTalk != HubZeldaTalk::Talents ||
        dComIfGp_getEvent()->mPt2 != s_hubZelda || !dComIfGs_isItemFirstBit(dItemNo_SWORD_e)) return;
    s_hubZeldaTalentsPending = true;
}
ModResult initialize_hub_zelda() {
    s_hubZeldaMessage = register_survivors_dialog(mods::flow::MessageBuilder{}
        .box_kind(MESSAGE_BOX_TALK).talk_anim(0).face_anim(0)
        .text("It's dangerous to go alone! Take this."));
    if (!s_hubZeldaMessage) return s_hubZeldaMessage.result();
    s_hubZeldaCallMessage = register_survivors_dialog(mods::flow::MessageBuilder{}
        .box_kind(MESSAGE_BOX_TALK).talk_anim(0).face_anim(0).camera_attr(1)
        .text("Wait! Come here!"));
    if (!s_hubZeldaCallMessage) return s_hubZeldaCallMessage.result();
    s_hubZeldaAfterGiftMessage = register_survivors_dialog(mods::flow::MessageBuilder{}
        .box_kind(MESSAGE_BOX_TALK).talk_anim(0).face_anim(0)
        .text("Come to me after your journey to get some rest and strengthen your ")
        .text_color(MESSAGE_COLOR_RED).text("Talents").text_color(MESSAGE_COLOR_DEFAULT).text("."));
    if (!s_hubZeldaAfterGiftMessage) return s_hubZeldaAfterGiftMessage.result();
    s_hubZeldaTalentsMessage = register_survivors_dialog(mods::flow::MessageBuilder{}
        .box_kind(MESSAGE_BOX_TALK).talk_anim(0).face_anim(0)
        .text("Let me help you on your journey."));
    if (!s_hubZeldaTalentsMessage) return s_hubZeldaTalentsMessage.result();
    s_hubZeldaTalentsEvent = mods::flow::register_event("Survivors Zelda Talents", hub_zelda_open_talents);
    if (!s_hubZeldaTalentsEvent) return s_hubZeldaTalentsEvent.result();
    s_hubZeldaReward = mods::flow::register_event("Survivors Zelda Ordon Sword", hub_zelda_give_sword);
    if (!s_hubZeldaReward) return s_hubZeldaReward.result();
    mods::flow::GraphBuilder graph{0};
    const auto text = graph.add_message(s_hubZeldaMessage.id());
    const auto reward = graph.add_event(s_hubZeldaReward.id(), {0,0,0,0});
    text.next(reward); reward.next(mods::flow::kEnd);
    const auto call = graph.add_message(s_hubZeldaCallMessage.id());
    call.next(mods::flow::kEnd); // Calling from the exit never hands out the sword.
    const auto afterGift = graph.add_message(s_hubZeldaAfterGiftMessage.id());
    afterGift.next(mods::flow::kEnd);
    const auto talents = graph.add_message(s_hubZeldaTalentsMessage.id());
    const auto openTalents = graph.add_event(s_hubZeldaTalentsEvent.id(), {0,0,0,0});
    talents.next(openTalents); openTalents.next(mods::flow::kEnd);
    s_hubZeldaGraph = graph.commit();
    if (!s_hubZeldaGraph) return s_hubZeldaGraph.result();
    s_hubZeldaStart = text.id();
    s_hubZeldaCallStart = call.id();
    s_hubZeldaAfterGiftStart = afterGift.id();
    s_hubZeldaTalentsStart = talents.id();
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
    s_hubZeldaRewardPending = false;
    if ((s_hubZeldaPendingTalk != HubZeldaTalk::None || s_hubZeldaActiveTalk == HubZeldaTalk::Reminder ||
         s_hubZeldaActiveTalk == HubZeldaTalk::AfterGift) && dComIfGp_getEvent()->mPt2 == s_hubZelda) {
        if (s_hubZeldaPendingTalk != HubZeldaTalk::None) s_hubZeldaActiveTalk = s_hubZeldaPendingTalk;
        s_hubZeldaPendingTalk = HubZeldaTalk::None;
        *static_cast<u16*>(result) = s_hubZeldaActiveTalk == HubZeldaTalk::AfterGift ?
            s_hubZeldaAfterGiftStart : s_hubZeldaCallStart;
        // Flow init has already cleared the old camera actors. Attribute 1
        // on the exit reminder focuses Zelda using native camera easing.
        dComIfGp_setMesgCameraInfoActor(actor,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr);
        return HOOK_SKIP_ORIGINAL;
    }
    if (dComIfGs_isItemFirstBit(dItemNo_SWORD_e)) {
        *static_cast<u16*>(result) = s_hubZeldaTalentsStart;
        s_hubZeldaActiveTalk = HubZeldaTalk::Talents;
        s_hubZeldaTalentsPending = false;
        return HOOK_SKIP_ORIGINAL;
    }
    *static_cast<u16*>(result) = s_hubZeldaStart;
    s_hubZeldaRewardPending = true;
    return HOOK_SKIP_ORIGINAL;
}
void update_hub_zelda() {
    const bool title = survivors_title_preparing();
    if (!title && !hub_zelda_enabled()) { retire_hub_zelda(); return; }
    if (!title) update_hub_zelda_gift();
    cleanup_hub_zelda();
    if (!title) update_hub_zelda_call();
    if (s_hubZelda != kNone || !s_hubZeldaStart || (!title && !playable()) ||
        s_progressSaving || s_hubSaveRequested) return;
    if (s_hubZeldaRetry) { --s_hubZeldaRetry; return; }
    cXyz anchor;
    if (!hub_brazier_anchor(anchor) && !(title && hub_center_ground(anchor))) return;
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
    } else s_hubZeldaRetry = 30;
}
