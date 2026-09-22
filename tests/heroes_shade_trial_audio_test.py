"""Exercise the production trial audio's per-source lifetime and event timing."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
fixture=r'''
#include "heroes_shade_battle.hpp"
#include <array>
#include <algorithm>
#include <cassert>
#include <vector>
namespace shade=dawnlight::shade;
using u32=unsigned;
struct cXyz {
    float x=0,y=0,z=0;
    cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}
    cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
    cXyz operator*(float s)const{return{x*s,y*s,z*s};}
    float inprod(cXyz b)const{return x*b.x+y*b.y+z*b.z;}
};
enum {Z2SE_OBJ_GANON_BARRIER_APPR=1,Z2SE_OBJ_GANON_BARRIER,
      Z2SE_EN_PZ_BALL_BURST,Z2SE_EN_BM_FIND,Z2SE_EN_BM_HEAT,
      Z2SE_EN_BM_BEAM2,Z2SE_EN_BM_SPARK};
struct Cue {u32 id;cXyz pos;};std::vector<Cue> cues;
int dComIfGp_getReverb(int room){assert(room==51);return 3;}
void mDoAud_seStart(u32 id,const cXyz* p,int,int r){assert(r==3);cues.push_back({id,*p});}
struct Z2SoundObjSimple {
    cXyz* position=nullptr;cXyz drawn{};u32 id=0;
    bool playing=false;int inits=0,stops=0,updates=0;
    bool isAlive(){return position!=nullptr;}
    void init(cXyz* p,int handles){assert(!isAlive() && handles==1);position=p;++inits;}
    void framework(int,int r){assert(r==3);drawn=*position;++updates;}
    void startLevelSound(u32 sound,int,int r){assert(r==-1);id=sound;playing=true;}
    void stopAllSounds(int fade){assert(fade==0);playing=false;++stops;}
};
#include "heroes_shade_trial_audio.inc"
int main(){
    ShadeTrialAudio audio;
    cXyz boss{10,110,20};std::array<cXyz,2> eyes{{{-100,200,0},{100,200,0}}};
    audio.begin(shade::Trial::Shield,boss,eyes);
    assert(cues.size()==1 && cues.back().id==Z2SE_OBJ_GANON_BARRIER_APPR);
    for(int i=0;i<30;++i){boss.x+=1;audio.update_shield(false,boss);}
    assert(audio.shield.sound.inits==1 && audio.shield.sound.drawn.x==boss.x);
    assert(audio.shield.sound.id==Z2SE_OBJ_GANON_BARRIER);
    audio.stop();audio.stop(); // Pause stops just this owned source once.
    assert(audio.shield.sound.stops==1);
    audio.update_shield(false,boss);assert(audio.shield.sound.inits==1);
    for(int i=0;i<12;++i)audio.update_shield(true,boss);
    assert(!audio.shield.sound.playing && cues.size()==2);
    assert(cues.back().id==Z2SE_EN_PZ_BALL_BURST && cues.back().pos.x==boss.x);
    audio.stop();audio.update_shield(true,boss);assert(cues.size()==2);
    audio.begin(shade::Trial::Eyes,boss,eyes);
    assert(cues.size()==6); // One detection and charge cue per eye.
    cXyz end{0,0,0},listener{0,30,0};
    for(unsigned i=0;i<2;++i){
        audio.update_beam(i,false,eyes[i],end,0,listener);
        assert(audio.beams[i].sound.inits==0);
        audio.update_beam(i,true,eyes[i],end,0.3f,listener);
        assert(audio.beams[i].sound.playing && !audio.sparks[i].sound.playing);
        audio.update_beam(i,true,eyes[i],end,1,listener);
        assert(audio.sparks[i].sound.playing && audio.sparks[i].sound.id==Z2SE_EN_BM_SPARK);
        assert(audio.beams[i].sound.id==Z2SE_EN_BM_BEAM2);
    }
    assert(audio.beams[0].sound.position!=audio.beams[1].sound.position);
    audio.update_beam(0,false,eyes[0],end,0,listener); // Destroy one eye.
    assert(!audio.beams[0].sound.playing && !audio.sparks[0].sound.playing);
    assert(audio.beams[1].sound.playing && audio.sparks[1].sound.playing);
    audio.stop();assert(!audio.beams[1].sound.playing && !audio.sparks[1].sound.playing);
    audio.update_beam(1,true,eyes[1],end,1,listener); // Resume without reallocating.
    assert(audio.beams[1].sound.inits==1 && audio.sparks[1].sound.inits==1);
    audio.update_beam(1,true,end,end,0,listener); // No division by zero at startup.
    assert(audio.beams[1].sound.drawn.x==0 && !audio.sparks[1].sound.playing);
    audio.begin(shade::Trial::Fire,boss,eyes);assert(!audio.beams[1].sound.playing);
    audio.begin(shade::Trial::Shield,boss,eyes);audio.update_shield(true,boss);
    assert(cues.size()==8 && cues.back().id==Z2SE_EN_PZ_BALL_BURST); // Fresh encounter.
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'audio.cpp';exe=Path(tmp)/'audio';cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Shade shield/Beamos audio timing, positions, separate handles and cleanup: passed')
