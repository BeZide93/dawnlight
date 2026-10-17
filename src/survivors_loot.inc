// Run-local treasure and item projectiles. Included inside Survivors' private
// namespace; native assets are borrowed/refcounted, never added to the bundle.
constexpr unsigned kChestCount = 2, kItemBegin = 8, kSlotsPerItem = 4;
struct Chest {
    bool active = false, opening = false;
    cXyz position{};
    s16 yaw = 0;
    unsigned age = 0;
    ChestReward reward = ChestReward::Count;
};
std::array<Chest, kChestCount> s_chests;
unsigned s_chestDelay = 20 * ticks_per_second, s_selectedChest = kChestCount;
struct LootModels {
    JKRSolidHeap* heap = nullptr;
    J3DModel* masterSword = nullptr;
    J3DModel* bow = nullptr;
    J3DModel* heldBoomerang = nullptr;
    std::array<J3DModel*, kSlotsPerItem> wind{};
    std::array<mDoExt_bckAnm*, 2> bowAnims{};
    mDoExt_bckAnm* windAnim = nullptr;
    request_of_phase_process_class chestPhase{}, linkPhase{};
    const char* linkArchive = nullptr;
    std::array<J3DModel*, projectile_capacity> projectiles{};
    std::array<J3DModel*, kChestCount> chests{};
    std::array<mDoExt_bckAnm*, kChestCount> lids{};
    bool ready = false, failed = false;
} s_loot;

void clear_loot() {
    cancel_treasure_demo();
    // Some engines cache model matrices outside the owning heap.
    using ForgetModel = void (*)(J3DModel*);
    void* address = nullptr;
    if (s_loot.heap && svc_hook && svc_hook->resolve &&
        svc_hook->resolve(mod_ctx, "J3DModel::forgetMtx", &address, nullptr) == MOD_OK && address) {
        auto forget = reinterpret_cast<ForgetModel>(address);
        for (auto* model : s_loot.projectiles) if (model) forget(model);
        for (auto* model : s_loot.chests) if (model) forget(model);
        if (s_loot.masterSword) forget(s_loot.masterSword);
        if (s_loot.bow) forget(s_loot.bow);
        if (s_loot.heldBoomerang) forget(s_loot.heldBoomerang);
        for (auto* model : s_loot.wind) if (model) forget(model);
    }
    if (s_loot.heap) mDoExt_destroySolidHeap(s_loot.heap);
    dComIfG_resDelete(&s_loot.chestPhase, "Dalways");
    if (s_loot.linkArchive) dComIfG_resDelete(&s_loot.linkPhase, s_loot.linkArchive);
    s_loot = {}; s_chests = {}; s_selectedChest = kChestCount;
    s_chestDelay = 20 * ticks_per_second;
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
    s_loot.heap = mDoExt_createSolidHeapFromGame(0x200000, 0x20);
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
    for (auto*& wind : s_loot.wind)
        wind = model(static_cast<J3DModelData*>(dComIfG_getObjectRes(s_loot.linkArchive, 0x34)), true);
    auto* chestData = static_cast<J3DModelData*>(dComIfG_getObjectRes("Dalways", 12));
    auto* lidData = static_cast<J3DAnmTransform*>(dComIfG_getObjectRes("Dalways", 7));
    // Master Sword has only a rupee placeholder in native item_resource.
    // Give its presentation a private model instance, borrowing Link's data.
    s_loot.masterSword = model(link->mpSwMModel ? link->mpSwMModel->getModelData() : nullptr);
    bool ready = chestData && lidData && s_loot.masterSword && s_loot.bow && s_loot.heldBoomerang &&
        s_loot.bowAnims[0] && s_loot.bowAnims[1] && s_loot.windAnim;
    for (auto* wind : s_loot.wind) ready = ready && wind;
    for (unsigned i = 0; i < kChestCount; ++i) {
        s_loot.chests[i] = model(chestData);
        s_loot.lids[i] = JKR_NEW mDoExt_bckAnm();
        ready = ready && s_loot.chests[i] && s_loot.lids[i] &&
            s_loot.lids[i]->init(lidData, TRUE, J3DFrameCtrl::EMode_NONE, 0, 0, -1, false);
    }
    // Read the ball into this heap, not Link's equipment heap. loadAramBmd
    // allocates its own resource buffer and does not equip/change Link's item.
    auto* ball = dComIfGp_getAnmArchive() ? link->loadAramBmd(dRes_ID_ALANM_BMD_AL_IB_e, 0x2800) : nullptr;
    const std::array<J3DModelData*, 4> data{
        static_cast<J3DModelData*>(dComIfG_getObjectRes(s_loot.linkArchive, 0x1c)),
        static_cast<J3DModelData*>(dComIfG_getObjectRes(s_loot.linkArchive, 0x1f)),
        static_cast<J3DModelData*>(dComIfG_getObjectRes(s_loot.linkArchive, 0x1e)), ball};
    for (unsigned type = 0; type < data.size(); ++type)
        for (unsigned slot = 0; slot < kSlotsPerItem; ++slot) {
            auto*& instance = s_loot.projectiles[kItemBegin + type * kSlotsPerItem + slot];
            instance = model(data[type], type == 3);
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
bool nearest_item_target(daAlink_c* link, cXyz& target) {
    float nearest = 1400.0f * 1400.0f;
    bool found = false;
    const cXyz origin = link->current.pos + cXyz(0, 70, 0);
    for (const auto& e : s_enemies) {
        if (!e.initialized || e.credited || e.retiring) continue;
        auto* actor = actor_by_id(e.id);
        if (!actor || actor->health <= 0) continue;
        const float distance = (actor->eyePos - origin).abs2();
        if (distance < 1 || distance >= nearest) continue;
        cXyz to = actor->eyePos;
        dBgS_LinChk wall; wall.Set(&origin, &to, link);
        if (dComIfG_Bgsp().LineCross(&wall)) continue;
        target = to; nearest = distance; found = true;
    }
    return found;
}
bool next_boom_target(Attack& a, daAlink_c* link, float range) {
    float nearest = range * range; ActorId chosen = kNone;
    for (const auto& e : s_enemies) {
        if (!e.initialized || e.credited || e.retiring ||
            std::find(a.visited.begin(), a.visited.begin() + a.visitedCount, e.id) != a.visited.begin() + a.visitedCount) continue;
        auto* actor = actor_by_id(e.id);
        if (!actor || actor->health <= 0) continue;
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
    constexpr std::array<u16, 2> effects{0x1fe, 0x1ff}; // Native Gale Boomerang trails.
    const csXyz angle(0, cM_atan2s(a.velocity.x, a.velocity.z), 0);
    for (unsigned i = 0; i < effects.size(); ++i) {
        a.emitters[i] = dComIfGp_particle_set(a.emitters[i], effects[i], &a.position, &link->tevStr,
            &angle, nullptr, 0xff, nullptr, -1, nullptr, nullptr, nullptr);
    }
}
bool launch_item(daAlink_c* link, Upgrade weapon, cXyz target, float spread = 0) {
    const unsigned index = unsigned(weapon), type = index - unsigned(Upgrade::Bow);
    auto begin = s_attacks.begin() + kItemBegin + type * kSlotsPerItem;
    auto end = begin + kSlotsPerItem;
    auto slot = std::find_if(begin, end, [](const auto& a) { return !a.active; });
    if (slot == end) return false;
    auto& a = *slot;
    a.weapon = index; a.age = 0; a.life = 100;
    a.returning = a.exploding = false; a.travel = 0;
    a.target = kNone; a.visitedCount = 0; a.hops = 0;
    a.position = link->current.pos + cXyz(0, 70, 0);
    cXyz delta = target - a.position;
    const float distance = std::max(1.0f, delta.abs());
    cXyz direction = delta * (1.0f / distance);
    const cXyz aimed(direction.x * std::cos(spread) + direction.z * std::sin(spread), direction.y,
                     direction.z * std::cos(spread) - direction.x * std::sin(spread));
    a.limit = std::min(distance + 80.0f, weapon == Upgrade::Ball ? 900.0f : 1400.0f);
    a.velocity = aimed * (weapon == Upgrade::Bow ? 70.0f : weapon == Upgrade::Ball ? 40.0f : 42.0f);
    const unsigned rank = s_run.rank(weapon);
    if (weapon == Upgrade::Boomerang) {
        a.hops = rank; a.life = 120 + 40 * rank;
        if (!next_boom_target(a, link, 1400)) return false;
    }
    a.radius = s_run.radius(weapon == Upgrade::Bow ? 35 : 55 + 8 * rank);
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
    a.collider.SetAtMtrl(dCcD_MTRL_NONE);
    a.collider.OnAtNoConHit(); a.collider.ResetAtHit();
    if (weapon == Upgrade::Bombs) a.collider.OffAtSetBit(); else a.collider.OnAtSetBit();
    a.active = true;
    return true;
}
void explode_item(Attack& a, daAlink_c* link) {
    a.exploding = true; a.life = 12; a.velocity = cXyz(0, 0, 0);
    a.radius = s_run.radius(170 + 30 * s_run.rank(Upgrade::Bombs));
    a.collider.ResetAtHit(); a.collider.OnAtSetBit();
    // Native bomb flash/smoke. Emitters are tracked and removed with the attack.
    constexpr std::array<u16, 2> effects{0x161, 0x162};
    const float size = a.radius / 200.0f;
    const cXyz scale(size, size, size);
    for (unsigned i = 0; i < effects.size(); ++i)
        a.emitters[i] = dComIfGp_particle_set(0, effects[i], &a.position, &link->tevStr,
                                            nullptr, &scale, 0xff, nullptr, -1, nullptr, nullptr, nullptr);
}
void update_item_attack(Attack& a, daAlink_c* link) {
    const auto weapon = static_cast<Upgrade>(a.weapon);
    if (!a.life--) { stop_attack(a); return; }
    ++a.age;
    if (!a.exploding) {
        const bool shield = a.collider.ChkAtShieldHit();
        if (weapon == Upgrade::Bow && (shield || a.collider.ChkAtHit())) { stop_attack(a); return; }
        if (weapon == Upgrade::Boomerang && !a.returning) {
            auto* target = actor_by_id(a.target);
            if (shield) a.returning = true;
            else if (!target || target->health <= 0 || (target->eyePos - a.position).abs() <= a.radius || a.collider.ChkAtHit()) {
                if (a.hops && next_boom_target(a, link, 800)) { --a.hops; a.collider.ResetAtHit(); }
                else { a.returning = true; a.collider.ResetAtHit(); }
            }
            if (!a.returning) {
                target = actor_by_id(a.target);
                if (target) {
                    const cXyz delta = target->eyePos - a.position;
                    const float distance = std::max(1.0f, delta.abs());
                    a.velocity = delta * (std::min(42.0f, distance) / distance);
                }
            }
        }
        if (weapon == Upgrade::Ball &&
            (shield || a.travel >= a.limit) && !a.returning) {
            a.returning = true; a.collider.ResetAtHit();
        }
        if (a.returning) {
            const cXyz delta = link->current.pos + cXyz(0, 70, 0) - a.position;
            const float distance = delta.abs();
            if (distance < 65) { stop_attack(a); return; }
            a.velocity = delta * (55.0f / distance);
        }
        const cXyz next = a.position + a.velocity;
        dBgS_LinChk wall; wall.Set(&a.position, &next, link);
        if (dComIfG_Bgsp().LineCross(&wall)) {
            if (weapon == Upgrade::Bombs) explode_item(a, link);
            else { stop_attack(a); return; } // Cannot hit through walls, even on return.
        } else {
            a.position = next; a.travel += a.velocity.abs();
            if (weapon == Upgrade::Bombs) {
                a.velocity.y -= 1.4f;
                if (a.age >= 30) explode_item(a, link);
            }
        }
    }
    if (weapon == Upgrade::Boomerang) update_boom_wind(a, link);
    if (weapon != Upgrade::Bombs || a.exploding) {
        a.collider.SetC(a.position); a.collider.SetR(a.radius); a.collider.SetAtVec(a.velocity);
        dComIfG_Ccsp()->Set(&a.collider);
    }
}
void fire_items(daAlink_c* link) {
    if (!s_loot.ready) return;
    cXyz target;
    if (!nearest_item_target(link, target)) return;
    for (unsigned i = unsigned(Upgrade::Bow); i <= unsigned(Upgrade::Ball); ++i) {
        if (!s_run.ranks[i] || s_cooldowns[i]) continue;
        const auto weapon = static_cast<Upgrade>(i);
        if (weapon == Upgrade::Bow || weapon == Upgrade::Boomerang) {
            queue_ranged(link, weapon); continue;
        }
        if (!launch_item(link, weapon, target)) continue;
        s_cooldowns[i] = s_run.item_cooldown(weapon);

    }
}
bool spawn_chest(daAlink_c* link) {
    auto slot = std::find_if(s_chests.begin(), s_chests.end(), [](const auto& c) { return !c.active; });
    if (slot == s_chests.end()) return false;
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
        for (const auto& c : s_chests) if (c.active && c.position.abs(pos) < 250) clear = false;
        // Require a reachable center and a flat, unobstructed chest footprint.
        cXyz from = link->current.pos + cXyz(0, 70, 0), to = pos + cXyz(0, 70, 0);
        dBgS_LinChk wall; wall.Set(&from, &to, link);
        if (dComIfG_Bgsp().LineCross(&wall)) continue;
        for (const cXyz offset : {cXyz(-80,0,0), cXyz(80,0,0), cXyz(0,0,-80), cXyz(0,0,80)}) {
            cXyz edge = to + offset; wall.Set(&to, &edge, link);
            if (dComIfG_Bgsp().LineCross(&wall)) clear = false;
            edge.y += 100; ground.SetPos(&edge);
            const float corner = dComIfG_Bgsp().GroundCross(&ground);
            if (!std::isfinite(corner) || std::abs(corner - floor) > 20) clear = false;
        }
        if (!clear) continue;
        *slot = {}; slot->active = true; slot->position = pos; slot->yaw = yaw + 0x8000;
        return true;
    }
    return false;
}
void update_chests(daAlink_c* link) {
    if (s_run.phase != Phase::Running || !prepare_loot(link)) return;
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
void draw_loot() {
    if (!s_loot.ready) return;
    for (unsigned i = 0; i < s_chests.size(); ++i) {
        const auto& c = s_chests[i]; if (!c.active) continue;
        auto* data = s_loot.chests[i]->getModelData();
        auto* root = data->getJointNodePointer(0); auto* previous = root->getMtxCalc();
        s_loot.lids[i]->entry(data, c.opening ? std::min(float(c.age), s_loot.lids[i]->getEndFrame()) : 0);
        draw_loot_model(s_loot.chests[i], c.position, c.yaw, 0, 1);
        root->setMtxCalc(previous); // Shared native chest data must not retain our animation pointer.
    }
    auto* link = daAlink_getAlinkActorClass();
    for (unsigned i = kItemBegin; i < s_attacks.size(); ++i) {
        const auto& a = s_attacks[i]; if (!a.active || a.exploding) continue;
        const auto weapon = static_cast<Upgrade>(a.weapon);
        const auto yaw = weapon == Upgrade::Boomerang ? s16(a.age * 7000) : cM_atan2s(a.velocity.x, a.velocity.z);
        const auto pitch = weapon == Upgrade::Bow ? s16(-cM_atan2s(a.velocity.y, a.velocity.absXZ())) : s16(0);
        draw_loot_model(s_loot.projectiles[i], a.position, yaw, pitch,
            weapon == Upgrade::Bow ? 1 : s_run.radius(1 + 0.08f * (s_run.rank(weapon) - 1)));
        if (weapon == Upgrade::Boomerang && s_loot.windAnim) {
            auto* model = s_loot.wind[i - kItemBegin - kSlotsPerItem];
            auto* data = model->getModelData(); auto* root = data->getJointNodePointer(0);
            auto* joint = data->getJointNodePointer(4);
            auto* oldCalc = root->getMtxCalc(); auto oldCallback = joint->getCallBack();
            // Native callback casts userArea to daBoomerang_c; our model isn't an actor.
            joint->setCallBack(nullptr);
            s_loot.windAnim->entry(data, std::fmod(float(a.age), std::max(1.0f, s_loot.windAnim->getEndFrame())));
            draw_loot_model(model, a.position, 0, 0, 1);
            root->setMtxCalc(oldCalc); joint->setCallBack(oldCallback);
        }
        if (weapon == Upgrade::Ball && link) {
            cXyz from = link->current.pos + cXyz(0, 60, 0), to = a.position;
            dDbVw_drawLineXlu(from, to, GXColor{170,170,180,255}, 1, 5);
        }
    }
}
