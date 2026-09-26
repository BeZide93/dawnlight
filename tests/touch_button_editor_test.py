"""Run the combined editor adapter against pinned native gesture/layout methods.

Requires the pinned Dusklight checkout; override its path with DUSKLIGHT_DIR.
No game assets or renderer needed. Device touch/visual QA remains separate.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
dusk = Path(os.environ.get('DUSKLIGHT_DIR', root / 'dusklight'))
if not dusk.exists():
    dusk = root.parent / 'dusk-source'
adapter = (root / 'src/touch_button_editor.inc').read_text()
native = (dusk / 'src/dusk/ui/touch_controls_editor.cpp').read_text()
header = (dusk / 'src/dusk/ui/touch_controls_editor.hpp').read_text()
common = (dusk / 'src/dusk/ui/touch_controls_common.cpp').read_text()

def function(source, name):
    start = source.index(name + '(')
    return source[start:source.index('\n}', start) + 2]

fixture = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <locale>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
using u8=unsigned char;using s32=int;using SDL_FingerID=int64_t;
#include "dusk/ui/controls.hpp"
#include "touch_button_state.hpp"
using namespace dawnlight;
namespace Rml {
 using String=std::string;
 struct Vector2f {float x=0,y=0;Vector2f operator-(Vector2f b)const{return{x-b.x,y-b.y};}
  Vector2f operator/(float b)const{return{x/b,y/b};}};
 struct Context {struct Dimensions{int x,y;};Dimensions dimensions{2400,1080};
  Dimensions GetDimensions(){return dimensions;}};
 struct Element {
  Element* parent=nullptr;Context* context=nullptr;bool hidden=false,selected=false,visible=false;
  std::vector<std::unique_ptr<Element>> children;
  Context* GetContext(){return context;}
  void SetClass(const String& name,bool v){if(name=="editor-selected")selected=v;else if(name=="visible")visible=v;}
  void SetPseudoClass(const String&,bool v){hidden=v;}
 };
 using ElementPtr=std::unique_ptr<Element>;
 using ElementDocument=Element;
 enum class EventId{Mousedown};
 struct Event {Element* target;Vector2f pos;int64_t id=1;bool stopped=false;
  template<class T>T GetParameter(const char*,T){return T(0);}
  void StopPropagation(){stopped=true;}};
}
namespace aurora::rmlui {constexpr const char* TouchStartEvent="touchstart";}
namespace dusk::ui {
 constexpr size_t kTouchLayoutControlCount=9;
 struct TouchLayoutControlInfo {std::string_view layoutId;const char* elementId=nullptr;
  ControlProps props;Control control=Control::COUNT;bool hasControl=false;};
 float touch_dp_scale(Rml::Context*){return 2.5f;}
 // COMMON
 Rml::Vector2f touch_event_position(Rml::Event& e){return e.pos;}
 Rml::Vector2f mouse_event_position(Rml::Event& e){return e.pos;}
 SDL_FingerID touch_event_id(Rml::Event& e){return e.id;}
 void apply_control_box_if_changed(Rml::Element*,std::optional<ControlRect>& box,ControlRect r){box=r;}
 void apply_control_dock_classes(Rml::Element*,ControlAnchor){}
 void apply_control_transform_if_changed(Rml::Element*,std::optional<float>& old,float s){old=s;}
 struct ScopedEventListener {using Callback=std::function<void(Rml::Event&)>;
  Rml::Element* element;Callback callback;bool capture;bool mouse;};
 struct Document {
  Rml::ElementDocument* mDocument=nullptr;
  std::vector<std::unique_ptr<ScopedEventListener>> mListeners;
  bool mClosed=false,mPendingClose=false,covered=false;
  virtual ~Document()=default;
  virtual void cover(){covered=true;}
  template<class Event>void listen(Rml::Element* e,Event,ScopedEventListener::Callback cb,bool c=false){
   mListeners.push_back(std::make_unique<ScopedEventListener>(ScopedEventListener{e,std::move(cb),c,std::is_same_v<Event,Rml::EventId>}));}
 };
 struct TouchControlsEditor:Document {
 // TYPES
 Rml::Element* mRoot=nullptr;Rml::Element* mSelectionFrame=nullptr;
 std::array<EditElement,kTouchLayoutControlCount> mElements{};
 ControlLayout mWorkingLayout;PointerEdit mPointerEdit;
 std::optional<ControlRect> mAppliedSelectionFrame;
 size_t mSelectedIndex=kTouchLayoutControlCount;
 // DECLARATIONS
 void native_sync_control_layouts() noexcept;
 void native_sync_selection_frame() noexcept;
 bool native_begin_edit(size_t,EditHandle,Rml::Vector2f,bool,SDL_FingerID) noexcept;
 bool native_continue_edit(Rml::Vector2f) noexcept;
 void native_restore_active_control() noexcept;
 };
}
using namespace dusk::ui;
struct ModContext{};ModContext* mod_ctx=nullptr;
constexpr int MOD_OK=0,MOD_ERROR=1;
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* a,int i){return *static_cast<T*>(static_cast<void**>(a)[i]);}}
struct Config {
 std::map<int,std::string> values;int fail=-1;
 int get_string(ModContext*,int k,char* out,size_t n,void*){const auto it=values.find(k);const std::string v=it==values.end()?"":it->second;assert(v.size()<n);std::strcpy(out,v.c_str());return MOD_OK;}
 int set_string(ModContext*,int k,const char* v){if(k==fail)return MOD_ERROR;values[k]=v;return MOD_OK;}
} config;auto* svc_config=&config;
struct Log {void warn(ModContext*,const char*){}} logService;auto* svc_log=&logService;
struct ButtonConfig{int layout;};
std::array<ButtonConfig,touch::Count> s_buttons={{{0},{1},{2},{3},{4},{5}}};
touch::Layout button_layout(size_t i){return touch::Defaults[i];}
bool s_touchEditorAvailable=true;
struct TouchApi {
 Rml::Context* (*context)(Rml::Element*)=[](Rml::Element* e){return e->context;};
 ControlLayoutSize (*dimensions)(Rml::Context*)=touch_document_size_dp;
 Rml::ElementPtr (*create)(Rml::ElementDocument*,const Rml::String*)=[](Rml::ElementDocument* doc,const Rml::String*){
  auto e=std::make_unique<Rml::Element>();e->context=doc->context;return e;};
 Rml::Element* (*append)(Rml::Element*,Rml::ElementPtr,bool)=[](Rml::Element* p,Rml::ElementPtr e,bool){
  e->parent=p;auto* result=e.get();p->children.push_back(std::move(e));return result;};
 void (*setRml)(Rml::Element*,const Rml::String*)=[](Rml::Element* p,const Rml::String* s){
  size_t pos=0;while((pos=s->find("<button",pos))!=std::string::npos){++pos;
   auto e=std::make_unique<Rml::Element>();e->parent=p;e->context=p->context;p->children.push_back(std::move(e));}};
 Rml::Element* (*child)(Rml::Element*,int)=[](Rml::Element* p,int i){return p->children.at(i).get();};
 Rml::Element* (*target)(Rml::Event*)=[](Rml::Event* e){return e->target;};
 Rml::Element* (*parent)(Rml::Element*)=[](Rml::Element* e){return e->parent;};
} s_touchApi;
void extra_property(Rml::Element*,const char*,const std::string&){}
// LAYOUT
struct ExtraEditorLayouts{static void g_orig(TouchControlsEditor* e){e->native_sync_control_layouts();}};
ControlLayout persistedVanilla;int saves=0,uncovered=0,deletedListeners=0;
struct ExtraEditorSave{static void g_orig(TouchControlsEditor* e){persistedVanilla=e->mWorkingLayout;++saves;e->mPendingClose=true;}};
// ADAPTER
std::array<TouchLayoutControlInfo,9> vanillaInfo;
std::span<const TouchLayoutControlInfo> touch_layout_controls(){
 LayoutSpan result=vanillaInfo;extra_editor_controls(nullptr,nullptr,&result,nullptr);return result;}
// NATIVE_HELPERS
// NATIVE_METHODS
void TouchControlsEditor::sync_control_layouts() noexcept {
 auto* p=this;void* args[]={&p};if(layout_extra_editor(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE)native_sync_control_layouts();}
void TouchControlsEditor::sync_selection_frame() noexcept {
 auto* p=this;void* args[]={&p};enter_extra_editor(nullptr,args,nullptr,nullptr);
 native_sync_selection_frame();leave_extra_editor(nullptr,args,nullptr,nullptr);}
bool TouchControlsEditor::begin_edit(size_t i,EditHandle h,Rml::Vector2f p,bool t,SDL_FingerID id) noexcept {
 auto* self=this;void* args[]={&self};enter_extra_editor(nullptr,args,nullptr,nullptr);
 auto r=native_begin_edit(i,h,p,t,id);leave_extra_editor(nullptr,args,nullptr,nullptr);return r;}
bool TouchControlsEditor::continue_edit(Rml::Vector2f p) noexcept {
 auto* self=this;void* args[]={&self};enter_extra_editor(nullptr,args,nullptr,nullptr);
 auto r=native_continue_edit(p);leave_extra_editor(nullptr,args,nullptr,nullptr);return r;}
void TouchControlsEditor::restore_active_control() noexcept {
 auto* p=this;void* args[]={&p};enter_extra_editor(nullptr,args,nullptr,nullptr);
 native_restore_active_control();leave_extra_editor(nullptr,args,nullptr,nullptr);}
void dispatch(TouchControlsEditor& e,Rml::Element* target,Rml::Vector2f pos,int64_t finger=1,bool mouse=false){
 Rml::Event event{target,pos,finger};
 // Rml capture on the root precedes target listeners; one of the two input kinds.
 for(auto& l:e.mListeners)if(l->capture&&l->mouse==mouse){l->callback(event);break;}
 for(auto& l:e.mListeners)if(!l->capture&&l->mouse==mouse&&l->element==target){l->callback(event);break;}
}
void save(TouchControlsEditor& e){auto* p=&e;void* a[]={&p};assert(save_extra_editor(nullptr,a,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);}
int main(){
 Rml::Context context;Rml::Element document;document.context=&context;
 Rml::Element frame;
 NativeEditor editor;editor.mRoot=&document;editor.mDocument=&document;editor.mSelectionFrame=&frame;
 std::array<Rml::Element,9> vanillaElements;
 for(size_t i=0;i<9;++i){
  vanillaInfo[i]={kControlLayoutIds[i],nullptr,{float(i)*10,20,78,46,1,ControlAnchor::TopLeft}};
  vanillaElements[i].parent=&document;editor.mElements[i].root=&vanillaElements[i];}
 editor.bind_control_events();
 s_editorApi.bindControls=[](NativeEditor* e){e->bind_control_events();};
 s_editorApi.listenTouch=[](EditorDocument* d,Rml::Element* e,const Rml::String* s,EditorListener::Callback cb,bool c){d->listen(e,*s,std::move(cb),c);};
 s_editorApi.listenMouse=[](EditorDocument* d,Rml::Element* e,Rml::EventId s,EditorListener::Callback cb,bool c){d->listen(e,s,std::move(cb),c);};
 s_editorApi.deleteListener=[](EditorListener* l){++deletedListeners;delete l;};
 s_editorApi.setClass=[](Rml::Element* e,const Rml::String* s,bool v){e->SetClass(*s,v);};
 s_editorApi.hide=[](EditorDocument* d,bool close){d->mClosed=close;};
 s_editorApi.uncover=[](){++uncovered;};
 const ControlLayoutSize viewport{960,432};
 // Existing fractional layouts migrate without touching any native setting.
 ControlProps migrated{.4f,.4f,64.25f,51.75f,1.25f,ControlAnchor::None};
 config.values[touch::Midna]=encode_extra_props(migrated);
 editor.mWorkingLayout.controls["buttonA"]={24,40,70,50,1,ControlAnchor::TopLeft};
 const auto storedBefore=config.values;const auto vanillaBefore=editor.mWorkingLayout;
 editor.sync_control_layouts();auto* state=editor_state(&editor);assert(state&&!state->extras);
 assert(editor.mElements.size()==9&&state->inactive.size()==9);
 assert(editor.mWorkingLayout.controls["dawnlight-midna"]==migrated&&config.values==storedBefore);
 assert(editor.mWorkingLayout.controls["buttonA"]==vanillaBefore.controls.at("buttonA"));
 std::array<Rml::Element*,6> extras;
 for(size_t i=0;i<6;++i){extras[i]=state->inactive[i].root;assert(extras[i]&&state->inactive[i].layout.visualRect);}
 assert(touch_layout_controls().size()==9); // global metadata never leaks into gameplay
 // Every original and added control selects its own bank/slot; mouse follows the same path.
 for(size_t i=0;i<9;++i){dispatch(editor,&vanillaElements[i],{50,50});
  assert(!state->extras&&editor.mSelectedIndex==i);editor.end_edit(true,1,false);}
 for(size_t i=0;i<6;++i){dispatch(editor,extras[i],{50,50});
  assert(state->extras&&editor.mSelectedIndex==i);editor.end_edit(true,1,false);}
 Rml::Element nested;nested.parent=extras[5];Rml::Event nestedEvent{&nested,{50,50}};
 select_editor_bank(*state,nestedEvent);assert(state->extras);
 editor.clear_selected_control();editor.sync_selection_frame();assert(!frame.visible);
 dispatch(editor,&vanillaElements[8],{50,50},0,true);
 assert(!state->extras&&editor.mSelectedIndex==8&&!editor.mPointerEdit.touch);
 editor.end_edit(false,0,false);
 dispatch(editor,extras[0],{50,50},0,true);
 assert(state->extras&&editor.mSelectedIndex==0&&!editor.mPointerEdit.touch);
 editor.end_edit(false,0,false);

 // Click Midna and drag using the actual native threshold/geometry methods.
 dispatch(editor,extras[5],{800,400});assert(state->extras&&editor.mSelectedIndex==5&&editor.mPointerEdit.active);
 const auto start=editor.mWorkingLayout.controls["dawnlight-midna"];
 editor.continue_edit({802,401});assert(editor.mWorkingLayout.controls["dawnlight-midna"]==start);
 editor.continue_edit({900,450});assert(editor.mWorkingLayout.controls["dawnlight-midna"]!=start);
 assert(editor.mElements[5].root==extras[5]&&extras[5]->selected);
 // A second finger on a vanilla control cannot switch banks mid-drag.
 dispatch(editor,&vanillaElements[1],{40,40},2);assert(state->extras&&editor.mSelectedIndex==5);
 assert(!editor.end_edit(true,2,false));assert(editor.end_edit(true,1,false));
 const auto moved=editor.mWorkingLayout.controls["dawnlight-midna"];
 dispatch(editor,&vanillaElements[1],{40,40});assert(!state->extras&&editor.mSelectedIndex==1&&!extras[5]->selected);
 editor.continue_edit({100,80});editor.end_edit(true,1,false);
 assert(editor.mWorkingLayout.controls["dawnlight-midna"]==moved);
 assert(editor.mWorkingLayout.controls["buttonA"]!=vanillaBefore.controls.at("buttonA"));
 // Resize an extra button through native edge and corner paths.
 dispatch(editor,extras[5],{500,300});editor.end_edit(true,1,false);
 auto rect=*editor.mElements[5].layout.visualRect;
 assert(editor.begin_edit(5,NativeEditor::EditHandle::Right,{(rect.l+rect.w)*2.5f,rect.t*2.5f},true,1));
 editor.continue_edit({(rect.l+rect.w+20)*2.5f,rect.t*2.5f});editor.end_edit(true,1,false);
 assert(editor.mWorkingLayout.controls["dawnlight-midna"].w>moved.w);
 const auto resized=editor.mWorkingLayout.controls["dawnlight-midna"];
 rect=*editor.mElements[5].layout.visualRect;
 editor.begin_edit(5,NativeEditor::EditHandle::BottomRight,{(rect.l+rect.w)*2.5f,(rect.t+rect.h)*2.5f},true,1);
 editor.continue_edit({(rect.l+rect.w+30)*2.5f,(rect.t+rect.h+30)*2.5f});
 assert(editor.mWorkingLayout.controls["dawnlight-midna"].scale>resized.scale);
 editor.end_edit(true,1,true);assert(editor.mWorkingLayout.controls["dawnlight-midna"]==resized); // cancel gesture
 // Save both groups; Dawnlight keys must never enter the host's nine-ID serializer.
 save(editor);assert(saves==1&&persistedVanilla.controls.size()==1);
 assert(persistedVanilla.controls.at("buttonA")==editor.mWorkingLayout.controls["buttonA"]);
 assert(saved_extra_props(5,viewport)==resized);editor.mPendingClose=false;
 // A write failure restores prior extra settings and leaves native Save untouched.
 auto saved=config.values;config.fail=3;editor.mWorkingLayout.controls["dawnlight-lb"]=migrated;
 save(editor);assert(config.values==saved&&saves==1);config.fail=-1;
 // Reset clears the shared working map only. Both banks show defaults, config stays saved.
 editor.reset_working_layout();assert(editor.mWorkingLayout.controls.empty()&&config.values==saved);
 assert(editor.mElements[5].layout.visualRect);save(editor);assert(saves==2&&persistedVanilla.controls.empty());
 assert(saved_extra_props(5,viewport)==state->controls[5].props);editor.mPendingClose=false;
 // Cancel the editor: no save callback means neither group is persisted.
 saved=config.values;const auto nativeSaved=persistedVanilla;
 editor.mWorkingLayout.controls["dawnlight-midna"]=migrated;
 editor.mWorkingLayout.controls["buttonA"]=migrated;
 EditorDocument modal;state->children.push_back(&modal);
 close_extra_editor();assert(config.values==saved&&persistedVanilla==nativeSaved);
 assert(modal.mClosed&&editor.mClosed&&deletedListeners==2&&uncovered==1&&s_editors.empty());
 assert(editor.mElements[1].root==&vanillaElements[1]);
 for(const auto& listener:editor.mListeners)if(listener)assert(!listener->capture); // no module callbacks after unload
 // No adapter touches the native touch overlay: modal visibility remains native.
 assert(!s_editorScope&&s_editorScopes.empty());
}
'''
fixture = fixture.replace('// COMMON', '\n'.join(function(common, name) for name in (
    'ControlLayoutSize touch_document_size_dp', 'bool control_float_near', 'ControlAnchor touch_control_dock_anchor')))
types = header[header.index('    enum class EditHandle'):header.index('    void bind_control_events')]
fixture = fixture.replace('// TYPES', types.replace('private:', 'public:'))
methods = ('bind_control_events', 'sync_control_layouts', 'sync_selection_frame',
           'set_selected_control', 'clear_selected_control', 'props_for', 'store_props',
           'restore_active_control', 'begin_edit', 'continue_edit', 'end_edit',
           'pointer_position_dp', 'rect_for_edit', 'clamp_visual_rect', 'min_visual_size',
           'reset_working_layout')
sigs = {name: re.search(r'^[\w:]+ TouchControlsEditor::' + name + r'\(', native, re.M).group()[:-1]
        for name in methods}
bodies = [function(native, sigs[name]) for name in methods]
fixture = fixture.replace('// DECLARATIONS', '\n'.join(b.split('{', 1)[0].replace('TouchControlsEditor::', '').replace('SDL_FingerID touchId)', 'SDL_FingerID touchId = 0)') + ';' for b in bodies))
wrapped = {'sync_control_layouts', 'sync_selection_frame', 'begin_edit', 'continue_edit', 'restore_active_control'}
fixture = fixture.replace('// NATIVE_METHODS', '\n'.join(
    b.replace('TouchControlsEditor::' + name, 'TouchControlsEditor::native_' + name, 1) if name in wrapped else b
    for name, b in zip(methods, bodies)))
helpers = [function(native, sig) for sig in (
    'bool is_corner', 'bool is_horizontal_edge', 'bool is_vertical_edge',
    'bool control_valid', 'float squared_distance')]
constants = native[native.index('constexpr float kDragThresholdDp'):native.index('struct HandleBinding')]
fixture = fixture.replace('// NATIVE_HELPERS', constants + '\n' + '\n'.join(helpers))
fixture = fixture.replace('// LAYOUT', (root / 'src/touch_button_layout.inc').read_text())
production = adapter[:adapter.index('struct EditorHookBinding')]
production = re.sub(r'DEFINE_HOOK_SYMBOL\(.*?\);\n', '', production, flags=re.S)
fixture = fixture.replace('// ADAPTER', production)
# The old workaround must never raise TouchControls above the combined editor/modal.
assert 'ExtraEditorBackground' not in adapter
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(root / 'src'), '-I' + str(dusk / 'src'),
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Combined editor passed: 15 controls, native move/resize/scale/cancel, bank isolation, migration, Save/Reset/Cancel, failed-save rollback and unload')
