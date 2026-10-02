#include "enemy_slow_motion/timing.hpp"
#include <algorithm>
#include <array>
#include <any>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>
using s8=int8_t;using u8=uint8_t;using s16=int16_t;using u16=uint16_t;using u32=uint32_t;
using JAISoundID=u32;
struct cXyz {
    float x=0,y=0,z=0;
    cXyz()=default;cXyz(float x,float y,float z):x(x),y(y),z(z){}
    cXyz operator+(const cXyz& p)const{return {x+p.x,y+p.y,z+p.z};}
    cXyz operator-(const cXyz& p)const{return {x-p.x,y-p.y,z-p.z};}
    cXyz operator*(float f)const{return {x*f,y*f,z*f};}
    static const cXyz Zero;
};
const cXyz cXyz::Zero{};
struct csXyz{s16 x=0,y=0,z=0;};
struct Event {bool catchCommand=false;bool checkCommandCatch()const{return catchCommand;}};
struct fopAc_ac_c {
    struct {cXyz pos;csXyz angle;} current,old;
    csXyz shape_angle;cXyz speed;
    float speedF=0,gravity=-3;
    int health=4,profile=0,parentActorID=0;
    Event eventInfo;
};
struct dBgS_Acch {bool ground=false;bool ChkGroundHit(){return ground;}};
struct J3DFrameCtrl{};
struct Model {void setBaseTRMtx(const float*){}};
using J3DModel = Model;
struct mDoExt_morf_c{};
struct mDoExt_McaMorfSO:mDoExt_morf_c {
    float frame=0;Model model;
    float getFrame(){return frame;}void setFrame(float f){frame=f;}
    Model* getModel(){return &model;}
};
struct mDoExt_brkAnm {J3DFrameCtrl ctrl;J3DFrameCtrl* getFrameCtrl(){return &ctrl;}};
struct Z2CreatureEnemy{};struct Z2SoundHandlePool{};
struct e_mm_class {
    fopAc_ac_c enemy;mDoExt_McaMorfSO* modelMorf=nullptr;dBgS_Acch acch;
    Z2CreatureEnemy sound;float field_0x6a8=0;s16 timers[4]{},field_0x6a4=0;
};
struct e_mm_mt_class {
    fopAc_ac_c enemy;Model* mp_model=nullptr;int m_action=0;
    s16 m_timer[2]{},m_invulnerabilityTimer=0;
};
struct e_ai_class:fopAc_ac_c {
    enum {ACTION_WAIT,ACTION_MOVE,ACTION_ATTACK,ACTION_DAMAGE,ACTION_RETURN};
    mDoExt_McaMorfSO* m_modelMorf=nullptr;mDoExt_brkAnm* m_brk=nullptr;dBgS_Acch m_acch;
    int m_action=0,m_mode=0,field_0x692=0;
    s16 m_lifetime=0,m_timers[4]{},m_invulnerabilityTimer=0,field_0x6bc=0,field_0x6ba=0,field_0x6a8=0;
    float field_0x6c0=0;
};
struct daE_GE_c:fopAc_ac_c {
    void executeFly();void executeAttack();void executeBack();void mtx_set();
    bool checkCircleSpeedAdd(cXyz*,cXyz*);
    void setAddCalcSpeed(cXyz&,const cXyz&,float,float,float,float);
    cXyz calcCircleFly(cXyz*,cXyz*,s16,float,s16,float);
    mDoExt_McaMorfSO* mpMorfSO=nullptr;dBgS_Acch mObjAcch;
    int mActionMode=0,mMode=0,mSubMode=0;
    float field_0xb58=0,field_0xb5c=0;
    s16 field_0xb8a=0,field_0xb8c=0,field_0xb8e[2]{},mDamageCooldownTimer=0,
        mAnmChangeTimer=0,mSurpriseTime=0;
    int mBackAnimeTimer=0;
};
struct e_kr_class {
    fopAc_ac_c enemy;mDoExt_McaMorfSO* mpMorf=nullptr;Z2CreatureEnemy mSound;
    void* field_0x6e4=nullptr;int field_0x66b=0;dBgS_Acch mAcch;
    s16 mCurAction=0,field_0x672=0,field_0x6d6=0,field_0x6d8=0,field_0x69c[6]{},
        field_0xe82=0,field_0xe84=0,field_0x6aa=0,field_0x6c8=0,field_0x6a8=0,
        field_0xebe=0,field_0x6d4=0,field_0xe7c=0,field_0xe80=0,field_0xeae=0,
        field_0xeb0=0,field_0xeb6=0,field_0xe8e[11]{},field_0xeac=0;
    csXyz field_0x6ea,field_0x6f0;cXyz field_0x678;
    float field_0x68c=0,field_0x690=0,field_0xea8=0,field_0xeb8=0,field_0xef8=0;
    JAISoundID field_0xe88=0;
};
enum {ACTION_NORMAL_MOVE,ACTION_ATTACK,ACTION_COMBINE,ACTION_ROOF=10,ACTION_WATER,ACTION_FAIL=20};
struct e_sm2_class {
    fopAc_ac_c enemy;mDoExt_McaMorfSO* modelMorf=nullptr; mDoExt_McaMorfSO* pieceModelMorf=nullptr;
    int isPiece=0,action=0,mode=0,sizetype=0;dBgS_Acch acch;
    float size=1,color_R=0,color_G=0,color_B=0,color_alpha=1,field_0x6ac=0,field_0x6b0=1,
        field_0x82c=0,field_0x830=0,field_0x838=0,field_0x6c8[8]{};
    cXyz field_0x840,field_0x708[8]{},jnt_pos[8]{};
    csXyz field_0x84c,field_0x768[8]{},field_0x7f8[8]{};
    s16 mCurrentAngleYTargetStep=0,counter=0,timers[3]{},invulernabilityTimer=0,combine_off_timer=0,field_0x828=0;
    s8 field_0x83f=0;u8 field_0x6a9=0,field_0x6aa=0,field_0x83e=0;
};
fopAc_ac_c* parent=nullptr;
fopAc_ac_c* fopAcM_SearchByID(int){return parent;}
int fopAcM_GetName(fopAc_ac_c* p){return p->profile;}
float cM_ssin(s16 a){return std::sin(float(a)*3.14159265359f/32768);}
float cM_scos(s16 a){return std::cos(float(a)*3.14159265359f/32768);}
s16 cM_atan2s(float x,float z){return std::atan2(x,z)*32768/3.14159265359f;}
#define TREG_F(x) 0.0f
#define TREG_S(x) 0
#define BREG_S(x) 0
#define KREG_S(x) 0
#define KREG_F(x) 0.0f
#define NREG_S(x) 0
#define NREG_F(x) 0.0f
#define ZREG_S(x) 0
#define XREG_S(x) 0
namespace mDoMtx_stack_c {
float matrix[16]{};
void transS(const cXyz&){}void transS(float,float,float){}
void YrotM(s16){}void XrotM(s16){}void ZrotM(s16){}void scaleM(float,float,float){}
const float* get(){return matrix;}
}
struct ModContext{};
enum ModResult{MOD_OK,MOD_ERROR};
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
#define DEFINE_HOOK(target, name) struct name{}
#define DEFINE_HOOK_SYMBOL(symbol, signature, name) struct name{}
// Unlike the generic asset-free fixtures, keep Guay hook return types visible:
// the pinned SDK cannot safely turn a struct-returning MSVC member into a
// free-function trampoline, even when every callback is a no-op.
template<class Signature>struct GuayHookSignature;
template<class C,class R,class... A>struct GuayHookSignature<R(C::*)(A...)> {
    static_assert(std::is_void_v<R> || std::is_scalar_v<R>,
        "Guay hooks must not use the SDK's unsafe aggregate-return trampoline");
};
template<class R,class... A>struct GuayHookSignature<R(A...)> {
    static_assert(std::is_void_v<R> || std::is_scalar_v<R>,
        "Guay hooks must not use the SDK's unsafe aggregate-return trampoline");
};
void* svc_hook=nullptr;
namespace mods {
template<class T>T arg(void* p,int i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}
template<class T>T& arg_ref(void* p,int i){return *std::any_cast<T>(&static_cast<std::any*>(p)[i]);}
namespace hook {
template<class T,class F>ModResult add_pre(void*,F){return MOD_OK;}
template<class T,class F>ModResult add_post(void*,F){return MOD_OK;}
}}
namespace dawnlight {
// PRODUCTION_PROFILE
EnemySlowStep* live=nullptr;
EnemySlowStep* current_enemy_slow_step(){return live;}
}
