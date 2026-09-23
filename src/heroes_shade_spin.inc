// Arena Jump Strike and spin counters use native knockback/landing/departure,
// but never the tutorial's parent talk flags or its group-wide end callbacks.
bool return_double(daNpc_Kn_c* actor,Fighter& entry) {
    if (!entry.divide || !entry.returning) return false;
    auto* boss=static_cast<daNpc_Kn_c*>(actor_by_id(sFighters[0].id));
    if (!boss) { entry.deleting=true; remove_actor(entry.id); return true; }
    if (!entry.formationTicks++) {
        entry.offense=-1; entry.chain.cancel(); entry.helmTurnPending=false;
        entry.animationStarted=false; entry.blade={}; stop_blade_sweeps(entry);
        actor->mMotionSeqMngr.setNo(9,3,1,0);
        actor->mFaceMotionSeqMngr.setNo(1,-1,0,0);
        actor->mJntAnm.lookNone(0);
        actor->field_0x15bc=0; actor->field_0x15bd=0;
        actor->field_0x170c=0; actor->field_0x170d=0;
    }
    actor->speedF=0; actor->speed.zero(); actor->mCcStts.ClrCcMove();
    actor->mCylCc.OffTgSetBit(); actor->mCylCc.ClrTgHit();
    actor->setAngle(cLib_targetAngleY(&actor->current.pos,&boss->current.pos));
    cXyz pos=actor->current.pos;
    // Four times the outward walking speed; pursue the live position rather
    // than a stale phase-change snapshot. No timer-based disappearance.
    cLib_chasePos(&pos,boss->current.pos,24.0f);
    actor->setPos(pos);
    if ((boss->current.pos-actor->current.pos).absXZ()<=1.0f) {
        entry.deleting=true;
        actor->mNoDraw=true;
        remove_actor(entry.id);
    }
    return true;
}
bool form_double(daNpc_Kn_c* actor,Fighter& entry) {
    if (!entry.divide || !entry.forming) return false;
    if (!entry.formationTicks) {
        // Actor creation is asynchronous: align with the live boss on the
        // first execution, not where he stood when the request was queued.
        if (auto* boss=static_cast<daNpc_Kn_c*>(actor_by_id(sFighters[0].id))) {
            actor->setPos(boss->current.pos);
            actor->home.pos=boss->current.pos;
            actor->home.angle.y=boss->shape_angle.y;
        }
        actor->mMode=1;
    }
    // This native formation walks 6 units/tick to mirrored 180-unit offsets.
    // Use it for both skills; the spin lesson's own setup positions actors on
    // a circle around Link, which would jump from the overlapped spawn point.
    actor->teach06_divideMove(nullptr);
    const bool arrived=(actor->mTargetPos-actor->current.pos).absXZ()<=1.0f;
    if (arrived || ++entry.formationTicks>=120) {
        entry.forming=false;
        actor->mActionMode=waiting_action(entry);
        actor->mMode=1;
    } else {
        // Vanilla leaves formation immediately when no teaching event runs.
        actor->mActionMode=sBattle.phase==6 ? 14 : 20;
    }
    return true;
}
bool spin_cut(int cut) {
    return cut==daPy_py_c::CUT_TYPE_TURN_LEFT || cut==daPy_py_c::CUT_TYPE_TURN_RIGHT ||
        cut==daPy_py_c::CUT_TYPE_LARGE_TURN_LEFT || cut==daPy_py_c::CUT_TYPE_LARGE_TURN_RIGHT;
}
bool defeated_double(const Fighter& entry) {
    return entry.divide>0 && (entry.defeatPhase>=0 || (sBattle.defeated_doubles & (1u<<entry.divide)));
}
bool skill_counter_action(daNpc_Kn_c* actor,Fighter& entry) {
    const bool jump=(entry.defeatPhase>=0 ? entry.defeatPhase : static_cast<int>(sBattle.phase))==6;
    const int departure=jump ? 17 : 23;
    if (defeated_double(entry)) {
        // Keep the native fall running even when a simultaneous boss hit starts
        // global recovery. Only depart after the landing animation finishes.
        if (actor->mActionMode!=departure && actor->mAcch.ChkGroundHit() &&
            (actor->mMotionSeqMngr.getNo()==15 || actor->mMotionSeqMngr.getNo()==19) &&
            actor->mMotionSeqMngr.getStepNo()>0) {
            actor->mActionMode=departure; actor->mMode=1;
        }
        if (actor->mActionMode==departure) {
            if (jump) actor->teach06_warpDelete(nullptr);
            else actor->teach07_warpDelete(nullptr);
        } else {
            if (jump) actor->teach06_superJumpedDivide(nullptr);
            else actor->teach07_superTurnAttackedDivide(nullptr);
        }
        return true;
    }
    if (entry.returning || (sBattle.phase!=6 && sBattle.phase!=7) ||
        sBattle.trial!=shade::Trial::None) return false;
    if (!entry.divide && sBattle.recovery) return false;
    auto* player=daPy_getPlayerActorClass();
    auto* hit=actor->mCylCc.GetTgHitObj();
    if (!player) return false;
    const int cut=player->getCutType();
    // The opening swipe remains the native small stagger. Only Jump Strike's
    // airborne strike/final blow knocks doubles down (not an ordinary jump).
    const bool counter=jump ? cut==daPy_py_c::CUT_TYPE_LARGE_JUMP ||
        cut==daPy_py_c::CUT_TYPE_LARGE_JUMP_FINISH : spin_cut(cut);
    if (!counter || !actor->mCylCc.ChkTgHit() ||
        actor->mCylCc.GetTgHitAc()!=player || !hit ||
        !hit->ChkAtType(AT_TYPE_NORMAL_SWORD|AT_TYPE_MASTER_SWORD)) return false;

    entry.offense=-1; entry.chain.cancel(); entry.helmTurnPending=false;
    entry.forming=false;
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
        entry.defeatPhase=sBattle.phase;
        sBattle.defeated_doubles|=1u<<entry.divide;
        actor->mCylCc.OffTgSetBit();
        actor->mActionMode=jump ? 16 : 22; actor->mMode=1;
    } else {
        actor->mEvtNo=shade::phases[sBattle.phase].success;
    }
    return true;
}
