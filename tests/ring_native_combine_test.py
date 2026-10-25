"""Exercise the real native cursor handler, including provider hook ordering."""
from pathlib import Path
import argparse
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--dusklight-dir', type=Path, required=True)
parser.add_argument('--without-guard', action='store_true')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
source = (root / 'src/item_slot_hooks.cpp').read_text()
native = (args.dusklight_dir / 'src/d/d_menu_ring.cpp').read_text()


def function(text, signature):
    start = text.index(signature)
    return text[start:text.index('\n}', start) + 2]


fixture = r'''
#include <cassert>
#include <cstddef>
#include <initializer_list>
using u8 = unsigned char;
using u32 = unsigned int;
constexpr int PAD_1=0, MAX_SELECT_ITEM=4, Z2SE_SYS_ERROR=0;
constexpr u8 dItemNo_NONE_e=255;
constexpr u32 PAD_TRIGGER_L=0x40, PAD_TRIGGER_R=0x20,
    PAD_TRIGGER_Z=0x10, PAD_BUTTON_X=0x400, PAD_BUTTON_Y=0x800;
struct ModContext {};
enum HookAction {HOOK_CONTINUE, HOOK_SKIP_ORIGINAL};
struct Pad {u32 mPressedButtonFlags=0, mButtonFlags=0; float mTriggerRight=0;};
Pad pad;
struct mDoCPd_c {
    static Pad& getCpadInfo(int) {return pad;}
    static u32 getTrigR(int) {return pad.mPressedButtonFlags & PAD_TRIGGER_R;}
    static u32 getTrigX(int) {return pad.mPressedButtonFlags & PAD_BUTTON_X;}
    static u32 getTrigY(int) {return pad.mPressedButtonFlags & PAD_BUTTON_Y;}
};
u32 s_ringNativeRPressed=0;
const char* provider=nullptr;
const char* z_slot_provider_notice() {return provider;}
int mixes=0, assignments=0, forced=0;
u8 dComIfGs_getItem(u8,bool) {return 5;}
struct Audio {template<class... T> void seStart(T...) {}} audio;
Audio* Z2GetAudioMgr() {return &audio;}
struct Explain {int getStatus() {return 0;}} explain;
struct dMenu_Ring_c {
    enum {STATUS_WAIT, STATUS_EXPLAIN_FORCE, STATUS_EXPLAIN};
    int mStatus=STATUS_WAIT, mOldStatus=STATUS_WAIT;
    bool mPlayerIsWolf=false;
    u8 mItemSlots[1]{1}, mCurrentSlot=0, field_0x6b3=255;
    Explain* mpItemExplain=&explain;
    using Init=void (dMenu_Ring_c::*)();
    static Init stick_init[3];
    void init() {}
    void setStatus(int status) {mStatus=status;}
    void setSelectItemForce(int) {++forced;}
    void setMixItem() {++mixes;}
    bool checkCombineBomb(int) {return false;}
    void setItem() {++assignments;}
    void setActiveCursor();
};
dMenu_Ring_c::Init dMenu_Ring_c::stick_init[3]{&dMenu_Ring_c::init};
// FUNCTIONS
// NATIVE

// Host dispatch is priority-descending. HD handles L in its pre-hook and
// skips remaining pre-hooks/the original; all post-hooks still execute.
// TE's L-tap handler invokes setMixItem in a post-hook after the native call.
void dispatch(dMenu_Ring_c& ring, bool hdL=false, bool teLRelease=false) {
    if (hdL) {
        ring.setMixItem();
    } else {
#ifndef WITHOUT_GUARD
        assert(before_ring_native_combine(nullptr,nullptr,nullptr,nullptr)==HOOK_CONTINUE);
#endif
        ring.setActiveCursor();
    }
#ifndef WITHOUT_GUARD
    after_ring_native_combine(nullptr,nullptr,nullptr,nullptr);
#endif
    if (teLRelease) ring.setMixItem();
}

int main() {
    dMenu_Ring_c ring;
    for (const char* notice:{"Z Items: Twilit Essentials.", "Z Items: Twilight HD HUD."}) {
        provider=notice;
        // The same native bit comes from R/R2 or Touch-R. It must not reach
        // setMixItem, but must remain intact for callers outside this scope.
        pad={PAD_TRIGGER_R, PAD_TRIGGER_R, 1.0f};
        mixes=assignments=forced=0;
        dispatch(ring);
        assert(mixes==0 && assignments==0 && forced==0);
        assert(pad.mPressedButtonFlags==PAD_TRIGGER_R);
        assert(pad.mButtonFlags==PAD_TRIGGER_R && pad.mTriggerRight==1.0f);

        // R must not swallow a simultaneous X/Y assignment in vanilla's
        // if/else chain. Unrelated input bits must survive restoration.
        for (u32 button:{PAD_BUTTON_X,PAD_BUTTON_Y}) {
            pad.mPressedButtonFlags=PAD_TRIGGER_R|button|PAD_TRIGGER_Z;
            mixes=assignments=0;
            dispatch(ring);
            assert(mixes==0 && assignments==1);
            assert(pad.mPressedButtonFlags==(PAD_TRIGGER_R|button|PAD_TRIGGER_Z));
        }
        // Provider pre-hook skips our late pre-hook; post must not restore
        // an R edge from an earlier call. L still performs exactly one mix.
        pad.mPressedButtonFlags=PAD_TRIGGER_L;
        mixes=0;dispatch(ring,true);
        assert(mixes==1 && pad.mPressedButtonFlags==PAD_TRIGGER_L);
        // A simultaneous R edge must not double-toggle TE's L-release mix.
        pad.mPressedButtonFlags=PAD_TRIGGER_R;
        mixes=0;dispatch(ring,false,true);
        assert(mixes==1 && pad.mPressedButtonFlags==PAD_TRIGGER_R);
        // Restore only R, preserving changes to unrelated buttons.
        pad.mPressedButtonFlags=PAD_TRIGGER_R|PAD_BUTTON_X;
        before_ring_native_combine(nullptr,nullptr,nullptr,nullptr);
        pad.mPressedButtonFlags=PAD_BUTTON_Y;
        after_ring_native_combine(nullptr,nullptr,nullptr,nullptr);
        assert(pad.mPressedButtonFlags==(PAD_TRIGGER_R|PAD_BUTTON_Y));
    }
    // No provider: original R behavior is unchanged.
    provider=nullptr;pad.mPressedButtonFlags=PAD_TRIGGER_R;
    mixes=0;dispatch(ring);assert(mixes==1);
    assert(pad.mPressedButtonFlags==PAD_TRIGGER_R && s_ringNativeRPressed==0);
}
'''
fixture = fixture.replace('// FUNCTIONS', '\n'.join(function(source, name) for name in (
    'bool dawnlight_combine_active(', 'HookAction before_ring_native_combine(',
    'void after_ring_native_combine(')))
fixture = fixture.replace('// NATIVE', function(native, 'void dMenu_Ring_c::setActiveCursor('))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    options = ['-DWITHOUT_GUARD'] if args.without_guard else []
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', *options,
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Native cursor passed: R blocked for providers, X/Y preserved, provider L and post-hook restoration')
