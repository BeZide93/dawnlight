// Reuse the ceremonial oil bowl, not the narrow L9Candle torch stand.
// Native creation/deletion own Obj_otubo and its models. Only our instance loses
// the oil-refill interaction and liquid drawing, and gains a decorative fire.
ActorId s_hubBrazier = kNone;
bool s_hubBrazierRetiring = false;
unsigned s_hubBrazierRetry = 0;
constexpr float kHubBrazierScale = 2.f;
std::array<u32,3> s_hubBrazierEmitters{};
LIGHT_INFLUENCE s_hubBrazierLight{};
bool s_hubBrazierLightSet = false;
void stop_hub_brazier_fire() {
    for (auto& id : s_hubBrazierEmitters) {
        if (id) if (auto* emitter = dComIfGp_particle_getEmitter(id)) {
            emitter->stopCreateParticle();
            emitter->quitImmortalEmitter();
            emitter->becomeInvalidEmitter();
        }
        id = 0;
    }
    if (s_hubBrazierLightSet) dKy_plight_cut(&s_hubBrazierLight);
    s_hubBrazierLightSet = false;
}
bool owned_hub_brazier(daObj_Oiltubo_c* actor) {
    return actor && s_hubBrazier != kNone && fopAcM_GetID(actor) == s_hubBrazier;
}
HookAction before_hub_brazier_execute(ModContext*, void* args, void* result, void*) {
    auto* bowl = mods::arg<daObj_Oiltubo_c*>(args,0);
    if (!owned_hub_brazier(bowl)) return HOOK_CONTINUE;
    // Also handles Execute invoked by initialize() during native creation.
    // Never run setParam() (which resets scale) or wait() (which offers oil).
    bowl->attention_info.flags = 0;
    bowl->scale.set(kHubBrazierScale,kHubBrazierScale,kHubBrazierScale);
    bowl->setMtx();
    fopAcM_setCullSizeBox2(bowl,bowl->mpModel->getModelData());
    if (!s_hubBrazierRetiring) {
        bowl->mCcCoCyl.SetCoSPrm(0x79);
        bowl->mCcCoCyl.OffTgSetBit();
        bowl->mCcCoCyl.OffAtSetBit();
        bowl->mCcCoCyl.SetC(bowl->current.pos);
        bowl->mCcCoCyl.SetR(50.f*kHubBrazierScale);
        bowl->mCcCoCyl.SetH(70.f*kHubBrazierScale);
        dComIfG_Ccsp()->Set(&bowl->mCcCoCyl);
        // The separate oil-surface model supplies the rim's actual local height.
        const float surface = bowl->mpBModel->getModelData()->getJointNodePointer(0)->getMax()->y;
        cXyz fire = bowl->current.pos;
        fire.y += surface*kHubBrazierScale;
        const cXyz scale(kHubBrazierScale,kHubBrazierScale,kHubBrazierScale);
        constexpr u16 effects[]{0x3ad,0x3af,0x3ae}; // Common FireWood2 glow, fire and sparks.
        for (unsigned i=0;i<s_hubBrazierEmitters.size();++i)
            s_hubBrazierEmitters[i] = dComIfGp_particle_set(s_hubBrazierEmitters[i],effects[i],
                &fire,nullptr,nullptr,&scale,0xff,nullptr,-1,nullptr,nullptr,nullptr);
        s_hubBrazierLight.mPosition = fire;
        s_hubBrazierLight.mPosition.y += 10.f*kHubBrazierScale;
        if (!s_hubBrazierLightSet) {
            s_hubBrazierLight.mColor = {0xbc,0x66,0x42,0xff};
            s_hubBrazierLight.mPow = 700.f;
            s_hubBrazierLight.mFluctuation = 1.f;
            dKy_plight_set(&s_hubBrazierLight);
            s_hubBrazierLightSet = true;
        }
        mDoAud_seStartLevel(Z2SE_OBJ_FIRE_BURNING,&fire,0,dComIfGp_getReverb(fopAcM_GetRoomNo(bowl)));
    }
    *static_cast<int*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_hub_brazier_draw(ModContext*, void* args, void* result, void*) {
    auto* bowl = mods::arg<daObj_Oiltubo_c*>(args,0);
    if (!owned_hub_brazier(bowl)) return HOOK_CONTINUE;
    g_env_light.settingTevStruct(8,&bowl->current.pos,&bowl->tevStr);
    g_env_light.setLightTevColorType_MAJI(bowl->mpModel,&bowl->tevStr);
    mDoExt_modelUpdateDL(bowl->mpModel); // Container only; no animated oil surface.
    if (!s_hubBrazierRetiring) {
        cXyz probe = bowl->current.pos;
        probe.y += 100.f;
        dBgS_ObjGndChk ground; ground.SetPos(&probe);
        const float height = dComIfG_Bgsp().GroundCross(&ground);
        if (std::isfinite(height) && height > -1.e8f)
            dComIfGd_setSimpleShadow(&bowl->current.pos,height,50.f*kHubBrazierScale,
                ground,0,1.f,dDlst_shadowControl_c::getSimpleTex());
    }
    *static_cast<int*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_hub_brazier_delete(ModContext*, void* args, void*, void*) {
    if (owned_hub_brazier(mods::arg<daObj_Oiltubo_c*>(args,0))) stop_hub_brazier_fire();
    return HOOK_CONTINUE; // Native resources and sound still need deletion.
}
fopAc_ac_c* hub_brazier_actor() {
    auto* actor = actor_by_id(s_hubBrazier);
    return actor && fopAcM_GetName(actor) == fpcNm_OBJ_OILTUBO_e ? actor : nullptr;
}
bool hub_brazier_anchor(cXyz& position) {
    auto* actor = hub_brazier_actor();
    if (!actor || s_hubBrazierRetiring) return false;
    position = actor->current.pos;
    return true;
}
// Available as soon as the room's collision and Link exist. Decorative actors
// can use this together instead of waiting for each other's resource loads.
bool hub_center_ground(cXyz& position) {
    auto* link = daAlink_getAlinkActorClass();
    if (!link) return false;
    cXyz probe(0, link->current.pos.y + 1000, 0);
    dBgS_ObjGndChk ground; ground.SetPos(&probe);
    const float height = dComIfG_Bgsp().GroundCross(&ground);
    if (!std::isfinite(height) || height <= -1.e8f) return false;
    position.set(0,height,0);
    return true;
}
void update_hub_brazier(bool enabled) {
    if (!enabled) {
        s_hubBrazierRetiring = true; s_hubBrazierRetry = 0;
        stop_hub_brazier_fire();
    }
    if (s_hubBrazier != kNone && !fpcM_IsCreating(s_hubBrazier)) {
        if (!hub_brazier_actor()) {
            stop_hub_brazier_fire();
            s_hubBrazier = kNone;
            s_hubBrazierRetiring = false;
        } else if (s_hubBrazierRetiring) svc_actor->delete_actor(mod_ctx, s_hubBrazier);
    }
    // Keep ownership during asynchronous creation/deletion, including departure.
    if (!enabled || s_hubBrazier != kNone) return;
    s_hubBrazierRetiring = false;
    if (s_hubBrazierRetry) {--s_hubBrazierRetry; return;}
    cXyz center;
    if (!hub_center_ground(center)) return;
    const ActorSpawnParams params{
        .parameters=0, .argument=-1, .room_num=arenas[0].room,
        .position={center.x,center.y,center.z}, .angle={0,0,0},
        .scale={kHubBrazierScale,kHubBrazierScale,kHubBrazierScale}, .create_function=nullptr,
    };
    ActorId id = kNone;
    if (create_standalone_actor(fpcNm_OBJ_OILTUBO_e, params, id) == MOD_OK && id != kNone)
        s_hubBrazier = id;
    else s_hubBrazierRetry = 30; // Back off only after an actual spawn failure.
}
