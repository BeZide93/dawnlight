// GroundCross alone accepts riverbeds and isolated ledges. Check water at the
// actual floor, so bridges above water remain usable.
bool dry_arena_floor(cXyz& pos, float rise, float drop) {
    cXyz probe = pos + cXyz(0, rise, 0);
    dBgS_ObjGndChk ground; ground.SetPos(&probe);
    const float height = dComIfG_Bgsp().GroundCross(&ground);
    if (!std::isfinite(height) || height < -1.e8f || height-pos.y > rise ||
        pos.y-height > drop || dComIfG_Bgsp().GetRoomId(ground) != arena().room) return false;
    const int code = dComIfG_Bgsp().GetGroundCode(ground);
    if (code == 4 || code == 5 || code == 10) return false;
    cXyz bottom(pos.x, height-5, pos.z);
    dBgS_WtrChk water; water.Set(bottom, height+5000);
    if (dComIfG_Bgsp().WaterChk(&water) && water.GetHeight() > height+1) return false;
    pos.y = height;
    return true;
}
// One bounded sample performs at most one ground, water and wall query.
// Long paths resume next frame instead of hiding unbounded work in one "path".
struct ArenaWalk {
    cXyz from{},to{},previous{},delta{};
    float footprint=40;
    unsigned steps=0,sample=0,corner=0;
    bool active=false;
    void begin(cXyz start,cXyz target,float radius) {
        from=previous=start;to=target;delta=to-from;footprint=radius;
        steps=std::max(1u,unsigned(std::ceil(delta.absXZ()/40.f)));
        sample=corner=0;active=true;
    }
    WalkResult advance(unsigned& budget) {
        while(budget) {
            --budget;
            if(sample==0) {
                if(!dry_arena_floor(previous,60,100)){active=false;return WalkResult::Blocked;}
                from=previous;delta=to-from;++sample;continue;
            }
            if(sample<=steps) {
                cXyz point=from+delta*(float(sample)/steps);point.y=previous.y;
                if(!dry_arena_floor(point,35,35)){active=false;return WalkResult::Blocked;}
                cXyz a=previous+cXyz(0,20,0),b=point+cXyz(0,20,0);
                dBgS_LinChk wall;wall.SetLink();wall.Set(&a,&b,nullptr);
                if(dComIfG_Bgsp().LineCross(&wall)||!arena_path_clear(a,b,footprint)) {active=false;return WalkResult::Blocked;}
                previous=point;++sample;continue;
            }
            const cXyz offsets[4]{cXyz(-footprint,0,0),cXyz(footprint,0,0),cXyz(0,0,-footprint),cXyz(0,0,footprint)};
            cXyz edge=previous+offsets[corner];
            if(!dry_arena_floor(edge,25,25)){active=false;return WalkResult::Blocked;}
            if(++corner==4){active=false;to=previous;return WalkResult::Clear;}
        }
        return WalkResult::Pending;
    }
};
bool walkable_spawn_path(cXyz from,cXyz& to,float footprint) {
    ArenaWalk path;path.begin(from,to,footprint);unsigned budget=~0u;
    if(path.advance(budget)!=WalkResult::Clear)return false;
    to=path.to;return true;
}
