void draw_chest_prices() {
    if (s_run.phase!=Phase::Running || dComIfGp_event_runCheck() || !dComIfGd_getView()) return;
    const auto* link=daAlink_getAlinkActorClass();
    if (!link) return;
    char price[24];
    for (unsigned i=0;i<kSmallChestCount;++i) {
        const auto& chest=s_chests[i];
        if (!chest.active || chest.opening) continue;
        if ((chest.position-link->current.pos).abs2()>=item_target_range*item_target_range) continue;
        if (chest_uses_key(chest)) std::snprintf(price,sizeof(price),"1 key");
        else std::snprintf(price,sizeof(price),"%u",s_run.chest_price(chest.free,chest.fixed));
        cXyz world=chest.position+cXyz(0,150+8*cM_ssin(s16(s_run.ticks*700)),0);
        const auto& matrix=*dComIfGd_getProjViewMtx();
        const float w=matrix[3][0]*world.x+matrix[3][1]*world.y+matrix[3][2]*world.z+matrix[3][3];
        if (w<=1) continue;
        cXyz screen; mDoLib_project(&world,&screen);
        if (!std::isfinite(screen.x) || !std::isfinite(screen.y) ||
            screen.x<mDoGph_gInf_c::getMinXF() || screen.x>mDoGph_gInf_c::getMaxXF() ||
            screen.y<0 || screen.y>mDoGph_gInf_c::getHeightF()) continue;
        label(price,screen.x,screen.y,std::clamp(18000.f/w,12.f,22.f));
    }
}

void draw_horde_portal_labels() {
    if(s_run.phase!=Phase::Running || s_victory || dComIfGp_event_runCheck() || !dComIfGd_getView())return;
    const auto* link=daAlink_getAlinkActorClass();
    if(!link)return;
    for(const auto& portal:s_hordePortals) {
        if(!horde_portal_visible(portal) ||
            (portal.position-link->current.pos).abs2()>=item_target_range*item_target_range)continue;
        cXyz world=portal.position+cXyz(0,150+8*cM_ssin(s16(s_run.ticks*700)),0);
        const auto& matrix=*dComIfGd_getProjViewMtx();
        const float w=matrix[3][0]*world.x+matrix[3][1]*world.y+matrix[3][2]*world.z+matrix[3][3];
        if (w<=1) continue;
        cXyz screen; mDoLib_project(&world,&screen);
        if (!std::isfinite(screen.x) || !std::isfinite(screen.y) ||
            screen.x<mDoGph_gInf_c::getMinXF() || screen.x>mDoGph_gInf_c::getMaxXF() ||
            screen.y<0 || screen.y>mDoGph_gInf_c::getHeightF()) continue;
        label("Fate",screen.x,screen.y,std::clamp(18000.f/w,12.f,22.f));
    }
}
