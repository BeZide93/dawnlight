// Z2AudioMgr::init installs sectionHeap->getSeqSeqDataMgr() on its sequence
// manager; that interface belongs to the host JAUSectionHeap. Follow this
// existing owner instead of importing unexported JASGlobalInstance storage.
JASWaveBank* trial_wave_bank() {
    auto* audio=Z2GetAudioMgr();
    if (!audio) return nullptr;
    auto* heap=static_cast<JAUSectionHeap*>(audio->mSoundMgr.getSeqMgr()->getSeqDataMgr());
    return heap ? heap->getWaveBankTable().getWaveBank(0) : nullptr;
}

// Each overflow archive gets a native heap backed by a reserved ARAM block.
// Keep these objects in the host system heap: pending DVD callbacks and scene
// adoption may outlive mod shutdown. Never leave a parent heap in mod storage.
struct TrialWaveStorage {
    JKRAramBlock* block=nullptr;
    JASHeap* heap=nullptr;
    JASWaveArc* wave=nullptr;

    void collect() {
        if (!heap || (wave && wave->_5a!=0) || heap->getFirstChild()!=heap->getEndChild()) return;
        heap->free();
        JKR_DELETE(heap); heap=nullptr;
        JKR_DELETE(block); block=nullptr; wave=nullptr;
    }
    bool load(JASWaveArc* arc) {
        collect();
        if (heap || !arc || arc->mEntryNum<0 || !arc->getFileSize() ||
            !JKRAram::getManager() || !JKRAram::getAramHeap()) return false;
        auto* root=JASKernel::getAramHeap();
        block=JKRAram::getAramHeap()->alloc(arc->getFileSize(),JKRAramHeap::TAIL);
        if (!block) return false;
        // Some host configurations reserve more audio ARAM than JKR's nominal
        // audio partition. Do not use any graph allocation overlapping it.
        const auto begin=static_cast<uintptr_t>(block->getAddress());
        const auto end=begin+block->getSize();
        const auto audioBegin=reinterpret_cast<uintptr_t>(root->getBase());
        if (begin<audioBegin+root->getSize() && end>audioBegin) {
            JKR_DELETE(block); block=nullptr; return false;
        }
        heap=JKR_NEW_ARGS(JKRGetSystemHeap(),0) JASHeap;
        if (!heap) { JKR_DELETE(block); block=nullptr; return false; }
        heap->initRootHeap(reinterpret_cast<void*>(begin),block->getSize());
        if (arc->load(heap)) { wave=arc; return true; }
        collect();
        return false;
    }
};

// The arena aliases the Darknut room (SE 0x15/0x17). Model archives do not
// load sound samples. Add the native scene sets for normal Temple of Time
// Beamos, Fyrus, Palace of Twilight's Phantom Zant, and Hyrule Castle's barrier.
// Shade's Realm (F_SP200) uses SE 0x4a for the actor's native voice cues.
struct ShadeTrialWaves {
    static constexpr std::array<u32,7> ids{0x4a,0x08,0x16,0x1d,0x1f,0x21,0x22};
    std::array<bool,ids.size()> owned{},warned{};
    std::array<TrialWaveStorage,ids.size()> storage{};
    std::array<int,ids.size()> idle{};
    shade::Trial selected=shade::Trial::None;

    static bool needed(u32 id,shade::Trial trial) {
        if (id==0x4a) return true; // voices survive every trial/ordinary combat
        switch (trial) {
        case shade::Trial::Shield: return id==0x1d || id==0x1f || id==0x21 || id==0x22;
        case shade::Trial::Fire: return id==0x08;
        case shade::Trial::Eyes: return id==0x16;
        default: return false; // Gale Boomerang samples are in the resident set.
        }
    }
    bool ready=false;
    int retry=0;

    bool owns(u32 id) const {
        for (unsigned i=0;i<ids.size();++i) if (ids[i]==id) return owned[i];
        return false;
    }
    bool prepare(shade::Trial trial) {
        auto* scene=static_cast<Z2SceneMgr*>(Z2GetAudioMgr());
        if (!scene || !scene->isSceneExist()) return false;
        if (selected!=trial) { selected=trial; ready=false; retry=0; warned.fill(false); }
        // Retire the previous phase after its one-shot tails have ended.
        for (unsigned i=0;i<ids.size();++i)
            idle[i]=needed(ids[i],trial) ? 0 : std::min(idle[i]+1,60);
        release(false);
        const bool request=retry==0;
        retry=request ? 60 : retry-1;
        bool loaded=true;
        for (unsigned i=0;i<ids.size();++i) {
            if (!needed(ids[i],trial)) continue;
            const int status=scene->getSeLoadStatus(ids[i]);
            if (status==2) continue; // 1 means queued, not playable yet.
            loaded=false;
            if (status==0 && request) {
                owned[i]=scene->loadSeWave(ids[i]);
                auto* bank=trial_wave_bank();
                auto* arc=bank ? bank->getWaveArc(ids[i]) : nullptr;
                auto* root=JASWaveArcLoader::getRootHeap();
                const auto free=root ? root->getFreeSize() : 0;
                // Only use overflow for a confirmed capacity failure. Missing
                // disc entries and DVD queue failures require separate diagnosis.
                if (!owned[i] && arc && arc->getStatus()==0 &&
                    arc->getFileSize()>free && storage[i].load(arc)) {
                    owned[i]=true;
                    char message[160];
                    std::snprintf(message,sizeof(message),
                        "Hero's Shade: SE 0x%02x queued in reserved ARAM (%u bytes; audio free %u)",
                        ids[i],arc->getFileSize(),free);
                    svc_log->info(mod_ctx,message);
                }
                if (!owned[i] && !warned[i]) {
                    warned[i]=true;
                    char message[224];
                    std::snprintf(message,sizeof(message),
                        "Hero's Shade: SE 0x%02x load failed (disc entry %d, bytes %u, audio free %u, status %d); retrying",
                        ids[i],arc ? arc->mEntryNum : -1,arc ? arc->getFileSize() : 0,
                        free,arc ? arc->getStatus() : -1);
                    svc_log->warn(mod_ctx,message);
                }
            }
        }
        if (loaded && !ready)
            svc_log->info(mod_ctx,"Hero's Shade: voice and current/next trial sound waves ready");
        ready=loaded;
        return ready;
    }
    void release(bool all=true) {
        auto* scene=static_cast<Z2SceneMgr*>(Z2GetAudioMgr());
        if (!scene) return;
        bool erased=false;
        for (unsigned i=0;i<ids.size();++i) {
            storage[i].collect();
            if (!owned[i] || (!all && idle[i]<60)) continue;
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
                storage[i].collect();
            }
        }
        if (erased) {
            // Native WSYS groups share sample IDs. Erasing one clears its
            // bindings without restoring still-loaded duplicates. Rebind those
            // through their native callback, without loading/erasing them.
            auto* bank=trial_wave_bank();
            if (bank) for (u32 i=0;i<bank->getArcCount();++i) {
                auto* arc=bank->getWaveArc(i);
                if (arc && arc->getStatus()==2) arc->onLoadDone();
            }
        }
        if (all) { ready=false;retry=0;warned.fill(false);selected=shade::Trial::None;idle.fill(0); }
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
        auto* bank=trial_wave_bank();
        if (auto* arc=bank ? bank->getWaveArc(id) : nullptr) arc->onLoadDone();
    }
    *static_cast<bool*>(result)=true;
    return HOOK_SKIP_ORIGINAL;
}
