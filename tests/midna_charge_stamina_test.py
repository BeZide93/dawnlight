"""Exercise production stamina hooks and the pinned native wolf charge procedure.

Model construction/rendering are mocked; native charge, release and movement
control flow runs unchanged. No game assets are required.
"""
from pathlib import Path
import argparse
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--dusklight-dir', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
source = (root / 'src/stamina.cpp').read_text()
native = (args.dusklight_dir / 'src/d/actor/d_a_alink_wolf.inc').read_text()


def function(text, name):
    start = re.search(r'^\w[^\n]*\b' + name + r'\([^;{}]*\)\s*\{', text, re.M).start()
    return text[start:text.index('\n}', start) + 2]


fixture = r'''
#include <cassert>
#include <algorithm>
#include <initializer_list>
using f32=float;using BOOL=bool;
constexpr bool TRUE=true,FALSE=false;
constexpr int RFLG0_UNK_10=1,Z2SE_MIDNA_BIND_AREA_SUS=2,Z2SE_WL_V_ROAR=3;
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}}
enum class StaminaSetting {MidnaAttack};
int cost=50;bool enabled=true,external=false,teExhausted=false;
float tePool=100;
int stamina_setting(StaminaSetting){return cost;}
bool stamina_enabled(){return enabled;}
bool twilit_stamina_active(){return external;}
bool twilit_stamina_available(float n){return n<=0||(!teExhausted&&tePool>=n);}
bool twilit_stamina_consume(float n){
    if(!twilit_stamina_available(n))return false;
    tePool-=n;if(n>0&&tePool==0)teExhausted=true;return true;
}
struct {float stamina=100;bool exhausted=false;} s_state;
struct Vec{};
struct fopAc_ac_c {struct {Vec pos;} current;Vec eyePos;};
struct daPy_frameCtrl_c {
    float getRate(){return 0;}void setRate(float){}void setLoop(float){}
    float getEnd(){return 30;}bool checkPass(float){return false;}
};
int cLib_targetAngleY(Vec*,Vec*){return 0;}
bool cLib_chaseF(float*,float,float){return false;}
void daAlink_searchWolfLockEnemy(){}
using fopAcIt_ExecutorFunc=void(*)();
void fopAcIt_Executor(fopAcIt_ExecutorFunc,void*){}
struct daAlink_c: fopAc_ac_c {
    struct {int field_0x3008=1;} mProcVar0;
    struct {int field_0x300a=30;} mProcVar1;
    struct {int field_0x3018=0;} mItemVar0;
    struct {int y=0;} shape_angle;
    struct {Vec pos;struct {int y=0;} angle;} current;
    struct Lock {fopAc_ac_c* getActor(){return nullptr;}void setData(fopAc_ac_c*){}} mWolfLockAcKeep[10];
    struct Hio {struct {struct {struct {struct {
        float mMaxRadius=1,mRadiusAcceleration=1;
    } m;} mWlAtLock;} mWlAttack;} mWolf;} hio;
    Hio* mpHIO=&hio;daPy_frameCtrl_c frame;daPy_frameCtrl_c* mUnderFrameCtrl=&frame;
    fopAc_ac_c* mTargetedActor=nullptr;
    int mEquipItem=0,mWolfLockNum=0,field_0x2f98=0;
    float mSearchBallScale=1,field_0x3478=0,mNormalSpeed=0;
    void* mHeldItemModel=nullptr;void* field_0x0724=nullptr;
    bool held=true,eligible=true,special=false,dead=false,scene=false;
    int constructors=0,recolors=0,movement=0,spins=0,locks=0,ordinary=0;
    bool checkDeadHP(){return dead;}bool checkSceneChangeAreaStart(){return scene;}
    bool swordButton(){return held;}bool checkWolfGroundSpecialMode(){return special;}
    bool checkWolfLockAttackChargeState(){return eligible;}
    int procWolfLockAttackInit(int){++locks;return 1;}
    int procWolfRollAttackInit(int,int){++spins;return 1;}
    int checkWolfAttackAction(){++ordinary;return 1;}
    void resetCombo(bool){}void onResetFlg0(int){}void seStartOnlyReverbLevel(int){}
    void voiceStartLevel(int){}float getWolfLieMoveAnmSpeed(){return 1;}
    void setWolfAtnMoveDirection(){}bool checkInputOnR(){return false;}
    void initBasAnime(){}void stopHalfMoveAnime(float){}
    void setSpeedAndAngleWolfAtn(){++movement;}float getWolfLieMoveSpeed(){return 8;}
    bool checkZeroSpeedF(){return false;}void onModeFlg(int){}void offModeFlg(int){}
    void setWolfLockDomeModel();int procWolfRollAttackMove();
} link;
daAlink_c* current_link(){return &link;}
// PRODUCTION
bool cosmeticsFirst=false,failConstruction=false;
void cosmetics(daAlink_c* p){
    // This models Cosmetics' assumption: its post-hook needs initialized resources.
    assert(p->mEquipItem==0x109&&p->mHeldItemModel&&p->field_0x0724);++p->recolors;
}
void daAlink_c::setWolfLockDomeModel(){
    ++constructors;
    if(!failConstruction){mEquipItem=0x109;mHeldItemModel=this;field_0x0724=this;}
    daAlink_c* owner=this;void* argv[]={&owner};
    if(cosmeticsFirst&&!failConstruction)cosmetics(this);
    after_midna_charge(nullptr,argv,nullptr,nullptr);
    if(!cosmeticsFirst&&!failConstruction)cosmetics(this);
}
// NATIVE
void reset(float amount=100){
    link={};link.mpHIO=&link.hio;link.mUnderFrameCtrl=&link.frame;
    s_state={amount,false};cost=50;enabled=true;external=false;
    tePool=amount;teExhausted=false;failConstruction=false;
}
void tick(){
    daAlink_c* owner=&link;void* argv[]={&owner};
    assert(before_midna_charge_move(nullptr,argv,nullptr,nullptr)==HOOK_CONTINUE);
    link.procWolfRollAttackMove();
}
int main(){
    for(bool te:{false,true})for(bool first:{false,true}){
        cosmeticsFirst=first;
        for(float amount:{0.0f,49.0f,50.0f,100.0f}){
            reset(amount);external=te;tick();
            const bool allowed=amount>=50;
            assert(link.constructors==int(allowed)&&link.recolors==int(allowed));
            assert(link.movement==1&&link.mNormalSpeed==8);
            assert((te?tePool:s_state.stamina)==amount-(allowed?50:0));
            for(int i=0;i<3;++i)tick(); // Holding never constructs or spends again.
            assert(link.constructors==int(allowed));
            assert((te?tePool:s_state.stamina)==amount-(allowed?50:0));
            if(!allowed){link.held=false;tick();assert(link.spins==1&&!link.locks);}
        }
        reset();external=te;s_state.exhausted=teExhausted=true;tick();
        assert(!link.constructors&&link.movement==1);
        reset(0);external=te;cost=0;s_state.exhausted=teExhausted=true;tick();
        assert(link.constructors==1&&(te?tePool:s_state.stamina)==0);
        reset(0);external=te;enabled=false;tick();
        assert(link.constructors==(te?0:1)); // TE still owns costs when our bar is Off.
    }
    for(int countdown:{0,1,2,3}){
        reset(49);link.mProcVar0.field_0x3008=countdown;tick();
        assert(!link.constructors&&link.mProcVar0.field_0x3008==std::max(0,countdown-1));
        assert(s_state.stamina==49);
    }
    reset();link.mProcVar0.field_0x3008=2;tick();assert(!link.constructors);tick();
    assert(link.constructors==1&&s_state.stamina==50);
    // Native early exits, eligibility and release logic still decide the action.
    for(int reason=0;reason<4;++reason){
        reset();
        if(reason==0)link.special=true;
        if(reason==1)link.eligible=false;
        if(reason==2)link.held=false;
        if(reason==3)link.mProcVar1.field_0x300a=0;
        tick();assert(!link.constructors&&s_state.stamina==100);
        if(reason>=2)assert(link.ordinary==1);
    }
    reset(49);link.held=false;tick();assert(link.ordinary==1&&link.mProcVar0.field_0x3008==1);
    reset(49);link.mProcVar0.field_0x3008=0;link.mWolfLockNum=1;link.held=false;
    tick();assert(link.locks==1);
    reset();failConstruction=true;tick();assert(s_state.stamina==100);
    // Partial initialization and null players must not charge stamina.
    for(int missing=0;missing<4;++missing){
        reset();link.mEquipItem=0x109;link.mHeldItemModel=link.field_0x0724=&link;
        daAlink_c* owner=&link;
        if(missing==0)link.mEquipItem=0;
        if(missing==1)link.mHeldItemModel=nullptr;
        if(missing==2)link.field_0x0724=nullptr;
        if(missing==3)owner=nullptr;
        void* argv[]={&owner};after_midna_charge(nullptr,argv,nullptr,nullptr);
        assert(s_state.stamina==100);
        if(!owner)assert(before_midna_charge_move(nullptr,argv,nullptr,nullptr)==HOOK_CONTINUE);
    }
}
'''
production = '\n'.join(function(source, n) for n in (
    'can_consume', 'try_consume', 'before_midna_charge_move', 'after_midna_charge'))
fixture = fixture.replace('// PRODUCTION', production).replace(
    '// NATIVE', function(native, 'procWolfRollAttackMove'))
assert 'add_pre<StaminaMidnaChargeHook>' not in source
assert 'add_pre<StaminaMidnaChargeMoveHook>(svc_hook, before_midna_charge_move)' in source
assert 'add_post<StaminaMidnaChargeHook>(svc_hook, after_midna_charge)' in source
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Midna stamina: native charge/release/movement, post-hook order, costs, TE routing and failed construction passed')
