"""Exercise production Midna touch handlers with a small event/game fixture."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
native = (root / 'src/touch_buttons_native.inc').read_text()
buttons = (root / 'src/touch_buttons.cpp').read_text()
items = (root / 'src/hud_touch_hooks.cpp').read_text()


def function(source, signature):
    start = source.index(signature + '(')
    return source[start:source.index('\n}', start) + 2]


fixture = r'''
#include "touch_button_state.hpp"
#include <cassert>
using namespace dawnlight;
struct ModContext {};
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
namespace mods {template<class T>T arg(void* args,int i){return *static_cast<T*>(static_cast<void**>(args)[i]);}}
struct NativeTouch {};
namespace Rml {
 struct Element {Element* parent=nullptr;};
 struct Event {Element* target;int64_t finger;bool stopped=false;};
}
struct {
 Rml::Element* (*target)(Rml::Event*)=[](Rml::Event* e){return e->target;};
 Rml::Element* (*parent)(Rml::Element*)=[](Rml::Element* e){return e->parent;};
 int64_t (*finger)(Rml::Event*)=[](Rml::Event* e){return e->finger;};
 void (*stop)(Rml::Event*)=[](Rml::Event* e){e->stopped=true;};
} s_touchApi;
NativeTouch owner;NativeTouch* s_touchOwner=&owner;
Rml::Element rootElement,midnaElement{&rootElement},icon{&midnaElement},skipElement{&rootElement};
Rml::Element* s_extraRoot=&rootElement;
std::array<Rml::Element*,touch::Count> s_extraElements{};
touch::Presses s_extraPresses;
touch::ActionInput s_jumpTouchInput,s_darkLinkTouchInput;
bool s_extraHooksReady=true;
bool s_midnaTouchTriggered=false;
bool s_midnaTouchPending=false,allowed=true,ready=true;
std::array<bool,touch::Count> enabled{};
bool button_enabled(size_t i){return enabled[i];}
bool extra_controls_available(NativeTouch* p){return p&&allowed;}
bool midna_touch_button_available(){return ready;}
// INPUT
using BOOL=int;
constexpr BOOL TRUE=1,FALSE=0;
struct daAlink_c {};
bool zActive=false,touchActive=true,s_dpadLeftTrig=false,s_touchZItemHeld=false,s_touchZItemTrig=false;
bool dawnlight_touch_ui_active(){return touchActive;}
namespace dusk::ui {enum class Control {Z,SKIP,A,R};}
constexpr unsigned PAD_TRIGGER_R=0x20,PAD_TRIGGER_Z=0x10,PAD_BUTTON_A=0x100;
unsigned observedButton=0;bool observedPressed=false;
void fierce_deity_touch_button(unsigned button,bool pressed){observedButton=button;observedPressed=pressed;}
// GAME
bool talk(){
 sample_midna_touch_press(nullptr,nullptr,nullptr,nullptr);
 daAlink_c link;const auto* p=&link;void* args[]={&p};BOOL result=FALSE;
 before_midna_talk_trigger(nullptr,args,&result,nullptr);return result;
}
HookAction down(int64_t finger,Rml::Element* target=&icon){
 Rml::Event event{target,finger};auto* p=&event;void* args[]={&s_touchOwner,&p};
 const auto result=extra_touch_down(nullptr,args,nullptr,nullptr);
 assert(event.stopped==(result==HOOK_SKIP_ORIGINAL));return result;
}
void up(int64_t finger,bool cancel=false){
 Rml::Event event{&icon,finger};auto* p=&event;void* args[]={&s_touchOwner,&p};
 if(cancel)extra_touch_cancel(nullptr,args,nullptr,nullptr);
 else extra_touch_up(nullptr,args,nullptr,nullptr);
}
int main(){
 s_extraElements[touch::Midna]=&midnaElement;
 assert(down(1)==HOOK_CONTINUE&&!talk()); // off by default
 enabled[touch::Midna]=true;
 assert(down(1)==HOOK_SKIP_ORIGINAL);up(1);assert(talk()&&!talk()); // short tap survives release
 down(2);assert(talk());down(2);assert(!talk()); // held/repeated events do not retrigger
 down(3);assert(!talk());up(2);assert(s_extraPresses.held(touch::Midna));
 up(3);assert(!s_extraPresses.held(touch::Midna));down(4);assert(talk());up(4);
 down(5);up(5,true);assert(!talk()); // cancelled touch never queues a future call
 down(6);enabled[touch::Midna]=false;assert(!talk());
 enabled[touch::Midna]=true;assert(!talk()&&!s_extraPresses.held(touch::Midna));
 down(7);allowed=false;assert(!talk());allowed=true;assert(!talk());
 down(8);ready=false;assert(!talk());assert(down(9)==HOOK_CONTINUE);
 ready=true;assert(!talk()); // locked Midna, cutscene or unavailable player
 assert(down(10,&skipElement)==HOOK_CONTINUE&&!talk()); // native skip untouched
 // Native Z input is only observed for Dark Link; Midna touch is independent.
 auto control=dusk::ui::Control::Z;bool pressed=true;void* args[]={&s_touchOwner,&control,&pressed};
 assert(before_touch_set_control_pressed(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
 assert(observedButton==PAD_TRIGGER_Z&&observedPressed);
 pressed=false;before_touch_set_control_pressed(nullptr,args,nullptr,nullptr);
 assert(observedButton==PAD_TRIGGER_Z&&!observedPressed);
 control=dusk::ui::Control::R;pressed=true;
 before_touch_set_control_pressed(nullptr,args,nullptr,nullptr);
 assert(observedButton==PAD_TRIGGER_R&&observedPressed);
 control=dusk::ui::Control::A;before_touch_set_control_pressed(nullptr,args,nullptr,nullptr);
 assert(observedButton==PAD_BUTTON_A&&observedPressed);
 down(11);assert(talk());up(11);
 down(14);sample_midna_touch_press(nullptr,nullptr,nullptr,nullptr);
 sample_midna_touch_press(nullptr,nullptr,nullptr,nullptr);
 assert(!consume_midna_touch_press());up(14); // a missed sample never queues a delayed call
 down(15);up(99,true);assert(talk());up(15); // unrelated cancelled finger does not cancel Midna
 down(16);s_extraPresses.press(17,0);up(17,true);assert(talk());up(16); // LB cancel keeps Midna tap
 Rml::Element jump{&rootElement},dark{&rootElement};
 s_extraElements[touch::Jump]=&jump;s_extraElements[touch::DarkLink]=&dark;
 enabled[touch::Jump]=enabled[touch::DarkLink]=true;
 down(30,&jump);sample_midna_touch_press(nullptr,nullptr,nullptr,nullptr);
 assert(touch_jump_pressed()&&touch_jump_held()&&!consume_dark_link_touch_press());
 sample_midna_touch_press(nullptr,nullptr,nullptr,nullptr);
 assert(!touch_jump_pressed()&&touch_jump_held()); // hold drives Gale/glide
 down(31,&jump);up(30);assert(touch_jump_held());up(31);assert(!touch_jump_held());
 down(32,&jump);up(32);sample_midna_touch_press(nullptr,nullptr,nullptr,nullptr);
 assert(touch_jump_pressed()&&!touch_jump_held()); // quick tap survives release
 down(33,&dark);up(33);sample_midna_touch_press(nullptr,nullptr,nullptr,nullptr);
 assert(consume_dark_link_touch_press()&&!consume_dark_link_touch_press());
 down(34,&jump);down(35,&dark);up(34,true);sample_midna_touch_press(nullptr,nullptr,nullptr,nullptr);
 assert(!touch_jump_pressed()&&consume_dark_link_touch_press());up(35); // cancel only owning action
 down(36,&dark);up(36,true);sample_midna_touch_press(nullptr,nullptr,nullptr,nullptr);
 assert(!consume_dark_link_touch_press());
 down(37,&jump);down(38,&dark);allowed=false;sample_midna_touch_press(nullptr,nullptr,nullptr,nullptr);
 allowed=true;assert(!touch_jump_pressed()&&!touch_jump_held()&&!consume_dark_link_touch_press());
 down(39,&jump);enabled[touch::Jump]=false;assert(!touch_jump_held()&&!touch_jump_pressed());
 down(40,&dark);sample_midna_touch_press(nullptr,nullptr,nullptr,nullptr);
 sample_midna_touch_press(nullptr,nullptr,nullptr,nullptr);assert(!consume_dark_link_touch_press());up(40);
 // Midna has no GameCube button bit (especially no START, Z or D-pad).
 struct Pad {unsigned short button=0x840;unsigned extButton=0;};Pad pad;
 down(13);assert(s_extraPresses.merge(pad)&&pad.button==0x840);up(13);assert(talk());
}
'''
fixture = fixture.replace('// INPUT', '\n'.join(function(native, sig) for sig in (
    'void prune_extra_presses', 'HookAction extra_touch_down',
    'HookAction extra_touch_up', 'HookAction extra_touch_cancel', 'void sample_midna_touch_press')) + '\n' +
    '\n'.join(function(buttons, sig) for sig in ('bool consume_midna_touch_press','bool touch_jump_pressed','bool touch_jump_held','bool consume_dark_link_touch_press')))
fixture = fixture.replace('// GAME', '\n'.join(function(items, sig) for sig in (
    'HookAction before_midna_talk_trigger', 'HookAction before_touch_set_control_pressed')))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-D__ANDROID__', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(root / 'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Midna touch passed: tap/hold/multitouch/cancel, availability, toggle, native Skip, Z and D-pad coexistence')

