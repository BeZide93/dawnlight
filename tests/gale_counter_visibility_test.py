"""Run actual stamina/counter draw callbacks against transition and HUD fades."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
combat = (root/'src/combat_meter.cpp').read_text()
stamina = (root/'src/stamina.cpp').read_text()
counter = (root/'src/gale_counter.cpp').read_text()

def function(source, name):
    start = re.search(r'^[^\n]*\b'+name+r'\([^;{}]*\)\s*\{', source, re.M).start()
    end = source.index('{', start)+1
    depth = 1
    while depth:
        depth += (source[end]=='{')-(source[end]=='}')
        end += 1
    return source[start:end]

fixture = r'''
#include <cassert>
#include <cmath>
#include <initializer_list>
struct ModContext {};
struct Pane {
    Pane* parent=nullptr;unsigned alpha=255;bool visible=true,influenced=true;
    Pane* getParentPane(){return parent;} bool isVisible(){return visible;}
    unsigned getAlpha(){return alpha;} bool isInfluencedAlpha(){return influenced;}
};
struct CPaneMgr {Pane pane;Pane* getPanePtr(){return &pane;}};
struct dMeter2Draw_c {Pane* mpKanteraScreen;CPaneMgr* mpMagicParent;};
struct daAlink_c {bool wolf=false;bool checkWolf(){return wolf;}} link;
bool window=false,pauseStatus=false,pause=false,event=false,shop=false,talk=false;
bool nextStage=false,gale=true,counterVisible=true,staminaVisible=true,custom=false;
bool dMeter2Info_getWindowStatus(){return window;}
bool dMeter2Info_getPauseStatus(){return pauseStatus;}
bool dComIfGp_isPauseFlag(){return pause;}
bool dComIfGp_event_runCheck(){return event;}
bool dMeter2Info_isShopTalkFlag(){return shop;}
bool dMsgObject_isTalkNowCheck(){return talk;}
bool dComIfGp_isEnableNextStage(){return nextStage;}
bool revalis_gale_enabled(){return gale;}
bool gale_counter_visible(){return counterVisible;}
bool stamina_meter_visible(){return staminaVisible;}
bool custom_hud_layout_enabled(){return custom;}
daAlink_c* daAlink_getAlinkActorClass(){return &link;}
namespace mods {template<class T> T arg(void* a,int){return static_cast<T>(a);}}
enum class CombatMeterStyle {Stamina,StaminaExhausted};
enum class HudElement {GaleCounter};
struct DuskModHudTransform {float offset_x=0,offset_y=0,scale=1;};
DuskModHudTransform hud_custom_element_transform(HudElement){return {25,0,1};}
struct {bool exhausted=false;float stamina=100;} s_state;
struct {int capacity=3;int available(){return 3;}} s_charges;
void update_charges(){}
float counterAlpha=0,barAlpha=0;
struct {void draw(float,float,float,float alpha,int,int){counterAlpha=alpha;}} s_visual;
bool combat_meter_next_row_anchor(dMeter2Draw_c*,int,float& x,float& y,float& s){x=10;y=20;s=1;return true;}
// HELPERS
void draw_combat_meter(dMeter2Draw_c* m,float,CombatMeterStyle,int){barAlpha=combat_meter_screen_alpha(m);}
// CALLBACKS
void near(float a,float b){assert(std::fabs(a-b)<0.0001f);}
int main(){
    Pane screen;CPaneMgr magic;magic.pane.parent=&screen;
    dMeter2Draw_c meter{&screen,&magic};
    auto draw=[&]{barAlpha=counterAlpha=0;after_meter_draw(nullptr,&meter,nullptr,nullptr);after_draw(nullptr,&meter,nullptr,nullptr);};
    for(bool* blocked:{&window,&pauseStatus,&pause,&event,&shop,&talk}){
        *blocked=true;draw();near(barAlpha,0);near(counterAlpha,0);
        *blocked=false;draw();near(barAlpha,1);near(counterAlpha,1);
    }
    // Fade in/out on successive presentation frames, without simulation ticks.
    for(unsigned alpha:{0u,16u,64u,128u,255u,128u,16u,0u}){
        screen.alpha=alpha;draw();near(counterAlpha,barAlpha);near(counterAlpha,alpha/255.0f);
    }
    screen.alpha=255;screen.visible=false;draw();near(counterAlpha,0);near(barAlpha,0);
    screen.visible=true;
    // The native magic/oil pane itself is restored to zero between bar draws;
    // that must not hide the counter. Only containing panes contribute.
    magic.pane.alpha=0;magic.pane.visible=false;draw();near(counterAlpha,1);near(barAlpha,1);
    Pane parent{&screen,128};magic.pane.parent=&parent;screen.alpha=128;
    draw();near(counterAlpha,(128u*128u/255u)/255.0f);
    magic.pane.influenced=false;draw();near(counterAlpha,1);
    parent.visible=false;draw();near(counterAlpha,0);parent.visible=true;
    magic.pane.parent=&screen;screen.alpha=255;magic.pane.influenced=true;
    // Stamina disabled does not suppress a usable Gale counter.
    staminaVisible=false;draw();near(barAlpha,0);near(counterAlpha,1);
    nextStage=true;draw();near(counterAlpha,0);nextStage=false;
    gale=false;draw();near(counterAlpha,0);gale=true;
    counterVisible=false;draw();near(counterAlpha,0);counterVisible=true;
    link.wolf=true;draw();near(counterAlpha,0);link.wolf=false;
    meter.mpKanteraScreen=nullptr;draw();near(counterAlpha,0);
}
'''
helpers = '\n'.join(function(combat,name) for name in ('combat_meter_hud_visible','combat_meter_screen_alpha'))
helpers += '\n'+function(stamina,'menu_or_pause_active')
callbacks = function(stamina,'after_meter_draw')+'\n'+function(counter,'after_draw')
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture.replace('// HELPERS',helpers).replace('// CALLBACKS',callbacks))
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Gale counter visibility passed: shared dialogue/menu gates, HUD fade, scene reset and stamina disabled')
