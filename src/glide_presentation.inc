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
