"""Check that round-button overlays disappear without hiding nested glyphs."""
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
#include <cstddef>
namespace JUtility {
struct TColor { unsigned char r=12,g=34,b=56,a=78; bool operator==(const TColor&) const = default; };
}
struct J2DPane {
    virtual ~J2DPane() = default;
    J2DPane *child=nullptr,*next=nullptr;
    bool visible=true;
    auto* getFirstChildPane(){return child;}
    auto* getNextChildPane(){return next;}
};
struct J2DPicture : J2DPane {
    std::array<JUtility::TColor,4> colors{};
    auto corner(std::size_t i){return colors[i];}
    void setCornerColor(JUtility::TColor a,JUtility::TColor b,JUtility::TColor c,JUtility::TColor d){colors={a,b,c,d};}
};
struct J2DScreen {};
struct CPaneMgr { J2DPane* pane; auto* getPanePtr(){return pane;} };
struct Meter { CPaneMgr* mpButtonXY[3]; CPaneMgr* mpBTextXY[3]; };
struct ModContext {};
enum HookAction {HOOK_CONTINUE};
namespace mods {template<class T> T arg(void* args,int){return *static_cast<T*>(args);}}
bool enabled=true;
bool round_xy_buttons_enabled(){return enabled;}
auto* as_picture(J2DPane* pane){return dynamic_cast<J2DPicture*>(pane);}
Meter* s_hudLayoutMeter=nullptr;
J2DScreen* s_hudLayoutScreen=nullptr;
// STATE
// FUNCTIONS
int main() {
    J2DPane x,y,z;
    J2DPicture baseX,baseY,baseZ,shineX,shineY,shineZ,glyphX,glyphY,glyphZ;
    x.child=&baseX; baseX.next=&shineX; shineX.child=&glyphX;
    y.child=&baseY; baseY.child=&shineY; shineY.next=&glyphY;
    z.child=&baseZ; baseZ.next=&shineZ; shineZ.child=&glyphZ;
    CPaneMgr bx{&x},by{&y},bz{&z},gx{&glyphX},gy{&glyphY},gz{&glyphZ};
    Meter meter{{&bx,&by,&bz},{&gx,&gy,&gz}};
    J2DScreen hud,other;
    auto* screen=&hud;
    s_hudLayoutMeter=&meter; s_hudLayoutScreen=&hud;
    for (int frame=0;frame<100;++frame) {
        // Preserve current alpha/colour from native animation or another mod.
        shineX.colors[1].a=static_cast<unsigned char>(frame);
        const auto savedX=shineX.colors, savedY=shineY.colors;
        const auto savedBase=baseX.colors, savedGlyph=glyphX.colors, savedZ=shineZ.colors;
        enabled=(frame%2)==0;
        before_round_xy_screen_draw(nullptr,&screen,nullptr,nullptr);
        for(auto c:shineX.colors) assert(enabled ? c.a==0 : true);
        for(auto c:shineY.colors) assert(enabled ? c.a==0 : true);
        assert(baseX.colors==savedBase && glyphX.colors==savedGlyph);
        assert(shineZ.colors==savedZ && shineX.visible && glyphX.visible);
        // An unrelated screen cannot restore or inherit the override.
        auto* otherScreen=&other;
        before_round_xy_screen_draw(nullptr,&otherScreen,nullptr,nullptr);
        after_round_xy_screen_draw(nullptr,&otherScreen,nullptr,nullptr);
        if(enabled) assert(shineX.colors[0].a==0);
        after_round_xy_screen_draw(nullptr,&screen,nullptr,nullptr);
        assert(shineX.colors==savedX && shineY.colors==savedY);
        assert(s_roundXYOverlayCount==0);
    }
    enabled=true; meter.mpBTextXY[0]=nullptr; meter.mpButtonXY[1]=nullptr;
    const auto saved=shineX.colors;
    before_round_xy_screen_draw(nullptr,&screen,nullptr,nullptr);
    after_round_xy_screen_draw(nullptr,&screen,nullptr,nullptr);
    assert(shineX.colors==saved); // Incomplete groups are left intact.
}
'''
start = source.index('struct RoundButtonOverlayState')
end = source.index('J2DScreen* s_roundXYOverlayScreen = nullptr;', start)
end += len('J2DScreen* s_roundXYOverlayScreen = nullptr;')
fixture = fixture.replace('// STATE', source[start:end])
fixture = fixture.replace('// FUNCTIONS', '\n'.join(function(name) for name in (
    'first_picture_pane', 'suppress_round_button_overlays',
    'before_round_xy_screen_draw', 'after_round_xy_screen_draw',
)))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Round-button overlays passed: nested glyphs, base/Z preservation, live toggles, restoration')
