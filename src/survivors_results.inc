// Shared rendering for live results and immutable saved high-score snapshots.
constexpr const char* kResultsStyle=R"(
window { background-color: #0c1210f5; }
window content pane.survivors-results-main { flex: 1 1 69%; padding: 20dp; gap: 12dp; overflow: hidden auto; }
window content pane.survivors-results-side { flex: 0 0 31%; padding: 20dp; gap: 12dp; overflow: hidden auto; background-color: #19221aee; }
.sr-body { display: block; width: 100%; flex-shrink: 0; }
.sr-integrity { font-size: 13dp; color: #ff4545; margin-bottom: 6dp; }
.sr-heading { font-family: var(--font-family-heading); font-size: 32dp; color: #ffdc75; }
.sr-subtitle { font-size: 17dp; color: #c4cebd; margin-bottom: 14dp; }
.sr-metrics { display: flex; flex-wrap: wrap; gap: 8dp; margin-bottom: 18dp; }
.sr-metric { flex: 1 1 28%; padding: 10dp; background-color: #263128; border-radius: 6dp; }
.sr-value { display: block; font-size: 24dp; color: #fff2ca; }
.sr-label { display: block; font-size: 13dp; color: #c4cebd; }
.sr-section { font-size: 19dp; color: #ffdc75; margin: 12dp 0dp 8dp; }
.sr-row { display: flex; align-items: center; padding: 8dp 0dp; border-bottom: 1dp #ffffff16; font-size: 15dp; }
.sr-row > div { box-sizing: border-box; min-width: 0dp; }
.sr-skill { flex: 0 0 39%; padding-right: 8dp; }
.sr-level { flex: 0 0 8%; text-align: right; }
.sr-damage { flex: 0 0 22%; text-align: right; color: #fff2ca; }
.sr-dps { flex: 0 0 17%; text-align: right; }
.sr-share { flex: 0 0 14%; text-align: right; }
.sr-columns { color: #aebda8; font-size: 13dp; }
.sr-total { color: #ffdc75; border-top: 2dp #ffdc7566; font-size: 17dp; }
.sr-track { height: 3dp; margin-top: 5dp; background-color: #ffffff12; }
.sr-bar { height: 3dp; background-color: #d8bc66; }
.sr-note { font-size: 12dp; color: #aebda8; margin-top: 12dp; }
.sr-stat { display: flex; justify-content: space-between; gap: 8dp; margin-bottom: 7dp; font-size: 15dp; }
.sr-equipment { display: flex; align-items: center; gap: 10dp; padding: 7dp 0dp; }
.sr-equipment img { width: 36dp; height: 36dp; object-fit: contain; }
.sr-actions { gap: 12dp; }
.sr-actions-first { margin-top: auto; }
.sr-actions button { flex: 1 1 0; padding: 14dp; }
@media (max-width: 800dp) {
 window content pane.survivors-results-main { padding: 10dp; flex-basis: 68%; }
 window content pane.survivors-results-side { padding: 10dp; flex-basis: 32%; }
 .sr-heading { font-size: 24dp; }
 .sr-row, .sr-stat { font-size: 12dp; }
 .sr-columns, .sr-label, .sr-note { font-size: 11dp; }
 .sr-value { font-size: 19dp; }
 .sr-equipment img { width: 26dp; height: 26dp; }
}
)";
std::string result_number(uint64_t n) {
    auto text=std::to_string(n);
    for(int i=int(text.size())-3;i>0;i-=3)text.insert(size_t(i),",");
    return text;
}
std::string result_decimal(double value,int places=1) {
    char text[48];std::snprintf(text,sizeof(text),"%.*f",places,value);return text;
}
std::string result_time(unsigned ticks) {
    char text[32];std::snprintf(text,sizeof(text),"%02u:%02u",ticks/1800,ticks/30%60);return text;
}
unsigned result_source_level(DamageSource source,const Run& run=s_run) {
    switch(source) {
    case DamageSource::DarkLink:return run.dark_link;
    case DamageSource::Sword:return run.master_sword? run.master_sword:1;
    case DamageSource::DualWield:return run.dual_wield;
    case DamageSource::FireArrows:return run.rank(Upgrade::FireArrows);
    case DamageSource::FireBomb:return run.rank(Upgrade::FireBomb);
    case DamageSource::FireEdge:return run.rank(Upgrade::FireEdge);
    case DamageSource::DarkAura:return run.rank(Upgrade::DarkAura);
    default:
        for(unsigned i=0;i<unsigned(Upgrade::Rupees);++i)
            if(weapon_damage_source(i)==source && source!=DamageSource::Other)return run.rank(Upgrade(i));
        return 0;
    }
}
std::string result_source_name(DamageSource source,const Run& run=s_run) {
    if(source==DamageSource::Spin)return run.upgrade_name(Upgrade::Spin);
    return damage_source_names[unsigned(source)];
}
DamageAmounts result_damage(const Run& run=s_run) {
    auto damage=run.damage.dealt;
    for(auto pair:std::array{std::pair{DamageSource::DualWield,DamageSource::Sword},
        std::pair{DamageSource::FireArrows,DamageSource::Bow},std::pair{DamageSource::FireBomb,DamageSource::Bombs},
        std::pair{DamageSource::BombArrow,DamageSource::Bow},std::pair{DamageSource::FireEdge,DamageSource::Sword},
        std::pair{DamageSource::DarkAura,DamageSource::DarkLink}}) {
        damage[unsigned(pair.second)]+=damage[unsigned(pair.first)];damage[unsigned(pair.first)]=0;
    }
    return damage;
}
std::string results_damage_rml(const Run& run=s_run,unsigned map=s_map) {
    const auto damageBySource=result_damage(run);
    const auto total=run.damage.total();
    const double seconds=std::max(1u,run.ticks)/double(ticks_per_second);
    std::string rml="<div class=\"sr-heading\">"+std::string(run.phase==Phase::Victory?"VICTORY":"DEFEAT")+"</div>";
    rml+="<div class=\"sr-section\">Twilit Rank "+result_decimal(run.twilit_rank(),std::floor(run.twilit_rank())==run.twilit_rank()?0:1)+"</div>";
    if(run.options.allow_cheats || run.modded) {
        const std::string labels=std::string(run.options.allow_cheats?"Cheated":"")+
            (run.options.allow_cheats && run.modded?" \xE2\x80\xA2 ":"")+(run.modded?"Modded":"");
        rml="<div class=\"sr-integrity\">"+labels+"</div>"+rml;
    }
    if(run.debug_run)rml="<div class=\"sr-integrity\">Debug — no saved scores, Survivors XP or map unlocks</div>"+rml;
    rml+="<div class=\"sr-subtitle\">"+std::string(run.endless?"Endless Mode · ":"")+treasure_rml_escape(arenas[map].name)+" \xE2\x80\xA2 "+result_time(run.ticks)+"</div>";
    rml+="<div class=\"sr-metrics\">";
    const auto metric=[&](const char* label,const std::string& value){
        rml+="<div class=\"sr-metric\"><span class=\"sr-value\">"+value+"</span><span class=\"sr-label\">"+label+"</span></div>";
    };
    metric("Total damage",result_number(total));metric("Run DPS",result_decimal(total/seconds));metric("Enemies defeated",result_number(run.kills));
    rml+="</div><div class=\"sr-section\">Damage breakdown</div>";
    rml+="<div class=\"sr-row sr-columns\"><div class=\"sr-skill\">Ability</div><div class=\"sr-level\">Lv.</div><div class=\"sr-damage\">Damage</div><div class=\"sr-dps\">DPS</div><div class=\"sr-share\">Share</div></div>";
    std::array<unsigned,damage_source_count> order{};std::iota(order.begin(),order.end(),0u);
    std::stable_sort(order.begin(),order.end(),[&](unsigned a,unsigned b){return damageBySource[a]>damageBySource[b];});
    for(unsigned i:order) {
        const auto source=DamageSource(i);const auto damage=damageBySource[i];
        const unsigned rank=result_source_level(source,run);
        if(!damage)continue;
        const double share=total?100.0*damage/total:0;
        rml+="<div class=\"sr-row\"><div class=\"sr-skill\">"+treasure_rml_escape(result_source_name(source,run))+
            "<div class=\"sr-track\"><div class=\"sr-bar\" style=\"width: "+result_decimal(share)+"%;\"></div></div></div>";
        rml+="<div class=\"sr-level\">"+(rank?number(rank):"-")+"</div><div class=\"sr-damage\">"+result_number(damage)+
            "</div><div class=\"sr-dps\">"+result_decimal(damage/seconds)+"</div><div class=\"sr-share\">"+result_decimal(share)+"%</div></div>";
    }
    if(!total)rml+="<div class=\"sr-note\">No damage dealt this run.</div>";
    rml+="<div class=\"sr-row sr-total\"><div class=\"sr-skill\">Total</div><div class=\"sr-level\"></div><div class=\"sr-damage\">"+
        result_number(total)+"</div><div class=\"sr-dps\">"+result_decimal(total/seconds)+"</div><div class=\"sr-share\">"+(total?"100%":"0%")+"</div></div>";
    rml+="<div class=\"sr-note\">Damage is enemy HP actually removed; blocked hits and overkill are excluded. DPS uses the full run time, excluding pauses. Simultaneous hits share damage proportionally. Passive effects count toward their ability. Dark Link shows its extra multiplier damage and Dark Aura; this damage is excluded from the other rows.</div>";
    return rml;
}
std::string results_loadout_rml(const HighScoreRun* archived=nullptr,unsigned map=s_map) {
    const Run& run=archived?archived->run:s_run;
    const auto score=archived?archived->score:run.score();
    const bool assisted=run.options.allow_cheats || run.modded;
    const auto best=archived?score:s_records.high_score(map,assisted,run.endless).score;
    std::string rml="<div class=\"sr-section\">Run summary</div>";
    const auto stat=[&](const char* label,const std::string& value){rml+="<div class=\"sr-stat\"><span>"+std::string(label)+"</span><span>"+value+"</span></div>";};
    stat(run.endless?"Endless Score":"Score",result_number(score));stat(assisted?"Cheated high score":"Ranked high score",result_number(best));
    stat("Difficulty","x"+result_decimal(run.options.score_percent()/100.0,2));
    stat("Level reached",number(run.level));stat("Timed elites",number(run.timed_elites)+(run.endless?"":" / 4"));
    stat("Other elites",number(run.guard_elites));stat("Chests opened",number(run.chests_opened));
    stat("XP earned",result_number(run.debug_run?0:run.endless?score-std::min(score,run.normal_score):score));stat("Meta level",number(archived?archived->meta_level:s_records.level()));
    stat("Unlock points",number(archived?archived->unlock_points:s_records.available_points()));
    rml+="<div class=\"sr-section\">Equipment</div>";
    for(unsigned i=0;i<unsigned(ChestReward::Count);++i) {
        const auto item=ChestReward(i);if(!run.owns(item))continue;
        char icon[48];std::snprintf(icon,sizeof(icon),"item://item/%02x",unsigned(treasure_item(item,run)));
        const auto name=item==ChestReward::OrdonSword?"Dual Wield":reward_name(item,run);
        rml+="<div class=\"sr-equipment\"><img src=\""+std::string(icon)+"\"/><span>"+treasure_rml_escape(name)+"</span></div>";
    }
    return rml;
}
ModResult build_results(UiElementHandle left,UiElementHandle right,const HighScoreRun* archived=nullptr,unsigned map=s_map) {
    auto result=svc_ui->elem_set_class(mod_ctx,left,"survivors-results-main",true);if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,right,"survivors-results-side",true);if(result!=MOD_OK)return result;
    UiElementHandle mainBody=0,sideBody=0;
    result=svc_ui->pane_add_rml(mod_ctx,left,results_damage_rml(archived?archived->run:s_run,map).c_str(),&mainBody);if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,mainBody,"sr-body",true);if(result!=MOD_OK)return result;
    result=svc_ui->pane_add_rml(mod_ctx,right,results_loadout_rml(archived,map).c_str(),&sideBody);if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,sideBody,"sr-body",true);if(result!=MOD_OK)return result;
    if(archived) {
        UiRowDesc desc=UI_ROW_DESC_INIT;UiElementHandle row=0;
        result=svc_ui->pane_add_row(mod_ctx,left,&desc,&row);if(result!=MOD_OK)return result;
        result=svc_ui->elem_set_class(mod_ctx,row,"sr-actions",true);if(result!=MOD_OK)return result;
        result=svc_ui->elem_set_class(mod_ctx,row,"sr-actions-first",true);if(result!=MOD_OK)return result;
        result=button(row,"Back to maps",close_high_score);if(result!=MOD_OK)return result;
        result=svc_ui->pane_add_row(mod_ctx,left,&desc,&row);if(result!=MOD_OK)return result;
        result=svc_ui->elem_set_class(mod_ctx,row,"sr-actions",true);if(result!=MOD_OK)return result;
        return button(row,"Delete Score",request_delete_score);
    }
    result=svc_ui->pane_add_text(mod_ctx,left,s_storeFailed?
        "Records could not be stored. Retry Save records or Return to hub.":
        "Save records before quitting. Return to hub keeps your progress and lets you prepare another run.",nullptr);
    if(result!=MOD_OK)return result;
    UiRowDesc rowDesc=UI_ROW_DESC_INIT;UiElementHandle row=0;
    result=svc_ui->pane_add_row(mod_ctx,left,&rowDesc,&row);if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,row,"sr-actions",true);if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,row,"sr-actions-first",true);if(result!=MOD_OK)return result;
    result=button(row,"Save records",save_results);if(result!=MOD_OK)return result;
    // Native UI rows navigate horizontally. Separate rows make Down/Up move
    // between these actions through Pane's existing vertical navigator.
    result=svc_ui->pane_add_row(mod_ctx,left,&rowDesc,&row);if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,row,"sr-actions",true);if(result!=MOD_OK)return result;
    result=button(row,"Return to hub",replay);if(result!=MOD_OK)return result;
    if(s_run.phase==Phase::Victory && !s_run.endless) {
        result=svc_ui->pane_add_row(mod_ctx,left,&rowDesc,&row);if(result!=MOD_OK)return result;
        result=svc_ui->elem_set_class(mod_ctx,row,"sr-actions",true);if(result!=MOD_OK)return result;
        return button(row,"Continue in Endless Mode",continue_endless);
    }
    return MOD_OK;
}
