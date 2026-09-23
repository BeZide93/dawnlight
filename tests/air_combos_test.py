"""Exercise the production air-combo hooks with an instrumented native API.

Native action initializers reset flags/gravity as in commonProcInit; native
attack animation/collision implementations remain the host's responsibility.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
jump = (root / 'src/jump_hooks.cpp').read_text()
shield = (root / 'src/manual_shield_hooks.cpp').read_text()
air = (root / 'src/air_combos.inc').read_text()
air = re.sub(r'^DEFINE_HOOK.*\n', '', air, flags=re.M)
air = air[:air.index('ModResult install_air_combo_hooks()')]


def function(text, name, result):
    start = text.index(f'{result} {name}(')
    return text[start:text.index('\n}', start) + 2] + '\n'


fixture = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cmath>
using BOOL=int;
constexpr int TRUE=1,FALSE=0,PAD_1=0;
constexpr unsigned kSwordItem=0x103;
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
struct daAlink_c;
struct Args {daAlink_c* link;int next=0;};
namespace mods {
template<class T> T arg(void* args,int index) {
    if constexpr (std::is_pointer_v<T>) {assert(index==0);return static_cast<Args*>(args)->link;}
    else {assert(index==1);return static_cast<T>(static_cast<Args*>(args)->next);}
}
}
namespace daPy_py_c {enum {RFLG0_UNK_2=2,RFLG0_UNK_40=0x40,FLG0_WATER_IN_MOVE=1};}
bool enabled=true,rJump=true,event=false,bTrigger=false,rHeld=false,shieldEnabled=true;
bool locked=false,lHeld=false,jumpReady=true,interaction=false,wall=false,chain=false;
int rStatus=0,forceRStatus=0;
constexpr int BUTTON_STATUS_NONE=0;
int dComIfGp_getRStatus(){return rStatus;}
int dComIfGp_getRStatusForce(){return forceRStatus;}
bool action_prompt_context_active(){return interaction;}
bool front_wall_context_active(daAlink_c*){return wall;}
bool target_or_shield_context_active(daAlink_c*){return locked||lHeld;}
bool chain_context_active(daAlink_c*){return chain;}
bool switch_target_active(daAlink_c*){return locked;}
bool air_combos_enabled(){return enabled;}
bool r_jump_enabled(){return rJump;}
bool manual_shielding_enabled(){return shieldEnabled;}
bool dComIfGp_event_runCheck(){return event;}
namespace mDoCPd_c {
bool getTrigB(int){return bTrigger;}
bool getHoldB(int){return bTrigger;}
bool getHoldLockR(int){return rHeld;}
bool getHoldLockL(int){return lHeld;}
}
int active_jump_binding(){return 0;}
bool jump_held(int){return rHeld;}
void set_manual_jump_direction(daAlink_c*){}
void apply_manual_jump_movement(daAlink_c*){}
bool jump_state_ready(daAlink_c*){return jumpReady;}
int bulletClears=0,bulletStarts=0;
void clear_manual_jump(daAlink_c*){++bulletClears;}
void mark_manual_jump_started(daAlink_c*){++bulletStarts;}
struct cXyz {
    float x=0,y=0,z=0;
    cXyz operator-(const cXyz& other)const{return {x-other.x,y-other.y,z-other.z};}
    float absXZ()const{return std::sqrt(x*x+z*z);}
};
std::int16_t cLib_targetAngleY(const cXyz* from,const cXyz* to) {
    return static_cast<std::int16_t>(static_cast<int>(std::atan2(to->x-from->x,to->z-from->z)*32768/3.14159265f));
}
struct fopAc_ac_c {
    cXyz eyePos;
    struct {cXyz pos;struct {std::int16_t y=0;} angle;} current;
};
struct daAlink_c : fopAc_ac_c {
    enum daAlink_PROC {PROC_AUTO_JUMP,PROC_CUT_NORMAL,PROC_CUT_FINISH,PROC_CUT_REVERSE,
        PROC_FALL,PROC_LAND,PROC_DAMAGE,PROC_DEMO,PROC_WOLF,PROC_WAIT,PROC_CUT_JUMP};
    enum {MODE_JUMP=2};
    daAlink_PROC mProcID=PROC_AUTO_JUMP;
    struct {bool grounded=false;bool ChkGroundHit()const{return grounded;}} mLinkAcch;
    struct {float y=-12;} speed;
    struct {int mCutTurnChargeCheckTimer=0;} mProcVar5;
    float gravity=-3,maxFallSpeed=-40,mNormalSpeed=20,mFallHeight=250;
    bool specialGravity=false,wolf=false,heavy=false,water=false,comboBuffer=false,collision=true;
    bool localEvent=false;
    bool attentionLock=false;
    fopAc_ac_c* mTargetedActor=nullptr;
    bool checkAttentionLock()const{return attentionLock;}
    unsigned flags=MODE_JUMP,mEquipItem=kSwordItem,mComboCutCount=0,resetFlags=0;
    int cutCalls=0,jumpCalls=0,landCalls=0,fallCalls=0;
    bool checkWolf()const{return wolf;}
    bool checkEventRun()const{return localEvent;}
    bool checkBootsOrArmorHeavy()const{return heavy;}
    bool checkNoResetFlg0(int)const{return water;}
    bool swordSwingTrigger()const{return bTrigger;}
    bool checkComboReserb()const{return comboBuffer;}
    void clearComboReserb(){comboBuffer=false;}
    void resetCombo(int){mComboCutCount=0;}
    void resetAtCollision(int){collision=false;}
    void offResetFlg0(int flag){resetFlags&=~flag;}
    void onModeFlg(int flag){flags|=flag;}
    void initGravity(){gravity=-3;maxFallSpeed=-40;specialGravity=false;}
    void setSpecialGravity(float g,float max,int off){gravity=g;maxFallSpeed=max;specialGravity=!off;}
    void commonProcInit(daAlink_PROC);
    int checkCutAction();
    int procFallInit(int,float);
    int checkLandAction(int);
    int procCutJumpInit(int);
    int procAutoJumpInit(int);
};
const daAlink_c* s_manualJumpOwner=nullptr;
HookAction before_common_proc_init(ModContext*,void*,void*,void*);
void after_air_common_proc_init(ModContext*,void*,void*,void*);
'''

native = r'''
void daAlink_c::commonProcInit(daAlink_PROC next) {
    Args args{this,next};before_common_proc_init(nullptr,&args,nullptr,nullptr);
    mProcID=next;flags=(next==PROC_AUTO_JUMP || next==PROC_FALL) ? MODE_JUMP : 0;
    initGravity();
    after_air_common_proc_init(nullptr,&args,nullptr,nullptr);
}
int daAlink_c::checkCutAction(){
    ++mComboCutCount;++cutCalls;
    commonProcInit(mComboCutCount==4 ? PROC_CUT_FINISH : PROC_CUT_NORMAL);
    collision=true;return 1;
}
int daAlink_c::procFallInit(int mode,float){assert(mode==0);++fallCalls;commonProcInit(PROC_FALL);return 1;}
int daAlink_c::checkLandAction(int){++landCalls;commonProcInit(PROC_LAND);return 1;}
int daAlink_c::procCutJumpInit(int){++jumpCalls;commonProcInit(PROC_CUT_JUMP);return 1;}
int daAlink_c::procAutoJumpInit(int){commonProcInit(PROC_AUTO_JUMP);return 1;}
'''

checks = r'''
void reset(daAlink_c& link) {
    link={};s_manualJumpOwner=&link;s_airCombo={};enabled=rJump=true;event=false;
    bTrigger=true;rHeld=false;bulletClears=0;
}
int main() {
    daAlink_c link;Args args{&link};int result=0;
    // Feature off preserves R+B Jump Attack, enabled starts without holding ZR.
    reset(link);enabled=false;rHeld=true;
    assert(start_air_jump_attack(&link));assert(link.jumpCalls==1 && !s_airCombo.active);
    reset(link);assert(start_air_jump_attack(&link));
    assert(link.cutCalls==1 && !link.jumpCalls && s_airCombo.active && s_airCombo.used);
    assert(bulletClears==1 && link.speed.y==-0.8f && link.specialGravity);
    assert(link.flags & daAlink_c::MODE_JUMP);assert(link.mFallHeight==250);
    assert(link.mNormalSpeed==6);
    // One native four-hit string, preserving buffered input and finisher.
    bTrigger=false;
    for(unsigned strike=2;strike<=4;++strike) {
        link.comboBuffer=true;args.next=1;
        assert(before_air_next_action(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(result==1 && link.mComboCutCount==strike && s_airCombo.strikes==strike);
        assert(!link.comboBuffer && link.specialGravity && link.mFallHeight==250);
    }
    assert(link.mProcID==daAlink_c::PROC_CUT_FINISH);
    bTrigger=true;args.next=1;
    before_air_next_action(nullptr,&args,&result,nullptr);assert(!result && link.cutCalls==4);
    args.next=0;before_air_next_action(nullptr,&args,&result,nullptr);
    assert(result && link.fallCalls==1 && !s_airCombo.active && s_airCombo.used);
    assert(link.gravity==-3 && !link.specialGravity && !link.collision);
    assert(s_manualJumpOwner==&link);
    assert(before_air_cut_in_fly(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL && !result);
    assert(link.cutCalls==4 && link.jumpCalls==0); // B spam cannot buy another hover
    link.commonProcInit(daAlink_c::PROC_LAND);assert(!s_manualJumpOwner);
    // A locked enemy beyond ordinary jump height draws Link diagonally upward.
    // Calling physics repeatedly in one frame must not accumulate lift or move
    // position directly; native posMove consumes the requested velocity once.
    reset(link);fopAc_ac_c high;high.eyePos={300,480,400};
    link.mTargetedActor=&high;link.attentionLock=true;
    assert(start_air_combo(&link));
    assert(s_airCombo.tracking && link.speed.y>0 && link.mNormalSpeed>0 && link.gravity==0);
    const float firstY=link.speed.y,firstH=link.mNormalSpeed;
    const auto firstYaw=link.current.angle.y;
    for(int i=0;i<5;++i)apply_air_combo_physics(&link);
    assert(link.speed.y==firstY && link.mNormalSpeed==firstH && link.current.angle.y==firstYaw);
    assert(link.current.pos.x==0 && link.current.pos.y==0 && link.current.pos.z==0);
    for(int tick=0;tick<80;++tick) {
        assert(before_air_cut(nullptr,&args,&result,nullptr)==HOOK_CONTINUE);
        assert(std::hypot(link.speed.y,link.mNormalSpeed)<=12.001f);
        const float yaw=link.current.angle.y*3.14159265f/32768;
        link.current.pos.x+=link.mNormalSpeed*std::sin(yaw);
        link.current.pos.z+=link.mNormalSpeed*std::cos(yaw);
        link.current.pos.y+=link.speed.y+link.gravity;
    }
    assert(link.current.pos.y>380 && link.current.pos.y<400);
    assert((high.eyePos-link.current.pos).absXZ()>=84.9f);
    assert((high.eyePos-link.current.pos).absXZ()<87);
    assert(s_airCombo.ticks==80); // Pursuit never extends the shared budget.
    // A follow-up and moving target retain the remaining budget and chase its
    // latest position. A lower target draws Link down rather than up.
    high.eyePos={-300,200,-100};link.comboBuffer=true;args.next=1;bTrigger=false;
    before_air_next_action(nullptr,&args,&result,nullptr);
    assert(s_airCombo.ticks==80 && link.speed.y<0 && link.mNormalSpeed>0);
    assert(link.current.angle.y<0);
    // Directly overhead needs no horizontal divide; in melee reach neither
    // axis overshoots, oscillates or pushes Link through the target.
    high.eyePos={link.current.pos.x,link.current.pos.y+400,link.current.pos.z};
    apply_air_combo_physics(&link);assert(link.speed.y>0 && link.mNormalSpeed==0);
    high.eyePos={link.current.pos.x,link.current.pos.y+80,link.current.pos.z};
    apply_air_combo_physics(&link);assert(link.speed.y==0 && link.mNormalSpeed==0);
    // Loss of lock or removal of the target cancels upward/forward drive.
    for(int reason=0;reason<2;++reason) {
        reset(link);link.attentionLock=true;link.mTargetedActor=&high;
        high.eyePos={0,800,500};assert(start_air_combo(&link));assert(link.speed.y>0);
        if(reason==0)link.attentionLock=false;else link.mTargetedActor=nullptr;
        apply_air_combo_physics(&link);
        assert(!s_airCombo.tracking && link.speed.y<=0 && link.mNormalSpeed==0 && link.gravity<0);
    }
    // An available actor without active lock-on is never pulled toward.
    reset(link);link.mTargetedActor=&high;assert(start_air_combo(&link));
    assert(!s_airCombo.tracking && link.speed.y<0);
    // Pursuit cancels on interruption and expiry, restoring native falling.
    for(int reason=0;reason<4;++reason) {
        reset(link);link.attentionLock=true;link.mTargetedActor=&high;
        assert(start_air_combo(&link));assert(link.speed.y>0);
        if(reason==0)link.commonProcInit(daAlink_c::PROC_DAMAGE);
        if(reason==1)link.commonProcInit(daAlink_c::PROC_CUT_REVERSE);
        if(reason==2){enabled=false;before_air_cut(nullptr,&args,&result,nullptr);}
        if(reason==3){s_airCombo.ticks=kAirComboTicks;before_air_cut(nullptr,&args,&result,nullptr);}
        assert(!s_airCombo.tracking && !link.specialGravity && link.speed.y<=0);
        assert(link.mNormalSpeed==0);
    }
    // Native fallback is only intercepted for our manual jump.
    reset(link);s_manualJumpOwner=nullptr;
    assert(before_air_cut_in_fly(nullptr,&args,&result,nullptr)==HOOK_CONTINUE);
    reset(link);enabled=false;
    assert(before_air_cut_in_fly(nullptr,&args,&result,nullptr)==HOOK_CONTINUE);
    reset(link);
    assert(before_air_cut_in_fly(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL && result);
    assert(link.cutCalls==1 && !rHeld);
    // Ascent receives no extra lift; the fall slowdown starts at the apex.
    reset(link);link.speed.y=12;assert(start_air_combo(&link));
    assert(link.speed.y==12 && link.gravity==-3 && !link.specialGravity);
    link.speed.y=-2;after_air_cut(nullptr,&args,nullptr,nullptr);
    assert(link.speed.y==-0.8f && link.specialGravity);
    // Budget cannot reset when a follow-up initializes another native action.
    reset(link);assert(start_air_combo(&link));bTrigger=false;
    for(int tick=0;tick<kAirComboTicks;++tick) {
        assert(before_air_cut(nullptr,&args,&result,nullptr)==HOOK_CONTINUE);
        if(tick==20){link.comboBuffer=true;args.next=1;before_air_next_action(nullptr,&args,&result,nullptr);}
        link.mProcVar5.mCutTurnChargeCheckTimer=3;link.resetFlags=daPy_py_c::RFLG0_UNK_40;
        after_air_cut(nullptr,&args,nullptr,nullptr);
        assert(!link.mProcVar5.mCutTurnChargeCheckTimer && !link.resetFlags);
    }
    assert(before_air_cut(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL);
    assert(!s_airCombo.active && !link.specialGravity && link.fallCalls==1);
    // Grounding/disable/item swap/heavy state/water restore physics and exit.
    for(int reason=0;reason<6;++reason) {
        reset(link);assert(start_air_combo(&link));
        if(reason==0)link.mLinkAcch.grounded=true;
        if(reason==1)enabled=false;
        if(reason==2)rJump=false;
        if(reason==3)link.mEquipItem=0;
        if(reason==4)link.heavy=true;
        if(reason==5)link.water=true;
        assert(before_air_cut(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(!s_airCombo.active && !link.specialGravity && !link.collision);
        assert(reason==0 ? link.landCalls==1 : link.fallCalls==1);
    }
    // Damage/demo/transformation leave their native initializers in control.
    for(auto proc:{daAlink_c::PROC_DAMAGE,daAlink_c::PROC_DEMO,daAlink_c::PROC_WOLF}) {
        reset(link);assert(start_air_combo(&link));link.commonProcInit(proc);
        assert(!s_airCombo.active && !s_manualJumpOwner && !link.specialGravity && link.mProcID==proc);
    }
    // Recoil keeps its native animation/velocity but receives no hover or follow-up.
    reset(link);assert(start_air_combo(&link));link.commonProcInit(daAlink_c::PROC_CUT_REVERSE);
    assert(s_airCombo.active && s_airCombo.recoil && !link.specialGravity);
    assert(link.flags & daAlink_c::MODE_JUMP);
    args.next=1;bTrigger=true;link.comboBuffer=true;
    before_air_next_action(nullptr,&args,&result,nullptr);assert(!result && link.cutCalls==1);
    args.next=0;before_air_next_action(nullptr,&args,&result,nullptr);
    assert(!s_airCombo.active && link.fallCalls==1);
    // Admission guards do not touch unrelated/invalid actors or reset physics.
    for(int reason=0;reason<8;++reason) {
        reset(link);
        if(reason==0)link.mLinkAcch.grounded=true;
        if(reason==1)link.wolf=true;
        if(reason==2)link.heavy=true;
        if(reason==3)link.water=true;
        if(reason==4)event=true;
        if(reason==5)link.localEvent=true;
        if(reason==6)link.mEquipItem=0;
        if(reason==7)s_manualJumpOwner=nullptr;
        assert(!start_air_combo(&link) && !link.cutCalls && !bulletClears && !link.specialGravity);
    }
    // Simultaneous ZR+B starts a regular jump with the option on, old Jump Attack off.
    reset(link);link.mLinkAcch.grounded=true;rHeld=true;
    assert(start_ground_jump(&link));assert(!link.jumpCalls && bulletStarts>0 && !s_airCombo.used);
    reset(link);enabled=false;assert(start_ground_jump(&link));assert(link.jumpCalls==1);
    // Lock-on is relaxed only by this option; world interactions still win.
    reset(link);locked=true;assert(!r_action_context_active(&link));
    enabled=false;assert(r_action_context_active(&link));enabled=true;
    for(int reason=0;reason<4;++reason) {
        rStatus=reason==0;forceRStatus=reason==1;interaction=reason==2;chain=reason==3;
        assert(r_action_context_active(&link));
    }
    rStatus=forceRStatus=0;interaction=chain=false;
    // Shield cannot swallow held-ZR+B in this airborne session; grounded/manual
    // and feature-off shielding retain the existing behavior.
    reset(link);rHeld=true;locked=true;
    assert(!manual_shield_button(&link));
    link.mLinkAcch.grounded=true;assert(manual_shield_button(&link));
    link.mLinkAcch.grounded=false;enabled=false;assert(manual_shield_button(&link));
}
'''

source = fixture + air + function(jump, 'before_common_proc_init', 'HookAction')
source += function(jump, 'start_ground_jump', 'bool') + function(jump, 'start_air_jump_attack', 'bool')
source += function(jump, 'r_action_context_active', 'bool') + function(jump, 'air_combo_jump_active', 'bool')
source += function(shield, 'manual_shield_button', 'bool') + native + checks
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'air_combos.cpp'
    exe = Path(tmp) / 'air_combos'
    cpp.write_text(source)
    subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Air Combos: input routes, native chain, 3D target pursuit, finite airtime, landing, recoil, cleanup and shield guards passed')
