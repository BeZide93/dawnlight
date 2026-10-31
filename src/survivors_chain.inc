// Native iron-ball chain geometry and material, with stable growable packets.
// Each packet owns its endpoints; no fake Link actor or shared equipment pointers.
struct SurvivorChain : J3DPacket {
    cXyz hand{}, ball{};
    unsigned detailLimit=200;
    daAlink_hsChainLight_c light{};
    void draw() override {
        if(!detailLimit)return;
        auto* data = s_loot.chainData;
        if (!s_loot.ready || !data) return;
        auto* material = data->getMaterialNodePointer(0);
        j3dSys.setVtxPos(data->getVtxPosArray(), data->getVtxNum());
        j3dSys.setVtxNrm(data->getVtxNrmArray(), data->getNrmNum());
        j3dSys.setVtxCol(data->getVtxColorArray(0), data->getColNum());
        j3dSys.setTexture(data->getTexture());
        J3DShape::resetVcdVatCache();
        material->loadSharedDL();
        material->getShape()->loadPreDrawSetting();
        GXSetChanAmbColor(GX_COLOR0A0, GXColor{u8(light.AmbCol.r), u8(light.AmbCol.g), u8(light.AmbCol.b), u8(light.AmbCol.a)});
        GXSetChanMatColor(GX_COLOR0A0, g_whiteColor);
        dKy_setLight_again(); dKy_GxFog_tevstr_set(&light);
        GXLoadLightObjImm(light.getLightObj(), GX_LIGHT0);
        const cXyz delta = ball - hand;
        const float distance = delta.abs();
        const unsigned count = std::clamp(unsigned(std::ceil(distance / 9.0f)), 1u, detailLimit);
        const s16 yaw = cM_atan2s(delta.x, delta.z);
        const s16 pitch = -cM_atan2s(delta.y, delta.absXZ());
        for (unsigned i = 0; i < count; ++i) {
            const cXyz position = hand + delta * (float(i) / count);
            mDoMtx_stack_c::copy(j3dSys.getViewMtx());
            mDoMtx_stack_c::transM(position);
            mDoMtx_stack_c::ZXYrotM(pitch, yaw, s16(i * 0x3000));
            mDoMtx_stack_c::scaleM(2, 2, 2);
            GXLoadPosMtxImm(mDoMtx_stack_c::get(), GX_PNMTX0);
            GXLoadNrmMtxImm(mDoMtx_stack_c::get(), GX_PNMTX0);
            material->getShape()->simpleDrawCache();
        }
    }
};
std::deque<SurvivorChain> s_chainPackets;
void draw_chain(unsigned slot, daAlink_c* link, cXyz ball,unsigned& remaining) {
    while (s_chainPackets.size() <= slot) s_chainPackets.emplace_back();
    auto& packet = s_chainPackets[slot];
    const auto hand = link->getLeftItemMatrix();
    packet.hand = cXyz(hand[0][3], hand[1][3], hand[2][3]); packet.ball = ball;
    packet.detailLimit=200;
    if(survivor_perf::enabled()) {
        const unsigned requested=std::clamp(unsigned(std::ceil((ball-packet.hand).abs()/9.f)),1u,
            survivor_perf::Pressure{survivor_perf::tier(),0}.chain_per_ball());
        packet.detailLimit=std::min(remaining,requested);remaining-=packet.detailLimit;
        // A continuous inexpensive strand also joins widely spaced decorative links.
        performance_line(packet.hand,ball,GXColor{185,190,200,255});
        if(!packet.detailLimit)return;
    }
    g_env_light.settingTevStruct(0, &link->current.pos, &packet.light);
    dComIfGd_getOpaListDark()->entryImm(&packet, 0);
}
