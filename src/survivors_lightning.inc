// Chain lightning is a run-owned hit sequence, not a native enemy actor. Every
// leg checks line of sight and every target is remembered once per chain.
// Shared by the simultaneous Multishot chains of one cast, not by delayed
// Multi Strike repeats. Reserve on selection so in-flight bolts spread too.
struct LightningTargets {
    std::array<ActorId,84> assigned{}; // Four chains, each initial hit + 20 hops.
    unsigned count=0;
    unsigned uses(ActorId id) const {
        return unsigned(std::count(assigned.begin(),assigned.begin()+count,id));
    }
    void reserve(ActorId id) { if(count<assigned.size())assigned[count++]=id; }
};
struct LightningBolt {
    bool active=false,reducedVisual=false;
    unsigned delay=0,life=0,hops=0,visitedCount=0,power=0,flash=0;
    float range=0;
    ActorId target=kNone;
    cXyz position{},legFrom{},flashFrom{},flashTo{};
    std::array<ActorId,21> visited{};
    std::array<u32,2> emitters{};
    std::shared_ptr<LightningTargets> targets;
};
std::deque<LightningBolt> s_lightning;
void stop_lightning(LightningBolt& bolt) {
    s_visualBudget.release(bolt.emitters.data());
    for(auto& key:bolt.emitters) {
        if(auto* emitter=key?dComIfGp_particle_getEmitter(key):nullptr) {
            emitter->stopCreateParticle();emitter->quitImmortalEmitter();emitter->becomeInvalidEmitter();
        }
        key=0;
    }
    bolt.active=false;bolt.flash=0;bolt.targets.reset();
}
void clear_lightning() { for(auto& bolt:s_lightning)stop_lightning(bolt); }
bool select_lightning_target(LightningBolt& bolt,daAlink_c* link) {
    if(bolt.visitedCount==bolt.visited.size())return false;
    Enemy* best=nullptr;float distance=bolt.range*bolt.range;unsigned fewest=~0u;
    const auto end=bolt.visited.begin()+bolt.visitedCount;
    for(auto& enemy:s_enemies) {
        if(!enemy.initialized || enemy.retiring || enemy.credited ||
           std::find(bolt.visited.begin(),end,enemy.id)!=end)continue;
        auto* actor=actor_by_id(enemy.id);
        if(!actor || actor->health<=0 || boss_blocks_from(enemy,actor,bolt.position))continue;
        const float d=actor->eyePos.abs2(bolt.position);
        if(d>=bolt.range*bolt.range)continue;
        const unsigned uses=bolt.targets?bolt.targets->uses(enemy.id):0;
        if(uses>fewest || (uses==fewest && d>=distance))continue;
        dBgS_LinChk wall;wall.Set(&bolt.position,&actor->eyePos,link);
        if(dComIfG_Bgsp().LineCross(&wall))continue;
        best=&enemy;distance=d;fewest=uses;
    }
    if(!best)return false;
    bolt.target=best->id;bolt.visited[bolt.visitedCount++]=best->id;
    if(bolt.targets)bolt.targets->reserve(best->id);
    bolt.legFrom=bolt.position;return true;
}
bool next_lightning_target(LightningBolt& bolt,daAlink_c* link) {
    if(!bolt.hops || !select_lightning_target(bolt,link))return false;
    --bolt.hops;return true;
}
void trigger_lightning(fopAc_ac_c* primary) {
    if(s_run.phase!=Phase::Running || !primary || !s_run.rank(Upgrade::Lightning) || s_cooldowns[unsigned(Upgrade::Lightning)] ||
       !s_run.roll_chance(s_run.lightning_proc_chance()))return;
    auto* owner=tracked(fopAcM_GetID(primary));
    if(!owner || owner->retiring || owner->credited)return;
    auto* link=daAlink_getAlinkActorClass();if(!link)return;
    s_cooldowns[unsigned(Upgrade::Lightning)]=s_run.item_cooldown(Upgrade::Lightning);
    // Multishot produces separate chains; Multi Strike repeats the complete cast.
    // Neither a chain contact nor a repeat can recursively trigger this proc.
    for(unsigned repeat=0;repeat<=s_run.multi_strikes();++repeat) {
        auto targets=std::make_shared<LightningTargets>();
        for(unsigned shot=0;shot<s_run.ranged_shots();++shot) {
            auto slot=std::find_if(s_lightning.begin(),s_lightning.end(),[](const auto& bolt){return !bolt.active;});
            if(slot==s_lightning.end()){s_lightning.emplace_back();slot=std::prev(s_lightning.end());}
            auto& bolt=*slot;bolt={};bolt.active=true;bolt.delay=repeat*4;bolt.targets=targets;
            bolt.position=bolt.legFrom=primary->eyePos;
            bolt.hops=s_run.lightning_hops();bolt.range=s_run.lightning_hop_range();bolt.power=s_run.lightning_power();
            // The first chain starts at the melee victim. Additional chains
            // prefer other reachable bodies before reusing an assigned target.
            if(!shot || !select_lightning_target(bolt,link)) {
                bolt.target=owner->id;bolt.visited[bolt.visitedCount++]=owner->id;
                targets->reserve(owner->id);
            }
            bolt.life=unsigned(std::ceil((bolt.hops+1)*(bolt.range/Run::lightning_speed+2)))+12;
        }
    }
}
void update_lightning(daAlink_c* link) {
    for(auto& bolt:s_lightning) {
        if(!bolt.active)continue;
        if(bolt.delay){--bolt.delay;continue;}
        if(!bolt.life--){stop_lightning(bolt);continue;}
        if(bolt.flash)--bolt.flash;
        auto* actor=actor_by_id(bolt.target);auto* enemy=tracked(bolt.target);
        if(!actor || !enemy || enemy->retiring || enemy->credited || actor->health<=0) {
            if(!next_lightning_target(bolt,link)) {
                if(!bolt.flash)stop_lightning(bolt);
                continue;
            }
            actor=actor_by_id(bolt.target);enemy=tracked(bolt.target);
        }
        const auto delta=actor->eyePos-bolt.position;
        const float distance=delta.abs();
        if(actor->eyePos.abs(bolt.legFrom)>bolt.range || boss_blocks_from(*enemy,actor,bolt.legFrom)) {
            bolt.target=kNone;continue;
        }
        const auto next=bolt.position+delta*(std::min(Run::lightning_speed,distance)/std::max(1.f,distance));
        dBgS_LinChk wall;wall.Set(&bolt.position,&next,link);
        if(dComIfG_Bgsp().LineCross(&wall)){stop_lightning(bolt);continue;}
        bolt.flashFrom=bolt.legFrom;bolt.flashTo=next;bolt.flash=6;bolt.position=next;
        // Use private common-bank copies, never a missing dungeon resource ID.
        bolt.reducedVisual=!full_visual(bolt.emitters,survivor_perf::Effect::Lightning,2);
        for(unsigned i=0;!bolt.reducedVisual && i<bolt.emitters.size();++i) {
            if(const u16 effect=lightning_strike_effect(i))
                bolt.emitters[i]=dComIfGp_particle_set(bolt.emitters[i],effect,&bolt.position,&link->tevStr,nullptr,nullptr,0xff,nullptr,-1,nullptr,nullptr,nullptr);
        }
        if(distance>Run::lightning_speed)continue;
        auto& effect=effect_for(*enemy);bool critical=false;
        queue_effect_damage(effect,DamageSource::Lightning,s_run.critical_power(bolt.power,&critical,true));
        effect.attribution.swordPending=true;effect.pendingCritical=effect.pendingCritical||critical;
        enemy->dormant=false;queue_knockback(*enemy,actor,false,DamageSource::Lightning);
        if(!next_lightning_target(bolt,link))bolt.target=kNone;
    }
}
void draw_lightning() {
    // Jagged cyan/white bolt geometry guarantees a visible connecting arc even
    // if a stage has not loaded Link's optional electric particle resource.
    for(const auto& bolt:s_lightning) {
        if(!bolt.active || !bolt.flash || bolt.delay)continue;
        if(bolt.reducedVisual) {
            performance_line(bolt.flashFrom,bolt.flashTo,GXColor{160,225,255,255});
            continue;
        }
        const auto delta=bolt.flashTo-bolt.flashFrom;
        cXyz previous=bolt.flashFrom;
        for(unsigned i=1;i<=8;++i) {
            cXyz point=bolt.flashFrom+delta*(float(i)/8);
            if(i<8) {
                const float jitter=((i+s_run.ticks/2)%2?1.f:-1.f)*std::min(24.f,delta.abs()*.08f);
                point.x+=jitter;point.y+=jitter*.6f;point.z-=jitter*.4f;
            }
            dDbVw_drawLineXlu(previous,point,GXColor{60,150,255,180},1,30);
            dDbVw_drawLineXlu(previous,point,GXColor{215,250,255,255},1,10);
            previous=point;
        }
    }
}
