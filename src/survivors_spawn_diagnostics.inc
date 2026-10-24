// Temporary Darknut spawn diagnosis. Development builds only; one bounded
// capture per floor-scan generation. No additional collision/path queries.
struct SpawnDiagnostics {
    unsigned generation=~0u,frames=0,triangle=0,accepted=0,rejected=0,sampleLogs=0;
    bool meshDone=false;
    float sampleLow=1.e30f,sampleHigh=-1.e30f,acceptedLow=1.e30f,acceptedHigh=-1.e30f;
} s_spawnDiagnostics;
bool spawn_diagnostics_active() {
    if(!debug_enabled || s_map!=0)return false;
    if(s_spawnDiagnostics.generation!=s_reachableFloor.generation()) {
        s_spawnDiagnostics={};s_spawnDiagnostics.generation=s_reachableFloor.generation();
    }
    return s_spawnDiagnostics.frames<1800; // First 60 seconds of active gameplay.
}
void record_arena_spawn_sample(cXyz sample,cXyz projected,bool accepted) {
    if(!spawn_diagnostics_active())return;
    auto& d=s_spawnDiagnostics;
    d.sampleLow=std::min(d.sampleLow,sample.y);d.sampleHigh=std::max(d.sampleHigh,sample.y);
    if(accepted) {
        ++d.accepted;d.acceptedLow=std::min(d.acceptedLow,projected.y);d.acceptedHigh=std::max(d.acceptedHigh,projected.y);
    } else ++d.rejected;
    if(d.sampleLogs>=16)return;
    ++d.sampleLogs;char message[256];
    std::snprintf(message,sizeof(message),"Survivors floor diag v1: generation=%u sample=(%.2f,%.2f,%.2f) projected=(%.2f,%.2f,%.2f) footprint=%s",
        d.generation,sample.x,sample.y,sample.z,projected.x,projected.y,projected.z,accepted?"accepted":"rejected");
    svc_log->info(mod_ctx,message);
}
void update_spawn_diagnostics(daAlink_c* link) {
    if(!link || !spawn_diagnostics_active())return;
    auto& d=s_spawnDiagnostics;char message[640];
    if(d.frames%150==0) {
        unsigned chests=0,pots=0;float chestLow=1.e30f,chestHigh=-1.e30f,potLow=1.e30f,potHigh=-1.e30f;
        for(const auto& c:s_chests)if(c.active){++chests;chestLow=std::min(chestLow,c.position.y);chestHigh=std::max(chestHigh,c.position.y);}
        for(const auto& p:s_pots)if(p.id!=kNone&&!p.retiring){++pots;potLow=std::min(potLow,p.position.y);potHigh=std::max(potHigh,p.position.y);}
        std::snprintf(message,sizeof(message),
            "Survivors floor diag v1: generation=%u frame=%u seed=(%.2f,%.2f,%.2f) link=(%.2f,%.2f,%.2f) native-ground=%.2f grounded=%u ready=%u reached=%u/%u area=%.0f samples=%u y=[%.2f,%.2f] accepted=%u y=[%.2f,%.2f] rejected=%u chests=%u y=[%.2f,%.2f] pots=%u y=[%.2f,%.2f]",
            d.generation,d.frames,s_arenaSpawnPosition.x,s_arenaSpawnPosition.y,s_arenaSpawnPosition.z,
            link->current.pos.x,link->current.pos.y,link->current.pos.z,link->mLinkAcch.GetGroundH(),unsigned(link->mLinkAcch.ChkGroundHit()),
            unsigned(s_reachableFloor.ready()),s_reachableFloor.reached_count(),unsigned(s_eliteFloors.size()),s_reachableFloor.floor_area(),
            d.accepted+d.rejected,d.accepted+d.rejected?d.sampleLow:0,d.accepted+d.rejected?d.sampleHigh:0,
            d.accepted,d.accepted?d.acceptedLow:0,d.accepted?d.acceptedHigh:0,d.rejected,
            chests,chests?chestLow:0,chests?chestHigh:0,pots,pots?potLow:0,pots?potHigh:0);
        svc_log->info(mod_ctx,message);
    }
    if(s_reachableFloor.ready()&&!d.meshDone) {
        const unsigned end=std::min(4096u,unsigned(s_eliteFloors.size()));
        for(unsigned budget=8;budget&&d.triangle<end;--budget,++d.triangle) {
            const auto& f=s_eliteFloors[d.triangle];
            std::snprintf(message,sizeof(message),"Survivors floor diag v1: generation=%u triangle=%u reached=%u a=(%.3f,%.3f,%.3f) b=(%.3f,%.3f,%.3f) c=(%.3f,%.3f,%.3f)",
                d.generation,d.triangle,unsigned(s_reachableFloor.reached(d.triangle)),f.a.x,f.a.y,f.a.z,f.b.x,f.b.y,f.b.z,f.c.x,f.c.y,f.c.z);
            svc_log->info(mod_ctx,message);
        }
        if(d.triangle==end) {
            d.meshDone=true;
            std::snprintf(message,sizeof(message),"Survivors floor diag v1: generation=%u mesh-complete dumped=%u total=%u truncated=%u",
                d.generation,end,unsigned(s_eliteFloors.size()),unsigned(end<s_eliteFloors.size()));
            svc_log->info(mod_ctx,message);
        }
    }
    ++d.frames;
}
