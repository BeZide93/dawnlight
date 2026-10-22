// Private native flow: no patches to the story/Boss Rush Midna menu or its edges.
mods::flow::Graph s_midnaGraph;
mods::flow::RegisteredMessage s_midnaPrompt, s_midnaOptions;
mods::flow::Event s_midnaEndEvent, s_midnaRestartEvent;
u16 s_midnaStart = 0;

void midna_end_run(ModContext*, const FlowEventContext*, void*) {
    if (in_arena() && s_run.phase==Phase::Running && s_midnaTalking && !s_midnaRestartPending)
        s_midnaEndPending=true;
}
void midna_restart_run(ModContext*, const FlowEventContext*, void*) {
    if (in_arena() && s_run.phase==Phase::Running && s_midnaTalking && !s_midnaEndPending)
        s_midnaRestartPending=true;
}
mods::flow::RegisteredMessage register_survivors_dialog(const mods::flow::MessageBuilder& builder) {
    std::vector<mods::flow::MessageVariant> variants;
    for (auto language : {MESSAGE_LANGUAGE_ENGLISH, MESSAGE_LANGUAGE_GERMAN,
            MESSAGE_LANGUAGE_FRENCH, MESSAGE_LANGUAGE_SPANISH, MESSAGE_LANGUAGE_ITALIAN,
            MESSAGE_LANGUAGE_JAPANESE}) variants.push_back(builder.build(language));
    return mods::flow::register_message(0,variants);
}
ModResult initialize_midna_dialog() {
    const auto style=mods::flow::MessageStyle{}.speaker(21).box_kind(MESSAGE_BOX_MIDNA)
        .draw_type(MESSAGE_DRAW_INSTANT).talk_anim(31).face_anim(31);
    s_midnaPrompt=register_survivors_dialog(mods::flow::MessageBuilder{style}
        .text("What would you like to do?\n").await_choice());
    if (!s_midnaPrompt) return s_midnaPrompt.result();
    s_midnaOptions=register_survivors_dialog(mods::flow::MessageBuilder{}.speaker(21)
        .options("Cancel","End Run","Restart Run"));
    if (!s_midnaOptions) return s_midnaOptions.result();
    s_midnaEndEvent=mods::flow::register_event("Survivors End Run",midna_end_run);
    if (!s_midnaEndEvent) return s_midnaEndEvent.result();
    s_midnaRestartEvent=mods::flow::register_event("Survivors Restart Run",midna_restart_run);
    if (!s_midnaRestartEvent) return s_midnaRestartEvent.result();
    mods::flow::GraphBuilder graph{0};
    const auto setup=graph.add_event(FLOW_EVENT_SELECT_VERTICAL,{0,0,0,4});
    const auto prompt=graph.add_message(s_midnaPrompt.id());
    const auto options=graph.add_message(s_midnaOptions.id());
    const auto choice=graph.add_branch(FLOW_QUERY_SELECT_3_CANCEL,0);
    const auto end=graph.add_event(s_midnaEndEvent.id(),{0,0,0,0});
    const auto restart=graph.add_event(s_midnaRestartEvent.id(),{0,0,0,0});
    setup.next(prompt); prompt.next(options); options.next(choice);
    // Cancel is both the initially selected option and the controller back path.
    choice.results({mods::flow::kEnd,end.id(),restart.id(),mods::flow::kEnd});
    end.next(mods::flow::kEnd); restart.next(mods::flow::kEnd);
    s_midnaGraph=graph.commit();
    if (!s_midnaGraph) return s_midnaGraph.result();
    s_midnaStart=setup.id();
    return MOD_OK;
}
HookAction before_midna_flow(ModContext*, void* args, void* result, void*) {
    auto* midna=daPy_py_c::getMidnaActor();
    if (!in_arena() || s_run.phase!=Phase::Running || !midna || !s_midnaStart ||
        mods::arg<dMsgFlow_c*>(args,0)!=&midna->mMsgFlow || mods::arg<u16>(args,1)!=0xbb9)
        return HOOK_CONTINUE;
    // Keep native init(3001): it binds the common message archive before resolving
    // this private node. Passing a custom flow ID instead would select a stage group.
    *static_cast<u16*>(result)=s_midnaStart;
    s_midnaRequestFrames=0; s_midnaTalking=true;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_midna_order(ModContext*, void*, void* result, void*) {
    if (!in_arena()) return HOOK_CONTINUE;
    // Own the call below, including the throne room where orderZTalk is disabled.
    // This also prevents Z-hints, transformation and warp menus in this mode.
    *static_cast<int*>(result)=0;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_midna_call(ModContext*, void* args, void*, void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    auto* midna=daPy_py_c::getMidnaActor();
    if (!link || fopAcM_GetID(link)!=s_player ||
        s_run.phase!=Phase::Running || !playable() || link->checkDeadHP()) return HOOK_CONTINUE;
    const bool equip=gear_notice_equip_available();
    if(!equip && (!midna || !s_midnaStart || s_victory ||
        !link->mLinkAcch.ChkGroundHit() || !link->checkMidnaRide()))return HOOK_CONTINUE;
    dMeter2Info_onUseButton(METER2_USEBUTTON_Z);
    // Preserve custom controller bindings and Dawnlight's separate touch button.
    if (!link->midnaTalkTrigger()) return HOOK_CONTINUE;
    if(equip) {equip_gear_notice();return HOOK_CONTINUE;}
    clear_attacks();
    s_swordAction=SwordAction::None; s_wavePending=false; s_comboStep=s_comboTimeout=0;
    for (auto& collider : link->mAtCps) { collider.OffAtSetBit(); collider.ResetAtHit(); }
    link->mAtSph.OffAtSetBit(); link->mAtSph.ResetAtHit();
    link->resetUpperAnime(daAlink_c::UPPER_2,0);
    link->procWaitInit();
    // Freeze run time/enemies immediately while the native event request is queued.
    // An unaccepted request expires rather than leaving combat permanently paused.
    s_midnaRequestFrames=30;
    dComIfGp_setMesgCameraInfoActor(nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr);
    fopAcM_orderTalkEvent(link,midna,0,0);
    link->field_0x35a0=link->field_0x3594;
    return HOOK_CONTINUE;
}
void update_midna_dialog() {
    if (!midna_dialog_busy()) return;
    if (!in_arena() || transitioning() || s_run.phase!=Phase::Running) {
        reset_midna_dialog(); return;
    }
    if (s_midnaRequestFrames) --s_midnaRequestFrames;
    if (!s_midnaTalking) return;
    auto* midna=daPy_py_c::getMidnaActor();
    if (dComIfGp_event_runCheck() || dMsgObject_isTalkNowCheck() ||
        (midna && midna->eventInfo.checkCommandTalk())) return;
    // Let Midna's native doFlow finish its message, animation and event reset.
    // Credit the completed run once before reloading and starting the same map.
    const bool end=s_midnaEndPending, restart=s_midnaRestartPending;
    reset_midna_dialog();
    if (end || restart) finish(false);
    if (restart) restart_run(nullptr,nullptr);
}
