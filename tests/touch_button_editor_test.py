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
 enum class EventId{Mousedown,Mousemove,Mouseup};
 struct Event {Element* target;Vector2f pos;int64_t id=1;bool stopped=false;int button=0;
  template<class T>T GetParameter(const char*,T){return T(button);}
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
  Rml::Element* element;Callback callback;bool capture;bool mouse;int kind;};
 struct Document {
  Rml::ElementDocument* mDocument=nullptr;
  std::vector<std::unique_ptr<ScopedEventListener>> mListeners;
  bool mClosed=false,mPendingClose=false,covered=false;
  virtual ~Document()=default;
  virtual void cover(){covered=true;}
  template<class Event>void listen(Rml::Element* e,Event event,ScopedEventListener::Callback cb,bool c=false){
   int kind=0;
   if constexpr(std::is_same_v<Event,Rml::EventId>)kind=int(event);
   else {const std::string name=event;kind=name=="touchmove"?1:name=="touchend"?2:name=="touchcancel"?3:0;}
   mListeners.push_back(std::make_unique<ScopedEventListener>(ScopedEventListener{e,std::move(cb),c,std::is_same_v<Event,Rml::EventId>,kind}));}
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
 void native_reset_working_layout() noexcept;
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
 float (*scale)(Rml::Context*)=touch_dp_scale;
 SDL_FingerID (*finger)(Rml::Event*)=[](Rml::Event* e){return e->id;};
 void (*stop)(Rml::Event*)=[](Rml::Event* e){e->StopPropagation();};
 void (*applyBox)(Rml::Element*,std::optional<ControlRect>*,ControlRect)=[](Rml::Element*,std::optional<ControlRect>* box,ControlRect r){*box=r;};
 Rml::Context* (*context)(Rml::Element*)=[](Rml::Element* e){return e->context;};
 ControlLayoutSize (*dimensions)(Rml::Context*)=touch_document_size_dp;
 Rml::ElementPtr (*create)(Rml::ElementDocument*,const Rml::String*)=[](Rml::ElementDocument* doc,const Rml::String*){
  auto e=std::make_unique<Rml::Element>();e->context=doc->context;return e;};
 Rml::Element* (*append)(Rml::Element*,Rml::ElementPtr,bool)=[](Rml::Element* p,Rml::ElementPtr e,bool){
  e->parent=p;auto* result=e.get();p->children.push_back(std::move(e));return result;};
 void (*setRml)(Rml::Element*,const Rml::String*)=[](Rml::Element* p,const Rml::String* s){
  size_t pos=0;while((pos=s->find("<button",pos))!=std::string::npos){++pos;
   auto e=std::make_unique<Rml::Element>();e->parent=p;e->context=p->context;p->children.push_back(std::move(e));}
  auto f=std::make_unique<Rml::Element>();f->parent=p;f->context=p->context;
  for(int i=0;i<8;++i){auto h=std::make_unique<Rml::Element>();h->parent=f.get();f->children.push_back(std::move(h));}
  p->children.push_back(std::move(f));};
 Rml::Element* (*child)(Rml::Element*,int)=[](Rml::Element* p,int i){return p->children.at(i).get();};
 Rml::Element* (*target)(Rml::Event*)=[](Rml::Event* e){return e->target;};
 Rml::Element* (*parent)(Rml::Element*)=[](Rml::Element* e){return e->parent;};
} s_touchApi;
void extra_property(Rml::Element*,const char*,const std::string&){}
// LAYOUT
ControlLayout persistedVanilla;int saves=0,uncovered=0,deletedListeners=0;
// ADAPTER
std::array<TouchLayoutControlInfo,10> vanillaInfo;
std::span<const TouchLayoutControlInfo> touch_layout_controls(){return vanillaInfo;}
// NATIVE_HELPERS
// NATIVE_METHODS
int foreignLayouts=0,foreignPre=0,foreignPost=0,foreignEvents=0;
void TouchControlsEditor::sync_control_layouts() noexcept {
 native_sync_control_layouts();++foreignLayouts;
 auto* p=this;void* args[]={&p};layout_extra_editor(nullptr,args,nullptr,nullptr);}
void TouchControlsEditor::reset_working_layout() noexcept {
 native_reset_working_layout();auto* p=this;void* args[]={&p};reset_extra_editor(nullptr,args,nullptr,nullptr);}
Rml::Event dispatch(TouchControlsEditor& e,Rml::Element* target,Rml::Vector2f pos,int64_t finger=1,bool mouse=false,int kind=0){
 Rml::Event event{target,pos,finger};
 for(auto& l:e.mListeners)if(l&&l->capture&&l->mouse==mouse&&l->kind==kind){l->callback(event);if(event.stopped)return event;}
 for(auto& l:e.mListeners)if(l&&!l->capture&&l->mouse==mouse&&l->kind==kind&&l->element==target){l->callback(event);if(event.stopped)break;}
 return event;
}
// The SDK runs post-hooks even when a pre-hook vetoes or replaces the original.
void save(TouchControlsEditor& e,bool foreignFirst=false,bool veto=false,bool replace=false){
 auto* p=&e;void* a[]={&p};bool skip=false;
 if(foreignFirst){++foreignPre;skip=veto||replace;}
 if(!skip)skip=save_extra_editor(nullptr,a,nullptr,nullptr)==HOOK_SKIP_ORIGINAL;
 if(!skip&&!foreignFirst){++foreignPre;skip=veto;}
 if(!skip||replace){persistedVanilla=e.mWorkingLayout;++saves;e.mPendingClose=true;}
 after_save_extra_editor(nullptr,a,nullptr,nullptr);++foreignPost;
}
int main(){
 Rml::Context context;Rml::Element document;document.context=&context;
 Rml::Element frame;
 NativeEditor editor;editor.mRoot=&document;editor.mDocument=&document;editor.mSelectionFrame=&frame;
 std::array<Rml::Element,9> vanillaElements;
 for(size_t i=0;i<9;++i){
  vanillaInfo[i]={kControlLayoutIds[i],nullptr,{float(i)*10,20,78,46,1,ControlAnchor::TopLeft}};
  vanillaElements[i].parent=&document;editor.mElements[i].root=&vanillaElements[i];}
 editor.bind_control_events();

 s_editorApi.listenTouch=[](EditorDocument* d,Rml::Element* e,const Rml::String* s,EditorListener::Callback cb,bool c){d->listen(e,*s,std::move(cb),c);};
 s_editorApi.listenMouse=[](EditorDocument* d,Rml::Element* e,Rml::EventId s,EditorListener::Callback cb,bool c){d->listen(e,s,std::move(cb),c);};
 s_editorApi.deleteListener=[](EditorListener* l){++deletedListeners;delete l;};
 s_editorApi.setClass=[](Rml::Element* e,const Rml::String* s,bool v){e->SetClass(*s,v);};
 s_editorApi.hide=[](EditorDocument* d,bool close){d->mClosed=close;};
 s_editorApi.uncover=[](){++uncovered;};
 s_editorApi.touchPosition=[](const Rml::Event* e){return e->pos;};
 s_editorApi.mousePosition=s_editorApi.touchPosition;
 s_editorApi.eventInt=[](const Rml::Event* e,const Rml::String*,const int*){return e->button;};
 s_editorApi.dockAnchor=touch_control_dock_anchor;
 s_editorApi.applyDock=apply_control_dock_classes;
 s_editorApi.applyTransform=[](Rml::Element* e,std::optional<float>* old,float v){apply_control_transform_if_changed(e,*old,v);};
 const ControlLayoutSize viewport{960,432};
 // A foreign mod extends metadata and stores a custom layout in the host map.
 vanillaInfo[9]={"foreign-mod",nullptr,{20,20,50,50,1,ControlAnchor::TopLeft}};
 Rml::Element foreign;foreign.parent=&document;
 editor.listen(&foreign,aurora::rmlui::TouchStartEvent,[](Rml::Event&){++foreignEvents;});
 ControlProps migrated{.4f,.4f,64.25f,51.75f,1.25f,ControlAnchor::None};
 config.values[touch::Midna]=encode_extra_props(migrated);
 editor.mWorkingLayout.controls["buttonA"]={24,40,70,50,1,ControlAnchor::TopLeft};
 editor.mWorkingLayout.controls["foreign-mod"]=migrated;
 const auto storedBefore=config.values;const auto vanillaBefore=editor.mWorkingLayout;
 editor.sync_control_layouts();auto* state=editor_state(&editor);assert(state);
 assert(state->working[5]==migrated&&config.values==storedBefore);
 assert(editor.mWorkingLayout==vanillaBefore&&touch_layout_controls().size()==10&&foreignLayouts==1);
 std::array<Rml::Element*,6> extras;
 for(size_t i=0;i<6;++i){extras[i]=state->elements[i].root;assert(extras[i]&&state->elements[i].layout.visualRect);}
 for(size_t i=0;i<9;++i){assert(editor.mElements[i].root==&vanillaElements[i]);
  dispatch(editor,&vanillaElements[i],{50,50});assert(editor.mSelectedIndex==i);editor.end_edit(true,1,false);}
 for(size_t i=0;i<6;++i){assert(dispatch(editor,extras[i],{50,50}).stopped);
  assert(state->selected==i&&state->pointer.active&&!editor.mPointerEdit.active);
  editor.sync_selection_frame();assert(!frame.visible);
  dispatch(editor,extras[i],{50,50},1,false,2);}
 assert(!dispatch(editor,&foreign,{40,40}).stopped&&foreignEvents==1);
 assert(state->selected==touch::Count&&touch_layout_controls().back().layoutId=="foreign-mod");
 Rml::Element nested;nested.parent=extras[5];
 dispatch(editor,&nested,{800,400});assert(state->selected==5);
 auto start=state->working[5];
 dispatch(editor,&document,{802,401},1,false,1);assert(state->working[5]==start);
 dispatch(editor,&document,{900,450},2,false,1);assert(state->working[5]==start); // other finger
 dispatch(editor,&document,{900,450},1,false,1);assert(state->working[5]!=start);
 dispatch(editor,&document,{900,450},2,false,2);assert(state->pointer.active);
 dispatch(editor,&document,{900,450},1,false,2);assert(!state->pointer.active);
 const auto moved=state->working[5];
 assert(editor.mWorkingLayout==vanillaBefore);
 // Foreign buttons still receive events immediately after one of our gestures.
 dispatch(editor,&foreign,{40,40});assert(foreignEvents==2&&!state->frame->visible);
 dispatch(editor,&vanillaElements[1],{40,40},0,true);
 assert(editor.mSelectedIndex==1&&!editor.mPointerEdit.touch);
 editor.continue_edit({100,80});editor.end_edit(false,0,false);
 assert(editor.mWorkingLayout.controls["buttonA"]!=vanillaBefore.controls.at("buttonA"));
 assert(state->working[5]==moved);
 // Every move/edge/corner uses the same geometry as the actual pinned native implementation.
 for(int i=0;i<9;++i){
  auto handle=static_cast<EditHandle>(i);
  assert(begin_extra_edit(*state,5,handle,{500,300},true,1));
  editor.mPointerEdit=state->pointer;editor.mPointerEdit.index=1;
  auto ours=state->pointer.startProps,nativeProps=ours;
  auto a=extra_edit_rect(state->pointer,{220,140},viewport,ours);
  auto b=editor.rect_for_edit({220,140},nativeProps);
  assert(a.l==b.l&&a.t==b.t&&a.w==b.w&&a.h==b.h&&ours==nativeProps);editor.mPointerEdit={};state->pointer={};
 }
 dispatch(editor,extras[5],{500,300},0,true);dispatch(editor,&document,{500,300},0,true,2);
 auto rect=*state->elements[5].layout.visualRect;
 dispatch(editor,state->handles[1],{(rect.l+rect.w)*2.5f,rect.t*2.5f},0,true);
 dispatch(editor,&document,{(rect.l+rect.w+20)*2.5f,rect.t*2.5f},0,true,1);
 dispatch(editor,&document,{0,0},0,true,2);
 assert(state->working[5]->w>moved->w);const auto resized=state->working[5];
 rect=*state->elements[5].layout.visualRect;
 dispatch(editor,state->handles[7],{(rect.l+rect.w)*2.5f,(rect.t+rect.h)*2.5f});
 dispatch(editor,&document,{(rect.l+rect.w+30)*2.5f,(rect.t+rect.h+30)*2.5f},1,false,1);
 assert(state->working[5]->scale>resized->scale);
 dispatch(editor,&document,{0,0},1,false,3);assert(state->working[5]==resized);
 // Native and foreign data/hooks survive saves in either hook registration order.
 save(editor);assert(saves==1&&foreignPre==1&&foreignPost==1);
 assert(persistedVanilla==editor.mWorkingLayout&&persistedVanilla.controls.size()==2);
 assert(persistedVanilla.controls.at("foreign-mod")==migrated);
 assert(saved_extra_props(5,viewport)==resized);editor.mPendingClose=false;
 save(editor,true);assert(saves==2&&foreignPre==2&&foreignPost==2);editor.mPendingClose=false;
 auto saved=config.values;state->working[0]=migrated;
 save(editor,false,true);assert(config.values==saved&&saves==2); // later foreign veto rolls back
 save(editor,true,true);assert(config.values==saved&&saves==2); // earlier veto skips our pre
 save(editor,true,false,true);assert(saves==3&&saved_extra_props(0,viewport)==migrated);editor.mPendingClose=false;
 saved=config.values;config.fail=3;state->working[0]=state->defaults[0];
 save(editor);assert(config.values==saved&&saves==3);config.fail=-1;
 editor.reset_working_layout();assert(editor.mWorkingLayout.controls.empty()&&config.values==saved);
 assert(!state->working[5]&&!state->pointer.active&&state->selected==touch::Count);
 save(editor);assert(saves==4&&persistedVanilla.controls.empty());
 assert(saved_extra_props(5,viewport)==state->defaults[5]);editor.mPendingClose=false;
 // Cancel/unload closes child modals and detaches only our callbacks.
 saved=config.values;const auto nativeSaved=persistedVanilla;state->working[5]=migrated;
 EditorDocument modal;state->children.push_back(&modal);
 close_extra_editor();assert(config.values==saved&&persistedVanilla==nativeSaved);
 assert(modal.mClosed&&editor.mClosed&&deletedListeners==7&&uncovered==1&&s_editors.empty());
 for(size_t i=0;i<9;++i)assert(editor.mElements[i].root==&vanillaElements[i]);
 for(const auto& listener:editor.mListeners)if(listener)assert(!listener->capture);
 dispatch(editor,&foreign,{40,40});assert(foreignEvents==3);

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
wrapped = {'sync_control_layouts', 'reset_working_layout'}
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
    sanitizers = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if os.environ.get('SANITIZE') else []
    subprocess.run(['c++', '-std=c++20', *sanitizers, '-Wall', '-Wextra', '-Werror',
                    '-I' + str(root / 'src'), '-I' + str(dusk / 'src'),
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Isolated editor passed: native geometry, foreign metadata/buttons/hooks, unchanged native slots, migration, Save/Reset/Cancel, veto/error rollback and unload')
