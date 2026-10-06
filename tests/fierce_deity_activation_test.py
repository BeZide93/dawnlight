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
bool directTouch=false;
bool consume_dark_link_touch_press(){bool v=directTouch;directTouch=false;return v;}
constexpr u32 PAD_BUTTON_A=0x100, PAD_TRIGGER_R=0x20, PAD_TRIGGER_Z=0x10, PAD_BUTTON_B=0x200;
constexpr int PAD_1=0;
struct Pad { u32 mButtonFlags=0, mPressedButtonFlags=0, mHoldLockR=0, mTrigLockR=0; float mTriggerRight=0; } pad;
struct mDoCPd_c { static Pad& getCpadInfo(int) { return pad; } };
constexpr u32 PAD_BUTTON_LEFT_STICK=0x4000000, PAD_BUTTON_RIGHT_STICK=0x2000000;
constexpr int PAD_ERR_NONE=0;
struct RawPad { u32 extButton=0; int err=PAD_ERR_NONE; };
struct JUTGamePad { inline static RawPad mPadStatus[4]{}; };
enum class FierceDeityActivation : int { SpinAttack, RZ, RA, L3, R3 };
FierceDeityActivation binding=FierceDeityActivation::RZ;
FierceDeityActivation fierce_deity_activation() { return binding; }
'''
production = callbacks + ''.join(function(n) for n in (
    'before_fierce_game_combos', 'update_spin_activation', 'update_drain', 'fierce_deity_input_consumed',
    'fierce_deity_touch_button', 'after_fierce_midna_trigger'))
checks = r'''
void input(u32 held, u32 pressed=0, bool analog=false) {
    pad={held,pressed,(held&PAD_TRIGGER_R)!=0,(pressed&PAD_TRIGGER_R)!=0,1.0f};
    if(analog) { pad.mButtonFlags &= ~PAD_TRIGGER_R; pad.mPressedButtonFlags &= ~PAD_TRIGGER_R; }
    assert(before_fierce_game_combos(nullptr,nullptr,nullptr,nullptr)==HOOK_CONTINUE);
}
void fresh(daAlink_c& link) {
    link={}; currentLink=&link; enabled=true; paused=false;
    reset_for_link(&link); s_state.meter=100;
    JUTGamePad::mPadStatus[0]={}; input(0);
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
            input(partner); assert(!(pad.mButtonFlags&partner)); // partner is owned until release
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
    for(auto mode:{FierceDeityActivation::L3,FierceDeityActivation::R3}) {
        binding=mode;fresh(link);
        const u32 selected=mode==FierceDeityActivation::L3?PAD_BUTTON_LEFT_STICK:PAD_BUTTON_RIGHT_STICK;
        const u32 other=selected^ (PAD_BUTTON_LEFT_STICK|PAD_BUTTON_RIGHT_STICK);
        auto click=[&](u32 buttons){JUTGamePad::mPadStatus[0].extButton=buttons;input(0);};
        // These bits in the normal interface are stick directions, never clicks.
        input(selected,selected);assert(!s_state.active&&pad.mButtonFlags==selected);
        input(PAD_TRIGGER_R|PAD_TRIGGER_Z|PAD_BUTTON_A,PAD_TRIGGER_Z|PAD_BUTTON_A);
        assert(!s_state.active); // R chords cannot trigger a stick binding.
        JUTGamePad::mPadStatus[1].extButton=selected;input(0);assert(!s_state.active);
        click(other);assert(!s_state.active);click(0);
        click(selected);assert(s_state.active&&fierce_deity_input_consumed());
        for(int i=0;i<60;++i)click(selected);
        assert(s_state.active&&!fierce_deity_input_consumed());
        click(0);s_state.meter=45;click(selected);assert(!s_state.active&&s_state.meter==45);
        click(0);click(selected);assert(!s_state.active); // insufficient meter
        s_state.meter=100;click(selected);assert(!s_state.active); // no deferred activation
        click(0);click(selected);assert(s_state.active);
        for(int blocked=0;blocked<6;++blocked) {
            fresh(link);
            if(blocked==0) paused=true;
            if(blocked==1) enabled=false;
            if(blocked==2) link.wolf=true;
            if(blocked==3) link.dead=true;
            if(blocked==4) link.event=true;
            if(blocked==5) link.sceneChange=true;
            click(selected);assert(!s_state.active&&!fierce_deity_input_consumed());
            paused=false;enabled=true;link.wolf=link.dead=link.event=link.sceneChange=false;
            click(selected);assert(!s_state.active);
            click(0);click(selected);assert(s_state.active);
        }
        fresh(link);JUTGamePad::mPadStatus[0].err=-1;click(selected);assert(!s_state.active);
        JUTGamePad::mPadStatus[0].err=PAD_ERR_NONE;click(selected);assert(!s_state.active);
        click(0);click(selected);assert(s_state.active);
        fresh(link);binding=FierceDeityActivation::SpinAttack;click(selected);
        binding=mode;click(selected);assert(!s_state.active);
        click(0);click(selected);assert(s_state.active);
        // Reset/save replacement cannot turn an already-held click into an edge.
        reset_for_link(&link);s_state.meter=100;click(selected);assert(!s_state.active);
        click(0);click(selected);assert(s_state.active);
    }
    // Touch-only path: simulate HD HUD removing Z/R after pad read. Repeated
    // visual sync callbacks and release before sampling must not erase a tap.
    binding=FierceDeityActivation::RZ; fresh(link);
    fierce_deity_touch_button(PAD_TRIGGER_R,true); input(0);
    assert(!s_state.active);
    fierce_deity_touch_button(PAD_TRIGGER_Z,true);
    fierce_deity_touch_button(PAD_TRIGGER_Z,true); // duplicate UI notification
    fierce_deity_touch_button(PAD_TRIGGER_Z,false); // quick tap between sim ticks
    input(0); assert(s_state.active && fierce_deity_input_consumed());
    BOOL midna=1; after_fierce_midna_trigger(nullptr,nullptr,&midna,nullptr); assert(midna==0);
    input(0); assert(s_state.active && !fierce_deity_input_consumed());
    midna=1; after_fierce_midna_trigger(nullptr,nullptr,&midna,nullptr); assert(midna==1);
    s_state.meter=41;
    fierce_deity_touch_button(PAD_TRIGGER_Z,true); input(0);
    assert(!s_state.active && s_state.meter==41);
    for(int i=0;i<5;++i) { fierce_deity_touch_button(PAD_TRIGGER_Z,true); input(0); }
    assert(!s_state.active); // held touch never becomes repeat presses
    fierce_deity_touch_button(PAD_TRIGGER_Z,false);
    fierce_deity_touch_button(PAD_TRIGGER_R,false); input(0);
    s_state.meter=100; paused=true;
    fierce_deity_touch_button(PAD_TRIGGER_R,true);
    fierce_deity_touch_button(PAD_TRIGGER_Z,true); input(0);
    paused=false; input(0); assert(!s_state.active); // menu tap not deferred
    // R+A must remain consumed on later held ticks too (native Moon Jump).
    binding=FierceDeityActivation::RA; fresh(link);
    fierce_deity_touch_button(PAD_TRIGGER_R,true);
    fierce_deity_touch_button(PAD_BUTTON_A,true);
    for(int i=0;i<5;++i) {
        input(PAD_TRIGGER_R|PAD_BUTTON_A,i==0?PAD_BUTTON_A:0);
        assert(s_state.active && !(pad.mButtonFlags&PAD_BUTTON_A) && pad.mHoldLockR);
    }
    fierce_deity_touch_button(PAD_BUTTON_A,false);
    input(PAD_TRIGGER_R); assert(!fierce_deity_input_consumed());
    binding=FierceDeityActivation::SpinAttack; fresh(link);
    input(PAD_TRIGGER_R|PAD_TRIGGER_Z|PAD_BUTTON_A,PAD_TRIGGER_R|PAD_TRIGGER_Z|PAD_BUTTON_A);
    assert(!s_state.active && pad.mHoldLockR && pad.mButtonFlags);
    link.mProcID=daAlink_c::PROC_CUT_TURN_CHARGE; update_spin_activation(&link);
    assert(s_state.spinChargeArmed);
    link.mProcID=daAlink_c::PROC_CUT_TURN; link.cut=true; update_spin_activation(&link);
    assert(s_state.active);
    s_state.meter=0.1f; s_state.lastDrainTime=Clock::now()-std::chrono::seconds(1);
    update_drain(&link); assert(!s_state.active && s_state.meter==0);
    for(auto mode:{FierceDeityActivation::SpinAttack,FierceDeityActivation::RZ,FierceDeityActivation::RA,
                   FierceDeityActivation::L3,FierceDeityActivation::R3}) {
        binding=mode;fresh(link);directTouch=true;input(0);
        assert(s_state.active&&s_state.meter==100&&pad.mButtonFlags==0&&s_state.consumedPartner==0);
        input(0);assert(s_state.active); // held input is not a second toggle
        s_state.meter=45;directTouch=true;input(0);assert(!s_state.active&&s_state.meter==45);
        directTouch=true;input(0);assert(!s_state.active); // activation still needs a full meter
        s_state.meter=100;input(0);assert(!s_state.active); // no deferred activation
        for(int blocked=0;blocked<6;++blocked) {
            fresh(link);
            if(blocked==0) paused=true;
            if(blocked==1) enabled=false;
            if(blocked==2) link.wolf=true;
            if(blocked==3) link.dead=true;
            if(blocked==4) link.event=true;
            if(blocked==5) link.sceneChange=true;
            directTouch=true;input(0);assert(!s_state.active&&!directTouch);
            paused=false;enabled=true;link.wolf=link.dead=link.event=link.sceneChange=false;
            input(0);assert(!s_state.active);
        }
    }
    // Gauge uses configurable points; all activation paths require that capacity.
    settings[static_cast<size_t>(DarkLinkSetting::Gauge)]=200;
    settings[static_cast<size_t>(DarkLinkSetting::SwordGain)]=25;
    binding=FierceDeityActivation::RA;fresh(link);
    input(PAD_TRIGGER_R|PAD_BUTTON_A,PAD_BUTTON_A);assert(!s_state.active);
    for(int i=0;i<4;++i)hit(link);
    assert(s_state.meter==200&&gauge_percentage()==100);
    input(0);input(PAD_TRIGGER_R|PAD_BUTTON_A,PAD_BUTTON_A);assert(s_state.active&&s_state.meter==200);
    settings[static_cast<size_t>(DarkLinkSetting::Depletion)]=20;
    s_state.lastDrainTime=Clock::now()-std::chrono::milliseconds(200);
    update_drain(&link);assert(s_state.meter>195.5f&&s_state.meter<196.5f);
    settings[static_cast<size_t>(DarkLinkSetting::Depletion)]=0;
    const float charge=s_state.meter;s_state.lastDrainTime=Clock::now()-std::chrono::seconds(1);
    update_drain(&link);assert(s_state.meter==charge&&s_state.active);
    settings[static_cast<size_t>(DarkLinkSetting::Gauge)]=50;input(0);
    assert(s_state.meter==50&&gauge_percentage()==100);
    settings[static_cast<size_t>(DarkLinkSetting::Gauge)]=200;input(0);
    assert(s_state.meter==50&&gauge_percentage()==25);
    // Save replacement/deletion still clears partial charge and input ownership.
    s_state.meter=40; on_save_started(nullptr,0,nullptr); assert(s_state.meter==0 && !s_state.link);
    currentLink=nullptr; input(0); assert(!fierce_deity_input_consumed());
}
'''
config = (root/'src/config.cpp').read_text()
ui = (root/'src/ui.cpp').read_text()
assert 'register_int("fierce-deity-activation", 2, s_fierceDeityActivation)' in config
assert 'get_int(s_fierceDeityActivation, 2, 0, 4)' in config
assert 'register_int("fierce-deity-visual", 1, s_fierceDeityVisual)' in config
assert 'get_int(s_fierceDeityVisual, 1, 0, 4)' in config
assert '{"Spin Attack", "R+Z", "R+A", "L3", "R3"}' in ui
assert '{"Magic Armor", "Dark", "Dark Magic", "White", "Gold"}' in ui
assert 'White = 3, Gold = 4' in (root/"src/config.hpp").read_text()
assert ui.index('"Dark Link Visual",') < ui.index('"Dark Link Activation",') < ui.index('"Great Spin Projectile",')
assert 'fierce_deity_visual_disabled' in ui[ui.index('"Dark Link Activation",'):ui.index('"Great Spin Projectile",')]
with tempfile.TemporaryDirectory(prefix='dawnlight-fierce-input-') as directory:
    cpp, exe = Path(directory)/'test.cpp', Path(directory)/'test'
    cpp.write_text(fixture+state+preload_state+production+checks)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Fierce Deity activation passed: bindings, consumption, held/release edges, pause, early stop and refill')
