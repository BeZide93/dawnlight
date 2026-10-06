"""Exercise interaction routing and A/B layer gating from the production HUD code."""
from pathlib import Path
import subprocess
import tempfile

source = (Path(__file__).resolve().parents[1] / 'src/item_slot_hooks.cpp').read_text()

def function(name):
    start = source.index(name + '(')
    start = source.rfind('\n', 0, start) + 1
    return source[start:source.index('\n}', start) + 2]

fixture = r'''
#include <array>
#include <cassert>
#include <cmath>
using f32 = float;
struct Pane {};
struct dMeter2Draw_c {
    Pane *mpButtonA=nullptr, *mpTextA=nullptr, *mpButtonB=nullptr;
    Pane *mpItemB=nullptr, *mpLightB=nullptr, *mpTextB=nullptr;
    Pane *mpAText[5]{}, *mpBText[5]{};
};
struct dMeter2_c {
    dMeter2Draw_c* draw=nullptr;
    unsigned showFlags=0;
    float mButtonATalkPosX[2]{}, mButtonATalkPosY[2]{};
    float field_0x148[2]{}, field_0x150[2]{};
    struct Targets {
        float buttonAX[2]{}, buttonAY[2]{}, buttonBX[2]{}, buttonBY[2]{};
        bool ready=true;
    } mPresentationTargets;
    auto* getMeterDrawPtr(){return draw;}
    bool isShowFlag(int i){return showFlags & (1u<<i);}
};
struct Link {
    bool grass=false, hawk=false;
    bool checkGrassWhistle(){return grass;}
    bool checkHawkWait(){return hawk;}
};
dMeter2_c* owner=nullptr;
Link* player=nullptr;
bool shop=false, explain=false, s_interactionPromptLayout=false;
auto* dMeter2Info_getMeterClass(){return owner;}
auto* daAlink_getAlinkActorClass(){return player;}
bool dMeter2Info_isShopTalkFlag(){return shop;}
bool dMeter2Info_getItemExplainWindowStatus(){return explain;}
struct DuskModHudTransform { float offset_x=250, offset_y=-125, scale=1.5f; };
struct DuskModHudButtonLayout {
    float text_offset_x=10, text_offset_y=20, text_scale=0.8f;
    float item_offset_x=30, item_offset_y=40, item_scale=0.5f;
    int text_anchor=1;
};
struct HudLayoutOffset {float x=0,y=0;};
DuskModHudTransform hud_layout_a_transform(){return {};}
DuskModHudTransform hud_layout_b_transform(){return {};}
DuskModHudButtonLayout hud_layout_a_button_layout(){return {};}
DuskModHudButtonLayout hud_layout_b_button_layout(){return {};}
int hud_button_b_item_variant(dMeter2Draw_c*){return 1;}
HudLayoutOffset hud_item_anchor_delta(const DuskModHudButtonLayout&, float, float){return {};}
struct {float mButtonBItemPosX[3]{},mButtonBItemPosY[3]{};} g_drawHIO;
enum class HudPaneSlot {ButtonA,TextA,ButtonB,ItemB,LightB,TextB,Count};
std::array<int,6> transforms{},bindings{};
void apply_hud_pane_transform(HudPaneSlot slot,Pane*,bool enabled,float,float,float){
    transforms[static_cast<int>(slot)]=enabled;
}
void apply_hud_text_pane_transform(HudPaneSlot slot,Pane* pane,bool enabled,
                                  float x,float y,float scale,int){
    apply_hud_pane_transform(slot,pane,enabled,x,y,scale);
}
void apply_hud_text_box_group_binding(HudPaneSlot slot,Pane* const*,int,bool enabled,int){
    bindings[static_cast<int>(slot)]=enabled;
}
// PRODUCTION
void check_layout(dMeter2Draw_c* meter, bool enabled, bool expected){
    transforms.fill(-1); bindings.fill(-1);
    apply_hud_action_button_layout(meter, enabled && !hud_interaction_prompts_active(meter));
    // Every A/B layer must receive the same decision, including item/light and
    // label alignment. Other HUD elements are outside this function.
    for(int value:transforms) assert(value == int(expected));
    assert(bindings[int(HudPaneSlot::TextA)] == int(expected));
    assert(bindings[int(HudPaneSlot::TextB)] == int(expected));
}
int main(){
    dMeter2Draw_c draw, other;
    dMeter2_c hud; hud.draw=&draw; owner=&hud;
    Link link; player=&link;
    // Repeat normal gameplay -> native prompt -> returning animation -> gameplay.
    // A-only conversation, shop A/B, B-only, grass, hawk, and item explanation.
    for(int frame=0;frame<200;++frame) for(int scenario=0;scenario<7;++scenario){
        check_layout(&draw,true,true);
        hud.showFlags=scenario==0?1:scenario==1?3:scenario==2?2:0;
        shop=scenario==3;link.grass=scenario==4;link.hawk=scenario==5;explain=scenario==6;
        check_layout(&draw,true,false);
        // Switching layouts off during the interaction must restore all layers too.
        check_layout(&draw,false,false);
        hud.showFlags=0;shop=explain=link.grass=link.hawk=false;
        // Both icon and label axes must finish returning before editor offsets resume.
        for(float* axis:{hud.mButtonATalkPosX,hud.mButtonATalkPosY,hud.field_0x148,hud.field_0x150}){
            for(int i=0;i<2;++i){axis[i]=5;check_layout(&draw,true,false);axis[i]=0;}
        }
        check_layout(&draw,true,true);
        check_layout(&draw,false,false);
    }
    // Nonzero engine targets (wolf/light vessel) are valid gameplay positions.
    hud.showFlags=1;check_layout(&draw,true,false);hud.showFlags=0;
    hud.field_0x148[0]=hud.mPresentationTargets.buttonBX[0]=45;
    check_layout(&draw,true,true);
    // No stale interaction state from another meter or a removed scene.
    hud.showFlags=1;check_layout(&draw,true,false);
    check_layout(&other,true,true);assert(!s_interactionPromptLayout);
    owner=nullptr;check_layout(&draw,true,true);
    owner=&hud;hud.showFlags=0;player=nullptr;check_layout(&draw,true,true);
}
'''
names = ['bool nearly_equal', 'f32 hud_item_scale', 'f32 hud_text_scale',
         'bool hud_interaction_prompts_active', 'void apply_hud_action_button_layout']
fixture = fixture.replace('// PRODUCTION', '\n'.join(function(name) for name in names))
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp'; exe=Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('HUD interaction regression passed: A/B layers, native prompts, transitions, and scene changes')
