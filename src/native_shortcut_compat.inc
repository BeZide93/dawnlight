// Scoped compatibility for native Quick Transform and Sun Song handlers.
#if defined(__ANDROID__)
using TouchGetConfigVarFn = dusk::config::ConfigVarBase* (*)(std::string_view);
TouchGetConfigVarFn s_touchGetConfigVar = nullptr;

bool native_touch_shortcuts_enabled() {
    if (!dawnlight_touch_ui_active() || s_touchGetConfigVar == nullptr) return false;
    const auto* setting = s_touchGetConfigVar("game.enableTouchControls");
    return setting != nullptr &&
        static_cast<const dusk::config::ConfigVar<bool>*>(setting)->getValue();
}

#else
bool native_touch_shortcuts_enabled() { return false; }
#endif

dMeter2Draw_c* s_shortcutMeter = nullptr;
f32 s_shortcutZAlpha = 0.0f;

bool midna_shortcut_available(daAlink_c* link, dMeter2Draw_c* meter) {
    if (link == nullptr || meter == nullptr || touch_midna_controls_suppressed() ||
        dComIfGp_isEnableNextStage() || meter->getCanoeFishing() ||
        !dComIfGs_isEventBit(0x0C10) || dComIfGs_isEventBit(0x6140) ||
        dComIfGs_isEventBit(dSv_event_flag_c::F_0800))
    {
        return false;
    }
    // Read-only eligibility from Link::orderZTalk. The meter clears its Z-use
    // bit after each update, so that transient bit cannot guard next tick's
    // combos. Pane visibility/alpha is also not eligibility with touch HUDs.
    if ((!link->checkReinRide() && !link->checkModeFlg(0x40000) &&
            !link->checkMagneBootsOn() &&
            (!link->mLinkAcch.ChkGroundHit() || link->checkModeFlg(0x70C52))) ||
        link->mThrowBoomerangAcKeep.getActor() != nullptr ||
        dComIfGp_checkPlayerStatus0(0, 0x8000000) ||
        link->mProcID == daAlink_c::PROC_CRAWL_END || link->checkHorseZelda() ||
        link->checkCloudSea() || link->checkStageName("D_MN08A") ||
        (link->checkStageName("D_MN09A") &&
            ((fopAcM_GetRoomNo(link) == 50 && dComIfG_play_c::getLayerNo(0) == 0) ||
                fopAcM_GetRoomNo(link) == 51)) ||
        (link->checkMagneBootsOn() &&
            (!link->checkStageName("D_MN04B") || !cBgW_CheckBGround(link->mMagneBootsTopVec.y))))
    {
        return false;
    }
    return link->checkMidnaRide();
}

HookAction before_native_shortcut(ModContext*, void* args, void*, void*) {
    s_shortcutMeter = nullptr;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!native_touch_shortcuts_enabled() && !gale_shortcut_priority_active(link)) {
        return HOOK_CONTINUE;
    }
    auto* meterClass = dMeter2Info_getMeterClass();
    auto* meter = meterClass != nullptr ? meterClass->getMeterDrawPtr() : nullptr;
    if (midna_shortcut_available(link, meter)) {
        s_shortcutMeter = meter;
        s_shortcutZAlpha = meter->mButtonZAlpha;
        meter->mButtonZAlpha = 1.0f;
    }
    // The original handler still checks its setting, progression, current
    // action, ground/water state and all transformation/howling restrictions.
    return HOOK_CONTINUE;
}

void after_native_shortcut(ModContext*, void*, void*, void*) {
    auto* meterClass = dMeter2Info_getMeterClass();
    if (s_shortcutMeter != nullptr && meterClass != nullptr &&
        meterClass->getMeterDrawPtr() == s_shortcutMeter)
    {
        s_shortcutMeter->mButtonZAlpha = s_shortcutZAlpha;
    }
    s_shortcutMeter = nullptr;
}
