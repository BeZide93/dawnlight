// Runs before status/projectile updates, never changes combat state.
void prepare_performance_visuals(daAlink_c* link) {
    static bool wasEnabled=false;
    static unsigned oldTier=0;
    const bool on=survivor_perf::enabled();const unsigned tier=survivor_perf::tier();
    if(on!=wasEnabled || (on && tier!=oldTier)) {
        // Stop old emitters before resetting reservations; otherwise a live toggle
        // or tier change could briefly double the particle population.
        for(auto& a:s_attacks){stop_visual_emitters(a.emitters);a.reducedVisual=on||a.exploding;}
        for(auto& b:s_lightning){stop_visual_emitters(b.emitters);b.reducedVisual=on;}
        for(auto& e:s_effects)stop_burn_visual(e);
        stop_visual_emitters(s_fireEdgeEmitters);stop_visual_emitters(s_jumpEmitters);
        s_visualBudget.clear();
    }
    wasEnabled=on;oldTier=tier;s_visualBudget.tier=tier;
    if(!on)return;
    std::array<std::pair<float,unsigned>,enemy_capacity> candidates{};size_t count=0;
    unsigned enemies=0,burning=0,projectiles=0,bolts=0;
    for(unsigned i=0;i<s_enemies.size();++i) {
        const auto& e=s_enemies[i];const auto& f=s_effects[i];
        if(!e.initialized||e.retiring||e.credited)continue;
        auto* a=actor_by_id(e.id);if(!a||a->health<=0)continue;++enemies;
        const bool burns=f.burn||std::any_of(f.legendaryBurns.begin(),f.legendaryBurns.end(),[](const auto& b){return b.remaining!=0;});
        if(f.id!=e.id||!burns)continue;
        ++burning;
        // Nearby visible targets are preferred. A projection test avoids a ray per enemy.
        cXyz screen;bool front=false;
        if(dComIfGd_getView()) {
            const auto& m=*dComIfGd_getProjViewMtx();const auto& p=a->eyePos;
            front=m[3][0]*p.x+m[3][1]*p.y+m[3][2]*p.z+m[3][3]>1;
            if(front)mDoLib_project(&a->eyePos,&screen);
        }
        const bool visible=front && screen.x>=mDoGph_gInf_c::getMinXF() && screen.x<=mDoGph_gInf_c::getMaxXF() &&
            screen.y>=0 && screen.y<=mDoGph_gInf_c::getHeightF();
        candidates[count++]={a->current.pos.abs2(link->current.pos)+(visible?0.f:1.e12f),i};
    }
    std::sort(candidates.begin(),candidates.begin()+count);
    s_burnVisuals.fill(false);
    for(size_t i=0;i<std::min(count,size_t(s_visualBudget.limit(survivor_perf::Effect::Burn)));++i)s_burnVisuals[candidates[i].second]=true;
    // Free non-selected burns first so newly selected near enemies can acquire slots this tick.
    for(unsigned i=0;i<s_effects.size();++i)if(!s_burnVisuals[i])stop_burn_visual(s_effects[i]);
    for(const auto& a:s_attacks)projectiles+=a.active;
    for(const auto& b:s_lightning)bolts+=b.active;
    survivor_perf::counts(enemies,projectiles,burning,bolts,s_visualBudget.used());
}

// One reusable packet for all simple ability markers; do not consume the game's
// 1000 debug-packet slots with a heap allocation for each line segment.
struct PerformanceLines : J3DPacket {
    struct Line {cXyz a,b;GXColor color;};
    std::vector<Line> lines;
    void draw() override {
        if(lines.empty())return;
        j3dSys.reinitGX();
        GXLoadPosMtxImm(j3dSys.getViewMtx(),GX_PNMTX0);GXSetCurrentMtx(GX_PNMTX0);
        GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
        GXSetNumChans(1);GXSetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
        GXSetNumTexGens(0);GXSetNumTevStages(1);
        GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);GXSetTevOp(GX_TEVSTAGE0,GX_PASSCLR);
        GXSetZMode(GX_TRUE,GX_LEQUAL,GX_FALSE);GXSetBlendMode(GX_BM_BLEND,GX_BL_SRCALPHA,GX_BL_INVSRCALPHA,GX_LO_CLEAR);
        GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);GXSetLineWidth(12,GX_TO_ZERO);
        for(size_t start=0;start<lines.size();start+=30000) {
            const size_t end=std::min(start+30000,lines.size());GXBegin(GX_LINES,GX_VTXFMT0,u16((end-start)*2));
            for(size_t i=start;i<end;++i)for(const auto& p:{lines[i].a,lines[i].b}) {
                const auto c=lines[i].color;GXPosition3f32(p.x,p.y,p.z);GXColor4u8(c.r,c.g,c.b,c.a);
            }
            GXEnd();
        }
        j3dSys.reinitGX();
        J3DShape::resetVcdVatCache();
    }
} s_performanceLines;
void performance_line(cXyz a,cXyz b,GXColor color){s_performanceLines.lines.push_back({a,b,color});}
void begin_performance_lines(){s_performanceLines.lines.clear();}
void submit_performance_lines(){
    auto* list=g_dComIfG_gameInfo.drawlist.mDrawBuffers[dDlst_list_c::DB_XLU_LIST];if(!list||s_performanceLines.lines.empty())return;
    for(u32 i=0;i<list->getEntryTableSize();++i)
        for(auto* p=list->mpBuffer[i];p;p=p->getNextPacket())if(p==&s_performanceLines)return;
    list->entryImm(&s_performanceLines,0);
}
void draw_reduced_attacks() {
    for(const auto& a:s_attacks) {
        if(!a.active||!a.reducedVisual)continue;
        if(!a.exploding && a.weapon!=0 && a.weapon!=unsigned(Upgrade::Shockwave))continue;
        const GXColor color=a.exploding?GXColor{255,170,65,220}:GXColor{120,235,255,230};
        // Keep the collision radius legible, including the lifetime of a reduced explosion.
        for(unsigned i=0;i<12;++i) {
            const float x=float(i)*6.2831853f/12,y=float(i+1)*6.2831853f/12;
            performance_line(a.position+cXyz(std::cos(x)*a.radius,0,std::sin(x)*a.radius),
                a.position+cXyz(std::cos(y)*a.radius,0,std::sin(y)*a.radius),color);
        }
    }
}

void clear_performance_lines() {
    auto* list=g_dComIfG_gameInfo.drawlist.mDrawBuffers[dDlst_list_c::DB_XLU_LIST];
    if(list && list->mpBuffer)for(u32 i=0;i<list->getEntryTableSize();++i) {
        J3DPacket* previous=nullptr;
        for(auto* p=list->mpBuffer[i];p;) {
            auto* next=p->getNextPacket();
            if(p==&s_performanceLines) {
                if(previous)previous->setNextPacket(next);else list->mpBuffer[i]=next;
            }else previous=p;
            p=next;
        }
    }
    s_performanceLines.setNextPacket(nullptr);s_performanceLines.lines.clear();
}
