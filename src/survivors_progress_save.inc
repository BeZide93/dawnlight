// Use the host's native background save state machine: it runs before ModLoader
// ticks even while a mod window is open, and flushes per-slot blobs on completion.
// Resolve lazily; a missing host symbol reports failure without closing the menu.
void (*s_enterProgressSave)()=nullptr;
u8* s_hostSavePhase=nullptr;
int* s_hostSaveResult=nullptr;
// MSVC decorates these data symbols; unlike functions, the Windows host
// manifest does not provide their undecorated display names.
#ifdef _WIN32
constexpr char kProgressSavePhaseSymbol[]="?mAutoSaveProc@@3EA";
constexpr char kProgressSaveResultSymbol[]="?autoSaveWriteState@@3HA";
#else
constexpr char kProgressSavePhaseSymbol[]="mAutoSaveProc";
constexpr char kProgressSaveResultSymbol[]="autoSaveWriteState";
#endif
void progress_save_failed(const std::string& reason) {
    s_storeFailed=true;
    svc_log->warn(mod_ctx,("Survivors progress save: "+reason).c_str());
}
bool resolve_progress_save_symbol(const char* name,void** address) {
    const auto status=svc_hook->resolve(mod_ctx,name,address,nullptr);
    if(status==MOD_OK && *address)return true;
    progress_save_failed(std::string("could not resolve ")+name+" (result "+std::to_string(status)+")");
    return false;
}
bool resolve_progress_save() {
    if(s_enterProgressSave && s_hostSavePhase && s_hostSaveResult)return true;
    void *enter=nullptr,*phase=nullptr,*result=nullptr;
    if(!resolve_progress_save_symbol("enterAutoSave",&enter) ||
       !resolve_progress_save_symbol(kProgressSavePhaseSymbol,&phase) ||
       !resolve_progress_save_symbol(kProgressSaveResultSymbol,&result))return false;
    s_enterProgressSave=reinterpret_cast<void(*)()>(enter);
    s_hostSavePhase=static_cast<u8*>(phase);s_hostSaveResult=static_cast<int*>(result);
    return true;
}
void save_progress(ModContext*,void*) {
    if(s_run.phase!=Phase::Ready || s_restart || s_progressSaving)return;
    s_progressSaved=false;
    if(!resolve_progress_save())return;
    if(*s_hostSavePhase!=0) {progress_save_failed("host save already in progress");return;}
    const auto cardStatus=mDoMemCd_getStatus(0);
    if(cardStatus!=2) {progress_save_failed("memory card is not ready (status "+std::to_string(cardStatus)+")");return;}
    const auto bytes=s_records.encode();
    s_storeFailed=svc_save->set_blob(mod_ctx,kBlob,bytes.data(),bytes.size())!=MOD_OK;
    if(s_storeFailed) {progress_save_failed("could not stage the records blob");return;}
    dComIfGs_getSaveData()->getPlayer().getPlayerReturnPlace().set(arenas[0].stage,arenas[0].room,arenas[0].point);
    *s_hostSaveResult=0;
    s_enterProgressSave();
    s_progressSaving=*s_hostSavePhase!=0;
    s_storeFailed=!s_progressSaving;
    if(s_storeFailed)progress_save_failed("host did not start the save");
}
void update_progress_save() {
    if(!s_progressSaving || *s_hostSavePhase!=0)return;
    s_progressSaving=false;
    s_storeFailed=*s_hostSaveResult!=1;
    s_progressSaved=!s_storeFailed;
    if(s_storeFailed)progress_save_failed("native write failed (result "+std::to_string(*s_hostSaveResult)+")");
}

// All automatic hub saves share the Survivors toggle. Finish writes already
// started, but discard disabled requests so they cannot block hub departure or
// run later when autosave is re-enabled. Explicit Save progress bypasses this gate.
bool update_hub_save() {
    if(s_progressSaving) {
        update_progress_save();
        if(!s_progressSaving && s_storeFailed)s_hubDeparture=false;
        return true;
    }
    if(!s_hubSaveRequested)return false;
    if(!survivors_autosave_enabled()) {s_hubSaveRequested=false;return false;}
    if(!in_arena() || transitioning() || dComIfGp_event_runCheck())return false;
    if(resolve_progress_save() && *s_hostSavePhase!=0)return true;
    s_hubSaveRequested=false;save_progress(nullptr,nullptr);
    if(s_storeFailed)s_hubDeparture=false;
    return s_progressSaving;
}
