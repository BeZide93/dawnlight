"""Exercise production config overrides and UI bindings without a game SDK."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
config = (root / 'src/config.cpp').read_text()
ui = (root / 'src/ui.cpp').read_text()

def function(source, name):
    match = re.search(r'^\w[^\n]*\b' + name + r'\([^;{}]*\)\s*\{', source, re.M)
    start = match.start()
    return source[start:source.index('\n}', start) + 2]

mapping = function(config, 'mode_setting_for_config')
variables = sorted((set(re.findall(r'\bs_\w+', mapping)) | {'s_dawnlightMode'}) - {'s_staminaSettings'})
fixture = r'''
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "general_modes.hpp"
#include "stamina_settings.hpp"
#include "twilit_stamina.hpp"
using namespace dawnlight;
namespace dawnlight {bool teActive=false;bool twilit_stamina_active(){return teActive;}}
using ConfigVarHandle = unsigned;
using UiElementHandle = uint64_t;
struct ModContext {};
ModContext* mod_ctx = nullptr;
struct ConfigService {
    std::map<ConfigVarHandle,int64_t> values;
    void get_bool(ModContext*,ConfigVarHandle v,bool* out){*out=values[v]!=0;}
    void get_int(ModContext*,ConfigVarHandle v,int64_t* out){*out=values[v];}
    void set_bool(ModContext*,ConfigVarHandle v,bool x){values[v]=x;}
    void set_int(ModContext*,ConfigVarHandle v,int64_t x){values[v]=x;}
} service;
ConfigService* svc_config=&service;
ProgressionState progression_state(){return {};}
std::array<ConfigVarHandle,kStaminaSettings.size()> s_staminaSettings;
enum UiControlKind {UI_CONTROL_TOGGLE,UI_CONTROL_NUMBER};
using UiPredicateFn=bool(*)(ModContext*,void*);
struct UiControlValue {bool bool_value=false;int64_t int_value=0;};
// PRODUCTION
int main(){
    const std::array<int,16> expected{100,5,50,5,5,5,5,15,50,10,60,40,20,20,20,50};
    const std::array<int,16> minimum{50,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0};
    const std::array<int,16> maximum{500,100,100,100,20,20,20,50,100,100,100,100,50,50,50,100};
    service.values[s_stamina]=1;
    for(size_t i=0;i<s_staminaSettings.size();++i){
        s_staminaSettings[i]=1000+i;
        assert(kStaminaSettings[i].standard==expected[i]);
        assert(kStaminaSettings[i].min==minimum[i]&&kStaminaSettings[i].max==maximum[i]);
        service.values[s_staminaSettings[i]]=maximum[i];
    }
    for(int cycle=0;cycle<2;++cycle){
        for(bool dawn:{false,true,false}){
            service.values[s_dawnlightMode]=dawn;
            service.values[s_progressionSystem]=cycle; // Progression alone must not lock settings.
            for(size_t i=0;i<s_staminaSettings.size();++i){
                auto key=static_cast<StaminaSetting>(i);auto handle=stamina_setting_config_var(key);
                ModeControlBinding binding{handle,UI_CONTROL_NUMBER,stamina_settings_disabled};
                assert(mode_control_disabled(nullptr,&binding)==dawn);
                UiControlValue value;mode_control_get(nullptr,&binding,&value);
                assert(value.int_value==(dawn?expected[i]:maximum[i]));
                assert(stamina_setting(key)==value.int_value);
                if(dawn){
                    value.int_value=minimum[i];mode_control_set(nullptr,&binding,&value);
                    assert(service.values[handle]==maximum[i]);
                }
            }
        }
    }
    service.values[s_stamina]=0;
    assert(stamina_settings_disabled(nullptr,nullptr));
    ModeControlBinding binding{s_staminaSettings[0],UI_CONTROL_NUMBER,stamina_settings_disabled};
    UiControlValue value;value.int_value=125;
    mode_control_set(nullptr,&binding,&value);assert(service.values[binding.var]==500);
    service.values[s_stamina]=1;
    mode_control_set(nullptr,&binding,&value);assert(stamina_setting(StaminaSetting::Amount)==125);
    // Invalid external config values are clamped at the runtime boundary.
    for(size_t i=0;i<s_staminaSettings.size();++i){
        auto key=static_cast<StaminaSetting>(i);
        service.values[s_staminaSettings[i]]=-999;assert(stamina_setting(key)==minimum[i]);
        service.values[s_staminaSettings[i]]=9999;assert(stamina_setting(key)==maximum[i]);
    }
    // Dawnlight enables the button, but locks its controls even with manual Stamina Off.
    service.values[s_stamina]=0;service.values[s_dawnlightMode]=1;
    assert(!stamina_settings_disabled(nullptr,nullptr)&&mode_control_disabled(nullptr,&binding));
    assert(stamina_capacity(stamina_setting(StaminaSetting::Amount),progression_system_enabled(),20)==105);
    assert(stamina_capacity(stamina_setting(StaminaSetting::Amount),progression_system_enabled(),100)==185);
    service.values[s_dawnlightMode]=0;service.values[s_progressionSystem]=0;
    assert(stamina_settings_disabled(nullptr,nullptr));
    teActive=true;
    assert(!stamina_settings_disabled(nullptr,nullptr)); // TE works with local bar off.
    for(size_t i=0;i<s_staminaSettings.size();++i){
        const auto setting=static_cast<StaminaSetting>(i);
        ModeControlBinding control{s_staminaSettings[i],UI_CONTROL_NUMBER,stamina_settings_disabled};
        assert(mode_control_disabled(nullptr,&control)==twilit_owns_stamina_setting(setting));
        if(twilit_owns_stamina_setting(setting)){
            const auto saved=service.values;
            UiControlValue edit{};edit.int_value=123;
            mode_control_set(nullptr,&control,&edit);
            assert(service.values==saved);
        }
    }
    teActive=false;assert(stamina_settings_disabled(nullptr,nullptr));
}
'''
production = '\n'.join(f'ConfigVarHandle {name}={i+1};' for i, name in enumerate(variables))
for name in ('dawnlight_mode_enabled', 'progression_system_enabled', 'mode_setting_for_config',
             'mode_config_override', 'get_bool', 'get_int', 'stamina_setting_config_var',
             'stamina_setting', 'stamina_enabled', 'stamina_config_var'):
    production += '\n' + function(config, name)
production += '\n' + function(ui, 'twilit_owns_stamina_control')
production += '\n' + ui[ui.index('struct ModeControlBinding {'):ui.index('void bind_mode_control(')]
production += '\n' + function(ui, 'stamina_settings_disabled') + '\n'
assert ui.index('"Stamina Bar",') < ui.index('"Stamina Settings", open_stamina_settings') < ui.index('"Sprint", sprint_config_var()')
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture.replace('// PRODUCTION', production))
    subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Stamina settings passed: defaults/ranges, persisted manual values, mode locks, button gating and progression bonus')
