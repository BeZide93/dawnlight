"""Exercise production sprint settings, cadence, and manual-jump momentum.

Run with python3 tests/sprint_speed_test.py; no game assets required.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/jump_hooks.cpp').read_text()
config = (root / 'src/config.cpp').read_text()


def function(signature, text=source):
    start = text.index(signature + '(')
    return text[start:text.index('\n}', start) + 2]


fixture = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <array>
using f32=float;
using u16=unsigned short;
constexpr int PAD_1=0;
constexpr bool FALSE=false;
constexpr u16 kSwordItem=0x103;
struct ModContext {};
enum HookAction {HOOK_CONTINUE};
namespace mods {
template<class T> T& arg_ref(void* args,int i){return *static_cast<T*>(static_cast<void**>(args)[i]);}
template<class T> T arg(void* args,int i){return arg_ref<T>(args,i);}
}
bool enabled=true,stamina=true,event=false,jumpReady=true,bPressed=false;
int staminaMarks=0,manualMarks=0,manualClears=0;
bool sprint_enabled(){return enabled;}
bool stamina_available_for_sprint(){return stamina;}
void mark_sprint_stamina_active(){++staminaMarks;}
bool dComIfGp_event_runCheck(){return event;}
namespace mDoCPd_c {
bool getHoldB(int){return bPressed;}
bool getTrigB(int){return bPressed;}
}
int s_sprintSpeedPercent=1,configPercent=150;
bool hasConfig=true;
int get_int(int,int fallback,int low,int high){return hasConfig?std::clamp(configPercent,low,high):fallback;}
struct Hio {
    struct Move {struct Values {f32 mMaxSpeed=23;} m;} mMove;
} hio;
struct daAlink_c {
    enum daAlink_PROC {PROC_MOVE,PROC_WAIT,PROC_AUTO_JUMP,PROC_CUT_JUMP,PROC_DAMAGE};
    enum daAlink_ANM {ANM_RUN,ANM_RUN_B,ANM_WALK,ANM_WAIT,ANM_ATTACK};
    daAlink_PROC mProcID=PROC_MOVE;
    struct Acch {bool ground=true;bool ChkGroundHit(){return ground;}void ClrGroundHit(){ground=false;}} mLinkAcch;
    struct Grab {void* actor=nullptr;void* getActor(){return actor;}} mGrabItemAcKeep;
    Hio* mpHIO=&hio;
    f32 mMaxSpeed=23,mNormalSpeed=23,speedF=23,gravity=-3.4f,groundProjection=1;
    struct Speed {f32 y=12;} speed;
    struct Angles {int y=0;} shape_angle;
    struct Position {Angles angle;} current;
    int mMoveAngle=123;
    u16 mEquipItem=0;
    bool rollHeld=true,rollTrigger=false,input=true,wolf=false,cutscene=false,boots=false,sumo=false;
    bool initSucceeds=true,jumpMode=false;
    bool doButton(){return rollHeld;}
    bool doTrigger(){return rollTrigger;}
    bool checkInputOnR(){return input;}
    bool checkWolf(){return wolf;}
    bool checkEventRun(){return cutscene;}
    bool checkMagneBootsOn(){return boots;}
    bool getSumouMode(){return sumo;}
    f32 getMoveGroundAngleSpeedRate(){return std::fabs(mNormalSpeed*groundProjection/mMaxSpeed);}
    void setJumpMode(){jumpMode=true;}
    bool procAutoJumpInit(int);
    bool procCutJumpInit(bool);
};
const daAlink_c* s_manualJumpOwner=nullptr;
daAlink_c* s_sprintOwner=nullptr;
float jump_height_multiplier(){return 1;}
void jump_abilities_proc_change(daAlink_c*,daAlink_c::daAlink_PROC){}
bool jump_state_ready(daAlink_c* link){return link&&jumpReady;}
void mark_manual_jump_started(daAlink_c*){++manualMarks;}
void clear_manual_jump(daAlink_c*){++manualClears;}
// FUNCTIONS
void transition(daAlink_c* link,daAlink_c::daAlink_PROC proc){
    void* args[]={&link,&proc};before_common_proc_init(nullptr,args,nullptr,nullptr);
    link->mProcID=proc;
}
bool daAlink_c::procAutoJumpInit(int){
    if(!initSucceeds)return false;
    transition(this,PROC_AUTO_JUMP);
    mMaxSpeed=26;speedF=26;mNormalSpeed=20;speed.y=25;gravity=-3.4f;
    return true;
}
bool daAlink_c::procCutJumpInit(bool){
    if(!initSucceeds)return false;
    transition(this,PROC_CUT_JUMP);
    mMaxSpeed=25;speedF=25;mNormalSpeed=25;speed.y=20;
    return true;
}
void close(float a,float b){assert(std::fabs(a-b)<0.002f);}
std::array<float,2> animate(daAlink_c* link,daAlink_c::daAlink_ANM a,daAlink_c::daAlink_ANM b){
    f32 blend=0.5f,speedA=0.75f,speedB=1.5f;
    void* args[]={&link,&blend,&speedA,&speedB,&a,&b};
    before_set_double_anime_sprint(nullptr,args,nullptr,nullptr);
    return {speedA,speedB};
}
void begin_sprint(daAlink_c* link){void* args[]={&link};before_proc_move_sprint(nullptr,args,nullptr,nullptr);}
int main(){
    // Existing configs retain 150%; invalid file values are bounded too.
    hasConfig=false;configPercent=999;close(sprint_speed_multiplier(),1.5f);
    hasConfig=true;
    for(int percent:{-100,100,150,200,300,999}){
        configPercent=percent;
        const float factor=std::clamp(percent,100,300)/100.0f;
        close(sprint_speed_multiplier(),factor);
        daAlink_c link;
        // Native movement consumes the boosted cap, then restores its base
        // before constructing animation. Repeated ticks must not compound.
        for(int tick=0;tick<300;++tick){
            begin_sprint(&link);close(link.mMaxSpeed,23*factor);
            link.mNormalSpeed=std::min(link.mNormalSpeed+1.9f,link.mMaxSpeed);
            link.mMaxSpeed=23;
            const float cadence=1.0f+0.5f*std::max(0.0f,link.mNormalSpeed/23-1.0f);
            auto rates=animate(&link,daAlink_c::ANM_RUN,daAlink_c::ANM_RUN);
            close(rates[0],0.75f*cadence);close(rates[1],1.5f*cadence);
            rates=animate(&link,daAlink_c::ANM_WALK,daAlink_c::ANM_RUN_B);
            close(rates[0],0.75f);close(rates[1],1.5f*cadence);
            rates=animate(&link,daAlink_c::ANM_RUN,daAlink_c::ANM_ATTACK);
            close(rates[0],0.75f*cadence);close(rates[1],1.5f);
            daAlink_c::daAlink_ANM id=daAlink_c::ANM_RUN;auto* owner=&link;
            void* args[]={&owner,&id};before_get_main_bck_data_sprint(nullptr,args,nullptr,nullptr);
            assert(id==daAlink_c::ANM_RUN_B);
            after_proc_move_sprint(nullptr,nullptr,nullptr,nullptr);
            assert(!s_sprintOwner);close(animate(&link,daAlink_c::ANM_RUN,daAlink_c::ANM_RUN)[1],1.5f);
        }
    }
    // Explicit playback targets: only half the movement bonus affects cadence.
    for(const auto sample : {std::array{1.0f,1.0f}, std::array{1.5f,1.25f},
                             std::array{2.0f,1.5f}, std::array{3.0f,2.0f}}){
        daAlink_c runner;configPercent=300;begin_sprint(&runner);
        runner.mMaxSpeed=23;runner.mNormalSpeed=23*sample[0];
        const auto rates=animate(&runner,daAlink_c::ANM_RUN,daAlink_c::ANM_RUN);
        close(rates[0],0.75f*sample[1]);close(rates[1],1.5f*sample[1]);
        after_proc_move_sprint(nullptr,nullptr,nullptr,nullptr);
    }
    configPercent=300;
    // Indoor/terrain speed is used instead of the configured 3x ceiling.
    daAlink_c link;begin_sprint(&link);link.mMaxSpeed=23;link.mNormalSpeed=34.5f;
    close(animate(&link,daAlink_c::ANM_RUN,daAlink_c::ANM_RUN)[1],1.875f);
    link.groundProjection=0.8f;
    close(animate(&link,daAlink_c::ANM_RUN,daAlink_c::ANM_RUN)[1],1.65f);
    link.mNormalSpeed=0;close(animate(&link,daAlink_c::ANM_RUN,daAlink_c::ANM_RUN)[1],1.5f);
    daAlink_c other;close(animate(&other,daAlink_c::ANM_RUN,daAlink_c::ANM_RUN)[1],1.5f);
    transition(&link,daAlink_c::PROC_DAMAGE);assert(!s_sprintOwner);
    // Retain existing eligibility and stamina rules; no boost outside sprint.
    for(int scenario=0;scenario<12;++scenario){
        link=daAlink_c{};enabled=stamina=true;event=false;
        switch(scenario){
        case 0:enabled=false;break;case 1:stamina=false;break;
        case 2:link.rollHeld=false;break;case 3:link.rollTrigger=true;break;
        case 4:link.input=false;break;case 5:link.mLinkAcch.ground=false;break;
        case 6:link.wolf=true;break;case 7:link.cutscene=true;break;
        case 8:event=true;break;case 9:link.boots=true;break;
        case 10:link.sumo=true;break;case 11:link.mGrabItemAcKeep.actor=&other;break;
        }
        const int marks=staminaMarks;
        begin_sprint(&link);assert(!s_sprintOwner);close(link.mMaxSpeed,23);
        close(sprint_jump_speed_multiplier(&link),1);assert(staminaMarks==marks);
    }
    enabled=stamina=true;event=false;
    begin_sprint(nullptr);assert(!s_sprintOwner);
    close(sprint_jump_speed_multiplier(nullptr),1);

    // Capture the earned bonus BEFORE native init resets horizontal speed.
    // The real manual-jump entry applies it once and leaves height/gravity alone.
    for(int percent:{100,150,200,300}) for(f32 runRatio:{0.0f,0.5f,1.0f,1.2f,1.5f,2.0f,3.0f}){
        configPercent=percent;link=daAlink_c{};
        link.mNormalSpeed=23*runRatio;
        float factor=std::clamp(runRatio,1.0f,percent/100.0f);
        assert(start_ground_jump(&link));assert(s_manualJumpOwner==&link);
        assert(!link.mLinkAcch.ground&&link.jumpMode);
        close(link.mNormalSpeed,20*factor);close(link.speedF,26*factor);close(link.mMaxSpeed,26*factor);
        close(link.speed.y,25);close(link.gravity,-3.4f);
        // Same airtime, proportionally longer flight at constant forward input.
        float height=0,vertical=link.speed.y,distance=0,normalDistance=0;
        int ticks=0;
        do {height+=vertical;vertical+=link.gravity;distance+=link.mNormalSpeed;normalDistance+=20;++ticks;}
        while(height>0&&ticks<100);
        close(distance,normalDistance*factor);assert(ticks<100);
        transition(&link,daAlink_c::PROC_WAIT);assert(!s_manualJumpOwner);
    }
    configPercent=300;
    for(int scenario=0;scenario<6;++scenario){
        link=daAlink_c{};link.mNormalSpeed=69;enabled=stamina=true;
        switch(scenario){
        case 0:enabled=false;break;case 1:stamina=false;break;
        case 2:link.rollHeld=false;break;case 3:link.mProcID=daAlink_c::PROC_WAIT;break;
        case 4:link.mpHIO=nullptr;break;case 5:link.input=false;break;
        }
        assert(start_ground_jump(&link));close(link.mNormalSpeed,scenario==5?0:20);
        close(link.speed.y,25);
        transition(&link,daAlink_c::PROC_WAIT);
    }
    enabled=stamina=true;link=daAlink_c{};link.mNormalSpeed=69;link.initSucceeds=false;
    assert(!start_ground_jump(&link));close(link.mNormalSpeed,69);close(link.mMaxSpeed,23);
    // Ordinary auto-jumps and the existing ZR+B jump attack are not rewritten.
    link=daAlink_c{};link.mNormalSpeed=69;assert(link.procAutoJumpInit(0));close(link.mNormalSpeed,20);
    link=daAlink_c{};link.mNormalSpeed=69;link.mEquipItem=kSwordItem;bPressed=true;
    assert(start_ground_jump(&link));assert(link.mProcID==daAlink_c::PROC_CUT_JUMP);
    close(link.mNormalSpeed,25);assert(!s_manualJumpOwner);
}
'''
names = ['bool sprint_requested', 'float sprint_jump_speed_multiplier',
         'void apply_sprint_jump_speed', 'void set_manual_jump_direction',
         'void apply_manual_jump_movement', 'void apply_manual_jump_height', 'bool start_ground_jump',
         'HookAction before_proc_move_sprint', 'void after_proc_move_sprint',
         'bool sprint_animation_active', 'HookAction before_set_double_anime_sprint',
         'HookAction before_get_main_bck_data_sprint', 'HookAction before_common_proc_init']
functions = function('float sprint_speed_multiplier', config)+'\n'+'\n'.join(function(n) for n in names)
fixture = fixture.replace('// FUNCTIONS', functions)
assert 'register_int("sprint-speed-percent", 150, s_sprintSpeedPercent)' in config
assert 'hook_add_pre<SetDoubleAnimeSprint>' in source
assert 'hook_add_post<ProcMoveSprint>' in source
assert '"Sprint Speed", sprint_speed_config_var(), 100, 300, 5, "%"' in (root/'src/ui.cpp').read_text()
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp'
    exe=Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Sprint regression passed: settings, speed/cadence, eligibility, and manual-jump momentum')
