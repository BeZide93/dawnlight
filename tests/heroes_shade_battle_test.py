"""Exercise progression, counter/combo scheduling and target-relative movement."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = r'''
#include "heroes_shade_battle.hpp"
#include <cassert>
#include <cstdint>
#include <set>
using namespace dawnlight::shade;
float keep_order(float count) { return count-1; }
std::uint32_t seed=1;
unsigned random_calls=0;
float random_value(float count) {
    ++random_calls;
    seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
    return static_cast<float>(seed>>8)*(1.0f/16777216.0f)*count;
}
void check_random_orders() {
    std::set<unsigned> openings,trial_orders;
    for (unsigned run=1;run<=256;++run) {
        seed=run*0x9e3779b9u;
        Battle b;b.begin(random_value);
        openings.insert(b.phase);
        unsigned code=0;
        for(auto trial:b.trial_order) code=code*5+static_cast<unsigned>(trial);
        trial_orders.insert(code);
        const auto trials=b.trial_order;
        assert(std::find(trials.begin(),trials.end(),Trial::Fire)<
               std::find(trials.begin(),trials.end(),Trial::Wind));
        const auto first_cycle=b.phase_order;
        unsigned seen_trials=0;
        // Full fights keep counter identities and HP triggers under every order.
        for(unsigned hit=0;hit<10;++hit) {
            const auto phase=b.phase;
            if(hit<8) assert(phase==first_cycle[hit]);
            assert(b.audio_trial()==b.next_trial());
            b.event(phases[phase].success);
            for(int i=0;i<44;++i) assert(!b.tick(random_value));
            assert(b.tick(random_value));
            assert(b.health==9-static_cast<int>(hit));
            if(hit%2==1 && hit<8) {
                const auto expected=trials[hit/2];
                if(expected==Trial::Wind) assert(seen_trials&(1u<<static_cast<unsigned>(Trial::Fire)));
                assert(b.trial==expected && b.audio_trial()==expected);
                const unsigned bit=1u<<static_cast<unsigned>(expected);
                assert(!(seen_trials&bit));seen_trials|=bit;
                const auto calls=random_calls;
                const auto timer=b.remaining;
                for(int tick=0;tick<100;++tick) {
                    b.event(phases[phase].success);
                    assert(!b.tick(random_value));
                    assert(b.audio_trial()==expected);
                }
                assert(b.remaining==timer && b.health==9-static_cast<int>(hit));
                assert(random_calls==calls); // no frame/prefetch-dependent draws
                b.trial=Trial::None;
                assert(b.tick(random_value));
            }
            if(hit<9) assert(b.phase!=phase);
        }
        assert(seen_trials==30 && b.dying && b.next_trial()==Trial::None);
        assert(b.audio_trial()==Trial::None);
        b.defeated_doubles=6;b.begin(random_value); // replay clears all progress
        assert(std::find(b.trial_order.begin(),b.trial_order.end(),Trial::Fire)<
               std::find(b.trial_order.begin(),b.trial_order.end(),Trial::Wind));
        assert(b.health==10 && !b.dying && !b.advance && !b.recovery);
        assert(b.phase_cursor==0 && b.phase==b.phase_order[0]);
        assert(!b.defeated_doubles && !b.trials_started && b.remaining==600);
        unsigned previous=99;
        for(int cycle=0;cycle<32;++cycle) {
            unsigned seen=0;
            for(unsigned slot=0;slot<8;++slot) {
                assert(b.phase<8 && b.phase!=previous && !(seen&(1u<<b.phase)));
                seen|=1u<<b.phase;previous=b.phase;
                b.remaining=1;b.defeated_doubles=6;
                assert(b.tick(random_value));
                assert(!b.defeated_doubles && b.remaining==600 && b.health==10);
                assert(b.trial==Trial::None && b.trials_started==0);
            }
            assert(seen==255);
        }
    }
    assert(openings.size()==8 && trial_orders.size()==12);
    // Force a shuffle that would repeat the last phase across a bag boundary.
    Battle boundary;boundary.phase_order={7,0,1,2,3,4,5,6};
    boundary.phase_cursor=7;boundary.phase=7;boundary.remaining=1;
    assert(boundary.tick(keep_order) && boundary.phase!=7);
    auto sorted=boundary.phase_order;std::sort(sorted.begin(),sorted.end());
    for(unsigned i=0;i<8;++i) assert(sorted[i]==i);
    // A floating-point upper endpoint cannot index past the bag.
    Battle rounded;rounded.begin([](float count){return count;});
    assert(rounded.phase==0 && rounded.next_trial()==Trial::Shield);
}
int main() {
    check_random_orders();
    Battle battle;
    battle.begin(keep_order);
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
        for (int i=0;i<44;++i) assert(!battle.tick(keep_order));
        assert(battle.tick(keep_order));
        if (hit%2==1 && hit<8) {
            assert(static_cast<unsigned>(battle.trial)==(hit+1)/2);
            const int remaining=battle.remaining;
            for (int i=0;i<1000;++i) {
                battle.event(phases[phase].success);
                assert(!battle.tick(keep_order) && battle.health==hp-1 && !battle.dying);
            }
            assert(battle.remaining==remaining);
            battle.trial=Trial::None; // shield/fire/eyes/wind controller finished
            assert(battle.tick(keep_order));
        } else assert(battle.trial==Trial::None);
        if (hit==7) assert(battle.health==2 && !battle.dying);
    }
    assert(battle.dying && battle.health==0);
    for (int i=0;i<100;++i) {
        battle.event(24);
        assert(!battle.tick(keep_order) && battle.health==0);
    }
    // A missed skill cannot soft-lock the fight or count as a successful hit.
    Battle timeout;
    for (unsigned phase=0;phase<16;++phase) {
        for (int i=0;i<599;++i) assert(!timeout.tick(keep_order));
        assert(timeout.tick(keep_order));
        assert(timeout.health==10 && !timeout.dying && timeout.phase==(phase+1)%8);
    }
    // Timeout starts a fresh phase; defeat slots never leak into later rounds.
    timeout.phase=7;timeout.remaining=1;timeout.defeated_doubles=6;
    assert(timeout.tick(keep_order) && timeout.phase!=7 && timeout.defeated_doubles==0);
    timeout.defeated_doubles=6;timeout={};assert(timeout.defeated_doubles==0);
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
