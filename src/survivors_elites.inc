// The first slots belong to the five-minute schedule, the rest to map guards.
// Actor IDs are checked on every spawn pass; failed creation/unload is retried
// without treating a deletion as a kill or issuing another chest.
struct EliteEncounter {
    bool placed = false, defeated = false, rewarded = false;
    unsigned respawnAt = 0;
    cXyz position{};
    s16 yaw = 0;
};
std::array<EliteEncounter, elite_capacity> s_eliteEncounters;
void reset_elite_encounters() { s_eliteEncounters = {}; }

bool elite_floor_position(cXyz& pos) {
    if (!dry_arena_floor(pos,250,240)) return false;
    // A doubled Bokoblin / large chest must fit on real floor, away from exits.
    constexpr float offsets[4][2]{{-100,-100},{100,-100},{100,100},{-100,100}};
    for (const auto& offset : offsets) {
        cXyz corner = pos + cXyz(offset[0], 0, offset[1]);
        if (!dry_arena_floor(corner,45,45)) return false;
        cXyz from = pos + cXyz(0, 100, 0), to = corner + cXyz(0, 100, 0);
        dBgS_LinChk wall; wall.Set(&from, &to, nullptr);
        if (dComIfG_Bgsp().LineCross(&wall) || !arena_path_clear(from,to,100)) return false;
    }
    return true;
}
// Flying enemies can die thousands of units above their landing. Project first,
// then apply the same dry-floor, footprint, wall and arena-exit checks as ground loot.
bool loot_floor_position(cXyz& pos) {
    cXyz floor=pos;
    if (!dry_arena_floor(floor,250,20000) || !elite_floor_position(floor)) return false;
    pos=floor;return true;
}
bool separated_guard(cXyz pos, float distance) {
    for (unsigned i=timed_elite_count; i<s_eliteEncounters.size(); ++i)
        if (s_eliteEncounters[i].placed && s_eliteEncounters[i].position.absXZ(pos) < distance) return false;
    return true;
}
bool random_arena_floor(cXyz& pos) {
    if(needs_reachable_floor()) {
        if(!sample_reachable_arena_floor(pos))return false;
        const cXyz sample=pos;const bool accepted=elite_floor_position(pos);
        record_arena_spawn_sample(sample,pos,accepted);
        return accepted;
    }
    if (s_eliteFloors.empty() || s_eliteFloorArea <= 0) return false;
    const float roll = float(s_run.next_random() & 0xffffff) / 16777216.f * s_eliteFloorArea;
    auto tile = std::lower_bound(s_eliteFloors.begin(),s_eliteFloors.end(),roll,
        [](const EliteFloor& f,float value){return f.cumulative < value;});
    if (tile == s_eliteFloors.end()) tile = s_eliteFloors.end()-1;
    float a = float(s_run.next_random() & 0xffff)/65536.f;
    float b = float(s_run.next_random() & 0xffff)/65536.f;
    if (a+b>1) {a=1-a;b=1-b;}
    pos = tile->a + (tile->b-tile->a)*a + (tile->c-tile->a)*b;
    return elite_floor_position(pos) && reachable_arena_floor(pos);
}
bool random_guard_position(cXyz& pos) {
    auto* link=daAlink_getAlinkActorClass();
    if (!link || !random_arena_floor(pos) || !separated_guard(pos,1800)) return false;
    // The connected-floor sampler allows detours around hills and walls.
    // Sample the whole room, but keep guards away from both the original start
    // and the player's current location, including when a guard respawns.
    const cXyz start=s_arenaSpawn ? s_arenaSpawnPosition : link->current.pos;
    if (pos.absXZ(start)<3000 || pos.absXZ(link->current.pos)<1800) return false;
    // Require an open landing in every direction, not just a 200-unit footprint.
    // Local paths preserve wall/cliff/water checks without requiring a straight
    // route across the entire map. Narrow entrance tunnels cannot qualify.
    for (unsigned i=0;i<8;++i) {
        const s16 angle=s16(i*0x2000);
        cXyz edge=pos+cXyz(cM_ssin(angle)*600,0,cM_scos(angle)*600);
        if (!walkable_spawn_path(pos,edge,100)) return false;
    }
    return true;
}
bool guard_floor_near(cXyz anchor, float radius, cXyz& pos) {
    // Interpolate actual floor triangles, instead of assuming an authored
    // chest/door's Y equals its floor. Search landings around steps/door walls.
    for (unsigned ring=0;ring<=unsigned(radius/80);++ring) {
        const unsigned count=ring ? 16 : 1;
        for (unsigned direction=0;direction<count;++direction) {
            const s16 yaw=s16(direction*0x1000);
            const float x=anchor.x+cM_ssin(yaw)*float(ring)*80;
            const float z=anchor.z+cM_scos(yaw)*float(ring)*80;
            bool found=false; float highest=-1.e30f;
            for (const auto& tile : s_eliteFloors) {
                const cXyz u=tile.b-tile.a,v=tile.c-tile.a;
                const float determinant=u.x*v.z-u.z*v.x;
                if (std::abs(determinant)<1) continue;
                const float a=((x-tile.a.x)*v.z-(z-tile.a.z)*v.x)/determinant;
                const float b=(u.x*(z-tile.a.z)-u.z*(x-tile.a.x))/determinant;
                if (a<0 || b<0 || a+b>1) continue;
                cXyz candidate(x,tile.a.y+a*u.y+b*v.y,z);
                if (candidate.y<=highest || std::abs(candidate.y-anchor.y)>1000 || !separated_guard(candidate,500)) continue;
                if (elite_floor_position(candidate)) {pos=candidate;highest=candidate.y;found=true;}
            }
            if (found) return true; // Prefer the landing over overlapping floor underneath it.
        }
    }
    return false;
}
bool landmark_guard_position(const EliteLandmark& site, cXyz& pos) {
    // Prefer the exterior door face. Check both faces for map-only Door records
    // whose angle is reversed; GroundCross still requires the arena's room.
    for (const s16 yaw : {site.yaw,s16(site.yaw+0x8000)}) {
        const cXyz anchor=site.position+cXyz(cM_ssin(yaw)*450,0,cM_scos(yaw)*450);
        if (guard_floor_near(anchor,640,pos)) return true;
    }
    return false;
}
void update_elite_spawns(daAlink_c* link) {
    if (s_run.phase != Phase::Running) return;
    if (s_map==3) refresh_elite_sites();
    const unsigned guards = !s_run.endless && s_run.ticks >= s_run.run_duration() ? 0 : s_map==1 || s_map==2 ? 4 :
        s_map==3 ? std::min<unsigned>(guard_capacity,s_eliteDoorSites.size()) : 0;
    for (unsigned i=0;i<timed_elite_count+guards;++i) {
        auto& encounter = s_eliteEncounters[i];
        const bool guard = i>=timed_elite_count;
        if (encounter.defeated && ((!guard && !s_run.endless) || s_run.ticks < encounter.respawnAt)) continue;
        if (!guard && s_run.ticks < (s_run.endless?encounter.respawnAt:(i+1)*s_run.boss_interval())) continue;
        bool exists = false;
        for (const auto& enemy : s_enemies) if (enemy.id!=kNone && enemy.eliteSlot==int(i)) {exists=true;break;}
        if (exists) continue;
        if (!guard) {
            if (!spawn_enemy(link,true,int(i))) ++s_spawnFailures;
            else if(s_run.endless) {
                encounter.defeated=false;encounter.rewarded=false;
            }
            continue;
        }
        const bool relocate = encounter.defeated && (s_map==1 || s_map==2);
        cXyz pos=encounter.position;
        s16 yaw=encounter.yaw;
        if (!encounter.placed || relocate) {
            const unsigned n=i-timed_elite_count;
            bool found=false;
            if (s_map==1 || s_map==2) {
                for (unsigned attempt=0;attempt<64 && !found;++attempt) found=random_guard_position(pos);
            } else if (s_map==3) {
                found=landmark_guard_position(s_eliteDoorSites[n],pos);
            }
            if (!found) {++s_spawnFailures;continue;}
            yaw=s16(s_run.next_random());
        }
        if (create_enemy_at(pos,yaw,unsigned(guard_species(s_map)),true,int(i),true)) {
            encounter.position=pos; encounter.yaw=yaw; encounter.placed=true;
            encounter.defeated=false; encounter.respawnAt=0;
        } else ++s_spawnFailures;
    }
}
// Skip only the idle AI, never Execute: native damage, animation, physics and
// collision registration continue, so a resting guard remains hittable.
HookAction before_bokoblin_wait(ModContext*, void* args, void*, void*) {
    auto* actor=mods::arg<daE_OC_c*>(args,0);
    auto* enemy=actor ? tracked(fopAcM_GetID(actor)) : nullptr;
    if (!in_arena() || !enemy || !enemy->dormant || enemy->retiring || enemy->credited) return HOOK_CONTINUE;
    if (!enemy->idleAnimation) {actor->setBck(0x18,2,5.f,1.f);enemy->idleAnimation=true;}
    actor->speedF=0; actor->speed.x=actor->speed.z=0;
    return HOOK_SKIP_ORIGINAL;
}
