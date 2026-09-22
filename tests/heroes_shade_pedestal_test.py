"""Exercise the actual sword interaction: end wind without resetting the fight."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root/'src/heroes_shade_encounter.cpp').read_text()
start = source.index('int execute_pedestal(')
method = source[start:source.index('\n}', start)+2]
fixture = r'''
#include "heroes_shade_battle.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
namespace shade=dawnlight::shade;
using s16=std::int16_t;
constexpr int PAD_1=0,PAD_BUTTON_A=1,MOD_OK=0,fpcNm_NPC_KN_e=1,kShadeParams=7;
struct cXyz {
    float x=0,y=0,z=0;
    cXyz operator-(cXyz b)const{return {x-b.x,y-b.y,z-b.z};}
    float absXZ()const{return std::sqrt(x*x+z*z);}
};
struct Transform {cXyz pos;};
struct Player {
    Transform current;struct {s16 y=0;}shape_angle;
    bool dead=false,wolf=false;
    bool checkDeadHP(){return dead;}bool checkWolf(){return wolf;}
} player;
auto daAlink_getAlinkActorClass(){return &player;}
bool inArena=true,nextStage=false,event=false,paused=false,menu=false;
bool arena(){return inArena;}bool dComIfGp_isEnableNextStage(){return nextStage;}
bool dComIfGp_event_runCheck(){return event;}bool dComIfGp_isPauseFlag(){return paused;}
bool ui_document_visible(){return menu;}
struct Trials {
    shade::TrialClock clock;bool ready=true;
    bool place(){return ready;}bool prepare_effects(){return ready;}
};
struct Animation {void play(){}};
struct Pedestal {bool ready=true;Animation btk,brk;int collision=0;Trials trials;Transform current;};
struct Space {void Set(int*){}} space;
auto dComIfG_Ccsp(){return &space;}
struct Fighter {int id=-1;};
std::array<Fighter,3> sFighters;
bool pending_or_live(int id){return id>=0;}
struct Cinema {
    bool running=false;int starts=0;
    bool active(){return running;}void begin(bool){running=true;++starts;}
} sCinema;
void release_cinema(){sCinema.running=false;}
shade::Battle sBattle;bool sStopping=false;
s16 cLib_targetAngleY(const cXyz*,const cXyz*){return 0;}
int prompts=0,spawns=0;
void dComIfGp_setDoStatusForce(int code,int){assert(code==8);++prompts;}
struct Pad {unsigned mPressedButtonFlags=0;} pad;
struct mDoCPd_c {
    static bool getTrigA(int){return pad.mPressedButtonFlags&PAD_BUTTON_A;}
    static Pad& getCpadInfo(int){return pad;}
};
struct dBgS_ObjGndChk {void SetPos(cXyz*){}};
struct Ground {float GroundCross(dBgS_ObjGndChk*){return 0;}} ground;
Ground& dComIfG_Bgsp(){return ground;}
int spawn(int,cXyz,s16,int,int& id){id=++spawns;return MOD_OK;}
'''
checks = r'''
int main() {
    Pedestal p;
    // Ordinary start still creates exactly one Shade and begins the intro.
    pad.mPressedButtonFlags=PAD_BUTTON_A|2;
    execute_pedestal(&p);
    assert(spawns==1 && sCinema.starts==1 && sBattle.health==10);
    assert(pad.mPressedButtonFlags==2);
    sCinema.running=false;
    sBattle.health=2;sBattle.phase=7;sBattle.trials_started=4;sBattle.advance=true;
    const auto id=sFighters[0].id;
    for(auto trial:{shade::Trial::None,shade::Trial::Shield,shade::Trial::Fire,shade::Trial::Eyes}) {
        sBattle.trial=trial;p.trials.clock.begin(trial);prompts=0;
        pad.mPressedButtonFlags=PAD_BUTTON_A;
        execute_pedestal(&p);
        assert(prompts==0 && spawns==1 && !p.trials.clock.windReleased);
        p.trials.clock.release_wind();assert(!p.trials.clock.windReleased);
    }
    sBattle.trial=shade::Trial::Wind;p.trials.clock.begin(shade::Trial::Wind);
    // Invalid interactions cannot resolve the trial or restart the encounter.
    for(int condition=0;condition<12;++condition) {
        player.current.pos={};player.shape_angle.y=0;
        player.dead=player.wolf=nextStage=event=paused=menu=sCinema.running=false;
        inArena=p.ready=p.trials.ready=true;
        switch(condition) {
        case 0: player.current.pos.x=231;break;
        case 1: player.current.pos.y=101;break;
        case 2: player.shape_angle.y=0x4000;break;
        case 3: player.dead=true;break;
        case 4: player.wolf=true;break;
        case 5: nextStage=true;break;
        case 6: event=true;break;
        case 7: paused=true;break;
        case 8: menu=true;break;
        case 9: sCinema.running=true;break;
        case 10: inArena=false;break;
        case 11: p.trials.ready=false;break;
        }
        prompts=0;execute_pedestal(&p);
        assert(prompts==0 && !p.trials.clock.windReleased && spawns==1);
    }
    p.trials.ready=true;pad.mPressedButtonFlags=0;
    execute_pedestal(&p);
    assert(prompts==1 && !p.trials.clock.windReleased); // proximity alone does not end wind
    pad.mPressedButtonFlags=PAD_BUTTON_A|2;
    execute_pedestal(&p);
    assert(p.trials.clock.windReleased && pad.mPressedButtonFlags==2);
    assert(spawns==1 && sFighters[0].id==id && sCinema.starts==1 && !sCinema.active());
    assert(sBattle.health==2 && sBattle.phase==7 && sBattle.trials_started==4 && sBattle.advance);
    assert(sBattle.trial==shade::Trial::Wind && p.trials.clock.tick());
    // A second press before Shade processes completion cannot restart the fight.
    prompts=0;pad.mPressedButtonFlags=PAD_BUTTON_A;
    execute_pedestal(&p);assert(prompts==0 && spawns==1);
    sBattle.trial=shade::Trial::None;
    execute_pedestal(&p);assert(prompts==0 && spawns==1);
    // Native progression still requires the two remaining successful counters.
    sBattle.tick();
    for(int hp=1;hp>=0;--hp) {
        sBattle.event(shade::phases[sBattle.phase].success);
        assert(sBattle.health==hp);
        for(int i=0;i<45;++i) sBattle.tick();
        assert(sBattle.dying==(hp==0));
    }
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'pedestal.cpp';exe=Path(tmp)/'pedestal'
    cpp.write_text(fixture+method+checks)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print("Hero's Shade sword interaction, wind resolution and preserved encounter progression: passed")
