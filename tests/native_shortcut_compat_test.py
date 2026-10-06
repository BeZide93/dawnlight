"""Compile the native shortcut compatibility hooks for Android and desktop.

Run: python3 tests/native_shortcut_compat_test.py. Device gameplay still needs QA.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <cassert>
#include <string_view>
#include <initializer_list>
using f32 = float;
struct ModContext {};
enum HookAction { HOOK_CONTINUE };
namespace mods { template<class T> T arg(void* args, int) { return *static_cast<T*>(args); } }
namespace dusk::config {
struct ConfigVarBase {};
template<class T> struct ConfigVar : ConfigVarBase { T value{}; T getValue() const { return value; } };
}
bool touch=true, priority=false, suppressed=false, nextStage=false, midnaUnlocked=true;
bool midnaBlocked=false, playerBlocked=false, storyBlocked=false;
struct dSv_event_flag_c { enum { F_0800=0xDEAD }; };
bool dawnlight_touch_ui_active() { return touch; }
bool touch_midna_controls_suppressed() { return suppressed; }
bool dComIfGp_isEnableNextStage() { return nextStage; }
bool dComIfGs_isEventBit(int bit) { return bit==0x0C10 ? midnaUnlocked : bit==dSv_event_flag_c::F_0800 ? storyBlocked : midnaBlocked; }
bool dComIfGp_checkPlayerStatus0(int, int) { return playerBlocked; }
struct daAlink_c {
 enum { PROC_WAIT, PROC_CRAWL_END };
 int mProcID=PROC_WAIT, room=0; unsigned mode=0;
 bool rein=false, boots=false, zelda=false, cloud=false, midna=true;
 std::string_view stage="F_SP108";
 struct { bool ground=true; bool ChkGroundHit() { return ground; } } mLinkAcch;
 struct { void* actor=nullptr; void* getActor() { return actor; } } mThrowBoomerangAcKeep;
 struct { float y=1; } mMagneBootsTopVec;
 bool checkReinRide() { return rein; }
 bool checkModeFlg(unsigned mask) { return (mode&mask)!=0; }
 bool checkMagneBootsOn() { return boots; }
 bool checkHorseZelda() { return zelda; }
 bool checkCloudSea() { return cloud; }
 bool checkStageName(const char* name) { return stage==name; }
 bool checkMidnaRide() { return midna; }
};
int fopAcM_GetRoomNo(daAlink_c* link) { return link->room; }
struct dComIfG_play_c { static int getLayerNo(int) { return 0; } };
bool cBgW_CheckBGround(float y) { return y>0.5f; }
bool gale_shortcut_priority_active(const daAlink_c* link) { return link && priority; }
struct dMeter2Draw_c {
 float mButtonZAlpha=0; bool canoe=false;
 bool getCanoeFishing() { return canoe; }
};
struct Meter { dMeter2Draw_c* draw=nullptr; auto getMeterDrawPtr() { return draw; } };
Meter* meterClass=nullptr;
auto dMeter2Info_getMeterClass() { return meterClass; }
// PRODUCTION
#if defined(__ANDROID__)
dusk::config::ConfigVar<bool> nativeTouch;
bool settingPresent=true;
dusk::config::ConfigVarBase* lookup(std::string_view name) {
 assert(name=="game.enableTouchControls"); return settingPresent ? &nativeTouch : nullptr;
}
#endif
int main() {
 daAlink_c link; auto* args=&link; dMeter2Draw_c meter; Meter owner{&meter}; meterClass=&owner;
 auto invoke=[&](bool eligible) {
  for(float alpha : {0.0f,0.4f,1.0f}) {
   meter.mButtonZAlpha=alpha;
   assert(before_native_shortcut(nullptr,&args,nullptr,nullptr)==HOOK_CONTINUE);
   assert(meter.mButtonZAlpha==(eligible ? 1.0f : alpha));
   after_native_shortcut(nullptr,nullptr,nullptr,nullptr);
   assert(meter.mButtonZAlpha==alpha);
  }
 };
 invoke(false); // No touch resolver (or desktop) and no Gale cancellation.
#if defined(__ANDROID__)
 s_touchGetConfigVar=lookup; nativeTouch.value=true; invoke(true);
 touch=false; invoke(false); touch=true;
 nativeTouch.value=false; invoke(false); nativeTouch.value=true;
 settingPresent=false; invoke(false); settingPresent=true;
 s_touchGetConfigVar=nullptr; invoke(false);
#endif
 priority=true; invoke(true); // Gale priority also works without Android touch support.
 for(bool* flag : {&suppressed,&nextStage,&midnaBlocked,&storyBlocked,&playerBlocked,&meter.canoe,
                    &link.zelda,&link.cloud}) {
  *flag=true; invoke(false); *flag=false;
 }
 for(bool* flag : {&midnaUnlocked,&link.midna,&link.mLinkAcch.ground}) {
  *flag=false; invoke(false); *flag=true;
 }
 link.mode=0x2; invoke(false); link.mode=0;
 link.mThrowBoomerangAcKeep.actor=&link; invoke(false); link.mThrowBoomerangAcKeep.actor=nullptr;
 link.mProcID=daAlink_c::PROC_CRAWL_END; invoke(false); link.mProcID=daAlink_c::PROC_WAIT;
 link.stage="D_MN08A"; invoke(false);
 link.stage="D_MN09A"; link.room=50; invoke(false); link.room=51; invoke(false);
 link.room=0; invoke(true);
 link.boots=true; invoke(false); link.stage="D_MN04B"; invoke(true);
 link.mMagneBootsTopVec.y=0; invoke(false); link.boots=false; link.stage="F_SP108";
 args=nullptr; invoke(false); args=&link;
 owner.draw=nullptr; invoke(false); owner.draw=&meter;
 meterClass=nullptr; invoke(false); meterClass=&owner;
 // Teardown/replacement during a native handler must not write into a new HUD.
 before_native_shortcut(nullptr,&args,nullptr,nullptr);
 dMeter2Draw_c replacement; replacement.mButtonZAlpha=0.3f; owner.draw=&replacement;
 after_native_shortcut(nullptr,nullptr,nullptr,nullptr);
 assert(replacement.mButtonZAlpha==0.3f && s_shortcutMeter==nullptr);
}
'''
fixture = fixture.replace('// PRODUCTION', (root/'src/native_shortcut_compat.inc').read_text())
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture)
    for defines in ([], ['-D__ANDROID__']):
        subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', *defines,
                        str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
print('Native shortcut regression passed: Android/desktop scope, gameplay guards and HUD restoration')
