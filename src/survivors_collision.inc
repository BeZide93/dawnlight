// Filter physical body contacts only. Attack/target collisions, terrain and
// object contacts (including pots and chests) keep their native processing.
HookAction before_enemy_contact(ModContext*,void* args,void*,void*) {
    if(!in_arena() || s_hub)return HOOK_CONTINUE;
    auto* first=mods::arg<cCcD_Obj*>(args,1);
    auto* second=mods::arg<cCcD_Obj*>(args,3);
    auto* a=first?first->GetAc():nullptr;
    auto* b=second?second->GetAc():nullptr;
    if(!a || !b)return HOOK_CONTINUE;
    const auto* ownerA=enemy_child_root(fopAcM_GetID(a));
    const auto* ownerB=enemy_child_root(fopAcM_GetID(b));
    // Run-owned enemies may overlap, including bosses and structural children.
    // Preserve contacts within one owner (e.g. rider/mount or multiple shapes).
    if(ownerA && ownerB && ownerA!=ownerB)return HOOK_SKIP_ORIGINAL;
    if(s_run.options.enemy_collision)return HOOK_CONTINUE;
    auto* link=daAlink_getAlinkActorClass();
    if(!link || fopAcM_GetID(link)!=s_player)return HOOK_CONTINUE;
    return (a==link && ownerB) || (b==link && ownerA) ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}
