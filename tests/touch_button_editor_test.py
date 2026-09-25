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
 struct Element { Context* context=nullptr; };
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
struct EditorDocument {bool closed=false;};
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
 // Mod unload closes reset modals before the editor and resumes its parent.
 EditorDocument modal;s_editorChildren.push_back(&modal);close_extra_editor();
 assert(modal.closed&&ours.closed&&uncovers==1&&!s_extraEditor&&s_editorChildren.empty());
}
'''
fixture=fixture.replace('// HOST_DIMENSIONS',function(common,'ControlLayoutSize touch_document_size_dp'))
fixture=fixture.replace('// LAYOUT',(root/'src/touch_button_layout.inc').read_text())
fixture=fixture.replace('// ADAPTER','\n'.join(function(adapter,s) for s in (
    'HookAction enter_extra_editor','void leave_extra_editor','void extra_editor_controls',
    'void extra_editor_fragment','HookAction save_extra_editor','void close_extra_editor')))
with tempfile.TemporaryDirectory() as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I'+str(root/'src'),
                    '-I'+str(dusk/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Native touch editor passed: real viewport contract, full-size buttons, fractional layouts, scoped metadata, Save/Cancel/Reset and teardown')
