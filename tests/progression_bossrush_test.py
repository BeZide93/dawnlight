"""Exercise the production save-to-progression mapping and effective settings."""
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/progression.cpp').read_text()
start = source.index('ProgressionState progression_state()')
function = source[start:source.index('\n}', start) + 2]
config = (root / 'src/config.cpp').read_text()
def config_function(name):
    start = re.search(r'^\w[^\n]*\b' + name + r'\([^;{}]*\)\s*\{', config, re.M).start()
    depth, end = 1, config.index('{', start) + 1
    while depth:
        depth += (config[end] == '{') - (config[end] == '}')
        end += 1
    return config[start:end]
mapping = config_function('mode_setting_for_config')
variables = sorted(set(re.findall(r'\bs_\w+', mapping)) - {'s_staminaSettings'})
config_code = '\n'.join(f'ConfigVarHandle {name}={i+1};' for i, name in enumerate(variables))
config_code += '\nConfigVarHandle s_dawnlightMode=999;'
for name in ('dawnlight_mode_enabled','progression_system_enabled','mode_setting_for_config',
             'mode_config_override','get_bool','fierce_deity_enabled','flurry_rush_enabled'):
    config_code += '\n' + config_function(name)
fixture = r'''
#include "general_modes.hpp"
#include "stamina_settings.hpp"
#include <map>
#include <cassert>
using namespace dawnlight;
struct dSv_event_flag_c { enum {F_0026, M_016, M_019}; };
bool bossRush=false, flags[3]{};
constexpr int dItemNo_SWORD_e=40;
bool ordonSword=false;
bool dComIfGs_isItemFirstBit(int item){assert(item==dItemNo_SWORD_e);return ordonSword;}
int maxLife=15;
bool save_state_boss_rush_active(){return bossRush;}
bool dComIfGs_isEventBit(int flag){return flags[flag];}
int dComIfGs_getMaxLife(){return maxLife;}
// MAPPING
using ConfigVarHandle=unsigned;
struct ModContext {};
ModContext* mod_ctx=nullptr;
struct ConfigService {
    std::map<ConfigVarHandle,bool> values;
    void get_bool(ModContext*,ConfigVarHandle key,bool* out){*out=values[key];}
} configService;
ConfigService* svc_config=&configService;
std::array<ConfigVarHandle,kStaminaSettings.size()> s_staminaSettings{};
// CONFIG
int main(){
    for(size_t i=0;i<s_staminaSettings.size();++i)s_staminaSettings[i]=1000+i;
    // Exercise the real config handle mapping, including the legacy fierce-deity
    // key retained by the Dark Link rename. Saved On must not bypass a locked save.
    for(bool preset:{false,true})for(bool manual:{false,true}){
        configService.values[s_dawnlightMode]=preset;
        configService.values[s_progressionSystem]=true;
        configService.values[s_fierceDeity]=manual;
        configService.values[s_flurryRush]=true;
        flags[2]=false;assert(!fierce_deity_enabled());assert(flurry_rush_enabled());
        flags[2]=true;assert(fierce_deity_enabled());
        flags[2]=false;assert(!fierce_deity_enabled()); // Load a pre-Faron save.
        bossRush=true;assert(fierce_deity_enabled());
        bossRush=false;assert(!fierce_deity_enabled()); // Return from Boss Rush.
        configService.values[s_dawnlightMode]=false;
        configService.values[s_progressionSystem]=false;
        assert(fierce_deity_enabled()==manual); // Restore the saved manual preference.
    }
    auto state=progression_state();
    assert(!state.glide&&!state.gale&&!state.fierceDeity&&!state.dualWield&&state.charges==1);
    bossRush=true;
    for(int hearts:{3,5,6,9,20}){
        maxLife=hearts*5;state=progression_state();
        assert(state.glide&&state.gale&&state.fierceDeity&&state.dualWield);
        assert(state.charges==hearts/3);
        for(bool dawnlight:{false,true}){
            int64_t value=0;
            for(auto setting:{ModeSetting::Glide,ModeSetting::GlideItem,
                    ModeSetting::Gale,ModeSetting::GaleCounter,ModeSetting::FierceDeity,ModeSetting::DualWield})
                assert(mode_override(setting,dawnlight,true,state,value)&&value==1);
            assert(mode_override(ModeSetting::GaleCharges,dawnlight,true,state,value)&&value==hearts/3);
            if(!dawnlight)assert(!mode_override(ModeSetting::Glide,false,false,state,value));
        }
    }
    assert(!flags[0]&&!flags[1]&&!flags[2]); // Never alter the story save.
    bossRush=false;state=progression_state();
    assert(!state.glide&&!state.gale&&!state.fierceDeity&&!state.dualWield);
    flags[0]=true;state=progression_state();assert(state.glide&&!state.gale&&!state.fierceDeity);
    flags[1]=true;state=progression_state();assert(state.glide&&state.gale&&!state.fierceDeity);
    flags[2]=true;state=progression_state();assert(state.glide&&state.gale&&state.fierceDeity);
    assert(!state.dualWield); // Other story milestones are not the Ordon Sword.
    ordonSword=true;state=progression_state();assert(state.dualWield);
    ordonSword=false;state=progression_state();assert(!state.dualWield); // another save
}
'''.replace('// MAPPING', function).replace('// CONFIG', config_code)
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Boss Rush progression: immediate abilities, heart-based charges, unchanged story flags and Off settings passed')
