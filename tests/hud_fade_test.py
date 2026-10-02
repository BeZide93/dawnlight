"""Exercise production idle timing, render scoping and bar fade callbacks without a game SDK."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/hud_fade.cpp').read_text()

def function(name, source=source):
    start = re.search(r'^\w[^\n]*\b' + name + r'\([^;{}]*\)\s*\{', source, re.M).start()
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

fixture = r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>
#include "hud_fade_state.hpp"
using namespace dawnlight;
using u8=uint8_t;using s16=int16_t;
struct ModContext {};
enum HookAction {HOOK_CONTINUE};
struct cXyz {float x=0,y=0,z=0; cXyz operator-(cXyz p){return {x-p.x,y-p.y,z-p.z};}
    float abs2() const {return x*x+y*y+z*z;}};
struct daAlink_c {
    enum {PROC_WAIT,PROC_SERVICE_WAIT,PROC_TIRED_WAIT,
          PROC_WOLF_WAIT,PROC_WOLF_SERVICE_WAIT,PROC_WOLF_TIRED_WAIT,
          PROC_ATTACK,PROC_AIM,PROC_DAMAGE};
    struct {cXyz pos;struct {s16 y=0;} angle;} current;
    int mProcID=PROC_WAIT;
} link;
daAlink_c* player=&link;
bool autoFade=true,visible=true,nextStage=false,custom=true,staminaFade=true,deityFade=true;
daAlink_c* daAlink_getAlinkActorClass(){return player;}
bool hud_auto_fade_enabled(){return autoFade;}
bool combat_meter_hud_visible(){return visible;}
bool dComIfGp_isEnableNextStage(){return nextStage;}
bool custom_hud_layout_enabled(){return custom;}
bool hud_custom_stamina_fade_when_full(){return staminaFade;}
bool hud_custom_fierce_deity_fade_when_empty(){return deityFade;}
double now=0;
double seconds(){return now;}
struct J2DPane {
    u8 alpha=255,mColorAlpha=255;
    bool influenced=true;
    J2DPane *child=nullptr,*next=nullptr;
    bool isInfluencedAlpha(){return influenced;}
    u8 getAlpha(){return alpha;}
    void setAlpha(u8 a){alpha=a;}
    J2DPane* getFirstChildPane(){return child;}
    J2DPane* getNextChildPane(){return next;}
};
struct J2DGrafContext {};
struct DawnlightFierceDeityHudFrame {float alpha=1;};
bool replacementHandled=false;float replacementAlpha=-1;int replacementCalls=0;
bool draw_external_fierce_deity_hud(const DawnlightFierceDeityHudFrame&);
void draw_combat_meter_screen(struct J2DScreen*,J2DGrafContext*,bool,float,
    const DawnlightFierceDeityHudFrame* = nullptr);
struct J2DScreen:J2DPane {float rendered=1;void draw(float,float,J2DGrafContext*);};
struct J2DPicture:J2DPane {};
namespace mods {template<class T>T arg(void* a,int){return static_cast<T>(a);}}
void shutdown_hud_fade();
struct dMeter2Draw_c {};
enum class CombatMeterStyle {FierceDeity};
struct {float meter=0;} s_state;
bool deityEnabled=true;
bool fierce_deity_enabled(){return deityEnabled;}
bool same_link(daAlink_c* p){return p!=nullptr;}
bool menu_or_pause_active(){return !visible;}
bool stamina_meter_visible(){return true;}
int meterDraws=0;
void draw_combat_meter(dMeter2Draw_c*,float,CombatMeterStyle,int){++meterDraws;}
// STATE
// FUNCTIONS
bool draw_external_fierce_deity_hud(const DawnlightFierceDeityHudFrame& frame) {
    assert(s_hudDepth==0 && s_barAlpha==1); // final alpha must not get applied twice
    replacementAlpha=frame.alpha;++replacementCalls;return replacementHandled;
}
void J2DScreen::draw(float,float,J2DGrafContext*) {
    before_screen(nullptr,this,nullptr,nullptr);
    rendered=alpha/255.0f;
    after_screen(nullptr,this,nullptr,nullptr);
}
void near(float a,float b,float eps=0.012f){assert(std::fabs(a-b)<eps);}
void tick(double t){now=t;before_hud(nullptr,nullptr,nullptr,nullptr);after_hud(nullptr,nullptr,nullptr,nullptr);}
int main(){
    // Even zero charge must reach the fade renderer: Off keeps the empty frame
    // visible; On needs successive draws to finish its animation.
    dMeter2Draw_c meter;
    draw_fierce_meter(&meter);assert(meterDraws==1);
    visible=false;draw_fierce_meter(&meter);assert(meterDraws==1);visible=true;
    deityEnabled=false;draw_fierce_meter(&meter);assert(meterDraws==1);deityEnabled=true;
    // Three seconds visible, one-second fade at different rendering rates.
    for(int fps:{20,30,60,144}) {
        shutdown_hud_fade();
        for(int i=0;i<=4*fps;++i){
            tick(double(i)/fps);
            if(i<=3*fps) near(s_hudAlpha,1);
            if(i==4*fps) near(s_hudAlpha,0);
        }
        // Stationary combat actions reveal the HUD within 0.15 seconds.
        link.mProcID=daAlink_c::PROC_ATTACK;
        for(int i=1;i<=fps;++i) tick(4+double(i)/fps);
        near(s_hudAlpha,1);link.mProcID=daAlink_c::PROC_WAIT;
    }
    shutdown_hud_fade();
    for(int i=0;i<=240;++i) tick(i/60.0);
    near(s_hudAlpha,0);
    link.current.pos.x+=1;
    tick(4.1);tick(4.2);near(s_hudAlpha,1);
    for(int i=1;i<=240;++i) tick(4.2+i/60.0);
    near(s_hudAlpha,0);
    // Dialogues, disabled option, scene transitions and long clock gaps reveal/reset.
    visible=false;tick(8.21);near(s_hudAlpha,1);visible=true;
    nextStage=true;tick(8.22);near(s_hudAlpha,1);nextStage=false;
    autoFade=false;tick(8.23);near(s_hudAlpha,1);autoFade=true;
    tick(100);near(s_hudAlpha,1);
    player=nullptr;tick(100.1);near(s_hudAlpha,1);player=&link;
    // Rotation and wolf movement also restart the timer.
    shutdown_hud_fade();link.mProcID=daAlink_c::PROC_WOLF_WAIT;
    for(int i=0;i<=240;++i) tick(i/60.0);
    near(s_hudAlpha,0);++link.current.angle.y;tick(4.1);tick(4.2);near(s_hudAlpha,1);

    // Idle gestures and low-health breathing preserve both the countdown and
    // an already hidden HUD, in human and wolf form. Include the return to wait.
    for (auto wait : {daAlink_c::PROC_WAIT, daAlink_c::PROC_WOLF_WAIT}) {
        const bool wolf = wait == daAlink_c::PROC_WOLF_WAIT;
        const auto gesture = wolf ? daAlink_c::PROC_WOLF_SERVICE_WAIT : daAlink_c::PROC_SERVICE_WAIT;
        const auto tired = wolf ? daAlink_c::PROC_WOLF_TIRED_WAIT : daAlink_c::PROC_TIRED_WAIT;
        for (auto animation : {gesture, tired}) {
            shutdown_hud_fade(); link.mProcID = wait;
            for (int i = 0; i <= 120; ++i) tick(i / 60.0);
            link.mProcID = animation;
            for (int i = 121; i <= 240; ++i) tick(i / 60.0);
            near(s_hudAlpha, 0); // The switch at 2s did not restart the 3s delay.
            link.mProcID = wait;
            for (int i = 241; i <= 300; ++i) { tick(i / 60.0); near(s_hudAlpha, 0); }
            link.mProcID = animation;
            for (int i = 301; i <= 420; ++i) { tick(i / 60.0); near(s_hudAlpha, 0); }
            // Real displacement/rotation still wakes the HUD in an idle proc.
            link.current.pos.x += 1;
            tick(7.1); tick(7.2); near(s_hudAlpha, 1);
            for (int i = 1; i <= 240; ++i) tick(7.2 + i / 60.0);
            near(s_hudAlpha, 0);
            ++link.current.angle.y;
            tick(11.3); tick(11.4); near(s_hudAlpha, 1);
            // Attacks, aiming and damage must also wake it without displacement.
            for (auto action : {daAlink_c::PROC_ATTACK, daAlink_c::PROC_AIM, daAlink_c::PROC_DAMAGE}) {
                shutdown_hud_fade(); link.mProcID = animation;
                for (int i = 0; i <= 240; ++i) tick(i / 60.0);
                near(s_hudAlpha, 0); link.mProcID = action;
                tick(4.1); tick(4.2); near(s_hudAlpha, 1);
            }
        }
    }

    // Alpha inheritance: apply the fade once, including branches opting out of
    // parent alpha. Preserve native fading and leave the next draw untouched.
    shutdown_hud_fade();
    J2DScreen screen;J2DPane inherited,independent;
    screen.alpha=200;screen.child=&inherited;inherited.child=&independent;
    inherited.alpha=128;independent.alpha=150;independent.influenced=false;
    s_hudDepth=1;s_hudAlpha=0.5f;
    before_screen(nullptr,&screen,nullptr,nullptr);
    assert(screen.alpha==100&&inherited.alpha==128&&independent.alpha==75);
    J2DScreen skipped;
    after_screen(nullptr,&skipped,nullptr,nullptr);assert(screen.alpha==100&&s_screens.size()==1);
    J2DScreen nested;before_screen(nullptr,&nested,nullptr,nullptr);
    assert(nested.alpha==127);after_screen(nullptr,&nested,nullptr,nullptr);
    assert(nested.alpha==255&&screen.alpha==100);
    after_screen(nullptr,&screen,nullptr,nullptr);
    assert(screen.alpha==200&&inherited.alpha==128&&independent.alpha==150);
    assert(s_screens.empty());
    // Standalone minimap/ammo pictures use the same alpha and are restored.
    J2DPicture picture;picture.alpha=128;
    before_picture(nullptr,&picture,nullptr,nullptr);assert(picture.alpha==64);
    after_picture(nullptr,&picture,nullptr,nullptr);assert(picture.alpha==128);
    // Screens and pictures outside HUD drawing (collection/menu/dialogue) are untouched.
    s_hudDepth=0;
    before_screen(nullptr,&screen,nullptr,nullptr);assert(screen.alpha==200);
    after_screen(nullptr,&screen,nullptr,nullptr);
    before_picture(nullptr,&picture,nullptr,nullptr);assert(picture.alpha==128);
    after_picture(nullptr,&picture,nullptr,nullptr);

    // Same screen reused for both bars: local fading must not leak between draws.
    screen.alpha=255;screen.child=nullptr;shutdown_hud_fade();
    for(int i=0;i<=60;++i) {
        now=i/60.0;
        draw_combat_meter_screen(&screen,nullptr,true,100);
        draw_combat_meter_screen(&screen,nullptr,false,0);
    }
    near(screen.rendered,0);assert(screen.alpha==255&&s_barAlpha==1);
    now+=0.15;draw_combat_meter_screen(&screen,nullptr,true,99);near(screen.rendered,1);
    draw_combat_meter_screen(&screen,nullptr,false,0);near(screen.rendered,0);
    now+=0.15;draw_combat_meter_screen(&screen,nullptr,false,1);near(screen.rendered,1);
    s_hudDepth=1;s_hudAlpha=0.5f;
    draw_combat_meter_screen(&screen,nullptr,true,50);near(screen.rendered,0.5f);
    s_hudDepth=0;
    // Built-in presets ignore saved Custom fade options; Off immediately restores the bar.
    for(int i=1;i<=60;++i){now+=1.0/60;draw_combat_meter_screen(&screen,nullptr,true,100);}
    near(screen.rendered,0);custom=false;
    draw_combat_meter_screen(&screen,nullptr,true,100);near(screen.rendered,1);
    custom=true;staminaFade=false;
    draw_combat_meter_screen(&screen,nullptr,true,100);near(screen.rendered,1);
    shutdown_hud_fade();
    s_hudDepth=1;s_hudAlpha=0.5f;
    DawnlightFierceDeityHudFrame replacement{0.4f};
    screen.rendered=-1;replacementHandled=true;
    draw_combat_meter_screen(&screen,nullptr,false,50,&replacement);
    near(replacementAlpha,0.2f);near(screen.rendered,-1);assert(replacementCalls==1);
    assert(s_hudDepth==1 && s_barAlpha==1);
    replacementHandled=false;
    draw_combat_meter_screen(&screen,nullptr,false,50,&replacement);
    near(screen.rendered,0.5f);assert(replacementCalls==2);
    replacementHandled=true;
    for(int i=0;i<=60;++i){now+=1.0/60;draw_combat_meter_screen(&screen,nullptr,false,0,&replacement);}
    near(replacementAlpha,0); // empty-bar fade keeps ticking while the built-in draw is suppressed
    now+=0.15;draw_combat_meter_screen(&screen,nullptr,false,1,&replacement);
    near(replacementAlpha,0.2f);
    const int callsBeforeStamina=replacementCalls;
    // Even with a replacement argument, Stamina is never offered to the consumer.
    draw_combat_meter_screen(&screen,nullptr,true,50,&replacement);assert(replacementCalls==callsBeforeStamina);
    shutdown_hud_fade();assert(s_player==nullptr&&s_hudDepth==0);near(s_hudAlpha,1);
}
'''
state = source[source.index('HudIdleFade s_idle;'):source.index('double seconds()')]
names = ('update_idle','fade_tree','restore_alpha','before_hud','after_hud',
         'before_screen','after_screen','before_picture','after_picture',
         'draw_combat_meter_screen','shutdown_hud_fade')
fixture = fixture.replace('// FUNCTIONS', function('draw_fierce_meter', (root/'src/fierce_deity.cpp').read_text())+'\n// FUNCTIONS')
fixture = fixture.replace('// STATE',state).replace('// FUNCTIONS','\n'.join(function(n) for n in names))
with tempfile.TemporaryDirectory() as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I'+str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('HUD fades passed: timing, wake/reset, alpha inheritance/restoration, map/ammo, independent bars and presets')
