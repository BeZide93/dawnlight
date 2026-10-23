// Native Midna floor portals, with only their visual execution retained. Never
// enter the native warp/event path, including Execute called from Create().
constexpr unsigned kHordePortalCount=4;
constexpr unsigned kHordePortalRespawn=3*60*ticks_per_second;
struct HordePortal {
    ActorId id=kNone;
    cXyz position{};
    unsigned respawnAt=0;
    bool initialized=false,consumed=false,retiring=false;
};
std::array<HordePortal,kHordePortalCount> s_hordePortals;
unsigned s_hordePortalDelay=0;
HordePortal* tracked_horde_portal(ActorId id) {
    if(id==kNone)return nullptr;
    for(auto& portal:s_hordePortals)if(portal.id==id)return &portal;
    return nullptr;
}
bool horde_portals_running() {
    return (s_map==1 || s_map==2) && s_run.phase==Phase::Running && !s_victory &&
        (s_run.endless || s_run.ticks<s_run.run_duration()) && playable();
}
bool horde_portal_visible(const HordePortal& portal) {
    return portal.id!=kNone && portal.initialized && !portal.retiring && !portal.consumed &&
        actor_by_id(portal.id)!=nullptr;
}
void retire_horde_portals() {
    s_hordePortalDelay=0;
    for(auto& portal:s_hordePortals) {
        if(portal.id==kNone)portal={};
        else {portal.retiring=true;portal.consumed=false;}
    }
}
void cleanup_horde_portals() {
    for(auto& portal:s_hordePortals) {
        if(portal.id==kNone)continue;
        auto* actor=actor_by_id(portal.id);
        if(!actor && !fpcM_IsCreating(portal.id)) {
            portal.id=kNone;portal.initialized=false;
            if(portal.retiring && !portal.consumed)portal={};
            else portal.retiring=false;
        } else if(actor && portal.retiring)svc_actor->delete_actor(mod_ctx,portal.id);
    }
}
bool horde_portal_position(const HordePortal& slot,daAlink_c* link,cXyz& pos) {
    if(!random_arena_floor(pos) || pos.absXZ(link->current.pos)<500 || !separated_guard(pos,450))return false;
    // Keep four map-wide destinations, including a genuinely new respawn site.
    const float spacing=std::clamp(std::sqrt(arena_distribution_area()/kHordePortalCount)*.4f,900.f,6000.f);
    if(slot.consumed && pos.absXZ(slot.position)<spacing)return false;
    for(const auto& portal:s_hordePortals)
        if(portal.id!=kNone && pos.absXZ(portal.position)<spacing)return false;
    for(const auto& chest:s_chests)if(chest.active && pos.absXZ(chest.position)<350)return false;
    for(const auto& pot:s_pots)if(pot.id!=kNone && pos.absXZ(pot.position)<250)return false;
    return true;
}
void update_horde_portals(daAlink_c* link) {
    if(!horde_portals_running() || !s_arenaMeshReady || !arena_distribution_ready())return;
    if(s_hordePortalDelay){--s_hordePortalDelay;return;}
    for(auto& portal:s_hordePortals) {
        if(portal.id!=kNone || s_run.ticks<portal.respawnAt)continue;
        cXyz pos;
        if(!horde_portal_position(portal,link,pos)){s_hordePortalDelay=5;return;}
        // Same parameters as createWarpHole(pos, angle, room, 0xff, 0, 0xff).
        // Our Execute hook is installed before any actor can be requested.
        const ActorSpawnParams params{
            .parameters=0x17ffffff,.argument=-1,.room_num=arena().room,
            .position={pos.x,pos.y,pos.z},.angle={0,0,0},.scale={1,1,1},.create_function=nullptr,
        };
        ActorId id=kNone;
        if(create_standalone_actor(fpcNm_Obj_BossWarp_e,params,id)==MOD_OK && id!=kNone) {
            portal={};portal.id=id;portal.position=pos;s_hordePortalDelay=5;
        } else s_hordePortalDelay=30;
        return; // At most one asynchronous native actor creation per pass.
    }
}
HookAction before_horde_portal_execute(ModContext*,void* args,void* result,void*) {
    auto* warp=mods::arg<daObjBossWarp_c*>(args,0);
    auto* portal=warp?tracked_horde_portal(fopAcM_GetID(warp)):nullptr;
    if(!portal)return HOOK_CONTINUE;
    if(!portal->retiring) {
        if(!portal->initialized) {
            warp->scale.y=1;
            warp->set_appear();
            warp->mpBrkAnm->setPlaySpeed(0);
            warp->mpBtkAnm[1]->setPlaySpeed(0);
            portal->initialized=true;
        }
        if(playable())warp->mpBtkAnm[0]->play();
        warp->setBaseMtx();
    }
    *static_cast<int*>(result)=1;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_horde_portal_action(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!link || fopAcM_GetID(link)!=s_player || !horde_portals_running() ||
        link->checkWolf() || !dComIfGs_getLife())return HOOK_CONTINUE;
    for(auto& portal:s_hordePortals) {
        if(!horde_portal_visible(portal) || portal.position.absXZ(link->current.pos)>=150 ||
            std::abs(portal.position.y-link->current.pos.y)>=35)continue;
        cXyz from=link->current.pos+cXyz(0,50,0),to=portal.position+cXyz(0,50,0);
        dBgS_LinChk wall;wall.SetLink();wall.Set(&from,&to,link);
        if(dComIfG_Bgsp().LineCross(&wall))continue;
        dComIfGp_setDoStatusForce(BUTTON_STATUS_ENTER,0);
        if(mDoCPd_c::getTrigA(PAD_1)) {
            s_run.horde_remaining+=horde_duration;
            s_spawnCooldown=0;
            portal.consumed=portal.retiring=true;
            portal.respawnAt=s_run.ticks+kHordePortalRespawn;
            static_cast<daObjBossWarp_c*>(actor_by_id(portal.id))->disappear(0);
            mDoCPd_c::getCpadInfo(PAD_1).mPressedButtonFlags&=~PAD_BUTTON_A;
        }
        return HOOK_CONTINUE;
    }
    return HOOK_CONTINUE;
}
