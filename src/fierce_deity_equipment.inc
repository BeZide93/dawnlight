// Included inside fierce_deity_visual.cpp's private namespace.
// TE submits custom shields/swords and visible bow/quiver models from post
// hooks on daAlink_c::draw. The outer actor dispatcher encloses those hooks
// regardless of mod hook order. Remember instances for the deferred J3D pass;
// never infer ownership from names or shared material/model-data pointers.
struct ObservedEquipment {
    J3DModel* model = nullptr;
    J3DModelData* data = nullptr;
};
std::array<ObservedEquipment, 32> s_observedEquipment{};
daAlink_c* s_equipmentOwner = nullptr;
fpc_ProcID s_equipmentOwnerId = fpcM_ERROR_PROCESS_ID_e;
std::array<daAlink_c*, 16> s_equipmentDrawScopes{};
size_t s_equipmentDrawDepth = 0;

void reset_observed_equipment() {
    s_observedEquipment = {};
    s_equipmentOwner = nullptr;
    s_equipmentOwnerId = fpcM_ERROR_PROCESS_ID_e;
}

daAlink_c* equipment_draw_owner(void* args) {
    auto* link = daAlink_getAlinkActorClass();
    if (args == nullptr || link == nullptr || mods::arg<void*>(args, 1) != link ||
        link->sub_method == nullptr) return nullptr;
    const auto* methods = reinterpret_cast<const leafdraw_method_class*>(link->sub_method);
#if defined(__APPLE__)
    if (mods::arg<process_method_func>(args, 0) != methods->draw_method) return nullptr;
#else
    if (mods::arg<const leafdraw_method_class*>(args, 0) != methods) return nullptr;
#endif
    return link;
}

HookAction before_player_draw(ModContext*, void* args, void*, void*) {
    auto* link = equipment_draw_owner(args);
    if (link != nullptr) {
        // Also run on interpolated frames: TE still calls the model helpers
        // there, even when they do not enqueue fresh J3D packets.
        reset_observed_equipment();
        s_equipmentOwner = link;
        s_equipmentOwnerId = fopAcM_GetID(link);
    }
    if (s_equipmentDrawDepth < s_equipmentDrawScopes.size())
        s_equipmentDrawScopes[s_equipmentDrawDepth] = link;
    ++s_equipmentDrawDepth;
    return HOOK_CONTINUE;
}

void end_equipment_draw() {
    if (s_equipmentDrawDepth == 0) return;
    --s_equipmentDrawDepth;
    if (s_equipmentDrawDepth < s_equipmentDrawScopes.size())
        s_equipmentDrawScopes[s_equipmentDrawDepth] = nullptr;
}

HookAction observe_equipment_model(ModContext*, void* args, void*, void*) {
    if (args == nullptr || s_equipmentDrawDepth == 0 ||
        s_equipmentDrawDepth > s_equipmentDrawScopes.size()) return HOOK_CONTINUE;
    auto* link = s_equipmentDrawScopes[s_equipmentDrawDepth - 1];
    if (link == nullptr || link != daAlink_getAlinkActorClass() ||
        link != s_equipmentOwner || fopAcM_GetID(link) != s_equipmentOwnerId ||
        link->checkWolf() || link->checkPlayerNoDraw()) return HOOK_CONTINUE;
    auto* model = mods::arg<J3DModel*>(args, 0);
    if (model == nullptr || player_model(link, model)) return HOOK_CONTINUE;
    auto* data = model->getModelData();
    if (data == nullptr) return HOOK_CONTINUE;
    for (auto& entry : s_observedEquipment) {
        if (entry.model == model || entry.model == nullptr) {
            entry = {model, data};
            break;
        }
    }
    return HOOK_CONTINUE;
}

bool observed_player_equipment(daAlink_c* link, const J3DModel* model) {
    if (link == nullptr || model == nullptr || link != s_equipmentOwner ||
        fopAcM_GetID(link) != s_equipmentOwnerId || link->checkWolf()) return false;
    for (const auto& entry : s_observedEquipment) {
        // Only dereference the live model supplied by the current draw packet,
        // never a saved instance that TE may have retired since the last draw.
        if (entry.model == model)
            return entry.data == const_cast<J3DModel*>(model)->getModelData();
    }
    return false;
}
