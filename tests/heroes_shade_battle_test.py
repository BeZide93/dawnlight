"""Exercise progression, counter/combo scheduling and target-relative movement."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = r'''
#include "heroes_shade_battle.hpp"
#include <cassert>
using namespace dawnlight::shade;
int main() {
    Battle battle;
    // Every native success advances once; repeated events cannot drain HP.
    for (unsigned hit=0; hit<10; ++hit) {
        const unsigned phase=hit%phases.size();
        assert(battle.phase==phase);
        const int hp=battle.health;
        battle.event(1); // teacher timeout / lecture
        battle.event(4); // Link got hit
        assert(battle.health==hp && battle.recovery==0);
        battle.event(phases[phase].success);
        assert(battle.health==hp-1);
        battle.event(phases[phase].success);
        assert(battle.health==hp-1);
        for (int i=0;i<44;++i) assert(!battle.tick());
        assert(battle.tick());
        if (hit%2==1 && hit<8) {
            assert(static_cast<unsigned>(battle.trial)==(hit+1)/2);
            const int remaining=battle.remaining;
            for (int i=0;i<1000;++i) {
                battle.event(phases[phase].success);
                assert(!battle.tick() && battle.health==hp-1 && !battle.dying);
            }
            assert(battle.remaining==remaining);
            battle.trial=Trial::None; // shield/fire/eyes/wind controller finished
            assert(battle.tick());
        } else assert(battle.trial==Trial::None);
        if (hit==7) assert(battle.health==2 && !battle.dying);
    }
    assert(battle.dying && battle.health==0);
    for (int i=0;i<100;++i) {
        battle.event(24);
        assert(!battle.tick() && battle.health==0);
    }
    // A missed skill cannot soft-lock the fight or count as a successful hit.
    Battle timeout;
    for (unsigned phase=0;phase<16;++phase) {
        for (int i=0;i<599;++i) assert(!timeout.tick());
        assert(timeout.tick());
        assert(timeout.health==10 && !timeout.dying && timeout.phase==(phase+1)%8);
    }
    // Replay starts independently; no defeated flags or lesson save bits needed.
    battle={};
    assert(battle.health==10 && battle.phase==0 && !battle.dying);
    for (unsigned phase=0;phase<phases.size();++phase) {
        const int attack=phases[phase].attack;
        if (attack<0) continue; // native projectile owns its collision
        const int step=attack==back_slice ? 2 : 0;
        BladeMotion blade;
        std::array<BladePoint,2> pose{{{60,100,0},{120,100,0}}};
        assert(!blade.sample(attack,step,0,pose)); // seed from the actual model
        pose[0].z=pose[1].z=12;
        assert(blade.sample(attack,step,5,pose)); // early physical swing
        assert(!blade.sample(attack,step,6,pose)); // held blade cannot hurt
        pose[0].z=pose[1].z=24;
        assert(blade.sample(attack,step,55,pose)); // late swing is not discarded
        pose[0].z=pose[1].z=36;
        assert(!blade.sample(attack,step,55,pose)); // frozen animation
        pose[0].z=pose[1].z=1000;
        assert(!blade.sample(attack,step,56,pose)); // discontinuity
        blade.contact();
        pose[0].z=pose[1].z=1012;
        assert(!blade.sample(attack,step,57,pose)); // shield/hit consumes strike
        blade={}; // next attack starts afresh, without a sweep from the last one
        assert(!blade.sample(attack,step,0,pose));
        pose[0].z=pose[1].z=1024;
        assert(blade.sample(attack,step,1,pose));
    }
    assert(!striking_step(back_slice,0)); // sidestep is not the cut
    assert(!striking_step(back_slice,1)); // neither is the roll
    assert(striking_step(back_slice,2));
    assert(!striking_step(back_slice,3)); // ready pose
    assert(!striking_step(-1,0));
    assert(!striking_step(999,0));
    BladeMotion ordinary;
    std::array<BladePoint,2> ordinary_pose{{{60,100,0},{120,100,0}}};
    assert(!ordinary.sample(sword,0,0,ordinary_pose));
    assert(!ordinary.sample(sword,0,29,ordinary_pose));
    assert(ordinary.sample(sword,0,30,ordinary_pose));
    assert(ordinary.sample(sword,0,40,ordinary_pose));
    assert(!ordinary.sample(sword,0,41,ordinary_pose));
    assert(!ordinary.sample(sword,1,35,ordinary_pose));

    BladeMotion cut;
    auto pose=ordinary_pose;
    assert(!cut.sample(back_slice,1,5,pose));
    assert(cut.sample(back_slice,2,0,pose) && !cut.sweep);
    pose[0].z=pose[1].z=20;
    assert(cut.sample(back_slice,2,1,pose) && cut.sweep);

    BladeMotion jump;
    assert(!jump.sample(jump_strike,0,0,pose));
    jump.contact();
    for (int frame=1;frame<=10;++frame) {
        pose[0].z=pose[1].z=frame*10;
        assert(!jump.sample(jump_strike,0,frame,pose,false));
    }
    pose[0].z=pose[1].z=150;
    assert(!jump.sample(jump_strike,0,11,pose,true));
    pose[0].z=pose[1].z=180;
    assert(jump.sample(jump_strike,0,12,pose,true));
    jump.contact();
    for (int frame=13;frame<30;++frame) {
        pose[0].z=pose[1].z=frame*10;
        assert(!jump.sample(jump_strike,0,frame,pose,true));
    }
    assert(jump.contacts==2);
    const BladePoint body{0,0,0};
    assert(!blade_clear_of_body({{{-100,100,0},{100,100,0}}},body,30,150));
    assert(!blade_clear_of_body({{{60,100,0},{120,100,0}}},body,30,150));
    assert(blade_clear_of_body({{{60,200,0},{120,200,0}}},body,30,150));
    for (float angle : {0.0f,0.7f,1.57f,3.14f,-2.4f}) {
        const GroundPoint link{std::sin(angle)*300,std::cos(angle)*300};
        const auto helm=jump_landing_target(helm_splitter,{0,0},link);
        const auto jumpEnd=jump_landing_target(jump_strike,{0,0},link);
        assert(std::abs(std::hypot(helm.x,helm.z)-440)<0.01f);
        assert(std::abs(std::hypot(jumpEnd.x,jumpEnd.z)-190)<0.01f);
    }
    const auto same=jump_landing_target(helm_splitter,{0,0},{0,0});
    assert(same.x==0 && same.z==0);

    // The native sequence keeps its number after the swing/block. These
    // returns must admit another attack without cutting off the reaction.
    for (int motion : {sword,27,24,31}) {
        assert(!ready_motion(motion,0));
        assert(ready_motion(motion,1));
    }
    assert(ready_motion(9,0));
    for (int motion : {11,18,19,28,30,31}) assert(!ready_motion(motion,0));

    // Every second confirmed block queues one riposte. Cancelling an attack
    // does not forget the first block, and two separate pairs stay distinct.
    AttackChain counter;
    counter.blocked_sword();
    assert(counter.blocks==1 && counter.counters==0);
    counter.cancel();
    counter.blocked_sword();
    assert(counter.blocks==0 && counter.counters==1);
    counter.blocked_sword(); counter.blocked_sword();
    assert(counter.counters==2);
    for (int n=0;n<2;++n) {
        counter.begin(0);
        assert(counter.next()==sword);
        assert(counter.next()==back_slice);
        assert(counter.next()==-1);
    }
    assert(counter.counters==0);

    // Even the opening lesson gets both jumps and Back Slice, with two sword
    // attacks per combo. The lesson's own move remains in the rotation.
    for (unsigned phase=0;phase<phases.size();++phase) {
        if (phases[phase].attack<0) continue;
        AttackChain combo;
        const int finishers[]={helm_splitter,back_slice,jump_strike,phases[phase].attack};
        for (int finisher : finishers) {
            combo.begin(phase);
            assert(combo.next()==sword);
            assert(combo.next()==sword);
            assert(combo.next()==finisher);
            assert(combo.next()==-1);
        }
    }

    // Rotate the same maneuver through all directions: the roll must end on
    // Link's opposite side at sword reach, not drift along Shade's old facing.
    for (float angle : {0.0f,0.7f,1.57f,3.14f,-2.4f}) {
        auto point=back_slice_offset(angle,280,0,0);
        const auto finish=back_slice_offset(angle,280,1,1);
        assert(point.x*finish.x+point.z*finish.z<0);
        assert(std::abs(std::hypot(finish.x,finish.z)-150)<0.01f);
        const auto seam0=back_slice_offset(angle,280,0,1);
        const auto seam1=back_slice_offset(angle,280,1,0);
        assert(std::hypot(seam0.x-seam1.x,seam0.z-seam1.z)<0.01f);
        for (int step=0;step<2;++step) {
            for (int frame=1;frame<=30;++frame) {
                const auto target=back_slice_offset(angle,280,step,frame/30.0f);
                const auto v=approach_velocity(target.x-point.x,target.z-point.z,14,0);
                assert(std::hypot(v.x,v.z)<=14.001f);
                point.x+=v.x; point.z+=v.z;
            }
        }
        assert(std::hypot(point.x-finish.x,point.z-finish.z)<15);
    }
    // Never overshoot the target or divide by zero during the follow-up cut.
    const auto stopped=approach_velocity(0,0,10,120);
    assert(stopped.x==0 && stopped.z==0);
    const auto near=approach_velocity(123,0,10,120);
    assert(std::abs(near.x-3)<0.001f && near.z==0);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/"test.cpp"
    exe=Path(tmp)/"test"
    cpp.write_text(source)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-I", str(root/"src"), str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Hero's Shade progression, blocks, combos, blade contact, readiness and targeted movement: passed")
