// Gameplay cannot use the status menu's multi-frame shield reload: it frees
// mShieldModel while setCollision() still unconditionally reads its matrix.
// Prepare the next archive without touching Link, then finish the native reload
// in one callback while that archive is held ready.
struct ShieldReload {
    ActorId player = kNone;
    u8 item = dItemNo_NONE_e;
    const char* archive = nullptr;
    bool held = false;
} s_shieldReload;

void clear_shield_reload() {
    const auto pending = s_shieldReload;
    s_shieldReload = {};
    // setObjectRes acquired exactly one reference, including while I/O is pending.
    // resDelete is unsuitable here: its phase wrapper only releases completed loads.
    if (pending.held) dComIfG_deleteObjectResMain(pending.archive);
}
void request_shield_reload(daAlink_c* link, u8 item) {
    clear_shield_reload();
    const char* archive = item == dItemNo_WOOD_SHIELD_e ? "CWShd" :
        item == dItemNo_SHIELD_e ? "SWShd" : "HyShd";
    s_shieldReload = {fopAcM_GetID(link), item, archive, false};
}
void validate_shield_reload() {
    if (s_shieldReload.player == kNone) return;
    auto* link = daAlink_getAlinkActorClass();
    if (!in_arena() || transitioning() || !link ||
        fopAcM_GetID(link) != s_shieldReload.player ||
        dComIfGs_getSelectEquipShield() != s_shieldReload.item)
        clear_shield_reload();
}
HookAction before_survivors_shield_update(ModContext*, void* args, void*, void*) {
    validate_shield_reload();
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (s_shieldReload.player == kNone || !link ||
        link != daAlink_getAlinkActorClass() || link->checkStatusWindowDraw() ||
        link->getShieldChangeWaitTimer() != 0) return HOOK_CONTINUE;

    // No-shield -> Ordon shield already uses CWShd. A native Collection change
    // may also have completed the requested reload while our request was pending.
    if (link->mShieldArcName &&
        std::strcmp(link->mShieldArcName, s_shieldReload.archive) == 0) {
        clear_shield_reload();
        return HOOK_CONTINUE;
    }
    if (!s_shieldReload.held) {
        if (!dComIfG_setObjectRes(s_shieldReload.archive, u8(0), nullptr)) {
            clear_shield_reload();
            return HOOK_CONTINUE; // Keep the old live model if loading fails.
        }
        s_shieldReload.held = true;
    }
    const int state = dComIfG_syncObjectRes(s_shieldReload.archive);
    if (state > 0) return HOOK_CONTINUE;
    if (state < 0 || !dComIfG_getObjectRes(s_shieldReload.archive, 3)) {
        clear_shield_reload();
        return HOOK_CONTINUE;
    }

    // The target differs from the old archive and is fully loaded outside
    // mpShieldArcHeap, so freeing that heap cannot invalidate the target. Native
    // resLoad acquires Link's own reference before our temporary hold is released.
    // Four bounded steps: 4->3, 3->2 (delete), 2->1->2 (cached phase advance),
    // 2->1->0 (create). No waiting/spinning for I/O, no null-model frame, and no
    // model/phase/heap pointers are retained or transplanted by Dawnlight.
    link->setShieldChange();
    for (unsigned step = 0; step < 4 && link->getShieldChangeWaitTimer(); ++step)
        link->loadShieldModelDVD();
    clear_shield_reload();
    return HOOK_CONTINUE;
}
