// The native arrow Draw only lights/submits its model. Execute, collision,
// particles and sound remain native. Store copied positions in the packet, never
// actor/model pointers that may outlive an asynchronous process deletion.
PerformanceLines s_enemyArrowLines;
std::array<ActorId,enemy_capacity*4> s_reducedEnemyArrowIds{};
std::array<bool,enemy_capacity*4> s_enemyArrowSubmitted{};
bool s_enemyArrowFrameReady=false;

bool enemy_arrow_visuals_active() {
    return survivor_perf::presentation.native.available && survivor_perf::enabled() &&
        in_arena() && !s_hub && !transitioning();
}
void clear_enemy_arrow_visuals() {
    clear_line_packet(s_enemyArrowLines);
    s_reducedEnemyArrowIds.fill(kNone);
    s_enemyArrowSubmitted.fill(false);
    s_enemyArrowFrameReady=false;
}
HookAction before_enemy_arrow_frame(ModContext*,void*,void*,void*) {
    clear_enemy_arrow_visuals();
    auto& stats=survivor_perf::presentation.native;
    const bool available=stats.available;stats={};stats.available=available;
    bool collect=survivor_perf::enabled();
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
    collect=collect||freeze_diag::enabled;
#endif
    if(!s_active||!collect)return HOOK_CONTINUE;
    for(const auto& e:s_enemies) {
        if(e.id==kNone||e.retiring||e.credited)continue;
        if(e.elite)++stats.eliteEnemies;else ++stats.normalEnemies;
    }
    auto* player=dComIfGp_getPlayer(0);
    const bool simplify=enemy_arrow_visuals_active() && player;
    // Flying threats precede stuck arrows, then nearest first, stable by slot.
    struct Candidate {bool stuck;float distance;size_t slot;};
    std::array<Candidate,enemy_capacity*4> candidates{};
    size_t count=0;
    for(size_t i=0;i<s_enemyArrows.size();++i) {
        const auto& child=s_enemyArrows[i];
        if(child.id==kNone)continue;
        if(child.retiring){++stats.retiring;continue;}
        auto* actor=actor_by_id(child.id);
        if(fpcM_IsCreating(child.id)){++stats.pending;continue;}
        if(!actor)continue;
        if(child.shell)++stats.attached;else ++stats.projectiles;
        if(child.profile!=fpcNm_E_ARROW_e || fopAcM_GetName(actor)!=fpcNm_E_ARROW_e)continue;
        ++stats.arrows;
        auto* arrow=static_cast<e_arrow_class*>(actor);
        stats.normalArrows+=arrow->mArrowType==ARROW_TYPE_NORMAL;
        stats.fireArrows+=arrow->mArrowType==ARROW_TYPE_FIRE;
        stats.bombArrows+=arrow->mArrowType==ARROW_TYPE_BOMB;
        stats.eliteArrows+=child.eliteOwner;
        if(!simplify || child.eliteOwner || arrow->mArrowType!=ARROW_TYPE_NORMAL ||
            arrow->field_0xa14 || !arrow->mpModel)continue;
        candidates[count++]={arrow->mAction!=ACTION_ARROW_SHOT,
            (arrow->current.pos-player->current.pos).abs2(),i};
    }
    if(!simplify)return HOOK_CONTINUE;
    stats.limit=survivor_perf::native_arrow_detail_limit(survivor_perf::tier());
    std::sort(candidates.begin(),candidates.begin()+count,[](const Candidate& a,const Candidate& b) {
        if(a.stuck!=b.stuck)return a.stuck<b.stuck;
        if(a.distance!=b.distance)return a.distance<b.distance;
        return a.slot<b.slot;
    });
    for(size_t i=std::min(count,size_t(stats.limit));i<count;++i) {
        const auto slot=candidates[i].slot;
        s_reducedEnemyArrowIds[slot]=s_enemyArrows[slot].id;
    }
    s_enemyArrowFrameReady=true;
    return HOOK_CONTINUE;
}
HookAction before_enemy_arrow_draw(ModContext*,void* args,void* result,void*) {
    auto* arrow=mods::arg<e_arrow_class*>(args,0);
    if(!arrow || !s_enemyArrowFrameReady || !enemy_arrow_visuals_active() ||
        arrow->field_0xa14 || !arrow->mpModel)return HOOK_CONTINUE;
    auto* child=tracked_enemy_arrow(fopAcM_GetID(arrow));
    if(!child || child->retiring || child->shell)return HOOK_CONTINUE;
    const size_t slot=size_t(child-s_enemyArrows.data());
    auto& stats=survivor_perf::presentation.native;
    // Recheck type: a hit can change an arrow's state after frame selection.
    if(child->eliteOwner || arrow->mArrowType!=ARROW_TYPE_NORMAL ||
        s_reducedEnemyArrowIds[slot]!=child->id) {
        ++stats.fullDraws;return HOOK_CONTINUE;
    }
    if(!s_enemyArrowSubmitted[slot]) {
        const float yawSin=cM_ssin(arrow->shape_angle.y),yawCos=cM_scos(arrow->shape_angle.y);
        const float pitchSin=cM_ssin(arrow->shape_angle.x),pitchCos=cM_scos(arrow->shape_angle.x);
        const cXyz direction(yawSin*pitchCos,-pitchSin,yawCos*pitchCos),side(yawCos,0,-yawSin);
        const cXyz tip=arrow->current.pos+direction*40.f;
        const cXyz tail=arrow->current.pos-direction*20.f;
        const cXyz head=tip-direction*12.f;
        const GXColor color{255,220,130,255};
        const auto previous=s_enemyArrowLines.lines.size();
        s_enemyArrowLines.lines.push_back({tail,tip,color});
        s_enemyArrowLines.lines.push_back({head+side*5.f,tip,color});
        s_enemyArrowLines.lines.push_back({head-side*5.f,tip,color});
        if(!submit_line_packet(s_enemyArrowLines)) {
            s_enemyArrowLines.lines.resize(previous);
            ++stats.fullDraws;return HOOK_CONTINUE; // No list: never hide a threat.
        }
        s_enemyArrowSubmitted[slot]=true;
    }
    ++stats.reducedDraws;
    *static_cast<int*>(result)=1;
    return HOOK_SKIP_ORIGINAL;
}
void initialize_enemy_arrow_visuals() {
    const bool ready=mods::hook_add_pre<SurvivorsArrowFrame>(svc_hook,before_enemy_arrow_frame)==MOD_OK &&
        mods::hook_add_pre<SurvivorsArrowDraw>(svc_hook,before_enemy_arrow_draw)==MOD_OK;
    survivor_perf::presentation.native.available=ready;
    if(!ready) {
        mods::hook_uninstall<SurvivorsArrowFrame>(svc_hook);
        mods::hook_uninstall<SurvivorsArrowDraw>(svc_hook);
        svc_log->warn(mod_ctx,"Survivors native arrow detail budget unavailable; keeping native models");
    }
}
void shutdown_enemy_arrow_visuals() {
    mods::hook_uninstall<SurvivorsArrowFrame>(svc_hook);
    mods::hook_uninstall<SurvivorsArrowDraw>(svc_hook);
    clear_enemy_arrow_visuals();
    survivor_perf::presentation.native={};
}
