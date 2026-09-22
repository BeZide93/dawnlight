"""Exercise the real cinematic fire and draw gate, without game assets."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/heroes_shade_encounter.cpp').read_text()
start = source.index('HookAction draw_cinema(')
draw_hook = source[start:source.index('\n}\n', start) + 3]
fixture = r'''
#include "heroes_shade_cinema.hpp"
#include <array>
#include <algorithm>
#include <cassert>
#include <cmath>
namespace shade=dawnlight::shade;
using u32=unsigned;using u16=unsigned short;using u8=unsigned char;
struct cXyz {float x=0,y=0,z=0;void set(float a,float b,float c){x=a;y=b;z=c;}};
struct daNpc_Kn_c {
    cXyz field_0x16f4;int field_0x170c=0,field_0x170d=0;bool mNoDraw=false;
    struct {cXyz pos{10,20,30};} current;
};
struct Emitter {
    bool alive=false,eventMove=false;int killed=0,cleared=0;
    cXyz scale,pos;u8 alpha=0;
    void becomeInvalidEmitter(){alive=false;++killed;}
    void deleteAllParticle(){++cleared;}
};
std::array<Emitter,3> particles;
bool failParticles=false;int emissions=0;
constexpr u16 ID_ZF_J_FIRE00_GLOW=0x3ad,ID_ZF_J_FIRE01_SPARK=0x3ae,ID_ZF_J_FIRE02_FIRE=0x3af;
Emitter* dComIfGp_particle_getEmitter(u32 id){return id && particles.at(id-1).alive ? &particles.at(id-1) : nullptr;}
u32 dComIfGp_particle_set(u32 key,u16 effect,const cXyz* pos,void* tev,void* rotation,
    const cXyz* scale,u8 alpha,void*,int room,void*,void*,void*){
    assert(effect>=0x3ad && effect<=0x3af && !tev && !rotation && room==51);
    if(failParticles) return 0;
    const u32 id=effect-0x3ad+1;assert(!key || key==id);
    auto& p=particles.at(id-1);p.alive=true;p.pos=*pos;p.scale=*scale;p.alpha=alpha;
    ++emissions;return id;
}
void dComIfGp_particle_levelEmitterOnEventMove(u32 id){particles.at(id-1).eventMove=true;}
constexpr int Z2SE_OBJ_FIRE_IGNITION=1,Z2SE_OBJ_FIRE_BURNING=2,Z2SE_OBJ_FIRE_OFF=3;
int ignitions=0,extinctions=0,burning=0;
int dComIfGp_getReverb(int){return 0;}
void mDoAud_seStart(int sound,cXyz*,int,int){
    assert(sound==1 || sound==3);if(sound==1)++ignitions;else ++extinctions;
}
void mDoAud_seStartLevel(int sound,cXyz*,int,int){assert(sound==2);++burning;}
#include "heroes_shade_flame.inc"
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* args,int){return *static_cast<T*>(args);}}
struct Fighter {bool divide=false,deleting=false;} entry;
bool registered=true;
Fighter* fighter(daNpc_Kn_c*){return registered ? &entry : nullptr;}
shade::Cinema sCinema;
// DRAW_HOOK
bool hidden(daNpc_Kn_c* actor){
    actor->mNoDraw=false; // native twilight() overwrites the flag after our tick
    int result=0;auto action=draw_cinema(nullptr,&actor,&result,nullptr);
    if(action==HOOK_SKIP_ORIGINAL) assert(result==1);
    return action==HOOK_SKIP_ORIGINAL;
}
int main(){
    daNpc_Kn_c actor;
    sCinema.begin(false);assert(hidden(&actor) && ignitions==0 && emissions==0);
    registered=false;assert(!hidden(&actor));registered=true;
    entry.divide=true;assert(!hidden(&actor));entry.divide=false;
    sCinema.begin(true);assert(!hidden(&actor));
    for(bool arrival:{true,false}){
        sCinema.enter(arrival ? shade::Shot::Arrival : shade::Shot::Depart);
        begin_shade_flame(&actor,arrival);
        assert(actor.field_0x16f4.x==1 && hidden(&actor)==arrival);
        float previous=0;
        for(int tick=0;tick<=shade::flame_total_ticks;++tick){
            tick_shade_flame(&actor,tick);
            assert(hidden(&actor)==(arrival ? tick<45 : tick>=45));
            const int count=emissions;const int elapsed=sShadeFlame.ticks;
            hidden(&actor);hidden(&actor);
            assert(emissions==count && sShadeFlame.ticks==elapsed);
            for(auto& p:particles){
                assert(p.alive && p.eventMove && p.pos.y==actor.current.pos.y);
                if(tick<=30) assert(p.scale.y>=previous);
                if(tick>=60) assert(p.scale.y<=previous);
                if(tick>=30 && tick<=60)
                    assert(p.scale.x==35.0f && p.scale.y==45.0f && p.scale.z==35.0f && p.alpha==255);
            }
            previous=particles[0].scale.y;
        }
        for(auto& p:particles) assert(p.alpha==0 && p.scale.y==0);
        end_shade_flame();assert(!sShadeFlame.active);
        for(auto& p:particles) assert(!p.alive && p.killed==p.cleared);
        const int killed=particles[0].killed;end_shade_flame();assert(particles[0].killed==killed);
        sCinema.enter(arrival ? shade::Shot::Words1 : shade::Shot::Afterglow);
        assert(hidden(&actor)==!arrival);
    }
    assert(ignitions==2 && extinctions==2 && burning==120);
    sCinema={};entry.deleting=true;assert(hidden(&actor));entry.deleting=false;assert(!hidden(&actor));
    // Cancellation at every stage removes all emitters; replay starts fresh.
    for(int tick:{0,20,45,70,119}){
        begin_shade_flame(&actor,true);tick_shade_flame(&actor,tick);end_shade_flame();
        for(auto& p:particles) assert(!p.alive);
    }
    // Missing particle resources cannot block the cinematic or dereference an emitter.
    failParticles=true;begin_shade_flame(&actor,true);
    tick_shade_flame(&actor,0);assert(hidden(&actor));
    tick_shade_flame(&actor,60);assert(!hidden(&actor));
    end_shade_flame();
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'flame.cpp'
    exe = Path(tmp) / 'flame'
    cpp.write_text(fixture.replace('// DRAW_HOOK', draw_hook))
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I',
                    str(root / 'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Hero's Shade cinematic flame, visibility, replay and cleanup: passed")
