void draw_combat_feedback() {
    if (s_run.phase!=Phase::Running || dComIfGp_event_runCheck() || !dComIfGd_getView()) return;
    auto* font=mDoExt_getMesgFont(); if (!font) return;
    const bool fateTooltips=show_fate_tooltips();
    for (const auto& text:s_combatTexts) {
        if (!text.remaining) continue;
        const auto& matrix=*dComIfGd_getProjViewMtx();
        auto world=text.position;
        const float w=matrix[3][0]*world.x+matrix[3][1]*world.y+matrix[3][2]*world.z+matrix[3][3];
        if (w<=1) continue;
        cXyz screen; mDoLib_project(&world,&screen);
        if (!std::isfinite(screen.x) || !std::isfinite(screen.y) ||
            screen.x<mDoGph_gInf_c::getMinXF() || screen.x>mDoGph_gInf_c::getMaxXF() ||
            screen.y<0 || screen.y>mDoGph_gInf_c::getHeightF()) continue;
        const u8 alpha=u8(255*text.remaining/text.lifetime);
        const char* name=text.kind==CombatText::Fate ? fate_names[unsigned(text.fate)] : text.kind==CombatText::Block ? "Block" : text.kind==CombatText::Dodge ? "Dodge" : "Crit";
        const JUtility::TColor color=text.kind==CombatText::Crit ? JUtility::TColor(255,210,60,255) :
            text.kind==CombatText::Dodge ? JUtility::TColor(140,255,170,255) : JUtility::TColor(150,220,255,255);
        J2DPrint shadow(font,JUtility::TColor(0,0,0,220),JUtility::TColor(0,0,0,220));
        shadow.setFontSize(16,16);
        shadow.printReturn(name,120,24,HBIND_CENTER,VBIND_TOP,screen.x-59,screen.y+1,alpha);
        J2DPrint print(font,color,color);print.setFontSize(16,16);
        print.printReturn(name,120,24,HBIND_CENTER,VBIND_TOP,screen.x-60,screen.y,alpha);
        if(text.kind==CombatText::Fate && fateTooltips) {
            const auto* tip=fate_tooltips[unsigned(text.fate)];
            shadow.setFontSize(12,12);print.setFontSize(12,12);
            shadow.printReturn(tip,360,20,HBIND_CENTER,VBIND_TOP,screen.x-179,screen.y+19,alpha);
            print.printReturn(tip,360,20,HBIND_CENTER,VBIND_TOP,screen.x-180,screen.y+18,alpha);
        }
    }
}
