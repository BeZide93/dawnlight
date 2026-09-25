"""Test the native-editor adapter and viewport against pinned Dusklight helpers.

Requires the source checkout fetched by CMake (no game assets). Override its
location with DUSKLIGHT_DIR if needed. Run: python3 tests/touch_button_editor_test.py
"""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
dusk = Path(os.environ.get('DUSKLIGHT_DIR', root/'dusklight'))
if not dusk.exists() and (root.parent/'dusk-source').exists():
    dusk = root.parent/'dusk-source'
common = (dusk/'src/dusk/ui/touch_controls_common.cpp').read_text()
adapter = (root/'src/touch_button_editor.inc').read_text()
native_source = (dusk/'src/dusk/ui/touch_controls.cpp').read_text()

def function(source, signature):
    start = source.index(signature + '(')
    return source[start:source.index('\n}', start) + 2]

fixture = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <locale>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
using u8=unsigned char;
#include "dusk/ui/controls.hpp"
#include "touch_button_state.hpp"
using namespace dawnlight;
namespace Rml {
 struct Context {
  struct Dimensions { int x=2400,y=1080; };
  Dimensions GetDimensions() { return {}; }
 };
 using String=std::string;
 struct Element {
  Context* context=nullptr;bool hidden=false;
  void SetPseudoClass(const String& name,bool value){assert(name=="hidden");hidden=value;}
 };
}
namespace dusk::ui {
 float touch_dp_scale(Rml::Context*) { return 2.5f; }
 // HOST_DIMENSIONS
 struct TouchLayoutControlInfo {
  std::string_view layoutId;const char* elementId=nullptr;ControlProps props;
  Control control=Control::COUNT;bool hasControl=false;
 };
}
struct ModContext {};
ModContext* mod_ctx=nullptr;
constexpr int MOD_OK=0;
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* args,int i){return *static_cast<T*>(static_cast<void**>(args)[i]);}}
struct Config {
 std::map<int,std::string> values;
 int get_string(ModContext*,int key,char* buf,size_t len,void*) {
  const auto& value=values[key];assert(value.size()<len);std::strcpy(buf,value.c_str());return MOD_OK;
 }
 int set_string(ModContext*,int key,const char* value){values[key]=value;return MOD_OK;}
} config;
auto* svc_config=&config;
struct Log {void warn(ModContext*,const char*){assert(false);}} logService;
auto* svc_log=&logService;
struct ButtonConfig{int layout;};
std::array<ButtonConfig,touch::Count> s_buttons={{{0},{1},{2},{3},{4}}};
touch::Layout button_layout(size_t i){return touch::Defaults[i];}
struct TouchApi {
 Rml::Context* (*context)(const Rml::Element*)=[](const Rml::Element* e){return e->context;};
 dusk::ui::ControlLayoutSize (*dimensions)(Rml::Context*)=dusk::ui::touch_document_size_dp;
} s_touchApi;
// LAYOUT
struct EditorDocument {bool closed=false,mClosed=false,mPendingClose=false;};
using dusk::ui::Control;
struct NativeTouch {
 bool mWasSuppressed=false,shown=false,held=true;
 struct Elements{Rml::Element* root=nullptr;};
 std::array<Elements,static_cast<size_t>(Control::COUNT)> mControlElements{};
 Rml::Element* mActionBar=nullptr;
 void show(){shown=true;}
 void clear_motion_touch_input(){} void release_control(Control){} void clear_equip_targets(){}
 void sync_visual_state() noexcept;void sync_action_bar_state() noexcept;void sync_control_displays() noexcept;
};
struct Settings{struct Game{bool enableTouchControls=true;}game;} settings;
Settings& getSettings(){return settings;}
// HOST_SUPPRESSION
EditorDocument* topDocument=nullptr;
struct NativeEditor:EditorDocument {dusk::ui::ControlLayout mWorkingLayout;};
using LayoutSpan=std::span<const dusk::ui::TouchLayoutControlInfo>;
NativeEditor* s_extraEditor=nullptr;
unsigned s_editorScope=0;
bool s_editorConstructing=false,s_touchEditorAvailable=true;
std::array<dusk::ui::TouchLayoutControlInfo,touch::Count> s_editorControls{};
std::string s_editorFragment="extra buttons";
std::vector<EditorDocument*> s_editorChildren;
const char* kExtraEditorIds[]={"zl","up","down","left","right"};
int cancels=0,uncovers=0;
struct EditorApi {
 void (*setPseudo)(Rml::Element*,const Rml::String*,bool)=[](Rml::Element* e,const Rml::String* n,bool v){e->SetPseudoClass(*n,v);};
 EditorDocument* (*top)()=[](){return topDocument;};
 void (*showTouch)(NativeTouch*)=[](NativeTouch* t){t->show();};
 void (*clearTouch)(NativeTouch*)=[](NativeTouch* t){t->held=false;};
 void (*cancel)(NativeEditor*)=[](NativeEditor*){++cancels;};
 void (*hide)(EditorDocument*,bool)=[](EditorDocument* e,bool c){e->closed=c;};
 void (*uncover)()=[](){++uncovers;};
} s_editorApi;
// ADAPTER
int main(){
 // Reproduce the actual host helper's null-context contract.
 const auto missing=dusk::ui::touch_document_size_dp(nullptr);assert(missing.w==0&&missing.h==0);
 Rml::Context context;Rml::Element document{&context};dusk::ui::ControlLayoutSize viewport;
 assert(extra_viewport(&document,viewport)&&viewport.w==960&&viewport.h==432);
 document.context=nullptr;assert(!extra_viewport(&document,viewport)&&viewport.w==0);
 assert(!extra_viewport(nullptr,viewport));document.context=&context;assert(extra_viewport(&document,viewport));
 for(size_t i=0;i<touch::Count;++i){
  const auto props=saved_extra_props(i,viewport);
  const auto rect=dusk::ui::resolve_control_layout(props,viewport).visual;
  assert(rect.w>=44&&rect.h>=44&&rect.l>1&&rect.t>1); // never 1px at the origin
  s_editorControls[i].props=default_extra_props(i,viewport);
 }
 // Fractional drag/resize results and anchors survive restart without rounding.
 dusk::ui::ControlProps p{0.367f,0.751f,82.125f,51.75f,1.875f,dusk::ui::ControlAnchor::None},loaded;
 assert(decode_extra_props(encode_extra_props(p).c_str(),loaded)&&loaded==p);
 for(const char* bad:{"","1 0 0 -2 40 1 0","1 0 0 40 40 1 9","2 0 0 40 40 1 0","1 0 0 40 40 1 0 junk","1 nan 0 40 40 1 0"})
  assert(!decode_extra_props(bad,loaded));
 config.values[0]=encode_extra_props(p);assert(saved_extra_props(0,viewport)==p);
 // Native editor is unaffected; only our scoped instance sees five entries.
 std::array<dusk::ui::TouchLayoutControlInfo,9> vanillaControls{};
 NativeEditor ours,vanilla;s_extraEditor=&ours;NativeEditor* which=&vanilla;void* args[]={&which};
 LayoutSpan span=vanillaControls;enter_extra_editor(nullptr,args,nullptr,nullptr);
 extra_editor_controls(nullptr,nullptr,&span,nullptr);assert(span.size()==9);
 leave_extra_editor(nullptr,args,nullptr,nullptr);assert(s_editorScope==0);
 which=&ours;enter_extra_editor(nullptr,args,nullptr,nullptr);enter_extra_editor(nullptr,args,nullptr,nullptr);
 extra_editor_controls(nullptr,nullptr,&span,nullptr);assert(span.size()==5&&span.data()==s_editorControls.data());
 leave_extra_editor(nullptr,args,nullptr,nullptr);assert(s_editorScope==1);
 leave_extra_editor(nullptr,args,nullptr,nullptr);span=vanillaControls;
 extra_editor_controls(nullptr,nullptr,&span,nullptr);assert(span.size()==9);
 std::string_view fragment="vanilla";extra_editor_fragment(nullptr,nullptr,&fragment,nullptr);assert(fragment=="vanilla");
 s_editorConstructing=true;extra_editor_fragment(nullptr,nullptr,&fragment,nullptr);assert(fragment==s_editorFragment);s_editorConstructing=false;
 // Vanilla Save reaches the host; custom Save writes only Dawnlight keys.
 which=&vanilla;assert(save_extra_editor(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE&&cancels==0);
 config.values[99]="native touch layout";
 ours.mWorkingLayout.controls["zl"]=p;which=&ours;
 assert(save_extra_editor(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&cancels==1);
 assert(config.values[99]=="native touch layout"&&saved_extra_props(0,viewport)==p);
 // Original native Reset clears the working map; save now uses default props.
 ours.mWorkingLayout={};save_extra_editor(nullptr,args,nullptr,nullptr);
 assert(saved_extra_props(0,viewport)==s_editorControls[0].props);
 // Unsaved changes and native Cancel never mutate persistent data.
 const auto before=config.values;ours.mWorkingLayout.controls["zl"]=p;
 s_editorApi.cancel(&ours);assert(config.values==before);
 // The actual native document is visible only behind our editor/reset modal.
 NativeTouch touchControls;auto* touchPointer=&touchControls;void* touchArgs[]={&touchPointer};
 topDocument=&ours;
 assert(show_editor_background(nullptr,touchArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(touchControls.shown&&touchControls.mWasSuppressed&&!touchControls.held);
 // Reproduce the host's three real suppressed-state branches, frame after frame.
 std::array<Rml::Element,static_cast<size_t>(Control::COUNT)> nativeElements;
 for(size_t i=0;i<nativeElements.size();++i)touchControls.mControlElements[i].root=&nativeElements[i];
 Rml::Element actionBar;touchControls.mActionBar=&actionBar;
 for(int frame=0;frame<10;++frame){
  touchControls.sync_visual_state();touchControls.sync_action_bar_state();touchControls.sync_control_displays();
  for(auto c:{Control::L,Control::R,Control::Z,Control::A,Control::B,Control::X,Control::Y})
   assert(nativeElements[static_cast<size_t>(c)].hidden);
  assert(actionBar.hidden);
  sync_editor_background(&touchControls);
  for(auto c:{Control::L,Control::R,Control::Z,Control::A,Control::B,Control::X,Control::Y})
   assert(!nativeElements[static_cast<size_t>(c)].hidden);
  assert(!actionBar.hidden&&nativeElements[static_cast<size_t>(Control::SKIP)].hidden);
  assert(touchControls.mWasSuppressed&&!touchControls.held);
 }
 topDocument=&vanilla;touchControls.shown=false;
 assert(show_editor_background(nullptr,touchArgs,nullptr,nullptr)==HOOK_CONTINUE&&!touchControls.shown);
 touchControls.sync_visual_state();sync_editor_background(&touchControls);
 assert(nativeElements[static_cast<size_t>(Control::L)].hidden);
 EditorDocument resetModal;s_editorChildren.push_back(&resetModal);topDocument=&resetModal;
 assert(show_editor_background(nullptr,touchArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&touchControls.shown);
 ours.mPendingClose=true;
 assert(show_editor_background(nullptr,touchArgs,nullptr,nullptr)==HOOK_CONTINUE);
 ours.mPendingClose=false;ours.mClosed=true;
 assert(show_editor_background(nullptr,touchArgs,nullptr,nullptr)==HOOK_CONTINUE);
 ours.mClosed=false;s_extraEditor=nullptr;
 assert(show_editor_background(nullptr,touchArgs,nullptr,nullptr)==HOOK_CONTINUE);
 s_extraEditor=&ours;s_editorChildren.clear();
 // Mod unload closes reset modals before the editor and resumes its parent.
 EditorDocument modal;s_editorChildren.push_back(&modal);close_extra_editor();
 assert(modal.closed&&ours.closed&&uncovers==1&&!s_extraEditor&&s_editorChildren.empty());
}
'''
fixture=fixture.replace('// HOST_DIMENSIONS',function(common,'ControlLayoutSize touch_document_size_dp'))
fixture=fixture.replace('// LAYOUT',(root/'src/touch_button_layout.inc').read_text())
fixture=fixture.replace('// ADAPTER','\n'.join(function(adapter,s) for s in (
    'bool editor_background_active','void sync_editor_background','HookAction show_editor_background','HookAction enter_extra_editor','void leave_extra_editor','void extra_editor_controls',
    'void extra_editor_fragment','HookAction save_extra_editor','void close_extra_editor')))
suppression=[]
for method in ('sync_visual_state','sync_action_bar_state','sync_control_displays'):
    body=function(native_source, 'void TouchControls::'+method)
    end=body.index('        return;\n    }')+len('        return;\n    }')
    suppression.append(body[:end].replace('TouchControls::','NativeTouch::')+'\n}')
fixture=fixture.replace('// HOST_SUPPRESSION','\n'.join(suppression))
# The correction runs in the existing update post-hook, after native suppression.
native_adapter=(root/'src/touch_buttons_native.inc').read_text()
assert 'sync_editor_background(controls);' in function(native_adapter,'void update_extra_elements')
with tempfile.TemporaryDirectory() as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I'+str(root/'src'),
                    '-I'+str(dusk/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Native touch editor passed: real viewport contract, full-size buttons, fractional layouts, scoped metadata, Save/Cancel/Reset, read-only native background and teardown')
