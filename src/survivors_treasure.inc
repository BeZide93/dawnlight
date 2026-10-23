u8 shield_item(const Run& run) {
    constexpr std::array<u8, 3> items{dItemNo_WOOD_SHIELD_e, dItemNo_SHIELD_e, dItemNo_HYLIA_SHIELD_e};
    return items[run.shield_tier()];
}
u8 treasure_item(ChestReward reward, const Run& run) {
    const std::array<u8, unsigned(ChestReward::Count)> items{dItemNo_BOW_e, dItemNo_BOOMERANG_e, dItemNo_BOMB_5_e,
        dItemNo_IRONBALL_e, u8(run.master_sword >= 5 ? dItemNo_LIGHT_SWORD_e : dItemNo_MASTER_SWORD_e),
        dItemNo_RED_BOTTLE_e, dItemNo_SWORD_e, shield_item(run), dItemNo_POU_SPIRIT_e};
    return items[unsigned(reward)];
}
// Native DEFAULT_GETITEM presentation, following the Glider reward's actor
// ownership protocol. The prop is presentation-only; completion explicitly
// commits the run reward and, for swords/bottles, the isolated save inventory.
struct TreasureDemo {
    ActorId item = kNone;
    bool started = false, poseStarted = false, retiring = false, key = false;
    unsigned waitFrames = 0;
    Run result;
    unsigned price = 0, bonusRupees = 0, potions = 0;
} s_treasure;
ChestReward s_chosenTreasure = ChestReward::Count;
void dismiss_treasure_choice() {
    if(s_run.starting_item_pending)return; // Closing the UI cannot skip or reroll the starting reward.
    s_chosenTreasure = ChestReward::Count; s_selectedChest = kChestCount;
    if (s_run.phase == Phase::TreasureChoice) s_run.phase = Phase::Running;
}

// Add a bottle before refilling any existing empty one. Never overwrite a
// filled bottle, and never offer an unusable potion when all four are full.
int potion_slot() {
    for (int slot = SLOT_11; slot <= SLOT_14; ++slot)
        if (dComIfGs_getItem(slot, true) == dItemNo_NONE_e) return slot;
    for (int slot = SLOT_11; slot <= SLOT_14; ++slot)
        if (dComIfGs_getItem(slot, true) == dItemNo_EMPTY_BOTTLE_e) return slot;
    return -1;
}
unsigned potion_capacity() {
    unsigned count = 0;
    for (int slot = SLOT_11; slot <= SLOT_14; ++slot) {
        const auto item = dComIfGs_getItem(slot, true);
        count += item == dItemNo_NONE_e || item == dItemNo_EMPTY_BOTTLE_e;
    }
    return count;
}
daDitem_c* treasure_actor() {
    auto* actor = actor_by_id(s_treasure.item);
    return actor && fopAcM_GetName(actor) == fpcNm_Demo_Item_e ? static_cast<daDitem_c*>(actor) : nullptr;
}
bool owns_treasure_event() {
    return s_treasure.item != kNone && dComIfGp_getEvent()->mPt2 == s_treasure.item;
}
void retire_treasure_actor() {
    if (s_treasure.item != kNone && fpcM_IsCreating(s_treasure.item)) {
        for (auto* node = g_fpcCtTg_Queue.mpHead; node; node = NODE_GET_NEXT(node)) {
            auto* request = static_cast<create_request*>(reinterpret_cast<create_tag*>(node)->base.mpTagData);
            if (request && request->id == s_treasure.item) {
                if (fpcCtRq_IsDoing(request) || !fpcCtRq_Cancel(request)) return;
                break;
            }
        }
        if (fpcM_IsCreating(s_treasure.item)) return;
    }
    if (auto* item = treasure_actor(); item && !fopAcM_delete(item)) return;
    s_treasure = {};
}
void cancel_treasure_demo() {
    s_run.starting_item_pending=false;
    if (s_run.phase == Phase::TreasureChoice) { s_run.phase = Phase::Running; close_window(); }
    s_chosenTreasure = ChestReward::Count;
    // A save/scene reset can already have installed another event. Only reset
    // our own event while its arena is still current.
    if (in_arena() && !transitioning() && owns_treasure_event()) dComIfGp_event_reset();
    s_treasure.retiring = true;
    retire_treasure_actor();
    if (s_selectedChest < s_chests.size() && s_chests[s_selectedChest].active) {
        s_chests[s_selectedChest].opening = false; s_chests[s_selectedChest].age = 0;
    }
    s_selectedChest = kChestCount;
}
bool chest_interactable(const Chest& chest, daAlink_c* link) {
    if (!chest.active || chest.opening || !s_loot.ready || (s_run.phase != Phase::Running && s_run.phase != Phase::TreasureChoice) ||
        !playable() || s_treasure.item != kNone || !s_run.chest_rewards_left(potion_slot() >= 0) ||
        (dComIfGs_getRupee() < chest_rupee_price(chest)) || link->checkWolf() || dComIfGs_getLife() == 0) return false;
    const cXyz delta = chest.position - link->current.pos;
    // Auto-aim/spin changes facing; it must never hide a nearby Open action.
    if (delta.absXZ() >= 150 || std::abs(delta.y) >= 20) return false;
    const cXyz from = link->current.pos + cXyz(0, 50, 0), to = chest.position + cXyz(0, 50, 0);
    dBgS_LinChk wall; wall.Set(&from, &to, link);
    return !dComIfG_Bgsp().LineCross(&wall);
}
bool begin_treasure_choice(unsigned index, daAlink_c* link) {
    if (s_run.phase != Phase::Running || index >= s_chests.size() || !chest_interactable(s_chests[index], link)) return false;
    auto* layer = link->layer_tag.layer;
    if (!layer || layer == fpcLy_RootLayer() || fpcLy_IsDeletingMesg(layer)) return false;
    auto& chest = s_chests[index];
    // Preserve offers on cancel. Reroll only when every cached reward became unusable.
    if (std::none_of(chest.offers.begin(), chest.offers.end(), [](ChestReward reward) {
            return s_run.chest_eligible(reward, potion_slot() >= 0);
        })) {
        chest.offers = s_run.roll_chest_offers(potion_slot() >= 0);
        for (auto& rarity : chest.rarities) rarity = s_run.roll_rarity();
    }
    if (std::none_of(chest.offers.begin(), chest.offers.end(), [](ChestReward reward) {
            return s_run.chest_eligible(reward, potion_slot() >= 0);
        })) return false;
    s_selectedChest = index; s_chosenTreasure = ChestReward::Count;
    s_run.phase = Phase::TreasureChoice;
    clear_attacks();
    s_swordAction = SwordAction::None; s_wavePending = false;
    s_comboStep = s_comboTimeout = 0;
    for (auto& collider : link->mAtCps) { collider.OffAtSetBit(); collider.ResetAtHit(); }
    link->mAtSph.OffAtSetBit(); link->mAtSph.ResetAtHit();
    link->resetUpperAnime(daAlink_c::UPPER_2, 0);
    link->procWaitInit(); // Open interrupts swings, rolls, damage and item actions.
    return true;
}
bool begin_treasure_demo(unsigned index, daAlink_c* link, ChestReward reward) {
    if (s_run.phase != Phase::TreasureChoice || index != s_selectedChest || index >= s_chests.size() ||
        !chest_interactable(s_chests[index], link)) return false;
    const auto& offers = s_chests[index].offers;
    if (reward == ChestReward::Count || std::find(offers.begin(), offers.end(), reward) == offers.end()) return false;
    auto* layer = link->layer_tag.layer;
    if (!layer || layer == fpcLy_RootLayer() || fpcLy_IsDeletingMesg(layer)) return false;
    Run preview = s_run; preview.phase = Phase::Treasure;
    unsigned rupees = dComIfGs_getRupee();
    const auto choice = unsigned(std::find(offers.begin(), offers.end(), reward) - offers.begin());
    const auto rarity = s_chests[index].rarities[choice];
    const bool key = chest_uses_key(s_chests[index]);
    if (!preview.buy_chest(rupees, reward, potion_capacity(), rarity, s_chests[index].free, s_chests[index].fixed, key)) return false;
    if (!prepare_treasure_message(reward, s_run, preview, rarity)) return false;
    auto* previous = fpcLy_CurrentLayer(); fpcLy_SetCurrentLayer(layer);
    // 0x01 is native presentation-only: daDitem_c skips execItemGet.
    const auto item = fopAcM_createDemoItem(&link->current.pos, treasure_item(reward, preview),
                                          -1, nullptr, fopAcM_GetRoomNo(link), nullptr, 0x01);
    fpcLy_SetCurrentLayer(previous);
    if (item == kNone) return false;
    s_treasure = {}; s_treasure.item = item;
    s_treasure.result = preview; s_treasure.key = key;
    s_treasure.price = key ? 0 : s_run.chest_price(s_chests[index].free, s_chests[index].fixed);
    s_treasure.bonusRupees = s_run.preview_rupees(s_run.chest_rupees(reward, rarity, potion_capacity()));
    if (reward == ChestReward::RedPotion) s_treasure.potions = s_run.chest_gain(reward, rarity, potion_capacity());
    s_chests[index].reward = reward;
    s_run.phase = Phase::Treasure;
    return true;
}
void apply_treasure_inventory(daAlink_c* link,ChestReward reward,const Run& result,unsigned potions) {
    if (reward == ChestReward::MasterSword) {
        const bool light = result.master_sword >= 5;
        const u8 sword = light ? dItemNo_LIGHT_SWORD_e : dItemNo_MASTER_SWORD_e;
        dComIfGs_setCollectSword(light ? COLLECT_LIGHT_SWORD : COLLECT_MASTER_SWORD);
        dComIfGs_onItemFirstBit(sword);
        dMeter2Info_setSword(sword, false);
        dComIfGs_setBButtonItemKey(sword);
        if (light) link->onLightSwordGetEffect();
    } else if (reward == ChestReward::Shield) {
        const u8 shield = shield_item(result);
        dComIfGs_setCollectShield(result.shield_tier());
        dComIfGs_onItemFirstBit(shield);
        dMeter2Info_setShield(shield, false);
    } else if (reward == ChestReward::RedPotion) {
        for (unsigned i = 0; i < potions; ++i)
            dComIfGs_setItem(potion_slot(), dItemNo_RED_BOTTLE_e);
        dComIfGs_onItemFirstBit(dItemNo_EMPTY_BOTTLE_e);
        dComIfGs_onItemFirstBit(dItemNo_RED_BOTTLE_e);
    }
}
// Reuse the choice cards with an inactive, free slot. No world chest/model or
// Get Item event is needed, and the ordinary chest price never advances.
void begin_starting_item_choice() {
    if(!s_run.starting_item_pending||s_run.phase!=Phase::Running)return;
    s_selectedChest=0;s_chosenTreasure=ChestReward::Count;
    auto& chest=s_chests[0];chest={};chest.free=true;
    chest.offers=s_run.roll_chest_offers(potion_slot()>=0);
    for(auto& rarity:chest.rarities)rarity=s_run.roll_rarity();
    s_run.phase=Phase::TreasureChoice;
}
void finish_starting_item_choice() {
    s_run.starting_item_pending=false;s_run.phase=Phase::Running;
    s_chests[0]={};s_selectedChest=kChestCount;s_chosenTreasure=ChestReward::Count;
}
bool grant_starting_item(daAlink_c* link) {
    if(!link||!s_run.starting_item_pending||s_run.phase!=Phase::TreasureChoice||s_selectedChest!=0)return false;
    const auto& chest=s_chests[0];
    const auto found=std::find(chest.offers.begin(),chest.offers.end(),s_chosenTreasure);
    if(s_chosenTreasure==ChestReward::Count||found==chest.offers.end())return false;
    const auto rarity=chest.rarities[unsigned(found-chest.offers.begin())];
    Run preview=s_run;preview.phase=Phase::Treasure;
    unsigned rupees=dComIfGs_getRupee();const unsigned capacity=potion_capacity();
    if(!preview.buy_chest(rupees,s_chosenTreasure,capacity,rarity,true))return false;
    const auto potions=s_chosenTreasure==ChestReward::RedPotion?s_run.chest_gain(s_chosenTreasure,rarity,capacity):0;
    apply_treasure_inventory(link,s_chosenTreasure,preview,potions);
    dComIfGs_setRupee(std::min<unsigned>(dComIfGs_getRupeeMax(),rupees));
    s_run=preview;sync_survivors_inventory();finish_starting_item_choice();return true;
}
void select_treasure(ModContext*, void* data) {
    const auto choice = *static_cast<unsigned*>(data);
    if (s_run.phase != Phase::TreasureChoice || s_chosenTreasure != ChestReward::Count ||
        s_selectedChest >= s_chests.size() || choice >= 3) return;
    const auto reward = s_chests[s_selectedChest].offers[choice];
    if (!s_run.chest_eligible(reward, potion_slot() >= 0)) return;
    s_chosenTreasure = reward;
    close_window(); // Defer native actor/event creation until the UI has closed.
}
void update_treasure_choice(daAlink_c* link) {
    if (s_run.phase != Phase::TreasureChoice || s_chosenTreasure == ChestReward::Count || !playable()) return;
    if(s_run.starting_item_pending) {
        if(!grant_starting_item(link))s_chosenTreasure=ChestReward::Count;
        return;
    }
    if (!begin_treasure_demo(s_selectedChest, link, s_chosenTreasure)) dismiss_treasure_choice();
}
HookAction before_chest_action(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if(link && hub_gear_interaction(link))return HOOK_CONTINUE;
    if (!link || !in_arena() || s_run.phase != Phase::Running || fopAcM_GetID(link) != s_player) return HOOK_CONTINUE;
    for (unsigned i = 0; i < s_chests.size(); ++i) {
        if (!chest_interactable(s_chests[i], link)) continue;
        dComIfGp_setDoStatusForce(BUTTON_STATUS_OPEN, 0);
        if (mDoCPd_c::getTrigA(PAD_1) && begin_treasure_choice(i, link))
            mDoCPd_c::getCpadInfo(PAD_1).mPressedButtonFlags &= ~PAD_BUTTON_A;
        return HOOK_CONTINUE; // Link must execute the native Get Item event.
    }
    return HOOK_CONTINUE;
}
void retain_treasure_actor(fopAc_ac_c* actor, void* args) {
    if (!s_treasure.started || s_treasure.retiring || !owns_treasure_event() ||
        fopAcM_GetID(actor) != s_treasure.item || !actor->sub_method) return;
    const auto* methods = reinterpret_cast<const process_method_class*>(actor->sub_method);
    if (mods::arg<process_method_func>(args, 0) != methods->execute_method) return;
    auto* item = static_cast<daDitem_c*>(actor);
    // The event's partner must survive until the camera/event finishes.
    if (item->mAction == daDitem_c::ACTION_END_e) item->setAction(daDitem_c::ACTION_WAIT_LIGHT_END_e);
}
HookAction before_treasure_get(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    auto* item = treasure_actor();
    if (!link || !item || s_treasure.retiring || !owns_treasure_event() ||
        !item->eventInfo.checkCommandItem() || fopAcM_GetID(link) != s_player) return HOOK_CONTINUE;
    s_treasure.started = true;
    dComIfGp_event_setItemPartnerId(s_treasure.item);
    dComIfGp_event_setGtItm(item->getItemNo());
    link->mDemo.setParam0(0); link->mDemo.setParam1(0);
    return HOOK_CONTINUE;
}
void after_treasure_get(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !s_treasure.started || s_treasure.poseStarted || !owns_treasure_event() ||
        fopAcM_GetID(link) != s_player || link->mProcID != daAlink_c::PROC_GET_ITEM) return;
    link->setSingleAnimeBase(daAlink_c::ANM_GET_A);
    link->mProcVar1.field_0x300a = 0;
    link->field_0x32cc = treasure_message_id();
    s_treasure.poseStarted = true;
}
void update_treasure_demo(daAlink_c* link) {
    if (s_run.phase != Phase::Treasure) return;
    auto* item = treasure_actor();
    if (item && owns_treasure_event() && item->eventInfo.checkCommandItem()) s_treasure.started = true;
    if (s_treasure.started && owns_treasure_event()) {
        auto& chest = s_chests[s_selectedChest];
        chest.opening = true; ++chest.age;
        if (!dComIfGp_evmng_endCheck("DEFAULT_GETITEM")) return;
        const unsigned rupees = dComIfGs_getRupee();
        // Commit the displayed reward, without rerolling against possibly
        // changed inventory. No charge when a potion no longer has space.
        if (rupees >= s_treasure.price && (!s_treasure.key || chest_keys() != 0) &&
            (chest.reward != ChestReward::RedPotion || potion_capacity() >= s_treasure.potions)) {
            apply_treasure_inventory(link,chest.reward,s_treasure.result,s_treasure.potions);
            if (s_treasure.key) set_chest_keys(chest_keys() - 1);
            dComIfGs_setRupee(std::min<unsigned>(dComIfGs_getRupeeMax(), rupees - s_treasure.price + s_treasure.bonusRupees));
            s_run = s_treasure.result;
            sync_survivors_inventory();
            chest = {}; // Remove immediately after the native reward/camera ends.
        }
        cancel_treasure_demo(); s_run.phase = Phase::Running;
        return;
    }
    if (s_treasure.started || ++s_treasure.waitFrames > 300 ||
        (!item && !fpcM_IsCreating(s_treasure.item))) {
        cancel_treasure_demo(); s_run.phase = Phase::Running;
        return; // Failed/canceled demo: no charge, rank or price change.
    }
    if (item && playable() && sword_action_available(link)) {
        item->eventInfo.onCondition(dEvtCnd_CANGETITEM_e);
        fopAcM_orderItemEvent(item, 0, 0);
    }
}

HookAction before_treasure_draw(ModContext*, void* args, void* result, void*) {
    auto* process = mods::arg<leafdraw_class*>(args, 0);
    if (!process || s_selectedChest >= s_chests.size() ||
        s_chests[s_selectedChest].reward != ChestReward::MasterSword ||
        fopAcM_GetID(process) != s_treasure.item) return HOOK_CONTINUE;
    auto* item = treasure_actor();
    auto* model = s_loot.masterSword;
    if (!s_treasure.retiring && s_treasure.started && owns_treasure_event() &&
        model && item && item->mpModel && item->chkDraw() && !item->chkDead()) {
        // Preserve the native item position/rotation/scale without ever moving
        // Link's equipped model or changing the global item resource table.
        model->setBaseTRMtx(item->mpModel->getBaseTRMtx());
        model->setBaseScale(*item->mpModel->getBaseScale());
        dKy_tevstr_c tev{}; g_env_light.settingTevStruct(0, &item->current.pos, &tev);
        g_env_light.setLightTevColorType_MAJI(model, &tev);
        mDoExt_modelUpdateDL(model);
    }
    *static_cast<int*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
}
