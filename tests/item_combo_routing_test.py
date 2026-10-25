"""Run the production combo dispatcher against controller/touch/provider cases."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/item_slot_hooks.cpp').read_text()


def function(signature):
    start = source.index(signature)
    end = source.index('\n}', start) + 2
    return source[start:end]


fixture = r'''
#include <array>
#include <cassert>
#include <string_view>
using u8 = unsigned char;
constexpr int SELECT_ITEM_X=0, SELECT_ITEM_Y=1, SELECT_ITEM_NUM=2,
    kZItemSlot=2, kExtendedSelectItemCount=3, MAX_SELECT_ITEM=4, PAD_1=0;
constexpr u8 SLOT_4=4, dItemNo_NONE_e=255, dItemNo_BOW_e=1,
    dItemNo_NORMAL_BOMB_e=2, dItemNo_WATER_BOMB_e=3, dItemNo_POKE_BOMB_e=4,
    dItemNo_HAWK_EYE_e=5, dItemNo_KANTERA_e=6;
constexpr int Z2SE_SY_ITEM_COMBINE_OFF=0, Z2SE_SY_ITEM_COMBINE_ON=1, Z2SE_SYS_ERROR=2;
struct ModContext {};
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
namespace mods { template<class T> T arg(void* args,int) { return *static_cast<T*>(args); } }
std::array<u8,3> selected, mixed;
std::array<u8,24> inventory;
bool hd=false, own=true, touchUi=true, standalone=true, left=false, right=false, z=false;
bool s_touchZItemHeld=false, s_touchRingCombineRequested=false, s_ringComboChanged=false;
int s_pendingAssign=0, assignments=0, restores=0, jumps=0;
bool hd_item_slot_active() { return hd; }
bool z_item_slot_active() { return own; }
bool dawnlight_touch_ui_active() { return touchUi; }
bool external_item_feature_enabled(const char* key) {
    assert(std::string_view(key)=="mod.dev_baxlek_fire__arrows.enabled"); return standalone;
}
struct mDoCPd_c {
    static bool getTrigL(int) {return left;}
    static bool getTrigR(int) {return right;}
    static bool getTrigZ(int) {return z;}
};
u8 dComIfGs_getItem(u8 slot,bool) {return inventory.at(slot);}
u8 dComIfGs_getSelectItemIndex(int slot) {return selected.at(slot);}
u8 dComIfGs_getMixItemIndex(int slot) {return mixed.at(slot);}
void dComIfGp_setSelectItem(int) {++restores;}
struct Explain {int getStatus(){return 0;}} explain;
struct dMenu_Ring_c {
    enum {STATUS_WAIT, STATUS_EXPLAIN_FORCE, STATUS_EXPLAIN};
    bool mPlayerIsWolf=false;
    int mStatus=STATUS_WAIT, mOldStatus=STATUS_WAIT;
    u8 mCurrentSlot=0, mItemSlots[24]{1}, field_0x6cb=255, field_0x6cd=255,
        field_0x6b3=255, field_0x674[3]{};
    Explain* mpItemExplain=&explain;
    void setSelectItemForce(int){}
    void setJumpItem(bool){++jumps;}
    void setStatus(int status){mStatus=status;}
    void stick_wait_init(){}
};
struct Audio {template<class... T> void seStart(T...) {}} audio;
Audio* Z2GetAudioMgr(){return &audio;}
void store_select_slots(const std::array<u8,3>& slots,const std::array<u8,3>& mixes) {
    selected=slots;mixed=mixes;
}
void sync_ring_fields(dMenu_Ring_c*){}
void fix_z_select_item_animation(dMenu_Ring_c*){}
void capture_vanilla_assign(dMenu_Ring_c*){}
void assign_current_item(dMenu_Ring_c*,u8){++assignments;}
// FUNCTIONS
int main() {
    dMenu_Ring_c ring; auto* arg=&ring;
    // L under HD and R under Dawnlight must combine every supported item on
    // every slot. Touch requests remain valid after HD clears logical R.
    for(bool provider:{false,true}) for(bool touch:{false,true})
    for(int slot=0;slot<3;++slot)
    for(u8 item:{dItemNo_NORMAL_BOMB_e,dItemNo_WATER_BOMB_e,dItemNo_POKE_BOMB_e,
            dItemNo_HAWK_EYE_e,dItemNo_KANTERA_e}) {
        hd=provider;own=!provider;standalone=true;
        selected.fill(255);mixed.fill(255);selected[slot]=SLOT_4;
        inventory.fill(255);inventory[4]=dItemNo_BOW_e;inventory[1]=item;
        left=!touch && hd;right=!touch && own;s_touchRingCombineRequested=touch;
        assert(before_ring_combine(nullptr,&arg,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(selected[slot]==1 && mixed[slot]==4);
        assert(ring.field_0x6b3==slot && ring.field_0x674[slot]==1);
        assert(!s_touchRingCombineRequested);
        // The Standalone post-hook restores its disguise before our refresh.
        if(item==dItemNo_HAWK_EYE_e)inventory[1]=dItemNo_KANTERA_e;
        int before=restores;
        after_ring_combine(nullptr,&arg,nullptr,nullptr);
        assert(restores==before+3 && !s_ringComboChanged);
        s_touchRingCombineRequested=touch;
        assert(before_ring_combine(nullptr,&arg,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
        assert(selected[slot]==4 && mixed[slot]==255);
        after_ring_combine(nullptr,&arg,nullptr,nullptr);
    }
    // Without Standalone, ordinary Lantern assignment remains untouched.
    hd=true;own=false;left=true;right=false;standalone=false;
    selected.fill(255);mixed.fill(255);selected[0]=4;inventory[1]=dItemNo_KANTERA_e;
    assert(before_ring_combine(nullptr,&arg,nullptr,nullptr)==HOOK_CONTINUE);
    assert(selected[0]==4 && mixed[0]==255);
    // Never create the disabled Z slot, even for a native combo.
    hd=own=false;selected[0]=255;selected[2]=4;inventory[1]=dItemNo_NORMAL_BOMB_e;
    s_touchRingCombineRequested=true;
    assert(before_ring_combine(nullptr,&arg,nullptr,nullptr)==HOOK_CONTINUE);
    assert(selected[2]==4 && mixed[2]==255);
    // No button: do not steal X/Y assignment. Wolf/explanation states are gated.
    hd=true;left=right=false;selected[2]=255;selected[0]=4;
    assert(before_ring_combine(nullptr,&arg,nullptr,nullptr)==HOOK_CONTINUE);
    left=true;ring.mPlayerIsWolf=true;
    assert(before_ring_combine(nullptr,&arg,nullptr,nullptr)==HOOK_CONTINUE);
    ring.mPlayerIsWolf=false;ring.mOldStatus=dMenu_Ring_c::STATUS_EXPLAIN;
    assert(before_ring_combine(nullptr,&arg,nullptr,nullptr)==HOOK_CONTINUE);
    ring.mOldStatus=dMenu_Ring_c::STATUS_WAIT;
    // A physical Z edge must reach HD. Our touch-only fallback cannot replace
    // the new combination with a plain item assignment, in either hook order.
    z=true;own=false;hd=true;s_touchZItemHeld=false;
    assert(before_ring_set_active_cursor(nullptr,&arg,nullptr,nullptr)==HOOK_CONTINUE);
    s_touchZItemHeld=true;
    assert(before_ring_set_active_cursor(nullptr,&arg,nullptr,nullptr)==HOOK_CONTINUE);
    assert(assignments==0);
    hd=false;s_touchZItemHeld=false;
    assert(before_ring_set_active_cursor(nullptr,&arg,nullptr,nullptr)==HOOK_CONTINUE);
    s_touchZItemHeld=true;
    assert(before_ring_set_active_cursor(nullptr,&arg,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
    assert(assignments==1);
}
'''
names = ['bool bow_mix_item(', 'bool bow_mix_item_on(', 'bool bow_mix_item_off(',
         'bool set_bow_mix_item(', 'bool item_assign_allowed(',
         'HookAction before_ring_combine(', 'void after_ring_combine(',
         'HookAction before_ring_set_active_cursor(']
fixture = fixture.replace('// FUNCTIONS', '\n'.join(function(name) for name in names))
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    exe = Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Combo routing passed: HD/Dawnlight, controller/touch, X/Y/Z, native and Lantern combos')
