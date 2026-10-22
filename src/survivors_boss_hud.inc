// A single stable encounter target: timed bosses take precedence over awakened
// chest guards. Pending, sleeping, retired and defeated actors never get a bar.
ActorId s_bossHudTarget = kNone;

void draw_run_rank_hud(float x, bool preview=false) {
    auto* font=mDoExt_getRubyFont();auto* graf=dComIfGp_getCurrentGrafPort();
    if(!font || !graf)return;
    char text[96];
    if(s_run.endless)std::snprintf(text,sizeof(text),"%02u:%02u  Endless",s_run.ticks/1800,s_run.ticks/30%60);
    else std::snprintf(text,sizeof(text),"%02u:%02u / %02u:00",s_run.ticks/1800,s_run.ticks/30%60,s_run.run_duration()/1800);
    J2DPrint clock(font,JUtility::TColor(255,240,210,255),JUtility::TColor(230,190,150,255));
    const auto timer=hud_position(HudPart::Timer,x);
    clock.setFontSize(18*timer.scale,18*timer.scale);
    clock.printReturn(text,230*timer.scale,26*timer.scale,HBIND_CENTER,VBIND_TOP,timer.x-115*timer.scale,timer.y,255);
    if(!preview && !s_run.twilit_rank())return;
    const auto counter=hud_position(HudPart::Rank,x);
    // Reuse the native map boss texture. Do not retain archive-backed pointers:
    // opening/closing the dungeon map can clear its resource cache.
    graf->setup2D();
    if(auto* archive=dComIfGp_getDmapResArchive()) {
        if(auto* texture=static_cast<ResTIMG*>(JKRGetNameResource("tt_map_icon_boss_ci8_32_00.bti",archive))) {
            J2DPicture skull(texture);
            skull.draw(counter.x-32*counter.scale,counter.y,20*counter.scale,20*counter.scale,false,false,false);
        }
    }
    graf->setup2D();
    std::snprintf(text,sizeof(text),"%g",double(preview?std::max(1.f,s_run.twilit_rank()):s_run.twilit_rank()));
    J2DPrint rank(font,JUtility::TColor(255,240,210,255),JUtility::TColor(230,190,150,255));
    rank.setFontSize(18*counter.scale,18*counter.scale);
    rank.printReturn(text,45*counter.scale,26*counter.scale,HBIND_CENTER,VBIND_TOP,counter.x-12*counter.scale,counter.y,255);
}

const char* boss_hud_name(unsigned species) {
    constexpr const char* names[] = {
        "Bokoblin", "Keese", "Rat", "Bulblin", "Bulblin Archer", "Tektite",
        "Lizalfos", "Helmasaur", "Helmasaurus", "Guay", "Kargarok", "Bomskit",
        "Stalchild", "Gibdo", "Stalfos", "Puppet", "Bubble", "Fire Bubble",
        "Black Chu", "Dynalfos", "Aeralfos", "Darknut", "Dodongo", "Stalhound",
        "Shadow Bulblin", "Shadow Bulblin Archer", "Shadow Beast", "Shadow Keese",
        "Twilit Carrier Kargarok", "Stallord", "Zant's Hand", "King Bulblin",
        "Death Sword", "Twilit Bloat", "Phantom Zant", "Darkhammer", "Puppet Zelda", "Ganondorf",
    };
    static_assert(std::size(names) == species_count);
    return species < species_count ? names[species] : "Elite";
}

Enemy* boss_hud_target() {
    auto* link = daAlink_getAlinkActorClass();
    Enemy* best = nullptr;
    float nearest = 0.0f;
    if (link) for (auto& enemy : s_enemies) {
        if (enemy.id == kNone || !enemy.elite || !enemy.initialized ||
            enemy.dormant || enemy.retiring || enemy.credited) continue;
        auto* actor = actor_by_id(enemy.id);
        if (!actor || actor->health <= 0 || actor->field_0x560 <= 0) continue;
        const float distance = actor->current.pos.abs2(link->current.pos);
        const bool timed = enemy.eliteSlot >= 0;
        const bool bestTimed = best && best->eliteSlot >= 0;
        if (!best || (timed && !bestTimed) || (timed == bestTimed &&
            (enemy.id == s_bossHudTarget || (best->id != s_bossHudTarget && distance < nearest)))) {
            best = &enemy;
            nearest = distance;
        }
    }
    s_bossHudTarget = best ? best->id : kNone;
    return best;
}

bool draw_boss_hud(dMeter2Draw_c* meter, float x, bool preview=false) {
    auto* enemy = preview ? nullptr : boss_hud_target();
    if (!preview && !enemy) return false;
    auto* actor = enemy ? actor_by_id(enemy->id) : nullptr;
    if (!preview && !actor) return false;
    const float percentage = actor ? 100.0f * float(actor->health) / float(actor->field_0x560) : 65.f;
    const auto bar=hud_position(HudPart::BossBar,x);
    if (!draw_boss_health_meter(meter, percentage, bar.x-160*bar.scale, bar.y, 320*bar.scale, 24*bar.scale)) return false;
    // Native MESSAGE_BOX_BOSS_NAME (also used by Hero's Shade) uses RubyFont.
    if (auto* font = mDoExt_getRubyFont()) {
        const char* name = enemy ? boss_hud_name(enemy->species) : "Darknut";
        const auto title=hud_position(HudPart::BossName,x);
        J2DPrint shadow(font, JUtility::TColor(0,0,0,230), JUtility::TColor(0,0,0,230));
        shadow.setFontSize(20*title.scale,20*title.scale);
        shadow.printReturn(name,550*title.scale,28*title.scale,HBIND_CENTER,VBIND_TOP,title.x-275*title.scale+1,title.y+1,255);
        J2DPrint print(font, JUtility::TColor(255,240,210,255), JUtility::TColor(230,190,150,255));
        print.setFontSize(20*title.scale,20*title.scale);
        print.printReturn(name,550*title.scale,28*title.scale,HBIND_CENTER,VBIND_TOP,title.x-275*title.scale,title.y,255);
    }
    return true;
}

void draw_victory_hud(float x) {
    auto* font=mDoExt_getRubyFont();
    if (!font) return;
    J2DPrint shadow(font,JUtility::TColor(0,0,0,230),JUtility::TColor(0,0,0,230));
    shadow.setFontSize(34,34);
    shadow.printReturn("Victory",550,44,HBIND_CENTER,VBIND_TOP,x-274,85,255);
    J2DPrint title(font,JUtility::TColor(255,240,210,255),JUtility::TColor(230,190,150,255));
    title.setFontSize(34,34);
    title.printReturn("Victory",550,44,HBIND_CENTER,VBIND_TOP,x-275,84,255);
    const unsigned remaining=s_victoryTicks>=10*ticks_per_second ? 0 :
        (10*ticks_per_second-s_victoryTicks+ticks_per_second-1)/ticks_per_second;
    char text[16]; std::snprintf(text,sizeof(text),"%u",remaining);
    J2DPrint timer(font,JUtility::TColor(255,65,55,255),JUtility::TColor(220,25,20,255));
    timer.setFontSize(25,25);
    timer.printReturn(text,550,34,HBIND_CENTER,VBIND_TOP,x-275,128,255);
}
