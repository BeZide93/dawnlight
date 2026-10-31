// Run-local difficulty; native collision, health reduction and death paths remain in charge.
HookAction before_enemy_speed(ModContext*, void* args, void*, void*) {
    auto* acch=mods::arg<dBgS_Acch*>(args,0);
    auto* actor=acch ? acch->getMyAc() : nullptr;
    if (!actor || !playable() || s_run.phase!=Phase::Running) return HOOK_CONTINUE;
    const auto* enemy=tracked(fopAcM_GetID(actor));
    if (!enemy || !enemy->initialized || enemy->retiring || enemy->credited || enemy->dormant || actor->health<=0)
        return HOOK_CONTINUE;
    if(enemy->profile==fpcNm_B_TN_e&&acch!=&static_cast<daB_TN_c*>(actor)->mAcch)return HOOK_CONTINUE;
    const bool boss=faster_boss(*enemy);
    if(enemy->stagger)return HOOK_CONTINUE;
    // Boss locomotion and charges gain 50%; ordinary difficulty retains its
    // conservative locomotion whitelist. Bloat has no Acch call (own hook).
    const float endlessSpeed=float(std::min(32767.0,s_run.endless_factor()));
    if(boss) {
        const float extra=1.5f*endlessSpeed-1.f;
        actor->current.pos.x+=actor->speed.x*extra;
        actor->current.pos.z+=actor->speed.z*extra;
        if(enemy->profile==fpcNm_E_YC_e||enemy->profile==fpcNm_E_HZELDA_e||enemy->profile==fpcNm_E_KR_e)
            actor->current.pos.y+=actor->speed.y*extra;
        acch->OnLineCheck();
    }
    const float speciesSpeed=enemy->profile==fpcNm_E_BS_e?2.f:1.f;
    const float movement=float(std::min(32767.f,s_run.options.enemy_speed*(boss?1.f:endlessSpeed)*speciesSpeed));
    if(movement==1.f)return HOOK_CONTINUE;
    // Whitelist locomotion, not a numeric range which also contains attacks.
    // Bokoblin attack root motion is separate from speed and must stay native,
    // including the frame transitioning from attack back to the find state.
    if (enemy->profile==fpcNm_E_OC_e) {
        auto* bok=static_cast<daE_OC_c*>(actor);
        const bool walk=bok->mActionMode==1 && bok->mOcState==1 && bok->checkBck(0x1d);
        const bool chase=bok->mActionMode==3 && bok->mOcState==4 &&
            (bok->checkBck(0x1e) || bok->checkBck(0x0b));
        if (!walk && !chase) return HOOK_CONTINUE;
    } else if (enemy->profile==fpcNm_E_MS_e) {
        auto* rat=static_cast<e_ms_class*>(actor);
        const bool walk=rat->mAction==0 && (rat->mMode==1 || rat->mMode==6);
        const bool chase=rat->mAction==1 && rat->mMode==2;
        if (!walk && !chase) return HOOK_CONTINUE; // Windup, leap and recovery stay native.
    } else if (enemy->profile==fpcNm_E_BA_e) {
        auto* bat=reinterpret_cast<e_ba_class*>(actor);
        const bool fly=bat->mAction==e_ba_class::ACT_FIGHT_FLY || bat->mAction==e_ba_class::ACT_RETURN ||
            bat->mAction==e_ba_class::ACT_FLY || bat->mAction==e_ba_class::ACT_PATH_FLY;
        if (!fly || bat->mIsDying || bat->mKnockbackSpeed>0.1f) return HOOK_CONTINUE;
    } else if (enemy->profile==fpcNm_E_RD_e) {
        const auto* bulblin=reinterpret_cast<e_rd_class*>(actor);
        const bool chase=bulblin->action==3 && bulblin->mode>=1 && bulblin->mode<=3 &&
            (bulblin->anm==0x32 || bulblin->anm==0x1b);
        const bool bowRun=bulblin->action==5 && (bulblin->mode==1 || bulblin->mode==2) && bulblin->anm==0x32;
        if ((!chase && !bowRun) || bulblin->jump_z!=0 || bulblin->field_0x9f0>0.1f) return HOOK_CONTINUE;
    } else if (!extended_locomotion(actor,enemy->profile) || enemy->stagger) return HOOK_CONTINUE;
    const float extra=(movement-1.f)*(boss?1.5f*endlessSpeed:1.f);
    // Add only horizontal locomotion velocity before native wall correction.
    // Do not multiply animation translations, collision pushes, falls or teleports.
    actor->current.pos.x+=actor->speed.x*extra;
    actor->current.pos.z+=actor->speed.z*extra;
    acch->OnLineCheck();
    return HOOK_CONTINUE;
}
HookAction before_enemy_damage(ModContext*, void* args, void*, void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if (!link || fopAcM_GetID(link)!=s_player || !playable() || s_run.phase!=Phase::Running ||
        mods::arg<int>(args,1)<=0 || mods::arg<int>(args,4)!=0) return HOOK_CONTINUE;
    suppress_player_reaction(link);
    for (auto& target:link->mTgCyls) {
        auto* attacker=target.ChkTgHit() ? target.GetTgHitAc() : nullptr;
        const auto* enemy=attacker ? tracked(fopAcM_GetID(attacker)) : nullptr;
        if (attacker && active_enemy_arrow(fopAcM_GetID(attacker))) {
            mods::arg_ref<int>(args,1)=int(s_run.incoming_damage(unsigned(mods::arg<int>(args,1)),enemy_damage_factor(fopAcM_GetID(attacker))));
            break;
        }
        if (!enemy || !enemy->initialized || enemy->retiring) continue;
        mods::arg_ref<int>(args,1)=int(s_run.incoming_damage(unsigned(mods::arg<int>(args,1)),enemy_damage_factor(fopAcM_GetID(attacker))));
        break;
    }
    return HOOK_CONTINUE;
}
// Scale only the final rendered camera. Restore its native input before the next
// Run, so zoom cannot compound or feed back into native orbit smoothing.
dCamera_c* s_zoomCamera=nullptr;
cXyz s_nativeEye{}, s_zoomEye{};
HookAction before_run_camera(ModContext*, void* args, void*, void*) {
    auto* camera=mods::arg<dCamera_c*>(args,0);
    restore_hub_arrival_camera(camera);
    restore_sword_aim_camera(camera);
    if (camera && camera==s_zoomCamera && camera->mEye.abs2(s_zoomEye)<0.001f) {
        camera->mEye=s_nativeEye;
        camera->mDirection.Val(camera->mEye-camera->mCenter);
    }
    s_zoomCamera=nullptr;
    return HOOK_CONTINUE;
}
void update_run_camera(ModContext*, void* args, void*, void*) {
    auto* camera=mods::arg<dCamera_c*>(args,0);
    if(update_sword_aim_camera(camera))return;
    if (!camera || !in_arena() || transitioning() || (s_run.phase==Phase::Loading && !s_hub) ||
        dComIfGp_event_runCheck() || dCamera_c::isAimActive() || camera->mPadID!=0) return;
    // Hub warmup outlasts the arrival: keep its configured endpoint throughout.
    const unsigned percent=(s_run.phase==Phase::Ready || s_hub) ? s_options.camera_distance : s_run.options.camera_distance;
    if (percent==100) return;
    const cXyz delta=camera->mEye-camera->mCenter;
    if (delta.abs2()<1) return;
    cXyz eye=camera->mCenter+delta*(float(percent)/100.f);
    dBgS_CamLinChk wall;
    if (camera->lineBGCheck(&camera->mCenter,&eye,&wall,0x40b7))
        eye=wall.GetCross()-delta*(5.f/delta.abs());
    s_nativeEye=camera->mEye; s_zoomEye=eye; s_zoomCamera=camera;
    camera->mEye=eye; camera->mDirection.Val(eye-camera->mCenter);
}

void after_run_camera(ModContext* ctx,void* args,void* result,void* data) {
    update_run_camera(ctx,args,result,data);
    resolve_manual_projectiles(mods::arg<dCamera_c*>(args,0));
}
