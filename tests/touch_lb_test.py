"""Test native LB query hooks, including Twilight HD HUD's SDL/fallback paths."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/touch_buttons_native.inc').read_text()
def function(signature):
    start=source.index(signature+'(')
    return source[start:source.index('\n}',start)+2]
fixture=r'''
#include "touch_button_state.hpp"
#include <cassert>
using u32=unsigned;using s32=int;
constexpr u32 PAD_1=0;
enum SDL_GamepadButton {SDL_GAMEPAD_BUTTON_A=0,SDL_GAMEPAD_BUTTON_LEFT_SHOULDER=9,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER=10};
struct SDL_Gamepad {bool buttons[11]{};};
struct ModContext {};struct NativeTouch {};
namespace mods {template<class T>T arg(void* args,int i){return *static_cast<T*>(static_cast<void**>(args)[i]);}}
dawnlight::touch::Presses s_extraPresses;
NativeTouch owner;NativeTouch* s_touchOwner=&owner;
bool s_extraHooksReady=true,allowed=true,enabled=true,connected=false;
SDL_Gamepad primary,secondary;
int PADGetIndexForPort(u32 p){return connected&&p==0?4:-1;}
SDL_Gamepad* PADGetSDLGamepadForIndex(u32 i){return connected&&i==4?&primary:nullptr;}
void prune_extra_presses(NativeTouch*){if(!allowed||!enabled)s_extraPresses.clear();}
// PRODUCTION
bool sdl_read(SDL_Gamepad* gamepad,SDL_GamepadButton button){
 bool result=gamepad&&gamepad->buttons[button];void* args[]={&gamepad,&button};
 extra_sdl_button(nullptr,args,&result,nullptr);return result;
}
s32 native_read(u32 port){
 s32 result=-1;
 if(connected&&port==0)for(int i=0;i<11;++i)if(sdl_read(&primary,static_cast<SDL_GamepadButton>(i))){result=i;break;}
 void* args[]={&port};extra_native_button(nullptr,args,&result,nullptr);return result;
}
// TPHD reads SDL on its primary gamepad, or the native query when none exists.
bool mod_lb_read(){return connected?sdl_read(&primary,SDL_GAMEPAD_BUTTON_LEFT_SHOULDER):native_read(0)==9;}
int main(){
 assert(!mod_lb_read());
 s_extraPresses.press(1,0);assert(mod_lb_read()&&mod_lb_read());
 s_extraPresses.release(1);assert(!mod_lb_read());
 s_extraPresses.press(2,0);connected=true;
 assert(mod_lb_read());assert(!sdl_read(&primary,SDL_GAMEPAD_BUTTON_A));
 assert(!sdl_read(&primary,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));
 assert(!sdl_read(&secondary,SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
 assert(!sdl_read(nullptr,SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
 assert(native_read(1)==-1);
 primary.buttons[0]=true;assert(sdl_read(&primary,SDL_GAMEPAD_BUTTON_A)&&mod_lb_read());
 assert(native_read(0)==0); // single-button API preserves another physical result
 primary.buttons[0]=false;primary.buttons[9]=true;
 s_extraPresses.release(2);assert(mod_lb_read()); // release touch must not release real LB
 primary.buttons[9]=false;assert(!mod_lb_read());
 s_extraPresses.press(3,0);enabled=false;assert(!mod_lb_read());
 enabled=true;assert(!mod_lb_read()); // fresh touch required after disabling
 s_extraPresses.press(4,0);allowed=false;assert(!mod_lb_read());
 allowed=true;assert(!mod_lb_read());
 s_extraPresses.press(5,0);s_extraHooksReady=false;assert(!mod_lb_read());
 s_extraHooksReady=true;s_extraPresses.clear();assert(!mod_lb_read());
 // D-pad fingers never become LB, with or without a controller.
 s_extraPresses.press(6,1);assert(!mod_lb_read());connected=false;assert(!mod_lb_read());
 // Touch LB continues through disconnect/reconnect using the correct query path.
 s_extraPresses.press(7,0);assert(mod_lb_read());connected=true;assert(mod_lb_read());
 connected=false;s_extraPresses.release(7);assert(!mod_lb_read());
}
'''
fixture=fixture.replace('// PRODUCTION','\n'.join(function(s) for s in (
    'bool extra_lb_held','void extra_sdl_button','void extra_native_button')))
with tempfile.TemporaryDirectory() as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I'+str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Touch LB passed: controller-free TPHD fallback, simultaneous physical input, player isolation, release/disable/menu/unload and reconnect')
