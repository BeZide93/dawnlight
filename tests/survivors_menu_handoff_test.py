"""Exercise the menu handoff against native command mapping and stack semantics."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
core=(root/'src/survivors.cpp').read_text()
def extract(text,name):
 m=re.search(r'^(?:void|bool|NavCommand) '+re.escape(name)+r'\([^;{}]*\)\s*(?:noexcept\s*)?\{',text,re.M)
 assert m,name
 start=text.index('{',m.start());depth=0
 for i in range(start,len(text)):
  depth+=(text[i]=='{')-(text[i]=='}')
  if not depth:return text[m.start():i+1]
fixture=r'''
#define DUSK_MOD_FEATURE_GAME 1
#include "mods/svc/hook.hpp"
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include "survivors_rules.hpp"
#include "mods/svc/ui.h"
#include "dusk/ui/nav_types.hpp"
#define svc_ui test_ui
#define svc_hook test_hook
#define svc_log test_log
void close_score_windows(){} void close_hud_editor(){}
void close_debug_notice(){}
using namespace dawnlight::survivors;
using dusk::ui::NavCommand;
ModContext* mod_ctx=nullptr;
namespace Rml {
 namespace Input{enum KeyIdentifier{KI_UNKNOWN,KI_UP,KI_DOWN,KI_LEFT,KI_RIGHT,KI_ESCAPE,KI_RETURN,KI_NUMPADENTER,KI_F1,KI_NEXT,KI_PRIOR};}
 struct Event{Input::KeyIdentifier key=Input::KI_F1;int shift=0;
  template<class T>T GetParameter(const char* n,T)const{return T(std::string(n)=="shift_key"?shift:int(key));}};
}
struct Document{
 bool shown=false,pending=false,wasShown=false;UiWindowHandle handle=0;UiWindowClosedFn closed=nullptr;
 bool visible(){return shown;}void toggle(){shown=!shown;}
 bool handle_nav_command(Rml::Event&,NavCommand);
};
bool game_obscured_below(Document&){return false;}
constexpr int kSoundMenuClose=1,kSoundMenuOpen=2;void mDoAud_seStartMenu(int){}
'''
native_map=extract((root/'dusklight/src/dusk/ui/ui.cpp').read_text(),'map_nav_event')
native_command=extract((root/'dusklight/src/dusk/ui/document.cpp').read_text(),'Document::handle_nav_command')
boundaries=r'''
std::vector<std::unique_ptr<Document>> documents;
Document* top(){for(auto i=documents.rbegin();i!=documents.rend();++i)if(!(*i)->pending)return i->get();return nullptr;}
void* host_top(){return top();}
unsigned navigations=0;
bool host_navigate(void* d,void* e,NavCommand c){++navigations;return static_cast<Document*>(d)->handle_nav_command(*static_cast<Rml::Event*>(e),c);}
bool ui_visible(){for(auto& d:documents)if(d->shown&&!d->pending)return true;return false;}
unsigned nextHandle=1;
Document* push(){if(auto* old=top()){old->wasShown=old->shown;old->shown=false;}
 documents.push_back(std::make_unique<Document>());auto* d=documents.back().get();d->shown=true;d->handle=nextHandle++;return d;}
void pop(unsigned handle){for(auto& d:documents)if(d->handle==handle){d->shown=false;d->pending=true;break;}
 if(auto* d=top())d->shown=d->wasShown;}
struct UI{
 ModResult window_push(ModContext*,const UiWindowDesc* desc,UiWindowHandle* out){auto* d=push();d->closed=desc->on_closed;*out=d->handle;return MOD_OK;}
 ModResult window_close(ModContext*,UiWindowHandle h){pop(h);return MOD_OK;}
 ModResult dialog_close(ModContext*,UiDialogHandle h){pop(h);return MOD_OK;}
}ui;auto* test_ui=&ui;
int windowTarget=0,modalTarget=0;
std::string missingSymbol;bool nullAddress=false;
unsigned installs=0,adds=0,warnings=0,failInstall=0,failAdd=0;
HookPreFn windowCallback=nullptr,modalCallback=nullptr;
ModResult resolve_symbol(ModContext*,const char* name,void** out,HookSymbolFlags*){
 std::string n=name;*out=nullptr;
 if(n==missingSymbol)return nullAddress?MOD_OK:MOD_UNAVAILABLE;
 if(n=="dusk::ui::top_document")*out=reinterpret_cast<void*>(&host_top);
 else if(n=="dusk::ui::Document::handle_nav_command")*out=reinterpret_cast<void*>(&host_navigate);
 // In particular, Document::handle_nav_event is absent, as in the user's log.
 return *out?MOD_OK:MOD_UNAVAILABLE;
}
ModResult install_hook(ModContext*,void*,void*,void** original){
 if(++installs==failInstall)return MOD_UNAVAILABLE;
 *original=reinterpret_cast<void*>(&host_navigate);return MOD_OK;
}
ModResult add_callback(ModContext*,void* target,HookPreFn callback,const HookOptions*){
 if(++adds==failAdd)return MOD_UNAVAILABLE;
 (target==&windowTarget?windowCallback:modalCallback)=callback;return MOD_OK;
}
HookService hook{};auto* test_hook=&hook;
struct Log{void warn(ModContext*,const char*){++warnings;}}logService;auto* test_log=&logService;
enum class HubMenu{None,Talents,Run};unsigned s_hubClosingWindow=0;HubMenu s_hubMenu=HubMenu::Run;
bool s_hubDeparture=false;void hub_menu_closed(){s_hubMenu=HubMenu::None;}void load_option_config(){}
Run s_run;RunOptions s_options;unsigned s_map=3;Records s_records;
bool arena=true,transition=false;bool in_arena(){return arena;}bool transitioning(){return transition;}
void* s_menuDocument=nullptr;void* s_talentDocument=nullptr;
UiWindowHandle s_window=0,s_refreshChoices=0;UiDialogHandle s_talentDialog=0;
ChestReward s_chosenTreasure=ChestReward::Count;
void dismiss_treasure_choice(){assert(false);}
ModResult build_window(ModContext*,UiWindowHandle,UiElementHandle,UiElementHandle,void*,ModError*){return MOD_OK;}
auto build_difficulty=build_window,build_talents=build_window,build_run_settings=build_window,build_debug=build_window;
const char* kResultsStyle="results";
ModResult update_arena_tab(ModContext*,void*,ModError*){return MOD_OK;}
ModResult update_talent_tab(ModContext*,void*,ModError*){return MOD_OK;}
ModResult update_debug_tab(ModContext*,void*,ModError*){return MOD_OK;}
constexpr char kTreasureStyle[]="",kArenaStyle[]="";
'''
main=r'''
bool request(Document* d,Rml::Event& event){bool handled=false;void* document=d;void* e=&event;
 auto command=map_nav_event(event);void* args[]={&document,&e,&command};
 const auto callback=d==s_talentDocument?modalCallback:windowCallback;
 if(!callback)return false;
 const auto action=callback(nullptr,args,&handled,nullptr);return action==HOOK_SKIP_ORIGINAL&&handled;}
void reset_hooks(){
 missingSymbol.clear();nullAddress=false;installs=adds=failInstall=failAdd=0;
 windowCallback=modalCallback=nullptr;
 SurvivorsMenuWindow::target=SurvivorsMenuModal::target=nullptr;
 mod_meta_hook_SurvivorsMenuWindow.resolved=&windowTarget;
 mod_meta_hook_SurvivorsMenuModal.resolved=&modalTarget;
}
int main(){
 hook.resolve=resolve_symbol;hook.install=install_hook;hook.add_pre=add_callback;
 // Missing optional host APIs/metadata and install/add failures must never
 // propagate a load error, capture a menu document, or close an existing UI.
 for(unsigned failure=0;failure<10;++failure){
  reset_hooks();const auto oldWarnings=warnings;
  if(failure<4){missingSymbol=failure<2?"dusk::ui::top_document":"dusk::ui::Document::handle_nav_command";nullAddress=failure%2;}
  if(failure==4)mod_meta_hook_SurvivorsMenuWindow.resolved=nullptr;
  if(failure==5)mod_meta_hook_SurvivorsMenuModal.resolved=nullptr;
  if(failure==6)failInstall=1;
  if(failure==7)failInstall=2;
  if(failure==8)failAdd=1;
  if(failure==9)failAdd=2;
  initialize_menu_handoff();assert(!s_menuHost.enabled&&warnings==oldWarnings+1);
  if(failure<6)assert(!installs&&!adds);
  remember_menu_document(false);remember_menu_document(true);
  assert(!s_menuDocument&&!s_talentDocument);
  s_run.phase=Phase::Ready;s_hubMenu=HubMenu::Run;open_window();assert(s_window);
  Rml::Event event;assert(!request(top(),event)&&s_window);
  close_window();documents.clear();
 }
 reset_hooks();initialize_menu_handoff();assert(s_menuHost.enabled&&installs==2&&adds==2);
 auto* host=push();host->shown=false;
 s_run.phase=Phase::Ready;s_hubMenu=HubMenu::Run;s_options.enemy_hp=7;s_options.camera_distance=350;s_records.talents.ranks[0]=3;
 Rml::Event event;open_window();assert(s_window&&s_menuDocument==top());
 // Menu is the native F1 command: controller bindings and three-finger taps
 // synthesize the same key. Shift-F1 and ordinary navigation remain untouched.
 auto* main=top();event.shift=1;assert(!request(main,event)&&main->shown);event.shift=0;
 event.key=Rml::Input::KI_DOWN;assert(!request(main,event));event.key=Rml::Input::KI_F1;
 for(auto phase:{Phase::Running,Phase::Upgrade,Phase::TreasureChoice,Phase::Victory}){
  s_run.phase=phase;assert(!request(main,event)&&s_window);
 }s_run.phase=Phase::Ready;
 arena=false;assert(!request(main,event));arena=true;transition=true;assert(!request(main,event));transition=false;
 auto* foreign=push();assert(!request(foreign,event)&&s_window);assert(!request(main,event));pop(foreign->handle);
 const auto oldHandle=s_window;auto oldClosed=main->closed;
 assert(request(main,event)&&!s_window&&host->shown&&navigations==1&&main->pending);
 open_window();assert(!s_window); // Host menu has priority, not just one update.
 auto* settings=push();open_window();assert(!s_window);
 pop(settings->handle);open_window();assert(!s_window&&host->shown);
 host->toggle();open_window();assert(s_window&&s_window!=oldHandle);
 oldClosed(nullptr,oldHandle,nullptr);assert(s_window); // Delayed fade/destruction cannot clear the new window.
 assert(s_map==3&&s_options.enemy_hp==7&&s_options.camera_distance==350&&s_records.talents.ranks[0]==3);
 // A description popup must yield in the same press, closing both our windows.
 auto* dialog=push();s_talentDialog=dialog->handle;remember_menu_document(true);
 assert(request(dialog,event)&&!s_talentDialog&&!s_window&&host->shown);
 assert(!s_menuDocument&&!s_talentDocument);
 host->toggle();open_window();assert(s_window);
 // If pop already uncovered a visible host, never toggle it shut again.
 host->wasShown=true;const auto calls=navigations;assert(request(top(),event));
 assert(host->shown&&navigations==calls&&!s_window);
 for(unsigned i=0;i<20;++i){host->toggle();open_window();assert(s_window);assert(request(top(),event)&&host->shown&&!s_window);}
 // Mode exit does not reopen anything; ordinary host navigation is untouched.
 arena=false;assert(!request(host,event));close_window();assert(!s_menuDocument&&!s_talentDocument);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 declarations='\n'.join(re.findall(r'^DEFINE_HOOK_SYMBOL\("dusk::ui::(?:Window|Modal)::handle_nav_command"[^;]*;',core,re.M))+'\n'
 cpp.write_text(fixture+native_map+native_command+boundaries+declarations+extract(core,'close_window')+(root/'src/survivors_menu_handoff.inc').read_text()+extract(core,'open_window')+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-Wno-unused-parameter','-I',str(root/'src'),'-I',str(root/'dusklight/sdk/include'),'-I',str(root/'dusklight/src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
 # iOS deliberately emits no unsupported private metadata and never resolves
 # or installs these optional hooks; the ordinary window still opens/closes.
 ios_main=r'''
 int main(){
  hook.resolve=resolve_symbol;hook.install=install_hook;hook.add_pre=add_callback;
  initialize_menu_handoff();assert(!s_menuHost.enabled&&warnings==1&&!installs&&!adds);
  remember_menu_document(false);remember_menu_document(true);assert(!s_menuDocument&&!s_talentDocument);
  s_run.phase=Phase::Ready;s_hubMenu=HubMenu::Run;open_window();assert(s_window);
  close_window();assert(!s_window);
 }
 '''
 cpp.write_text('#define DAWNLIGHT_IOS 1\n'+fixture+native_map+native_command+boundaries+extract(core,'close_window')+(root/'src/survivors_menu_handoff.inc').read_text()+extract(core,'open_window')+ios_main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-Wno-unused-parameter','-I',str(root/'src'),'-I',str(root/'dusklight/sdk/include'),'-I',str(root/'dusklight/src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors menu: absent event symbol, missing APIs/targets, install failures, native Menu/Shift-F1 mapping, host subwindows, talent popup, restoration, state and delayed callbacks passed')
