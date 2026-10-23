// Keep native IDs: the item screen derives its icon from ID - 0x65, and
// ItemExplain selects the global/stage text archive by the numeric ID.
// Callback-owned buffers outlive rendering; overrides only apply in this mode.
struct SurvivorsMessage {
    unsigned id = 0;
    bool treasure = false;
    const char* activeText = nullptr;
    std::vector<mods::flow::MessageVariant> variants;
    std::vector<mods::flow::MessageOverride> overrides;
};
std::unique_ptr<SurvivorsMessage> s_treasureMessage, s_itemNameMessage, s_itemDescriptionMessage;
bool survivors_message_text(ModContext*, const MessageOverrideContext* message,
        MessageTextData* out, void* data) {
    auto& replacement = *static_cast<SurvivorsMessage*>(data);
    if (!in_arena() || (replacement.treasure && !owns_treasure_event())) return false;
    for (const auto& variant : replacement.variants) if (variant.language() == message->language) {
        out->text = variant.text().data(); out->text_size = variant.text().size();
        replacement.activeText = reinterpret_cast<const char*>(out->text);
        return true;
    }
    return false;
}
std::unique_ptr<SurvivorsMessage> override_survivors_message(unsigned id,
        const mods::flow::MessageBuilder& builder, bool treasure = false) {
    auto message = std::make_unique<SurvivorsMessage>();
    message->id = id; message->treasure = treasure;
    for (auto language : {MESSAGE_LANGUAGE_ENGLISH, MESSAGE_LANGUAGE_GERMAN,
            MESSAGE_LANGUAGE_FRENCH, MESSAGE_LANGUAGE_SPANISH, MESSAGE_LANGUAGE_ITALIAN,
            MESSAGE_LANGUAGE_JAPANESE}) {
        auto variant = builder.build(language);
        if (!variant) return {};
        message->variants.push_back(std::move(variant));
        auto handle = mods::flow::override_message_fn(0, id, language, survivors_message_text, message.get());
        if (!handle) return {};
        message->overrides.push_back(std::move(handle));
    }
    return message;
}
std::vector<ItemChange> treasure_changes(ChestReward reward, const Run& before, const Run& after, Rarity rarity = Rarity::Common) {
    auto changes = reward_changes(reward, before, after);
    if (reward == ChestReward::RedPotion) {
        unsigned bottles = 0, red = 0;
        for (int slot = SLOT_11; slot <= SLOT_14; ++slot) {
            const auto item = dComIfGs_getItem(slot, false);
            bottles += item != dItemNo_NONE_e; red += item == dItemNo_RED_BOTTLE_e;
        }
        const unsigned gain = before.chest_gain(reward, rarity, potion_capacity());
        if (bottles < 4) changes.push_back({"Bottles", number(bottles), number(std::min(4u, bottles + gain))});
        changes.push_back({"Red potions", number(red), number(red + gain)});
    }
    const auto extra = before.preview_rupees(before.chest_rupees(reward, rarity, potion_capacity()));
    if (extra) changes.push_back({"Excess reward", "0", "+" + number(extra) + " rupees"});
    return changes;
}
bool prepare_treasure_message(ChestReward reward, const Run& before, const Run& after, Rarity rarity) {
    const auto changes = treasure_changes(reward, before, after, rarity);
    constexpr std::array<MessageTextColor, 5> colors{MESSAGE_COLOR_DEFAULT, MESSAGE_COLOR_GREEN,
        MESSAGE_COLOR_BLUE, MESSAGE_COLOR_PURPLE, MESSAGE_COLOR_ORANGE};
    mods::flow::MessageBuilder builder;
    builder.box_kind(MESSAGE_BOX_ITEM_GET).box_position(MESSAGE_POSITION_MIDDLE)
        .text_color(colors[unsigned(rarity)]).text(reward_name(reward, after))
        .text(" x").text(number(reward_units(rarity))).text_color(MESSAGE_COLOR_DEFAULT);
    for (const auto& change : changes) {
        builder.text("\n").text(change.label).text(": ")
            .text_color(MESSAGE_COLOR_RED).text(change.before).text(" -> ").text(change.after)
            .text_color(MESSAGE_COLOR_DEFAULT);
    }
    auto message = override_survivors_message(treasure_item(reward, after) + 0x65, builder, true);
    if (!message) return false; // No event or payment with an unavailable reward message.
    s_treasureMessage = std::move(message);
    return true;
}
unsigned treasure_message_id() { return s_treasureMessage ? s_treasureMessage->id : 0; }

// HD HUD's item-get rewrite reads the native entry from the archive again,
// after the message service has supplied our text. Rebind at the next sequence
// update, after all setMessageIndex post-hooks, preserving the native ID/icon.
HookAction before_treasure_text_update(ModContext*, void* args, void*, void*) {
    auto* control = mods::arg<JMessage::TControl*>(args, 0);
    if (!s_treasureMessage || !s_treasureMessage->activeText || !in_arena() ||
        !owns_treasure_event()) return HOOK_CONTINUE;
    auto* message = dMsgObject_getMsgObjectClass();
    if (!message || message->mpCtrl != control || message->getMessageIDLocal() != s_treasureMessage->id ||
        !control->pSequenceProcessor_ || !control->pResourceCache_ || !control->pEntry_ ||
        !control->pMessageText_begin_) return HOOK_CONTINUE; // Never restart a completed sequence.
    const char* text = s_treasureMessage->activeText;
    if (control->pMessageText_begin_ == text) return HOOK_CONTINUE;
    control->pSequenceProcessor_->reset();
    control->pMessageText_begin_ = control->pMessageText_current_ = text;
    control->pszText_update_current_ = nullptr;
    control->oStack_renderingProcessor_.clear();
    control->pSequenceProcessor_->setBegin_messageEntryText(control->pResourceCache_, control->pEntry_, text);
    return HOOK_CONTINUE;
}

// Existing item slots for weapons; unused quest slots for the two swords and
// shield. This inventory belongs only to the isolated Survivors save.
constexpr std::array<int, 8> kSurvivorsSlots{SLOT_4, SLOT_0, SLOT_15, SLOT_6, SLOT_20, SLOT_21, SLOT_22, SLOT_23};
void sync_survivors_inventory() {
    const std::array<u8, 8> items{
        u8(s_run.rank(Upgrade::Bow) ? dItemNo_BOW_e : dItemNo_NONE_e),
        u8(s_run.rank(Upgrade::Boomerang) ? dItemNo_BOOMERANG_e : dItemNo_NONE_e),
        u8(s_run.rank(Upgrade::Bombs) ? dItemNo_NORMAL_BOMB_e : dItemNo_NONE_e),
        u8(s_run.rank(Upgrade::Ball) ? dItemNo_IRONBALL_e : dItemNo_NONE_e),
        u8(s_run.master_sword ? (s_run.master_sword >= 5 ? dItemNo_LIGHT_SWORD_e : dItemNo_MASTER_SWORD_e) : dItemNo_NONE_e),
        u8(s_run.owns(ChestReward::OrdonSword) ? dItemNo_SWORD_e : dItemNo_NONE_e),
        s_run.owns(ChestReward::Shield) ? shield_item(s_run) : u8(dItemNo_NONE_e),
        u8(s_run.dark_link ? dItemNo_POU_SPIRIT_e : dItemNo_NONE_e)};
    for (unsigned i = 0; i < items.size(); ++i)
        if (dComIfGs_getItem(kSurvivorsSlots[i], false) != items[i])
            dComIfGs_setItem(kSurvivorsSlots[i], items[i]);
}
bool survivors_slot_assignable(u8 slot) {
    return slot == dItemNo_NONE_e || (slot >= SLOT_11 && slot <= SLOT_14 &&
        dComIfGs_getItem(slot, false) != dItemNo_NONE_e);
}
HookAction before_ring_assignment(ModContext*, void* args, void*, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!in_arena() || !ring || ring->mPlayerIsWolf || ring->mCurrentSlot >= MAX_ITEM_SLOTS) return HOOK_CONTINUE;
    return survivors_slot_assignable(ring->mItemSlots[ring->mCurrentSlot]) ? HOOK_CONTINUE : HOOK_SKIP_ORIGINAL;
}
// Do not hook the tiny native save setters: Windows cannot relocate their
// entry branches. Validate the same saved slots before rebuilding the live
// item cache, and before Link can consume direct writes from another provider.
bool sanitize_survivors_binding(int button) {
    if(!in_arena() || button<0 || button>=MAX_SELECT_ITEM)return false;
    auto& status=g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA();
    const bool invalid=!survivors_slot_assignable(status.mSelectItem[button]);
    const bool mixed=status.mMixItem[button]!=dItemNo_NONE_e;
    if(invalid)status.mSelectItem[button]=dItemNo_NONE_e;
    if(mixed)status.mMixItem[button]=dItemNo_NONE_e;
    return invalid || mixed;
}
HookAction before_item_binding(ModContext*, void* args, void*, void*) {
    sanitize_survivors_binding(mods::arg<int>(args,0));
    return HOOK_CONTINUE; // Native code refreshes the cache from sanitized slots.
}
void enforce_survivors_bindings() {
    for(int button=0;button<MAX_SELECT_ITEM;++button)
        if(sanitize_survivors_binding(button))dComIfGp_setSelectItem(button);
}
HookAction before_player_bindings(ModContext*,void*,void*,void*) {
    enforce_survivors_bindings();
    return HOOK_CONTINUE;
}
ChestReward inventory_reward(u8 slot) {
    for (unsigned i = 0; i < kSurvivorsSlots.size(); ++i) if (slot == kSurvivorsSlots[i]) {
        if (i < 4) return static_cast<ChestReward>(i);
        return i == 4 ? ChestReward::MasterSword : i == 5 ? ChestReward::OrdonSword : i == 6 ? ChestReward::Shield : ChestReward::DarkLink;
    }
    return slot >= SLOT_11 && slot <= SLOT_14 && dComIfGs_getItem(slot, false) == dItemNo_RED_BOTTLE_e ?
        ChestReward::RedPotion : ChestReward::Count;
}
void after_item_explain(ModContext*, void* args, void* result, void*) {
    if (!in_arena() || !result || !*static_cast<u8*>(result)) return;
    auto* explain = mods::arg<dMenu_ItemExplain_c*>(args, 0);
    const auto reward = inventory_reward(mods::arg<u8>(args, 1));
    if (!explain || reward == ChestReward::Count) return;
    auto name = override_survivors_message(explain->field_0xcc, mods::flow::MessageBuilder{}
        .box_kind(MESSAGE_BOX_ITEM_NAME).text(reward_name(reward, s_run)));
    auto description = override_survivors_message(explain->field_0xc8, mods::flow::MessageBuilder{}
        .box_kind(MESSAGE_BOX_ITEM_NAME).text_scale(80).text(item_description(reward, s_run)));
    if (!name || !description) return;
    s_itemNameMessage = std::move(name); s_itemDescriptionMessage = std::move(description);
    // Reopening the same item after an upgrade must invalidate the native draw
    // cache (also used by Twilight HD HUD), even though its ID stays the same.
    explain->field_0xd0 = ~0u;
    // Automatic weapons have unlimited run ammo, not the native save's counts.
    if (reward <= ChestReward::Ball) { explain->field_0xde = explain->field_0xdf = 0; explain->setNumber(); }
}
