"""Mode switches must preserve sibling callbacks on every Boss Rush target."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/new_save_modes.cpp").read_text()


def function(name):
    match = re.search(r"^(?:ModResult|HookAction|void) " + name + r"\([^\n]*\) \{", source, re.M)
    assert match, name
    start = source.index("{", match.start())
    depth = 0
    for end in range(start, len(source)):
        depth += (source[end] == "{") - (source[end] == "}")
        if depth == 0:
            return source[match.start():end + 1]
    raise AssertionError(name)


install = function("install_bossrush_runtime_hooks")
registrations = re.findall(
    r"mods::hook_add_(pre|post)<(\w+)>\(svc_hook, bossrush_(?:pre|post)<(\w+)>\)", install)
assert len(registrations) == len(re.findall(r"mods::hook_add_(?:pre|post)<", install))
targets = list(dict.fromkeys(target for _, target, _ in registrations))
assert {"WarpPlayerExecuteHook", "WarpPlayerDrawHook", "MeterExecuteHook"} <= set(targets)
# Never perform destructive hook removal on a mode switch, even indirectly.
assert source.count("uninstall_bossrush_runtime_hooks(") == 2  # definition and mod shutdown

fixture = r'''
#include <array>
#include <cassert>
#include <vector>
struct ModContext {};
struct ModError {};
enum ModResult {MOD_OK,MOD_ERROR};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
using Pre=HookAction(*)(ModContext*,void*,void*,void*);
using Post=void(*)(ModContext*,void*,void*,void*);
struct Slot {std::vector<Pre> pre;std::vector<Post> post;};
// All registrations belong to Dawnlight, matching HookService's ownership.
std::array<Slot,64> slots;
int additions=0,removals=0,bossCalls=0,siblingCalls=0,dialogues=0;
bool sBossRushGameModeActive=false,sBossRushHooksInstalled=false;
bool banner=false,logo=false,failDialogue=false;
enum class MidnaRootFlowMode {None,Menu};
MidnaRootFlowMode sMidnaRootFlowMode=MidnaRootFlowMode::None;
void* svc_hook=nullptr;ModContext* mod_ctx=nullptr;
struct Log {void info(ModContext*,const char*){}} logger;
Log* svc_log=&logger;
namespace mods {
ModResult set_error(ModError*,ModResult r,const char*){return r;}
template<class T> ModResult hook_add_pre(void*,Pre fn){slots[T::id].pre.push_back(fn);++additions;return MOD_OK;}
template<class T> ModResult hook_add_post(void*,Post fn){slots[T::id].post.push_back(fn);++additions;return MOD_OK;}
template<class T> ModResult hook_uninstall(void*) {
    // Native uninstall erases ALL callbacks of this mod at the target,
    // not only the callback associated with T's original-function slot.
    slots[T::id]={};++removals;return MOD_OK;
}
}
void shutdown_midna_flow(){sMidnaRootFlowMode=MidnaRootFlowMode::None;}
ModResult install_midna_flow(ModError*) {
    ++dialogues;
    if(failDialogue)return MOD_ERROR;
    sMidnaRootFlowMode=MidnaRootFlowMode::Menu;return MOD_OK;
}
ModResult register_bossrush_hub_banner(ModError*){banner=true;return MOD_OK;}
void unregister_bossrush_hub_banner(){banner=false;}
void unregister_bossrush_title_logo(){logo=false;}
void reset_bossrush_runtime_state(bool){}
HookAction sibling_pre(ModContext*,void*,void*,void*){++siblingCalls;return HOOK_CONTINUE;}
void sibling_post(ModContext*,void*,void*,void*){++siblingCalls;}
void dispatch(bool active) {
    for(auto& slot:slots) {
        for(auto fn:slot.pre) {
            const auto result=fn(nullptr,nullptr,nullptr,nullptr);
            assert(result==(active && fn!=sibling_pre ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE));
        }
        for(auto fn:slot.post)fn(nullptr,nullptr,nullptr,nullptr);
    }
}
'''
for idx, target in enumerate(targets):
    fixture += f"struct {target} {{static constexpr int id={idx};}};\n"
for kind, _, callback in registrations:
    result = "HookAction" if kind == "pre" else "void"
    returning = "return HOOK_SKIP_ORIGINAL;" if kind == "pre" else ""
    fixture += f"{result} {callback}(ModContext*,void*,void*,void*){{++bossCalls;{returning}}}\n"
for name in ("bossrush_pre", "bossrush_post"):
    fixture += "template <auto Callback>\n" + function(name) + "\n"
fixture += install + "\n"
fixture += "template <class Entry>\n" + function("uninstall_bossrush_hook") + "\n"
for name in ("uninstall_bossrush_runtime_hooks", "activate_bossrush_runtime",
             "on_bossrush_game_mode_deactivated"):
    fixture += function(name) + "\n"
fixture += f"constexpr int targetCount={len(targets)}, callbackCount={len(registrations)};\n"
fixture += r'''
int main() {
    // Stand-ins for Dual Wield, jump abilities, HUD, and any other sibling
    // feature. Exercise both pre and post registrations at EVERY target.
    for(int i=0;i<targetCount;++i){slots[i].pre.push_back(sibling_pre);slots[i].post.push_back(sibling_post);}
    ModError error;
    for(int cycle=0;cycle<4;++cycle) {
        assert(activate_bossrush_runtime(&error,true)==MOD_OK);
        assert(additions==callbackCount && removals==0 && dialogues==cycle+1);
        assert(sMidnaRootFlowMode==MidnaRootFlowMode::Menu && banner);
        assert(activate_bossrush_runtime(&error,false)==MOD_OK); // save-load/play callback
        assert(additions==callbackCount && dialogues==cycle+1);
        int beforeBoss=bossCalls,beforeSibling=siblingCalls;
        dispatch(true);
        assert(bossCalls==beforeBoss+callbackCount && siblingCalls==beforeSibling+2*targetCount);
        logo=true;
        assert(on_bossrush_game_mode_deactivated(nullptr,&error)==MOD_OK);
        assert(!sBossRushGameModeActive && sBossRushHooksInstalled && removals==0);
        assert(!banner && !logo && sMidnaRootFlowMode==MidnaRootFlowMode::None);
        beforeBoss=bossCalls;beforeSibling=siblingCalls;
        dispatch(false);
        assert(bossCalls==beforeBoss && siblingCalls==beforeSibling+2*targetCount);
    }
    // Failed dialogue restoration leaves retained callbacks inert. A retry
    // restores the mode without registering duplicate hooks.
    failDialogue=true;
    assert(activate_bossrush_runtime(&error,true)==MOD_ERROR);
    assert(!sBossRushGameModeActive && !banner && additions==callbackCount);
    int beforeBoss=bossCalls;dispatch(false);assert(bossCalls==beforeBoss);
    failDialogue=false;
    assert(activate_bossrush_runtime(&error,true)==MOD_OK && additions==callbackCount);
    assert(on_bossrush_game_mode_deactivated(nullptr,&error)==MOD_OK);
    // Destructive removal is reserved for unloading the entire mod.
    assert(uninstall_bossrush_runtime_hooks(&error)==MOD_OK);
    assert(!sBossRushHooksInstalled && removals==targetCount);
    for(auto& slot:slots)assert(slot.pre.empty() && slot.post.empty());
}
'''
with tempfile.TemporaryDirectory() as folder:
    cpp = Path(folder) / "lifecycle.cpp"
    exe = Path(folder) / "lifecycle"
    cpp.write_text(fixture)
    subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Boss Rush mode switches preserve sibling hooks, gate callbacks and restore dialogue: OK")
