"""Title touch presentation/input cleanup and restoration, including extras disabled."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/survivors_title_touch.inc').read_text()
fixture=r'''
#include <any>
#include <array>
#include <bitset>
#include <cassert>
#include <string>
#include <iostream>
using ModResult=int;constexpr int MOD_OK=0;
struct ModError{};struct ModContext{};
using UiStyleHandle=unsigned;constexpr int UI_SCOPE_TOUCH_CONTROLS=4;
struct ModHook{};struct ExtraTouchUpdate:ModHook{};struct ExtraTouchSync:ModHook{};struct ExtraTouchDelete:ModHook{};
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace Rml {using String=std::string;struct Element{bool title=false;bool hidden=false;};}
namespace dusk::ui {enum class Control {A,B,X,Y,Z,L,R,Items,Map,COUNT};}
struct NativeTouch {
 Rml::Element root;Rml::Element* mRoot=&root;
 struct ControlElement {Rml::Element* root=nullptr;};
 std::array<ControlElement,size_t(dusk::ui::Control::COUNT)> mControlElements{};
 std::array<bool,size_t(dusk::ui::Control::COUNT)> pressed{};
 Rml::Element actionBar;Rml::Element* mActionBar=&actionBar;
 std::bitset<8> mQueuedActions;bool motion=true;
};
bool title=false;bool survivors_title_presentation_active(){return title;}
int releases=0,clears=0,registered=0,unregistered=0,hooks=0;
void setClass(Rml::Element* e,const Rml::String* name,bool value){assert(*name=="dawnlight-survivors-title");e->title=value;}
void setPseudoClass(Rml::Element* e,const Rml::String* name,bool value){assert(*name=="hidden");e->hidden=value;}
void release(NativeTouch* c,dusk::ui::Control control){assert(control!=dusk::ui::Control::A&&control!=dusk::ui::Control::B);c->pressed[size_t(control)]=false;++releases;}
void clearMotion(NativeTouch* c){c->motion=false;++clears;}
ModContext* mod_ctx=nullptr;void* svc_hook=nullptr;
struct Ui {
 ModResult register_styles(ModContext*,int scope,const char* css,UiStyleHandle* handle){assert(scope==UI_SCOPE_TOUCH_CONTROLS);assert(std::string(css).find(":not(#button-a):not(#button-b)")!=std::string::npos);*handle=7;++registered;return MOD_OK;}
 void unregister_styles(ModContext*,UiStyleHandle h){assert(h==7);++unregistered;}
}ui;auto* svc_ui=&ui;
bool resolve=true;
template<class F>bool resolve_extra(const char*,F&){return resolve;}
namespace mods {
 template<class T>T arg(void* a,int i){return std::any_cast<T>(static_cast<std::any*>(a)[i]);}
 template<class Hook,class Fn>ModResult hook_add_pre(void*,Fn){++hooks;return MOD_OK;}
 template<class Hook,class Fn>ModResult hook_add_post(void*,Fn){++hooks;return MOD_OK;}
 ModResult set_error(ModError*,ModResult r,const char*){return r;}
}
'''
main=r'''
int main(){
 NativeTouch controls,other;std::any args[]{&controls},otherArgs[]{&other};
 controls.pressed.fill(true);
 std::array<Rml::Element,size_t(dusk::ui::Control::COUNT)> elements{};
 for(size_t i=0;i<elements.size();++i){controls.mControlElements[i].root=&elements[i];}
 controls.mQueuedActions.set();
 resolve=false;assert(initialize_title_touch(nullptr)==MOD_OK&&!registered&&!hooks);
 resolve=true;assert(initialize_title_touch(nullptr)==MOD_OK&&registered==1&&hooks==5);
 s_titleTouchApi={setClass,setPseudoClass,release,clearMotion};
 before_title_touch(nullptr,args,nullptr,nullptr);assert(!controls.root.title&&!releases&&controls.motion);
 title=true;before_title_touch(nullptr,args,nullptr,nullptr);
 assert(controls.root.title&&releases==int(controls.mControlElements.size())-2&&!controls.motion&&controls.mQueuedActions.none());
 assert(controls.pressed[0]&&controls.pressed[1]); // A/B remain pressed and usable.
 for(size_t i=2;i<controls.mControlElements.size();++i)assert(!controls.pressed[i]);
 // Native sync can reveal controls every frame; post-sync restores the mask.
 after_title_touch(nullptr,args,nullptr,nullptr);
 assert(!elements[0].hidden&&!elements[1].hidden&&controls.actionBar.hidden);
 for(size_t i=2;i<elements.size();++i)assert(elements[i].hidden);
 elements[2].hidden=false;after_title_touch(nullptr,args,nullptr,nullptr);assert(elements[2].hidden);
 const int prior=releases;title=false;controls.motion=true;
 for(auto& e:elements){e.hidden=false;}controls.actionBar.hidden=false;
 after_title_touch(nullptr,args,nullptr,nullptr);
 for(auto& e:elements){assert(!e.hidden);}assert(!controls.actionBar.hidden);
 before_title_touch(nullptr,args,nullptr,nullptr);assert(!controls.root.title&&controls.motion&&releases==prior);
 title=true;before_title_touch(nullptr,args,nullptr,nullptr);
 before_title_touch_delete(nullptr,otherArgs,nullptr,nullptr);assert(s_titleTouchOwner==&controls);
 shutdown_title_touch();assert(!controls.root.title&&!s_titleTouchStyle&&!s_titleTouchOwner&&unregistered==1);
 s_titleTouchStyle=7;before_title_touch(nullptr,args,nullptr,nullptr);
 before_title_touch_delete(nullptr,args,nullptr,nullptr);assert(!s_titleTouchOwner);
 shutdown_title_touch();assert(unregistered==2);
 std::cout<<"Survivors title touch: A/B preserved, hidden holds/actions released, scope/teardown and optional extras independence passed\n";
}
'''
native=(root/'src/touch_buttons_native.inc').read_text()
assert native.index('initialize_title_touch(error)')<native.index('if (!s_extraSessionEnabled) return MOD_OK;')
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'touch.cpp';exe=Path(tmp)/'touch'
 cpp.write_text(fixture+source+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
