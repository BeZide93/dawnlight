// All additions are owned by an encounter. Native child creation is asynchronous;
// no child is dereferenced until its Execute method becomes available.
void maintain_carrier_rider(e_yc_class* bird,const Enemy& owner) {
    if(actor_by_id(bird->mRiderID)||fpcM_IsCreating(bird->mRiderID)||owner.age%30)return;
    csXyz angle=bird->shape_angle;angle.x=0xff;
    bird->mRiderID=fopAcM_createChild(fpcNm_E_RDY_e,owner.id,0xff000207,&bird->current.pos,
        fopAcM_GetRoomNo(bird),&angle,nullptr,-1,nullptr);
}
bool prepare_carrier_rider(e_rdy_class* rider) {
    const auto* child=tracked_enemy_arrow(fopAcM_GetID(&rider->actor));
    auto* owner=child&&!child->retiring?tracked(child->parent):nullptr;
    if(!owner||owner->profile!=fpcNm_E_YC_e)return false;
    auto* bird=actor_by_id(owner->id);
    if(!bird||!fopAcM_IsExecuting(owner->id))return false;
    rider->mKargarokID=owner->id;rider->mRideState=2;rider->mDemoMode=0;
    rider->mKargarokDeleteTimer=0;rider->mFadeTimer=0;
    rider->field_0xa6e=0;
    rider->mIFrameTimer=20; // Native knock-off/death would delete the mount.
    if(rider->mAction!=0x20){rider->mAction=0x20;rider->mMode=0;}
    return true;
}
HookAction before_boss_carrier_ride(ModContext*,void* args,void*,void*) {
    auto* rider=mods::arg<e_rdy_class*>(args,0);
    if(!prepare_carrier_rider(rider))return HOOK_CONTINUE;
    // Native mounted shooting only starts while the mount hovers. Allow the
    // same animation sequence during flight, without changing the bird's pose.
    if(rider->mMode==0) {
        rider->mMode=1;rider->mTimer[0]=15;
        encounter_animation(rider->mpMorf,rider->mpArcName,0x30);
        rider->mAnm=0x30;
    }
    if(rider->mMode==1&&rider->mTimer[0]>0)--rider->mTimer[0];
    if(rider->mMode==3&&rider->mTimer[1]>0)--rider->mTimer[1];
    return HOOK_CONTINUE;
}
void after_boss_carrier_ride(ModContext*,void* args,void*,void*) {
    auto* rider=mods::arg<e_rdy_class*>(args,0);
    if(!prepare_carrier_rider(rider))return;
    rider->field_0xa6e=0; // No scripted grass/capture interlock.
}
void summon_ghost_riders(b_gnd_class* boss,const Enemy& owner) {
    auto* link=daAlink_getAlinkActorClass();if(!link)return;
    const s16 yaw=fopAcM_searchPlayerAngleY(boss);
    // Same five rider variants as the horseback light-ball split. Spawn them
    // on verified ground, in front of the boss; no horse/story actor required.
    for(unsigned i=0;i<5;++i) {
        const float side=(float(i)-2.f)*200.f;
        cXyz pos=boss->current.pos+cXyz(cM_scos(yaw)*side+cM_ssin(yaw)*200,0,
            -cM_ssin(yaw)*side+cM_scos(yaw)*200);
        if(!dry_arena_floor(pos,500,2000))continue;
        csXyz angle(0,cM_atan2s(link->current.pos.x-pos.x,link->current.pos.z-pos.z),0);
        fopAcM_createChild(fpcNm_E_FK_e,owner.id,0xffffff00|i,&pos,fopAcM_GetRoomNo(boss),&angle,nullptr,-1,nullptr);
    }
}
HookAction before_boss_gnd_charge(ModContext*,void* args,void*,void*) {
    auto* boss=mods::arg<b_gnd_class*>(args,0);auto* owner=encounter_owner(boss);
    if(!owner)return HOOK_CONTINUE;
    // Native WAIT can independently choose C. Route that entry through the
    // same cooldown, while never interrupting C's running modes 11..13.
    if(boss->mMoveMode==10) {
        if(owner->specialCooldown)boss->mMoveMode=15;
        else owner->specialCooldown=150;
    }
    return HOOK_CONTINUE;
}
void after_boss_ymb_move(ModContext*,void* args,void*,void*) {
    auto* boss=mods::arg<daE_YMB_c*>(args,0);auto* owner=encounter_owner(boss);
    if(owner&&encounter_running(*owner)&&!owner->stagger)boss->current.pos+=boss->speed*float(1.5*std::min(32767.0,s_run.endless_factor())-1.0);
}
// Repeat the heavy sword's existing model/physics independently of the armor
// phase. A separate collider is needed: native cc_set prioritizes the light
// sword and otherwise omits the thrown sword once the armor is gone.
HookAction before_boss_tn_throw(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<daB_TN_c*>(args,0);auto* owner=encounter_owner(e);
    if(!owner||!encounter_running(*owner))return HOOK_CONTINUE;
    if(owner->throwLife) {
        --owner->throwLife;
        if(!owner->throwLife||e->mStates[13]==7) {
            owner->throwLife=0;e->mStates[13]=e->mActionMode1<8?1:7;
        }
    }
    if(!owner->specialCooldown&&fopAcM_searchPlayerDistance(e)>400.f) {
        mDoMtx_stack_c::transS(e->current.pos+cXyz(0,180,0));
        mDoMtx_stack_c::YrotM(e->shape_angle.y);
        e->mpPartModels[13]->setBaseTRMtx(mDoMtx_stack_c::get());
        e->mStates[13]=2;owner->specialCooldown=90;owner->throwLife=44;
    }
    return HOOK_CONTINUE;
}
void after_boss_tn_throw_collision(ModContext*,void* args,void*,void*) {
    auto* e=mods::arg<daB_TN_c*>(args,0);auto* owner=encounter_owner(e);
    if(!owner)return;
    if(owner->extraAttackReady)owner->extraAttack.OffAtSetBit();
    if(!encounter_running(*owner)||e->mStates[13]!=3)return;
    // With no light sword, native CC already registers these positions. Disable
    // that attack for our repeat throw so one sword never damages twice.
    if(!owner->throwLife)return;
    if(e->mStates[15]!=1)e->setSwordAtBit(0);
    if(!owner->extraAttackReady) {
        static const dCcD_SrcSph source={
            {{0,{{AT_TYPE_CSTATUE_SWING,4,0xd},{0,0},0}},
             {dCcD_SE_SWORD,0,0xd,0,0},{dCcD_SE_NONE,0,0,0,0},{0}},
            {{{0,0,0},65}}};
        owner->extraStatus.Init(254,0xff,e);owner->extraAttack.Set(source);
        owner->extraAttack.SetStts(&owner->extraStatus);owner->extraAttackReady=true;
    }
    owner->extraAttack.SetC(e->mPositions[13]);owner->extraAttack.OnAtSetBit();
    dComIfG_Ccsp()->Set(&owner->extraAttack);
}
