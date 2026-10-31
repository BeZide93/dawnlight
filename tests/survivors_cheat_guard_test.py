"""Exercise the real native ConfigVar layers: run suppression never saves over preferences."""
from pathlib import Path
import os, re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
# Keep the lock exhaustive when the host adds a Cheats-tab setting.
native=(root/'dusklight/src/dusk/settings.cpp').read_text().split('// Cheats',1)[1].split('// Technical',1)[0]
keys=set(re.findall(r'"(game\.[^" ]+)"',native))
implementation=(root/'src/survivors_integrity.cpp').read_text()
assert len(keys)==18 and all('"'+key+'"' in implementation for key in keys)
fixture=r'''
#include <cassert>
#include <functional>
#include "survivors_cheat_guard.hpp"
using namespace dusk::config;
std::function<void()> changed;
namespace dusk::config {
ConfigVarBase::ConfigVarBase(std::string n,const ConfigImplBase* i):name(std::move(n)),registered(true),layer(ConfigVarLayer::Default),impl(i){}
ConfigVarBase::~ConfigVarBase()=default;
bool ConfigVarBase::has_subscribers()const{return bool(changed);}
void ConfigVarBase::notify_changed(const void*){if(changed)changed();}
template<ConfigValue T> const ConfigImplBase* GetConfigImpl(){return nullptr;}
}
enum class Mode:u8{Normal=0,Cheat=1,OtherCheat=2};
template<class T> void check(T enabled){
 ConfigVar<T> value("cheat",T{});dawnlight::survivors::CheatGuard<T> guard{&value};
 changed=[&]{guard.enforce();};
 guard.enforce();assert(value.getLayer()==ConfigVarLayer::Default);
 // The user can change the saved preference, but its live effect stays off.
 value.setValue(enabled);assert(value.getValue()==T{}&&value.getValueForSave()==enabled);
 guard.enforce();assert(value.getValueForSave()==enabled);
 changed={};guard.release();assert(value.getValue()==enabled);
 // Launch-argument overrides survive a whole run, including their lower saved value.
 value.setValue(T{});value.setOverrideValue(enabled);guard.enforce();
 assert(value.getValue()==T{}&&value.getValueForSave()==T{});
 guard.release();assert(value.getLayer()==ConfigVarLayer::Override&&value.getValue()==enabled);
 value.clearOverride();assert(value.getValue()==T{});
 // Disabling a cheat in the menu must not resurrect the previous override.
 value.setOverrideValue(enabled);guard.enforce();changed=[&]{guard.enforce();};
 value.setValue(T{});changed={};guard.release();assert(value.getValue()==T{});
 // The stored user value is also preserved while our runtime mask is active.
 value.setValue(enabled);guard.enforce();
 assert(value.getLayer()==ConfigVarLayer::Speedrun&&value.getValueForSave()==enabled);
 guard.release();assert(value.getLayer()==ConfigVarLayer::Value&&value.getValue()==enabled);
 // A native speedrun override already present is not owned or removed by us.
 value.setSpeedrunValue(T{});guard.enforce();guard.release();
 assert(value.getLayer()==ConfigVarLayer::Speedrun&&value.getValue()==T{});
 value.clearSpeedrunOverride();assert(value.getValue()==enabled);
 // Removing an active launch override notifies synchronously. Do not recurse
 // into enforcement or lose the launch value that must return on clean exit.
 value.setValue(enabled);value.setOverrideValue(enabled);changed=[&]{guard.enforce();};
 guard.enforce();assert(value.getValue()==T{}&&value.getValueForSave()==enabled);
 changed={};guard.release();assert(value.getLayer()==ConfigVarLayer::Override&&value.getValue()==enabled);
 value.clearOverride();
 // Repeated end/restart/shutdown cleanup is idempotent.
 guard.enforce();guard.release();guard.release();assert(value.getValue()==enabled);
}

'''
def function(signature):
 start=implementation.index(signature);body=implementation.index('{',start);depth=1;end=body+1
 while depth:
  depth+=(implementation[end]=='{')-(implementation[end]=='}');end+=1
 return implementation[start:end]
fixture+=r'''
#include <array>
#include <ranges>
#include <string_view>
#include <vector>
namespace integrity_test {
namespace survivors=dawnlight::survivors;
struct Item{struct{std::string id;}metadata;bool active;};
struct Loader{std::vector<Item> items;auto active_mods(){return items|std::views::filter([](const auto& m){return m.active;});}}loader;
auto s_loader=+[]()->Loader&{return loader;};
struct Host{const char* mod_id(void*){return "dev.bezide.dawnlight";}}host;
auto* svc_host=&host;void* mod_ctx=nullptr;
ConfigVar<bool> cheat("cheat",false);
std::array<survivors::CheatGuard<bool>,16> s_bools{};
survivors::CheatGuard<bool> s_spin,s_armor;
// BOOL_CHEATS
std::array<unsigned,18> s_subscriptions{};
unsigned registered=0,removed=0;
auto s_subscribe=[](const char*,auto){return ++registered;};
void unsubscribe(unsigned token){assert(token);++removed;}
auto s_unsubscribe=&unsubscribe;
bool s_running=false,s_locked=false;
void end_survivors_integrity();
'''
for signature in ['void enforce()', 'bool other_mod_active()', 'void begin_survivors_integrity(', 'void update_survivors_integrity(', 'void end_survivors_integrity()']:
 fixture+=function(signature)+'\n'
fixture+=r'''
void run(){
 s_bools[0].var=&cheat;cheat.setValue(true);
 loader.items={{{"dev.bezide.dawnlight"},true},{{"disabled.cosmetic"},false}};
 bool modded=true;begin_survivors_integrity(false,modded);
 assert(!modded&&!cheat.getValue()&&cheat.getValueForSave()&&registered==18);
 loader.items.push_back({{"active.cosmetic"},true});update_survivors_integrity(modded);assert(modded);
 loader.items.back().active=false;update_survivors_integrity(modded);assert(modded);
 end_survivors_integrity();assert(cheat.getValue()&&removed==18&&!s_locked&&!s_running);
 end_survivors_integrity();assert(removed==18);
 begin_survivors_integrity(true,modded);assert(!modded&&cheat.getValue()&&registered==18);
 loader.items.back().active=true;update_survivors_integrity(modded);assert(modded);
 end_survivors_integrity();assert(removed==18);
 // Normal cleanup must not install a host speedrun policy on any cheat.
 for(auto& guard:s_bools)guard={};
 s_bools[0].var=&cheat;
 begin_survivors_integrity(false,modded);assert(!cheat.getValue());
 end_survivors_integrity();assert(cheat.getValue()&&cheat.getLayer()==ConfigVarLayer::Value);

}
}
int main(){check(true);check(Mode::Cheat);check(Mode::OtherCheat);integrity_test::run();}
'''
a=implementation.index('constexpr std::array kBoolCheats')
fixture=fixture.replace('// BOOL_CHEATS',implementation[a:implementation.index('};',a)+2])
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test';cpp.write_text(fixture)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DTARGET_PC','-Wall','-Wextra','-Werror',
  '-I',str(root/'src'),'-I',str(root/'dusklight/src'),'-I',str(root/'dusklight/extern/aurora/include'),
  str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors cheat guards: live edits, saved preferences, launch overrides, speedrun and cleanup passed')
