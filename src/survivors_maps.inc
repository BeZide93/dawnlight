// Arena-only stage edits. The user's story files and the native room archives
// are never patched. In particular, filtering happens before actor creation,
// not by deleting a boss after its opening event has already started.
std::vector<barriers::Wall> s_barriers;
std::vector<ActorId> s_arenaBackgrounds;
bool s_arenaMeshReady = false;
cXyz s_arenaSafePosition;
bool s_arenaSafe = false;
cXyz s_arenaSpawnPosition;
s16 s_arenaSpawnYaw = 0;
bool s_arenaSpawn = false;
barriers::Point map_point(const cXyz& p) { return {p.x,p.y,p.z}; }
cXyz game_point(barriers::Point p) { return cXyz(p.x,p.y,p.z); }
void clear_arena_geometry() {
    retire_palace_ferries();
    clear_elite_sites();
    clear_reachable_floor();
    s_barriers.clear(); s_arenaBackgrounds.clear(); s_arenaMeshReady = false; s_arenaSafe = s_arenaSpawn = false;
}
void add_exit_box(const fopAcM_prm_class& params, float width, float depth) {
    const cXyz center = params.base.position;
    const csXyz angle = params.base.angle;
    const float sine=cM_ssin(angle.y), cosine=cM_scos(angle.y);
    std::array<barriers::Point,4> points;
    constexpr float signs[4][2]{{-1,-1},{1,-1},{1,1},{-1,1}};
    for (unsigned i=0;i<4;++i) {
        const float x=signs[i][0]*width,z=signs[i][1]*depth;
        points[i]={center.x+x*cosine+z*sine,center.y,center.z+z*cosine-x*sine};
    }
    for (unsigned i=0;i<4;++i) s_barriers.push_back({points[i],points[(i+1)%4]});
}
bool arena_story_actor(int name) {
    const auto* base=fpcPf_Get(static_cast<s16>(name));
    const auto* profile=reinterpret_cast<const actor_process_profile_definition*>(base);
    if (profile && (profile->group==fopAc_ENEMY_e || profile->group==fopAc_NPC_e)) return true;
    switch (name) {
    case fpcNm_TBOX_e: case fpcNm_TBOX2_e: case fpcNm_Obj_Carry_e:
    case fpcNm_HORSE_e: case fpcNm_TAG_EVENT_e: case fpcNm_TAG_EVT_e:
    case fpcNm_TAG_MSG_e: case fpcNm_TAG_EVTAREA_e: case fpcNm_TAG_EVTMSG_e:
    case fpcNm_Tag_TWGate_e: case fpcNm_Tag_Lv8Gate_e: case fpcNm_Tag_Mist_e:
    case fpcNm_Tag_Restart_e: case fpcNm_Tag_ChgRestart_e: case fpcNm_Tag_RetRoom_e:
        return true;
    default: return false;
    }
}
HookAction before_arena_stage_actor(ModContext*, void* args, void*, void*) {
    if (!in_arena()) return HOOK_CONTINUE;
    const auto* record=mods::arg<stage_actor_data_class*>(args,0);
    auto* params=mods::arg<fopAcM_prm_class*>(args,1);
    if (!record || !params) return HOOK_CONTINUE;
    // DZS names are fixed-width, not necessarily null-terminated.
    char name[9]{}; std::memcpy(name,record->name,8);
    const auto* info=dStage_searchName(name);
    if (!info) return HOOK_CONTINUE; // native creator owns/frees its append
    if (info->procname==fpcNm_ALINK_e) {
        // The original arena already has a working authored entrance. Keep it
        // intact. Mode 0 is NOT an idle/no-demo spawn: setStartProcInit starts a
        // 35-frame entrance demo for it. Use native idle mode 16 on the new maps.
        if (s_map!=0) params->base.parameters=
            (params->base.parameters & 0x00fe0fffu) | 0xff010000u;
        return HOOK_CONTINUE;
    }
    const bool local=params->room_no==arena().room || params->room_no==-1;
    if (capture_palace_ferry(info->procname,*params)) {
        // Recreate either native light-lift variant from the same route once
        // room paths are ready. Suppress the original to prevent duplicates.
        JKRFree(params);
        return HOOK_SKIP_ORIGINAL;
    }
    if (s_map==3) capture_elite_site(info->procname,*params);
    bool skip=arena_story_actor(info->procname);
    if (info->procname==fpcNm_SCENE_EXIT_e || info->procname==fpcNm_SCENE_EXIT2_e) {
        if (local) {
            // Native SCENE_EXIT scales its XZ half-extents by 75. Grotto exit
            // radius is scale.x*100; enclose that circle rather than cut it off.
            const float unit=info->procname==fpcNm_SCENE_EXIT_e ? 7.5f : 10.f;
            const float width=std::max(40.f,float(params->scale.x)*unit);
            const float depth=info->procname==fpcNm_SCENE_EXIT_e ?
                std::max(40.f,float(params->scale.z)*unit) : width;
            add_exit_box(*params,width,depth);
        }
        skip=true;
    } else if (local && (info->procname==fpcNm_DOOR20_e || info->procname==fpcNm_DBDOOR_e ||
                        info->procname==fpcNm_BOSS_DOOR_e)) {
        if(!s_hub)add_exit_box(*params,std::max(300.f,float(params->scale.x)*30.f),30.f);
    }
    if (!skip) return HOOK_CONTINUE;
    // The original creator normally owns this allocation. Skipping it must
    // release the append, including while streaming neighbouring field rooms.
    JKRFree(params);
    return HOOK_SKIP_ORIGINAL;
}
void collect_arena_mesh(fopAc_ac_c* actor) {
    if (fopAcM_GetName(actor)!=fpcNm_BG_e || fopAcM_GetRoomNo(actor)!=arena().room) return;
    const auto id=fopAcM_GetID(actor);
    if (std::find(s_arenaBackgrounds.begin(),s_arenaBackgrounds.end(),id)!=s_arenaBackgrounds.end()) return;
    auto* bg=static_cast<daBg_c*>(actor);
    dBgW_Base* mesh=bg->mpBgW ? static_cast<dBgW_Base*>(bg->mpBgW) : bg->mpKCol;
    if (!mesh || mesh->ChkNotReady()) return;
    int begin=0, count=0;
    if (bg->mpBgW) count=bg->mpBgW->GetBgd()->m_t_num;
    else {
        // KCL has a dummy first prism. DZB and KCL expose identical polygon
        // queries; keep both native collision formats supported.
        auto* header=bg->mpKCol->m_pkc_head;
        const auto* first=static_cast<KC_PrismData*>(header->m_prism_data);
        const auto* last=reinterpret_cast<const KC_PrismData*>(static_cast<BE(u32)*>(header->m_block_data));
        count=static_cast<int>(last-first); begin=1;
    }
    if (count<=begin || count>65535) return;
    barriers::Boundary boundary;
    cBgS_PolyInfo polygon;
    for (int i=begin;i<count;++i) {
        polygon.SetPolyIndex(i);
        cXyz a,b,c;
        if (!mesh->GetTriPnt(polygon,&a,&b,&c)) continue;
        const auto normal=mesh->GetTriPla(polygon);
        // A mesh edge or another room group is not necessarily an exit. Only
        // native scene-exit polygons justify an additional curtain here.
        const bool exit=mesh->GetExitId(polygon)!=0x3f;
        capture_elite_floor(a,b,c,normal.GetNP()->y,exit);
        boundary.triangle({map_point(a),map_point(b),map_point(c)},normal.GetNP()->y>0.25f,exit);
    }
    auto walls=boundary.walls();
    s_barriers.insert(s_barriers.end(),walls.begin(),walls.end());
    s_arenaBackgrounds.push_back(id); s_arenaMeshReady=true;
    clear_reachable_floor(); // Newly streamed collision can open previously unknown routes.
    char message[160];
    std::snprintf(message,sizeof(message),"Hyrule Survivors: arena %s room %d, %u exit barrier segments",
        arena().stage,arena().room,unsigned(s_barriers.size()));
    svc_log->info(mod_ctx,message);
}
bool arena_path_clear(const cXyz& from, const cXyz& to, float radius=35) {
    const auto target=barriers::slide(map_point(from),map_point(to),radius,s_barriers);
    return std::abs(target.x-to.x)<1 && std::abs(target.z-to.z)<1;
}
void confine_arena_actor(fopAc_ac_c* actor, float radius) {
    actor->current.pos=game_point(barriers::slide(map_point(actor->old.pos),map_point(actor->current.pos),radius,s_barriers));
}
void after_arena_position(ModContext*, void* args, void*, void*) {
    // Authored entrances can begin behind a door/exit curtain. Let the native
    // entry and ground correction finish before constraining movement.
    if (!in_arena() || transitioning() || s_hub || s_run.phase==Phase::Loading || !s_arenaSafe) return;
    auto* link=mods::arg<daAlink_c*>(args,0);
    if (!link || fopAcM_GetID(link)!=s_player) return;
    confine_arena_actor(link,40);
}
// Reject exits before the native code puts Link into an unfinishable scene-change
// action. setNextStage is additionally guarded against scripted transitions.
HookAction before_arena_exit(ModContext*, void*, void* result, void*) {
    if (!in_arena() || transitioning() || (s_hub&&s_hubDoorStarted)) return HOOK_CONTINUE;
    *static_cast<int*>(result)=0;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_arena_restart(ModContext*, void* args, void* result, void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if (!in_arena() || transitioning() || !s_arenaSafe || !s_arenaSpawn ||
        s_run.phase!=Phase::Running || !link || fopAcM_GetID(link)!=s_player) return HOOK_CONTINUE;
    // Native void returns set an event/damage before setNextStage is blocked.
    // Our local rescue owns the penalty and never reloads the active run.
    *static_cast<BOOL*>(result)=FALSE;
    return HOOK_SKIP_ORIGINAL;
}
bool arena_grounded(daAlink_c* link) {
    const float floor=link->mLinkAcch.GetGroundH();
    return link->mGroundCode!=4 && link->mGroundCode!=5 && link->mGroundCode!=10 &&
        link->mLinkAcch.ChkGroundHit() && std::isfinite(floor) && floor>-1.0e8f &&
        std::abs(link->current.pos.y-floor)<20 && fopAcM_GetRoomNo(link)==arena().room;
}
bool dry_arena_floor(cXyz& pos, float rise, float drop);
bool arena_fallen(daAlink_c* link) {
    if (!s_arenaSafe || link->current.pos.y>=s_arenaSafePosition.y-500) return false;
    // Height lost alone cannot distinguish a cliff from a void. Probe beneath
    // Link's current XZ, including the landing frame, before rescuing him.
    // Use the native valid-ground range, not another maximum jump height.
    cXyz landing=link->current.pos;
    return !dry_arena_floor(landing,20,1.e8f);
}
// Native ground contact can persist at an edge with most of Link over the void.
// Only promote positions with a supported footprint to recovery checkpoints.
bool arena_safe_footprint(const cXyz& position) {
    cXyz center=position;
    if (!dry_arena_floor(center,20,40)) return false;
    for (const auto& offset : std::array<cXyz,4>{cXyz(-40,0,-40),cXyz(40,0,-40),
            cXyz(-40,0,40),cXyz(40,0,40)}) {
        cXyz edge=center+offset;
        if (!dry_arena_floor(edge,40,40)) return false;
    }
    return true;
}
void restore_arena_player(daAlink_c* link, const cXyz& position, s16 yaw) {
    s_warmup=0;
    clear_attacks();interrupt_auto_actions(link);cancel_jump_abilities_for(link);
    // The native setter also resets the movement anchor and collision push.
    // Assigning current/old alone leaves these pointing into the hole.
    link->setPlayerPosAndAngle(&position,yaw,TRUE);
    link->speed.set(0,0,0);link->speedF=link->mNormalSpeed=0;
    link->mLastJumpPos=position;
    link->mFallHeight=link->field_0x33c8=position.y;
    fopAcM_SetRoomNo(link,arena().room);
    link->procWaitInit();
    link->mLinkAcch.CrrPos(dComIfG_Bgsp());
    link->mGroundCode=dComIfG_Bgsp().GetGroundCode(link->mLinkAcch.m_gnd);
}
void update_arena_safety(daAlink_c* link) {
    // Native ground contact is available before our derived barrier mesh. Keep
    // the first safe position during Loading as well, not just after warmup.
    if (s_deathPending || fpcM_IsCreating(fopAcM_GetID(link)) || (s_hub&&s_hubDoorStarted)) return;
    const int room=fopAcM_GetRoomNo(link);
    const bool palace=s_map==3 && s_arenaSpawn && s_run.phase==Phase::Running;
    const bool fallen=s_arenaSafe && (arena_fallen(link) ||
        (link->mLinkAcch.ChkGroundHit() &&
         (link->mGroundCode==4 || link->mGroundCode==5 || link->mGroundCode==10)));
    if (palace && fallen) {
        restore_arena_player(link,s_arenaSpawnPosition,s_arenaSpawnYaw);
        s_arenaSafePosition=s_arenaSpawnPosition;
    } else if (s_arenaSafe && (room!=arena().room || fallen)) {
        // Last safe floor position, not a scene reload that would discard a run.
        restore_arena_player(link,s_arenaSafePosition,link->shape_angle.y);
    } else if (arena_grounded(link) && arena_safe_footprint(link->current.pos)) {
        s_arenaSafePosition=link->current.pos; s_arenaSafe=true;
        if (!s_arenaSpawn) {
            s_arenaSpawnPosition=link->current.pos;s_arenaSpawnYaw=link->shape_angle.y;s_arenaSpawn=true;
        }
    }
    // Charge once per actual fall recovery during a run, on every map.
    // Loading and room-boundary corrections are not damaging falls.
    if (fallen && s_run.phase==Phase::Running) {
        // Life is stored in quarter hearts; this environmental cost ignores
        // enemy damage scaling, shields, Dodge and ordinary hit invulnerability.
        const int life=std::max(s_victory ? 1 : 0,int(dComIfGs_getLife())-4);
        dComIfGs_setLife(life);
        if (!life) begin_defeat();
    }
}
void draw_arena_barriers() {
    if(s_hub)return;
    const auto* link=daAlink_getAlinkActorClass();
    if (!link) return;
    const auto& color=arena().barrier_color;
    const GXColor glow{color[0],color[1],color[2],185};
    const GXColor veil{color[0],color[1],color[2],55};
    // Near segments only, leaving ample space in the shared native packet queue
    // for XP, projectiles and other mods. Collision is never distance-culled.
    unsigned drawn=0;
    for (const auto& wall : s_barriers) {
        if (barriers::distance_squared(map_point(link->current.pos),wall)>3500.f*3500.f) continue;
        if (++drawn>160) break;
        cXyz a=game_point(wall.a),b=game_point(wall.b);
        a.y-=40; b.y-=40;
        cXyz quad[4]{a,b,b+cXyz(0,1500,0),a+cXyz(0,1500,0)};
        dDbVw_drawQuadXlu(quad,veil,1);
        dDbVw_drawLineXlu(quad[0],quad[1],glow,1,16);
        const float pulse=100.f+float(s_run.ticks%120)*10.f;
        a.y+=pulse; b.y+=pulse;
        dDbVw_drawLineXlu(a,b,glow,1,8);
    }
}
