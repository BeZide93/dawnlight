ReachableFloor<cXyz> s_reachableFloor;
ArenaWalk s_reachableWalk;
cXyz s_reachableSample{};
bool s_reachableSamplePending=false,s_reachableSampleReady=false;
void clear_reachable_floor() {
    s_reachableFloor.clear();s_reachableWalk={};
    s_reachableSamplePending=s_reachableSampleReady=false;
}
// All five arenas now scatter Fate portals; validate their connected walking surface too.
bool needs_reachable_floor() {return s_map<5;}
bool arena_distribution_ready() {return !needs_reachable_floor() || s_reachableFloor.ready();}
float arena_distribution_area() {return needs_reachable_floor()?s_reachableFloor.floor_area():s_eliteFloorArea;}
void update_reachable_floor() {
    if(!needs_reachable_floor() || !s_arenaMeshReady || !s_arenaSpawn || s_eliteFloors.empty())return;
    if(s_reachableFloor.empty()) {
        cXyz start=s_arenaSpawnPosition;
        if(!dry_arena_floor(start,60,100))return;
        s_reachableFloor.seed(start,unsigned(s_eliteFloors.size()));
    }
    s_reachableFloor.build(s_eliteFloors,128);
    unsigned budget=64; // Shared by graph edges AND requested spawn landings.
    s_reachableFloor.expand([&](cXyz from,cXyz to){
        if(!s_reachableWalk.active)s_reachableWalk.begin(from,to,40);
        const auto result=s_reachableWalk.advance(budget);
        if(result==WalkResult::Clear && std::abs(s_reachableWalk.to.y-to.y)>=25.f)return WalkResult::Blocked;
        return result;
    });
    if(s_reachableFloor.ready() && s_reachableSamplePending) {
        const auto result=s_reachableWalk.advance(budget);
        if(result!=WalkResult::Pending) {
            s_reachableSamplePending=false;
            s_reachableSampleReady=result==WalkResult::Clear && std::abs(s_reachableWalk.to.y-s_reachableSample.y)<25.f;
            if(s_reachableSampleReady)s_reachableSample=s_reachableWalk.to;
        }
    }
}
// Hold one map-wide draw while its route is checked. Rerolling every frame
// would favor short paths near the start, even after the component is complete.
bool sample_reachable_arena_floor(cXyz& pos) {
    if(!s_reachableFloor.ready())return false;
    if(s_reachableSampleReady) {pos=s_reachableSample;s_reachableSampleReady=false;return true;}
    if(s_reachableSamplePending)return false;
    const float roll=float(s_run.next_random()&0xffffff)/16777216.f;
    const auto index=s_reachableFloor.sample(roll);
    if(!s_reachableFloor.valid(index))return false;
    const auto& tile=s_eliteFloors[index];
    float a=float(s_run.next_random()&0xffff)/65536.f,b=float(s_run.next_random()&0xffff)/65536.f;
    if(a+b>1){a=1-a;b=1-b;}
    s_reachableSample=tile.a+(tile.b-tile.a)*a+(tile.c-tile.a)*b;
    s_reachableWalk.begin(s_reachableFloor.anchor(index),s_reachableSample,40);
    s_reachableSamplePending=true;
    return false;
}
bool reachable_arena_floor(const cXyz& pos) {
    if(!needs_reachable_floor())return true;
    const auto index=s_reachableFloor.containing(pos);
    if(!s_reachableFloor.valid(index))return false;
    // Replenishing chests already check the complete walking path from Link.
    // Also reject another side of a wall inside the same large floor triangle.
    cXyz from=s_reachableFloor.anchor(index)+cXyz(0,20,0),to=pos+cXyz(0,20,0);
    dBgS_LinChk wall;wall.SetLink();wall.Set(&from,&to,nullptr);
    return !dComIfG_Bgsp().LineCross(&wall)&&arena_path_clear(from,to,40);
}
