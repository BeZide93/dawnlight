// Arrows/eggs outlive their shooter. Armor, weapons, cloaks and Phantom Zant
// children retire with their owner; all share run pause, damage and teardown.
struct EnemyArrow { ActorId id=kNone; bool retiring=false; ActorId parent=kNone; bool shell=false; unsigned damageFactor=1; };
std::array<EnemyArrow, enemy_capacity*4> s_enemyArrows;
ActorSlotIndex<ActorId> s_enemyArrowIndex;
EnemyArrow* tracked_enemy_arrow(ActorId id) {
    auto* arrow=id==kNone?nullptr:s_enemyArrowIndex.find(s_enemyArrows,id);
    if(arrow && arrow->shell) {
        const auto* parent=tracked(arrow->parent);
        if(!parent||parent->retiring||parent->credited)arrow->retiring=true;
    }
    return arrow;
}

bool active_enemy_arrow(ActorId id) {
    const auto* arrow=tracked_enemy_arrow(id);
    return arrow && !arrow->retiring;
}
Enemy* enemy_child_root(ActorId id) {
    if(auto* e=tracked(id))return e;
    const auto* child=tracked_enemy_arrow(id);
    return child&&!child->retiring?tracked(child->parent):nullptr;
}
unsigned enemy_damage_factor(ActorId id) {
    if(const auto* e=tracked(id))return enemy_attack_factor(*e);
    const auto* child=tracked_enemy_arrow(id);
    return child&&!child->retiring?child->damageFactor:1;
}
bool structural_enemy_child(s16 profile) {
    return profile==fpcNm_E_RD_e||profile==fpcNm_E_MM_MT_e||profile==fpcNm_E_TH_BALL_e||profile==fpcNm_MANT_e||profile==fpcNm_E_RDY_e;
}
bool attached_enemy_child(s16 profile) {return structural_enemy_child(profile)||profile==fpcNm_E_PZ_e||profile==fpcNm_E_FK_e;}
bool owned_arrow_spawn(void* args) {
    if (!in_arena()) return false;
    const auto* enemy=enemy_child_root(mods::arg<ActorId>(args,1));if(!enemy)return false;
    const auto profile=mods::arg<s16>(args,0);
    auto* parent=actor_by_id(mods::arg<ActorId>(args,1));
    if(enemy->profile==fpcNm_E_YC_e&&parent&&fopAcM_GetName(parent)==fpcNm_E_RDY_e&&profile==fpcNm_E_ARROW_e)return true;
    if(enemy->profile==fpcNm_E_WB_e&&parent&&fopAcM_GetName(parent)==fpcNm_E_RD_e&&profile==fpcNm_E_ARROW_e)return true;
    return (enemy->profile==fpcNm_E_WB_e&&profile==fpcNm_E_RD_e) ||
        (enemy->profile==fpcNm_E_YC_e&&profile==fpcNm_E_RDY_e) ||
        (enemy->profile==fpcNm_B_GND_e&&profile==fpcNm_E_FK_e) ||
        ((enemy->profile==fpcNm_E_RD_e||enemy->profile==fpcNm_E_RDY_e)&&profile==fpcNm_E_ARROW_e) ||
        (enemy->profile==fpcNm_E_CR_e&&profile==fpcNm_E_CR_EGG_e) ||
        (enemy->profile==fpcNm_E_MM_e&&profile==fpcNm_E_MM_MT_e) ||
        (enemy->profile==fpcNm_E_TH_e&&profile==fpcNm_E_TH_BALL_e) ||
        (enemy->profile==fpcNm_B_GND_e&&profile==fpcNm_MANT_e) ||
        (enemy->profile==fpcNm_E_PZ_e&&profile==fpcNm_E_PZ_e) ||
        (enemy->profile==fpcNm_B_DS_e&&profile==fpcNm_B_DS_e&&
         ((mods::arg<u32>(args,2)&0xff)==daB_DS_c::TYPE_BULLET_B||
          (mods::arg<u32>(args,2)&0xff)==daB_DS_c::TYPE_BULLET_C));
}
HookAction before_enemy_arrow(ModContext*, void* args, void* result, void*) {
    if (!owned_arrow_spawn(args)) return HOOK_CONTINUE;
    const auto* enemy=enemy_child_root(mods::arg<ActorId>(args,1));
    const bool shell=structural_enemy_child(mods::arg<s16>(args,0));
    const auto used=std::count_if(s_enemyArrows.begin(),s_enemyArrows.end(),[](const auto& a){return a.id!=kNone;});
    // Structural children must finish asynchronous creation during menus too.
    // Reserve one armor slot per enemy; projectiles cannot exhaust that reserve.
    const bool full=used>=int(s_enemyArrows.size()-(shell?0:enemy_capacity));
    if (enemy->retiring || enemy->credited || (!shell&&s_run.phase!=Phase::Running) || full) {
        *static_cast<ActorId*>(result)=kNone;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
bool s_spawningEnemyArrows=false;
void spawn_enemy_arrow_spread(void* args) {
    if (s_spawningEnemyArrows || s_run.phase!=Phase::Running ||
        mods::arg<s16>(args,0)!=fpcNm_E_ARROW_e) return;
    auto* owner=enemy_child_root(mods::arg<ActorId>(args,1));
    if (!owner || owner->retiring || owner->credited) return;
    auto* parent=actor_by_id(mods::arg<ActorId>(args,1));
    const auto profile=parent?fopAcM_GetName(parent):owner->profile;
    if (profile!=fpcNm_E_RD_e && profile!=fpcNm_E_RDY_e) return;
    const auto* source=mods::arg<const csXyz*>(args,5);
    if (!source) return;
    // Count successful original shots only; recursive side arrows never count.
    // Enemy slot lifetime owns the cadence, so replacement archers start fresh.
    owner->arrowVolley=(owner->arrowVolley+1)%(owner->elite?2:3);
    if(owner->elite) {if(owner->profile!=fpcNm_E_YC_e && owner->profile!=fpcNm_E_WB_e && owner->arrowVolley==0)return;}
    else if (!s_run.endless && owner->arrowVolley!=0) return;
    struct SpreadScope { SpreadScope(){s_spawningEnemyArrows=true;} ~SpreadScope(){s_spawningEnemyArrows=false;} } scope;
    // Same native +/-0x900 yaw fan as enemy_slow_motion/bulblin.cpp Hard Mode,
    // every third normal shot and every Endless shot. Child hooks retain ownership/capacity.
    for (const s16 offset:{-0x900,0x900}) {
        csXyz angle=*source;angle.y=static_cast<s16>(angle.y+offset);
        fopAcM_createChild(mods::arg<s16>(args,0),mods::arg<ActorId>(args,1),mods::arg<u32>(args,2),
            mods::arg<const cXyz*>(args,3),mods::arg<int>(args,4),&angle,
            mods::arg<const cXyz*>(args,6),mods::arg<s8>(args,7),mods::arg<createFunc>(args,8));
    }
}
void after_enemy_arrow(ModContext*, void* args, void* result, void*) {
    const auto id=*static_cast<ActorId*>(result);
    if (id==kNone || !owned_arrow_spawn(args) || tracked_enemy_arrow(id)) return;
    for (auto& arrow:s_enemyArrows) if (arrow.id==kNone) {const auto* owner=enemy_child_root(mods::arg<ActorId>(args,1));
        arrow={id,false,owner->id,attached_enemy_child(mods::arg<s16>(args,0)),enemy_attack_factor(*owner)};
        s_enemyArrowIndex.add(id,std::size_t(&arrow-s_enemyArrows.data()));
        spawn_enemy_arrow_spread(args);return;}
}
void retire_enemy_arrows() {
    for (auto& arrow:s_enemyArrows) if (arrow.id!=kNone) arrow.retiring=true;
}
void cleanup_enemy_arrows() {
    for (auto& arrow:s_enemyArrows) {
        if (arrow.id==kNone) continue;
        if(arrow.shell) {
            const auto* parent=tracked(arrow.parent);
            if(!parent||parent->retiring||parent->credited)arrow.retiring=true;
        }
        auto* actor=actor_by_id(arrow.id);
        if (!actor && !fpcM_IsCreating(arrow.id)) {
            s_enemyArrowIndex.erase(arrow.id); s_bossTactics.erase(arrow.id); arrow={};
        }
        else if (actor && arrow.retiring) svc_actor->delete_actor(mod_ctx,arrow.id);
    }
}
