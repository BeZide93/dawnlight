"""Native brazier parameters, central placement and async hub ownership."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
sdk=root/'dusklight'
header=(sdk/'include/d/actor/d_a_obj_lv2Candle.h').read_text()
getters=header[header.index('    int getOffType()'):header.index('    /* 0x568 */')]
fixture=r'''
#include <cassert>
#include <cmath>
#include <iostream>
#include "mods/svc/actor.h"
using u32=uint32_t;using u8=uint8_t;
constexpr ActorId kNone=~0u;
constexpr int fpcNm_Obj_Lv2Candle_e=73;
struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}};
struct fopAc_ac_c {struct{cXyz pos;}current;int profile=fpcNm_Obj_Lv2Candle_e;u32 parameters=0;};
unsigned fopAcM_GetParamBit(fopAc_ac_c* a,int shift,int bits){return (a->parameters>>shift)&((1u<<bits)-1);}
struct NativeBrazier:fopAc_ac_c {
'''+getters+r'''
}brazier;
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
 assert(profile==fpcNm_Obj_Lv2Candle_e&&p.room_num==51&&p.argument==-1&&!p.create_function);
 assert(p.position.x==0&&p.position.y==floorY&&p.position.z==0);
 assert(p.scale.x==1&&p.scale.y==1&&p.scale.z==1&&p.angle.x==0&&p.angle.y==0&&p.angle.z==0);
 ++spawns;if(failSpawn)return MOD_UNAVAILABLE;
 brazier.parameters=p.parameters;brazier.current.pos={0,floorY,0};id=55;creating=true;return MOD_OK;
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
 // Decode with the SDK's actual getters: castle model, always lit, point light,
 // no room switch or wind-extinguishing behavior.
 assert(brazier.getModelType()==2&&brazier.getSw()==0xff&&brazier.getSwType()==0);
 assert(brazier.getLightType()==1&&brazier.getLightAnm()==0&&brazier.getOffType()==0);
 const auto placedRays=rays;
 for(int i=0;i<100;++i){update_hub_brazier(true);}assert(spawns==2&&rays==placedRays);
 assert(!hub_brazier_anchor(anchor));creating=false;live=true;
 update_hub_brazier(true);assert(hub_brazier_anchor(anchor)&&anchor.x==0&&anchor.y==-400&&anchor.z==0);
 update_hub_brazier(false);assert(deletes==1&&!hub_brazier_anchor(anchor));
 update_hub_brazier(true);assert(deletes==2&&spawns==2); // Rapid re-entry waits for deletion.
 live=false;update_hub_brazier(true); // Old ID is retired before a replacement.
 for(int i=0;i<31&&!creating;++i){update_hub_brazier(true);}assert(spawns==3&&creating);
 // Leave during async resource loading: finish creation then delete, no duplicate.
 update_hub_brazier(false);assert(s_hubBrazier==55&&s_hubBrazierRetiring&&deletes==2);
 creating=false;live=true;update_hub_brazier(false);assert(deletes==3);
 live=false;update_hub_brazier(false);assert(s_hubBrazier==kNone);
 std::cout<<"Survivors brazier: native castle parameters, grounded placement, retries and async cleanup passed\n";
}
'''
native=(sdk/'src/d/actor/d_a_obj_lv2Candle.cpp').read_text()
assert '"L2candl",\n    "L4candl",\n    "L9Candle",' in native
assert 'if (mSw == 0xff) {\n            mIsLit = true;' in native
assert 'dKy_plight_cut(&mLight);' in native
hub=(root/'src/survivors_hub.inc').read_text()
assert 'hub_label("Talents"' not in hub
shade=(root/'src/heroes_shade_encounter.cpp').read_text()
assert 'survivors_open_talents' not in shade and 'sSurvivorsPedestal' not in shade
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_hub_brazier.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-Wno-missing-field-initializers',
  '-I',str(sdk/'sdk/include'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
