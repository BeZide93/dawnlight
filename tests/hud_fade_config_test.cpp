// Build with the pinned Dusklight sdk/include, src/config.cpp, -ffunction-sections
// -fdata-sections and -Wl,--gc-sections. Pass a temporary data directory as argv[1].
#include "../src/config.hpp"
#include "../src/progression.hpp"
#include "../src/service_imports.hpp"
#include <cassert>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

ModContext* mod_ctx = nullptr;
const ConfigService* svc_config = nullptr;
const UiService* svc_ui = nullptr;
const HostService* svc_host = nullptr;
const LogService* svc_log = nullptr;
namespace dawnlight {
bool g_configCheckForUpdatesEnabled = false;
ProgressionState progression_state() { return {}; }
ModResult register_touch_button_config(ModError*) { return MOD_OK; }
}
namespace {
std::map<ConfigVarHandle, int64_t> values;
ConfigVarHandle nextHandle = 1;
std::string dataDir;
ModResult register_var(ModContext*, const ConfigVarDesc* desc, ConfigVarHandle* out) {
    *out = nextHandle++;
    values[*out] = desc->type == CONFIG_VAR_BOOL ? desc->default_bool : desc->default_int;
    return MOD_OK;
}
ModResult get_bool(ModContext*, ConfigVarHandle var, bool* out) { *out = values[var]; return MOD_OK; }
ModResult get_int(ModContext*, ConfigVarHandle var, int64_t* out) { *out = values[var]; return MOD_OK; }
ModResult set_bool(ModContext*, ConfigVarHandle var, bool x) { values[var] = x; return MOD_OK; }
ModResult set_int(ModContext*, ConfigVarHandle var, int64_t x) { values[var] = x; return MOD_OK; }
ModResult subscribe(ModContext*, ConfigVarHandle, ConfigChangedFn, void*, ConfigSubscriptionHandle*) { return MOD_OK; }
ModResult data_dir(ModContext*, const char** out) { *out = dataDir.c_str(); return MOD_OK; }
}
int main(int argc, char** argv) {
    using namespace dawnlight;
    assert(argc == 2); dataDir = argv[1];
    ConfigService config{};
    config.register_var = register_var; config.get_bool = get_bool; config.get_int = get_int;
    config.set_bool = set_bool; config.set_int = set_int; config.subscribe = subscribe;
    svc_config = &config;
    HostService host{}; host.data_dir = data_dir; svc_host = &host;
    assert(register_config(nullptr) == MOD_OK);
    for(size_t i=0;i<kDarkLinkSettings.size();++i){
        auto setting=static_cast<DarkLinkSetting>(i);const auto& d=kDarkLinkSettings[i];
        auto handle=dark_link_setting_config_var(setting);
        assert(dark_link_setting(setting)==d.standard);
        values[handle]=d.min-1;assert(dark_link_setting(setting)==d.min);
        values[handle]=d.max+1;assert(dark_link_setting(setting)==d.max);
        values[handle]=d.standard;
    }
    assert(!hud_auto_fade_enabled());
    assert(!hud_custom_stamina_fade_when_full());
    assert(!hud_custom_fierce_deity_fade_when_empty());
    for (const auto element : {HudElement::TearsOfLight, HudElement::EponaSpurs}) {
        assert(hud_custom_element_x(element)==0 && hud_custom_element_y(element)==0);
        assert(hud_custom_element_scale_percent(element)==100);
        for (bool autoFade : {false, true}) {
            set_bool(nullptr, hud_auto_fade_config_var(), autoFade);
            for (auto preset : {HudLayout::GameCube, HudLayout::XBox, HudLayout::WiiU, HudLayout::Dawnlight}) {
                set_bool(nullptr, hud_custom_stamina_fade_when_full_config_var(), true);
                set_bool(nullptr, hud_custom_fierce_deity_fade_when_empty_config_var(), true);
                set_int(nullptr,hud_custom_element_x_config_var(element),123);
                set_int(nullptr,hud_custom_element_y_config_var(element),-45);
                set_int(nullptr,hud_custom_element_scale_config_var(element),175);
                assert(copy_hud_preset_to_custom(preset) == HudSettingsIoResult::Ok);
                assert(hud_custom_element_x(element)==0 && hud_custom_element_y(element)==0);
                assert(hud_custom_element_scale_percent(element)==100);
                assert(hud_custom_stamina_fade_when_full() && hud_custom_fierce_deity_fade_when_empty());
                assert(hud_auto_fade_enabled() == autoFade);
            }
            std::string path;
            set_bool(nullptr, hud_custom_stamina_fade_when_full_config_var(), true);
            set_bool(nullptr, hud_custom_fierce_deity_fade_when_empty_config_var(), true);
            set_int(nullptr,hud_custom_element_x_config_var(element),123);
            set_int(nullptr,hud_custom_element_y_config_var(element),-45);
            set_int(nullptr,hud_custom_element_scale_config_var(element),175);
            assert(export_custom_hud_settings(path) == HudSettingsIoResult::Ok);
            assert(reset_custom_hud_settings() == HudSettingsIoResult::Ok);
            assert(hud_custom_element_x(element)==0 && hud_custom_element_y(element)==0);
            assert(hud_custom_element_scale_percent(element)==100);
            assert(hud_custom_stamina_fade_when_full() && hud_custom_fierce_deity_fade_when_empty());
            assert(hud_auto_fade_enabled() == autoFade);
            assert(import_custom_hud_settings(path) == HudSettingsIoResult::Ok);
            assert(hud_custom_element_x(element)==123 && hud_custom_element_y(element)==-45);
            assert(hud_custom_element_scale_percent(element)==175);
            assert(hud_custom_stamina_fade_when_full() && hud_custom_fierce_deity_fade_when_empty());
            assert(hud_auto_fade_enabled() == autoFade);
            // Old presets leave both independent gameplay fade settings intact.
            { std::ofstream out(path); out << R"({"version":14,"elements":{}})"; }
            assert(import_custom_hud_settings(path) == HudSettingsIoResult::Ok);
            assert(hud_custom_element_x(element)==0 && hud_custom_element_y(element)==0);
            assert(hud_custom_element_scale_percent(element)==100);
            assert(hud_custom_stamina_fade_when_full() && hud_custom_fierce_deity_fade_when_empty());
            assert(hud_auto_fade_enabled() == autoFade);
            // Legacy layout fade fields cannot override the relocated settings.
            { std::ofstream out(path); out << R"({"version":14,"elements":{"stamina-bar":{"fadeWhenFull":false},"fierce-deity-bar":{"fadeWhenEmpty":false}}})"; }
            assert(import_custom_hud_settings(path) == HudSettingsIoResult::Ok);
            assert(hud_custom_element_x(element)==0 && hud_custom_element_y(element)==0);
            assert(hud_custom_element_scale_percent(element)==100);
            assert(hud_custom_stamina_fade_when_full() && hud_custom_fierce_deity_fade_when_empty());
        }
    }
}
