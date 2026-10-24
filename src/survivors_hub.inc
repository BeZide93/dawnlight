// Intercept the native A/door request before an event starts. Once Start run is
// confirmed we replay that same event, and redirect only its final scene change.
HookAction before_hub_door(ModContext*,void* args,void* result,void*) {
    if(!in_arena())return HOOK_CONTINUE;
    auto* link=mods::arg<fopAc_ac_c*>(args,0);
    auto* door=mods::arg<fopAc_ac_c*>(args,1);
    if(s_allowHubDoor && s_hub && s_hubDeparture && door && fopAcM_GetID(door)==s_hubDoor)
        return HOOK_CONTINUE;
    if(s_hub && s_run.phase==Phase::Ready && !s_hubDeparture && !s_progressSaving &&
       link && door && fopAcM_GetID(link)==s_player && playable()) {
        s_hubDoor=fopAcM_GetID(door);
        s_hubDoorPriority=mods::arg<u16>(args,2);s_hubDoorFlags=mods::arg<u16>(args,3);
        if (!dComIfGs_isItemFirstBit(dItemNo_SWORD_e)) request_hub_zelda_call();
        else {s_hubMenu=HubMenu::Run;open_window();}
    }
    *static_cast<s32*>(result)=0;
    return HOOK_SKIP_ORIGINAL;
}
bool hub_chest_position(cXyz& position,const daAlink_c* link) {
    // The marked wall is on the +X side of the central brazier. Leave space
    // for the chest's back instead of embedding its model in the wall trim.
    cXyz from(0,link->current.pos.y+100,0),to(10000,from.y,0);
    dBgS_LinChk wall;wall.Set(&from,&to,nullptr);
    if(!dComIfG_Bgsp().LineCross(&wall))return false;
    position=wall.GetCross();position.x-=150;
    if(position.x<=0)return false;
    cXyz probe(position.x,link->current.pos.y+1000,position.z);
    dBgS_ObjGndChk ground;ground.SetPos(&probe);
    const float height=dComIfG_Bgsp().GroundCross(&ground);
    if(!std::isfinite(height) || height<=-1.e8f)return false;
    position.y=height;return true;
}
void update_hub(daAlink_c* link) {
    const bool enabled=s_run.phase==Phase::Ready && !s_hubDeparture;
    if(enabled) {
        cXyz position;
        // Chest front is local +Z; -90 degrees points inward from the +X wall.
        if(hub_chest_position(position,link))update_survivors_equipment_chest(true,position,-0x4000);
    } else update_survivors_equipment_chest(false,{},0);
    if(!s_hubDeparture || s_hubClosingWindow || ui_visible() || s_progressSaving || s_hubSaveRequested)return;
    if(s_hubDoorStarted) {
        // A rejected/aborted native event must never leave the hub locked. No
        // synthetic teleport: the next interaction can retry the real door.
        if(++s_hubDoorTicks>600 && !dComIfGp_event_runCheck()) {
            s_hubDeparture=s_hubDoorStarted=false;s_hubDoorTicks=0;
            svc_log->warn(mod_ctx,"Survivors: door event ended without a scene change; please retry");
        }
        return;
    }
    auto* door=actor_by_id(s_hubDoor);
    if(!door) {s_hubDeparture=false;s_hubDoorTicks=0;return;}
    // orderDoorEvent only enqueues a request. commonCheck can reject it later
    // if either actor has lost CANDOOR while the menu was open.
    if(link->eventInfo.checkCommandDoor() && door->eventInfo.checkCommandDoor()) {
        s_hubDoorStarted=true;s_hubDoorTicks=0;return;
    }
    if(!playable())return;
    if(++s_hubDoorTicks>90) {
        s_hubDeparture=false;s_hubDoorTicks=0;
        svc_log->warn(mod_ctx,"Survivors: door event was not accepted; please retry");
        return;
    }
    // Let native actor updates rebuild eligibility; never fabricate conditions
    // or mark an enqueued request as an accepted door animation.
    if(!link->eventInfo.chkCondition(dEvtCnd_CANDOOR_e) ||
       !door->eventInfo.chkCondition(dEvtCnd_CANDOOR_e))return;
    s_allowHubDoor=true;
    fopAcM_orderDoorEvent(link,door,s_hubDoorPriority,s_hubDoorFlags);
    s_allowHubDoor=false;
}
void* hub_exit_actor(void* ptr,void*) {
    if(!fopAcM_IsActor(ptr))return nullptr;
    auto* actor=static_cast<fopAc_ac_c*>(ptr);
    // Remembered interaction targets are authoritative, including modded doors.
    if(fopAcM_GetID(actor)==s_hubDoor)return ptr;
    const auto name=fopAcM_GetName(actor);
    if(name!=fpcNm_DOOR20_e && name!=fpcNm_KNOB20_e && name!=fpcNm_DBDOOR_e &&
       name!=fpcNm_BOSS_DOOR_e && name!=fpcNm_L1BOSS_DOOR_e &&
       name!=fpcNm_L1MBOSS_DOOR_e && name!=fpcNm_L5BOSS_DOOR_e && name!=fpcNm_SPIRAL_DOOR_e)return nullptr;
    const auto room=arenas[0].room;
    return fopAcM_GetRoomNo(actor)==room || fopAcM_GetRoomNo(actor)==-1 ||
        door_param2_c::getFRoomNo(actor)==room || door_param2_c::getBRoomNo(actor)==room ? ptr : nullptr;
}
void hub_label(const char* text,cXyz world) {
    auto* font=mDoExt_getRubyFont();
    if(!font || !dComIfGd_getView())return;
    const auto& matrix=*dComIfGd_getProjViewMtx();
    const float w=matrix[3][0]*world.x+matrix[3][1]*world.y+matrix[3][2]*world.z+matrix[3][3];
    if(w<=1)return;
    cXyz screen;mDoLib_project(&world,&screen);
    if(!std::isfinite(screen.x)||!std::isfinite(screen.y)||screen.x<mDoGph_gInf_c::getMinXF()||
       screen.x>mDoGph_gInf_c::getMaxXF()||screen.y<0||screen.y>mDoGph_gInf_c::getHeightF())return;
    const float size=std::clamp(24000.f/w,16.f,28.f);
    J2DPrint shadow(font,JUtility::TColor(0,0,0,230),JUtility::TColor(0,0,0,230));
    shadow.setFontSize(size,size);
    shadow.printReturn(text,550,size+8,HBIND_CENTER,VBIND_TOP,screen.x-274,screen.y+1,255);
    J2DPrint print(font,JUtility::TColor(255,240,210,255),JUtility::TColor(230,190,150,255));
    print.setFontSize(size,size);
    print.printReturn(text,550,size+8,HBIND_CENTER,VBIND_TOP,screen.x-275,screen.y,255);
}
void* draw_hub_exit_label(void* ptr,void*) {
    if(auto* door=static_cast<fopAc_ac_c*>(hub_exit_actor(ptr,nullptr)))
        hub_label("Start Run",door->current.pos+cXyz(0,400,0));
    return nullptr; // Visit every exit; the first actor may be off camera.
}
void draw_hub_labels() {
    if(!survivors_hub_active()||transitioning()||ui_visible()||dComIfGp_event_runCheck())return;
    auto* graf=dComIfGp_getCurrentGrafPort();if(!graf)return;
    graf->setup2D();
    cXyz chestAnchor;
    if(survivors_equipment_chest_anchor(chestAnchor))hub_label("Equipment",chestAnchor+cXyz(0,200,0));
    fpcM_Search(draw_hub_exit_label,nullptr);
    graf->setup2D();
    if(s_storeFailed)label("Progress save failed. Retry Save progress with Zelda.",
        mDoGph_gInf_c::getMinXF()+mDoGph_gInf_c::getWidthF()*.5f,40,16);
    graf->setup2D();
}
