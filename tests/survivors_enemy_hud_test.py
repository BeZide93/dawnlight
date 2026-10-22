"""Exercise the production enemy HUD with SDK service ABIs and engine fixtures."""
from pathlib import Path
import os, re, subprocess, tempfile, sys
root=Path(__file__).resolve().parents[1]
sdk=Path(sys.argv[1]) if len(sys.argv)>1 else root/'dusklight'
source=(root/'src/survivors_enemy_hud.cpp').read_text()
source=re.sub(r'^#include "[^"\n]+"\n','',source,flags=re.M)
fixture=r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "mods/svc/ui.h"
#include "mods/svc/config.h"
#include "mods/svc/hook.h"
#include "survivors_enemy_hud.hpp"
#include "survivors_enemy_hud_state.hpp"
ModContext* mod_ctx=nullptr;
namespace dusk::config {
struct ConfigVarBase{};
template<class T>struct ConfigVar:ConfigVarBase {T value{};T getValue()const{return value;}};
}
std::map<std::string,dusk::config::ConfigVar<bool>> vars;
dusk::config::ConfigVarBase* get_var(std::string_view key){auto it=vars.find(std::string(key));return it==vars.end()?nullptr:&it->second;}
void te(const char* key,bool v){vars[std::string("mod.com_dusklight_twilit__essentials.")+key].value=v;}
bool resolveFails=false;unsigned resolves=0;
HookService hook=[] {HookService s{};s.resolve=[](ModContext*,const char* name,void** out,HookSymbolFlags*) {
 ++resolves;assert(std::string_view(name)=="dusk::config::GetConfigVar");
 *out=resolveFails?nullptr:reinterpret_cast<void*>(&get_var);return resolveFails?MOD_UNAVAILABLE:MOD_OK;
};return s;}();
const HookService* svc_hook=&hook;
std::vector<bool> values;std::vector<std::string> names;
unsigned writes=0;
namespace dawnlight {
ModResult register_owned_setting(const ConfigVarDesc* desc,ConfigVarHandle* out){
 assert(desc->type==CONFIG_VAR_BOOL&&!desc->default_bool);
 const auto it=std::find(names.begin(),names.end(),desc->name);
 if(it==names.end()){names.push_back(desc->name);values.push_back(desc->default_bool);*out=names.size();}
 else *out=1+(it-names.begin());
 return MOD_OK;
}}
ConfigService cfg=[] {ConfigService s{};
 s.get_bool=[](ModContext*,ConfigVarHandle h,bool* out){*out=values.at(h-1);return MOD_OK;};
 s.set_bool=[](ModContext*,ConfigVarHandle h,bool v){++writes;values.at(h-1)=v;return MOD_OK;};return s;}();
const ConfigService* svc_config=&cfg;
std::vector<UiControlDesc> controls;
std::map<UiElementHandle,bool> visible;UiElementHandle nextHandle=1;
UiService ui=[] {UiService s{};
 s.pane_add_section=[](ModContext*,UiElementHandle,const char* name){assert(std::string_view(name)=="Enemy HP Bars");return MOD_OK;};
 s.pane_add_text=[](ModContext*,UiElementHandle,const char* text,UiElementHandle* out){
  assert(std::string_view(text).find("managed by Twilit Essentials")!=std::string_view::npos);
  *out=nextHandle++;visible[*out]=true;return MOD_OK;};
 s.elem_set_visible=[](ModContext*,UiElementHandle h,bool v){if(!visible.count(h))return MOD_INVALID_ARGUMENT;visible[h]=v;return MOD_OK;};
 s.pane_add_control=[](ModContext*,UiElementHandle,const UiControlDesc* c,UiElementHandle*){assert(c->kind==UI_CONTROL_TOGGLE);controls.push_back(*c);return MOD_OK;};return s;}();
const UiService* svc_ui=&ui;
using u8=unsigned char;using f32=float;
struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float n)const{return{x*n,y*n,z*n};}float abs()const{return std::sqrt(x*x+y*y+z*z);}};
constexpr unsigned fopAcStts_NODRAW_e=1,fopAcCnd_NODRAW_e=1;
struct fopAc_ac_c {unsigned id=1,actor_status=0,actor_condition=0;int health=100,field_0x560=100;
 struct{cXyz pos{200,100,10};}current;struct{cXyz position{200,155,10};}attention_info;cXyz eyePos{200,155,10};};
unsigned fopAcM_GetID(const fopAc_ac_c* actor){return actor->id;}
fopAc_ac_c player;
auto* dComIfGp_getPlayer(int){return &player;}
bool event=false,view=true,occluded=false;unsigned rays=0;
bool dComIfGp_event_runCheck(){return event;}bool dComIfGd_getView(){return view;}
float matrix[4][4]={{},{},{},{0,0,1,0}};
auto* dComIfGd_getProjViewMtx(){return &matrix;}
void mDoLib_project(cXyz* world,cXyz* screen){*screen=*world;}
struct mDoGph_gInf_c{static float getMinXF(){return 0;}static float getMaxXF(){return 608;}static float getHeightF(){return 448;}};
struct Camera{cXyz iEye(){return{0,0,0};}bool lineBGCheck(cXyz*,cXyz*,int flags){assert(flags==0x40b7);++rays;return occluded;}}camera;
auto* dCam_getBody(){return &camera;}
namespace JUtility {struct TColor{u8 r,g,b,a;TColor(unsigned red,unsigned green,unsigned blue,unsigned alpha):r(red),g(green),b(blue),a(alpha){}bool operator==(const TColor&)const=default;};}
namespace JGeometry{template<class T>struct TBox2{T x,y,right,bottom;TBox2(T a,T b,T c,T d):x(a),y(b),right(c),bottom(d){}};}
unsigned boxes=0;bool fontState=false;
struct Graf {JUtility::TColor mColorTL{1,2,3,4},mColorTR{5,6,7,8},mColorBR{9,10,11,12},mColorBL{13,14,15,16};
 void setup2D(){fontState=false;}void setColor(JUtility::TColor c){setColor(c,c,c,c);}
 void setColor(JUtility::TColor a,JUtility::TColor b,JUtility::TColor c,JUtility::TColor d){mColorTL=a;mColorTR=b;mColorBR=c;mColorBL=d;}
 void fillBox(JGeometry::TBox2<float> box){assert(!fontState&&box.right>=box.x&&box.bottom>=box.y);++boxes;}}graf;
auto* dComIfGp_getCurrentGrafPort(){return &graf;}
int font=1;int* mDoExt_getSubFont(){return &font;}int* mDoExt_getMesgFont(){return &font;}
constexpr int HBIND_CENTER=1,VBIND_TOP=1;std::vector<std::string> printed;
struct J2DPrint{J2DPrint(int*,JUtility::TColor,JUtility::TColor){}void setFontSize(float,float){}
 void printReturn(const char* s,int,float,int,int,float,float,u8){fontState=true;printed.emplace_back(s);}};
'''
main=r'''
int main(){
 using namespace dawnlight::enemy_hud;
 // Independent ownership and prerequisite combinations; no mutation of preferences.
 for(unsigned a=0;a<8;++a)for(unsigned b=0;b<8;++b){
  const Options own{bool(a&1),bool(a&2),bool(a&4)},te{bool(b&1),bool(b&2),bool(b&4)};
  const auto active=local_effects(own,te);
  assert(active.bars==(own.bars&&!te.bars));assert(active.damage==(own.damage&&!te.damage));
  assert(active.numbers==(own.numbers&&!te.numbers&&(own.bars||te.bars)));
  assert(disabled(0,own,te)==te.bars&&disabled(2,own,te)==te.damage);
 }
 Health h;assert(!h.observe(1,80,100));assert(h.observe(1,65,100)==15);
 assert(!h.observe(1,90,100));assert(!h.observe(1,190,200));
 assert(!h.observe(2,30,100));assert(h.observe(2,-10,100)==30);assert(!h.observe(2,-20,100));
 assert(health_color(1)[1]>health_color(1)[0]);assert(health_color(0)[0]>health_color(0)[1]);
 resolveFails=true;assert(initialize()==MOD_OK);assert(names.size()==3&&!values[0]);
 assert(build(1)==MOD_OK);assert(controls.size()==3);assert(controls[1].is_disabled(nullptr,controls[1].user_data));
 UiControlValue on{};on.bool_value=true;for(unsigned i:{0u,1u,2u})controls[i].set(nullptr,controls[i].user_data,&on);
 assert(values[0]&&values[1]&&values[2]);assert((effects==Options{true,true,true}));
 resolveFails=false;te("enabled",true);te("hpBarsEnabled",true);te("hpBarsShowNumbers",false);refresh();
 assert((effects==Options{false,true,true}));assert(visible[1]&&!visible[2]&&!visible[3]);
 const auto previousWrites=writes;UiControlValue off{};controls[0].set(nullptr,controls[0].user_data,&off);
 assert(writes==previousWrites&&values[0]);
 te("hpBarsShowNumbers",true);te("damageNumbersEnabled",true);refresh();
 assert((effects==Options{}));for(auto c:controls){assert(c.is_disabled(nullptr,c.user_data));UiControlValue v{};c.get(nullptr,c.user_data,&v);assert(v.bool_value);}
 te("enabled",false);refresh();assert((effects==Options{true,true,true})); // Retained provider vars ignored.
 assert(build(2)==MOD_OK&&notes_live.size()==6); // Two concurrent settings windows.
 for(auto it=visible.begin();it!=visible.end();)if(it->first<=3)it=visible.erase(it);else ++it;
 refresh();assert(notes_live.size()==3);visible.clear();refresh();assert(notes_live.empty());
 shutdown();assert(initialize()==MOD_OK&&values[0]&&values[1]&&values[2]); // Reload keeps settings.
 fopAc_ac_c enemy;observe(0,&enemy,true);assert(popups[0].age==lifetime);
 enemy.health=75;observe(0,&enemy,true);assert(popups[0].amount==25&&popups[0].age==0);
 const auto y=popups[0].position.y;refresh();draw();assert(popups[0].age==0&&popups[0].position.y==y); // Rendering/host updates don't age a paused run.
 tick();assert(popups[0].age==1&&popups[0].position.y>y);
 const auto originalColor=graf.mColorTL;boxes=0;printed.clear();draw();
 assert(boxes>0&&std::find(printed.begin(),printed.end(),"75/100")!=printed.end());
 assert(std::find(printed.begin(),printed.end(),"-25")!=printed.end());assert(graf.mColorTL==originalColor&&!fontState);
 enemy.health=0;observe(0,&enemy,true);assert(popups[1].amount==75&&!entries[0].eligible);
 forget(0);enemy.id=2;enemy.health=20;observe(0,&enemy,true);assert(popups[2].age==lifetime);
 enemy.health=19;observe(0,&enemy,true);assert(popups[2].amount==1);
 // Ownership handoff clears pending feedback and establishes a fresh health baseline.
 te("enabled",true);refresh();assert(popups[0].age==lifetime);enemy.health=1;observe(0,&enemy,true);
 te("enabled",false);refresh();observe(0,&enemy,true);assert(popups[0].age==lifetime);
 reset();enemy.health=100;
 for(unsigned i=0;i<100;++i){enemy.id=i+10;enemy.current.pos.z=10+i;observe(i,&enemy,true);}
 rays=0;tick();assert(rays==8);for(unsigned n=0;n<8;++n){rays=0;tick();assert(rays==8);}
 unsigned fading=0;for(auto e:entries)if(e.alpha>0)++fading;assert(fading==64);
 // Off-screen, behind camera, invisible, ineligible and missing slots don't draw.
 reset();enemy.current.pos={200,100,-10};observe(0,&enemy,true);tick();assert(entries[0].alpha==0);
 enemy.current.pos={2000,100,10};observe(0,&enemy,true);tick();assert(entries[0].alpha==0);
 enemy.current.pos={200,100,10};enemy.actor_status=fopAcStts_NODRAW_e;observe(0,&enemy,true);tick();assert(!entries[0].eligible);
 enemy.actor_status=0;observe(0,&enemy,false);tick();assert(!entries[0].eligible);
 observe(0,&enemy,true);occluded=true;rays=0;tick();assert(rays==1&&entries[0].alpha==0);occluded=false;
 view=false;rays=0;tick();assert(!rays);view=true;
 reset();for(unsigned i=0;i<200;++i){enemy.id=i+1;enemy.health=100;observe(0,&enemy,true);enemy.health=90;observe(0,&enemy,true);}
 assert(std::count_if(popups.begin(),popups.end(),[](auto p){return p.age<lifetime;})==96);
 event=true;boxes=0;printed.clear();draw();assert(!boxes&&printed.empty());event=false;
 for(unsigned i=0;i<lifetime;++i)tick();
 assert(std::all_of(popups.begin(),popups.end(),[](auto p){return p.age==lifetime;}));
 reset();assert(!entries[0].eligible);values={false,false,false};refresh();rays=0;observe(0,&enemy,true);tick();draw();assert(!rays&&!entries[0].eligible);
 shutdown();puts("Survivors enemy HUD: independent TE ownership, UI lifetime, health, bounded work, rendering and reset passed");
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp=Path(temp)/'test.cpp';exe=Path(temp)/'test';cpp.write_text(fixture+source+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-I',str(root/'src'),'-I',str(sdk/'sdk/include'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
