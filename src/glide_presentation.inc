// Only the Cucco created by this glide session is replaced. Ordinary carried
// Cuccos keep their model, shadow and calls, including in Glider mode.
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
