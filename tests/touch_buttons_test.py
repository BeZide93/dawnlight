"""Compile extra touch button state and production pad hooks against a fixture.

Run: python3 tests/touch_buttons_test.py. Rendering/multitouch device QA is separate.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
native = (root / 'src/touch_buttons_native.inc').read_text()

def function(name):
    start = native.index('HookAction ' + name + '(')
    return native[start:native.index('\n}', start) + 2]

fixture = r'''
#include "touch_button_state.hpp"
#include <cassert>
#include <tuple>
using namespace dawnlight;
using u32 = unsigned;
constexpr u32 PAD_1=0;
constexpr int PAD_ERR_NONE=0;
struct PADStatus {
 unsigned short button=0; signed char stickX=0,stickY=0,substickX=0,substickY=0;
 unsigned char triggerLeft=0,triggerRight=0,analogA=0,analogB=0; signed char err=0;
 unsigned extButton=0;
};
struct NativeTouch {};
struct ModContext {};
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
namespace mods {
 template<class T> T& arg_ref(void* args, int index) { return *static_cast<T*>(static_cast<void**>(args)[index]); }
 template<class T> T arg(void* args, int index) { return arg_ref<T>(args,index); }
}
touch::Presses s_extraPresses;
NativeTouch touchOwner;NativeTouch* s_touchOwner=&touchOwner;
bool allowed=true;
std::array<bool, touch::Count> enabled={true,true,true,true,true};
PADStatus s_extraBasePad{},s_extraMergedPad{},written{};
bool s_extraPadOwned=false;
int writes=0;
void prune_extra_presses(NativeTouch*) {
 if(!allowed)s_extraPresses.clear();
 for(size_t i=0;i<touch::Count;++i)if(!enabled[i])s_extraPresses.disable(i);
}
void write_pad(u32 port,const PADStatus* pad){assert(port==PAD_1);written=*pad;++writes;}
struct ExtraTouchSetPad{static constexpr auto g_orig=write_pad;};
// HOOKS
int main(){
 touch::Presses presses;
 PADStatus pad;
 assert(!presses.merge(pad));
 // Every button maps independently. ZL never sets digital L.
 const unsigned masks[5]={0,8,4,1,2};
 for(size_t i=0;i<touch::Count;++i){
  presses.clear();pad={};assert(presses.press(123,i));assert(presses.merge(pad));
  assert(pad.button==masks[i]);assert(pad.triggerLeft==(i==0?255:0));
  assert(presses.release(123));assert(!presses.release(123));pad={};assert(!presses.merge(pad));
 }
 assert(!presses.press(1,touch::Count));
 // Multiple fingers on one button do not release each other.
 presses.press(1,1);presses.press(2,1);presses.press(3,0);presses.press(4,4);
 presses.release(1);assert(presses.held(1));presses.release(2);assert(!presses.held(1));
 assert(presses.held(0)&&presses.held(4));
 presses.disable(0);assert(!presses.held(0)&&presses.held(4));
 presses.clear();assert(!presses.owns(4));
 // Safe area and orientation: all button bounds stay inside the viewport.
 for(float w:{240.f,800.f,1200.f})for(float h:{200.f,450.f,900.f}){
  for(auto layout:touch::Defaults){
   const auto r=touch::rect(layout,w,h,10,20);
   assert(r.x>=10&&r.y>=20&&r.x+r.size<=10+w&&r.y+r.size<=20+h);
  }
 }
 const auto r=touch::rect({500,-1,500},100,80,10,20);
 assert(r.x==30&&r.y==20&&r.size==80);
 // Preserve all vanilla touch input while adding new buttons.
 u32 port=0;PADStatus base{};base.button=0x840;base.stickX=50;base.stickY=-30;
 base.substickX=-70;base.triggerRight=180;base.analogB=140;base.extButton=0x2000;
 const PADStatus* input=&base;void* args[]={&port,&input};
 s_extraPresses.press(5,0);s_extraPresses.press(6,1);
 assert(extra_set_pad(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
 assert(input!=&base&&base.button==0x840&&base.triggerLeft==0);
 assert(input->button==0x848&&input->triggerLeft==255&&input->triggerRight==180);
 assert(input->stickX==50&&input->stickY==-30&&input->substickX==-70);
 assert(input->analogB==140&&input->extButton==0x2000);
 // Native clears when only our buttons are held: keep extras, not old native bits.
 assert(extra_clear_pad(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(writes==1&&written.button==8&&written.triggerLeft==255&&written.stickX==0);
 // Other controller ports are untouched.
 port=1;input=&base;
 assert(extra_set_pad(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE&&input==&base);
 assert(extra_clear_pad(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE&&writes==1);
 port=0;
 // Disabling a held button and opening a menu both release it without a new edge.
 enabled[1]=false;input=&base;extra_set_pad(nullptr,args,nullptr,nullptr);
 assert(input->button==base.button&&input->triggerLeft==255);
 allowed=false;input=&base;extra_set_pad(nullptr,args,nullptr,nullptr);
 assert(input==&base&&!s_extraPadOwned);
 assert(extra_clear_pad(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
 allowed=true;input=&base;extra_set_pad(nullptr,args,nullptr,nullptr);assert(input==&base);
 enabled[1]=true;s_extraPresses.press(7,1);
 extra_clear_input(nullptr,nullptr,nullptr,nullptr);
 assert(!s_extraPresses.held(1)&&extra_clear_pad(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
 // Global input loss must also invalidate the snapshot used during unload.
 s_extraPresses.press(8,0);input=&base;extra_set_pad(nullptr,args,nullptr,nullptr);
 assert(s_extraPadOwned);extra_clear_all(nullptr,nullptr,nullptr,nullptr);
 assert(!s_extraPadOwned&&!s_extraPresses.held(0)&&s_extraBasePad.button==0);
}
'''
fixture = fixture.replace('// HOOKS', '\n'.join(function(n) for n in (
    'extra_set_pad', 'extra_clear_pad', 'extra_clear_input', 'extra_clear_all')))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I'+str(root/'src'),
                    str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Touch buttons passed: independent ZL/D-pad, multitouch ownership, pad merge, menu/disable release and layout bounds')
