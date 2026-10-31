// Per-encounter geometry only. Archive vertices are shared with Boss Rush and
// must never be edited. The model callback supplies our private position array;
// dBgW already owns a private world-space collision vertex table.
struct BossRingTerrain {
    static constexpr unsigned samples=96;
    std::array<float,samples> floor{};
    unsigned sampled=0;
    bool ready=false;
    std::unique_ptr<unsigned char[]> positions;
    J3DModel* model=nullptr;
    J3DCalcCallBack previousCallback=nullptr;
};
BossRingTerrain s_bossRingTerrain;
constexpr float kBossRingHeight=5.f,kBossRingSkirt=40.f;

void boss_ring_model_vertices(J3DModel* model,u32 timing) {
    auto& t=s_bossRingTerrain;
    if(model!=t.model)return;
    if(t.previousCallback)t.previousCallback(model,timing);
    if(t.ready)model->getVertexBuffer()->setCurrentVtxPos(t.positions.get());
}
void release_boss_ring_terrain(obj_gb_class* barrier=nullptr) {
    auto& t=s_bossRingTerrain;
    if(barrier&&barrier->mModel==t.model&&t.model) {
        if(t.model->mCalcCallBack==boss_ring_model_vertices)t.model->mCalcCallBack=t.previousCallback;
        t.model->getVertexBuffer()->frameInit();
    }
    t={};
}
float boss_ring_floor(float x,float z) {
    const auto& t=s_bossRingTerrain;
    constexpr float tau=6.2831853071795864769f;
    float angle=std::atan2(x,z);if(angle<0)angle+=tau;
    const float index=angle*(BossRingTerrain::samples/tau);
    const auto i=static_cast<unsigned>(index)%BossRingTerrain::samples;
    const float fraction=index-std::floor(index);
    return t.floor[i]+(t.floor[(i+1)%BossRingTerrain::samples]-t.floor[i])*fraction;
}
// Dusklight converts the source model array to host byte order. Keep its native
// format, count and stride (including padding) when replacing only Y values.
unsigned boss_ring_component_size(int type) {
    return type==GX_F32?4:(type==GX_S16||type==GX_U16)?2:(type==GX_S8||type==GX_U8)?1:0;
}
float boss_ring_component(const unsigned char* p,int type,unsigned frac) {
    float value=0;
    switch(type) {
    case GX_F32:std::memcpy(&value,p,4);return value;
    case GX_S16:{s16 n;std::memcpy(&n,p,2);value=n;break;}
    case GX_U16:{u16 n;std::memcpy(&n,p,2);value=n;break;}
    case GX_S8:{s8 n;std::memcpy(&n,p,1);value=n;break;}
    case GX_U8:value=*p;break;
    }
    return std::ldexp(value,-static_cast<int>(frac));
}
bool write_boss_ring_component(unsigned char* p,int type,unsigned frac,float value) {
    if(!std::isfinite(value))return false;
    if(type==GX_F32){std::memcpy(p,&value,4);return true;}
    const float n=std::round(std::ldexp(value,static_cast<int>(frac)));
    switch(type) {
    case GX_S16:if(n<-32768||n>32767)return false;{const s16 v=static_cast<s16>(n);std::memcpy(p,&v,2);break;}
    case GX_U16:if(n<0||n>65535)return false;{const u16 v=static_cast<u16>(n);std::memcpy(p,&v,2);break;}
    case GX_S8:if(n<-128||n>127)return false;{const s8 v=static_cast<s8>(n);std::memcpy(p,&v,1);break;}
    case GX_U8:if(n<0||n>255)return false;*p=static_cast<u8>(n);break;
    default:return false;
    }
    return true;
}
bool prepare_boss_ring_terrain(obj_gb_class* barrier,const cXyz& center) {
    auto& t=s_bossRingTerrain;
    if(t.ready)return true;
    if(!barrier->mModel||!barrier->mpBgW)return false;
    // Bound spawn-time work, and never sample again while this encounter lives.
    const auto stop=std::min(t.sampled+24,BossRingTerrain::samples);
    for(;t.sampled<stop;++t.sampled) {
        const float angle=t.sampled*(6.2831853071795864769f/BossRingTerrain::samples);
        cXyz probe(center.x+std::sin(angle)*boss_ring_radius,center.y+6000.f,center.z+std::cos(angle)*boss_ring_radius);
        dBgS_GndChk ground;ground.SetActorPid(fopAcM_GetID(barrier));ground.SetPos(&probe);
        const float y=dComIfG_Bgsp().GroundCross(&ground);
        t.floor[t.sampled]=std::isfinite(y)&&y>center.y-20000.f&&y<=probe.y?y:NAN;
    }
    if(t.sampled<BossRingTerrain::samples)return false;
    // Continue across voids/cliffs with the nearest real edge heights. A missing
    // ground hit must not send a section down to the engine's -1e9 sentinel.
    const auto sampled=t.floor;
    for(unsigned i=0;i<BossRingTerrain::samples;++i)if(!std::isfinite(sampled[i])) {
        unsigned left=1,right=1;
        while(left<BossRingTerrain::samples&&!std::isfinite(sampled[(i+BossRingTerrain::samples-left)%BossRingTerrain::samples]))++left;
        while(right<BossRingTerrain::samples&&!std::isfinite(sampled[(i+right)%BossRingTerrain::samples]))++right;
        t.floor[i]=left==BossRingTerrain::samples?center.y:
            (sampled[(i+BossRingTerrain::samples-left)%BossRingTerrain::samples]*right+
             sampled[(i+right)%BossRingTerrain::samples]*left)/(left+right);
    }
    auto* model=barrier->mModel;
    const auto& data=model->getModelData()->getVertexData();
    const unsigned count=data.getVtxNum(),stride=data.getVtxArrStride(GX_VA_POS),frac=data.getVtxPosFrac();
    const int type=data.getVtxPosType();const unsigned bytes=boss_ring_component_size(type);
    auto* bg=barrier->mpBgW;const auto bgCount=bg->GetVtxNum();
    if(!data.getVtxPosArray()||!count||count>32768||count>data.getVtxArrNum(GX_VA_POS)||
       !bytes||stride<3*bytes||stride>32||frac>31||bgCount<=0||bgCount>32768||
       !bg->GetVtxTbl()||!bg->GetBgd()||!bg->GetBgd()->m_v_tbl)return false;
    auto positions=std::unique_ptr<unsigned char[]>(new(std::nothrow) unsigned char[count*stride]);
    if(!positions)return false;
    std::memcpy(positions.get(),data.getVtxPosArray(),count*stride);
    float modelBottom=1.e9f,bgBottom=1.e9f;
    for(unsigned i=0;i<count;++i) {
        const auto* v=positions.get()+i*stride;
        for(unsigned axis=0;axis<3;++axis)if(!std::isfinite(boss_ring_component(v+axis*bytes,type,frac)))return false;
        modelBottom=std::min(modelBottom,boss_ring_component(v+bytes,type,frac));
    }
    for(int i=0;i<bgCount;++i)bgBottom=std::min(bgBottom,float(bg->GetBgd()->m_v_tbl[i].y));
    const float base=*std::min_element(t.floor.begin(),t.floor.end())-kBossRingSkirt;
    float top=base;
    for(unsigned i=0;i<count;++i) {
        auto* v=positions.get()+i*stride;
        const float x=boss_ring_component(v,type,frac),z=boss_ring_component(v+2*bytes,type,frac);
        const float y=boss_ring_component(v+bytes,type,frac)-modelBottom+
            (boss_ring_floor(x,z)-kBossRingSkirt-base)/kBossRingHeight;
        if(!write_boss_ring_component(v+bytes,type,frac,y))return false;
        top=std::max(top,base+y*kBossRingHeight);
    }
    // Publish only after the entire model copy has passed validation. Source
    // collision vertices remain untouched; these are the actor's world vertices.
    for(int i=0;i<bgCount;++i) {
        const auto& v=bg->GetBgd()->m_v_tbl[i];auto& out=bg->GetVtxTbl()[i];
        out.x=center.x+float(v.x)*1.5f;out.z=center.z+float(v.z)*1.5f;
        out.y=boss_ring_floor(float(v.x),float(v.z))-kBossRingSkirt+(float(v.y)-bgBottom)*kBossRingHeight;
        top=std::max(top,out.y);
    }
    barrier->current.pos=center;barrier->current.pos.y=base;
    // Rebuild planes and the broad-phase bounds once, then keep native Move
    // from flattening the private vertices again (including its 255-tick reset).
    bg->SetNoCalcVtx();bg->mNeedsFullTransform=1;bg->Move();bg->SetLock();
    t.positions=std::move(positions);t.model=model;t.previousCallback=model->mCalcCallBack;t.ready=true;
    model->mCalcCallBack=boss_ring_model_vertices;
    for(auto& sound:barrier->field_0x594)sound.y=boss_ring_floor(sound.x-center.x,sound.z-center.z);
    // Bounds are world-relative to the actor, whose Y now begins at the lowest
    // section. The tall, bent wall must not be culled using its flat archive box.
    fopAcM_setCullSizeBox(barrier,-1650.f,0.f,-1650.f,1650.f,top-base+100.f,1650.f);
    return true;
}
