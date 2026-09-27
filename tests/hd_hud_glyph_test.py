"""Exercise production HD HUD glyph suppression at the final screen draw boundary."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/item_slot_hooks.cpp').read_text()

def function(name):
    start = source.rfind('\n', 0, source.index(name + '(')) + 1
    return source[start:source.index('\n}', start) + 2]

fixture = r'''
#include <array>
#include <cassert>
#include <string_view>
struct ModContext {};
ModContext* mod_ctx = nullptr;
constexpr int MOD_OK = 0;
enum HookAction {HOOK_CONTINUE};
namespace dusk::config {
struct ConfigVarBase {};
template<class T> struct ConfigVar : ConfigVarBase {
    T value = false;
    T getValue() const {return value;}
};
}
using ZSlotGetConfigVarFn = dusk::config::ConfigVarBase* (*)(std::string_view);
dusk::config::ConfigVar<bool> hdEnabled;
bool registered = true, resolveFails = false, nullAddress = false;
int resolves = 0;
dusk::config::ConfigVarBase* getVar(std::string_view key) {
    assert(key == "mod.org_twilight_hd__hud.enabled");
    return registered ? &hdEnabled : nullptr;
}
int resolve(ModContext*, const char* name, void** address, void*) {
    assert(std::string_view(name) == "dusk::config::GetConfigVar");
    ++resolves;
    *address = nullAddress ? nullptr : reinterpret_cast<void*>(&getVar);
    return resolveFails ? 1 : MOD_OK;
}
using ResolveFn = decltype(&resolve);
struct HookService {ResolveFn resolve;};
HookService hook{resolve};
HookService* svc_hook = &hook;
struct J2DPane {
    bool visible = true;
    float x = 0, y = 0;
    bool isVisible() const {return visible;}
    void show() {visible = true;}
    void hide() {visible = false;}
};
struct CPaneMgr {J2DPane* pane;};
J2DPane* pane_ptr(CPaneMgr* pane) {return pane ? pane->pane : nullptr;}
struct J2DScreen {};
struct Meter {CPaneMgr* mpBTextA; CPaneMgr* mpBTextB; CPaneMgr* mpBTextXY[3];};
Meter* s_hudLayoutMeter = nullptr;
J2DScreen* s_hudLayoutScreen = nullptr;
namespace mods {template<class T> T arg(void* args, int) {return *static_cast<T*>(args);}}
// STATE
// FUNCTIONS
int main() {
    J2DScreen hud, menu;
    auto* screen = &hud;
    auto* nested = &hud;
    auto* unrelated = &menu;
    J2DPane a,b,x,y,z,disc,action;
    CPaneMgr ma{&a},mb{&b},mx{&x},my{&y},mz{&z};
    Meter meter{&ma,&mb,{&mx,&my,&mz}};
    s_hudLayoutMeter = &meter; s_hudLayoutScreen = &hud;
    hdEnabled.value = true;
    for (int frame = 0; frame < 100; ++frame) {
        // Native updates may show the letters again every frame. Suppression
        // is independent of how far the user's layout moved their parents.
        a.show(); b.show(); x.show(); y.visible = frame % 2;
        a.x = -1000 + frame * 20; a.y = frame;
        const bool oldY = y.visible;
        before_hd_hud_glyph_draw(nullptr,&screen,nullptr,nullptr);
        assert(!a.visible && !b.visible && !x.visible && !y.visible);
        assert(z.visible && disc.visible && action.visible);
        assert(a.x == -1000 + frame * 20 && a.y == frame);
        before_hd_hud_glyph_draw(nullptr,&unrelated,nullptr,nullptr);
        after_hd_hud_glyph_draw(nullptr,&unrelated,nullptr,nullptr);
        before_hd_hud_glyph_draw(nullptr,&nested,nullptr,nullptr);
        after_hd_hud_glyph_draw(nullptr,&nested,nullptr,nullptr);
        assert(!a.visible && !x.visible); // nested/skipped draw cannot restore the outer one
        after_hd_hud_glyph_draw(nullptr,&screen,nullptr,nullptr);
        assert(a.visible && b.visible && x.visible && y.visible == oldY);
        assert(s_hdHudGlyphDraw == nullptr);
    }
    assert(resolves == 1); // Cache only the host function, never the mod variable.
    // Disabled or unloaded HD HUD must preserve the vanilla letters.
    for (int scenario = 0; scenario < 3; ++scenario) {
        hdEnabled.value = scenario != 0; registered = scenario != 1;
        auto** args = scenario == 2 ? &unrelated : &screen;
        before_hd_hud_glyph_draw(nullptr,args,nullptr,nullptr);
        assert(a.visible && b.visible && x.visible);
        after_hd_hud_glyph_draw(nullptr,args,nullptr,nullptr);
    }
    registered = true; hdEnabled.value = true;
    // Missing panes and independently hidden glyphs are safe.
    meter.mpBTextA = nullptr; meter.mpBTextXY[1] = nullptr; b.hide();
    before_hd_hud_glyph_draw(nullptr,&screen,nullptr,nullptr);
    assert(a.visible && !b.visible && !x.visible);
    hdEnabled.value = false; // restore even if the provider changes mid-draw
    after_hd_hud_glyph_draw(nullptr,&screen,nullptr,nullptr);
    assert(a.visible && !b.visible && x.visible);
    // Missing resolver/API leaves letters alone instead of guessing at the layout.
    hdEnabled.value = true;
    for (int scenario = 0; scenario < 4; ++scenario) {
        s_hdHudConfigLookupAttempted = false; s_hdHudGetConfigVar = nullptr;
        svc_hook = scenario == 0 ? nullptr : &hook;
        hook.resolve = scenario == 1 ? nullptr : resolve;
        resolveFails = scenario == 2; nullAddress = scenario == 3;
        before_hd_hud_glyph_draw(nullptr,&screen,nullptr,nullptr);
        assert(x.visible);
        after_hd_hud_glyph_draw(nullptr,&screen,nullptr,nullptr);
    }
}
'''
start = source.index('struct HudPaneVisibilityState')
end = source.index('std::array<HudPaneVisibilityState, 4> s_dpadArrowVisibility;', start)
state = source[start:end]
start = source.index('std::array<HudPaneVisibilityState, 4> s_hdHudGlyphVisibility;')
end = source.index('bool s_hdHudConfigLookupAttempted = false;', start) + len('bool s_hdHudConfigLookupAttempted = false;')
fixture = fixture.replace('// STATE', state + source[start:end])
fixture = fixture.replace('// FUNCTIONS', '\n'.join(function(name) for name in (
    'twilight_hd_hud_active', 'before_hd_hud_glyph_draw', 'after_hd_hud_glyph_draw')))
with tempfile.TemporaryDirectory() as tmp:
    cpp,exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('HD HUD glyphs passed: all four letters, visibility restoration, disabled/unloaded providers, missing panes/API, nested draws and untouched discs/text/Z')
