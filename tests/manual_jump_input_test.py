"""Exercise production jump bindings with fake controller and UI services."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
config = (root / 'src/config.cpp').read_text()
start = config.index('JumpButton jump_button() {')
getter = config[start:config.index('\n}', start) + 2]
hooks = (root / 'src/jump_hooks.cpp').read_text()
start = hooks.index('bool jump_state_ready(')
jump_gate = hooks[start:hooks.index('\n}', start) + 2]
fixture = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <string_view>
#include "jump_button.hpp"
using namespace dawnlight;
using u32=uint32_t;using s32=int32_t;using s16=int16_t;
constexpr int PAD_1=0,PAD_ERR_NONE=0;
constexpr u32 PAD_BUTTON_RIGHT_STICK=0x2000000,PAD_BUTTON_LEFT_STICK=0x4000000;
struct ModContext {};
ModContext* mod_ctx=nullptr;
enum ModResult{MOD_OK,MOD_ERROR};
int s_jumpButton=27,configured=0;
bool registered=true;
int get_int(int handle,int fallback,int low,int high){
    assert(handle==s_jumpButton);return registered?std::clamp(configured,low,high):fallback;
}
bool directPressed=false,directHeld=false;
bool touch_jump_pressed(){return directPressed;}
bool touch_jump_held(){return directHeld;}
// GETTER
bool leftTriggerPressed=false,leftTriggerHeld=false;
bool nativePressed=false,nativeHeld=false,player=true,paused=false,visible=false,uiFails=false;
namespace mDoCPd_c {
bool getTrigLockL(int){return leftTriggerPressed;}
bool getHoldLockL(int){return leftTriggerHeld;}
bool getTrigLockR(int){return nativePressed;}
bool getHoldLockR(int){return nativeHeld;}
}
void* daAlink_getAlinkActorClass(){return player?reinterpret_cast<void*>(1):nullptr;}
bool dComIfGp_isPauseFlag(){return paused;}
ModResult document_visible(ModContext*,bool* out){*out=visible;return uiFails?MOD_ERROR:MOD_OK;}
struct UiService{ModResult (*is_any_document_visible)(ModContext*,bool*);};
UiService ui{document_visible};UiService* svc_ui=&ui;
struct SDL_Gamepad{} gamepad;
int portIndex=0,nativeButton=-1;
bool bumper=false,resolveFails=false;
s32 PADGetIndexForPort(int){return portIndex;}
SDL_Gamepad* PADGetSDLGamepadForIndex(u32 index){assert(index==0);return &gamepad;}
s32 PADGetNativeButtonPressed(int){return nativeButton;}
bool read_button(SDL_Gamepad* pad,int button){assert(pad==&gamepad&&button==9);return bumper;}
ModResult resolve(ModContext*,const char* symbol,void** out,void*){
    assert(std::string_view(symbol)=="SDL_GetGamepadButton");
    *out=resolveFails?nullptr:reinterpret_cast<void*>(&read_button);return resolveFails?MOD_ERROR:MOD_OK;
}
struct HookService{ModResult (*resolve)(ModContext*,const char*,void**,void*);};
HookService hook{resolve};HookService* svc_hook=&hook;
struct PadStatus{int err=0;u32 extButton=0;};
struct JUTGamePad {static PadStatus mPadStatus[1];};
PadStatus JUTGamePad::mPadStatus[1]{};
// INPUT
struct daAlink_c {bool checkWolf(){return false;}} link;
bool manualJump=true,gale=true,s_galeInputCancelled=false;
bool r_jump_enabled(){return manualJump;}
bool revalis_gale_enabled(){return gale;}
bool ground_jump_context_ready(daAlink_c*){return true;}
// JUMP_GATE
void sample(){after_jump_pad_read(nullptr,nullptr,nullptr,nullptr);}
void reset(){
    JUTGamePad::mPadStatus[0]={};
    nativePressed=nativeHeld=leftTriggerPressed=leftTriggerHeld=false;
    portIndex=0;nativeButton=-1;bumper=resolveFails=false;svc_hook=&hook;
    player=true;paused=visible=uiFails=false;svc_ui=&ui;
    initialize_jump_input();
}
void down(int option,bool held){
    if(option==5)bumper=held;
    else {
        assert(option==3||option==4);
        auto mask=option==3?PAD_BUTTON_RIGHT_STICK:PAD_BUTTON_LEFT_STICK;
        if(held)JUTGamePad::mPadStatus[0].extButton|=mask;
        else JUTGamePad::mPadStatus[0].extButton&=~mask;
    }
}
int main(){
    // Exercise the real input adapter and launch gate for every physical binding
    // and the independent touch button, with Gale both on and off.
    for(bool galeEnabled:{false,true})for(bool touch:{false,true})for(int option:{0,5,3,4}){
        reset();configured=option;gale=galeEnabled;
        nativePressed=nativeHeld=false;directPressed=directHeld=false;sample();
        if(touch)directPressed=directHeld=true;
        else if(option==0)nativePressed=nativeHeld=true;
        else down(option,true);
        sample();assert(jump_pressed(active_jump_binding()));
        manualJump=false;assert(!jump_state_ready(&link));
        manualJump=true;assert(jump_state_ready(&link));
        nativePressed=nativeHeld=directPressed=directHeld=false;
    }
    reset();
    for(int option:{0,5,3,4}){
        configured=option;directPressed=directHeld=true;
        assert(jump_pressed(active_jump_binding())&&jump_held(active_jump_binding()));
        directPressed=false;assert(!jump_pressed(active_jump_binding())&&jump_held(active_jump_binding()));
        directHeld=false;assert(!jump_pressed(active_jump_binding())&&!jump_held(active_jump_binding()));
    }
    reset();registered=false;configured=4;assert(active_jump_binding()==JumpButton::R);registered=true;
    for(int option:{0,5,3,4}){configured=option;assert(static_cast<int>(active_jump_binding())==option);}
    configured=-1;assert(active_jump_binding()==JumpButton::R);
    configured=99;assert(active_jump_binding()==JumpButton::R);
    for(int removed:{1,2}){configured=removed;assert(active_jump_binding()==JumpButton::R);}
    // LT's native camera/targeting state cannot become an LB jump press.
    configured=5;leftTriggerPressed=leftTriggerHeld=true;sample();
    assert(!jump_pressed(active_jump_binding())&&!jump_held(active_jump_binding()));
    leftTriggerPressed=leftTriggerHeld=false;
    nativePressed=nativeHeld=true;
    assert(jump_pressed(JumpButton::R)&&jump_held(JumpButton::R));
    nativePressed=false;assert(!jump_pressed(JumpButton::R)&&jump_held(JumpButton::R));
    nativeHeld=false;
    for(int option:{5,3,4}){
        reset();configured=option;sample();
        assert(!jump_held(active_jump_binding())&&!jump_pressed(active_jump_binding()));
        down(option,true);sample();
        assert(jump_held(active_jump_binding())&&jump_pressed(active_jump_binding()));
        for(int other:{0,5,3,4})if(other!=option)assert(!jump_held(static_cast<JumpButton>(other)));
        sample();assert(jump_held(active_jump_binding())&&!jump_pressed(active_jump_binding()));
        down(option,false);sample();assert(!jump_held(active_jump_binding())&&!jump_pressed(active_jump_binding()));
        // Holding a physical input across menu close must not launch or charge.
        for(int block=0;block<5;++block){
            visible=block==0;paused=block==1;player=block!=2;uiFails=block==3;svc_ui=block==4?nullptr:&ui;
            down(option,true);sample();assert(!jump_held(active_jump_binding())&&!jump_pressed(active_jump_binding()));
            visible=paused=uiFails=false;player=true;svc_ui=&ui;sample();
            assert(!jump_held(active_jump_binding())&&!jump_pressed(active_jump_binding()));
            down(option,false);sample();down(option,true);sample();assert(jump_pressed(active_jump_binding()));
            down(option,false);sample();
        }
        // Disconnect clears the cached held state.
        down(option,true);sample();JUTGamePad::mPadStatus[0].err=-1;portIndex=-1;sample();
        assert(!jump_held(active_jump_binding())&&!jump_pressed(active_jump_binding()));
    }
    reset();
    // All buttons are sampled, so changing a setting to an already-held input
    // does not create an extra press edge.
    down(3,true);sample();sample();configured=3;assert(jump_held(active_jump_binding())&&!jump_pressed(active_jump_binding()));
    // Physical LB is read independently of other held buttons; missing SDL
    // resolution and controller-free touch LB use the native query fallback.
    reset();configured=5;bumper=true;nativeButton=0;sample();assert(jump_pressed(active_jump_binding()));
    for(bool disconnected:{false,true}){
        reset();configured=5;resolveFails=true;initialize_jump_input();
        if(disconnected)portIndex=-1;
        nativeButton=9;sample();assert(jump_pressed(active_jump_binding()));
        sample();assert(jump_held(active_jump_binding())&&!jump_pressed(active_jump_binding()));
        nativeButton=-1;sample();assert(!jump_held(active_jump_binding()));
    }
    // Retired trigger IDs cannot select a stick-click state.
    for(int removed:{1,2}){
        assert(!jump_pressed(static_cast<JumpButton>(removed))&&!jump_held(static_cast<JumpButton>(removed)));
    }
    assert(!jump_pressed(static_cast<JumpButton>(-1))&&!jump_held(static_cast<JumpButton>(99)));
}
'''.replace('// GETTER', getter).replace('// INPUT', (root / 'src/jump_input.inc').read_text()).replace('// JUMP_GATE', jump_gate)
assert 'register_bool("r-jump", true, s_rJump)' in config  # Preserve existing saved toggles.
assert 'register_int("jump-button", 0, s_jumpButton)' in config
ui = (root / 'src/ui.cpp').read_text()
assert ui.index('"Manual Jump",') < ui.index('"Jump Button",') < ui.index('"Disable Auto Jump",')
assert '{"R", "L (LB)", "R3", "L3"}' in ui
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root / 'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Manual Jump input: default, four bindings, edges, held/release, UI suppression, disconnect and retired trigger fallback passed')
