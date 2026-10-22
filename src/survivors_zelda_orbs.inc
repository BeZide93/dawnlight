// Extra light balls use stable collision storage; never copy a native actor or
// its collider internals. The original ball keeps its native return mechanics.
struct ZeldaLightOrb {
    ActorId owner=kNone;
    unsigned delay=0,life=0,scheduled=0;
    cXyz position{},velocity{},offset{},target{};
    dCcD_Stts status{};
    dCcD_Sph collider{};
    std::array<u32,3> emitters{};
    EnemyAttackUsePtr use;
};
std::array<ZeldaLightOrb,64> s_zeldaLightOrbs;
void stop_zelda_light_orb(ZeldaLightOrb& orb) {
    orb.collider.OffAtSetBit();orb.collider.ResetAtHit();orb.collider.OffAtShieldHit();
    for(auto& id:orb.emitters) {
        if(id)if(auto* emitter=dComIfGp_particle_getEmitter(id)) {
            emitter->stopCreateParticle();emitter->quitImmortalEmitter();emitter->becomeInvalidEmitter();
        }
        id=0;
    }
    orb.owner=kNone;orb.life=orb.delay=0;orb.use.reset();
}
void clear_zelda_light_orbs(ActorId owner) {
    for(auto& orb:s_zeldaLightOrbs)if(orb.owner!=kNone&&(owner==kNone||orb.owner==owner))stop_zelda_light_orb(orb);
}
EnemyAttackUsePtr zelda_light_orb_use(cCcD_Obj* attack) {
    for(auto& orb:s_zeldaLightOrbs)if(orb.owner!=kNone&&attack==&orb.collider)return orb.use;
    return {};
}
void update_zelda_light_volley(e_hzelda_class* actor,Enemy& owner) {
    if(owner.retiring||owner.credited||actor->health<=0){clear_zelda_light_orbs(owner.id);return;}
    if(!in_arena()||!playable()||s_run.phase!=Phase::Running||owner.stagger)return;
    auto& t=s_bossTactics[owner.id];
    const bool launched=actor->mAction==3&&actor->field_0x6e4==2;
    if(launched&&!t.zeldaBallFlying) {
        const bool fan=(t.zeldaVolley++%2)==0;
        observe_enemy_attack(owner,actor);
        const auto use=s_enemyHitSources[owner.id].cycle.current();
        s_enemyHitSources[owner.id].magic=use;
        unsigned side=0;
        for(auto& orb:s_zeldaLightOrbs) {
            if(orb.owner!=kNone)continue;
            static const dCcD_SrcSph source={
                {{0,{{AT_TYPE_CSTATUE_SWING,4,0xd},{0,0},0}},
                 {dCcD_SE_13,1,0,0,0},{dCcD_SE_NONE,0,0,0,2},{0}},
                {{{0,0,0},15}}};
            orb.owner=owner.id;orb.life=120;orb.delay=fan?0:(side+1)*9;orb.scheduled=s_run.ticks;
            orb.position=actor->mBallPos;orb.offset=orb.position-actor->current.pos;
            orb.velocity=actor->mBallMove;
            auto* link=daAlink_getAlinkActorClass();
            orb.target=link?link->eyePos-cXyz(0,40,0):orb.position+orb.velocity*30.f;
            if(fan) {
                const s16 yaw=side==0?-0x900:0x900;
                const cXyz v=orb.velocity;
                orb.velocity.x=v.x*cM_scos(yaw)+v.z*cM_ssin(yaw);
                orb.velocity.z=v.z*cM_scos(yaw)-v.x*cM_ssin(yaw);
            }
            orb.status.Init(0xff,0,actor);orb.collider.Set(source);
            orb.collider.SetStts(&orb.status);orb.collider.SetAtMtrl(dCcD_MTRL_ELECTRIC);
            orb.collider.StartCAt(orb.position);orb.collider.OffAtSetBit();orb.use=use;
            if(++side==2)break;
        }
    }
    // Release can coincide with the animation's transition into recovery.
    // Reflected/returned balls (states 3/4) still belong to the original cast.
    t.zeldaBallFlying=actor->field_0x6e4>=2;
    for(auto& orb:s_zeldaLightOrbs) {
        if(orb.owner!=owner.id)continue;
        if(owner.retiring||owner.credited||actor->health<=0){stop_zelda_light_orb(orb);continue;}
        if(orb.delay) {
            // Do not count the scheduling frame as part of the shot interval.
            if(orb.scheduled==s_run.ticks)continue;
            --orb.delay;
            if(orb.delay)continue;
            orb.position=actor->current.pos+orb.offset;
            const cXyz d=orb.target-orb.position;const float n=d.abs();
            if(n>1)orb.velocity=d*(35.f/n);
            orb.collider.StartCAt(orb.position);
        }
        if(!orb.life||orb.collider.ChkAtHit()||orb.collider.ChkAtShieldHit()) {stop_zelda_light_orb(orb);continue;}
        --orb.life;
        cXyz next=orb.position+orb.velocity;
        dBgS_LinChk line;line.Set(&orb.position,&next,actor);
        if(dComIfG_Bgsp().LineCross(&line)){stop_zelda_light_orb(orb);continue;}
        orb.position=next;orb.collider.OnAtSetBit();orb.collider.MoveCAt(orb.position);
        dComIfG_Ccsp()->Set(&orb.collider);
        constexpr u16 effects[]{0x8918,0x8919,0x891a};
        for(unsigned i=0;i<3;++i)orb.emitters[i]=dComIfGp_particle_set(orb.emitters[i],effects[i],&orb.position,nullptr,nullptr);
    }
}
