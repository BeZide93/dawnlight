"""Exercise the production pad hook, spin gate, drain and hit refill together."""
from pathlib import Path
import os
import subprocess
import tempfile
from fierce_deity_lifecycle_test import fixture, state, preload_state, callbacks, function, root

fixture = fixture.replace('void update_spin_activation(daAlink_c*) { ++spinUpdates; }',
                          'void update_spin_activation(daAlink_c*);')
fixture = fixture.replace('void update_drain(daAlink_c*) { ++drainUpdates; }',
                          'void update_drain(daAlink_c*);')
fixture = fixture.replace('fpc_ProcID id = 1;', '''
    enum { PROC_WAIT, PROC_CUT_TURN_MOVE, PROC_CUT_TURN_CHARGE, PROC_CUT_TURN };
    int mProcID = PROC_WAIT;
    bool cut = false;
    bool getCutAtFlg() const { return cut; }
    fpc_ProcID id = 1;''')
fixture += r'''
constexpr float kMeterDrainPerSecond = 5.0f;
constexpr u32 PAD_BUTTON_A=0x100, PAD_TRIGGER_R=0x20, PAD_TRIGGER_Z=0x10, PAD_BUTTON_B=0x200;
constexpr int PAD_1=0;
struct Pad { u32 mButtonFlags=0, mPressedButtonFlags=0, mHoldLockR=0, mTrigLockR=0; float mTriggerRight=0; } pad;
struct mDoCPd_c { static Pad& getCpadInfo(int) { return pad; } };
enum class FierceDeityActivation : int { SpinAttack, RZ, RA };
FierceDeityActivation binding=FierceDeityActivation::RZ;
FierceDeityActivation fierce_deity_activation() { return binding; }
'''
production = callbacks + ''.join(function(n) for n in (
    'before_fierce_game_combos', 'update_spin_activation', 'update_drain', 'fierce_deity_input_consumed'))
checks = r'''
void input(u32 held, u32 pressed=0, bool analog=false) {
    pad={held,pressed,(held&PAD_TRIGGER_R)!=0,(pressed&PAD_TRIGGER_R)!=0,1.0f};
    if(analog) { pad.mButtonFlags &= ~PAD_TRIGGER_R; pad.mPressedButtonFlags &= ~PAD_TRIGGER_R; }
    assert(before_fierce_game_combos(nullptr,nullptr,nullptr,nullptr)==HOOK_CONTINUE);
}
void fresh(daAlink_c& link) {
    link={}; currentLink=&link; enabled=true; paused=false;
    reset_for_link(&link); s_state.meter=100;
}
void hit(daAlink_c& link) {
    fopAc_ac_c enemy; Collider collider; dCcU_AtInfo attack{&link,&collider};
    void* args[]={&enemy,&attack}; after_damage_check(nullptr,args,nullptr,nullptr);
}
int main() {
    daAlink_c link;
    for(auto mode:{FierceDeityActivation::RZ,FierceDeityActivation::RA}) {
        binding=mode;
        const u32 partner=mode==FierceDeityActivation::RA?PAD_BUTTON_A:PAD_TRIGGER_Z;
        const u32 chord=PAD_TRIGGER_R|partner;
        for(bool analog:{false,true}) {
            fresh(link);
            input(PAD_TRIGGER_R,PAD_TRIGGER_R,analog);
            assert(!s_state.active && !fierce_deity_input_consumed());
            assert(pad.mHoldLockR && pad.mTrigLockR && pad.mTriggerRight==1);
            // Simulate the initial R jump moving Link before Z/A is pressed.
            link.execute(); const int jumpY=link.y, jumpVY=link.vy;
            input(chord,partner,analog);
            assert(link.y==jumpY && link.vy==jumpVY);
            assert(s_state.active && s_state.meter==100 && fierce_deity_input_consumed());
            assert((pad.mButtonFlags&partner)==0 && (pad.mPressedButtonFlags&partner)==0);
            assert(pad.mHoldLockR && !pad.mTrigLockR && pad.mTriggerRight==1);
            for(int i=0;i<60;++i) input(chord,0,analog);
            assert(s_state.active); // a held chord must never toggle repeatedly
            s_state.lastDrainTime=Clock::now()-std::chrono::milliseconds(200);
            update_drain(&link); assert(s_state.meter<99.1f && s_state.meter>98.7f);
            s_state.meter=47.0f;
            hit(link); assert(s_state.meter==47.0f); // no recharge while active
            input(PAD_TRIGGER_R,0,analog); // R may stay down; partner can be pressed again
            assert(!fierce_deity_input_consumed() && pad.mHoldLockR);
            input(chord,partner,analog);
            assert(!s_state.active && s_state.meter==47.0f);
            s_state.lastDrainTime=Clock::now()-std::chrono::seconds(5);
            update_drain(&link); assert(s_state.meter==47.0f); // drain stops immediately
            hit(link); assert(s_state.meter==52.0f);
            input(0); input(chord,chord,analog);
            assert(!s_state.active && s_state.meter==52.0f); // cannot refill by reactivating
            for(int i=0;i<10;++i) hit(link);
            assert(s_state.meter==100.0f);
            input(chord,0,analog); assert(!s_state.active); // filling while held is not a new press
            input(0); input(chord,chord,analog); assert(s_state.active);
            input(partner); assert(pad.mButtonFlags&partner); // no persistent input masking
            input(0); assert(!fierce_deity_input_consumed());
            input(PAD_TRIGGER_R,PAD_TRIGGER_R,analog); assert(pad.mHoldLockR); // ordinary R works again
        }
        // Z/A held first followed by a fresh R press must NOT trigger the shortcut.
        fresh(link); input(partner,partner); assert(!s_state.active);
        input(chord|PAD_BUTTON_B,PAD_TRIGGER_R);
        assert(!s_state.active && !fierce_deity_input_consumed());
        assert(pad.mTrigLockR && (pad.mButtonFlags&partner));
        input(PAD_TRIGGER_R);
        input(chord|PAD_BUTTON_B,partner);
        assert(s_state.active && (pad.mButtonFlags&PAD_BUTTON_B));
        // Native shortcuts also accept R and a fresh partner in the same tick.
        fresh(link); input(chord,chord);
        assert(s_state.active && pad.mTrigLockR && pad.mHoldLockR);
        // Native menus/cutscenes/restricted forms never toggle or steal the chord.
        for(int blocked=0;blocked<6;++blocked) {
            fresh(link);
            if(blocked==0) paused=true;
            if(blocked==1) enabled=false;
            if(blocked==2) link.wolf=true;
            if(blocked==3) link.dead=true;
            if(blocked==4) link.event=true;
            if(blocked==5) link.sceneChange=true;
            input(chord,chord); assert(!s_state.active && pad.mButtonFlags==chord);
            paused=false; enabled=true; link.wolf=link.dead=link.event=link.sceneChange=false;
            input(chord); assert(!s_state.active); // no deferred activation on resume
            input(0); input(chord,chord); assert(s_state.active);
        }
        fresh(link); input(chord,0); assert(!s_state.active); // no phantom press after owner reset
        // Spin cannot activate when a button binding is selected.
        link.mProcID=daAlink_c::PROC_CUT_TURN_CHARGE; update_spin_activation(&link);
        link.mProcID=daAlink_c::PROC_CUT_TURN; link.cut=true; update_spin_activation(&link);
        assert(!s_state.active);
    }
    binding=FierceDeityActivation::SpinAttack; fresh(link);
    input(PAD_TRIGGER_R|PAD_TRIGGER_Z|PAD_BUTTON_A,PAD_TRIGGER_R|PAD_TRIGGER_Z|PAD_BUTTON_A);
    assert(!s_state.active && pad.mHoldLockR && pad.mButtonFlags);
    link.mProcID=daAlink_c::PROC_CUT_TURN_CHARGE; update_spin_activation(&link);
    assert(s_state.spinChargeArmed);
    link.mProcID=daAlink_c::PROC_CUT_TURN; link.cut=true; update_spin_activation(&link);
    assert(s_state.active);
    s_state.meter=0.1f; s_state.lastDrainTime=Clock::now()-std::chrono::seconds(1);
    update_drain(&link); assert(!s_state.active && s_state.meter==0);
    // Save replacement/deletion still clears partial charge and input ownership.
    s_state.meter=40; on_save_started(nullptr,0,nullptr); assert(s_state.meter==0 && !s_state.link);
    currentLink=nullptr; input(0); assert(!fierce_deity_input_consumed());
}
'''
config = (root/'src/config.cpp').read_text()
ui = (root/'src/ui.cpp').read_text()
assert 'register_int("fierce-deity-activation", 1, s_fierceDeityActivation)' in config
assert 'get_int(s_fierceDeityActivation, 1, 0, 2)' in config
assert '{"Spin Attack", "R+Z", "R+A"}' in ui
assert ui.index('"Fierce Deity Visual",') < ui.index('"Fierce Deity Activation",') < ui.index('"Great Spin Projectile",')
assert 'fierce_deity_visual_disabled' in ui[ui.index('"Fierce Deity Activation",'):ui.index('"Great Spin Projectile",')]
with tempfile.TemporaryDirectory(prefix='dawnlight-fierce-input-') as directory:
    cpp, exe = Path(directory)/'test.cpp', Path(directory)/'test'
    cpp.write_text(fixture+state+preload_state+production+checks)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Fierce Deity activation passed: bindings, consumption, held/release edges, pause, early stop and refill')
