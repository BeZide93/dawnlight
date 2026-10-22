// Native hit objects are borrowed pointers. GetAtHitAc/GetTgHitAc cache an
// actor pointer, so their non-null result does not prove that the collider is
// still alive (notably in Link's checkElecReturnDamage).
struct ContactPeer {
    cCcD_Obj* collider = nullptr;
    fopAc_ac_c* actor = nullptr;
    ActorId id = kNone;
};
std::unordered_map<dCcD_GObjInf*, ContactPeer> s_contactPeers;
// Read hit identity from Link's own native storage, never from the borrowed
// attack object or its potentially cached/freed actor pointer.
struct ContactFields : dCcD_GObjInf { using dCcD_GObjInf::mGObjTg; };
const auto& native_target_peer(dCcD_GObjInf& collider) {
    return collider.*&ContactFields::mGObjTg;
}

template<class F> void visit_player_contacts(F&& visit) {
    auto* actor = actor_by_id(s_player);
    auto* link = daAlink_getAlinkActorClass();
    if (link && actor == link) {
        for (auto& collider : link->mAtCps) visit(collider);
        for (auto& collider : link->mTgCyls) visit(collider);
        visit(link->mAtSph); visit(link->mAtCyl); visit(link->mGuardAtCps);
    }
    // Include inactive slots: stopping an attack is not destruction of its
    // collider, and native code may still retain its previous contact.
    for (auto& attack : s_attacks) visit(attack.collider);
}

// Read only the owned collider's base hit pointer. The dCcD getters consult
// cached actor state; Get*HitGObj additionally dereferences the borrowed peer.
cCcD_Obj* raw_attack_peer(dCcD_GObjInf& collider) {
    return collider.cCcD_ObjHitInf::GetAtHitObj();
}
cCcD_Obj* raw_target_peer(dCcD_GObjInf& collider) {
    return collider.cCcD_ObjHitInf::GetTgHitObj();
}
bool live_contact_peer(cCcD_Obj* raw, const ContactPeer& peer) {
    return raw && raw == peer.collider && peer.id != kNone &&
        peer.actor && actor_by_id(peer.id) == peer.actor;
}
bool live_native_target_peer(dCcD_GObjInf& collider) {
    const auto& peer=native_target_peer(collider);
    if (peer.mApid==kNone) return collider.ChkTgHitNoActor();
    auto* actor=actor_by_id(peer.mApid);
    return actor && (!peer.mAc || peer.mAc==actor);
}
void validate_player_contacts() {
    visit_player_contacts([](dCcD_GObjInf& collider) {
        const auto found = s_contactPeers.find(&collider);
        const ContactPeer empty{};
        const auto& peer = found == s_contactPeers.end() ? empty : found->second;
        if (raw_attack_peer(collider) && !live_contact_peer(raw_attack_peer(collider), peer))
            collider.ResetAtHit();
        // Missing observer bookkeeping must not cancel accepted native damage.
        if (raw_target_peer(collider) && !live_native_target_peer(collider))
            collider.ResetTgHit();
    });
}
void clear_player_contacts() {
    visit_player_contacts([](dCcD_GObjInf& collider) {
        collider.ResetAtHit(); collider.ResetTgHit();
    });
    s_contactPeers.clear();
}
HookAction before_player_contacts(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (in_arena() && link && actor_by_id(s_player) == link) validate_player_contacts();
    return HOOK_CONTINUE;
}

void after_player_contact(ModContext*, void* args, void*, void*) {
    if (!in_arena()) return;
    auto* attack = mods::arg<cCcD_Obj*>(args, 1);
    auto* target = mods::arg<cCcD_Obj*>(args, 2);
    if (!attack || !target) return;
    auto* link = daAlink_getAlinkActorClass();
    if (!link || actor_by_id(s_player) != link ||
        attack->GetAc() != link) return;
    // Both arguments are alive during collision dispatch. Record only accepted
    // contacts, including our skip-original multihit and boss-block paths.
    visit_player_contacts([&](dCcD_GObjInf& collider) {
        const auto remember = [](cCcD_Obj* peer) {
            auto* actor = peer->GetAc();
            return ContactPeer{peer, actor, actor ? fopAcM_GetID(actor) : kNone};
        };
        if (&collider == attack && raw_attack_peer(collider) == target)
            s_contactPeers[&collider] = remember(target);
    });
}

HookAction before_contact_owner_delete(ModContext*, void* args, void*, void*) {
    if (!in_arena() && s_contactPeers.empty()) return HOOK_CONTINUE;
    auto* process = mods::arg<void*>(args, 1);
    if (!process || !is_actor_process(process)) return HOOK_CONTINUE;
    auto* actor = static_cast<fopAc_ac_c*>(process);
    const auto* methods = reinterpret_cast<const process_method_class*>(actor->sub_method);
    if (!methods || mods::arg<process_method_func>(args, 0) != methods->delete_method)
        return HOOK_CONTINUE;
    const auto id = fopAcM_GetID(actor);
    // Run before the native destructor, including untracked child actors such
    // as Helmasaur armor and enemy arrows. Never inspect a dying peer's vtable.
    visit_player_contacts([&](dCcD_GObjInf& collider) {
        auto found = s_contactPeers.find(&collider);
        if (found != s_contactPeers.end() && found->second.id == id) {
            collider.ResetAtHit(); found->second = {};
        }
        if (native_target_peer(collider).mApid == id) collider.ResetTgHit();
    });
    if (id == s_player) s_contactPeers.clear();
    return HOOK_CONTINUE;
}
