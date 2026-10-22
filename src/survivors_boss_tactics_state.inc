struct BossTactics {
    BossWatchdog watchdog;
    BossStagger stagger;
    unsigned majorUntil=0,ridersUntil=0,lastContact=0;bool hasContact=false,fanPending=false;
    unsigned grace=0, recovery=0, phaseTick=0, combo=0, sequence=0;
    unsigned pattern=0, summons=0, pendingRiders=0, riderDelay=0;
    ActorId summoner=kNone;
    int swordState=0;
    cXyz throwTarget{};
    int motionMode=-1;unsigned motionTick=0,passes=0;
    int pzStage=0;
    int lastAction=-1,lastMode=-1,lastTimer=0;
    bool waitingVolley=false;
    bool attacking=false, targetLocked=false, triangle=false, projectileReady=false;
    bool magicActive[2]{};
    unsigned queuedShots=0, shotDelay=0, shotIndex=0;
    u32 shotParams=0;
    csXyz shotAngle{};
    cXyz shotPosition{}, shotTarget{};
    EnemyAttackUsePtr volley;
    cXyz target{}, origin{}, direction{};
    s16 yaw=0;
};
std::unordered_map<ActorId,BossTactics> s_bossTactics;
bool timed_boss(const Enemy& e) {
    return e.id!=kNone && e.elite && e.eliteSlot>=0 &&
        unsigned(e.eliteSlot)<timed_elite_count && !e.retiring && !e.credited;
}
bool active_boss_encounter() {
    return std::any_of(s_enemies.begin(),s_enemies.end(),[](const auto& e){
        return timed_boss(e) || (e.id!=kNone && e.elite && e.initialized &&
            !e.dormant && !e.retiring && !e.credited);
    });
}
bool boss_tactics_owned(const Enemy& e) {
    return e.id!=kNone && (e.elite||native_boss_profile(e.profile)) &&
        e.initialized && !e.retiring && !e.credited && !e.dormant;
}
bool boss_accept_stagger(Enemy& e) {
    return !boss_tactics_owned(e) || s_bossTactics[e.id].stagger.accept();
}
void boss_received_damage(Enemy& e,int hp) {
    if(boss_tactics_owned(e))s_bossTactics[e.id].watchdog.damaged(hp);
}
bool boss_attack_paused(const Enemy& e) {
    const auto it=s_bossTactics.find(e.id);
    return it!=s_bossTactics.end()&&(it->second.grace||it->second.recovery);
}
void update_boss_watchdogs(daAlink_c*);
void before_boss_tactics(Enemy&,fopAc_ac_c*);
void after_boss_tactics(Enemy&,fopAc_ac_c*);
void update_boss_projectile(fopAc_ac_c*,bool);

cXyz boss_lead_target(float ticks);
s16 boss_target_yaw(fopAc_ac_c*,const cXyz&);
bool boss_major_ready(Enemy&,unsigned);
void boss_idle(Enemy&,fopAc_ac_c*);
void boss_ground_step(fopAc_ac_c*,const cXyz&,float);
bool boss_pz_minion(Enemy&,ProfileName);
void boss_queue_shots(Enemy&,fopAc_ac_c*);

void boss_mobile_patterns(Enemy&,fopAc_ac_c*);
void boss_after_patterns(Enemy&,fopAc_ac_c*);

HookAction boss_carrier_pass(e_yc_class*,Enemy&);

void boss_record_contact(cCcD_Obj*);
