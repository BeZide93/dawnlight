#include "../src/config.hpp"
#include "../src/progression.hpp"
#include "../src/service_imports.hpp"
#include "../src/settings_snapshot.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>

ModContext* mod_ctx=nullptr;
const ConfigService* svc_config=nullptr;
const HostService* svc_host=nullptr;
const UiService* svc_ui=nullptr;
const LogService* svc_log=nullptr;
namespace dawnlight {
bool g_configCheckForUpdatesEnabled=false;
ProgressionState progression_state() { return {}; }
}
namespace {
using dawnlight::settings::Value;
std::map<ConfigVarHandle,Value> values;
std::map<std::string,ConfigVarHandle> names;
std::map<ConfigVarHandle,ConfigChangedFn> callbacks;
ConfigVarHandle next=1, failOnce=0;
std::string directory;
ModResult reg(ModContext*,const ConfigVarDesc* d,ConfigVarHandle* h) {
    assert(!names.contains(d->name));names[d->name]=*h=next++;
    if(d->type==CONFIG_VAR_BOOL) values[*h]=d->default_bool;
    else if(d->type==CONFIG_VAR_INT) values[*h]=d->default_int;
    else { assert(d->type==CONFIG_VAR_STRING);values[*h]=std::string(d->default_string?d->default_string:""); }
    return MOD_OK;
}
ModResult get_bool(ModContext*,ConfigVarHandle h,bool* v) { *v=std::get<bool>(values.at(h));return MOD_OK; }
ModResult get_int(ModContext*,ConfigVarHandle h,int64_t* v) { *v=std::get<int64_t>(values.at(h));return MOD_OK; }
ModResult get_string(ModContext*,ConfigVarHandle h,char* out,size_t size,size_t* length) {
    const auto& s=std::get<std::string>(values.at(h));*length=s.size();
    if(out) {if(size<=s.size())return MOD_INVALID_ARGUMENT;std::memcpy(out,s.c_str(),s.size()+1);}return MOD_OK;
}
ModResult set(ConfigVarHandle h,Value value) {
    if(failOnce==h) {failOnce=0;return MOD_ERROR;}
    if(values[h]==value) return MOD_OK;
    values[h]=std::move(value);
    if(auto it=callbacks.find(h);it!=callbacks.end()) it->second(nullptr,h,nullptr,nullptr,nullptr);
    return MOD_OK;
}
ModResult set_bool(ModContext*,ConfigVarHandle h,bool v) { return set(h,v); }
ModResult set_int(ModContext*,ConfigVarHandle h,int64_t v) { return set(h,v); }
ModResult set_string(ModContext*,ConfigVarHandle h,const char* v) { return set(h,std::string(v)); }
ModResult subscribe(ModContext*,ConfigVarHandle h,ConfigChangedFn fn,void*,ConfigSubscriptionHandle*) {
    callbacks[h]=fn;return MOD_OK;
}
ModResult data_dir(ModContext*,const char** out) { *out=directory.c_str();return MOD_OK; }
ModResult toast(ModContext*,const UiToastDesc*) { return MOD_OK; }
}
int main(int argc,char** argv) {
    using namespace dawnlight;
    assert(argc==2);directory=argv[1];
    ConfigService config{};config.register_var=reg;config.get_bool=get_bool;config.get_int=get_int;
    config.get_string=get_string;config.set_bool=set_bool;config.set_int=set_int;config.set_string=set_string;
    config.subscribe=subscribe;svc_config=&config;
    HostService host{};host.data_dir=data_dir;svc_host=&host;
    UiService ui{};ui.push_toast=toast;svc_ui=&ui;
    assert(register_config(nullptr)==MOD_OK);
    assert(names.contains("touch-button-twe-quick-access-layout"));
    for(auto& [h,v]:values) if(std::holds_alternative<std::string>(v)) v=std::string("{\"x\":42,\"text\":\"UTF-8 \xc3\xa4\"}\n");
    set_bool(nullptr,names.at("r-jump"),true);
    set_bool(nullptr,names.at("disable-auto-jump"),true);
    std::string path;assert(export_all_settings(path)==HudSettingsIoResult::Ok);
    std::ifstream in(path,std::ios::binary);std::string json((std::istreambuf_iterator<char>(in)),{});
    settings::Values exported;assert(settings::Reader(json).decode(exported));
    assert(exported.size()==names.size());
    for(auto& [name,h]:names) assert(exported.at(name)==values.at(h));
    const auto original=values;
    // Scramble all types without notifications, then restore the complete actual registry.
    for(auto& [h,v]:values) {
        if(auto* b=std::get_if<bool>(&v)) *b=!*b;
        else if(auto* i=std::get_if<int64_t>(&v)) *i=17;
        else v=std::string("different");
    }
    assert(import_all_settings_json(json)==HudSettingsIoResult::Ok);assert(values==original);
    // In particular, disable-auto-jump sorts before r-jump: callbacks must not erase it mid-import.
    assert(std::get<bool>(values.at(names.at("disable-auto-jump"))));
    for(const auto& invalid : {std::string("{}"),std::string("{\"elements\":{}}"),json+"junk"}) {
        assert(import_all_settings_json(invalid)==HudSettingsIoResult::InvalidFormat);assert(values==original);
    }
    auto bad=exported;bad["r-jump"]=int64_t(1);
    assert(import_all_settings_json(settings::encode(bad))==HudSettingsIoResult::InvalidFormat);assert(values==original);
    settings::Values partial{{"aim-mode",int64_t(0)},{"other-mod-option",true}};
    assert(import_all_settings_json(settings::encode(partial))==HudSettingsIoResult::Ok);
    assert(values.at(names.at("aim-mode"))==Value(int64_t(0)));
    assert(names.size()==exported.size());
    assert(import_all_settings_json(json)==HudSettingsIoResult::Ok);
    auto changed=exported;changed["aim-mode"]=int64_t(0);changed["r-jump"]=false;
    failOnce=names.at("r-jump");
    assert(import_all_settings_json(settings::encode(changed))==HudSettingsIoResult::ConfigFailed);
    assert(values==original);
}
