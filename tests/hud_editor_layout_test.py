"""Run production HUD transforms against native and HD HUD presentation order.

Run with python3 tests/hud_editor_layout_test.py; no game assets required.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/item_slot_hooks.cpp').read_text()


def function(signature):
    start = source.index(signature + '(')
    return source[start:source.index('\n}', start) + 2]


def declaration(signature):
    start = source.index(signature)
    return source[start:source.index('\n};', start) + 3]


fixture = r'''
#include <array>
#include <cassert>
#include <cmath>
using f32 = float;
using u8 = unsigned char;
struct J2DPane {
    J2DPane* parent = nullptr;
    f32 x=0,y=0,sx=1,sy=1;
    J2DPane* getParentPane(){return parent;}
    f32 getTranslateX(){return x;}
    f32 getTranslateY(){return y;}
    f32 getScaleX(){return sx;}
    f32 getScaleY(){return sy;}
    void translate(f32 a,f32 b){x=a;y=b;}
    void scale(f32 a,f32 b){sx=a;sy=b;}
};
struct J2DPicture : J2DPane {};
struct CPaneMgr {J2DPane* pane;};
J2DPane* pane_ptr(CPaneMgr* p){return p?p->pane:nullptr;}
struct dMeter2Draw_c {CPaneMgr* mpButtonMidona=nullptr;};
struct dMeterMap_c {
    J2DPicture* mMapJ2DPicture;
    f32 mDrawPosX=35,mDrawPosY=250,mSizeW=200,mSizeH=160;
    f32 mSlidePositionOffset=0,mSlidePositionOffsetTarget=-240;
};
struct DuskModHudTransform {
    f32 offset_x=0,offset_y=0,scale=1;
    int slide_direction=1;
};
constexpr int kHudSlideRightToLeft=2;
bool enabled=true;
bool hardcoded_hud_layout_enabled(){return enabled;}
DuskModHudTransform mapTransform,midnaTransform;
auto hud_layout_minimap_transform(){return mapTransform;}
auto hud_layout_midna_transform(){return midnaTransform;}
struct ModContext {};
enum HookAction {HOOK_CONTINUE};
namespace mods {
template<class T> T& arg_ref(void* args,int i){return *static_cast<T*>(static_cast<void**>(args)[i]);}
template<class T> T arg(void* args,int i){return arg_ref<T>(args,i);}
}
// DECLARATIONS
std::array<HudPaneTransformState,static_cast<std::size_t>(HudPaneSlot::Count)> s_wiiUHudPaneTransforms;
MinimapTransformState s_wiiUMinimapTransform;
// FUNCTIONS
void close(f32 a,f32 b){assert(std::fabs(a-b)<0.002f);}
f32 drawn_map_x(dMeterMap_c& map,J2DPicture* picture,f32 safeLeft){
    // Native draw overwrites pre-hook X after advancing presentation slide.
    map.mDrawPosX=35+2*map.mSlidePositionOffset;
    f32 x=safeLeft+map.mDrawPosX;
    void* args[]={&picture,&x};
    before_minimap_picture_draw(nullptr,args,nullptr,nullptr);
    return x;
}
int main(){
    J2DPicture mapPicture,otherPicture;
    dMeterMap_c map{&mapPicture};
    // Both directions, wide-screen safe area, user HUD scale, and HD base layout.
    for(bool hd:{false,true}) for(f32 userScale:{0.75f,1.0f,1.5f})
    for(int direction:{1,2}) for(f32 offset:{-350.0f,0.0f,730.0f}) {
        mapTransform={offset,-40,0.7f,direction};
        for(f32 slide:{0.0f,-12.5f,-125.0f,-240.0f,-125.0f,0.0f}) {
            map.mDrawPosY=250+(hd?30:0);
            map.mSizeW=hd?180:200;map.mSizeH=hd?144:160;
            const auto original=map;
            apply_wii_u_minimap_layout(&map);
            map.mSlidePositionOffset=slide; // advanced INSIDE the native draw
            close(drawn_map_x(map,&mapPicture,90),125+offset+(direction==1?2:-2)*slide);
            close(drawn_map_x(map,&otherPicture,90),125+2*slide);
            close(map.mDrawPosY,original.mDrawPosY-40);
            close(map.mSizeW*userScale,original.mSizeW*0.7f*userScale);
            close(map.mSizeH*userScale,original.mSizeH*0.7f*userScale);
            restore_wii_u_minimap_layout(&map);
            close(map.mDrawPosY,original.mDrawPosY);
            close(map.mSizeW,original.mSizeW);close(map.mSizeH,original.mSizeH);
            close(map.mSlidePositionOffset,slide);
            close(map.mSlidePositionOffsetTarget,-240);
            close(map.mDrawPosX,35+2*slide); // do not rewind native interpolation
            close(drawn_map_x(map,&mapPicture,90),125+2*slide);
        }
    }
    enabled=false;apply_wii_u_minimap_layout(&map);
    assert(!s_wiiUMinimapTransform.active);
    close(drawn_map_x(map,&mapPicture,0),map.mDrawPosX);
    apply_wii_u_minimap_layout(nullptr);restore_wii_u_minimap_layout(nullptr);

    // A child already inherits its primary picture's transform. HD reparents
    // it as a sibling, preserving independent scales (including bow combos).
    constexpr std::array primaries{HudPaneSlot::ItemX,HudPaneSlot::ItemY,HudPaneSlot::ItemZ};
    constexpr std::array secondaries{HudPaneSlot::ItemXSecondary,HudPaneSlot::ItemYSecondary,HudPaneSlot::ItemZSecondary};
    J2DPane parent,primary,secondary;
    primary.parent=&parent;
    for(int slot=0;slot<3;++slot) for(bool sibling:{false,true})
    for(bool combo:{false,true}) {
        secondary.parent=sibling?&parent:&primary;
        const f32 primaryScale=combo?0.6f:1;
        for(int frame=0;frame<1000;++frame){
            enabled=frame%11!=0;
            restore_hud_layout_base(); // before HD snapshots / presentation
            primary.translate(20,30);primary.scale(primaryScale,primaryScale);
            secondary.translate(sibling?20:0,sibling?30:0);secondary.scale(1,1);
            f32 dx=(frame%9-4)*25,dy=(frame%7-3)*17,scale=1.25f;
            apply_hud_pane_transform(hud_pane_state(primaries[slot]),&primary,enabled,dx,dy,scale);
            apply_hud_item_secondary_transform(secondaries[slot],&primary,&secondary,enabled,dx,dy,scale);
            close(primary.x,20+(enabled?dx:0));close(primary.y,30+(enabled?dy:0));
            close(primary.sx,primaryScale*(enabled?scale:1));
            close(secondary.x,sibling?primary.x:0);close(secondary.y,sibling?primary.y:0);
            close(secondary.sx,sibling&&enabled?scale:1);
            restore_hud_layout_base();
            close(primary.x,20);close(primary.sx,primaryScale);
            close(secondary.x,sibling?20:0);close(secondary.sx,1);
        }
    }
    apply_hud_item_secondary_transform(secondaries[0],nullptr,nullptr,true,1,2,1);
    // Native, Dawnlight D-pad, and HD anchors: X/Y/scale remain relative to the
    // current owner and restore without drift, including live preset changes.
    J2DPane midna;CPaneMgr manager{&midna};dMeter2Draw_c meter{&manager};
    for(f32 baseX:{0.0f,-18.0f,40.0f}) for(int frame=0;frame<1000;++frame){
        enabled=frame%11!=0;
        restore_hud_layout_base();
        midna.translate(baseX,30);midna.scale(0.9f,0.9f);
        midnaTransform={float(frame%9-4)*50,float(frame%7-3)*25,1.5f,1};
        apply_midna_hud_layout(&meter);
        close(midna.x,baseX+(enabled?midnaTransform.offset_x:0));
        close(midna.y,30+(enabled?midnaTransform.offset_y:0));
        close(midna.sx,0.9f*(enabled?1.5f:1));
        restore_hud_layout_base();close(midna.x,baseX);close(midna.y,30);close(midna.sx,0.9f);
    }
    apply_midna_hud_layout(nullptr);meter.mpButtonMidona=nullptr;apply_midna_hud_layout(&meter);
}
'''
names = ['bool nearly_equal', 'HudPaneTransformState& hud_pane_state',
         'void remove_applied_hud_pane_transform', 'void restore_applied_hud_pane_transform',
         'void restore_hud_layout_base', 'void apply_hud_pane_transform',
         'void apply_hud_item_secondary_transform', 'void apply_midna_hud_layout',
         'void apply_wii_u_minimap_layout', 'void restore_wii_u_minimap_layout',
         'HookAction before_minimap_picture_draw']
functions = '\n'.join(function(n) for n in names)
# Include the CPaneMgr overload used by the actual Midna call.
start = source.index('void apply_hud_pane_transform(const HudPaneSlot slot, CPaneMgr*')
overload = source[start:source.index('\n}', start) + 2]
functions = functions.replace('void apply_midna_hud_layout', overload+'\nvoid apply_midna_hud_layout')
fixture = fixture.replace('// DECLARATIONS', '\n'.join(declaration(n) for n in
    ['struct HudPaneTransformState {', 'enum class HudPaneSlot', 'struct MinimapTransformState {']))
fixture = fixture.replace('// FUNCTIONS', functions)
# Verify the tested helpers are wired into the final HUD pass for all slots.
layout = function('void apply_wii_u_hud_layout')
assert 'apply_midna_hud_layout(meter)' in layout
for slot in 'XYZ':
    assert f'apply_hud_item_secondary_transform(HudPaneSlot::Item{slot}Secondary' in layout
assert 'hud_layout_z_' not in function('void layout_z_hud_item')
assert 'hud_layout_midna_transform' not in function('void move_midna_hud_to_dpad')
assert 'hook_add_pre<MinimapPictureDrawHook>' in source
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    exe = Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('HUD editor regression passed: minimap X/slide, Midna, and X/Y/Z item layers')
