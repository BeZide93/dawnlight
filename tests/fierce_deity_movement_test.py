"""Regression for native setFootSpeed's world/local fallback after changeLink(1).

The native function below is from pinned Dusklight 35cdedc d_a_alink.cpp.
No renderer or game assets are needed; both the bad baseline and fix are run.
"""
from pathlib import Path
import subprocess
import tempfile
from fierce_deity_lifecycle_test import function

fixture = r'''
#include <cassert>
#include <cmath>
using f32=float; using s16=short; using u16=unsigned short;
using Mtx=float[3][4];
struct cXyz {
 float x=0,y=0,z=0;
 void set(float a,float b,float c){x=a;y=b;z=c;}
 cXyz operator-(const cXyz& b) const{return {x-b.x,y-b.y,z-b.z};}
};
void MTXConcat(const Mtx a,const Mtx b,Mtx out){
 Mtx temp{};
 for(int i=0;i<3;++i){for(int j=0;j<3;++j)for(int k=0;k<3;++k)temp[i][j]+=a[i][k]*b[k][j];
 temp[i][3]=a[i][3];for(int k=0;k<3;++k)temp[i][3]+=a[i][k]*b[k][3];}
 for(int i=0;i<3;++i)for(int j=0;j<4;++j)out[i][j]=temp[i][j];
}
void mDoMtx_concat(const Mtx a,const Mtx b,Mtx out){MTXConcat(a,b,out);}
struct mDoMtx_stack_c {
 static Mtx matrix; static auto& get(){return matrix;}
 static void multVecZero(cXyz* p){p->set(matrix[0][3],matrix[1][3],matrix[2][3]);}
};
Mtx mDoMtx_stack_c::matrix{};
float cM_scos(s16 a){return std::cos(a*3.14159265358979323846f/32768);}
float cM_ssin(s16 a){return std::sin(a*3.14159265358979323846f/32768);}
struct ModelData {u16 getJointNum(){return 2;}};
struct Model {
 ModelData data; Mtx feet[2]{};
 auto* getModelData(){return &data;} auto& getAnmMtx(int i){assert(i<2);return feet[i];}
};
struct OldFrame {bool valid=false; bool getOldFrameFlg(){return valid;}};
struct daAlink_c {
 Model* mpLinkModel=nullptr; Mtx mInvMtx{};
 u16 field_0x30bc=0,field_0x30be=1;
 cXyz field_0x37b0[2]{};
 struct Position {cXyz pos;struct {s16 y=0;} angle;} current;
 struct {s16 y=0;} shape_angle;
 OldFrame* field_0x2060=nullptr;
 float field_0x33a0=0,mSpeedModifier=0.5f,field_0x33a4=0,mStickValue=0;
 bool checkInputOnR(){return false;}
 void setFootSpeed();
};
struct {bool refreshFootBaseline=false;} s_state;
'''
native = r'''
void daAlink_c::setFootSpeed() {
    int i;
    cXyz sp18[2];

    f32 var_f31;
    if (field_0x2060->getOldFrameFlg()) {
        mDoMtx_concat(mInvMtx, mpLinkModel->getAnmMtx(field_0x30bc), mDoMtx_stack_c::get());
        mDoMtx_stack_c::multVecZero(&sp18[0]);

        mDoMtx_concat(mInvMtx, mpLinkModel->getAnmMtx(field_0x30be), mDoMtx_stack_c::get());
        mDoMtx_stack_c::multVecZero(&sp18[1]);

        int var_r28;
        if (sp18[0].y < sp18[1].y) {
            var_r28 = 0;
        } else {
            var_r28 = 1;
        }

        cXyz sp8 = sp18[var_r28] - field_0x37b0[var_r28];
        s16 temp_r0 = current.angle.y - shape_angle.y;
        var_f31 = fabsf(sp8.z * cM_scos(temp_r0)) + fabsf(sp8.x * cM_ssin(temp_r0));

        if (fabsf(mSpeedModifier) < 1.0f && checkInputOnR() && fabsf(field_0x33a4 - mStickValue) < 0.2f) {
            var_f31 = (0.3f * var_f31) + (0.7f * field_0x33a0);
        }
    } else {
        var_f31 = 0.0f;

        for (i = 0; i < 2; i++) {
            sp18[i] = current.pos;
        }
    }

    for (i = 0; i < 2; i++) {
        field_0x37b0[i] = sp18[i];
    }

    field_0x33a0 = var_f31;
}
'''
checks = r'''
void pose(daAlink_c& link,Model& model,float x,float z,bool rotated,float stride){
 link.current.pos={x,800,z};
 for(int i=0;i<3;++i)for(int j=0;j<4;++j)link.mInvMtx[i][j]=0;
 link.mInvMtx[1][1]=1;link.mInvMtx[1][3]=-800;
 if(rotated){link.mInvMtx[0][2]=-1;link.mInvMtx[2][0]=1;
 link.mInvMtx[0][3]=z;link.mInvMtx[2][3]=-x;}
 else {link.mInvMtx[0][0]=link.mInvMtx[2][2]=1;link.mInvMtx[0][3]=-x;link.mInvMtx[2][3]=-z;}
 for(int i=0;i<2;++i){
  float lx=i?5:-5,ly=i?2:0,lz=10+stride;
  for(int j=0;j<3;++j)model.feet[i][j][j]=1;
  model.feet[i][0][3]=x+(rotated?lz:lx);
  model.feet[i][1][3]=800+ly;
  model.feet[i][2][3]=z+(rotated?-lx:lz);
 }
}
int main(){
 for(bool rotated:{false,true})for(float world:{-50000.0f,50000.0f}){
  for(bool fixed:{false,true}){
   daAlink_c link;Model model;OldFrame oldFrame;
   link.mpLinkModel=&model;link.field_0x2060=&oldFrame;
   pose(link,model,world,world,rotated,0);
   // Native changeLink(1) calls offOldFrameFlg before the first execute.
   link.setFootSpeed();assert(link.field_0x33a0==0);
   assert(link.field_0x37b0[0].z==world); // problematic WORLD fallback
   // The original execute poses the model before our post callback.
   oldFrame.valid=true;s_state.refreshFootBaseline=fixed;
   refresh_foot_baseline(&link);
   pose(link,model,world+12,world+3,rotated,1);
   link.setFootSpeed();
   if(fixed){assert(!s_state.refreshFootBaseline);assert(std::fabs(link.field_0x33a0-1)<0.02f);}
   else {assert(link.field_0x33a0>40000);} // reproduce the pre-fix speed spike
  }
 }
 daAlink_c link;Model model;link.mpLinkModel=&model;
 link.field_0x37b0[0]={3,4,5};s_state.refreshFootBaseline=false;
 refresh_foot_baseline(&link);assert(link.field_0x37b0[0].z==5); // normal ticks untouched
 s_state.refreshFootBaseline=true;link.mpLinkModel=nullptr;
 refresh_foot_baseline(&link);assert(s_state.refreshFootBaseline);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture+native+function('refresh_foot_baseline')+checks)
    subprocess.run(['c++','-std=c++20','-include','initializer_list','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Fierce Deity native foot-speed regression passed: world-space spike reproduced, local baseline restored')
