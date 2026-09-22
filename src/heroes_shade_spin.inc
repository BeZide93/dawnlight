// Arena spin counters use the native knockback/landing and departure actions,
// but never the tutorial's parent talk flags or its group-wide end callbacks.
bool spin_cut(int cut) {
    return cut==daPy_py_c::CUT_TYPE_TURN_LEFT || cut==daPy_py_c::CUT_TYPE_TURN_RIGHT ||
        cut==daPy_py_c::CUT_TYPE_LARGE_TURN_LEFT || cut==daPy_py_c::CUT_TYPE_LARGE_TURN_RIGHT;
}
bool defeated_double(const Fighter& entry) {
    return entry.divide>0 && (sBattle.defeated_doubles & (1u<<entry.divide));
}
bool spin_action(daNpc_Kn_c* actor,Fighter& entry) {
    if (sBattle.phase!=7 || sBattle.trial!=shade::Trial::None) return false;
    if (defeated_double(entry)) {
        // Keep the native fall running even when a simultaneous boss hit starts
        // global recovery. Only depart after the landing animation finishes.
        if (actor->mActionMode!=23 && actor->mAcch.ChkGroundHit() &&
            (actor->mMotionSeqMngr.getNo()==15 || actor->mMotionSeqMngr.getNo()==19) &&
            actor->mMotionSeqMngr.getStepNo()>0) {
            actor->mActionMode=23; actor->mMode=1;
        }
        if (actor->mActionMode==23) actor->teach07_warpDelete(nullptr);
        else actor->teach07_superTurnAttackedDivide(nullptr);
        return true;
    }
    if (!entry.divide && sBattle.recovery) return false;
    auto* player=daPy_getPlayerActorClass();
    auto* hit=actor->mCylCc.GetTgHitObj();
    if (!player || !spin_cut(player->getCutType()) || !actor->mCylCc.ChkTgHit() ||
        actor->mCylCc.GetTgHitAc()!=player || !hit ||
        !hit->ChkAtType(AT_TYPE_NORMAL_SWORD|AT_TYPE_MASTER_SWORD)) return false;

    entry.offense=-1; entry.chain.cancel(); entry.helmTurnPending=false;
    entry.swordContact=false; entry.animationStarted=false; entry.blade={};
    stop_blade_sweeps(entry);
    actor->field_0x15bc=0; actor->field_0x15bd=0;
    const s16 facing=fopAcM_searchPlayerAngleY(actor);
    const s16 angle=static_cast<s16>(facing-actor->current.angle.y);
    const bool front=std::abs(static_cast<int>(angle))<0x4000;
    actor->mFaceMotionSeqMngr.setNo(1,-1,0,0);
    actor->mMotionSeqMngr.setNo(front ? 18 : 14,-1,0,0);
    actor->setAngle(static_cast<s16>(facing+(front ? 0 : 0x8000)));
    actor->speed.zero();
    actor->speedF=(front ? -1 : 1)*actor->mpHIO->m.attack_disappear_speed_h;
    actor->speed.y=actor->mpHIO->m.attack_disappear_speed_v;
    actor->mJntAnm.lookNone(0);
    actor->mSound.startCreatureVoice(Z2SE_KN_V_DAMAGE_L,-1);
    actor->mSound.startCollisionSE(Z2SE_HIT_SWORD,0x1f);
    actor->mCylCc.ClrTgHit();
    if (entry.divide) {
        sBattle.defeated_doubles|=1u<<entry.divide;
        actor->mCylCc.OffTgSetBit();
        actor->mActionMode=22; actor->mMode=1;
    } else {
        actor->mEvtNo=shade::phases[7].success;
    }
    return true;
}
