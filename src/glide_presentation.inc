// Only the Cucco created by this glide session is replaced. Ordinary carried
// Cuccos keep their model, shadow and calls, including in Glider mode.
bool custom_glider_active(daAlink_c* link) {
    if (!owns_jump_abilities(link) || !s_jumpAbilities.attached || s_jumpAbilities.retiring ||
        !glide_enabled() || glide_item() != GlideItem::Glider || !aerial_glide_context(link) ||
        link->checkStatusWindowDraw() || link->checkPlayerNoDraw()) return false;
    auto* actor = glide_actor();
    return actor && link->mGrabItemAcKeep.getActor() == actor;
}

void after_glider_draw_hand(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!custom_glider_active(link) || !link->mpLinkHandModel) return;
    auto* data = link->mpLinkHandModel->getModelData();
    if (!data || data->getMaterialNum() <= 6) return;
    auto* leftMaterial = data->getMaterialNodePointer(0);
    auto* rightMaterial = data->getMaterialNodePointer(6);
    if (!leftMaterial || !rightMaterial) return;
    auto* left = leftMaterial->getShape();
    auto* right = rightMaterial->getShape();
    if (!left || !right) return;
    // These are the native sword (left) / shield (right) grip shapes selected
    // by setDrawHand. Keep the overhead arm animation, just close the fingers.
    // Update the native draw pointers so the next setDrawHand hides our shapes
    // before choosing the ordinary pose again after landing/item switching.
    if (link->field_0x06d0) link->field_0x06d0->hide();
    if (link->field_0x06d4) link->field_0x06d4->hide();
    link->field_0x06d0 = left;
    link->field_0x06d4 = right;
    left->show();
    right->show();
}

// Cucco feathers are independent particle draws, emitted by native actor
// execute rather than draw. Keep its physics running and silence only the
// owned carrier's emitter after each native tick (also before retirement).
void suppress_glide_carrier_feathers(ni_class* actor) {
    if (!actor || s_jumpAbilities.cucco == kNoGlideActor ||
        fopAcM_GetID(actor) != s_jumpAbilities.cucco ||
        (!s_jumpAbilities.retiring && glide_item() != GlideItem::Glider)) return;
    if (auto* emitter = dComIfGp_particle_getEmitter(actor->mHaneEmitterID)) {
        emitter->setRate(0.0f);
        emitter->deleteAllParticle();
        // Do not invalidate the emitter or leave StopEmit/StopDraw flags set:
        // native hane_set restores its rate when switching back to Cucco.
    }
}

void after_glide_carrier_execute(ModContext*, void* args, void*, void*) {
    if (s_jumpAbilities.cucco == kNoGlideActor ||
        (!s_jumpAbilities.retiring && glide_item() != GlideItem::Glider)) return;
    auto* actor = static_cast<ni_class*>(glide_actor());
    if (!actor || mods::arg<void*>(args, 1) != actor || !actor->sub_method) return;
    const auto* methods = reinterpret_cast<const process_method_class*>(actor->sub_method);
#if defined(__APPLE__)
    const bool executing = mods::arg<process_method_func>(args, 0) == methods->execute_method;
#else
    const bool executing = mods::arg<const process_method_class*>(args, 0) == methods;
#endif
    // The process dispatcher also handles deletion/creation on some hosts.
    // Never inspect a deleted process or touch unrelated actors' emitters.
    if (executing) suppress_glide_carrier_feathers(actor);
}

HookAction before_glide_carrier_draw(ModContext*, void* args, void* result, void*) {
    auto* process = mods::arg<leafdraw_class*>(args, 0);
    if (process && s_jumpAbilities.cucco != kNoGlideActor &&
        fpcM_GetID(process) == s_jumpAbilities.cucco &&
        (s_jumpAbilities.retiring || glide_item() == GlideItem::Glider))
    {
        *static_cast<int*>(result) = 1;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction before_glide_carrier_sound(ModContext*, void* args, void* result, void*) {
    if (s_jumpAbilities.cucco == kNoGlideActor || glide_item() != GlideItem::Glider)
        return HOOK_CONTINUE;
    auto* actor = glide_actor();
    if (actor && mods::arg<Z2SoundObjSimple*>(args, 0) == &static_cast<ni_class*>(actor)->mSound) {
        *static_cast<Z2SoundHandlePool**>(result) = nullptr;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
