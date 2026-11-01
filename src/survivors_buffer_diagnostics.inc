#if DAWNLIGHT_FREEZE_DIAGNOSTICS
// Game-thread reporting only. The render worker publishes POD samples and never logs.
struct DiagnosticContext {
    uint64_t request=0;
    BufferScene scene{};
    FogInventory fog{};
    bool performanceMode=false;
    unsigned pressureTier=0;
    uint64_t capturedUs=0;
    PresentationStats presentation{};
    unsigned pressureTriggerMask=0;
};
std::array<DiagnosticContext,64> diagnosticContexts{};
BufferScene (*sceneProvider)()=nullptr;
FogInventory (*fogProvider)()=nullptr;
BufferScene currentScene{},reportedScene{};
FogInventory currentFog{},reportedFog{};
BufferAlerts alerts{};
Sample warningSample{};
DiagnosticContext warningContext{};
uint64_t sceneCapturedUs=0;
uint64_t diagnosticStart=0,lastWarning=0,lastHeartbeat=0,lastContext=0,lastFogPoll=0,lastItemEvent=0,reportedDrops=0;
unsigned warningPressure=0;
bool warningPending=false,contextReported=false,reportedMode=false,lossPending=false;
const char* measurementStatus="not_initialized";
std::array<uint32_t,4> usage(const Sample& s){return {s.vertices,s.indices,s.uniforms,s.storage};}
std::array<uint64_t,4> capacities(const Sample& s){return {s.vertexCapacity,s.indexCapacity,s.uniformCapacity,s.storageCapacity};}
unsigned utilization(const Sample& s) {
    const auto u=usage(s);const auto c=capacities(s);uint64_t n=0;
    for(unsigned i=0;i<4;++i)if(c[i])n=std::max(n,uint64_t(u[i])*100/c[i]);
    return unsigned(n);
}
void reset_diagnostic_window() {
    sceneCapturedUs=0;diagnosticContexts={};alerts={};warningSample={};warningContext={};warningPending=false;warningPressure=0;
    lastWarning=lastHeartbeat=lastContext=lastFogPoll=lastItemEvent=reportedDrops=0;contextReported=false;lossPending=false;
}
DiagnosticContext sample_context(const Sample& s) {
    const auto& c=diagnosticContexts[s.request%diagnosticContexts.size()];
    return c.request==s.request&&c.capturedUs?c:DiagnosticContext{};
}
void diagnostic_observe(const Sample& s) {
    if(s.dropped!=reportedDrops){lossPending=true;reportedDrops=s.dropped;}
    alerts.observe(usage(s),capacities(s),s.requestedUs);
    if(alerts.pending()) {
        const unsigned pressure=utilization(s);
        if(!warningPending||pressure>=warningPressure) {
            warningSample=s;warningContext=sample_context(s);warningPressure=pressure;
        }
        warningPending=true;
    }
}
void emit_buffer_record(const char* reason,const Sample& s,const DiagnosticContext& c,uint64_t now,bool contextOnly=false) {
    // Keep native actor counts in a separate short record: the host/platform can
    // truncate the long effect record. request links both to the same snapshot.
    const auto& native=c.presentation.native;
    char nativeLine[768];
    std::snprintf(nativeLine,sizeof(nativeLine),
        "[SurvivorsNative] schema=1 reason=%s request=%llu context_request=%llu context_missing=%u map=%u tick=%u normal_enemies=%u elite_enemies=%u native_projectiles=%u native_arrows=%u arrow_normal=%u arrow_fire=%u arrow_bomb=%u arrow_elite=%u native_attached=%u native_pending=%u native_retiring=%u arrow_full_draws=%u arrow_reduced_draws=%u arrow_detail_limit=%u arrow_budget_available=%u",
        reason,(unsigned long long)s.request,(unsigned long long)c.request,unsigned(!contextOnly&&!c.request),c.scene.map,c.scene.tick,
        native.normalEnemies,native.eliteEnemies,native.projectiles,native.arrows,native.normalArrows,native.fireArrows,native.bombArrows,native.eliteArrows,native.attached,native.pending,native.retiring,
        native.fullDraws,native.reducedDraws,native.limit,unsigned(native.available));
    svc_log->info(mod_ctx,nativeLine);
    char line[4096];
    const auto& x=c.scene;
    const int wrote=std::snprintf(line,sizeof(line),
        "[SurvivorsBuffer] schema=1 reason=%s t_ms=%llu epoch=%llu request=%llu consumed_sequence=%llu sample_sequence=%llu stats_frame=unknown context_request=%llu context_missing=%u request_age_ms=%llu sample_age_ms=%llu measured=%u telemetry=%s samples_dropped=%llu mode=%u tier=%u map=%u phase=%u tick=%u hub=%u transition=%u paused=%u event=%u menu=%u item_event=%u reward=%u enemies=%u projectiles=%u reserved_emitters=%u fog_inventory=%d fog_matches=%u fog_id=%s fog_version=%s fog_render_setting=unknown",
        reason,(unsigned long long)((now-diagnosticStart)/1000),(unsigned long long)epoch,
        (unsigned long long)s.request,(unsigned long long)latest.sequence,(unsigned long long)s.sequence,
        (unsigned long long)c.request,unsigned(!contextOnly&&!c.request),
        (unsigned long long)(s.requestedUs&&now>=s.requestedUs?(now-s.requestedUs)/1000:0),
        (unsigned long long)(s.readUs&&now>=s.readUs?(now-s.readUs)/1000:0),unsigned(s.sequence!=0),measurementStatus,
        (unsigned long long)latest.dropped,unsigned(c.performanceMode),c.pressureTier,
        x.map,x.phase,x.tick,unsigned(x.hub),unsigned(x.transition),unsigned(x.paused),unsigned(x.event),unsigned(x.menu),
        x.itemEvent,x.reward,x.enemies,x.projectiles,x.reserved,c.fog.state,c.fog.matches,
        c.fog.id[0]?c.fog.id.data():"none",c.fog.version[0]?c.fog.version.data():"none");
    size_t used=wrote>0?std::min(size_t(wrote),sizeof(line)-1):0;
    const auto append=[&](const char* fmt,auto... values){
        const int n=std::snprintf(line+used,sizeof(line)-used,fmt,values...);
        if(n>0)used+=std::min(size_t(n),sizeof(line)-used-1);
    };
    const auto& p=c.presentation;
    append(" tier_trigger_mask=%u buffer_max_mask=%u hud_bars=%u hud_hp_numbers=%u hud_damage_numbers=%u hud_text_passes=%u te_hud_mask=%u shadow_budget_available=%u shadow_allowed_total=%llu shadow_suppressed_total=%llu",
        c.pressureTriggerMask,dominant_arena_mask(s),p.bars,p.numbers,p.popups,p.textPasses,p.teMask,
        unsigned(p.shadowAvailable),(unsigned long long)p.shadowAllowed,(unsigned long long)p.shadowSuppressed);
    constexpr const char* arenas[]{"vertex","index","uniform","storage"};
    const auto u=usage(s);const auto cap=capacities(s);
    for(unsigned i=0;i<4;++i)append(" %s_bytes=%u %s_capacity=%llu %s_peak=%u %s_crossed_pct=%u",arenas[i],u[i],arenas[i],
        (unsigned long long)cap[i],arenas[i],latest.peaks[i],arenas[i],alerts.crossed[i]==3?95:alerts.crossed[i]==2?90:alerts.crossed[i]==1?80:0);
    append(" rapid_mask=%u",alerts.rapidMask);
    constexpr const char* names[]{"burn","wave","lightning","explosion","boomerang","fire_arrow","fire_edge","jump","chain"};
    for(unsigned i=0;i<x.effects.size();++i) {
        const auto& e=x.effects[i];append(" %s_active=%u %s_full=%u %s_reduced=%u %s_emitters=%u",names[i],e.active,names[i],e.full,names[i],e.reduced,names[i],e.emitters);
    }
    append(" request_lag=%llu context_age_ms=%llu",(unsigned long long)(s.request?requestSequence-s.request:0),
        (unsigned long long)(c.capturedUs&&now>=c.capturedUs?(now-c.capturedUs)/1000:0));
    append(" requests_failed=%llu",(unsigned long long)requestFailures);
    if(contextOnly)append("%s"," context_only=1");
    svc_log->info(mod_ctx,line);
}
void diagnostic_capture(uint64_t now) {
    currentScene=sceneProvider?sceneProvider():BufferScene{};sceneCapturedUs=sceneProvider?now:0;
    if(!lastFogPoll||now-lastFogPoll>=1000000) {lastFogPoll=now;currentFog=fogProvider?fogProvider():FogInventory{};}
}
void diagnostic_report(uint64_t now) {
    if((warningPending||lossPending)&&(!lastWarning||now-lastWarning>=1000000)) {
        if(warningPending)emit_buffer_record(alerts.rapidMask?"buffer_growth":"buffer_threshold",warningSample,warningContext,now);
        else emit_buffer_record("sample_loss",latest,sample_context(latest),now);
        lastWarning=lastHeartbeat=now;lossPending=false;warningPending=false;warningPressure=0;alerts.acknowledged();
    }
    // Context-only records precede rendering and do not mislabel the previous frame's statistics.
    const bool changed=!contextReported||mode!=reportedMode||currentScene.map!=reportedScene.map||
        currentScene.hub!=reportedScene.hub||currentScene.itemEvent!=reportedScene.itemEvent||
        currentScene.event!=reportedScene.event||currentFog.state!=reportedFog.state||
        currentFog.id!=reportedFog.id||currentFog.version!=reportedFog.version;
    const bool itemStarted=currentScene.itemEvent&&currentScene.itemEvent!=reportedScene.itemEvent;
    if(changed&&(!lastContext||now-lastContext>=1000000||(itemStarted&&(!lastItemEvent||now-lastItemEvent>=1000000)))) {
        emit_buffer_record("scene",{},DiagnosticContext{0,currentScene,currentFog,mode,pressure.tier,sceneCapturedUs,presentation,pressureTriggerMask},now,true);
        if(itemStarted)lastItemEvent=now;
        lastContext=now;reportedScene=currentScene;reportedFog=currentFog;reportedMode=mode;contextReported=true;
    }
    if(!lastHeartbeat||now-lastHeartbeat>=30000000) {
        emit_buffer_record(latest.sequence?"heartbeat":"measurement_unavailable",latest,sample_context(latest),now);
        lastHeartbeat=now;
    }
}
#endif
