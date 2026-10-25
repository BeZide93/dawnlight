// Probe the open rest area, not the centre of the room: the centre is now
// occupied by the table and GroundCross there would return its tabletop.
bool hub_center_ground(cXyz& position) {
    auto* link = daAlink_getAlinkActorClass();
    if (!link) return false;
    cXyz probe(0, link->current.pos.y + 1000, roundtable_layout::restZ);
    dBgS_ObjGndChk ground; ground.SetPos(&probe);
    const float height = dComIfG_Bgsp().GroundCross(&ground);
    if (!std::isfinite(height) || height <= -1.e8f) return false;
    position.set(0,height,0);
    return true;
}
