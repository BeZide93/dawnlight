"""Run the host's actual config writer while cheats are masked, then reload without cleanup."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
host = (root / 'dusklight/src/dusk/config.cpp').read_text()


def extract(signature):
    start = host.index(signature)
    body = host.index('{', start)
    depth = 1
    end = body + 1
    while depth:
        depth += (host[end] == '{') - (host[end] == '}')
        end += 1
    return host[start:end]


fixture = r'''
#include <cassert>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <nlohmann/json.hpp>
#include "survivors_cheat_guard.hpp"
using json=nlohmann::json;
std::function<void()> changed;
namespace borealis::io {std::string fs_path_to_string(const std::filesystem::path& p){return p.string();}}
namespace io {struct FileStream {
 static void WriteAllText(const std::filesystem::path& p,const std::string& text){
  std::ofstream out(p);out<<text;assert(out.good());
 }
};}
namespace dusk::config {
// Use native ConfigVar storage/layers and native JSON serialization below.
// Only host logging, registration and the filesystem path plumbing are stubbed.
class ConfigImplBase {
public:
 virtual ~ConfigImplBase()=default;
 virtual json dumpToJson(const ConfigVarBase&)const=0;
};
template<ConfigValue T> class ConfigImpl:public ConfigImplBase {
public:
 json dumpToJson(const ConfigVarBase& value)const override{
  return dumpToJson(static_cast<const ConfigVar<T>&>(value));
 }
 static json dumpToJson(const ConfigVar<T>&);
};
ConfigVarBase::ConfigVarBase(std::string n,const ConfigImplBase* i):
 name(std::move(n)),registered(true),layer(ConfigVarLayer::Default),impl(i){}
ConfigVarBase::~ConfigVarBase()=default;
const ConfigImplBase* ConfigVarBase::getImpl()const noexcept{return impl;}
bool ConfigVarBase::has_subscribers()const{return bool(changed);}
void ConfigVarBase::notify_changed(const void*){if(changed)changed();}
template<ConfigValue T> const ConfigImplBase* GetConfigImpl(){static ConfigImpl<T> impl;return &impl;}
std::map<std::string,ConfigVarBase*> RegisteredConfigVars;
std::map<std::string,json> UnregisteredConfigVars;
std::filesystem::path GetConfigJsonPath(){return "config.json";}
std::filesystem::path GetTempConfigJsonPath(const std::filesystem::path& p){return p.string()+".tmp";}
void ReplaceFile(const std::filesystem::path& from,const std::filesystem::path& to){std::filesystem::rename(from,to);}
struct Log {
 template<class... Args>void info(Args&&...){}
 template<class... Args>void error(Args&&...){assert(false);}
}DuskConfigLog;
'''
fixture += 'template<ConfigValue T>\n' + extract('nlohmann::json ConfigImpl<T>::dumpToJson(')
fixture += '\n' + extract('void save()') + '\n}\n'
fixture += r'''
using namespace dusk::config;
enum class Mode:u8{Off=0,On=1,Other=2};
json read_saved(){std::ifstream in("config.json");json result;in>>result;return result;}
template<class T>void test(T enabled){
 ConfigVar<T> value("cheat",T{});
 ConfigVar<bool> unrelated("unrelated",true);
 RegisteredConfigVars={{"cheat",&value},{"unrelated",&unrelated}};
 UnregisteredConfigVars={{"disabled-mod-setting",42}};
 value.setValue(enabled);unrelated.setValue(true);save();const auto baseline=read_saved();
 dawnlight::survivors::CheatGuard<T> guard{&value};
 changed=[&]{guard.enforce();};guard.enforce();assert(value.getValue()==T{});
 // No run-end/release before serialization or reload: model a process crash.
 save();const auto during=read_saved();assert(during==baseline);
 changed={};ConfigVar<T> fresh("cheat",T{});fresh.setValue(during.at("cheat").get<T>());
 assert(fresh.getValue()==enabled);
 // Manual edits remain saved even when their effect stays suppressed.
 changed=[&]{guard.enforce();};value.setValue(T{});save();assert(read_saved().at("cheat")==json(T{}));
 value.setValue(enabled);save();assert(value.getValue()==T{}&&read_saved()==baseline);
 // Clearing a launch override must not recurse through the change callback or
 // replace the underlying preference with the launch-argument value.
 changed={};guard.release();value.setValue(T{});value.setOverrideValue(enabled);
 changed=[&]{guard.enforce();};guard.enforce();save();
 assert(value.getValue()==T{}&&read_saved().at("cheat")==json(T{}));
 changed={};guard.release();assert(value.getValue()==enabled&&value.getLayer()==ConfigVarLayer::Override);
 value.clearOverride();assert(value.getValue()==T{});
 // Existing native policy is never owned, changed, or cleared by Survivors.
 value.setValue(enabled);value.setSpeedrunValue(T{});guard.enforce();save();
 assert(read_saved()==baseline);guard.release();assert(value.getLayer()==ConfigVarLayer::Speedrun);
 value.clearSpeedrunOverride();assert(value.getValue()==enabled);
 // Clean exits/restarts and an unmasked (Allow Cheats) run keep the same file.
 for(int i=0;i<3;++i){guard.enforce();save();assert(read_saved()==baseline);guard.release();}
 assert(value.getValue()==enabled);save();assert(read_saved()==baseline);
 RegisteredConfigVars.clear();UnregisteredConfigVars.clear();
}
int main(){test(true);test(Mode::On);test(Mode::Other);}
'''

json_include = Path(os.environ.get('DAWNLIGHT_JSON_INCLUDE',
    root / 'build/_deps/dawnlight_json_headers-src/include'))
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    exe = Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-DTARGET_PC',
        '-Wall', '-Wextra', '-Werror', '-I', str(root / 'src'),
        '-I', str(root / 'dusklight/src'), '-I', str(root / 'dusklight/extern/aurora/include'),
        '-I', str(json_include), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], cwd=tmp, check=True)
print('Survivors native config save: crash/reload, live edits, launch overrides, policy and cleanup passed')
