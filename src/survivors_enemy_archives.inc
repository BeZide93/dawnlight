// Retain only archives actually loaded by run-owned standalone actors. This
// covers every map, boss variants and descendants without a second roster of
// archive names. Native actors keep their own references and delete normally.
// Both resLoad overloads reach this resource-processing function. The two-arg
// resLoad entry fails funchook relocation on Windows v2.0.3.
DEFINE_HOOK(static_cast<int (dRes_info_c::*)()>(&dRes_info_c::setRes), SurvivorsArchiveReady);
struct EnemyArchiveHold {
    std::array<char,11> name{};
    JKRArchive* archive=nullptr;
};
// Only one map's encountered resources are retained. If full, native loading
// continues unchanged; no allocation, spawn failure or cross-map accumulation.
std::array<EnemyArchiveHold,32> s_enemyArchives{};
bool s_enemyArchivesEnabled=false;
bool s_enemyArchivesAvailable=false;
unsigned s_enemyArchiveMap=0;

void release_enemy_archives() {
    s_enemyArchivesEnabled=false;
    for(auto& hold:s_enemyArchives) {
        if(!hold.archive)continue;
        auto* info=dComIfG_getObjectResInfo(hold.name.data());
        // Reset/shutdown may already have removed a host resource. Never drop a
        // reference on a different archive that reused the same resource slot.
        const bool owned=info && info->getArchive()==hold.archive;
        const auto name=hold.name;
        hold={};
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
        freeze_diag::cache(freeze_diag::CacheEvent::Release);
#endif
        if(owned)dComIfG_deleteObjectResMain(name.data());
    }
}
void begin_enemy_archives() {
    release_enemy_archives();
    s_enemyArchiveMap=s_map;
    s_enemyArchivesEnabled=s_enemyArchivesAvailable;
}
void update_enemy_archives() {
    if(s_enemyArchivesEnabled && (!s_active || !s_gameStarted || s_hub ||
        s_restart || s_map!=s_enemyArchiveMap || !in_arena() || transitioning()))
        release_enemy_archives();
}
void after_enemy_archive_ready(ModContext*,void* args,void* result,void*) {
    if(!s_enemyArchivesEnabled || !s_active || !s_gameStarted || s_hub ||
       s_restart || s_map!=s_enemyArchiveMap || !in_arena() || transitioning() ||
       !enemy_spawner_process_active() || !result ||
       *static_cast<int*>(result)!=0)return; // setRes: 0 ready, 1 pending, -1 error.
    auto* info=mods::arg<dRes_info_c*>(args,0);
    if(!info || !info->getArchive() || info->getDMCommand() || info->heap ||
       info->getCount()<=0 || info->getCount()>=0xffff)return;
    const auto* canonical=info->getArchiveName();
    const auto length=std::strlen(canonical);
    if(!length || length>=s_enemyArchives.front().name.size())return;
    // setRes also handles stage archives and other resource tables. Only retain
    // the exact live object-table record, never a same-name stage resource.
    if(dComIfG_getObjectResInfo(canonical)!=info)return;
    EnemyArchiveHold* available=nullptr;
    for(auto& hold:s_enemyArchives) {
        if(hold.archive==info->getArchive() && std::strcmp(hold.name.data(),canonical)==0) {
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
            freeze_diag::cache(freeze_diag::CacheEvent::Reuse);
#endif
            return;
        }
        if(!hold.archive && !available)available=&hold;
    }
    if(!available) {
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
        freeze_diag::cache(freeze_diag::CacheEvent::Full);
#endif
        return;
    }
    // The successful native load already mounted and prepared the archive.
    // setObjectRes now only increments its reference count: no second load,
    // phase request, polling, recursion or change to the actor's phase state.
    if(!dComIfG_setObjectRes(canonical,u8(0),nullptr))return;
    std::memcpy(available->name.data(),canonical,length+1);
    available->archive=info->getArchive();
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
    freeze_diag::cache(freeze_diag::CacheEvent::Acquire);
#endif
}
