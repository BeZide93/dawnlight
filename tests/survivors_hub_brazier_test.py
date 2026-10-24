"""Scaled oil bowl: owned visual/physics hooks and asynchronous hub lifecycle."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
sdk=root/'dusklight'
fixture=r'''
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include "mods/svc/actor.h"
using u32=uint32_t;using u16=uint16_t;
constexpr ActorId kNone=~0u;
constexpr int fpcNm_OBJ_OILTUBO_e=123;
struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){} void set(float a,float b,float c){x=a;y=b;z=c;}};
struct Color{int r,g,b,a;};
struct LIGHT_INFLUENCE {cXyz mPosition;Color mColor{};float mPow=0,mFluctuation=0;};
struct Model {cXyz max{50,64,50};Model* getModelData(){return this;}Model* getJointNodePointer(int){return this;}cXyz* getMax(){return &max;}};
struct Collider {unsigned co=0;bool target=true,attack=true;cXyz pos;float radius=0,height=0;
 void SetCoSPrm(unsigned n){co=n;}void OffTgSetBit(){target=false;}void OffAtSetBit(){attack=false;}
 void SetC(cXyz p){pos=p;}void SetR(float r){radius=r;}void SetH(float h){height=h;}};
struct fopAc_ac_c {struct{cXyz pos;}current;int profile=fpcNm_OBJ_OILTUBO_e;ActorId id=55;cXyz scale;};
struct daObj_Oiltubo_c:fopAc_ac_c {struct{int flags=123;}attention_info;Model container,liquid;Model* mpModel=&container;Model* mpBModel=&liquid;int tevStr=0,matrices=0;Collider mCcCoCyl;void setMtx(){++matrices;}}brazier;
unsigned lights=0,cuts=0,sounds=0,particles=0,draws=0,collisions=0,culls=0,invalidated=0;
Model* drawn=nullptr;cXyz firePos,fireScale;
struct Emitter{void stopCreateParticle(){}void quitImmortalEmitter(){}void becomeInvalidEmitter(){++invalidated;}}emitter;
Emitter* dComIfGp_particle_getEmitter(u32 id){return id?&emitter:nullptr;}
u32 dComIfGp_particle_set(u32 id,u16 effect,cXyz* p,void*,void*,const cXyz* s,int,void*,int,void*,void*,void*){
 assert(effect==0x3ad||effect==0x3af||effect==0x3ae);++particles;firePos=*p;fireScale=*s;return id?id:particles;
}
void dKy_plight_set(LIGHT_INFLUENCE*){++lights;}void dKy_plight_cut(LIGHT_INFLUENCE*){++cuts;}
constexpr int Z2SE_OBJ_FIRE_BURNING=1;
void mDoAud_seStartLevel(int,cXyz*,int,int){++sounds;}
int fopAcM_GetRoomNo(fopAc_ac_c*){return 51;}int dComIfGp_getReverb(int){return 0;}
ActorId fopAcM_GetID(fopAc_ac_c* a){return a->id;}
void fopAcM_setCullSizeBox2(fopAc_ac_c*,Model*){++culls;}
struct CollisionSpace{void Set(Collider*){++collisions;}}cc;
CollisionSpace* dComIfG_Ccsp(){return &cc;}
struct EnvLight{void settingTevStruct(int,cXyz*,int*){}void setLightTevColorType_MAJI(Model*,int*){}}g_env_light;
void mDoExt_modelUpdateDL(Model* m){drawn=m;++draws;}
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* args,int){return static_cast<T>(args);}}
fopAc_ac_c link;bool hasLink=true,creating=false,live=false,failSpawn=false;
unsigned spawns=0,deletes=0,rays=0;
float floorY=-400;
ModContext* mod_ctx=nullptr;
fopAc_ac_c* daAlink_getAlinkActorClass(){return hasLink?&link:nullptr;}
fopAc_ac_c* actor_by_id(ActorId id){return id==55&&live&&!creating?&brazier:nullptr;}
bool fpcM_IsCreating(ActorId id){return id==55&&creating;}
int fopAcM_GetName(fopAc_ac_c* a){return a->profile;}
struct{int8_t room=51;}arenas[1];
struct dBgS_ObjGndChk{cXyz pos;void SetPos(cXyz* p){pos=*p;}};
struct Background{float GroundCross(dBgS_ObjGndChk* g){++rays;assert(g->pos.x==0&&g->pos.z==0&&g->pos.y==link.current.pos.y+1000);return floorY;}}bg;
Background& dComIfG_Bgsp(){return bg;}
ModResult deleteActor(ModContext*,ActorId id){assert(id==55);++deletes;return MOD_OK;}
ActorService actorService{.delete_actor=deleteActor};auto* svc_actor=&actorService;
ModResult create_standalone_actor(int profile,const ActorSpawnParams& p,ActorId& id){
 assert(profile==fpcNm_OBJ_OILTUBO_e&&p.room_num==51&&p.argument==-1&&!p.create_function&&p.parameters==0);
 assert(p.position.x==0&&p.position.y==floorY&&p.position.z==0);
 assert(p.scale.x==2&&p.scale.y==2&&p.scale.z==2&&p.angle.x==0&&p.angle.y==0&&p.angle.z==0);
 ++spawns;if(failSpawn)return MOD_UNAVAILABLE;
 brazier.current.pos={0,floorY,0};id=55;creating=true;return MOD_OK;
}
'''
main=r'''
int main(){
 cXyz anchor;link.current.pos.y=-400;
 update_hub_brazier(false);assert(!spawns&&!hub_brazier_anchor(anchor));
 hasLink=false;update_hub_brazier(true);assert(!rays);hasLink=true;
 for(float invalid:{-1.e9f,std::nanf("")}){
  s_hubBrazierRetry=0;floorY=invalid;update_hub_brazier(true);assert(!spawns);
 }
 floorY=-400;s_hubBrazierRetry=0;failSpawn=true;update_hub_brazier(true);assert(spawns==1&&s_hubBrazier==kNone);
 for(int i=0;i<30;++i){update_hub_brazier(true);}assert(spawns==1);
 failSpawn=false;update_hub_brazier(true);assert(spawns==2&&creating);
 // Scoped hooks: ordinary oil bowls keep their own draw, Execute and Delete.
 daObj_Oiltubo_c other;other.id=99;int result=0;
 assert(before_hub_brazier_execute(nullptr,&other,&result,nullptr)==HOOK_CONTINUE);
 assert(before_hub_brazier_draw(nullptr,&other,&result,nullptr)==HOOK_CONTINUE);
 assert(before_hub_brazier_delete(nullptr,&other,nullptr,nullptr)==HOOK_CONTINUE);
 assert(!particles&&!collisions&&!draws&&!cuts);
 // Owned Execute is intercepted even inside native creation, before ID lookup.
 assert(before_hub_brazier_execute(nullptr,&brazier,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==1);
 assert(brazier.scale.x==2&&brazier.scale.y==2&&brazier.scale.z==2&&!brazier.attention_info.flags);
 assert(brazier.mCcCoCyl.radius==100&&brazier.mCcCoCyl.height==140&&brazier.mCcCoCyl.co==0x79);
 assert(!brazier.mCcCoCyl.target&&!brazier.mCcCoCyl.attack&&collisions==1&&culls==1);
 assert(firePos.y==-272&&fireScale.x==2&&fireScale.y==2&&fireScale.z==2&&particles==3);
 assert(s_hubBrazierLight.mPosition.y==-252&&lights==1&&sounds==1);
 before_hub_brazier_execute(nullptr,&brazier,&result,nullptr);assert(lights==1&&particles==6);
 assert(before_hub_brazier_draw(nullptr,&brazier,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(draws==1&&drawn==brazier.mpModel); // No liquid surface or oil-refill logic.
 const auto placedRays=rays;
 for(int i=0;i<100;++i){update_hub_brazier(true);}assert(spawns==2&&rays==placedRays);
 assert(!hub_brazier_anchor(anchor));creating=false;live=true;
 update_hub_brazier(true);assert(hub_brazier_anchor(anchor)&&anchor.x==0&&anchor.y==-400&&anchor.z==0);
 update_hub_brazier(false);assert(deletes==1&&!hub_brazier_anchor(anchor));
 assert(cuts==1&&invalidated==3&&!s_hubBrazierLightSet);
 before_hub_brazier_execute(nullptr,&brazier,&result,nullptr);assert(particles==6&&lights==1);
 update_hub_brazier(true);assert(deletes==2&&spawns==2); // Rapid re-entry waits for deletion.
 live=false;update_hub_brazier(true);
 for(int i=0;i<31&&!creating;++i){update_hub_brazier(true);}assert(spawns==3&&creating);
 // Leave during async resource loading: finish creation then delete, no duplicate.
 update_hub_brazier(false);assert(s_hubBrazier==55&&s_hubBrazierRetiring&&deletes==2);
 creating=false;live=true;update_hub_brazier(false);assert(deletes==3);
 live=false;update_hub_brazier(false);assert(s_hubBrazier==kNone&&cuts==1);
 // Unexpected native deletion also removes only the owned fire/light.
 s_hubBrazierRetry=0;update_hub_brazier(true);
 before_hub_brazier_execute(nullptr,&brazier,&result,nullptr);assert(lights==2);
 assert(before_hub_brazier_delete(nullptr,&other,nullptr,nullptr)==HOOK_CONTINUE&&cuts==1);
 assert(before_hub_brazier_delete(nullptr,&brazier,nullptr,nullptr)==HOOK_CONTINUE&&cuts==2&&invalidated==6);
 std::cout<<"Survivors brazier: scaled oil bowl, flame alignment, collision, scoped hooks and async cleanup passed\n";
}
'''
native=(sdk/'src/d/actor/d_a_obj_oiltubo.cpp').read_text()
assert '"Obj_otubo"' in native and '"x_oiltubo_00.bmd"' in native and '"x_oiltubo_00b.bmd"' in native
hub=(root/'src/survivors_hub.inc').read_text()
assert 'hub_label("Talents"' not in hub
shade=(root/'src/heroes_shade_encounter.cpp').read_text()
assert 'survivors_open_talents' not in shade and 'sSurvivorsPedestal' not in shade
source=(root/'src/survivors.cpp').read_text()
for name in ['execute','draw','delete']:
 assert f'PRE(SurvivorsHubBrazier{name.title()}, before_hub_brazier_{name});' in source
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_hub_brazier.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-Wno-missing-field-initializers',
  '-I',str(sdk/'sdk/include'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
