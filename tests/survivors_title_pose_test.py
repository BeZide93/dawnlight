"""One-knee geometry and temporary title-only animation ownership, without game assets."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <any>
#include <cassert>
#include <cmath>
#include <iostream>
#include "survivors_title_pose.hpp"
using namespace dawnlight;
using namespace dawnlight::survivors;
using u16=unsigned short;using ActorId=unsigned;
constexpr ActorId kNone=~0u;
struct ModContext{};
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* a,int i){return std::any_cast<T>(static_cast<std::any*>(a)[i]);}}
struct J3DTransformInfo {dual::Quat rotation;dual::Vec mTranslate;};
dual::Quat joint_rotation(const J3DTransformInfo& t){return t.rotation;}
void set_joint_rotation(J3DTransformInfo& t,dual::Quat q){t.rotation=q;}
struct J3DAnmTransform {
 virtual ~J3DAnmTransform()=default;
 virtual void getTransform(u16,J3DTransformInfo*)const=0;
 int getKind()const{return 8;}
};
struct J3DAnmTransformKey : J3DAnmTransform {
 unsigned field_0x1e=35;
 std::array<J3DTransformInfo,35> sample{};
 void getTransform(u16 j,J3DTransformInfo* out)const override{*out=sample[j];}
 void calcTransform(float f,u16 j,J3DTransformInfo* out)const{assert(f==0);*out=sample[j];}
};
struct J3DJoint {
 unsigned id=0;J3DJoint* child=nullptr;J3DJoint* younger=nullptr;
 unsigned getJntNo(){return id;}J3DJoint* getChild(){return child;}J3DJoint* getYounger(){return younger;}
};
struct ModelData {
 unsigned count=35;std::array<J3DJoint,35> joints{};
 unsigned getJointNum(){return count;}
 J3DJoint* getJointNodePointer(unsigned i){return &joints[i];}
};
struct J3DModel{ModelData data;ModelData* getModelData(){return &data;}} model,foreignModel;
struct Pack{J3DAnmTransform* animation=nullptr;
 J3DAnmTransform* getAnmTransform(){return animation;}void setAnmTransform(J3DAnmTransform* p){animation=p;}};
struct daAlink_c{ActorId id=7;J3DModel* mpLinkModel=&model;Pack mNowAnmPackUnder[3],mNowAnmPackUpper[3];}link,foreign;
bool active=true;
bool survivors_title_active(){return active;}
daAlink_c* daAlink_getAlinkActorClass(){return &link;}
ActorId fopAcM_GetID(daAlink_c* p){return p->id;}
'''
main=r'''
bool near(float a,float b){return std::abs(a-b)<.01f;}
int main(){
 // Human Link segment lengths from setFootMatrix; an idle FK fixture with
 // the native joint layout. This verifies geometry, not the shipped mesh.
 TitleRestPose pose;pose.parent.fill(0);pose.parent[0]=-1;
 pose.local[0].p={0,79.363499f,0};
 pose.parent[1]=0;pose.local[1].p={0,15,0};
 pose.parent[4]=1;pose.local[4].p={0,35,0};
 for(unsigned first:{7u,12u}){
  const float side=first==7?1.f:-1.f;
  pose.parent[first]=1;pose.local[first].p={side*18,22,0};
  pose.parent[first+1]=first;pose.local[first+1].p={0,-24,0};
  pose.parent[first+2]=first+1;pose.local[first+2].p={0,-26,0};
  pose.parent[first+3]=first+2;pose.local[first+3].p={0,-4,0};
 }
 pose.parent[16]=0;
 for(unsigned first:{18u,23u}){
  pose.parent[first]=16;pose.local[first].p={first==18?9.f:-9.f,0,0};
  pose.parent[first+1]=first;pose.local[first+1].p={0,-30,0};
  pose.parent[first+2]=first+1;pose.local[first+2].p={0,-39.363499f,0};
 }
 // Include a non-identity ankle basis and a toe descendant: the flip must
 // be in actor space, rather than an arbitrary bone-local rotation.
 pose.local[25].q=dual::from_euler({0,.7f,.4f});
 pose.parent[26]=25;pose.local[26].p=dual::rotate(dual::conjugate(pose.local[25].q),{0,0,14.18f});
 pose.evaluate();
 const auto originalSole=dual::rotate(dual::conjugate(pose.world[25].q),{0,-1,0});
 const auto idle=pose;
 assert(pose.kneel());
 assert(near(pose.world[24].p.y,6)); // Floor knee centre includes its thickness.
 assert(near(pose.world[20].p.y,10)&&near(pose.world[25].p.y,10));
 const auto sole=dual::rotate(pose.world[25].q,originalSole);
 assert(sole.y>.999f&&std::abs(sole.x)<.001f&&std::abs(sole.z)<.001f);
 assert(pose.world[26].p.z<pose.world[25].p.z-14&&near(pose.world[26].p.y,pose.world[25].p.y));
 assert(dual::rotate(pose.world[20].q,{0,-1,0}).y<-.999f); // Planted sole still faces down.
 assert(pose.world[19].p.y>pose.world[18].p.y&&pose.world[19].p.z>25);
 assert(pose.world[25].p.z<-35&&near(pose.world[24].p.z,pose.world[23].p.z));
 for(unsigned first:{18u,23u}){
  assert(near(dual::length(pose.world[first+1].p-pose.world[first].p),30));
  assert(near(dual::length(pose.world[first+2].p-pose.world[first+1].p),39.363499f));
 }
 assert(dual::length(pose.world[9].p-(pose.world[19].p+dual::Vec{0,8,3}))<.02f);
 assert(near(dual::length(pose.world[10].p-pose.world[9].p),4)); // Socket follows hand.
 auto bad=idle;bad.parent[18]=34;assert(!bad.kneel());
 J3DAnmTransformKey original[6];
 for(unsigned i=0;i<35;++i){
  auto& j=model.data.joints[i];j.id=i;
  if(i){auto& p=model.data.joints[idle.parent[i]];j.younger=p.child;p.child=&j;}
  for(auto& clip:original)clip.sample[i]={idle.local[i].q,idle.local[i].p};
 }
 for(unsigned i=0;i<6;++i)(i<3?link.mNowAnmPackUnder[i]:link.mNowAnmPackUpper[i-3]).animation=&original[i];
 std::any args[]{&link,&model},other[]{&foreign,&model},item[]{&link,&foreignModel},joint[]{&link};
 assert(before_title_model(nullptr,other,nullptr,nullptr)==HOOK_CONTINUE&&!s_titleModel.call);
 assert(before_title_model(nullptr,item,nullptr,nullptr)==HOOK_CONTINUE&&!s_titleModel.call);
 for(int frame=0;frame<100;++frame){
  before_title_model(nullptr,args,nullptr,nullptr);assert(title_model_ready(&link)&&s_titleModel.call==args);
  for(unsigned i=0;i<6;++i)assert((i<3?link.mNowAnmPackUnder[i]:link.mNowAnmPackUpper[i-3]).animation==&s_titleModel.animation);
  bool result=false;assert(before_title_joint(nullptr,joint,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result);
  assert(before_title_joint(nullptr,other,&result,nullptr)==HOOK_CONTINUE);
  J3DTransformInfo transformed;s_titleModel.animation.getTransform(0,&transformed);
  assert(near(transformed.mTranslate.y,pose.local[0].p.y)); // No accumulating root offset.
  after_title_model(nullptr,item,nullptr,nullptr);assert(s_titleModel.call==args);
  after_title_model(nullptr,args,nullptr,nullptr);assert(!s_titleModel.call);
  for(unsigned i=0;i<6;++i)assert((i<3?link.mNowAnmPackUnder[i]:link.mNowAnmPackUpper[i-3]).animation==&original[i]);
  assert(before_title_joint(nullptr,joint,&result,nullptr)==HOOK_CONTINUE);
 }
 active=false;before_title_model(nullptr,args,nullptr,nullptr);assert(!s_titleModel.call);
 active=true;link.id=8;assert(!title_model_ready(&link));
 before_title_model(nullptr,args,nullptr,nullptr);assert(title_model_ready(&link));after_title_model(nullptr,args,nullptr,nullptr);
 model.data.count=40;before_title_model(nullptr,args,nullptr,nullptr);assert(!title_model_ready(&link)&&!s_titleModel.call);
 model.data.count=35;original[0].field_0x1e=40;before_title_model(nullptr,args,nullptr,nullptr);assert(!title_model_ready(&link));
 std::cout<<"Survivors title pose: floor knee, planted foot, bone lengths, hand/socket, scoped borrowing and restoration passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'pose.cpp';exe=Path(tmp)/'pose'
 cpp.write_text(fixture+(root/'src/survivors_title_pose.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
