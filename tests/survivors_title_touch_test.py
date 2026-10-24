"""Run native title UI rules while proving game event state never changes."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/survivors_title_touch.inc').read_text()
host=(root/'dusklight/src/dusk/ui/touch_controls.cpp').read_text()

def function(signature):
    start=host.index(signature)
    brace=host.index('{',start)
    depth=1
    end=brace+1
    while depth:
        depth+=(host[end]=='{')-(host[end]=='}')
        end+=1
    return host[start:end].replace('TouchControls::','NativeTouch::')

fixture=r'''
#include <any>
#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
using u8=uint8_t;using dEvt_SkipCb=int(*)(void*,int);
struct dEvt_control_c {u8 mEventStatus=0;dEvt_SkipCb mSkipFunc=nullptr;unsigned flags=0;bool chkFlag2(unsigned f){return flags&f;}};
dEvt_control_c event;bool haveEvent=true,title=false;
auto*dComIfGp_getEvent(){return haveEvent?&event:nullptr;}
bool dComIfGp_event_runCheck(){return event.mEventStatus!=0;}
bool survivors_title_presentation_active(){return title;}
int dummySkip(void*,int){return 1;}
using ModResult=int;constexpr int MOD_OK=0;struct ModError{};struct ModContext{};
struct ExtraTouchUpdate{};struct ExtraTouchSync{};
#define DEFINE_HOOK_SYMBOL(symbol,signature,name) struct name {};
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
struct HookOptions{int priority=0;};
#define HOOK_OPTIONS_INIT {}
constexpr unsigned PAD_BUTTON_A=1,PAD_BUTTON_B=2;
namespace dusk::ui {enum class Control{A,B,X,Y,Z,L,R,FIRST_PERSON,ITEMS,COLLECTIONS,MAP,SKIP,COUNT};enum class ControlOverride{Default,Action};}
using dusk::ui::Control;using dusk::ui::ControlOverride;
std::array<ControlOverride,size_t(Control::COUNT)> sControlOverrides{};
struct Element{bool hidden=false;void SetPseudoClass(const char*,bool v){hidden=v;}void SetClass(const char*,bool){}};
struct NativeTouch {
 struct ControlTouch{bool active=false;};struct Motion{bool active=false;};
 struct Elements{Element* root=nullptr;};
 std::array<ControlTouch,size_t(Control::COUNT)> mControlTouches{};
 std::array<Element,size_t(Control::COUNT)> buttons{};
 std::array<Elements,size_t(Control::COUNT)> mControlElements{};
 Element actionBar;Element*mActionBar=&actionBar;
 std::bitset<8> mQueuedActions;Motion mMoveTouch,mCameraTouch;
 unsigned mButtonMask=0;
 bool mLPressed=false,mLLatched=false,mManualLLatched=false,mLReleasePending=false,mRTriggerHeld=false,mWasSuppressed=false;
 int mLPressStartTime=0,mLastLTapTime=0;
 NativeTouch(){for(size_t i=0;i<buttons.size();++i)mControlElements[i].root=&buttons[i];}
 void release_control(Control c){mControlTouches[size_t(c)]={};}
 void clear_motion_touch_input(){mMoveTouch={};mCameraTouch={};}
 void sync_visual_state() noexcept;void sync_action_bar_state() noexcept;
};
struct Settings{struct Game{bool enableTouchControls=true;}game;}settings;
auto&getSettings(){return settings;}
bool controls_available(bool){return true;}
void*dComIfGp_getMsgObjectClass(){return nullptr;}bool dMsgObject_isTalkNowCheck(){return false;}
void*dComIfGp_getLinkPlayer(){return &event;}
struct daPy_py_c{static int checkNowWolf(){return 0;}};
bool control_override_active(Control c){return sControlOverrides[size_t(c)]!=ControlOverride::Default;}
bool player_attention_locked(){return false;}
std::string item_icon_source_for_button(Control){return "sword";}
std::string midna_icon_source(){return "midna";}uint64_t midna_icon_revision(){return 1;}
constexpr int METER2_USEBUTTON_B=1;bool dMeter2Info_isUseButton(int){return true;}
struct FaceButtonState{std::string iconSource;uint64_t iconRevision=0;bool visible=false,showIcon=false;};
FaceButtonState override_button_state(Control){return {.iconSource="",.visible=true};}
void*svc_hook=nullptr;int hooks=0;
namespace mods {
 template<class T>T arg(void*a,int i){return std::any_cast<T>(static_cast<std::any*>(a)[i]);}
 template<class H,class F>ModResult hook_add_pre(void*,F,const HookOptions*o){assert(o->priority==INT32_MAX);++hooks;return MOD_OK;}
 template<class H,class F>ModResult hook_add_post(void*,F,const HookOptions*o){assert(o->priority==INT32_MIN);++hooks;return MOD_OK;}
 ModResult set_error(ModError*,ModResult r,const char*){return r;}
}
'''
native_rules='\n'.join(function(s) for s in [
    'FaceButtonState xy_button_state(Control control)',
    'FaceButtonState z_button_state()',
    'FaceButtonState b_button_state()',
    'void TouchControls::sync_visual_state() noexcept',
    'void TouchControls::sync_action_bar_state() noexcept'])
dispatch=function('bool game_controls_suppressed() noexcept').replace('bool game_controls_suppressed()', 'bool native_game_controls_suppressed()')+r'''
bool game_controls_suppressed() noexcept {
 bool suppressed=false;
 if(before_title_gameplay_controls(nullptr,nullptr,&suppressed,nullptr)==HOOK_CONTINUE)
  suppressed=native_game_controls_suppressed();
 return suppressed;
}
'''
main=r'''
void action_bar(NativeTouch& c){
 std::any args[]{&c};
 before_title_action_bar(nullptr,args,nullptr,nullptr);
 c.sync_action_bar_state();
 after_title_action_bar(nullptr,nullptr,nullptr,nullptr);
}
int main(){
 assert(initialize_title_touch(nullptr)==MOD_OK&&hooks==5);
 NativeTouch controls;std::any args[]{&controls};
 for(auto&t:controls.mControlTouches)t.active=true;
 controls.mButtonMask=0xffff;controls.mQueuedActions.set();
 controls.mMoveTouch.active=controls.mCameraTouch.active=true;
 controls.mLPressed=controls.mLLatched=controls.mManualLLatched=true;
 controls.mLReleasePending=controls.mRTriggerHeld=true;
 controls.sync_visual_state();action_bar(controls);
 assert(!controls.actionBar.hidden&&xy_button_state(Control::X).visible&&z_button_state().visible);
 assert(b_button_state().showIcon);
 title=true;event.mSkipFunc=dummySkip;event.flags=4;
 before_title_touch(nullptr,args,nullptr,nullptr);
 assert(event.mEventStatus==0&&event.mSkipFunc==dummySkip&&event.flags==4);
 assert(controls.mControlTouches[0].active&&controls.mControlTouches[1].active);
 for(size_t i=2;i<controls.mControlTouches.size();++i)assert(!controls.mControlTouches[i].active);
 assert(controls.mButtonMask==(PAD_BUTTON_A|PAD_BUTTON_B)&&controls.mQueuedActions.none());
 assert(!controls.mMoveTouch.active&&!controls.mCameraTouch.active&&!controls.mLLatched&&!controls.mRTriggerHeld);
 controls.sync_visual_state();
 before_title_action_bar(nullptr,args,nullptr,nullptr);
 // The game event must stay idle even INSIDE the native UI callback.
 assert(!dComIfGp_event_runCheck()&&event.mSkipFunc==dummySkip&&event.flags==4);
 assert(controls.mWasSuppressed);
 controls.sync_action_bar_state();
 assert(controls.buttons[size_t(Control::L)].hidden&&controls.buttons[size_t(Control::R)].hidden);
 assert(controls.actionBar.hidden&&controls.buttons[size_t(Control::SKIP)].hidden);
 after_title_action_bar(nullptr,nullptr,nullptr,nullptr);
 assert(!controls.mWasSuppressed);
 assert(!xy_button_state(Control::X).visible&&!xy_button_state(Control::Y).visible&&!z_button_state().visible);
 assert(b_button_state().visible&&!b_button_state().showIcon);
 assert(!dComIfGp_event_runCheck()&&event.mSkipFunc==dummySkip&&event.flags==4);
 // Nesting and a mode change restore only the widget's original flag.
 before_title_action_bar(nullptr,args,nullptr,nullptr);
 before_title_action_bar(nullptr,args,nullptr,nullptr);title=false;
 after_title_action_bar(nullptr,nullptr,nullptr,nullptr);assert(controls.mWasSuppressed);
 after_title_action_bar(nullptr,nullptr,nullptr,nullptr);assert(!controls.mWasSuppressed);
 controls.sync_visual_state();action_bar(controls);
 assert(!controls.actionBar.hidden&&!controls.buttons[size_t(Control::L)].hidden&&b_button_state().showIcon);
 // Real skippable events remain completely untouched, including in callbacks.
 event.mEventStatus=1;action_bar(controls);
 assert(!controls.buttons[size_t(Control::SKIP)].hidden);
 title=true;before_title_touch(nullptr,args,nullptr,nullptr);
 before_title_action_bar(nullptr,args,nullptr,nullptr);
 assert(event.mEventStatus==1&&event.mSkipFunc==dummySkip&&event.flags==4);
 controls.sync_action_bar_state();assert(controls.buttons[size_t(Control::SKIP)].hidden);
 shutdown_title_touch();assert(!controls.mWasSuppressed&&s_titleTouchBars.empty());
 assert(event.mEventStatus==1&&event.mSkipFunc==dummySkip&&event.flags==4);
 title=false;action_bar(controls);assert(!controls.buttons[size_t(Control::SKIP)].hidden);
 // Preserve host menu suppression and handle startup/teardown with no widget.
 title=true;controls.mWasSuppressed=true;action_bar(controls);assert(controls.mWasSuppressed);
 controls.mWasSuppressed=false;
 std::any empty[]{static_cast<NativeTouch*>(nullptr)};
 before_title_touch(nullptr,empty,nullptr,nullptr);
 before_title_action_bar(nullptr,empty,nullptr,nullptr);after_title_action_bar(nullptr,nullptr,nullptr,nullptr);
 for(int i=0;i<3;++i){action_bar(controls);assert(!controls.mWasSuppressed&&event.mEventStatus==1&&event.mSkipFunc==dummySkip);}
 assert(s_titleTouchBars.empty());
 std::cout<<"Native title UI: game events untouched inside callbacks; gameplay/Skip hidden, A/B preserved, nesting and mode changes passed\n";
}
'''
native=(root/'src/touch_buttons_native.inc').read_text()
assert native.index('initialize_title_touch(error)')<native.index('if (!s_extraSessionEnabled) return MOD_OK;')
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'touch.cpp';exe=Path(tmp)/'touch'
    cpp.write_text(fixture+source+dispatch+native_rules+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
