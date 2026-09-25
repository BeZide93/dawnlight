"""Exercise the authored sampler, actor-local installation and animation lifetime."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
encounter = (root / 'src/heroes_shade_encounter.cpp').read_text()

def function(name):
    start = encounter.index('void ' + name + '(')
    return encounter[start:encounter.index('\n}\n', start) + 3]

native = r'''
#pragma once
#include <cstdint>
using u16=std::uint16_t;using s16=std::int16_t;
struct J3DTransformInfo {
    struct Vec {float x,y,z;};struct Rot {s16 x,y,z;};
    Vec mScale;Rot mRotation;Vec mTranslate;
};
struct J3DAnmTransform {
    int frameMax=30;u16 field_0x1e=37;float frame=0;int attribute=2;
    virtual ~J3DAnmTransform()=default;
    virtual int getKind()const{return 8;}
    int getFrameMax()const{return frameMax;}int getAttribute()const{return attribute;}
    float getFrame()const{return frame;}
    virtual void getTransform(u16,J3DTransformInfo*)const=0;
};
struct J3DAnmTransformKey:J3DAnmTransform {
    void calcTransform(float t,u16 j,J3DTransformInfo* out)const {
        *out={{1,1,1},{123,456,789},{float(j),100+t,2}};
    }
    void getTransform(u16 j,J3DTransformInfo* out)const override {calcTransform(frame,j,out);}
};
'''
fixture = r'''
#include "heroes_shade_stride_animation.hpp"
#include <memory>
#include <new>
#include <cassert>
#include <cstring>
using namespace dawnlight;
struct ModContext{};
namespace mods {
template<class T>T arg(void* raw,int n){return *static_cast<T*>(static_cast<void**>(raw)[n]);}
}
struct Model {
    J3DAnmTransform* animation=nullptr;
    float frame=7,blend=3;
    auto getAnm(){return animation;}void changeAnm(J3DAnmTransform* a){animation=a;}
};
struct Fighter{std::unique_ptr<ShadeStrideAnimation> stride;};
struct daNpc_Kn_c{Model* mpModelMorf[2]{};Fighter entry;bool owned=true;};
Fighter* fighter(daNpc_Kn_c* a){return a&&a->owned?&a->entry:nullptr;}
'''
checks = r'''
int main(){
    J3DAnmTransformKey original;original.frame=13;
    Model model{&original},second{&original};
    daNpc_Kn_c actor{{&model,&second},{},true};auto* owner=&actor;
    int animation=1;bool success=true;void* args[]={&owner,&animation};
    actor.owned=false;install_step_animation(nullptr,args,&success,nullptr);
    assert(!actor.entry.stride&&model.getAnm()==&original);
    actor.owned=true;
    for(int idx=0;idx<5;++idx){
        success=idx!=0;animation=idx==1?25:1;original.frameMax=idx==2?15:30;
        original.field_0x1e=idx==3?36:37;original.attribute=idx==4?0:2;
        install_step_animation(nullptr,args,&success,nullptr);
        assert(!actor.entry.stride&&model.getAnm()==&original);
    }
    original.attribute=2;
    install_step_animation(nullptr,args,&success,nullptr);
    assert(actor.entry.stride&&model.getAnm()==actor.entry.stride.get());
    assert(second.getAnm()==model.getAnm()&&model.frame==7&&model.blend==3);
    auto& custom=*actor.entry.stride;custom.frame=4;
    for(u16 joint=0;joint<37;++joint){
        J3DTransformInfo source{},sample{};original.calcTransform(4,joint,&source);
        custom.weight=0;custom.getTransform(joint,&sample);
        assert(sample.mRotation.x==source.mRotation.x&&sample.mTranslate.y==source.mTranslate.y);
        custom.weight=1;custom.getTransform(joint,&sample);
        assert(sample.mScale.x==1&&sample.mScale.y==1&&sample.mScale.z==1);
        bool leg=false;for(int i:shade::stride::joints)leg|=i==joint;
        if(!leg)assert(sample.mRotation.x==source.mRotation.x&&sample.mRotation.y==source.mRotation.y&&sample.mRotation.z==source.mRotation.z);
        if(joint!=0)assert(sample.mTranslate.x==source.mTranslate.x&&sample.mTranslate.y==source.mTranslate.y&&sample.mTranslate.z==source.mTranslate.z);
    }
    assert(original.frame==13); // Sampling does not change shared animation state.
    ShadeStrideAnimation other(original);other.frame=19;other.weight=.25f;
    J3DTransformInfo a{},b{};custom.getTransform(25,&a);other.getTransform(25,&b);
    assert(a.mRotation.x!=b.mRotation.x&&custom.frame==4&&custom.weight==1);
    for(float time:{0.f,7.25f,15.f,29.99f}){
        shade::stride::Frame a(time),b(time+30);
        for(int j=0;j<6;++j)for(int ax=0;ax<3;++ax)
            assert(std::abs(shade::stride::angle_delta(a.rotation(j,ax),b.rotation(j,ax)))<1);
    }
    for(int j=0;j<6;++j)for(int ax=0;ax<3;++ax){
        float previous=shade::stride::Frame(0).rotation(j,ax);
        for(int i=1;i<=1200;++i){float current=shade::stride::Frame(i/40.f).rotation(j,ax);
            assert(std::abs(shade::stride::angle_delta(current,previous))<150);
            previous=current;
        }
    }
    float weight=0;
    for(int i=0;i<8;++i)weight=shade::stride::blend_weight(weight,true);
    assert(weight==1);
    for(int i=0;i<8;++i)weight=shade::stride::blend_weight(weight,false);
    assert(weight==0);
    // An attack on the second model must not be replaced during detachment.
    J3DAnmTransformKey attack;second.changeAnm(&attack);
    release_step_animation(&actor,actor.entry);
    assert(!actor.entry.stride&&model.getAnm()==&original&&second.getAnm()==&attack);
    release_step_animation(&actor,actor.entry);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    for name, text in [('J3DGraphAnimator/J3DAnimation.h', native), ('J3DGraphBase/J3DTransform.h', '#pragma once\n')]:
        path = tmp / 'JSystem' / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
    cpp=tmp/'test.cpp';exe=tmp/'test'
    cpp.write_text(fixture+function('install_step_animation')+function('release_step_animation')+checks)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(tmp),'-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Shade stride: looping/interpolation, native transforms, actor isolation, guarded installation and safe detachment passed')
