// Temporary PR148 diagnostic: observe the real animated bite and Link's real
// target cylinders. Never move a collider, allocate an attack use or alter damage.
#if DAWNLIGHT_SURVIVORS_DEBUG
void trace_stalhound_contact(const Enemy& e,fopAc_ac_c* actor,cCcD_Obj* at) {
    if(e.profile!=fpcNm_E_SH_e||fopAcM_GetID(actor)!=e.id)return;
    auto* hound=reinterpret_cast<e_sh_class*>(actor);
    if(at!=&hound->mSph2)return;
    auto it=s_bossTactics.find(e.id);if(it==s_bossTactics.end())return;
    auto& t=it->second;auto& p=t.stalhoundProbe;
    if(p.reports>=8)return;
    ++p.attempts;
    if(t.grace||t.recovery||(t.hasContact&&s_run.ticks-t.lastContact<15))++p.safety;
    const auto hit=s_enemyHitSources.find(e.id);
    if(hit!=s_enemyHitSources.end()&&hit->second.cycle.use&&hit->second.cycle.use->contacted)++p.spent;
}
void trace_stalhound_frame(const Enemy& e,fopAc_ac_c* actor) {
    if(e.profile!=fpcNm_E_SH_e||!e.elite||!e.initialized||e.retiring||e.credited||
       e.dormant||actor->health<=0||!playable())return;
    auto* link=daAlink_getAlinkActorClass();if(!link)return;
    auto it=s_bossTactics.find(e.id);if(it==s_bossTactics.end())return;
    auto& p=it->second.stalhoundProbe;if(p.reports>=8)return;
    auto* hound=reinterpret_cast<e_sh_class*>(actor);
    const auto action=hound->field_0x676,mode=hound->field_0x678;
    if(!p.frames&&!p.reports) {
        char message[256];
        std::snprintf(message,sizeof(message),
            "Survivors SH probe v2 id=%u map=%u scale=%.2f action=%d mode=%d (passive; max 8 reports)",
            unsigned(e.id),unsigned(s_map),double(actor->scale.x),int(action),int(mode));
        svc_log->info(mod_ctx,message);
    }
    ++p.frames;
    if(action==3&&mode>=0&&mode<8)p.phases|=1u<<unsigned(mode);
    auto& sphere=hound->mSph2;
    const auto center=sphere.GetC();
    // Native disables the bite by parking it at (15000,-14000,23000), while
    // leaving its At bit set. Report hidden frames separately from enabled ones.
    const bool visible=(center-actor->current.pos).abs2()<4000000.f;
    if(visible) {
        ++p.visible;if(sphere.ChkAtSet())++p.enabled;
        bool overlap=false;
        for(const auto& cylinder:link->mTgCyls) {
            if(!cylinder.ChkTgSet())continue;
            const auto base=cylinder.GetC();const auto delta=center-base;
            const float horizontal=std::max(0.f,delta.absXZ()-cylinder.GetR());
            const float vertical=std::max({0.f,-delta.y,delta.y-cylinder.GetH()});
            const float gap=std::sqrt(horizontal*horizontal+vertical*vertical)-sphere.GetR();
            overlap|=gap<=0;
            if(gap<p.gap) {
                p.gap=gap;p.bite=center;p.target=base;p.root=actor->current.pos;
                p.radius=sphere.GetR();p.height=cylinder.GetH();p.cylinderRadius=cylinder.GetR();
            }
        }
        if(overlap)++p.overlaps;
    }
    if(sphere.ChkAtHit()&&sphere.GetAtHitAc()==link)++p.nativeHits;
    const auto hit=s_enemyHitSources.find(e.id);
    if(hit!=s_enemyHitSources.end()&&hit->second.cycle.use)
        p.accepted|=hit->second.cycle.use->contacted;
    const bool ended=p.lastAction==3&&(action!=3||mode<p.lastMode);
    p.lastAction=action;p.lastMode=mode;
    // Include idle/stuck attacks after five active seconds, even without a bite.
    if(!ended&&p.frames<150)return;
    char message[768];
    std::snprintf(message,sizeof(message),
        "Survivors SH probe v2 id=%u report=%u end=%d frames=%u action=%d mode=%d phases=0x%x "
        "visible=%u enabled=%u overlap=%u attempts=%u safety=%u spent=%u accepted=%d nativeHitFrames=%u "
        "gap=%.2f bite=(%.1f,%.1f,%.1f) r=%.1f target=(%.1f,%.1f,%.1f) r=%.1f h=%.1f "
        "root=(%.1f,%.1f,%.1f) speed=%.1f ground=%d",
        unsigned(e.id),p.reports+1,int(ended),p.frames,int(action),int(mode),p.phases,
        p.visible,p.enabled,p.overlaps,p.attempts,p.safety,p.spent,int(p.accepted),p.nativeHits,
        double(p.gap),double(p.bite.x),double(p.bite.y),double(p.bite.z),double(p.radius),
        double(p.target.x),double(p.target.y),double(p.target.z),double(p.cylinderRadius),double(p.height),
        double(p.root.x),double(p.root.y),double(p.root.z),double(actor->speedF),int(hound->mObjAcch.ChkGroundHit()));
    svc_log->info(mod_ctx,message);
    const auto reports=p.reports+1;p={};p.reports=reports;p.lastAction=action;p.lastMode=mode;
}
#endif
