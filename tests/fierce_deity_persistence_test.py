"""Exercise meter retention and per-slot SaveService callbacks from production."""
from pathlib import Path
import os
import subprocess
import tempfile
from fierce_deity_lifecycle_test import fixture, state, preload_state, callbacks, function, source

fixture = fixture.replace('void update_drain(daAlink_c*) { ++drainUpdates; }',
                          'void update_drain(daAlink_c*);')
production = callbacks + function('update_drain')
checks = r'''
int main() {
    daAlink_c link; currentLink = &link; reset_for_link(&link);
    s_state.meter = 37.625f;
    s_state.activationInputConsumed = true;
    void* deletion[] = {static_cast<leafdraw_class*>(&link)};
    before_player_delete(nullptr, deletion, nullptr, nullptr);
    assert(!s_state.link && s_state.meter == 37.625f && !s_state.activationInputConsumed);
    // Multiple no-player frames and allocator address reuse cannot erase charge.
    reset_for_link(nullptr); ++link.id; reset_for_link(&link);
    assert(same_link(&link) && s_state.meter == 37.625f && !s_state.active);

    for (int context=0; context<3; ++context) {
        link={}; reset_for_link(&link); s_state.active=true; s_state.meter=42.75f;
        link.dead=context==0; link.sceneChange=context==1; link.wolf=context==2;
        s_state.lastDrainTime=Clock::now()-std::chrono::seconds(1);
        update_drain(&link);
        assert(!s_state.active && s_state.meter==42.75f);
        assert(s_state.lastDrainTime.time_since_epoch().count()==0);
        before_player_delete(nullptr,deletion,nullptr,nullptr);
        link={};++link.id;reset_for_link(&link);
        assert(s_state.meter==42.75f && !s_state.active);
    }
    // Temporarily disabling the feature must not destroy accumulated progress.
    enabled=false;
    void* args[]={const_cast<process_method_class*>(&playerMethods),&link};
    before_player_execute(nullptr,args,nullptr,nullptr);
    assert(s_state.meter==42.75f);enabled=true;

    // Saving while active captures the remaining charge, not the activation refill.
    s_state.active=true;s_state.meter=37.625f;
    on_save_written(nullptr,0,nullptr);
    assert(s_state.active && s_state.meter==37.625f);
    const auto saved=saveService.slots;
    s_state={}; // fresh process / module state
    on_save_started(nullptr,0,nullptr);
    assert(s_state.meter==37.625f && !s_state.link && !s_state.active);
    reset_for_link(&link);assert(s_state.meter==37.625f);
    // Explicit reload restores the last saved value, not unsaved runtime charge.
    s_state.meter=90;on_save_started(nullptr,0,nullptr);assert(s_state.meter==37.625f);

    saveService.slot=1;on_save_started(nullptr,1,nullptr);assert(s_state.meter==0);
    s_state.meter=81.125f;on_save_written(nullptr,1,nullptr);
    saveService.slot=0;on_save_started(nullptr,0,nullptr);assert(s_state.meter==37.625f);
    saveService.slot=1;on_save_started(nullptr,1,nullptr);assert(s_state.meter==81.125f);
    // Native new-save notification happens after clearing that slot's blobs.
    saveService.slots[1].clear();on_save_started(nullptr,1,nullptr);assert(s_state.meter==0);
    saveService.slot=0;bossRush=true;on_save_started(nullptr,0,nullptr);assert(s_state.meter==0);
    s_state.meter=63;on_save_written(nullptr,0,nullptr);
    bossRush=false;on_save_started(nullptr,0,nullptr);assert(s_state.meter==37.625f);
    bossRush=true;on_save_started(nullptr,0,nullptr);assert(s_state.meter==63);
    bossRush=false;

    // Capacity changes clamp stored points; invalid/unknown blobs never leak state.
    settings[static_cast<size_t>(DarkLinkSetting::Gauge)]=25;
    on_save_started(nullptr,0,nullptr);assert(s_state.meter==25);
    settings[static_cast<size_t>(DarkLinkSetting::Gauge)]=100;
    for (auto bytes : {std::vector<uint8_t>{},std::vector<uint8_t>{1},
                      std::vector<uint8_t>{2,1,0,0,0},std::vector<uint8_t>{1,255,255,255,255},
                      std::vector<uint8_t>{1,0,0,0,0,0}}) {
        saveService.slots[0][meter_blob_name()]=bytes;s_state.meter=99;
        on_save_started(nullptr,0,nullptr);assert(s_state.meter==0);
    }
    saveService.slots=saved;
    for (float invalid : {-10.f,INFINITY,NAN}) {
        s_state.meter=invalid;on_save_written(nullptr,0,nullptr);
        assert(load_saved_meter()==0);
    }
    settings[static_cast<size_t>(DarkLinkSetting::Gauge)]=10000;
    s_state.meter=10000;on_save_written(nullptr,0,nullptr);assert(load_saved_meter()==10000);
    saveService.failWrite=true;s_state.meter=44;
    on_save_written(nullptr,0,nullptr);
    assert(s_state.meter==44 && load_saved_meter()==10000 && logService.warnings==1);
    svc_save=nullptr;on_save_started(nullptr,0,nullptr);assert(s_state.meter==0);
}
'''
assert 'on_save_started, on_save_started, on_save_written' in source
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture + state + preload_state + production + checks)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(cpp),'-o',str(exe)],check=True)
    env=dict(os.environ);env.setdefault('ASAN_OPTIONS','detect_leaks=0')
    subprocess.run([str(exe)],check=True,env=env)
print('Fierce Deity meter: actor/scene/death/wolf retention, saved charge, slots, Boss Rush, validation and write failures passed')
