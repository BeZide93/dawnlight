// Run-local treasure and item projectiles. Included inside Survivors' private
// namespace; native assets are borrowed/refcounted, never added to the bundle.
constexpr unsigned kPaidChestCount = 6, kSmallChestCount = kPaidChestCount + map_chest_capacity, kChestCount = kSmallChestCount + elite_reward_capacity, kItemBegin = 8, kSlotsPerItem = 4;
struct Chest {
    bool active = false, opening = false;
    bool free = false, fixed = false;
    cXyz position{};
    s16 yaw = 0;
    unsigned age = 0;
    uint64_t spawnOrder = 0;
    ChestReward reward = ChestReward::Count;
    std::array<ChestReward, 3> offers{ChestReward::Count, ChestReward::Count, ChestReward::Count};
    std::array<Rarity, 3> rarities{};
};
std::array<Chest, kChestCount> s_chests;
std::array<bool,map_chest_capacity> s_mapChestPlaced{};
unsigned s_chestDelay = 20 * ticks_per_second, s_selectedChest = kChestCount;
uint64_t s_chestSequence = 0;
bool s_chestsExhausted = false;
struct LootModels {
    JKRSolidHeap* heap = nullptr;
    J3DModel* masterSword = nullptr;
    J3DModel* bow = nullptr;
    J3DModel* heldBoomerang = nullptr;
    J3DModel* heldBomb = nullptr;
    J3DModel* heldBall = nullptr;
    J3DModelData* chainData = nullptr;
    std::array<J3DModel*, kSlotsPerItem> wind{};
    std::array<mDoExt_bckAnm*, 2> bowAnims{};
    mDoExt_bckAnm* windAnim = nullptr;
    request_of_phase_process_class chestPhase{}, linkPhase{};
    const char* linkArchive = nullptr;
    std::array<J3DModel*, initial_projectile_slots> projectiles{};
    std::array<J3DModelData*, 4> projectileData{};
    J3DModelData* windData = nullptr;
    std::array<J3DModel*, kChestCount> chests{};
    std::array<mDoExt_bckAnm*, kChestCount> lids{};
    bool ready = false, failed = false;
} s_loot;

void clear_loot() {
    clear_locomotion();
    cancel_treasure_demo();
    // Some engines cache model matrices outside the owning heap.
    using ForgetModel = void (*)(J3DModel*);
    void* address = nullptr;
    if (s_loot.heap && svc_hook && svc_hook->resolve &&
        svc_hook->resolve(mod_ctx, "J3DModel::forgetMtx", &address, nullptr) == MOD_OK && address) {
        auto forget = reinterpret_cast<ForgetModel>(address);
        for (auto& a : s_attacks) {
            if (a.bombModel) forget(a.bombModel);
            if (!a.modelHeap) continue;
            if (a.model) forget(a.model);
            if (a.wind) forget(a.wind);
        }
        for (auto* model : s_loot.projectiles) if (model) forget(model);
        for (auto* model : s_loot.chests) if (model) forget(model);
        if (s_loot.masterSword) forget(s_loot.masterSword);
        if (s_loot.bow) forget(s_loot.bow);
        if (s_loot.heldBoomerang) forget(s_loot.heldBoomerang);
        if (s_loot.heldBomb) forget(s_loot.heldBomb);
        if (s_loot.heldBall) forget(s_loot.heldBall);
        for (auto* model : s_loot.wind) if (model) forget(model);
    }
    for (auto& a : s_attacks) {
        if (a.bombHeap) mDoExt_destroySolidHeap(a.bombHeap);
        a.bombHeap=nullptr; a.bombModel=nullptr;
        if (a.modelHeap) mDoExt_destroySolidHeap(a.modelHeap);
        a.modelHeap = nullptr; a.model = a.wind = nullptr;
    }
    if (s_loot.heap) mDoExt_destroySolidHeap(s_loot.heap);
    dComIfG_resDelete(&s_loot.chestPhase, "Dalways");
    if (s_loot.linkArchive) dComIfG_resDelete(&s_loot.linkPhase, s_loot.linkArchive);
    s_loot = {}; s_chests = {}; s_mapChestPlaced = {}; s_selectedChest = kChestCount;
    s_chestDelay = 20 * ticks_per_second; s_chestSequence = 0; s_chestsExhausted=false;
}
bool prepare_loot(daAlink_c* link) {
    if (s_loot.ready) return true;
    if (s_loot.failed) return false;
    s_loot.linkArchive = daAlink_c::getAlinkArcName();
    const auto chestPhase = dComIfG_resLoad(&s_loot.chestPhase, "Dalways");
    const auto linkPhase = dComIfG_resLoad(&s_loot.linkPhase, s_loot.linkArchive);
    if (chestPhase == cPhs_ERROR_e || linkPhase == cPhs_ERROR_e) {
        clear_loot(); s_loot.failed = true; return false;
    }
    if (chestPhase != cPhs_COMPLEATE_e || linkPhase != cPhs_COMPLEATE_e) return false;
    if (!dComIfGp_getAnmArchive()) { clear_loot(); s_loot.failed = true; return false; }
    s_loot.heap = mDoExt_createSolidHeapFromGame(0x300000, 0x20);
    if (!s_loot.heap) { clear_loot(); s_loot.failed = true; return false; }
    auto* previous = mDoExt_setCurrentHeap(s_loot.heap);
    const auto model = [](J3DModelData* data, bool environment = false) {
        return data ? mDoExt_J3DModel__create(data, environment ? 0 : 0x80000,
                                            environment ? 0x13000684 : 0x11000084) : nullptr;
    };
    s_loot.bow = model(link->loadAramBmd(dRes_ID_ALANM_BMD_AL_BOW_e, 0x4c00), true);
    s_loot.heldBoomerang = model(static_cast<J3DModelData*>(dComIfG_getObjectRes(s_loot.linkArchive, 0x1f)));
    const auto animation = [](J3DAnmTransform* data) -> mDoExt_bckAnm* {
        auto* result = JKR_NEW mDoExt_bckAnm();
        return data && result && result->init(data, TRUE, J3DFrameCtrl::EMode_NONE, 0, 0, -1, false) ? result : nullptr;
    };
    s_loot.bowAnims[0] = animation(static_cast<J3DAnmTransform*>(link->loadAram(dRes_ID_ALANM_BCK_BARELORD_e, 0x4000)));
    s_loot.bowAnims[1] = animation(static_cast<J3DAnmTransform*>(link->loadAram(dRes_ID_ALANM_BCK_BASHOOT_e, 0x4000)));
    s_loot.windAnim = animation(static_cast<J3DAnmTransform*>(dComIfG_getObjectRes(s_loot.linkArchive, 0x13)));
    s_loot.windData = static_cast<J3DModelData*>(dComIfG_getObjectRes(s_loot.linkArchive, 0x34));
    for (auto*& wind : s_loot.wind) wind = model(s_loot.windData, true);
    auto* chestData = static_cast<J3DModelData*>(dComIfG_getObjectRes("Dalways", 12));
    auto* bigChestData = static_cast<J3DModelData*>(dComIfG_getObjectRes("Dalways", 13));
    auto* bigLidData = static_cast<J3DAnmTransform*>(dComIfG_getObjectRes("Dalways", 8));
    auto* lidData = static_cast<J3DAnmTransform*>(dComIfG_getObjectRes("Dalways", 7));
    // Master Sword has only a rupee placeholder in native item_resource.
    // Give its presentation a private model instance, borrowing Link's data.
    s_loot.masterSword = model(link->mpSwMModel ? link->mpSwMModel->getModelData() : nullptr);
    bool ready = chestData && lidData && bigChestData && bigLidData && s_loot.masterSword && s_loot.bow && s_loot.heldBoomerang &&
        s_loot.bowAnims[0] && s_loot.bowAnims[1] && s_loot.windAnim;
    for (auto* wind : s_loot.wind) ready = ready && wind;
    for (unsigned i = 0; i < kChestCount; ++i) {
        s_loot.chests[i] = model(i < kSmallChestCount ? chestData : bigChestData);
        s_loot.lids[i] = JKR_NEW mDoExt_bckAnm();
        ready = ready && s_loot.chests[i] && s_loot.lids[i] &&
            s_loot.lids[i]->init(i < kSmallChestCount ? lidData : bigLidData, TRUE, J3DFrameCtrl::EMode_NONE, 0, 0, -1, false);
    }
    // Read the ball into this heap, not Link's equipment heap. loadAramBmd
    // allocates its own resource buffer and does not equip/change Link's item.
    auto* ball = link->loadAramBmd(dRes_ID_ALANM_BMD_AL_IB_e, 0x2800);
    s_loot.heldBall = model(ball, true);
    s_loot.heldBomb = model(static_cast<J3DModelData*>(dComIfG_getObjectRes(s_loot.linkArchive, 0x1e)));
    s_loot.chainData = link->loadAramBmd(dRes_ID_ALANM_BMD_AL_HS_KUSARI_e, 0x1000);
    ready = ready && s_loot.heldBall && s_loot.heldBomb && s_loot.chainData;
    s_loot.projectileData = {
        static_cast<J3DModelData*>(dComIfG_getObjectRes(s_loot.linkArchive, 0x1c)),
        static_cast<J3DModelData*>(dComIfG_getObjectRes(s_loot.linkArchive, 0x1f)),
        static_cast<J3DModelData*>(dComIfG_getObjectRes(s_loot.linkArchive, 0x1e)), ball};
    for (unsigned type = 0; type < s_loot.projectileData.size(); ++type)
        for (unsigned slot = 0; slot < item_initial_slots(type); ++slot) {
            auto*& instance = s_loot.projectiles[item_projectile_begin(type) + slot];
            instance = model(s_loot.projectileData[type], type == 3);
            auto& attack = s_attacks[item_projectile_begin(type) + slot];
            attack.model = instance;
            if (type == 1) attack.wind = s_loot.wind[slot];
            ready = ready && instance;
        }
    if (ball) dRes_info_c::offWarpMaterial(ball);
    mDoExt_setCurrentHeap(previous);
    if (!ready) {
        clear_loot(); s_loot.failed = true;
        svc_log->warn(mod_ctx, "Hyrule Survivors: treasure models unavailable; purchases disabled");
        return false;
    }
    mDoExt_adjustSolidHeap(s_loot.heap);
    s_loot.ready = true;
    return true;
}
bool chest_spawning_allowed() {
    // Latch for this run, including Endless continuation. Never erase a live
    // item-get choice/demo; check again as soon as it returns to Running.
    if (!s_chestsExhausted && s_run.phase==Phase::Running && !s_run.starting_item_pending &&
        !s_run.chest_rewards_left(potion_slot()>=0)) {
        s_chestsExhausted=true;s_chests={};s_selectedChest=kChestCount;
    }
    return !s_chestsExhausted;
}
// Small replenishing and large reward chests have separate model/slot pools.
// Fixed map chests and the live treasure interaction are never replacement candidates.
bool find_chest_spawn_slot(unsigned first, unsigned end, unsigned& slot) {
    slot=end;
    for(unsigned i=first;i<end;++i) {
        const auto& chest=s_chests[i];
        if(chest.fixed || chest.opening || i==s_selectedChest)continue;
        if(!chest.active) {slot=i;return true;}
        if(slot==end || chest.spawnOrder<s_chests[slot].spawnOrder)slot=i;
    }
    return slot!=end;
}
void drop_elite_chest(Enemy& enemy, fopAc_ac_c* actor) {
    if (enemy.eliteSlot<0 || unsigned(enemy.eliteSlot)>=elite_capacity) return;
    auto& encounter=s_eliteEncounters[enemy.eliteSlot];
    if (encounter.defeated) return;
    encounter.defeated=true;
    if(s_run.endless && unsigned(enemy.eliteSlot)<timed_elite_count)
        encounter.respawnAt=std::max(encounter.respawnAt+timed_elite_count*elite_interval,s_run.ticks+1);
    if (unsigned(enemy.eliteSlot)>=timed_elite_count)
        encounter.respawnAt=s_run.ticks+guard_respawn_interval;
    // A guard slot rewards its first confirmed kill only, including relocation.
    // Respawns still give XP and retain their respawn schedule.
    if (encounter.rewarded || !chest_spawning_allowed()) return;
    unsigned slot;
    if (!find_chest_spawn_slot(kSmallChestCount,kChestCount,slot)) return;
    encounter.rewarded=true;
    auto& chest=s_chests[slot];
    chest={}; chest.active=true; chest.free=true; chest.yaw=actor->shape_angle.y;
    chest.spawnOrder=++s_chestSequence;
    cXyz pos=actor->current.pos;
    if (!loot_floor_position(pos)) pos=enemy.lootPosition;
    chest.position=pos;
}

bool nearest_item_target(daAlink_c* link, cXyz& target, float range=item_target_range, std::span<const ActorId> excluded={}, ActorId* chosen=nullptr) {
    float nearest = range * range;
    bool found = false;
    const cXyz origin = link->current.pos + cXyz(0, 70, 0);
    for (const auto& e : s_enemies) {
        if (!e.initialized || e.credited || e.retiring || std::find(excluded.begin(),excluded.end(),e.id)!=excluded.end()) continue;
        auto* actor = actor_by_id(e.id);
        if (!actor || actor->health <= 0) continue;
        const float distance = (actor->eyePos - origin).abs2();
        if (distance < 1 || distance >= nearest) continue;
        cXyz to = actor->eyePos;
        dBgS_LinChk wall; wall.Set(&origin, &to, link);
        if (dComIfG_Bgsp().LineCross(&wall)) continue;
        target = to; nearest = distance; found = true; if (chosen) *chosen=e.id;
    }
    return found;
}
bool next_boom_target(Attack& a, daAlink_c* link, float range) {
    float nearest = range * range; ActorId chosen = kNone;
    for (const auto& e : s_enemies) {
        if (!e.initialized || e.credited || e.retiring ||
            std::find(a.visited.begin(), a.visited.begin() + a.visitedCount, e.id) != a.visited.begin() + a.visitedCount) continue;
        auto* actor = actor_by_id(e.id);
        if (!actor || actor->health <= 0 ||
            std::find(a.ballVictims.begin(),a.ballVictims.begin()+a.ballVictimCount,e.id)!=a.ballVictims.begin()+a.ballVictimCount) continue;
        const float distance = (actor->eyePos - a.position).abs2();
        if (distance >= nearest) continue;
        cXyz to = actor->eyePos;
        dBgS_LinChk wall; wall.Set(&a.position, &to, link);
        if (dComIfG_Bgsp().LineCross(&wall)) continue;
        chosen = e.id; nearest = distance;
    }
    if (chosen == kNone || a.visitedCount == a.visited.size()) return false;
    a.target = chosen; a.visited[a.visitedCount++] = chosen;
    return true;
}
void update_boom_wind(Attack& a, daAlink_c* link) {
    a.reducedVisual=!full_visual(a.emitters,survivor_perf::Effect::Boomerang,2);
    if(a.reducedVisual)return;
    constexpr std::array<u16, 2> effects{0x1fe, 0x1ff}; // Native Gale Boomerang trails.
    const csXyz angle(0, cM_atan2s(a.velocity.x, a.velocity.z), 0);
    const float size=a.hurricane ? 2.f : 1.f;
    const cXyz scale(size,size,size);
    for (unsigned i = 0; i < effects.size(); ++i) {
        a.emitters[i] = dComIfGp_particle_set(a.emitters[i], effects[i], &a.position, &link->tevStr,
            &angle, &scale, 0xff, nullptr, -1, nullptr, nullptr, nullptr);
    }
}
// Models cannot share deferred draw matrices. Each added projectile owns a
// small, trimmed heap; the initial batch remains in the shared treasure heap.
bool prepare_projectile_model(Attack& a, unsigned type) {
    if (a.model) return true;
    a.modelHeap = mDoExt_createSolidHeapFromGame(0x40000, 0x20);
    if (!a.modelHeap) return false;
    auto* previous = mDoExt_setCurrentHeap(a.modelHeap);
    a.model = mDoExt_J3DModel__create(s_loot.projectileData[type], type == 3 ? 0 : 0x80000,
                                    type == 3 ? 0x13000684 : 0x11000084);
    if (type == 1) a.wind = mDoExt_J3DModel__create(s_loot.windData, 0, 0x13000684);
    mDoExt_setCurrentHeap(previous);
    if (!a.model || (type == 1 && !a.wind)) {
        // Forget any matrices cached during model creation before freeing its heap.
        void* address = nullptr;
        if (svc_hook && svc_hook->resolve &&
            svc_hook->resolve(mod_ctx, "J3DModel::forgetMtx", &address, nullptr) == MOD_OK && address) {
            auto forget = reinterpret_cast<void (*)(J3DModel*)>(address);
            if (a.model) forget(a.model);
            if (a.wind) forget(a.wind);
        }
        mDoExt_destroySolidHeap(a.modelHeap);
        a.modelHeap = nullptr; a.model = a.wind = nullptr;
        svc_log->warn(mod_ctx, "Hyrule Survivors: projectile model allocation failed");
        return false;
    }
    mDoExt_adjustSolidHeap(a.modelHeap);
    return true;
}
// Native combined bomb-arrow model, with its own deferred draw matrices.
bool prepare_bomb_arrow_model(Attack& a) {
    if (a.bombModel) return true;
    auto* data=static_cast<J3DModelData*>(dComIfG_getObjectRes(s_loot.linkArchive,0x1d));
    if (!data) return false;
    a.bombHeap=mDoExt_createSolidHeapFromGame(0x10000,0x20);
    if (!a.bombHeap) return false;
    auto* previous=mDoExt_setCurrentHeap(a.bombHeap);
    a.bombModel=mDoExt_J3DModel__create(data,0x80000,0x11000084);
    mDoExt_setCurrentHeap(previous);
    if (!a.bombModel) { mDoExt_destroySolidHeap(a.bombHeap);a.bombHeap=nullptr;return false; }
    mDoExt_adjustSolidHeap(a.bombHeap);return true;
}
bool launch_item(daAlink_c* link, Upgrade weapon, cXyz target, float spread = 0, unsigned volleySlot = 20, uint64_t volley = 0, ActorId initialTarget = kNone, bool manual = false) {
    const unsigned index = unsigned(weapon), type = index - unsigned(Upgrade::Bow);
    auto& a = acquire_projectile(s_attacks, index, item_projectile_begin(type), item_initial_slots(type));
    if (!prepare_projectile_model(a, type)) return false;
    a.bombArrow=weapon==Upgrade::Bow && s_run.rank(Upgrade::BombArrow);
    if (a.bombArrow && !prepare_bomb_arrow_model(a)) return false;
    a.hurricane=weapon==Upgrade::Boomerang && s_run.rank(Upgrade::Hurricane);
    a.chainSpin=weapon==Upgrade::Ball&&s_run.rank(Upgrade::ChainSpin);
    a.chainSpinTick=0;a.chainSpinRadius=a.chainSpinAngle=0;
    a.detonated=false;
    a.aimPending=manual;a.aimSpread=spread;a.manualFlight=manual;
    a.weapon = index; a.age = 0; a.life = 100;
    a.bowVolleySlot=volleySlot;a.bowVolley=volley;a.arrowTargets=s_run.arrow_targets();
    a.returning = a.exploding = false; a.travel = 0;
    a.target = kNone; a.visitedCount = 0; a.hops = 0; a.ballVictimCount = 0;
    a.position = link->current.pos + cXyz(0, 70, 0);
    // Native daArrow_c::setKeepMatrix attaches the arrow to the left item
    // matrix. Its 180-degree local rotation does not change this origin.
    if (weapon == Upgrade::Bow) mDoMtx_multVecZero(link->getLeftItemMatrix(), &a.position);
    cXyz delta = target - a.position;
    if (weapon == Upgrade::Ball && !manual) {
        delta.y = 0;
        if (delta.abs() < 1.0f)
            delta = cXyz(cM_ssin(link->shape_angle.y), 0, cM_scos(link->shape_angle.y));
    }
    const float distance = std::max(1.0f, delta.abs());
    cXyz direction = delta * (1.0f / distance);
    const cXyz aimed(direction.x * std::cos(spread) + direction.z * std::sin(spread), direction.y,
                     direction.z * std::cos(spread) - direction.x * std::sin(spread));
    a.limit = weapon == Upgrade::Ball ? s_run.ball_range() : std::min(distance + 80.0f, s_run.item_range(weapon));
    a.velocity = aimed * s_run.item_flight_speed(weapon);
    const unsigned rank = s_run.rank(weapon);
    if (weapon == Upgrade::Boomerang) {
        a.hops = s_run.boomerang_hops();
        // Keep enough flight time for the longer first leg, every hop and return.
        a.life = unsigned(std::ceil(s_run.radius(float(120 + 40 * a.hops),weapon)));
        if (initialTarget != kNone) {
            a.target=initialTarget; a.visited[a.visitedCount++]=initialTarget;
        } else if (!manual && !next_boom_target(a, link, s_run.item_range(weapon))) return false;
    }
    if (weapon == Upgrade::Ball) {
        // Reach must not expire the projectile before its outbound/return legs.
        a.life=std::max(a.life,unsigned(std::ceil(a.limit/40.f+a.limit/55.f))+30)+(a.chainSpin?60:0);
    }
    if (weapon == Upgrade::Bow) {
        // Reach extends acquisition and flight, never the arrow's hit radius.
        // Retain the original lifetime unless a longer shot needs more ticks.
        if(manual)a.limit=s_run.item_range(weapon);
        a.life = std::max(a.life, unsigned(std::ceil((manual?a.limit:distance) / 70.0f)) + 1);
    }
    a.radius = weapon == Upgrade::Bow ? 35.0f : s_run.radius(weapon == Upgrade::Boomerang ? 63 : 55 + 8 * rank,weapon);
    if (a.hurricane) a.radius*=2.f;
    if (weapon == Upgrade::Bombs) {
        // Discrete ballistic arc reaches the target in 30 simulation ticks.
        a.velocity = delta * (1.0f / 30); a.velocity.y += 1.4f * 29 / 2;
        a.limit = 30;
    }
    a.status.Init(120, 0xff, link);
    a.collider.Set(kAttackSource); a.collider.SetStts(&a.status);
    // Native Bokoblin damage_check hardcodes death for BOMB and adds 80/140
    // health back for IRON_BALL, after at_power_check. Use the linear sword path
    // for these custom hits (and damage rather than stun-only for Boomerang).
    a.collider.SetAtType(weapon == Upgrade::Bow ? AT_TYPE_ARROW : AT_TYPE_NORMAL_SWORD);
    a.collider.SetAtAtp(static_cast<u8>(s_run.attack_power(index) + (weapon == Upgrade::Ball ? 3 : 0)));
    a.collider.SetAtMtrl(dCcD_MTRL_NONE); // Burn is a separate timed status, not native elemental damage.
    a.collider.OnAtNoConHit(); a.collider.ResetAtHit();
    if (weapon == Upgrade::Bombs) a.collider.OffAtSetBit(); else a.collider.OnAtSetBit();
    a.active = true;
    return true;
}
void explode_item(Attack& a, daAlink_c* link) {
    a.exploding = true; a.life = 60; a.age = 0; a.velocity = cXyz(0, 0, 0);
    a.radius = a.weapon==unsigned(Upgrade::BombArrow) ? s_run.bomb_arrow_radius() : s_run.bomb_radius();
    a.collider.ResetAtHit(); a.collider.OnAtSetBit();
    // Release an old projectile trail before reserving a different effect family.
    stop_visual_emitters(a.emitters);
    a.reducedVisual=!full_visual(a.emitters,survivor_perf::Effect::Explosion,9);
    if(a.reducedVisual)return; // Collider and lifetime above remain unchanged.
    // Complete native normal-bomb burst, including splash, debris and smoke.
    constexpr std::array<u16, 9> effects{0x161, 0x162, 0x163, 0x164, 0x165, 0x166, 0x167, 0x168, 0x1ec};
    const float size = a.radius / 200.0f;
    const cXyz scale(size, size, size);
    for (unsigned i = 0; i < effects.size(); ++i)
        a.emitters[i] = dComIfGp_particle_set(0, effects[i], &a.position, &link->tevStr,
                                            nullptr, &scale, 0xff, nullptr, -1, nullptr, nullptr, nullptr);
}
void detonate_bomb_arrow(Attack& arrow, const cXyz& point) {
    if (!arrow.bombArrow || arrow.detonated) return;
    arrow.detonated=true;
    auto* link=daAlink_getAlinkActorClass();if (!link) return;
    auto& blast=acquire_projectile(s_attacks,unsigned(Upgrade::BombArrow),0,0);
    stop_attack(blast);blast={};blast.active=true;blast.weapon=unsigned(Upgrade::BombArrow);
    blast.position=point;blast.status.Init(120,0xff,link);
    blast.collider.Set(kAttackSource);blast.collider.SetStts(&blast.status);
    blast.collider.SetAtType(AT_TYPE_NORMAL_SWORD);blast.collider.SetAtMtrl(dCcD_MTRL_NONE);
    blast.collider.SetAtAtp(static_cast<u8>(std::max(1u,(unsigned(arrow.collider.GetAtAtp())+1)/2)));
    blast.collider.OnAtNoConHit();explode_item(blast,link);
}
void update_item_attack(Attack& a, daAlink_c* link) {
    const auto weapon = static_cast<Upgrade>(a.weapon);
    if (!a.life--) { stop_attack(a); return; }
    ++a.age;
    if (!a.exploding) {
        const bool shield = a.collider.ChkAtShieldHit();
        // Keep the impact even if the enemy dies or native hit metadata is
        // cleared before this update. A spent arrow must never move again.
        if (weapon == Upgrade::Bow) {
            if (a.ballVictimCount >= a.arrowTargets || shield ||
                (!a.ballVictimCount && a.collider.ChkAtHit())) { stop_attack(a); return; }
            // Each pierced actor is remembered independently of native hit
            // metadata. Reset only contact metadata, never the victim list.
            a.collider.ResetAtHit();
        }
        if (weapon == Upgrade::Boomerang && !a.returning) {
            auto* target = actor_by_id(a.target);
            if (shield) a.returning = true;
            else if ((a.manualFlight ? a.travel>=a.limit : !target || target->health<=0 || (target->eyePos-a.position).abs()<=a.radius) ||
                     (!a.hurricane && a.collider.ChkAtHit())) {
                a.manualFlight=false;
                if (a.hops && next_boom_target(a, link, s_run.boomerang_hop_range())) { --a.hops; a.collider.ResetAtHit(); }
                else { a.returning = true; a.collider.ResetAtHit(); }
            }
            if (!a.returning) {
                target = actor_by_id(a.target);
                if (target) {
                    const cXyz delta = target->eyePos - a.position;
                    const float distance = std::max(1.0f, delta.abs());
                    a.velocity = delta * (std::min(s_run.item_flight_speed(weapon), distance) / distance);
                }
            }
        }
        if (weapon==Upgrade::Ball && a.travel>=a.limit && !a.returning) {
            a.returning=true;a.collider.ResetAtHit();a.ballVictimCount=0;
            a.status.Init(120,0xff,link);
            if(a.chainSpin) {
                const auto delta=a.position-link->current.pos;
                a.chainSpinRadius=std::sqrt(delta.x*delta.x+delta.z*delta.z);
                a.chainSpinAngle=std::atan2(delta.x,delta.z);
            }
        }
        const bool spinning=weapon==Upgrade::Ball && a.returning && a.chainSpin;
        if (weapon==Upgrade::Boomerang && a.hurricane) a.collider.ResetAtHit();
        if (spinning) {
            if(a.chainSpinTick>=60) { stop_attack(a); return; }
            // Retract throughout one full revolution, following Link's current
            // X/Z. Model, chain and collider share this inward spiral.
            const unsigned remaining=60-a.chainSpinTick;
            const float progress=float(++a.chainSpinTick)/60.f;
            const float angle=a.chainSpinAngle+6.28318530718f*progress;
            const float radius=a.chainSpinRadius*(1.f-progress);
            const float y=a.manualFlight?a.position.y+(link->current.pos.y+70.f-a.position.y)/float(remaining):a.position.y;
            const cXyz next(link->current.pos.x+std::sin(angle)*radius,
                y,link->current.pos.z+std::cos(angle)*radius);
            a.velocity=next-a.position;
        } else if (a.returning) {
            cXyz delta = link->current.pos + cXyz(0, 70, 0) - a.position;
            if (weapon == Upgrade::Ball && !a.manualFlight) delta.y = 0; // Keep the launch height on both passes.
            const float distance = delta.abs();
            if (distance < 65) { stop_attack(a); return; }
            a.velocity = delta * (s_run.item_flight_speed(weapon,true) / distance);
        }
        const cXyz next = a.position + a.velocity;
        dBgS_LinChk wall; wall.Set(&a.position, &next, link);
        if (dComIfG_Bgsp().LineCross(&wall)) {
            if (weapon == Upgrade::Bombs) explode_item(a, link);
            else { stop_attack(a); return; } // Cannot hit through walls, even on return.
        } else {
            a.position = next; if(!spinning)a.travel += a.velocity.abs();
            if (weapon == Upgrade::Bombs) {
                a.velocity.y -= 1.4f;
                if (a.age >= 30) explode_item(a, link);
            }
        }
    }
    if (weapon == Upgrade::Boomerang) update_boom_wind(a, link);
    if (weapon == Upgrade::Bow && s_run.rank(Upgrade::FireArrows)) {
        if (const u16 effect = bow_fire_effect())
            if(full_visual(a.emitters,survivor_perf::Effect::Other,1))
                a.emitters[0] = dComIfGp_particle_set(a.emitters[0], effect, &a.position, nullptr,
                nullptr, nullptr, 0xff, nullptr, -1, nullptr, nullptr, nullptr);
    }
    if (a.exploding && a.age > 12) { a.collider.OffAtSetBit(); return; }
    if (weapon != Upgrade::Bombs || a.exploding) {
        a.collider.SetC(a.position); a.collider.SetR(a.radius); a.collider.SetAtVec(a.velocity);
        dComIfG_Ccsp()->Set(&a.collider);
    }
}
void fire_items(daAlink_c* link) {
    if (!s_loot.ready) return;
    for (unsigned i = unsigned(Upgrade::Bow); i <= unsigned(Upgrade::Ball); ++i) {
        if (!s_run.ranks[i] || s_cooldowns[i]) continue;
        const auto weapon = static_cast<Upgrade>(i);
        queue_ranged(link, weapon);
    }
}
bool spawn_chest(daAlink_c* link) {
    if (!chest_spawning_allowed()) return false;
    unsigned index;
    if (!find_chest_spawn_slot(0,kPaidChestCount,index)) return false;
    auto* slot=&s_chests[index];
    for (unsigned attempt = 0; attempt < 12; ++attempt) {
        const auto yaw = static_cast<s16>(s_run.next_random());
        const float radius = 350 + float(s_run.next_random() % 600);
        cXyz pos = link->current.pos + cXyz(cM_ssin(yaw) * radius, 0, cM_scos(yaw) * radius);
        cXyz probe = pos + cXyz(0, 300, 0);
        dBgS_ObjGndChk ground; ground.SetPos(&probe);
        const float floor = dComIfG_Bgsp().GroundCross(&ground);
        if (!std::isfinite(floor) || floor < -1.0e8f || std::abs(floor - link->current.pos.y) > 100) continue;
        pos.y = floor;
        bool clear = true;
        for (const auto& c : s_chests) if (&c!=slot && c.active && c.position.abs(pos) < 250) clear = false;
        // Require a reachable center and a flat, unobstructed chest footprint.
        cXyz from = link->current.pos + cXyz(0, 70, 0), to = pos + cXyz(0, 70, 0);
        dBgS_LinChk wall; wall.Set(&from, &to, link);
        if (dComIfG_Bgsp().LineCross(&wall) || !arena_path_clear(from,to,85)) continue;
        if (dComIfG_Bgsp().GetRoomId(ground) != arena().room) continue;
        for (const cXyz offset : {cXyz(-80,0,0), cXyz(80,0,0), cXyz(0,0,-80), cXyz(0,0,80)}) {
            cXyz edge = to + offset; wall.Set(&to, &edge, link);
            if (dComIfG_Bgsp().LineCross(&wall)) clear = false;
            edge.y += 100; ground.SetPos(&edge);
            const float corner = dComIfG_Bgsp().GroundCross(&ground);
            if (!std::isfinite(corner) || std::abs(corner - floor) > 20) clear = false;
        }
        if (!clear || !walkable_spawn_path(link->current.pos,pos,85) || !reachable_arena_floor(pos)) continue;
        // Keep the old chest intact until a replacement position is validated.
        *slot = {}; slot->active = true; slot->position = pos; slot->yaw = yaw + 0x8000;
        slot->spawnOrder=++s_chestSequence;
        return true;
    }
    return false;
}
// Separate, one-use map chests never consume the six replenishing slots.
void seed_map_chests(daAlink_c* link) {
    if (!chest_spawning_allowed()) return;
    if (!s_arenaMeshReady || !arena_distribution_ready()) return;
    const unsigned budget=map_chest_budget(s_map);
    if (!budget) return;
    // Exploration chests sample the whole room, independently of Link's view
    // and straight-line walking path. random_arena_floor checks dry native
    // ground, room membership, exits, walls and the chest footprint locally,
    // plus a verified walking route from the entrance on every arena map.
    // A direct path from Link rejects safe sites behind hills/walls/rivers and
    // concentrates the entire map budget near the entrance.
    const float spacing=std::clamp(std::sqrt(arena_distribution_area()/float(budget))*0.5f,250.f,2500.f);
    unsigned attempts=0;
    for (unsigned n=0;n<budget;++n) {
        if (s_mapChestPlaced[n]) continue;
        cXyz pos; bool found=false;
        while (!found && attempts<24) {
            ++attempts;
            if (!random_arena_floor(pos) || pos.absXZ(link->current.pos)<spacing) continue;
            found=std::none_of(s_chests.begin(),s_chests.end(),[&](const Chest& c) {
                return c.active && c.position.absXZ(pos)<(c.fixed ? spacing : 250.f);
            });
        }
        if (!found) return; // Retry missing safe positions on later frames.
        auto& chest=s_chests[kPaidChestCount+n];
        chest={}; chest.active=true; chest.fixed=true;
        chest.position=pos; chest.yaw=s16(s_run.next_random());
        s_mapChestPlaced[n]=true;
    }
}
void update_chests(daAlink_c* link) {
    if (s_run.phase != Phase::Running || !chest_spawning_allowed() || !prepare_loot(link)) return;
    seed_map_chests(link);
    if (s_chestDelay) { --s_chestDelay; return; }
    if (!s_run.chest_rewards_left(potion_slot() >= 0)) return;
    s_chestDelay = spawn_chest(link) ? (25 + s_run.next_random() % 21) * ticks_per_second : 3 * ticks_per_second;
}
void draw_loot_model(J3DModel* model, cXyz pos, s16 yaw, s16 pitch, float scale) {
    if (!model) return;
    mDoMtx_stack_c::transS(pos); mDoMtx_stack_c::YrotM(yaw); mDoMtx_stack_c::XrotM(pitch);
    model->setBaseTRMtx(mDoMtx_stack_c::get());
    const cXyz size(scale, scale, scale); model->setBaseScale(size);
    dKy_tevstr_c tev{}; g_env_light.settingTevStruct(0, &pos, &tev);
    g_env_light.setLightTevColorType_MAJI(model, &tev);
    mDoExt_modelUpdateDL(model);
}
#include "survivors_chain.inc"
void draw_loot() {
    if (!s_loot.ready) return;
    // This runs inside Link's draw dispatcher, but chests and flying projectiles
    // are world objects. Held weapons are submitted separately by draw_ranged.
    const dark_link::WorldModelDrawScope worldModels;
    for (unsigned i = 0; i < s_chests.size(); ++i) {
        const auto& c = s_chests[i]; if (!c.active) continue;
        auto* data = s_loot.chests[i]->getModelData();
        auto* root = data->getJointNodePointer(0); auto* previous = root->getMtxCalc();
        s_loot.lids[i]->entry(data, c.opening ? std::min(float(c.age), s_loot.lids[i]->getEndFrame()) : 0);
        draw_loot_model(s_loot.chests[i], c.position, c.yaw, 0, 1);
        root->setMtxCalc(previous); // Shared native chest data must not retain our animation pointer.
    }
    auto* link = daAlink_getAlinkActorClass();
    unsigned chainBudget=survivor_perf::Pressure{survivor_perf::tier(),0}.chain_total();
    for (unsigned i = kItemBegin; i < s_attacks.size(); ++i) {
        const auto& a = s_attacks[i]; if (!a.active || a.exploding || a.weapon < unsigned(Upgrade::Bow) || a.weapon > unsigned(Upgrade::Ball)) continue;
        const auto weapon = static_cast<Upgrade>(a.weapon);
        const auto yaw = weapon == Upgrade::Boomerang ? s16(a.age * 7000) : cM_atan2s(a.velocity.x, a.velocity.z);
        const auto pitch = weapon == Upgrade::Bow ? s16(-cM_atan2s(a.velocity.y, a.velocity.absXZ())) : s16(0);
        draw_loot_model(a.bombArrow ? a.bombModel : a.model, a.position, yaw, pitch,
            weapon == Upgrade::Bow ? 1 : s_run.radius(weapon == Upgrade::Boomerang ? (a.hurricane ? 2.f : 1.f) : 1 + 0.08f * (s_run.rank(weapon) - 1),weapon));
        if (weapon == Upgrade::Boomerang && s_loot.windAnim && !a.reducedVisual) {
            auto* model = a.wind;
            auto* data = model->getModelData(); auto* root = data->getJointNodePointer(0);
            auto* joint = data->getJointNodePointer(4);
            auto* oldCalc = root->getMtxCalc(); auto oldCallback = joint->getCallBack();
            // Native callback casts userArea to daBoomerang_c; our model isn't an actor.
            joint->setCallBack(nullptr);
            s_loot.windAnim->entry(data, std::fmod(float(a.age), std::max(1.0f, s_loot.windAnim->getEndFrame())));
            draw_loot_model(model, a.position, 0, 0, a.hurricane ? 2.f : 1.f);
            root->setMtxCalc(oldCalc); joint->setCallBack(oldCallback);
        }
        if (weapon == Upgrade::Ball && link) draw_chain(i, link, a.position,chainBudget);
    }
}
