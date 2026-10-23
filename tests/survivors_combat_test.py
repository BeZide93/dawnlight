"""Run production Survivors combat orchestration against observable engine boundaries."""
from pathlib import Path
import re
import os
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/survivors.cpp').read_text()
sdk = Path(sys.argv[1] if len(sys.argv) > 1 else root / 'dusklight')
native = (sdk / 'src/d/actor/d_a_alink_cut.inc').read_text()


def extract(name, text=source):
    match = re.search(r'^(?:void|bool|int|HookAction) ' + re.escape(name) + r'\([^;{}]*\)\s*\{', text, re.M)
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
#include <span>
#include <iostream>
#include <map>
#include <vector>
#include "survivors_rules.hpp"
#include "survivors_multistrike.hpp"
using namespace dawnlight::survivors;
MultiStrikeState s_multiStrike;
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
    cXyz operator*(float f)const{return {x*f,y*f,z*f};}
    cXyz operator+(cXyz p)const{return {x+p.x,y+p.y,z+p.z};}
    cXyz operator-(cXyz p)const{return {x-p.x,y-p.y,z-p.z};}
    float inprod(cXyz p)const{return x*p.x+y*p.y+z*p.z;}
    float abs()const{return std::sqrt(x*x+y*y+z*z);}
    float absXZ()const{return std::sqrt(x*x+z*z);}
};
struct csXyz {short x=0,y=0,z=0; csXyz(short a=0,short b=0,short c=0):x(a),y(b),z(c){}};
short cM_atan2s(float a,float b){return short(std::atan2(a,b)*32768/3.14159265);}
struct cCcD_Obj {
    cXyz start,end;void SetStartEnd(cXyz a,cXyz b){start=a;end=b;}
    float radius=0;void SetR(float r){radius=r;}
    unsigned power=1; bool active=true,hit=true;
    unsigned GetAtAtp()const{return power;}
    void OffAtSetBit(){active=false;} void ResetAtHit(){hit=false;}
};
struct fopAc_ac_c {unsigned id=7;int health=10;cXyz eyePos;struct {cXyz pos;csXyz angle;}current;};
constexpr int fpcNm_E_YMB_e=101,fpcNm_B_DS_e=102;
struct dCcD_Sph {
 cXyz center;float radius=0;bool target=true;
 const cXyz& GetC()const{return center;}float GetR()const{return radius;}
 bool ChkTgSet()const{return target;}
};
struct daE_YMB_c:fopAc_ac_c{dCcD_Sph field_0x82c[2],field_0x1324;} bloat;
struct daB_DS_c:fopAc_ac_c{dCcD_Sph mHeadSph[5],mBreathAtSph;} stallord;
constexpr int dItemNo_NONE_e=255,dItemNo_HEART_e=0,dItemNo_GREEN_RUPEE_e=1,dItemNo_BLUE_RUPEE_e=2,dItemNo_YELLOW_RUPEE_e=3,dItemNo_RED_RUPEE_e=4,dItemNo_PURPLE_RUPEE_e=5,dItemNo_ORANGE_RUPEE_e=6,dItemNo_SILVER_RUPEE_e=7;
HookAction before_sword_attack(ModContext*,void*,void*,void*);
struct daAlink_c : fopAc_ac_c {
    enum {PROC_WAIT,PROC_MOVE,PROC_ATN_MOVE,PROC_ATN_ACTOR_WAIT,PROC_ATN_ACTOR_MOVE,
          PROC_WAIT_TURN,PROC_MOVE_TURN,PROC_CUT_NORMAL,PROC_CUT_TURN,PROC_CUT_FINISH,PROC_DAMAGE,PROC_JUMP,PROC_DODGE,PROC_CUT_LARGE_JUMP,PROC_CUT_LARGE_JUMP_LAND};
    int mProcID=PROC_WAIT,mEquipItem=0x103,field_0x2fde=0;
    bool swordHeld=false,swordPressed=false;bool swordButton(){return swordHeld;}bool swordTrigger(){return swordPressed;}
    struct{int mCutTurnChargeCheckTimer=0;}mProcVar5;
    bool wolf=false,model=true; void* mSwordModel=&model;
    struct {bool ground=true;bool ChkGroundHit(){return ground;}}mLinkAcch;
    struct Frame {float frame=0,rate=1;float getFrame(){return frame;}float getRate(){return rate;}void setRate(float r){rate=r;}}mUnderFrameCtrl[1],mUpperFrameCtrl[1];
    cXyz field_0x3498{},field_0x34bc{},mSwordTopPos{0,70,200},field_0x34b0{0,70,190};float field_0x33d0=1;
    int field_0x3198=2;
    int procCutLargeJumpInit(){mProcID=PROC_CUT_LARGE_JUMP;return 1;}
    float field_0x3478=0,field_0x347c=10,field_0x3484=15,mNormalSpeed=0,mStickValue=0;short mMoveAngle=0;csXyz shape_angle;
    struct HIO {struct {struct {float mMaxSpeed=20;}m;}mMove;struct{struct{struct{float mAttackRadius=260,mLargeAttackRadius=400;}m;}mCutTurn;}mCut;}hio;
    HIO* mpHIO=&hio;
    cCcD_Obj mAtCps[3],mAtSph,mGuardAtCps;int tevStr=0;
    u32 field_0x3204[6]{};
    unsigned normals=0,spins=0,equips=0,finishes=0,mComboCutCount=0;
    enum {FLG0_UNK_1000000,FLG0_UNK_8000,DIR_FORWARD=0,DIR_BACKWARD=1};
    struct {int field_0x300a=4;}mProcVar1;
    struct {short field_0x300c=0;}mProcVar2;
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
    bool equipping=false;bool checkEquipAnime(){return equipping;}
    void swordEquip(int){++equips;field_0x2fde=0x103;equipping=true;}
    int procCutNormalInit(int type){normalTypes.push_back(type);comboCounts.push_back(mComboCutCount);
        ++normals;mProcID=PROC_CUT_NORMAL;mProcVar1.field_0x300a=4;
        field_0x3478=5;mUnderFrameCtrl[0]={};mUpperFrameCtrl[0]={};return 1;}
    int procCutFinishInit(int type){finishTypes.push_back(type);comboCounts.push_back(mComboCutCount);
        ++finishes;mProcID=PROC_CUT_FINISH;field_0x3478=5;mUnderFrameCtrl[0]={};mUpperFrameCtrl[0]={};return 1;}
    int procCutTurnInit(int quick,int dir){assert(quick==1 && dir==1);++spins;
        mProcID=PROC_CUT_TURN;field_0x3478=220;field_0x347c=130;return 1;}
} player;
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}
int rewardRoom=51;
int fopAcM_GetRoomNo(fopAc_ac_c*){return rewardRoom;}
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
Run s_run;
enum class SwordAction {None,Normal,Spin,Jump};
SwordAction s_swordAction=SwordAction::None;
bool s_waveManual=false,s_manualSwing=false;
bool s_wavePending=false,s_allowDrop=false,s_active=true,arena=true,playing=true;
unsigned s_meleeCooldown=0,s_player=7,s_comboStep=0,s_comboTimeout=0;
bool s_autoSwordCall=false,s_trackMelee=false;unsigned s_meleeTarget=kNone;
std::array<unsigned,unsigned(Upgrade::Rupees)>s_cooldowns{};
bool in_arena(){return arena;} bool playable(){return arena && playing;}
struct Enemy {unsigned id=kNone;bool initialized=false,retiring=false,credited=false;int profile=0;};
std::array<Enemy,enemy_capacity>s_enemies;
Enemy* tracked(unsigned id){for(auto& e:s_enemies)if(e.id==id&&e.initialized)return &e;return nullptr;}
bool abilityBusy=false;bool jump_abilities_busy_for(daAlink_c*){return abilityBusy;}
fopAc_ac_c enemy,enemy2;
fopAc_ac_c potActor; bool potCarried=false;
struct ArenaPot{unsigned id=kNone;bool initialized=false,retiring=false,broken=false;};
std::array<ArenaPot,1>s_pots;
bool fopAcM_checkCarryNow(fopAc_ac_c*){return potCarried;}bool fopAcM_checkHookCarryNow(fopAc_ac_c*){return false;}
fopAc_ac_c* actor_by_id(unsigned id){return id==bloat.id?&bloat:id==stallord.id?&stallord:id==enemy.id?&enemy:id==enemy2.id?&enemy2:id==potActor.id?&potActor:nullptr;}
struct dBgS_LinChk {void Set(const cXyz*,const cXyz*,daAlink_c*){}cXyz GetCross(){return {0,70,250};}};
bool blocked=false;
struct World{bool LineCross(dBgS_LinChk*){return blocked;}}world;
World& dComIfG_Bgsp(){return world;}
float cM_ssin(short a){return std::sin(a*3.14159265f/32768);}float cM_scos(short a){return std::cos(a*3.14159265f/32768);}
struct Camera{struct{cXyz center{100,0,0},eye{};cXyz Center(){return center;}cXyz Eye(){return eye;}}mCamera;}camera;
bool cameraPresent=true;Camera* dComIfGp_getCamera(int){return cameraPresent?&camera:nullptr;}
bool sword_aim_target(daAlink_c* link,cXyz& target,cXyz& forward){
 if(!cameraPresent)return false;
 forward=camera.mCamera.center-camera.mCamera.eye;
 const float length=forward.abs();if(length<.001f)return false;forward=forward*(1.f/length);
 target=link->current.pos+cXyz(0,65,0)+forward*1000;return true;
}
cXyz target_direction(daAlink_c*){return {0,0,1};}
unsigned waves=0;std::vector<cXyz> waveDirections;
void launch(daAlink_c*,unsigned weapon,cXyz direction,bool,float){assert(weapon==0);++waves;waveDirections.push_back(direction);}
struct Emitter {
    int mMaxFrame=5,age=0;bool stopped=false,quit=false,invalid=false;cXyz position,scale{1,1,1};
    void setGlobalScale(const cXyz& value){scale=value;}
    int getAge(){return age;}void stopCreateParticle(){stopped=true;}
    void quitImmortalEmitter(){quit=true;}void becomeInvalidEmitter(){invalid=true;}
    void setGlobalTranslation(cXyz p){position=p;}
};
std::map<u32,Emitter>emitters;unsigned nextEmitter=1;
constexpr u16 ID_ZI_J_KAITENGIRI_A=11,ID_ZI_J_KAITENGIRI_B=12,ID_ZI_J_BOMBex_A=0x161,ID_ZI_J_BOMBex_B=0x162;
constexpr u16 ID_ZI_J_KAITENGIRID_A=21,ID_ZI_J_KAITENGIRID_B=22,ID_ZI_J_KAITENGIRID_C=23,ID_ZI_J_KAITENGIRID_D=24,ID_ZI_J_KAITENGIRID_E=25,ID_ZI_J_KAITENGIRID_F=26;
#include "survivors_spin_effects.inc"

std::vector<u16>particles;std::vector<float>particleScales;
Emitter* dComIfGp_particle_getEmitter(u32 id){auto it=emitters.find(id);return it==emitters.end()?nullptr:&it->second;}
u32 dComIfGp_particle_set(u32 id,u16 effect,cXyz*,int*,const csXyz*,const cXyz* scale,int,void*,int,void*,void*,void*){
    assert(effect==ID_ZI_J_KAITENGIRI_A || effect==ID_ZI_J_KAITENGIRI_B || (effect>=21&&effect<=26) || effect==0x161 || effect==0x162);
    particles.push_back(effect);particleScales.push_back(scale?scale->x:1);
    if(!id){id=nextEmitter++;emitters.emplace(id,Emitter{});}return id;
}
void dComIfGp_particle_levelEmitterOnEventMove(u32){}
struct Attack {unsigned weapon=0;bool active=true,greatSpin=false;float radius=65,effectBaseRadius=260;unsigned life=10;cXyz position,velocity;cCcD_Obj collider;std::array<u32,9>emitters{};};
std::array<Attack,initial_projectile_slots>s_attacks;
struct Effect{cCcD_Obj collider;unsigned damage=0,fixedDamage=0;bool critical=false;};std::array<Effect,24>s_effects;
void new_melee_swing(){}
bool rangedActive=false;bool ranged_pose_active(){return rangedActive;}
void cancel_ranged(){}
constexpr int HIT_TYPE_LINK_HEAVY_ATTACK=9;
struct dCcU_AtInfo {cCcD_Obj* mpCollider;unsigned mAttackPower=200;int mHitType=0;};
HookAction before_drop(ModContext*,void*,void*,void*);
std::vector<int>drops;
fpc_ProcID fopAcM_createItem(const cXyz*,int item,int bit,int room,void*,void*,int mode){
    fpc_ProcID result=0;
    assert(before_drop(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);
    assert(bit==-1 && room==rewardRoom && mode==0);drops.push_back(item);return 1;
}
unsigned maxLife=15,rupees=0,life=4;
unsigned dComIfGs_getMaxLife(){return maxLife;}
void dComIfGs_setMaxLife(unsigned v){maxLife=v;}
unsigned dComIfGs_getMaxLifeGauge(){return (maxLife/5)*4;}
void dComIfGs_setLife(unsigned v){life=v;}
unsigned dComIfGs_getRupeeMax(){return 300;}unsigned dComIfGs_getRupee(){return rupees;}
void dComIfGs_setRupee(unsigned v){rupees=v;}
void close_window(){}
fpc_ProcID create_run_pickup(cXyz pos,int item){
 const bool old=s_allowDrop;s_allowDrop=true;const auto id=fopAcM_createItem(&pos,item,-1,rewardRoom,nullptr,nullptr,0);s_allowDrop=old;return id;}

'''
stubs += r'''
struct BulblinHit{bool critical=false;unsigned basePower=0,scaledPower=0;};BulblinHit* bulblin_power_hit(dCcU_AtInfo*){return nullptr;}
bool reuse_bulblin_power(dCcU_AtInfo*){return false;}
void mark_damage_critical(dCcU_AtInfo*,bool){}
void mark_damage_split(dCcU_AtInfo*,unsigned,unsigned){}
struct BulblinPowerCache{dCcU_AtInfo* info;~BulblinPowerCache(){}};
'''
stubs += "\nvoid update_lightning(daAlink_c*){}\nvoid apply_treasure_inventory(daAlink_c*,ChestReward,const Run&,unsigned){}\nvoid sync_survivors_inventory(){}\nDamageAmounts hit_damage_sources(const cCcD_Obj* c){DamageAmounts d{};for(const auto& a:s_attacks)if(c==&a.collider){d[unsigned(weapon_damage_source(a.weapon))]=1;return d;}d[unsigned(c==&player.mGuardAtCps?DamageSource::Shield:DamageSource::Sword)]=1;return d;}\n"
main = r'''
void reset(){bloat={};bloat.id=91;stallord={};stallord.id=92;s_multiStrike={};player.swordHeld=player.swordPressed=false;s_waveManual=s_manualSwing=false;player.id=7;playing=arena=true;abilityBusy=false;s_pots={};potActor.id=99;potCarried=false;
    player.mProcID=daAlink_c::PROC_WAIT;player.mEquipItem=0x103;
    player.field_0x3478=0;player.field_0x347c=10;player.field_0x3484=15;
    for(auto& id:player.field_0x3204)id=0;
    player.mLinkAcch.ground=true;player.wolf=false;player.mSwordModel=&player.model;
    player.normals=player.spins=player.equips=player.finishes=player.mComboCutCount=0;
    player.normalTypes.clear();player.finishTypes.clear();player.comboCounts.clear();
    player.direction=4;player.locked=player.hit=false;
    s_comboStep=s_comboTimeout=0;s_autoSwordCall=s_trackMelee=false;s_meleeTarget=kNone;blocked=false;
    enemy.health=10;enemy2.id=11;player.current.pos={};
    s_run.start(1234);s_swordAction=SwordAction::None;s_wavePending=false;
    s_meleeCooldown=0;s_cooldowns={};waves=0;
    enemy.id=10;enemy.current.pos={0,0,150};s_enemies={};s_enemies[0]={10,true,false,false};
}
int main(){
    // Both body tracks and the next-swing cooldown accelerate once per native
    // combo entry. Consecutive swings never compound the speed multiplier.
    for(unsigned dual:{0u,1u,10u,30u})for(unsigned haste:{0u,1u,5u}) {
        reset();s_run.dual_wield=dual;s_run.talents.ranks[unsigned(Talent::MeleeHaste)]=haste;
        for(unsigned swing=0;swing<4;++swing) {
            s_meleeCooldown=0;player.mUnderFrameCtrl[0].frame=16;update_sword(&player);
            assert(std::abs(player.mUnderFrameCtrl[0].rate-(1+float(5*dual/2+5*haste)/100.f))<.0001f);
            assert(player.mUpperFrameCtrl[0].rate==player.mUnderFrameCtrl[0].rate);
            assert(s_meleeCooldown==s_run.melee_cooldown());
        }
    }
    // Native normal and Light Sword emitters match the expanding hitbox at all
    // Reach ranks. Reuse, new emitters and shrinking Reach never compound scale.
    reset();s_swordAction=SwordAction::Spin;player.mProcID=daAlink_c::PROC_CUT_TURN;
    std::any spinArgs[]={&player};
    for(unsigned count:{2u,4u})for(float nativeRadius:{220.f,300.f}) {
        player.field_0x3478=nativeRadius;
        for(unsigned i=0;i<count;++i){player.field_0x3204[i]=nextEmitter++;emitters[player.field_0x3204[i]]={};}
        for(unsigned reach:{0u,1u,10u,30u,0u})for(float current:{130.f,175.f,nativeRadius}) {
            s_run.ranks[unsigned(Upgrade::Reach)]=reach;player.field_0x347c=current;
            for(unsigned frame=0;frame<3;++frame) {
                after_sword_move(nullptr,spinArgs,nullptr,nullptr);
                after_spin_effect(nullptr,spinArgs,nullptr,nullptr);
                const float radius=current*(1+2*(std::sqrt(1+.075f*reach)-1));
                assert(std::abs(player.mAtSph.radius-radius)<.001f);
                for(unsigned i=0;i<count;++i){
                    const auto& emitter=emitters[player.field_0x3204[i]];
                    assert(std::abs(nativeRadius*emitter.scale.x-radius)<.001f);
                    assert(emitter.scale.x==emitter.scale.y&&emitter.scale.y==emitter.scale.z);
                    assert(!emitter.stopped&&!emitter.quit&&!emitter.invalid);
                }
            }
        }
        for(auto& id:player.field_0x3204)id=0;
    }
    // Empty/stale emitter slots are safe; paused/outside/foreign actions stay native.
    player.field_0x3204[0]=nextEmitter++; // deliberately not in the emitter registry
    after_spin_effect(nullptr,spinArgs,nullptr,nullptr);
    player.field_0x3204[0]=nextEmitter++;emitters[player.field_0x3204[0]]={};
    for(unsigned gate=0;gate<5;++gate) {
        playing=gate!=0;arena=gate!=1;s_run.phase=gate==2?Phase::Upgrade:Phase::Running;
        player.id=gate==3?99:7;s_swordAction=gate==4?SwordAction::Normal:SwordAction::Spin;
        player.mAtSph.radius=77;emitters[player.field_0x3204[0]].scale={7,7,7};
        after_spin_effect(nullptr,spinArgs,nullptr,nullptr);
        assert(player.mAtSph.radius==77&&emitters[player.field_0x3204[0]].scale.x==7);
    }
    player.field_0x3204[0]=0;
    // Reach extends native swept capsules each frame from fresh blade points,
    // clips extension at walls, and does not widen capsules or invent hit flags.
    reset();update_sword(&player);std::any swordArgs[]={&player};
    for(unsigned reach:{0u,1u,10u,30u}) {
        s_run.ranks[unsigned(Upgrade::Reach)]=reach;
        player.mAtCps[0].radius=35;player.mAtCps[1].active=false;
        for(int frame=0;frame<3;++frame) {
            after_sword_collision(nullptr,swordArgs,nullptr,nullptr);
            assert(std::abs(player.mAtCps[0].end.z-1.3f*s_run.melee_reach(200))<.001f);
            assert(std::abs(player.mAtCps[2].end.z-1.3f*s_run.melee_reach(190))<.001f);
            assert(player.mAtCps[0].radius==35&&!player.mAtCps[1].active);
        }
    }
    blocked=true;after_sword_collision(nullptr,swordArgs,nullptr,nullptr);assert(player.mAtCps[0].end.z==250);
    blocked=false;
    for(int gate=0;gate<5;++gate) {
        s_run.phase=gate==0?Phase::Upgrade:Phase::Running;playing=gate!=1;
        player.mProcID=gate==2?daAlink_c::PROC_DAMAGE:daAlink_c::PROC_CUT_NORMAL;
        s_swordAction=gate==3?SwordAction::Spin:SwordAction::Normal;
        player.id=gate==4?99:7;player.mAtCps[0].end.z=123;
        after_sword_collision(nullptr,swordArgs,nullptr,nullptr);assert(player.mAtCps[0].end.z==123);
    }
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
    player.mUnderFrameCtrl[0].frame=5;update_sword(&player);assert(waves==0);
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
    // A due Spin replaces an ordinary cut and keeps ownership until native
    // recovery. Normal cuts and wave casts must yield to active item poses.
    reset();update_sword(&player);s_run.ranks[1]=1;
    s_wavePending=true;assert(!begin_spin_attack(&player));s_wavePending=false;
    assert(begin_spin_attack(&player)&&player.spins==1&&s_swordAction==SwordAction::Spin);
    const auto normalCount=player.normals;update_sword(&player);assert(player.normals==normalCount);
    assert(!begin_spin_attack(&player)); // No animation restart while spinning.
    reset();update_sword(&player);rangedActive=true;player.shape_angle.y=123;
    track_melee_facing(&player);assert(player.shape_angle.y==123);
    reset();rangedActive=true;update_sword(&player);assert(!player.normals);
    s_run.ranks[0]=1;update_sword_wave(&player);assert(!player.normals);
    assert(!begin_spin_attack(&player));rangedActive=false;
    for(int proc:{daAlink_c::PROC_DAMAGE,daAlink_c::PROC_DODGE,daAlink_c::PROC_JUMP}) {
        reset();player.mProcID=proc;assert(!begin_spin_attack(&player));
    }
    // Ordinary sword combo targets nearby unbroken pots when no enemy is reachable.
    reset();s_enemies={};potActor.current.pos={0,0,150};s_pots[0]={99,true,false,false};
    update_sword(&player);assert(player.normals==1&&s_meleeTarget==99);
    player.mProcID=daAlink_c::PROC_WAIT;s_meleeCooldown=0;s_pots[0].broken=true;
    update_sword(&player);assert(player.normals==1);
    s_pots[0].broken=false;potCarried=true;update_sword(&player);assert(player.normals==1);
    potCarried=false;potActor.current.pos.y=200;update_sword(&player);assert(player.normals==1);
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
    // A stale pending item marker without its upper animation must recover.
    reset();player.mEquipItem=dItemNo_NONE_e;player.field_0x2fde=0x103;
    update_sword(&player);assert(player.equips==1 && player.equipping);
    update_sword(&player);assert(player.equips==1);
    player.equipping=false;player.mEquipItem=0x103;
    update_sword(&player);assert(player.normals==1);
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
    // Passive Wave/Spin ranks must not replace or reset the native combo.
    reset();s_run.ranks[0]=s_run.ranks[1]=30;s_run.ranks[5]=2;
    update_sword(&player);assert(player.spins==0 && player.normals==1 && waves==0);
    player.mUnderFrameCtrl[0].frame=16;s_meleeCooldown=0;update_sword(&player);
    assert(player.mComboCutCount==2 && waves==0 && !s_wavePending);
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
    // Apply the debug multiplier once at the final owned damage boundary,
    // including combined fixed burns. Release builds must ignore stale debug
    // values. Foreign/native attacks remain untouched.
    reset();s_swordAction=SwordAction::Normal;
    for(unsigned multiplier=1;multiplier<=10;++multiplier){
        s_run.options.debug.player_damage=multiplier;
        const auto effectiveMultiplier=debug_enabled?multiplier:1u;
        dCcU_AtInfo hit{&player.mAtCps[0]};std::any debugArgs[]={&hit};
        after_power(nullptr,debugArgs,nullptr,nullptr);assert(hit.mAttackPower==s_run.melee_power()*effectiveMultiplier);
        s_effects[0].damage=7;s_effects[0].fixedDamage=13;hit={&s_effects[0].collider};
        after_power(nullptr,debugArgs,nullptr,nullptr);assert(hit.mAttackPower==20*effectiveMultiplier);
    }
    s_run.options.debug.player_damage=1;s_effects[0].fixedDamage=0;
    // Only this Link's active native sword colliders receive linear Survivors damage.
    s_swordAction=SwordAction::Spin;s_run.ranks[0]=s_run.ranks[1]=1;s_run.ranks[3]=2;dCcU_AtInfo info{&player.mAtSph};std::any powerArgs[]={&info};
    after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==5);
    s_swordAction=SwordAction::Normal;
    for(auto& capsule:player.mAtCps){info={&capsule};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==5);}
    s_run.master_sword=true;
    info={&player.mAtCps[0]};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==8);
    assert(info.mHitType==HIT_TYPE_LINK_HEAVY_ATTACK); // Native Master Sword multiplier must not apply again.
    assert(s_run.attack_power(0)==7+s_run.ranks[0]);
    s_swordAction=SwordAction::Spin;info={&player.mAtSph};after_power(nullptr,powerArgs,nullptr,nullptr);
    assert(info.mAttackPower==s_run.attack_power(1));s_run.master_sword=false;
    cCcD_Obj unrelated;info={&unrelated};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==200 && info.mHitType==0);
    arena=false;info={&player.mAtCps[0]};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==200);arena=true;
    // Native sword/spin/shield and every projectile use one critical roll per hit.
    s_run.ranks[unsigned(Upgrade::CritChance)]=30;s_run.ranks[unsigned(Upgrade::CritDamage)]=30;
    s_run.dark_link=29;s_run.dark_remaining=150;
    auto checkCritical=[&](cCcD_Obj& collider,unsigned base){
        s_run.random=1;info={&collider};after_power(nullptr,powerArgs,nullptr,nullptr);
        assert(info.mAttackPower==s_run.dark_power((base*350+50)/100));
        assert(s_run.random==270369); // one roll, no second Master Sword/Dark multiplier
    };
    s_swordAction=SwordAction::Normal;checkCritical(player.mAtCps[0],s_run.melee_power());
    info={&player.mAtCps[0]};after_power(nullptr,powerArgs,nullptr,nullptr);
    assert(info.mAttackPower==s_run.melee_power()*2&&s_run.random==67634689); // next target can miss its crit
    s_swordAction=SwordAction::Spin;checkCritical(player.mAtSph,s_run.attack_power(1));
    checkCritical(player.mGuardAtCps,s_run.attack_power(unsigned(Upgrade::ShieldAttack)));
    for(unsigned base:{4u,5u,6u,7u,8u}){s_attacks[0].collider.power=base;checkCritical(s_attacks[0].collider,base);}
    // Fractional Dark Link multipliers reach every native and owned damage path.
    for(unsigned rank:{1u,2u,3u,28u,29u,30u}){
        s_run.dark_link=rank;s_run.dark_remaining=150;
        s_swordAction=SwordAction::Normal;checkCritical(player.mAtCps[0],s_run.melee_power());
        s_swordAction=SwordAction::Spin;checkCritical(player.mAtSph,s_run.attack_power(1));
        checkCritical(player.mGuardAtCps,s_run.attack_power(unsigned(Upgrade::ShieldAttack)));
        checkCritical(s_attacks[0].collider,s_attacks[0].collider.GetAtAtp());
    }
    info={&unrelated};const auto critRng=s_run.random;after_power(nullptr,powerArgs,nullptr,nullptr);
    assert(info.mAttackPower==200&&s_run.random==critRng);
    s_run.ranks[unsigned(Upgrade::CritChance)]=s_run.ranks[unsigned(Upgrade::CritDamage)]=0;
    s_run.dark_link=s_run.dark_remaining=0;
    // Equipment-only Bow crit also reaches the native power fallback. A
    // different projectile does not inherit the bow's critical chance.
    const auto preEquipment=s_run;const auto priorWeapon=s_attacks[0].weapon;
    s_run.gear_weapon={GearKind::HerosBow,5,5,5};s_run.ranks[unsigned(Upgrade::Bow)]=1;
    s_attacks[0].weapon=unsigned(Upgrade::Bow);s_attacks[0].collider.power=10;
    s_run.random=1;info={&s_attacks[0].collider};after_power(nullptr,powerArgs,nullptr,nullptr);
    assert(info.mAttackPower==20&&s_run.random==270369);
    s_attacks[0].weapon=unsigned(Upgrade::Bombs);s_run.random=1;info={&s_attacks[0].collider};after_power(nullptr,powerArgs,nullptr,nullptr);
    assert(info.mAttackPower==10&&s_run.random==1);
    s_run=preEquipment;s_attacks[0].weapon=priorWeapon;

    // Ranged waves start a visible normal cut even without a melee target.
    reset();s_run.ranks[0]=15;enemy.current.pos.z=900;waves=0;
    update_sword_wave(&player);assert(player.normals==1&&s_wavePending&&waves==0);
    player.mUnderFrameCtrl[0].frame=player.field_0x3478-1;update_sword_wave(&player);assert(waves==0);
    player.mUnderFrameCtrl[0].frame=player.field_0x3478;update_sword_wave(&player);
    assert(waves==1&&!s_wavePending&&s_cooldowns[0]==s_run.cooldown(60));
    update_sword_wave(&player);assert(waves==1&&player.normals==1);
    // Interruptions cancel wind-up; no shot appears during damage or a jump.
    for(int proc:{daAlink_c::PROC_DAMAGE,daAlink_c::PROC_JUMP}){
      reset();s_run.ranks[0]=1;waves=0;update_sword_wave(&player);assert(s_wavePending);
      player.mProcID=proc;update_sword_wave(&player);assert(!s_wavePending&&waves==0);
    }
    // Existing melee cuts are reused, finishers are never replaced for a wave.
    reset();s_run.ranks[0]=1;waves=0;update_sword(&player);auto cuts=player.normals;
    update_sword_wave(&player);assert(s_wavePending&&player.normals==cuts);
    player.mUnderFrameCtrl[0].frame=player.field_0x3478;update_sword_wave(&player);assert(waves==1);
    s_cooldowns[0]=0;player.mProcID=daAlink_c::PROC_CUT_FINISH;
    update_sword_wave(&player);assert(!s_wavePending&&player.normals==cuts);
    // Sheathed casting draws the sword, then waits for equip before starting.
    reset();s_run.ranks[0]=1;player.mEquipItem=dItemNo_NONE_e;player.field_0x2fde=0;player.equipping=false;waves=0;
    update_sword_wave(&player);assert(player.equips==1&&!s_wavePending&&waves==0);
    player.equipping=false;player.mEquipItem=0x103;update_sword_wave(&player);assert(s_wavePending);
    // Expired and missing emitters restart; all surviving emitters are invalidated on cleanup.
    for(unsigned level:{1u,3u,5u,14u,15u,20u,30u}) {
        for(unsigned multi=0;multi<=3;++multi){
            s_run.ranks[unsigned(Upgrade::Multishot)]=multi;
            s_run.ranks[0]=level;waves=0;waveDirections.clear();release_sword_wave(&player);assert(waves==1+multi);
            for(unsigned i=0;i<waves;++i)for(unsigned j=0;j<i;++j)assert(waveDirections[i].abs(waveDirections[j])>.01f);
        }
        s_run.ranks[unsigned(Upgrade::Multishot)]=0;
    }
    for(bool great:{false,true})for(unsigned reach:{0u,30u}) {
        Attack wave;wave.greatSpin=great;wave.effectBaseRadius=great?400:260;
        s_run.ranks[0]=great?15:14;s_run.ranks[5]=reach;wave.radius=s_run.wave_radius();
        particles.clear();particleScales.clear();update_wave_effects(wave,&player);
        assert(particles.size()==(great?6u:2u));
        for(unsigned i=0;i<particles.size();++i) {
            assert(particles[i]==survivor_spin_effects(great)[i]);
            assert(std::abs(particleScales[i]*wave.effectBaseRadius-wave.radius)<.001f);
        }
        // In-flight particles retain their tier even when another upgrade arrives.
        s_run.ranks[0]=great?14:15;particles.clear();update_wave_effects(wave,&player);
        assert(particles.size()==(great?6u:2u));stop_attack(wave);
    }
    Attack a;update_wave_effects(a,&player);auto first=a.emitters;
    a.position={10,20,30};update_wave_effects(a,&player);
    assert(a.emitters==first && emitters[first[0]].position.x==10);
    emitters[first[0]].age=5;emitters.erase(first[1]);update_wave_effects(a,&player);
    assert(a.emitters[0]!=first[0] && a.emitters[1]!=first[1] && emitters[first[0]].invalid);
    auto last=a.emitters;stop_attack(a);
    assert(!a.active && !a.collider.active && !a.collider.hit && a.emitters[0]==0 && a.emitters[1]==0);
    for(auto id:last)if(id)assert(emitters[id].stopped && emitters[id].quit && emitters[id].invalid);
    stop_attack(a); // Repeated cleanup is safe.
    // Circle a target through a whole turn at full running speed. Facing is
    // recomputed from the moved position every frame; travel remains untouched.
    reset();update_sword(&player);assert(s_trackMelee&&s_meleeTarget==enemy.id);
    for(unsigned frame=0;frame<120;++frame){
        const float a=frame*6.2831853f/120;
        player.current.pos={150*std::sin(a),0,150+150*std::cos(a)};
        player.current.angle.y=player.mMoveAngle=short(1234+frame);player.mNormalSpeed=20;
        track_melee_facing(&player);
        const auto delta=enemy.current.pos-player.current.pos;
        assert(player.shape_angle.y==cM_atan2s(delta.x,delta.z));
        assert(player.mProcVar2.field_0x300c==player.shape_angle.y);
        assert(player.current.angle.y==short(1234+frame)&&player.mMoveAngle==short(1234+frame)&&player.mNormalSpeed==20);
    }
    // A slightly nearer bystander does not steal a live lock mid-swing.
    player.current.pos={};enemy.current.pos={0,0,150};enemy2.current.pos={60,0,40};
    s_enemies[1]={11,true,false,false};track_melee_facing(&player);
    assert(s_meleeTarget==10&&player.shape_angle.y==0);
    enemy.health=0;track_melee_facing(&player);assert(s_meleeTarget==11&&player.shape_angle.y==cM_atan2s(60,40));
    player.mProcID=daAlink_c::PROC_CUT_FINISH;player.mProcVar2.field_0x300c=9;
    enemy2.current.pos={-80,0,40};track_melee_facing(&player);
    assert(player.shape_angle.y==cM_atan2s(-80,40)&&player.mProcVar2.field_0x300c==9);
    blocked=true;track_melee_facing(&player);assert(s_meleeTarget==kNone);blocked=false;
    enemy2.current.pos={0,250,30};assert(!melee_target(&player));
    enemy2.current.pos={0,0,1000};assert(!melee_target(&player));
    enemy2.current.pos={80,0,40};
    for(int reason=0;reason<6;++reason){
        player.mProcID=reason==0?daAlink_c::PROC_DAMAGE:daAlink_c::PROC_CUT_NORMAL;
        s_trackMelee=reason!=1;s_run.phase=reason==2?Phase::Upgrade:Phase::Running;
        arena=reason!=3;player.mLinkAcch.ground=reason!=4;player.id=reason==5?8:7;
        player.shape_angle.y=123;track_melee_facing(&player);assert(player.shape_angle.y==123);
    }
    reset();
    // Automatic Jump Strike owns only its native jump/landing poses and damage.
    reset();s_run.master_sword=1;s_run.ranks[unsigned(Upgrade::JumpStrike)]=1;
    for(unsigned i=0;i<100&&s_swordAction!=SwordAction::Jump;++i){
        player.mProcID=daAlink_c::PROC_WAIT;s_meleeCooldown=0;update_sword(&player);
    }
    assert(s_swordAction==SwordAction::Jump&&player.mProcID==daAlink_c::PROC_CUT_LARGE_JUMP);
    assert(s_cooldowns[unsigned(Upgrade::JumpStrike)]==180&&!s_wavePending&&!s_trackMelee);
    assert(player.mUnderFrameCtrl[0].rate==2&&player.mUpperFrameCtrl[0].rate==2);
    info={&player.mAtCps[0]};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==6);
    player.mProcID=daAlink_c::PROC_CUT_LARGE_JUMP_LAND;player.field_0x3478=300;
    particles.clear();particleScales.clear();s_run.ranks[unsigned(Upgrade::Reach)]=5;
    assert(before_jump_strike_effect(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
    assert((particles==std::vector<u16>{0x161,0x162}));
    assert(std::abs(particleScales[0]-2.625f)<.01f&&particleScales[0]==particleScales[1]);
    for(unsigned rank:{1u,30u}){
        s_run.ranks[unsigned(Upgrade::JumpStrike)]=rank;
        info={&player.mAtSph};after_power(nullptr,powerArgs,nullptr,nullptr);
        assert(info.mAttackPower==s_run.jump_splash_power());
    }
    s_run.ranks[unsigned(Upgrade::CritChance)]=30;s_run.ranks[unsigned(Upgrade::CritDamage)]=30;
    s_run.dark_link=29;s_run.dark_remaining=150;checkCritical(player.mAtSph,s_run.jump_splash_power());
    for(float radius:{130.f,200.f,300.f}){
        player.field_0x347c=radius;player.mUnderFrameCtrl[0].rate=1;
        after_jump_strike(nullptr,args,nullptr,nullptr);
        assert(player.mAtSph.radius==s_run.melee_reach(radius)&&player.mUnderFrameCtrl[0].rate==2);
    }
    const auto jumpParticles=s_jumpEmitters;clear_jump_effects();clear_jump_effects();
    for(auto id:jumpParticles)assert(emitters[id].invalid);
    abilityBusy=true;assert(!update_jump_strike(&player));
    assert(before_jump_strike_effect(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);abilityBusy=false;
    player.mProcID=daAlink_c::PROC_DAMAGE;assert(!update_jump_strike(&player)&&s_swordAction==SwordAction::None);
    assert(before_jump_strike_effect(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    // Ordinary attacks still work when the jump's cooldown has not elapsed.
    player.mProcID=daAlink_c::PROC_WAIT;s_meleeCooldown=0;update_sword(&player);
    assert(s_swordAction==SwordAction::Normal);
    reset();
    // 130% base acquisition, with 130% retention and no old 200% range.
    reset();enemy.current.pos={0,0,363};assert(melee_target(&player)==&enemy);
    enemy.current.pos.z=428;assert(melee_target(&player)==&enemy);
    enemy.current.pos.z=430;assert(!melee_target(&player));
    s_meleeTarget=kNone;enemy.current.pos.z=365;assert(!melee_target(&player));
    // Joint-relative body surfaces, including the elevated head, determine reach.
    // Real native radii: Bloat 180/240; Stallord 300/350/270/190/190.
    reset();s_enemies={};s_enemies[0]={91,true,false,false,fpcNm_E_YMB_e};
    bloat.current.pos={0,450,900};bloat.eyePos=bloat.current.pos;
    bloat.field_0x82c[1]={{500,70,0},240,true};
    cXyz contact;assert(melee_target(&player,&contact)==&bloat);
    assert(std::abs(contact.x-260)<.01f&&contact.z==0);
    update_sword(&player);assert(player.normals==1&&player.shape_angle.y==0x4000);
    bloat.field_0x82c[1].center={0,70,500};track_melee_facing(&player);
    assert(player.shape_angle.y==0); // Follow the animated body, not actor origin.
    blocked=true;assert(!melee_target(&player));blocked=false;
    bloat.field_0x82c[1].target=false;assert(!melee_target(&player));
    bloat.field_0x1324={{0,70,100},360,true};assert(!melee_target(&player)); // Electric field is not a body.
    bloat.field_0x82c[1]={{0,70,605},240,true};s_meleeTarget=kNone;
    assert(!melee_target(&player));
    s_run.ranks[unsigned(Upgrade::Reach)]=1;assert(melee_target(&player)==&bloat);
    reset();s_enemies={};s_enemies[0]={92,true,false,false,fpcNm_B_DS_e};
    stallord.current.pos={0,320,900};stallord.eyePos=stallord.current.pos;
    stallord.mHeadSph[1]={{0,320,600},350,true};
    assert(melee_target(&player,&contact)==&stallord); // Surface gap 300, origin > 900.
    assert(std::abs(contact.z-276.923f)<.01f);
    update_sword(&player);assert(player.normals==1);
    stallord.mHeadSph[1].center={0,1200,100};assert(!melee_target(&player)); // Too high.
    stallord.mHeadSph[1].radius=0;stallord.mBreathAtSph={{0,70,100},500,true};
    assert(!melee_target(&player)); // Breath cannot extend sword acquisition.
    for(unsigned i=0;i<5;++i) {
        stallord.mHeadSph[i]={{0,70,450},190,true};assert(melee_target(&player)==&stallord);
        stallord.mHeadSph[i].radius=0;s_meleeTarget=kNone;
    }
    stallord.mHeadSph[0]={{0,70,0},300,true};assert(melee_target(&player)==&stallord);
    s_enemies[0].retiring=true;assert(!melee_target(&player));
    // Disabling individual skills keeps held attack from bypassing their target gates.
    reset();s_enemies={};player.swordHeld=true;s_run.options.manual_aim[unsigned(ManualAbility::Sword)]=false;
    update_sword(&player);assert(!player.normals);
    s_run.ranks[0]=1;s_run.options.manual_aim[unsigned(ManualAbility::Wave)]=false;
    update_sword_wave(&player);assert(!s_wavePending);
    for(bool enabled:{false,true}) {
        reset();player.swordHeld=true;s_run.master_sword=1;s_run.ranks[unsigned(Upgrade::JumpStrike)]=1;
        s_run.options.manual_aim[unsigned(ManualAbility::JumpStrike)]=enabled;camera.mCamera.center={100,0,0};camera.mCamera.eye={};enemy.current.pos={0,0,150};
        bool jumped=false;for(int attempt=0;attempt<100&&!jumped;++attempt)jumped=try_jump_strike(&player);
        assert(jumped&&player.shape_angle.y==(enabled?0x4000:0));
    }
    // Both a tap and a hold start owned melee with no target and retain cooldowns.
    for(bool hold:{false,true}){
        reset();s_enemies={};player.swordPressed=!hold;player.swordHeld=hold;
        camera.mCamera.center={100,0,0};camera.mCamera.eye={};
        assert(manual_attack(&player));update_sword(&player);
        assert(player.normals==1&&s_manualSwing&&player.shape_angle.y==0x4000);
        update_sword(&player);assert(player.normals==1&&s_meleeCooldown);
        player.swordPressed=player.swordHeld=false;
        track_melee_facing(&player);assert(player.shape_angle.y==0x4000);
    }
    reset();s_enemies={};player.swordPressed=true;s_run.ranks[0]=1;
    update_sword_wave(&player);assert(s_wavePending&&s_waveManual);
    player.swordPressed=false;camera.mCamera.center={0,30,40};
    player.mUnderFrameCtrl[0].frame=player.field_0x3478;waveDirections.clear();
    update_sword_wave(&player);assert(waveDirections.size()==1&&std::abs(waveDirections[0].y-.6f)<.001f&&std::abs(waveDirections[0].z-.8f)<.001f);
    // Even multishot volleys retain one wave exactly on the manual reticle.
    for(unsigned multi:{1u,2u,3u}){
        s_run.ranks[unsigned(Upgrade::Multishot)]=multi;s_waveManual=true;waveDirections.clear();
        release_sword_wave(&player);assert(waveDirections.size()==multi+1);
        assert(std::abs(waveDirections[0].y-.6f)<.001f&&std::abs(waveDirections[0].z-.8f)<.001f&&waveDirections[0].x==0);
    }
    cameraPresent=false;player.shape_angle.y=0;assert(manual_direction(&player).z==1);cameraPresent=true;
    camera.mCamera.center={};assert(manual_direction(&player).z==1);camera.mCamera.center={100,0,0};
    player.swordHeld=true;playing=false;assert(!manual_attack(&player));playing=true;
    s_run.phase=Phase::Upgrade;assert(!manual_attack(&player));s_run.phase=Phase::Running;
    player.id=999;assert(!manual_attack(&player));player.id=7;
    player.mProcVar5.mCutTurnChargeCheckTimer=3;
    assert(before_manual_charge(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&player.mProcVar5.mCutTurnChargeCheckTimer==0);
    arena=false;assert(before_manual_charge(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);arena=true;
    // Owned Jump Strike never calls native target-derived launch adjustments.
    reset();s_swordAction=SwordAction::Jump;player.mProcID=daAlink_c::PROC_CUT_LARGE_JUMP;
    assert(before_jump_target(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
    s_swordAction=SwordAction::None;assert(before_jump_target(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    reset();
    // Native random drops stay suppressed; only scoped reward creation bypasses the hook.
    fpc_ProcID result=0;assert(before_drop(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL && result==kNone);
    bool seen[8]{};
    for(int room : {51,6,0,50}) {
        rewardRoom=room;
        for(int i=0;i<25000;++i){drop_reward(&enemy);assert(!s_allowDrop);}
    }
    for(auto item:drops){assert(item>=0 && item<=7);seen[item]=true;}
    for(bool found:seen)assert(found);
    arena=false;assert(before_drop(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);arena=true;
    // Level-up choices fill the native gauge, including new containers; keep heart pieces.
    reset();unsigned choice=0;s_run.phase=Phase::Upgrade;s_run.offers[0]=Upgrade::Heart;
    maxLife=17; // Preserve two collected heart pieces when adding a container.
    select_upgrade(nullptr,&choice);assert(maxLife==22 && life==16);
    s_run.phase=Phase::Upgrade;s_run.offers[0]=Upgrade::Rupees;rupees=290;life=1;
    select_upgrade(nullptr,&choice);assert(rupees==300 && life==16);
    // Invalid choices and calls outside a level-up never grant healing.
    life=3;select_upgrade(nullptr,&choice);assert(life==3);
    s_run.phase=Phase::Upgrade;choice=3;select_upgrade(nullptr,&choice);assert(life==3);
    choice=0;s_run.offers[0]=Upgrade::Count;select_upgrade(nullptr,&choice);assert(life==3);
    // An XP surplus opens another choice immediately, still at full health.
    s_run.offers[0]=Upgrade::Power;s_run.xp=1000;
    const auto oldLevel=s_run.level;
    select_upgrade(nullptr,&choice);
    assert(life==16&&s_run.level==oldLevel+1&&s_run.phase==Phase::Upgrade);
    // Greed also blocks the choice-time refill, including newly gained containers.
    reset();s_run.phase=Phase::Upgrade;s_run.apply_fate(FateEffect::Greed);
    s_run.offers[0]=Upgrade::Heart;maxLife=17;life=3;choice=0;
    select_upgrade(nullptr,&choice);assert(maxLife==22&&life==3);
    s_run.phase=Phase::Upgrade;s_run.offers[0]=Upgrade::Rupees;rupees=0;
    const auto greedBonus=s_run.upgrade_rupees(Upgrade::Rupees,s_run.offer_rarities[0]);
    select_upgrade(nullptr,&choice);assert(life==3&&rupees==greedBonus*3/2);
    // Every Multi Strike activation uses a new native cut, after the previous
    // pose/hit window; repeats do not recursively re-arm the burst.
    for(unsigned extra=1;extra<=3;++extra){
        reset();s_run.dual_wield=1;s_run.ranks[unsigned(Upgrade::MultiStrike)]=extra;
        update_sword(&player);assert(s_multiStrike.count(StrikeSeries::Sword)==extra);
        for(unsigned n=extra;n>0;--n){
            player.mProcID=daAlink_c::PROC_WAIT;update_sword(&player);
            assert(s_multiStrike.count(StrikeSeries::Sword)==n-1);
        }
        assert(player.normals+player.finishes==1+extra);
        player.mProcID=daAlink_c::PROC_WAIT;update_sword(&player);
        assert(player.normals+player.finishes==1+extra);
        // Repeat the full Sword Wave volley, including Multishot projectiles.
        reset();s_run.dual_wield=1;s_run.ranks[unsigned(Upgrade::MultiStrike)]=extra;
        s_run.ranks[unsigned(Upgrade::Multishot)]=2;s_run.ranks[0]=1;
        update_sword_wave(&player);player.mUnderFrameCtrl[0].frame=player.field_0x3478;
        update_sword_wave(&player);assert(waves==3);
        for(unsigned frame=0;frame<extra*4;++frame)update_sword_wave(&player);
        assert(waves==3*(1+extra)&&!s_multiStrike.count(StrikeSeries::Wave));
    }
    std::cout<<"Survivors combat: native four-hit combos, sword movement, wave timing, particles and rewards passed\n";
}
'''
functions = ['stop_attack', 'update_wave_effects', 'sword_action_available', 'enemy_in_range',
             'before_cut', 'before_sword_attack', 'normal_sword_proc', 'melee', 'targeting', 'begin_sword_combo',
             'release_sword_wave', 'wave_cast', 'jump_strike', 'sync_spin_radius', 'after_spin_effect', 'begin_spin_attack', 'update_sword', 'after_sword_move', 'before_large_spin',
             'native_sword_collider', 'after_power', 'before_drop', 'drop_reward', 'select_upgrade']
# Execute Dusklight's actual combo counter/selector, not a reimplementation of
# the sequence. Animation setup remains an observable boundary above.
native_combo = native[native.index('enum daAlink_CutNmParamType'):native.index('static fopAc_ac_c*')]
native_combo_functions = extract('daAlink_c::commonCutAction', native) + '\n' + extract(
    'daAlink_c::checkCutAction', native).replace('daAlink_c::checkCutAction', 'daAlink_c::nativeCheckCutAction')
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    binary = Path(tmp) / 'test'
    cpp.write_text(native_combo + stubs + (root/'src/survivors_manual_attack.inc').read_text().replace('#include "survivors_sword_aim.inc"','') + native_combo_functions + '\n'.join((root/'src/survivors_melee.inc').read_text() if name=='melee' else (root/'src/survivors_targeting.inc').read_text() if name=='targeting' else (root/'src/survivors_jump_strike.inc').read_text() if name=='jump_strike' else (root/'src/survivors_wave_cast.inc').read_text() if name=='wave_cast' else extract(name) for name in functions) + '\n' + main)
    subprocess.run([os.environ.get('CXX','c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root/'src'),
                    str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
