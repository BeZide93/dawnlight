"""Run the production guard and pinned native Armos death routine without game assets."""
from pathlib import Path
import argparse
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--dusklight-dir', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
source = (root / 'src/enemy_slow_motion/armos.cpp').read_text()
guard = source[source.index('HookAction before_damage('):source.index('bool eligible(')]
native_source = (args.dusklight_dir / 'src/d/actor/d_a_e_ai.cpp').read_text()
native = native_source[native_source.index('void e_ai_class::e_ai_damage()'):native_source.index('void e_ai_class::e_ai_attack()')]
ids = sorted(set(re.findall(r'\bZ2SE_\w+\b', guard + native)))
fixture = r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
using s16=int16_t; using u16=uint16_t;
struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float x,float y,float z):x(x),y(y),z(z){}};
struct csXyz {s16 x=0,y=0,z=0;};
// SOUND_IDS
int requests=0,flashes=0,explosions=0,deletions=0,switchWrites=0;
bool failParticle=false;
struct JPABaseEmitter {
    bool immortal=false;cXyz position;int follows=0;
    void becomeImmortalEmitter(){immortal=true;}
    void setGlobalTranslation(const cXyz& p){position=p;++follows;}
} emitter;
struct Sound {
    void startCreatureSound(int id,int,int){if(id==Z2SE_EN_AI_FLASH)++flashes;}
    void startCreatureVoice(int,int){}
};
struct Morph {
    bool stop=false;
    bool checkFrame(float){return false;}bool isStop(){return stop;}void setFrame(float){}
};
struct Acch {bool ChkGroundHit(){return false;}};
struct e_ai_class {
    enum {ACTION_MOVE=1};
    int field_0x692=0,field_0x694=0,field_0x696=0,field_0x67c=0,m_mode=1,m_action=3;
    s16 m_timers[4]{},field_0x6a8=0,m_lifetime=10;
    unsigned char m_swbit=0xff;
    float speedF=0,gravity=0;
    cXyz speed;struct {cXyz pos;csXyz angle;} current;csXyz shape_angle;
    int field_0xd2c=0,field_0xd34=0,tevStr=0;
    JPABaseEmitter* mpEmitter=nullptr;
    Morph morph;Morph* m_modelMorf=&morph;Sound m_sound;Acch m_acch;
    void anm_init(int,float,int,float){}
    void e_ai_damage();
};
struct {float movement_speed=10;} l_HIO;
#define TREG_F(x) 0.0f
#define TREG_S(x) 0
#define YREG_S(x) 0
#define yREG_F(x) 0.0f
float cM_rndF(float limit){return limit*.25f;}
float cM_ssin(s16 angle){return std::sin(angle*3.14159265359f/32768);}
struct Vibration {void StartShock(int,int,cXyz){}} vibration;
Vibration& dComIfGp_getVibration(){return vibration;}
void fopAcM_effSmokeSet1(int*,int*,cXyz*,csXyz*,float,int*,int){}
JPABaseEmitter* dComIfGp_particle_set(u16 id,const cXyz*,const int*,const csXyz*,const cXyz*){
    assert(id==0x81ed);++requests;return failParticle?nullptr:&emitter;
}
void fopAcM_createDisappear(e_ai_class*,cXyz* pos,int a,int b,int c){
    assert(a==12&&b==0&&c==0x1e);assert(pos->y==80);++explosions;
}
bool dComIfGs_isSwitch(int,int){return switchWrites!=0;}
void dComIfGs_onSwitch(int,int){++switchWrites;}
int fopAcM_GetRoomNo(e_ai_class*){return 0;}
void fopAcM_delete(e_ai_class*){++deletions;}
struct ModContext{};enum HookAction{HOOK_CONTINUE};
namespace mods {template<class T>T arg(void* args,int i){return *static_cast<T*>(static_cast<void**>(args)[i]);}}
'''
fixture = fixture.replace('// SOUND_IDS', 'enum { ' + ', '.join(ids) + ' };')
fixture += guard + native
fixture += r'''
void guarded_damage(e_ai_class& a){
    auto* actor=&a;void* args[]{&actor};
    assert(before_damage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    a.e_ai_damage();
}
void reset(bool fail){
    requests=flashes=explosions=deletions=switchWrites=0;emitter={};failParticle=fail;
}
int main(){
    // Missing resources / exhausted pool return null. Successful creation keeps
    // a real immortal emitter. Both paths reach the native explosion and switch.
    for(bool fail:{false,true}) for(int cadence:{1,4}) for(int sw:{0xff,7}) {
        reset(fail);e_ai_class a;a.m_swbit=sw;
        guarded_damage(a);
        assert(requests==1&&flashes==1&&a.m_timers[1]==1000&&a.m_timers[2]==56);
        assert(a.mpEmitter==(fail?nullptr:&emitter));assert(emitter.immortal==!fail);
        assert(a.speedF==5&&a.current.angle.y==0x800); // native motion still ran
        for(int frame=1;frame<=55*cadence;++frame){
            if(frame%cadence==0)for(auto& timer:a.m_timers)if(timer)--timer;
            guarded_damage(a);
            assert(requests==1&&flashes==1); // never retry a failed flash every frame
            assert(deletions==(frame==55*cadence?1:0));
        }
        assert(explosions==1&&deletions==1&&switchWrites==(sw==0xff?0:1));
        if(!fail)assert(emitter.follows==1+55*cadence);
    }
    // Waiting, damage recovery, and the start of the death sequence must not
    // trigger an early flash. Mode zero still initializes its native 70 ticks.
    reset(true);e_ai_class a;a.m_mode=0;guarded_damage(a);
    assert(a.m_mode==1&&a.m_timers[1]==70&&requests==0);
    guarded_damage(a);assert(requests==0);
    a.field_0x692=1;a.m_timers[1]=0;guarded_damage(a);assert(requests==0);
    // Different actors have independent countdowns and retain their own pointer.
    reset(false);e_ai_class first,second;guarded_damage(first);
    failParticle=true;guarded_damage(second);
    assert(first.mpEmitter==&emitter&&second.mpEmitter==nullptr&&requests==2);
    assert(first.m_timers[2]==56&&second.m_timers[2]==56);
    std::cout<<"Armos native death: available/missing effect, timer cadence, switches and deletion passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'armos.cpp'
    cpp.write_text(fixture)
    binary = Path(tmp) / 'armos'
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
