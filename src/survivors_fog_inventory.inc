#if DAWNLIGHT_FREEZE_DIAGNOSTICS
survivor_perf::FogInventory survivors_fog_inventory() {
    survivor_perf::FogInventory result;
    if(!s_loader)return result;
    result.state=0;
    for(const auto& mod:s_loader().active_mods()) {
        const auto& m=mod.metadata;
        if(!survivor_perf::fog_name_matches(m.id)&&!survivor_perf::fog_name_matches(m.name))continue;
        result.state=1;++result.matches;
        if(result.matches==1) {
            survivor_perf::log_token(result.id,m.id);
            survivor_perf::log_token(result.version,m.version);
        }
    }
    return result;
}
#endif
