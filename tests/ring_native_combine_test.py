"""Run registered Dawnlight combine hooks with native cursor/mix/commit code."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--dusklight-dir', type=Path, required=True)
parser.add_argument('--baseline-ref', help='Run the same regression against older production code')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
source = subprocess.check_output(
    ['git', 'show', args.baseline_ref + ':src/item_slot_hooks.cpp'], cwd=root, text=True
) if args.baseline_ref else (root / 'src/item_slot_hooks.cpp').read_text()
native = (args.dusklight_dir / 'src/d/d_menu_ring.cpp').read_text()
integrity = (root / 'src/item_integrity.cpp').read_text()


def function(text, signature):
    start = text.index(signature)
    return text[start:text.index('\n}', start) + 2]


fixture = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <vector>
using u8=unsigned char; using u32=unsigned int;
constexpr int SELECT_ITEM_X=0, SELECT_ITEM_Y=1, SELECT_ITEM_NUM=2,
    kZItemSlot=2, kExtendedSelectItemCount=3, MAX_SELECT_ITEM=4, MAX_ITEM_SLOTS=24, PAD_1=0;
constexpr u8 SLOT_4=4, dItemNo_NONE_e=255, dItemNo_BOW_e=1,
    dItemNo_NORMAL_BOMB_e=2, dItemNo_WATER_BOMB_e=3, dItemNo_POKE_BOMB_e=4,
    dItemNo_HAWK_EYE_e=5, dItemNo_KANTERA_e=6, dItemNo_FISHING_ROD_1_e=7,
    dItemNo_BEE_CHILD_e=8, dItemNo_WORM_e=9, dItemNo_ZORAS_JEWEL_e=10;
constexpr int Z2SE_SY_ITEM_COMBINE_OFF=0, Z2SE_SY_ITEM_COMBINE_ON=1, Z2SE_SYS_ERROR=2,
    MOD_OK=0, MOD_ERROR=1;
constexpr u32 PAD_TRIGGER_R=0x20, PAD_TRIGGER_L=0x40, PAD_BUTTON_X=0x400, PAD_BUTTON_Y=0x800;
struct ModContext {};
using ModResult=int;
enum HookAction {HOOK_CONTINUE, HOOK_SKIP_ORIGINAL};
namespace mods {template<class T> T arg(void* args,int) {return *static_cast<T*>(args);}}
struct Pad {u32 mPressedButtonFlags=0,mButtonFlags=0;float mTriggerRight=0;} pad;
struct mDoCPd_c {
    static Pad& getCpadInfo(int) {return pad;}
    static u32 getTrigR(int) {return pad.mPressedButtonFlags & PAD_TRIGGER_R;}
    static u32 getTrigX(int) {return pad.mPressedButtonFlags & PAD_BUTTON_X;}
    static u32 getTrigY(int) {return pad.mPressedButtonFlags & PAD_BUTTON_Y;}
};
namespace dusk::config {
struct ConfigVarBase {};
template<class T> struct ConfigVar:ConfigVarBase {T value=false;T getValue() const {return value;}};
}
std::map<std::string,dusk::config::ConfigVar<bool>> vars;
dusk::config::ConfigVarBase* getVar(std::string_view key) {
    auto it=vars.find(std::string(key));return it==vars.end()?nullptr:&it->second;
}
using ZSlotGetConfigVarFn=dusk::config::ConfigVarBase* (*)(std::string_view);
ModContext* mod_ctx=nullptr;
bool resolveFails=false,nullAddress=false,companionTrigger=false,companionHold=false;
int resolve(ModContext*,const char* symbol,void** address,void*) {
    *address=nullptr;
    if(std::string_view(symbol)=="dusk::config::GetConfigVar") {
        if(resolveFails) return MOD_ERROR;
        if(!nullAddress) *address=reinterpret_cast<void*>(&getVar);
        return MOD_OK;
    }
    if((std::string_view(symbol)=="dusk::companion::slotTriggerBits" && companionTrigger) ||
       (std::string_view(symbol)=="dusk::companion::slotHoldBits" && companionHold)) {
        *address=reinterpret_cast<void*>(&getVar);return MOD_OK;
    }
    return MOD_ERROR;
}
struct HookService {int (*resolve)(ModContext*,const char*,void**,void*);};
HookService hook{resolve};HookService* svc_hook=&hook;
// Used only when exercising the old masking implementation as a negative control.
struct daAlink_c {char data[8];}; struct daPy_anmHeap_c {char data[4];};
struct Profile {struct {struct {size_t process_size=sizeof(daAlink_c);} base;} base;};
Profile g_profile_ALINK;
u32 s_ringNativeRPressed=0;
constexpr const char* hd="mod.org_twilight_hd__hud.enabled";
constexpr const char* hdSlot="mod.org_twilight_hd__hud.third-item-slot";
constexpr const char* te="mod.com_dusklight_twilit__essentials.enabled";
constexpr const char* teSlot="mod.com_dusklight_twilit__essentials.customZButtonEnabled";
std::array<u8,3> selected,mixed;
struct dSv_player_item_c {u8 mItems[MAX_ITEM_SLOTS];} inventory;
bool s_repairingMixPairs=false;
bool item_integrity_fixes_enabled() {return true;}
u8 dComIfGs_getSelectItemIndex(int i) {return selected.at(i);}
u8 dComIfGs_getMixItemIndex(int i) {return mixed.at(i);}
void dComIfGs_setMixItemIndex(int i,u8 slot) {mixed.at(i)=slot;}
void dComIfGp_setSelectItem(int);
// INTEGRITY
u8 dComIfGs_getItem(u8 slot,bool) {
    repair_mix_pairs(&inventory);
    return slot<MAX_ITEM_SLOTS?inventory.mItems[slot]:dItemNo_NONE_e;
}
void dComIfGp_setSelectItem(int i) {(void)dComIfGs_getItem(selected.at(i),false);}
void dComIfGs_setSelectItemIndex(int i,u8 slot) {selected.at(i)=slot;dComIfGp_setSelectItem(i);}
int combines=0,uncombines=0,assignments=0;
struct Audio {
    template<class... T> void seStart(int sound,T...) {
        if(sound==Z2SE_SY_ITEM_COMBINE_ON) ++combines;
        if(sound==Z2SE_SY_ITEM_COMBINE_OFF) ++uncombines;
    }
} audio;
Audio* Z2GetAudioMgr() {return &audio;}
struct Explain {int status=0;int getStatus() {return status;}} explain;
struct dMenu_Ring_c {
    enum {STATUS_WAIT,STATUS_EXPLAIN_FORCE,STATUS_EXPLAIN};
    int mStatus=STATUS_WAIT,mOldStatus=STATUS_WAIT;
    bool mPlayerIsWolf=false;
    u8 mItemSlots[1]{15},mCurrentSlot=0,field_0x6b3=255,field_0x6cb=255,field_0x6cd=255;
    u8 field_0x6b4[4]{255,255,255,255},field_0x6b8[4]{255,255,255,255},field_0x674[4]{};
    u8 mXButtonSlot=255,mYButtonSlot=255;
    Explain* mpItemExplain=&explain;
    using Init=void (dMenu_Ring_c::*)();static Init stick_init[3];
    void init() {} void setStatus(int status) {mStatus=status;}
    void setSelectItemForce(int); void setMixItem(); void setActiveCursor();
    void setJumpItem(bool) {field_0x674[field_0x6b3]=1;}
    u8 getCursorPos(u8 slot) {return slot;}
    bool checkCombineBomb(int) {return false;} void setItem() {++assignments;}
};
dMenu_Ring_c::Init dMenu_Ring_c::stick_init[3]{&dMenu_Ring_c::init};
bool own=false,s_touchRingCombineRequested=false,s_ringComboChanged=false;
int s_pendingAssign=0;
bool z_item_slot_active() {return own;}
bool external_item_feature_enabled(const char*) {return false;}
void store_select_slots(const std::array<u8,3>& slots,const std::array<u8,3>& mixes) {
    for(int i=0;i<3;++i) {dComIfGs_setMixItemIndex(i,mixes[i]);dComIfGs_setSelectItemIndex(i,slots[i]);}
}
void sync_ring_fields(dMenu_Ring_c* ring) {
    for(int i=0;i<3;++i) {ring->field_0x6b4[i]=selected[i];ring->field_0x6b8[i]=mixed[i];}
}
void fix_z_select_item_animation(dMenu_Ring_c*) {}
// FUNCTIONS
// NATIVE
using Pre=HookAction (*)(ModContext*,void*,void*,void*);
using Post=void (*)(ModContext*,void*,void*,void*);
struct HookOptions {int priority=0;};
#define HOOK_OPTIONS_INIT {}
struct RingSetActiveCursorHook {};
std::vector<std::pair<int,Pre>> pres;
std::vector<std::pair<int,Post>> posts;
namespace mods {
template<class T> ModResult hook_add_pre(HookService*,Pre fn,const HookOptions* opts) {
    pres.emplace_back(opts->priority,fn);return MOD_OK;
}
template<class T> ModResult hook_add_post(HookService*,Post fn,const HookOptions* opts) {
    posts.emplace_back(opts->priority,fn);return MOD_OK;
}
}
void install() {
    pres.clear();posts.clear();
    // REGISTRATION
    assert(result==MOD_OK);
    std::stable_sort(pres.begin(),pres.end(),[](auto a,auto b) {return a.first>b.first;});
    std::stable_sort(posts.begin(),posts.end(),[](auto a,auto b) {return a.first>b.first;});
}
void dispatch(dMenu_Ring_c& ring,bool providerPre=false,bool providerPost=false) {
    auto* arg=&ring;
    bool skip=providerPre;
    if(providerPre) ring.setMixItem(); // external hook handles input and skips original
    if(!skip) for(auto [priority,fn]:pres) {
        (void)priority;
        if(fn(nullptr,&arg,nullptr,nullptr)==HOOK_SKIP_ORIGINAL) {skip=true;break;}
    }
    if(!skip) ring.setActiveCursor();
    for(auto [priority,fn]:posts) {(void)priority;fn(nullptr,&arg,nullptr,nullptr);}
    if(providerPost) ring.setMixItem(); // e.g. external L-release handler
    for(int i=0;i<MAX_SELECT_ITEM;++i) ring.setSelectItemForce(i);
}
void check(int slot,u8 partner,bool providerPre=false,bool providerPost=false) {
    selected.fill(255);mixed.fill(255);selected[slot]=SLOT_4;
    std::fill(std::begin(inventory.mItems),std::end(inventory.mItems),255);
    inventory.mItems[4]=dItemNo_BOW_e;inventory.mItems[15]=partner;
    combines=uncombines=assignments=0;
    dMenu_Ring_c ring;sync_ring_fields(&ring);
    pad={providerPre||providerPost?PAD_TRIGGER_L:PAD_TRIGGER_R,PAD_TRIGGER_R,1.0f};
    const auto saved=pad;
    dispatch(ring,providerPre,providerPost);
    assert(selected[slot]==15 && mixed[slot]==4 && combines==1 && uncombines==0);
    assert(pad.mPressedButtonFlags==saved.mPressedButtonFlags && pad.mButtonFlags==saved.mButtonFlags &&
           pad.mTriggerRight==saved.mTriggerRight && assignments==0);
    dispatch(ring,providerPre,providerPost);
    assert(selected[slot]==4 && mixed[slot]==255 && combines==1 && uncombines==1);
    for(int i=0;i<3;++i) if(i!=slot) assert(selected[i]==255 && mixed[i]==255);
}
int main() {
    // Provider CVars and host probes must never turn a physical/native Touch-R
    // edge into no action. Exercise actual registration and real native mixing,
    // including save commit and Dawnlight's item-integrity read hooks.
    int cases=0;
    for(int providers=0;providers<16;++providers)
    for(int host=0;host<16;++host) {
        vars.clear();vars[hd].value=providers&1;vars[hdSlot].value=providers&2;
        vars[te].value=providers&4;vars[teSlot].value=providers&8;
        companionTrigger=host&1;companionHold=host&2;
        if(host&4) vars["game.zItemScale"].value=false;
        g_profile_ALINK.base.base.process_size=sizeof(daAlink_c)+((host&8)?sizeof(daPy_anmHeap_c):0);
        for(bool enableOwn:{false,true}) {
            own=enableOwn && z_slot_provider_notice()==nullptr;
            install();
            for(int slot=0;slot<2;++slot)
            for(u8 partner:{dItemNo_NORMAL_BOMB_e,dItemNo_WATER_BOMB_e,dItemNo_POKE_BOMB_e,dItemNo_HAWK_EYE_e}) {
                check(slot,partner);++cases;
            }
            if(own) check(2,dItemNo_NORMAL_BOMB_e);
        }
    }
    // Missing provider settings and lookup failures must also preserve native R.
    for(const char* provider:{hd,te}) {
        vars.clear();vars[provider].value=true;own=false;install();check(0,dItemNo_NORMAL_BOMB_e);
        for(int failure=0;failure<4;++failure) {
            svc_hook=failure==0?nullptr:&hook;hook.resolve=failure==1?nullptr:resolve;
            resolveFails=failure==2;nullAddress=failure==3;
            install();check(1,dItemNo_WATER_BOMB_e);
        }
        svc_hook=&hook;hook.resolve=resolve;resolveFails=nullAddress=false;
    }
    // External L paths still execute once; Dawnlight adds no competing mix.
    vars.clear();vars[te].value=true;own=false;install();
    check(0,dItemNo_NORMAL_BOMB_e,true,false);
    check(1,dItemNo_NORMAL_BOMB_e,false,true);
    // Native guards and X/Y assignment remain active with a provider enabled.
    dMenu_Ring_c ring;
    for(u32 button:{PAD_BUTTON_X,PAD_BUTTON_Y}) {
        pad.mPressedButtonFlags=button;assignments=0;dispatch(ring);assert(assignments==1);
    }
    // Simultaneous R+X/Y keeps the native R-first priority, without an assignment.
    for(u32 button:{PAD_BUTTON_X,PAD_BUTTON_Y}) {
        selected.fill(255);mixed.fill(255);selected[0]=4;
        ring=dMenu_Ring_c{};sync_ring_fields(&ring);
        pad.mPressedButtonFlags=PAD_TRIGGER_R|button;assignments=0;
        dispatch(ring);assert(selected[0]==15 && mixed[0]==4 && assignments==0);
    }
    pad.mPressedButtonFlags=PAD_TRIGGER_R;
    const int previous=combines+uncombines;
    ring.mPlayerIsWolf=true;dispatch(ring);ring.mPlayerIsWolf=false;
    ring.mStatus=dMenu_Ring_c::STATUS_EXPLAIN;dispatch(ring);ring.mStatus=dMenu_Ring_c::STATUS_WAIT;
    explain.status=1;dispatch(ring);explain.status=0;
    assert(combines+uncombines==previous);
    std::printf("Native R combine passed: %d provider/host/slot/item cases, uncombine, integrity, L paths and guards\n",cases);
}
'''
names = ['bool z_slot_provider_enabled(', 'const char* z_slot_provider_notice(',
         'bool dawnlight_combine_active(', 'bool bow_mix_item(', 'bool bow_mix_item_on(',
         'bool bow_mix_item_off(', 'bool set_bow_mix_item(', 'bool item_assign_allowed(',
         'HookAction before_ring_combine(', 'void after_ring_combine(']
# Keep the actual old registered mask when testing a baseline, so the same
# assertions demonstrate the reported regression rather than mocking its cause.
if 'bool external_ring_combine_active(' in source:
    names += ['bool external_ring_combine_active(', 'HookAction before_ring_native_combine(',
              'void after_ring_native_combine(']
fixture = fixture.replace('// FUNCTIONS', '\n'.join(function(source, name) for name in names))
fixture = fixture.replace('// INTEGRITY', '\n'.join(function(integrity, name) for name in (
    'bool valid_slot(', 'bool valid_mix_pair(', 'void repair_mix_pairs(')))
fixture = fixture.replace('// NATIVE', '\n'.join(function(native, name) for name in (
    'void dMenu_Ring_c::setActiveCursor(', 'void dMenu_Ring_c::setMixItem(',
    'void dMenu_Ring_c::setSelectItemForce(')))
start = source.index('    HookOptions comboOptions = HOOK_OPTIONS_INIT;')
end = source.index('    if (result == MOD_OK && s_zItemSlotSessionEnabled)', start)
fixture = fixture.replace('// REGISTRATION', source[start:end])
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
