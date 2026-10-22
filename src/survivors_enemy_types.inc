// Run-owned enemy health/model setup and absolute collider sizes.
void initialize_enemy(Enemy* enemy, fopAc_ac_c* actor) {
    if (!enemy || !actor) return;
    if (enemy->initialized) {
        const auto minute=s_run.endless_minutes();
        if(!enemy->retiring && !enemy->credited && actor->health>0 && minute>enemy->endlessMinute) {
            const double factor=std::pow(1.1,double(std::min(1000u,minute-enemy->endlessMinute)));
            const int maximum=std::min(32767.0,std::ceil(actor->field_0x560*factor));
            // Preserve damage already taken as a fraction; do not heal on scaling.
            actor->health=std::max(1, int((int64_t(actor->health)*maximum)/std::max(1,int(actor->field_0x560))));
            actor->field_0x560=maximum;enemy->endlessMinute=minute;
        }
        // Guards can wait from minute zero until late in the run. Keep untouched
        // dormant guards current; a hit wakes them or lowers HP, freezing growth.
        if (enemy->elite && enemy->dormant && !enemy->retiring && !enemy->credited &&
            actor->health > 0 && actor->health == actor->field_0x560)
            actor->field_0x560 = actor->health = static_cast<s16>(
                s_run.enemy_health(native_roster[enemy->species].hp, true, s_map));
        return;
    }
    enemy->initialized = true;enemy->endlessMinute=s_run.endless_minutes();
    if (extended_enemy(enemy->profile)) {initialize_extended_enemy(*enemy,actor);return;}
    if (enemy->elite) {
        // These native matrix paths use HIO scale, ignoring actor scale.
        if (enemy->profile == fpcNm_E_OC_e) static_cast<daE_OC_c*>(actor)->getOcModel()->setBaseScale(actor->scale);
        else if (enemy->profile == fpcNm_E_MS_e) static_cast<e_ms_class*>(actor)->mpModelMorf->getModel()->setBaseScale(actor->scale);
        else if (enemy->profile == fpcNm_E_BA_e) reinterpret_cast<e_ba_class*>(actor)->mpMorf->getModel()->setBaseScale(actor->scale);
        // Bulblin's native matrix already includes actor->scale.
    }
    actor->field_0x560 = actor->health = static_cast<s16>(
        s_run.spawn_enemy_health(native_roster[enemy->species].hp, enemy->elite, s_map, enemy->eliteSlot));
}
void scale_elite_colliders(const Enemy* enemy, fopAc_ac_c* actor) {
    if (enemy && actor && enemy->elite && enemy->profile == fpcNm_E_OC_e) {
        // cc_set follows the scaled joints but writes fixed radii each Execute.
        // Assign absolute values so paused callbacks cannot compound the scale.
        auto* bokoblin = static_cast<daE_OC_c*>(actor);
        bokoblin->mSphs_cc[0].SetR(50.0f * actor->scale.x);
        bokoblin->mSphs_cc[1].SetR(45.0f * actor->scale.x);
        bokoblin->mSphs_at[0].SetR(45.0f * actor->scale.x);
        bokoblin->mSphs_at[1].SetR(30.0f * actor->scale.x);
    }
    if (enemy && actor && enemy->elite) {
        if (enemy->profile == fpcNm_E_MS_e) {
            auto* rat=static_cast<e_ms_class*>(actor);
            rat->mSph1.SetR(35.f*actor->scale.x); rat->mSph2.SetR(20.f*actor->scale.x);
        } else if (enemy->profile == fpcNm_E_BA_e) {
            reinterpret_cast<e_ba_class*>(actor)->mSph.SetR(30.f*actor->scale.x);
        } else if (enemy->profile == fpcNm_E_RD_e) {
            auto* bulblin=reinterpret_cast<e_rd_class*>(actor);
            for (auto& sphere:bulblin->cc_sph) sphere.SetR(35.f*actor->scale.x);
        }
    }
}
