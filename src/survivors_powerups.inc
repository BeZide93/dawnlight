// The scroll is present in itemicon.arc but has no inventory item ID. Use an
// unused ID only for our namespaced UI URL, redirecting the native texture read
// while a Survivors upgrade/overview is visible. Never change the item table,
// inventory, or the event flags that grant actual Hidden Skills.
constexpr u8 kPowerupScrollItem = dItemNo_NOENTRY_146_e;
constexpr int kPowerupScrollTexture = 0x3d; // ITEMICON_BTI_NI_ITEM_ICON_MAKIMONO
constexpr const char* kPowerupScrollIcon = "item://item/92?dawnlight-survivors-powerup";
// These ranks belong to items already described in the native item wheel.
ChestReward powerup_item(Upgrade upgrade) {
    if (upgrade >= Upgrade::Bow && upgrade <= Upgrade::Ball)
        return ChestReward(unsigned(upgrade) - unsigned(Upgrade::Bow));
    if (upgrade == Upgrade::MasterSword) return ChestReward::MasterSword;
    if (upgrade == Upgrade::DualWield) return ChestReward::OrdonSword;
    if (upgrade == Upgrade::PotionHealing) return ChestReward::RedPotion;
    return ChestReward::Count;
}
std::string powerup_icon(Upgrade upgrade, Rarity rarity, const Run& preview) {
    const auto item = powerup_item(upgrade);
    if (item != ChestReward::Count) {
        char icon[40];
        std::snprintf(icon, sizeof(icon), "item://item/%02x", unsigned(treasure_item(item, preview)));
        return icon;
    }
    // Native full Heart Container (UTAWA_HEART), not a piece or recovery heart.
    if (upgrade == Upgrade::Heart) return "item://item/22";
    if (upgrade == Upgrade::Rupees) {
        // Item IDs preserve the native layered rupee colors in the icon cache:
        // Common green, Uncommon blue, Rare red, Epic purple, Legendary orange.
        constexpr std::array icons{"item://item/01", "item://item/02", "item://item/04",
                                   "item://item/05", "item://item/06"};
        return icons[std::min(unsigned(rarity), unsigned(icons.size() - 1))];
    }
    return kPowerupScrollIcon;
}
HookAction before_powerup_icon(ModContext*, void* args, void*, void*) {
    if (in_arena() && (s_run.phase == Phase::Upgrade || s_powerupsRequested || s_powerupsWindow) &&
        mods::arg<u8>(args, 1) == kPowerupScrollItem && mods::arg<int>(args, 10) == -1)
        mods::arg_ref<int>(args, 10) = kPowerupScrollTexture;
    return HOOK_CONTINUE;
}
std::string powerup_description_rml(Upgrade upgrade) {
    std::string text = "<span class=\"survivors-treasure-name\">";
    text += treasure_rml_escape(s_run.upgrade_name(upgrade)) + " - Level " + number(s_run.rank(upgrade)) + "</span><br/>";
    const auto description = upgrade_description(upgrade, s_run);
    std::string line;
    for (char c : description) {
        if (c == '\n') { text += treasure_rml_escape(line) + "<br/>"; line.clear(); }
        else line += c;
    }
    return text + treasure_rml_escape(line);
}
ModResult build_upgrade_choices(UiElementHandle left, UiElementHandle right) {
    UiElementHandle row = 0;
    auto result = begin_choice_cards(left, right, "", row);
    if (result != MOD_OK) return result;
    for (unsigned i = 0; i < s_run.offers.size(); ++i) {
        Run preview = s_run;
        if (!preview.choose(i)) continue;
        const auto upgrade = s_run.offers[i];
        const auto rarity = s_run.offer_rarities[i];
        const auto icon = powerup_icon(upgrade, rarity, preview);
        std::string rml = choice_rml_start(icon.c_str(), preview.upgrade_name(upgrade), rarity, "", upgrade_units(upgrade,rarity));
        rml += "<div class=\"survivors-treasure-effect\">" + treasure_rml_escape(descriptions[unsigned(upgrade)]) + "</div>";
        for (const auto& change : upgrade_changes(upgrade, s_run, preview, rarity))
            rml += "<div class=\"survivors-treasure-change\">" + treasure_rml_escape(change.label) +
                ": <span class=\"survivors-treasure-delta\">" + treasure_rml_escape(change.before) +
                " -&gt; " + treasure_rml_escape(change.after) + "</span></div>";
        const unsigned extra = s_run.preview_rupees(s_run.upgrade_rupees(upgrade, rarity));
        if (upgrade != Upgrade::Rupees && extra)
            rml += "<div class=\"survivors-treasure-change\">Excess reward: <span class=\"survivors-treasure-delta\">+" +
                number(extra) + " rupees</span></div>";
        rml += "</div>";
        result = add_choice_card(row, rml, preview.upgrade_name(upgrade), select_upgrade, &s_choices[i]);
        if (result != MOD_OK) return result;
    }
    return add_choice_actions(left,false);
}

ModResult build_powerup_overview(ModContext*, UiWindowHandle, UiElementHandle left, UiElementHandle right,
                                void*, ModError*) {
    auto result = svc_ui->pane_add_text(mod_ctx, left, "Collected power-ups", nullptr);
    if (result != MOD_OK) return result;
    result = svc_ui->pane_add_rml(mod_ctx, right, "Choose a power-up to view its current effects. Equipment upgrades are shown in the item wheel. Rupee rewards have no permanent rank.", nullptr);
    if (result != MOD_OK) return result;
    bool any = false;
    for (unsigned i = 0; i < unsigned(Upgrade::Count); ++i) {
        const auto upgrade = Upgrade(i);
        if (upgrade==Upgrade::Rupees || !s_run.rank(upgrade) || powerup_item(upgrade) != ChestReward::Count) continue;
        const std::string label = std::string(s_run.upgrade_name(upgrade)) + " - Level " + number(s_run.rank(upgrade));
        const auto help = powerup_description_rml(upgrade);
        UiControlDesc control = UI_CONTROL_DESC_INIT;
        control.label = label.c_str();
        // Native focus/hover updates the right pane for controller and touch.
        // Buttons require on_pressed even when read-only. Omitting it rejects
        // the descriptor and fails the tab build, deactivating the entire mod.
        control.on_pressed = [](ModContext*, void*) {};
        control.help_rml = help.c_str();
        result = svc_ui->pane_add_control(mod_ctx, left, &control, nullptr);
        if (result != MOD_OK) return result;
        any = true;
    }
    if (!any) return svc_ui->pane_add_text(mod_ctx, left, "No power-ups collected yet. Gain experience during the run; view equipment upgrades in the item wheel.", nullptr);
    return MOD_OK;
}
void close_powerup_window() {
    s_powerupsRequested = false; s_powerupsMenu = nullptr;
    const auto window = s_powerupsWindow; s_powerupsWindow = 0;
    if (window) svc_ui->window_close(mod_ctx, window);
}
void open_powerup_window() {
    if (!s_powerupsRequested || s_powerupsWindow || ui_visible()) return;
    UiTabDesc tab = UI_TAB_DESC_INIT;
    tab.title = "Power-ups"; tab.build = build_powerup_overview;
    UiWindowDesc window = UI_WINDOW_DESC_INIT;
    window.tabs = &tab; window.tab_count = 1;
    window.rcss = "window content pane { font-size: 18dp; } .survivors-treasure-name { font-size: 22dp; color: #f4d78b; margin-bottom: 12dp; }";
    window.on_closed = [](ModContext*, UiWindowHandle handle, void*) {
        if (s_powerupsWindow == handle) {
            s_powerupsWindow = 0; s_powerupsRequested = false;
            s_powerupsMenu = nullptr;
        }
    };
    if (svc_ui->window_push(mod_ctx, &window, &s_powerupsWindow) != MOD_OK) {
        s_powerupsRequested = false; s_powerupsMenu = nullptr;
        svc_log->warn(mod_ctx, "Hyrule Survivors: could not open power-ups");
    }
}
void after_powerup_scroll_visible(ModContext*, void*, void* result, void*) {
    if (in_arena() && result) *static_cast<bool*>(result) = true;
}
void after_powerup_collection_action(ModContext*, void* args, void*, void*) {
    auto* menu = mods::arg<dMenu_Collect2D_c*>(args, 0);
    if (!in_arena() || !menu || menu->mSubWindowOpenCheck != 5) return;
    // Both native A and pointerActivateCurrent set this request. Consume it
    // before dMw closes Collection/fades into the vanilla Hidden Skills screen.
    menu->mSubWindowOpenCheck = 0;
    if (!s_powerupsWindow && !s_powerupsRequested) {
        s_powerupsMenu = menu; s_powerupsRequested = true;
    }
}
HookAction before_powerup_collection_wait(ModContext*, void* args, void*, void*) {
    return in_arena() && mods::arg<dMenu_Collect2D_c*>(args, 0) == s_powerupsMenu &&
        (s_powerupsRequested || s_powerupsWindow) ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}
HookAction before_powerup_collection_delete(ModContext*, void* args, void*, void*) {
    if (mods::arg<dMenu_Collect2D_c*>(args, 0) == s_powerupsMenu) close_powerup_window();
    return HOOK_CONTINUE;
}
