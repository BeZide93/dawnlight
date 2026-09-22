// The arena aliases the Darknut room (SE 0x15/0x17). Model archives do not
// load sound samples. Add the native scene sets for normal Temple of Time
// Beamos, Palace of Twilight's Phantom Zant, and Hyrule Castle's barrier.
struct ShadeTrialWaves {
    static constexpr std::array<u32,5> ids{0x16,0x1d,0x1f,0x21,0x22};
    std::array<bool,ids.size()> owned{},warned{};
    bool ready=false;
    int retry=0;

    bool owns(u32 id) const {
        for (unsigned i=0;i<ids.size();++i) if (ids[i]==id) return owned[i];
        return false;
    }
    bool prepare() {
        auto* scene=Z2GetSceneMgr();
        if (!scene || !scene->isSceneExist()) return false;
        const bool request=retry==0;
        retry=request ? 60 : retry-1;
        bool loaded=true;
        for (unsigned i=0;i<ids.size();++i) {
            const int status=scene->getSeLoadStatus(ids[i]);
            if (status==2) continue; // 1 means queued, not playable yet.
            loaded=false;
            if (status==0 && request) {
                owned[i]=scene->loadSeWave(ids[i]);
                if (!owned[i] && !warned[i]) {
                    warned[i]=true;
                    char message[128];
                    std::snprintf(message,sizeof(message),
                        "Hero's Shade: cannot load trial SE wave 0x%02x; retrying",ids[i]);
                    svc_log->warn(mod_ctx,message);
                }
            }
        }
        if (loaded && !ready)
            svc_log->info(mod_ctx,"Hero's Shade: Beamos/barrier/burst sound waves ready");
        ready=loaded;
        return ready;
    }
    void release() {
        auto* scene=Z2GetSceneMgr();
        if (!scene) return;
        bool erased=false;
        for (unsigned i=0;i<ids.size();++i) {
            if (!owned[i]) continue;
            const auto id=ids[i];
            // The destination scene can adopt a preloaded archive. In that
            // case its normal audio teardown owns it from now on.
            if (id==scene->loadedSeWave_1 || id==scene->loadedSeWave_2 ||
                id==scene->loadedDemoWave) { owned[i]=false; continue; }
            if (id==scene->requestSeWave_1 || id==scene->requestSeWave_2 ||
                id==scene->requestDemoWave) continue;
            const int status=scene->getSeLoadStatus(id);
            if (status==1) continue; // Never reuse memory while DVD writes it.
            if (status==0 || scene->eraseSeWave(id)) {
                erased=erased || status==2;
                owned[i]=false;
            }
        }
        if (erased) {
            // Native WSYS groups share sample IDs. Erasing one clears its
            // bindings without restoring still-loaded duplicates. Rebind those
            // through their native callback, without loading/erasing them.
            auto* heap=JASGlobalInstance<JAUSectionHeap>::getInstance();
            auto* bank=heap ? heap->getWaveBankTable().getWaveBank(0) : nullptr;
            if (bank) for (u32 i=0;i<bank->getArcCount();++i) {
                auto* arc=bank->getWaveArc(i);
                if (arc && arc->getStatus()==2) arc->onLoadDone();
            }
        }
        ready=false;retry=0;warned.fill(false);
    }
} sTrialWaves;

// loadSeWave normally returns false for an already queued/loaded archive.
// Let the native scene manager adopt our preload and record its loaded slot.
DEFINE_HOOK(&Z2SceneMgr::loadSeWave, ShadeWaveLoadHook);
HookAction before_trial_wave_load(ModContext*,void* args,void* result,void*) {
    const auto id=mods::arg<u32>(args,1);
    auto* scene=mods::arg<Z2SceneMgr*>(args,0);
    if (!sTrialWaves.owns(id) || scene->getSeLoadStatus(id)==0) return HOOK_CONTINUE;
    if (scene->getSeLoadStatus(id)==2) {
        auto* heap=JASGlobalInstance<JAUSectionHeap>::getInstance();
        auto* bank=heap ? heap->getWaveBankTable().getWaveBank(0) : nullptr;
        if (auto* arc=bank ? bank->getWaveArc(id) : nullptr) arc->onLoadDone();
    }
    *static_cast<bool*>(result)=true;
    return HOOK_SKIP_ORIGINAL;
}
