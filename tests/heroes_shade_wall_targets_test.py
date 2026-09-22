"""Exercise the actual DZB-face alignment using real matrix arithmetic.

Checks ceiling, reversed and tilted archive faces, all four room walls,
identical draw/collision matrices and refusal of non-hookable geometry.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root/'src/heroes_shade_trials.inc').read_text()
start = source.index('    bool align_anchor(')
method = source[start:source.index('\n    }', start)+6]
fixture = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
using s16=std::int16_t;
using Mtx=float[3][4];
constexpr float pi=3.14159265358979323846f;
struct cXyz {
    float x=0,y=0,z=0;
    cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
    cXyz operator-(cXyz b)const{return {x-b.x,y-b.y,z-b.z};}
    float abs()const{return std::sqrt(x*x+y*y+z*z);}
    float absXZ()const{return std::sqrt(x*x+z*z);}
};
s16 cM_atan2s(float y,float x){return static_cast<s16>(static_cast<int>(std::atan2(y,x)*32768/pi));}
void MTXCopy(const Mtx a,Mtx b){std::memcpy(b,a,sizeof(Mtx));}
cXyz transform(const Mtx m,cXyz p){return {
    m[0][0]*p.x+m[0][1]*p.y+m[0][2]*p.z+m[0][3],
    m[1][0]*p.x+m[1][1]*p.y+m[1][2]*p.z+m[1][3],
    m[2][0]*p.x+m[2][1]*p.y+m[2][2]*p.z+m[2][3]};}
struct mDoMtx_stack_c {
    static inline Mtx m{};
    static auto get()->float(*)[4]{return m;}
    static void transS(cXyz p){std::memset(m,0,sizeof(m));for(int i=0;i<3;++i)m[i][i]=1;m[0][3]=p.x;m[1][3]=p.y;m[2][3]=p.z;}
    static void mult(const Mtx b){Mtx out{};for(int i=0;i<3;++i)for(int j=0;j<4;++j){for(int k=0;k<3;++k)out[i][j]+=m[i][k]*b[k][j];if(j==3)out[i][j]+=m[i][3];}MTXCopy(out,m);}
    static void YrotM(s16 a){float c=std::cos(a*pi/32768),s=std::sin(a*pi/32768);Mtx r{{c,0,s,0},{0,1,0,0},{-s,0,c,0}};mult(r);}
    static void XrotM(s16 a){float c=std::cos(a*pi/32768),s=std::sin(a*pi/32768);Mtx r{{1,0,0,0},{0,c,-s,0},{0,s,c,0}};mult(r);}
    static void YrotS(s16 a){transS({0,0,0});YrotM(a);}
    static void multVec(cXyz* p,cXyz* q){*q=transform(m,*p);}
};
struct Triangle {int m_vtx_idx0=0,m_vtx_idx1=1,m_vtx_idx2=2;};
struct Data {int m_t_num=1;std::array<Triangle,1> m_t_tbl;};
struct Background {
    Data data;std::array<cXyz,3> vertices{{{0,0,0},{1,0,0},{0,1,0}}};
    struct {struct {cXyz mNormal;}m_plane;}pm_tri[1];
    bool hookable=true;int moves=0;
    auto GetBgd(){return &data;}auto GetVtxTbl(){return vertices.data();}
    bool GetPolyHSStick(int){return hookable;}void Move(){++moves;}
};
struct Model {Mtx m{};void setBaseTRMtx(const Mtx a){MTXCopy(a,m);}};
struct daHsTarget_c {Background* mpBgW;Model* mpModel;Mtx mBgMtx{};struct{s16 y=0;}shape_angle;};
struct Trials {std::array<cXyz,4> anchorPos{};std::array<s16,4> anchorYaw{};
'''
checks = r'''
};
int main(){
    Trials t;
    for(const cXyz local: {cXyz(0,-1,0),cXyz(0,1,0),cXyz(0,0,-1),cXyz(0,0,1),cXyz(0.36f,0.8f,0.48f)}) {
        for(unsigned i=0;i<4;++i){
            Background bg;Model model;daHsTarget_c actor{};actor.mpBgW=&bg;actor.mpModel=&model;
            actor.shape_angle.y=static_cast<s16>(0x2000+i*0x4000);
            t.anchorYaw[i]=static_cast<s16>(i*0x4000);
            t.anchorPos[i]={100,650,200};
            mDoMtx_stack_c::YrotS(actor.shape_angle.y);
            bg.pm_tri[0].m_plane.mNormal=transform(mDoMtx_stack_c::get(),local);
            assert(t.align_anchor(&actor,i) && bg.moves==1);
            assert(std::memcmp(model.m,actor.mBgMtx,sizeof(Mtx))==0);
            const cXyz face=transform(model.m,local)-t.anchorPos[i];
            assert(std::abs(face.y)<0.001f); // native wall wait requires |normal.y| < .05
            assert(std::abs(face.x-std::sin(t.anchorYaw[i]*pi/32768))<0.001f);
            assert(std::abs(face.z-std::cos(t.anchorYaw[i]*pi/32768))<0.001f);
        }
    }
    Background bg;Model model;daHsTarget_c actor{};actor.mpBgW=&bg;actor.mpModel=&model;
    bg.hookable=false;assert(!t.align_anchor(&actor,0) && !bg.moves);
    actor.mpBgW=nullptr;assert(!t.align_anchor(&actor,0));
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'wall.cpp';binary=Path(tmp)/'wall'
    cpp.write_text(fixture+method+checks)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print("Hero's Shade wall targets: vertical hookable faces and matching model/MoveBG transforms passed")
