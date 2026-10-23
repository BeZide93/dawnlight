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
bool walkable_spawn_path(cXyz from, cXyz& to, float footprint) {
    if (!dry_arena_floor(from, 60, 100)) return false;
    const cXyz delta = to-from;
    const unsigned steps = std::max(1u, unsigned(std::ceil(delta.absXZ()/40.f)));
    cXyz previous = from;
    for (unsigned i=1; i<=steps; ++i) {
        cXyz sample = from + delta*(float(i)/steps); sample.y = previous.y;
        if (!dry_arena_floor(sample, 35, 35)) return false;
        // Sweep at ankle height, not above a low wall or a raised platform.
        cXyz a = previous+cXyz(0,20,0), b = sample+cXyz(0,20,0);
        dBgS_LinChk wall; wall.SetLink(); wall.Set(&a,&b,nullptr);
        if (dComIfG_Bgsp().LineCross(&wall) || !arena_path_clear(a,b,footprint)) return false;
        previous=sample;
    }
    to=previous;
    for (const auto& offset : std::array<cXyz,4>{cXyz(-footprint,0,0),cXyz(footprint,0,0),
            cXyz(0,0,-footprint),cXyz(0,0,footprint)}) {
        cXyz edge=to+offset;
        if (!dry_arena_floor(edge,25,25)) return false;
    }
    return true;
}
