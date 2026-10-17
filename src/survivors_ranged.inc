// Native upper-body animations with run-owned held models/projectiles. No
// native item actor, inventory assignment or ammo consumption is synthesized.
struct RangedAction {
    bool active = false, released = false;
    Upgrade weapon = Upgrade::Bow;
    u16 animation = 0;
    cXyz target{};
} s_ranged;
unsigned s_rangedRest = 0;
Upgrade s_lastRanged = Upgrade::Boomerang;

void cancel_ranged() {
    auto* link = daAlink_getAlinkActorClass();
    if (s_ranged.active && link && fopAcM_GetID(link) == s_player &&
        link->checkUpperAnime(s_ranged.animation)) {
        link->resetUpperAnime(daAlink_c::UPPER_2, 3);
        link->mBodyAngle.x = link->mBodyAngle.y = 0;
    }
    s_rangedRest = s_ranged.active ? 6 : 0;
    s_ranged = {};
}
void ranged_anime(daAlink_c* link, u16 animation, unsigned ticks) {
    s_ranged.animation = animation;
    link->setUpperAnimeBaseSpeed(animation, 1, 2);
    auto& frame = link->mUpperFrameCtrl[2];
    frame.setRate(std::max(1.0f, float(frame.getEnd()) / std::max(3u, ticks)));
}
bool queue_ranged(daAlink_c* link, Upgrade weapon) {
    if (s_ranged.active || s_rangedRest || !s_loot.ready || !playable() || link->checkEquipAnime() ||
        (link->mEquipItem != dItemNo_NONE_e && link->mEquipItem != 0x103)) return false;
    const auto other = weapon == Upgrade::Bow ? Upgrade::Boomerang : Upgrade::Bow;
    if (weapon == s_lastRanged && s_run.rank(other) && !s_cooldowns[unsigned(other)]) return false;
    const bool recover = normal_sword_proc(link) && link->mUnderFrameCtrl[0].getFrame() >
        std::max(link->field_0x347c, link->field_0x3484);
    if (!sword_action_available(link) && !recover) return false;
    cXyz target;
    if (!nearest_item_target(link, target)) return false;
    if (recover) link->procWaitInit();
    link->deleteEquipItem(FALSE, FALSE); // Sheathe the sword while the held item is visible.
    s_swordAction = SwordAction::None; s_wavePending = false;
    // Resume the sword combo after this automatic item animation.
    s_lastRanged = weapon;
    s_ranged.active = true; s_ranged.weapon = weapon; s_ranged.target = target;
    const unsigned interval = s_run.item_cooldown(weapon);
    s_cooldowns[unsigned(weapon)] = interval;
    ranged_anime(link, weapon == Upgrade::Bow ? dRes_ID_ALANM_BCK_ARELORD_e : dRes_ID_ALANM_BCK_BOOMTHROW_e,
                 interval / (weapon == Upgrade::Bow ? 3 : 2));
    return true;
}
void release_ranged(daAlink_c* link) {
    cXyz target;
    if (!nearest_item_target(link, target)) return;
    s_ranged.target = target;
    if (!launch_item(link, s_ranged.weapon, target)) return;
    if (s_ranged.weapon == Upgrade::Bow && s_run.rank(Upgrade::Bow) >= 3) {
        launch_item(link, Upgrade::Bow, target, 0.14f);
        if (s_run.rank(Upgrade::Bow) >= 5) launch_item(link, Upgrade::Bow, target, -0.14f);
    }
}
void update_ranged(daAlink_c* link) {
    if (s_rangedRest) --s_rangedRest;
    if (!s_ranged.active) return;
    if (!sword_action_available(link) || !link->checkUpperAnime(s_ranged.animation)) {
        cancel_ranged(); return; // Damage, drinking, jumping or another native action wins.
    }
    auto& frame = link->mUpperFrameCtrl[2];
    if (s_ranged.weapon == Upgrade::Bow && !s_ranged.released) {
        if (!link->checkAnmEnd(&frame)) return;
        ranged_anime(link, dRes_ID_ALANM_BCK_ASHOOT_e, s_run.item_cooldown(Upgrade::Bow) / 3);
        s_ranged.released = true; release_ranged(link);
    } else {
        // Native boomerang leaves the hand at frame 5.5; >= also handles fast playback.
        if (!s_ranged.released && frame.getFrame() >= 5.5f) {
            s_ranged.released = true; release_ranged(link);
        }
        if (link->checkAnmEnd(&frame)) cancel_ranged();
    }
}
HookAction before_ranged_native(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!s_ranged.active || !in_arena() || !link || fopAcM_GetID(link) != s_player ||
        !link->checkUpperAnime(s_ranged.animation)) return HOOK_CONTINUE;
    *static_cast<int*>(result) = 0;
    return HOOK_SKIP_ORIGINAL; // No native arrow/ammo/aim-mode side effects for our upper animation.
}
HookAction before_ranged_aim(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!s_ranged.active || !link || fopAcM_GetID(link) != s_player || !playable() ||
        !sword_action_available(link) || !link->checkUpperAnime(s_ranged.animation)) return HOOK_CONTINUE;
    if (!s_ranged.released) nearest_item_target(link, s_ranged.target);
    const cXyz delta = s_ranged.target - link->current.pos - cXyz(0, 70, 0);
    link->shape_angle.y = cM_atan2s(delta.x, delta.z);
    link->mBodyAngle.y = 0;
    link->mBodyAngle.x = -cM_atan2s(delta.y, std::max(1.0f, delta.absXZ()));
    // current.angle controls travel; changing only shape/body preserves movement.
    return HOOK_CONTINUE;
}
void draw_ranged() {
    auto* link = daAlink_getAlinkActorClass();
    if (!s_ranged.active || !s_loot.ready || !link || fopAcM_GetID(link) != s_player ||
        !link->checkUpperAnime(s_ranged.animation)) return;
    const bool bow = s_ranged.weapon == Upgrade::Bow;
    if (!bow && s_ranged.released) return;
    auto* model = bow ? s_loot.bow : s_loot.heldBoomerang;
    mDoMtx_stack_c::copy(bow ? link->getRightItemMatrix() : link->getLeftItemMatrix());
    if (!bow) { // Native daBoomerang_c::setKeepMatrix hand attachment.
        mDoMtx_stack_c::transM(32, -5, -6);
        mDoMtx_stack_c::XYZrotM(cM_deg2s(-4), cM_deg2s(39), cM_deg2s(-9));
    }
    model->setBaseTRMtx(mDoMtx_stack_c::get());
    auto* data = model->getModelData(); auto* root = data->getJointNodePointer(0);
    auto* previous = root->getMtxCalc();
    if (bow) {
        auto* animation = s_loot.bowAnims[s_ranged.released ? 1 : 0];
        animation->entry(data, std::min(link->mUpperFrameCtrl[2].getFrame(), animation->getEndFrame()));
    }
    dKy_tevstr_c tev{}; g_env_light.settingTevStruct(0, &link->current.pos, &tev);
    g_env_light.setLightTevColorType_MAJI(model, &tev);
    mDoExt_modelUpdateDL(model);
    root->setMtxCalc(previous);
}
