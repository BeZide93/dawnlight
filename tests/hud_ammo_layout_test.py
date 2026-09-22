"""Exercise production HUD hooks with Twilight HD HUD's screen/post-draw order.

Run with python3 tests/hud_ammo_layout_test.py; no game assets required.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/item_slot_hooks.cpp').read_text()

def function(name):
    start = source.index(name + '(')
    start = source.rfind('\n', 0, start) + 1
    end = source.index('\n}', start) + 2
    return source[start:end]

fixture = r'''
#include <array>
#include <cassert>
#include <cmath>
using f32 = float;
using u8 = unsigned char;
namespace JGeometry {
template<class T> struct TVec2 { T x=0,y=0; TVec2()=default; TVec2(T x,T y):x(x),y(y){} };
template<class T> struct TBox2 {
    TVec2<T> i,f;
    void addPos(TVec2<T> v){i.x+=v.x;i.y+=v.y;f.x+=v.x;f.y+=v.y;}
};
}
struct J2DPane {
    JGeometry::TBox2<f32> mGlobalBounds{{400,50},{440,90}};
    const auto& getGlbBounds() const {return mGlobalBounds;}
};
struct CPaneMgr { J2DPane pane; J2DPane* getPanePtr(){return &pane;} };
struct J2DScreen {};
struct dMeter2Draw_c {
    enum {SELECT_X_e,SELECT_Y_e,SELECT_Z_e};
    struct item_params {f32 num_pos_x=-4,num_pos_y=2,num_scale=1;};
    item_params mItemParams[3]; CPaneMgr *mpItemXY[2],*mpItemR;
};
struct DuskModHudTransform {f32 scale=1;};
struct DuskModHudButtonLayout {f32 item_scale=1,ammo_scale=1,ammo_offset_x=0,ammo_offset_y=0;};
DuskModHudTransform transforms[3]; DuskModHudButtonLayout layouts[3];
auto hud_layout_xy_transform(int i){return transforms[i];}
auto hud_layout_xy_button_layout(int i){return layouts[i];}
auto hud_layout_z_transform(){return transforms[2];}
auto hud_layout_z_button_layout(){return layouts[2];}
bool enabled=true,ownedZ=false,ammo=true;
bool hardcoded_hud_layout_enabled(){return enabled;}
bool z_item_slot_active(){return ownedZ;}
bool z_item_has_ammo(u8){return ammo;}
u8 dComIfGp_getSelectItem(int){return 1;}
constexpr int kZItemSlot=2;
std::array<dMeter2Draw_c::item_params,2> s_xyAmmoOriginalParams;
std::array<bool,2> s_xyAmmoOriginalValid{};
// STATE
struct ModContext {};
enum HookAction {HOOK_CONTINUE};
namespace mods {template<class T> T arg(void* args,int){return *static_cast<T*>(args);}}
dMeter2Draw_c* s_hudLayoutMeter=nullptr; J2DScreen* s_hudLayoutScreen=nullptr;
void update_z_hud_item(dMeter2Draw_c*){}
void apply_round_xy_buttons(dMeter2Draw_c*){}
void restore_hud_layout_base(){}
void apply_wii_u_hud_layout(dMeter2Draw_c*){}
void apply_hud_backing_visibility(dMeter2Draw_c*){}
// FUNCTIONS
void close(float a,float b){assert(std::fabs(a-b)<0.001f);}
int main(){
    CPaneMgr x,y,z; dMeter2Draw_c meter{{},{&x,&y},&z};
    auto* m=&meter; J2DScreen screen; auto* s=&screen;
    s_hudLayoutMeter=m; s_hudLayoutScreen=s;
    const auto base=m->mItemParams[0]; const auto zBounds=z.pane.mGlobalBounds;
    // Repeated presentation draws (without a simulation update) used to retain
    // Dawnlight's edited values in HD HUD's snapshot and compound each frame.
    for(bool hd:{false,true}) for(int frame=0;frame<1000;++frame){
        enabled=frame%11!=0;
        for(int i=0;i<3;++i){
            transforms[i].scale=1.25f;
            layouts[i]={0.8f,1.5f,float((frame%9-4)*25+i),float((frame%7-3)*17-i)};
        }
        before_meter_draw(nullptr,&m,nullptr,nullptr);
        before_meter_screen_draw_restore_hud(nullptr,&s,nullptr,nullptr);
        const std::array native{m->mItemParams[0],m->mItemParams[1]};
        if(hd) for(int i=0;i<2;++i){
            // HD HUD snapshots first, then authors its base at ScreenDraw.
            auto& p=m->mItemParams[i];
            p.num_pos_x=(p.num_pos_x+(i==0?7:-1))*0.9f;
            p.num_pos_y=(p.num_pos_y+(i==0?1:6))*0.9f;
            p.num_scale*=0.55f*0.9f;
        }
        const std::array hdBase{m->mItemParams[0],m->mItemParams[1]};
        before_meter_screen_draw_apply_hud(nullptr,&s,nullptr,nullptr);
        for(int i=0;i<2;++i){
            close(m->mItemParams[i].num_pos_x,hdBase[i].num_pos_x+(enabled?layouts[i].ammo_offset_x:0));
            close(m->mItemParams[i].num_pos_y,hdBase[i].num_pos_y+(enabled?layouts[i].ammo_offset_y:0));
            close(m->mItemParams[i].num_scale,hdBase[i].num_scale*(enabled?1.5f:1));
        }
        after_meter_draw_restore_xy_ammo(nullptr,&m,nullptr,nullptr);
        for(int i=0;i<2;++i){
            close(m->mItemParams[i].num_pos_x,hdBase[i].num_pos_x);
            if(hd) m->mItemParams[i]=native[i];
            close(m->mItemParams[i].num_pos_x,base.num_pos_x);
            close(m->mItemParams[i].num_scale,base.num_scale);
        }
        // External Z uses the drawn item's bottom-right corner for 2/3 digits.
        for(int digits:{2,3}){
            const float size=m->mItemParams[2].num_scale*16*0.55f;
            const auto bounds=z.pane.getGlbBounds();
            close(bounds.f.x-size*digits,zBounds.f.x+(enabled?layouts[2].ammo_offset_x:0)-16*0.55f*(enabled?1.5f:1)*digits);
            close(bounds.f.y,zBounds.f.y+(enabled?layouts[2].ammo_offset_y:0));
        }
        after_meter_draw_restore_external_z_ammo(nullptr,nullptr,nullptr,nullptr);
        close(m->mItemParams[2].num_scale,1);
        close(z.pane.mGlobalBounds.f.x,zBounds.f.x);
        close(z.pane.mGlobalBounds.f.y,zBounds.f.y);
    }
    // No adjustments for Dawnlight's own Z renderer, non-ammo items, or no pane.
    enabled=true;
    for(int scenario=0;scenario<3;++scenario){
        ownedZ=scenario==0;ammo=scenario!=1;m->mpItemR=scenario==2?nullptr:&z;
        apply_external_z_ammo_layout(m);assert(s_externalZAmmoDraw.meter==nullptr);
    }
    apply_external_z_ammo_layout(nullptr);restore_external_z_ammo_layout();
}
'''
start = source.index('struct ExternalZAmmoDrawState {')
state = source[start:source.index('ExternalZAmmoDrawState s_externalZAmmoDraw;', start) + len('ExternalZAmmoDrawState s_externalZAmmoDraw;')]
names = ['f32 hud_ammo_scale', 'void apply_xy_ammo_layout', 'void restore_xy_ammo_layout',
         'void apply_external_z_ammo_layout', 'void restore_external_z_ammo_layout',
         'HookAction before_meter_draw', 'void after_meter_draw_restore_xy_ammo',
         'void after_meter_draw_restore_external_z_ammo',
         'HookAction before_meter_screen_draw_restore_hud', 'HookAction before_meter_screen_draw_apply_hud']
fixture = fixture.replace('// STATE', state).replace('// FUNCTIONS', '\n'.join(function(n) for n in names))
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    exe = Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('HUD ammo regression passed: X/Y/Z positions, scales, restoration, and 2,000 presentation draws')
