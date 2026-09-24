"""Exercise the production save-to-progression mapping and effective settings."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/progression.cpp').read_text()
start = source.index('ProgressionState progression_state()')
function = source[start:source.index('\n}', start) + 2]
fixture = r'''
#include "general_modes.hpp"
#include <cassert>
using namespace dawnlight;
struct dSv_event_flag_c { enum {F_0026, M_016, M_019}; };
bool bossRush=false, flags[3]{};
int maxLife=15;
bool save_state_boss_rush_active(){return bossRush;}
bool dComIfGs_isEventBit(int flag){return flags[flag];}
int dComIfGs_getMaxLife(){return maxLife;}
// MAPPING
int main(){
    auto state=progression_state();
    assert(!state.glide&&!state.gale&&!state.fierceDeity&&state.charges==1);
    bossRush=true;
    for(int hearts:{3,5,6,9,20}){
        maxLife=hearts*5;state=progression_state();
        assert(state.glide&&state.gale&&state.fierceDeity);
        assert(state.charges==hearts/3);
        for(bool dawnlight:{false,true}){
            int64_t value=0;
            for(auto setting:{ModeSetting::Sprint,ModeSetting::Glide,ModeSetting::GlideItem,
                    ModeSetting::Gale,ModeSetting::GaleCounter,ModeSetting::FierceDeity})
                assert(mode_override(setting,dawnlight,true,state,value)&&value==1);
            assert(mode_override(ModeSetting::GaleCharges,dawnlight,true,state,value)&&value==hearts/3);
            if(!dawnlight)assert(!mode_override(ModeSetting::Glide,false,false,state,value));
        }
    }
    assert(!flags[0]&&!flags[1]&&!flags[2]); // Never alter the story save.
    bossRush=false;state=progression_state();
    assert(!state.glide&&!state.gale&&!state.fierceDeity);
    flags[0]=true;state=progression_state();assert(state.glide&&!state.gale&&!state.fierceDeity);
    flags[1]=true;state=progression_state();assert(state.glide&&state.gale&&!state.fierceDeity);
    flags[2]=true;state=progression_state();assert(state.glide&&state.gale&&state.fierceDeity);
}
'''.replace('// MAPPING', function)
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Boss Rush progression: immediate abilities, heart-based charges, unchanged story flags and Off settings passed')
