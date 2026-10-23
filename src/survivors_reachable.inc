ReachableFloor<cXyz> s_reachableFloor;
unsigned s_reachableQueries=0;
void clear_reachable_floor() {s_reachableFloor.clear();s_reachableQueries=0;}
bool needs_reachable_floor() {return s_map==1 || s_map==2 || s_map==4;}
void update_reachable_floor() {
    s_reachableQueries=8;
    if(!needs_reachable_floor() || !s_arenaMeshReady || !s_arenaSpawn)return;
    if(s_reachableFloor.empty()) {
        cXyz start=s_arenaSpawnPosition;
        if(!dry_arena_floor(start,60,100))return;
        s_reachableFloor.seed(start);
    }
    // Fixed work budget shared by the whole frame, independent of spawn retries.
    s_reachableFloor.expand(16,[](cXyz from,cXyz& to){return walkable_spawn_path(from,to,40);});
}
bool reachable_arena_floor(const cXyz& pos) {
    if(!needs_reachable_floor())return true;
    return s_reachableFloor.contains(pos,[](cXyz from,cXyz& to){
        if(!s_reachableQueries)return false;
        --s_reachableQueries;
        return walkable_spawn_path(from,to,40);
    });
}
