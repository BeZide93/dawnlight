"""Run production Survivors combat orchestration against observable engine boundaries."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/survivors.cpp').read_text()
sdk = Path(sys.argv[1] if len(sys.argv) > 1 else root / 'dusklight')
native = (sdk / 'src/d/actor/d_a_alink_cut.inc').read_text()


def extract(name, text=source):
    match = re.search(r'^(?:void|bool|int|HookAction) ' + re.escape(name) + r'\(', text, re.M)
    assert match, name
    brace = text.index('{', match.start())
    depth = 0
    for end in range(brace, len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if not depth:
            return text[match.start():end + 1]
    raise AssertionError(name)


stubs = r'''
#include <any>
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <vector>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using u32=unsigned; using u16=unsigned short; using BOOL=int; using fpc_ProcID=unsigned;
constexpr BOOL FALSE=0,TRUE=1;
constexpr int BUTTON_STATUS_PUT_AWAY=1,BUTTON_STATUS_NONE=0;
int dComIfGp_getDoStatus(){return BUTTON_STATUS_NONE;}
constexpr unsigned kNone=~0u;
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {
template<class T> T arg(void* args,int n){return std::any_cast<T>(static_cast<std::any*>(args)[n]);}
}
struct cXyz {
    float x=0,y=0,z=0;
    float abs(const cXyz& p)const{return std::sqrt((x-p.x)*(x-p.x)+(y-p.y)*(y-p.y)+(z-p.z)*(z-p.z));}
    float absXZ()const{return std::sqrt(x*x+z*z);}
};
struct csXyz {short x=0,y=0,z=0; csXyz(short a=0,short b=0,short c=0):x(a),y(b),z(c){}};
short cM_atan2s(float a,float b){return short(std::atan2(a,b)*32768/3.14159265);}
struct cCcD_Obj {
    unsigned power=1; bool active=true,hit=true;
    unsigned GetAtAtp()const{return power;}
    void OffAtSetBit(){active=false;} void ResetAtHit(){hit=false;}
};
struct fopAc_ac_c {unsigned id=7;struct {cXyz pos;csXyz angle;}current;};
constexpr int dItemNo_NONE_e=255,dItemNo_HEART_e=0,dItemNo_GREEN_RUPEE_e=1,dItemNo_BLUE_RUPEE_e=2;
HookAction before_sword_attack(ModContext*,void*,void*,void*);
struct daAlink_c : fopAc_ac_c {
    enum {PROC_WAIT,PROC_MOVE,PROC_ATN_MOVE,PROC_ATN_ACTOR_WAIT,PROC_ATN_ACTOR_MOVE,
          PROC_WAIT_TURN,PROC_MOVE_TURN,PROC_CUT_NORMAL,PROC_CUT_TURN,PROC_CUT_FINISH,PROC_DAMAGE,PROC_JUMP,PROC_DODGE};
    int mProcID=PROC_WAIT,mEquipItem=0x103,field_0x2fde=0;
    bool wolf=false,model=true; void* mSwordModel=&model;
    struct {bool ground=true;bool ChkGroundHit(){return ground;}}mLinkAcch;
    struct Frame {float frame=0;float getFrame(){return frame;}}mUnderFrameCtrl[1];
    float field_0x3478=0,field_0x347c=10,field_0x3484=15,mNormalSpeed=0,mStickValue=0;short mMoveAngle=0;csXyz shape_angle;
    struct HIO {struct {struct {float mMaxSpeed=20;}m;}mMove;}hio;
    HIO* mpHIO=&hio;
    cCcD_Obj mAtCps[3],mAtSph;int tevStr=0;
    unsigned normals=0,spins=0,equips=0,finishes=0,mComboCutCount=0;
    enum {FLG0_UNK_1000000,FLG0_UNK_8000,DIR_FORWARD=0,DIR_BACKWARD=1};
    struct {int field_0x300a=4;}mProcVar1;
    int direction=4;bool locked=false,hit=false;
    std::vector<int> normalTypes,finishTypes,comboCounts;
    int commonCutAction();int nativeCheckCutAction();
    int checkCutAction(){
        int result=0;
        if(before_sword_attack(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL)return result;
        return nativeCheckCutAction();
    }
    void resetCombo(BOOL){mComboCutCount=0;}
    bool checkReinRide(){return false;}void procHorseCutInit(){assert(false);}
    void setDoStatus(int){}int getCutDirection(){return direction;}
    bool checkNoResetFlg0(int flag){return flag==FLG0_UNK_8000 && hit;}
    bool checkInputOnR(){return false;}bool checkAttentionLock(){return locked;}
    bool checkWolf(){return wolf;} bool checkItemSwordEquip(){return mEquipItem==0x103;}
    void swordEquip(int){++equips;field_0x2fde=0x103;}
    int procCutNormalInit(int type){normalTypes.push_back(type);comboCounts.push_back(mComboCutCount);
        ++normals;mProcID=PROC_CUT_NORMAL;mProcVar1.field_0x300a=4;
        field_0x3478=5;mUnderFrameCtrl[0].frame=0;return 1;}
    int procCutFinishInit(int type){finishTypes.push_back(type);comboCounts.push_back(mComboCutCount);
        ++finishes;mProcID=PROC_CUT_FINISH;field_0x3478=5;mUnderFrameCtrl[0].frame=0;return 1;}
    int procCutTurnInit(int quick,int dir){assert(quick==1 && dir==1);++spins;
        mProcID=PROC_CUT_TURN;field_0x3478=220;return 1;}
} player;
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
Run s_run;
enum class SwordAction {None,Normal,Spin};
SwordAction s_swordAction=SwordAction::None;
bool s_wavePending=false,s_allowDrop=false,s_active=true,arena=true,playing=true;
unsigned s_meleeCooldown=0,s_player=7,s_comboStep=0,s_comboTimeout=0;
bool s_autoSwordCall=false;
std::array<unsigned,3>s_cooldowns{};
bool in_arena(){return arena;} bool playable(){return arena && playing;}
struct Enemy {unsigned id=kNone;bool initialized=false,retiring=false,credited=false;};
std::array<Enemy,enemy_capacity>s_enemies;
fopAc_ac_c enemy;
fopAc_ac_c* actor_by_id(unsigned id){return id==enemy.id?&enemy:nullptr;}
cXyz target_direction(daAlink_c*){return {0,0,1};}
unsigned waves=0;
void launch(daAlink_c*,unsigned weapon,cXyz){assert(weapon==0);++waves;}
struct Emitter {
    int mMaxFrame=5,age=0;bool stopped=false,quit=false,invalid=false;cXyz position;
    int getAge(){return age;}void stopCreateParticle(){stopped=true;}
    void quitImmortalEmitter(){quit=true;}void becomeInvalidEmitter(){invalid=true;}
    void setGlobalTranslation(cXyz p){position=p;}
};
std::map<u32,Emitter>emitters;unsigned nextEmitter=1;
constexpr u16 ID_ZI_J_KAITENGIRI_A=11,ID_ZI_J_KAITENGIRI_B=12;
Emitter* dComIfGp_particle_getEmitter(u32 id){auto it=emitters.find(id);return it==emitters.end()?nullptr:&it->second;}
u32 dComIfGp_particle_set(u32 id,u16 effect,cXyz*,int*,const csXyz*,void*,int,void*,int,void*,void*,void*){
    assert(effect==ID_ZI_J_KAITENGIRI_A || effect==ID_ZI_J_KAITENGIRI_B);
    if(!id){id=nextEmitter++;emitters.emplace(id,Emitter{});}return id;
}
void dComIfGp_particle_levelEmitterOnEventMove(u32){}
struct Attack {bool active=true;unsigned life=10;cXyz position,velocity;cCcD_Obj collider;std::array<u32,2>emitters{};};
std::array<Attack,projectile_capacity>s_attacks;
struct dCcU_AtInfo {cCcD_Obj* mpCollider;unsigned mAttackPower=200;};
HookAction before_drop(ModContext*,void*,void*,void*);
std::vector<int>drops;
fpc_ProcID fopAcM_createItem(const cXyz*,int item,int bit,int room,void*,void*,int mode){
    fpc_ProcID result=0;
    assert(before_drop(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);
    assert(bit==-1 && room==51 && mode==0);drops.push_back(item);return 1;
}
unsigned maxLife=15,rupees=0,life=4;
void dComIfGs_setMaxLife(unsigned v){maxLife=v;}
unsigned dComIfGs_getRupeeMax(){return 300;}unsigned dComIfGs_getRupee(){return rupees;}
void dComIfGs_setRupee(unsigned v){rupees=v;}
void close_window(){}
'''
main = r'''
void reset(){
    player.mProcID=daAlink_c::PROC_WAIT;player.mEquipItem=0x103;
    player.mLinkAcch.ground=true;player.wolf=false;player.mSwordModel=&player.model;
    player.normals=player.spins=player.equips=player.finishes=player.mComboCutCount=0;
    player.normalTypes.clear();player.finishTypes.clear();player.comboCounts.clear();
    player.direction=4;player.locked=player.hit=false;
    s_comboStep=s_comboTimeout=0;s_autoSwordCall=false;
    s_run.start(1234);s_swordAction=SwordAction::None;s_wavePending=false;
    s_meleeCooldown=0;s_cooldowns={};waves=0;
    enemy.id=10;enemy.current.pos={0,0,150};s_enemies={};s_enemies[0]={10,true,false,false};
}
int main(){
    reset();update_sword(&player);
    assert(player.normals==1 && player.spins==0 && waves==0 && !s_wavePending);
    player.mUnderFrameCtrl[0].frame=6;update_sword(&player);assert(waves==0);
    // Run the native directional selector through two complete four-hit combos.
    // Recovery can clear the native count; automatic cadence must preserve it.
    for(bool recoverToIdle:{false,true}){
        reset();update_sword(&player);
        for(unsigned hit=1;hit<8;++hit){
            player.mUnderFrameCtrl[0].frame=16;
            s_meleeCooldown=0;
            if(recoverToIdle || hit%4==0){player.mProcID=daAlink_c::PROC_WAIT;player.mComboCutCount=0;}
            update_sword(&player);assert(!s_autoSwordCall);
        }
        assert((player.comboCounts==std::vector<int>{1,2,3,4,1,2,3,4}));
        assert((player.normalTypes==std::vector<int>{CUT_NM_PARAM_LEFT,CUT_NM_PARAM_RIGHT,CUT_NM_PARAM_LEFT,
                                                  CUT_NM_PARAM_LEFT,CUT_NM_PARAM_RIGHT,CUT_NM_PARAM_LEFT}));
        assert((player.finishTypes==std::vector<int>{CUT_FINISH_PARAM_VERTICAL,CUT_FINISH_PARAM_VERTICAL}));
    }
    // The third-hit collision variant is a native finisher animation, but must
    // still chain into hit four and carry its scheduled Sword Wave.
    reset();player.hit=true;update_sword(&player);
    for(unsigned i=0;i<2;++i){player.mUnderFrameCtrl[0].frame=16;s_meleeCooldown=0;update_sword(&player);}
    assert(player.mProcID==daAlink_c::PROC_CUT_FINISH && s_comboStep==3);
    s_wavePending=true;player.mUnderFrameCtrl[0].frame=5;update_sword(&player);assert(waves==1);
    player.mUnderFrameCtrl[0].frame=16;s_meleeCooldown=0;update_sword(&player);
    assert(player.mComboCutCount==4 && s_comboStep==0);
    // Final recovery is preserved, even at zero cooldown.
    player.mUnderFrameCtrl[0].frame=16;s_meleeCooldown=0;update_sword(&player);assert(player.mComboCutCount==4);
    // Attack/cancel windows, interruption, timeout, and manual-input isolation.
    reset();update_sword(&player);s_meleeCooldown=0;
    player.mUnderFrameCtrl[0].frame=10;update_sword(&player);assert(player.normals==1);
    player.mUnderFrameCtrl[0].frame=15;update_sword(&player);assert(player.normals==1);
    player.mUnderFrameCtrl[0].frame=16;update_sword(&player);assert(player.normals==2);
    player.mProcID=daAlink_c::PROC_DAMAGE;update_sword(&player);assert(s_comboStep==0);
    player.mProcID=daAlink_c::PROC_WAIT;s_meleeCooldown=0;update_sword(&player);assert(player.mComboCutCount==1);
    player.mProcID=daAlink_c::PROC_WAIT;enemy.current.pos.z=2000;
    for(unsigned i=0;i<2*ticks_per_second;++i)update_sword(&player);
    enemy.current.pos.z=150;update_sword(&player);assert(player.mComboCutCount==1);
    auto count=player.normals;assert(!player.checkCutAction() && player.normals==count);
    // Movement is restored after every native normal/spin/finish procedure.
    // It changes travel direction, while retaining the sword-facing angle.
    for(int proc:{daAlink_c::PROC_CUT_NORMAL,daAlink_c::PROC_CUT_FINISH,daAlink_c::PROC_CUT_TURN}){
        reset();player.mProcID=proc;player.shape_angle.y=123;
        s_swordAction=proc==daAlink_c::PROC_CUT_TURN?SwordAction::Spin:SwordAction::Normal;
        std::any moveArgs[]={&player};
        for(float stick:{0.0f,0.5f,1.0f})for(short angle:{short(-16000),short(16000)}){
            player.mNormalSpeed=0;player.mStickValue=stick;player.mMoveAngle=angle;
            after_sword_move(nullptr,moveArgs,nullptr,nullptr);
            assert(player.mNormalSpeed==20*stick && player.current.angle.y==angle && player.shape_angle.y==123);
        }
        for(int reason=0;reason<4;++reason){
            player.mNormalSpeed=7;player.mProcID=reason==0?daAlink_c::PROC_DAMAGE:proc;
            s_run.phase=reason==1?Phase::Upgrade:Phase::Running;
            player.mLinkAcch.ground=reason!=2;arena=reason!=3;
            after_sword_move(nullptr,moveArgs,nullptr,nullptr);assert(player.mNormalSpeed==7);
        }
        arena=true;
    }
    // No attacks against dead, retiring, pending or absent targets.
    for(int reason=0;reason<4;++reason){
        reset();s_enemies[0].credited=reason==0;s_enemies[0].retiring=reason==1;
        s_enemies[0].initialized=reason!=2;if(reason==3)s_enemies[0].id=99;
        update_sword(&player);assert(player.normals==0);
    }
    reset();enemy.current.pos.z=900;update_sword(&player);assert(player.normals==0);
    // Drawing a sheathed sword is requested once and the native equip must finish.
    reset();player.mEquipItem=dItemNo_NONE_e;player.field_0x2fde=0;
    update_sword(&player);update_sword(&player);assert(player.equips==1 && player.normals==0);
    player.mEquipItem=0x103;update_sword(&player);assert(player.normals==1);
    // Idle locomotion can attack; damage, dodges and jumps must keep their procedure.
    for(int proc:{daAlink_c::PROC_DAMAGE,daAlink_c::PROC_JUMP,daAlink_c::PROC_DODGE}){
        reset();player.mProcID=proc;s_run.ranks[0]=s_run.ranks[1]=1;update_sword(&player);
        assert(player.mProcID==proc && player.normals==0 && player.spins==0);
    }
    for(int reason=0;reason<4;++reason){
        reset();if(reason==0)player.mLinkAcch.ground=false;if(reason==1)player.wolf=true;
        if(reason==2)player.mSwordModel=nullptr;
        if(reason==3)player.mEquipItem=2;
        update_sword(&player);assert(player.normals==0);
    }
    // Wave unlock adds a real swing, then fires once at its attack frame (never on windup).
    for(unsigned rank:{1u,3u,5u}){
        reset();s_run.ranks[0]=rank;enemy.current.pos.z=900;update_sword(&player);
        assert(player.normals==1 && waves==0 && s_wavePending);
        player.mUnderFrameCtrl[0].frame=4;update_sword(&player);assert(waves==0);
        player.mUnderFrameCtrl[0].frame=5;update_sword(&player);
        unsigned expected=(rank+1)/2;assert(waves==expected && !s_wavePending);
        update_sword(&player);assert(waves==expected && player.normals==1);
    }
    reset();s_run.ranks[0]=1;update_sword(&player);player.mProcID=daAlink_c::PROC_DAMAGE;
    player.mUnderFrameCtrl[0].frame=9;update_sword(&player);assert(waves==0 && !s_wavePending);
    // Native spin has priority, receives Reach and follows the stick at normal speed.
    reset();s_run.ranks[0]=s_run.ranks[1]=1;s_run.ranks[5]=2;
    update_sword(&player);assert(player.spins==1 && player.normals==0 && waves==0);
    assert(std::abs(player.field_0x3478-286)<0.01);
    std::any args[]={&player};
    for(float stick:{0.0f,0.5f,1.0f})for(short angle:{short(-16000),short(16000)}){
        player.mStickValue=stick;player.mMoveAngle=angle;
        after_sword_move(nullptr,args,nullptr,nullptr);
        assert(player.current.angle.y==angle && player.mNormalSpeed==20*stick);
    }
    playing=false;player.mNormalSpeed=9;after_sword_move(nullptr,args,nullptr,nullptr);assert(player.mNormalSpeed==9);
    playing=true;player.id=8;after_sword_move(nullptr,args,nullptr,nullptr);assert(player.mNormalSpeed==9);player.id=7;
    BOOL large=1;assert(before_large_spin(nullptr,nullptr,&large,nullptr)==HOOK_SKIP_ORIGINAL && large==0);
    arena=false;assert(before_large_spin(nullptr,nullptr,&large,nullptr)==HOOK_CONTINUE);arena=true;
    // Only this Link's active native sword colliders receive linear Survivors damage.
    s_run.ranks[3]=2;dCcU_AtInfo info{&player.mAtSph};std::any powerArgs[]={&info};
    after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==5);
    s_swordAction=SwordAction::Normal;
    for(auto& capsule:player.mAtCps){info={&capsule};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==5);}
    s_run.master_sword=true;
    info={&player.mAtCps[0]};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==8);
    assert(s_run.attack_power(0)==7+s_run.ranks[0]);
    s_swordAction=SwordAction::Spin;info={&player.mAtSph};after_power(nullptr,powerArgs,nullptr,nullptr);
    assert(info.mAttackPower==s_run.attack_power(1));s_run.master_sword=false;
    cCcD_Obj unrelated;info={&unrelated};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==200);
    arena=false;info={&player.mAtCps[0]};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==200);arena=true;
    // Expired and missing emitters restart; all surviving emitters are invalidated on cleanup.
    Attack a;update_wave_effects(a,&player);auto first=a.emitters;
    a.position={10,20,30};update_wave_effects(a,&player);
    assert(a.emitters==first && emitters[first[0]].position.x==10);
    emitters[first[0]].age=5;emitters.erase(first[1]);update_wave_effects(a,&player);
    assert(a.emitters[0]!=first[0] && a.emitters[1]!=first[1] && emitters[first[0]].invalid);
    auto last=a.emitters;stop_attack(a);
    assert(!a.active && !a.collider.active && !a.collider.hit && a.emitters[0]==0 && a.emitters[1]==0);
    for(auto id:last)assert(emitters[id].stopped && emitters[id].quit && emitters[id].invalid);
    stop_attack(a); // Repeated cleanup is safe.
    // Native random drops stay suppressed; only scoped reward creation bypasses the hook.
    fpc_ProcID result=0;assert(before_drop(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL && result==kNone);
    bool seen[3]{};
    for(int i=0;i<1000;++i){drop_reward(&enemy);assert(!s_allowDrop);}
    for(auto item:drops){assert(item>=0 && item<=2);seen[item]=true;}
    assert(seen[0] && seen[1] && seen[2]);
    arena=false;assert(before_drop(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);arena=true;
    // Heart Container raises capacity only; fallback currency clamps to the wallet.
    reset();unsigned choice=0;s_run.phase=Phase::Upgrade;s_run.offers[0]=Upgrade::Heart;
    select_upgrade(nullptr,&choice);assert(maxLife==20 && life==4);
    s_run.phase=Phase::Upgrade;s_run.offers[0]=Upgrade::Rupees;rupees=290;
    select_upgrade(nullptr,&choice);assert(rupees==300 && life==4);
    std::cout<<"Survivors combat: native four-hit combos, sword movement, wave timing, particles and rewards passed\n";
}
'''
functions = ['stop_attack', 'update_wave_effects', 'sword_action_available', 'enemy_in_range',
             'before_cut', 'before_sword_attack', 'normal_sword_proc', 'begin_sword_combo',
             'release_sword_wave', 'update_sword', 'after_sword_move', 'before_large_spin',
             'native_sword_collider', 'after_power', 'before_drop', 'drop_reward', 'select_upgrade']
# Execute Dusklight's actual combo counter/selector, not a reimplementation of
# the sequence. Animation setup remains an observable boundary above.
native_combo = native[native.index('enum daAlink_CutNmParamType'):native.index('static fopAc_ac_c*')]
native_combo_functions = extract('daAlink_c::commonCutAction', native) + '\n' + extract(
    'daAlink_c::checkCutAction', native).replace('daAlink_c::checkCutAction', 'daAlink_c::nativeCheckCutAction')
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    binary = Path(tmp) / 'test'
    cpp.write_text(native_combo + stubs + native_combo_functions + '\n'.join(extract(name) for name in functions) + '\n' + main)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root/'src'),
                    str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
