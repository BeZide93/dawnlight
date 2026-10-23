// Separate enemy projectiles: never reuse player attack storage or ownership.
struct KingWave {
    ActorId owner=kNone;
    float remaining=0;
    cXyz position{},velocity{};
    csXyz rotation{};
    dCcD_Stts status{};
    dCcD_Sph collider{};
    std::array<u32,2> emitters{};
    EnemyAttackUsePtr use;
};
std::array<KingWave,32> s_kingWaves;
void stop_king_wave(KingWave& wave) {
    wave.collider.OffAtSetBit();wave.collider.ResetAtHit();wave.collider.OffAtShieldHit();
    for(auto& id:wave.emitters) {
        if(id)if(auto* emitter=dComIfGp_particle_getEmitter(id)) {
            emitter->stopCreateParticle();emitter->quitImmortalEmitter();emitter->becomeInvalidEmitter();
        }
        id=0;
    }
    wave.owner=kNone;wave.remaining=0;wave.use.reset();
}
void clear_king_waves(ActorId owner) {
    for(auto& wave:s_kingWaves)if(wave.owner!=kNone&&(owner==kNone||owner==wave.owner))stop_king_wave(wave);
}
EnemyAttackUsePtr king_wave_use(cCcD_Obj* attack) {
    for(auto& wave:s_kingWaves)if(wave.owner!=kNone&&attack==&wave.collider)return wave.use;
    return {};
}
void launch_king_wave(e_rdb_class* actor,Enemy& owner,s16 yaw,const EnemyAttackUsePtr& use) {
    for(auto& wave:s_kingWaves) {
        if(wave.owner!=kNone)continue;
        static const dCcD_SrcSph source={
            {{0,{{AT_TYPE_CSTATUE_SWING,12,0xd},{0,0},0}},
             {dCcD_SE_METAL,0,2,0,0},{dCcD_SE_NONE,0,0,0,2},{0}},
            {{{0,0,0},90}}};
        wave.owner=owner.id;wave.remaining=2.f*boss_ring_radius;wave.use=use;
        wave.position=actor->enemy.current.pos+cXyz(0,65,0);
        wave.velocity=cXyz(cM_ssin(yaw)*35.f,0,cM_scos(yaw)*35.f);
        wave.rotation=csXyz(0,yaw,0);
        wave.status.Init(0xff,0,&actor->enemy);wave.collider.Set(source);wave.collider.SetStts(&wave.status);
        wave.collider.StartCAt(wave.position);wave.collider.OffAtSetBit();return;
    }
}
void update_king_waves(e_rdb_class* actor,Enemy& owner) {
    if(owner.retiring||owner.credited||actor->enemy.health<=0){clear_king_waves(owner.id);return;}
    if(!in_arena()||!playable()||s_run.phase!=Phase::Running)return;
    // The native action sets 6c4 only during the axe's real damage window.
    const bool normal=actor->mAction==3&&actor->field_0x6c4==1;
    const bool spin=actor->mAction==4&&actor->mMode==2&&actor->field_0x6c4==2;
    const unsigned count=owner.kingWaveClock.advance(normal&&!owner.stagger,spin&&!owner.stagger);
    if(count) {
        // One contact budget per strike/salvo, across all of Link's colliders.
        auto use=new_enemy_attack_use();
        // Aim the first wave at Link's current position on each release.
        const s16 aimedYaw=fopAcM_searchPlayerAngleY(&actor->enemy);
        if(count==1)actor->enemy.current.angle.y=actor->enemy.shape_angle.y=aimedYaw;
        for(unsigned i=0;i<count;++i)launch_king_wave(actor,owner,
            i==0?aimedYaw:s16(s_run.next_random()&0xffff),use);
    }
    for(auto& wave:s_kingWaves) {
        if(wave.owner!=owner.id)continue;
        if(wave.remaining<=0||wave.collider.ChkAtHit()||wave.collider.ChkAtShieldHit()) {stop_king_wave(wave);continue;}
        const float step=std::min(35.f,wave.remaining);
        wave.remaining-=step;
        cXyz next=wave.position+wave.velocity*(step/35.f);
        dBgS_LinChk line;line.Set(&wave.position,&next,&actor->enemy);
        if(dComIfG_Bgsp().LineCross(&line)){stop_king_wave(wave);continue;}
        wave.position=next;wave.collider.OnAtSetBit();wave.collider.MoveCAt(next);dComIfG_Ccsp()->Set(&wave.collider);
        constexpr u16 effects[]{ID_ZI_J_KAITENGIRI_A,ID_ZI_J_KAITENGIRI_B};
        const cXyz scale(.35f,.35f,.35f);
        for(unsigned i=0;i<2;++i) {
            auto& id=wave.emitters[i];
            if(auto* emitter=id?dComIfGp_particle_getEmitter(id):nullptr;
               emitter&&emitter->mMaxFrame>0&&emitter->getAge()>=emitter->mMaxFrame) {
                emitter->stopCreateParticle();emitter->quitImmortalEmitter();emitter->becomeInvalidEmitter();id=0;
            }
            id=dComIfGp_particle_set(id,effects[i],&wave.position,&actor->enemy.tevStr,&wave.rotation,&scale,
                0xff,nullptr,-1,nullptr,nullptr,nullptr);
        }
    }
}
