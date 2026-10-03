"""Run the production mode transition and confirmation callbacks against fake services."""
from pathlib import Path
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
config=(root/'src/config.cpp').read_text()
ui=(root/'src/ui.cpp').read_text()
def function(source,name):
    start=re.search(r'^\w[^\n]*\b'+name+r'\([^;{}]*\)\s*\{',source,re.M).start()
    body=source.index('{',start)
    depth=1;end=body+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]
mapping=function(config,'mode_setting_for_config')
variables=sorted((set(re.findall(r'\bs_\w+',mapping))|{'s_dawnlightMode','s_zItemSlot'})-{'s_staminaSettings'})
fixture=r'''
#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "general_modes.hpp"
#include "stamina_settings.hpp"
#include "twilit_stamina.hpp"
using namespace dawnlight;
namespace dawnlight { bool teActive=false;bool twilit_stamina_active(){return teActive;} }
namespace dawnlight {
bool teHumanSprint=false,teWolfSprint=false;
bool twilit_sprint_enabled(bool wolf){return wolf?teWolfSprint:teHumanSprint;}
const char* zProvider=nullptr;
const char* z_item_slot_provider_notice(){return zProvider;}
}
using ConfigVarHandle=uint64_t;
using UiElementHandle=uint64_t;
using UiDialogHandle=uint64_t;
using UiWindowHandle=uint64_t;
struct ModContext {};
ModContext* mod_ctx=nullptr;
enum ModResult {MOD_OK,MOD_ERROR,MOD_INVALID_ARGUMENT,MOD_UNAVAILABLE};
enum ConfigVarType {CONFIG_VAR_BOOL,CONFIG_VAR_INT};
struct ConfigVarDesc {const char* name=nullptr;ConfigVarType type=CONFIG_VAR_BOOL;bool default_bool=false;int64_t default_int=0;};
#define CONFIG_VAR_DESC_INIT {}
struct ConfigService {
    std::map<ConfigVarHandle,int64_t> values;
    std::map<ConfigVarHandle,ConfigVarType> types;
    ConfigVarHandle next=5000,fail=0,failRead=0;
    int writes=0;
    ModResult register_var(ModContext*,const ConfigVarDesc* desc,ConfigVarHandle* out){
        *out=next++;types[*out]=desc->type;values[*out]=desc->type==CONFIG_VAR_BOOL?desc->default_bool:desc->default_int;return MOD_OK;
    }
    ModResult get_bool(ModContext*,ConfigVarHandle h,bool* v){assert(types.at(h)==CONFIG_VAR_BOOL);*v=values[h]!=0;return h==failRead?MOD_ERROR:MOD_OK;}
    ModResult get_int(ModContext*,ConfigVarHandle h,int64_t* v){assert(types.at(h)==CONFIG_VAR_INT);*v=values[h];return h==failRead?MOD_ERROR:MOD_OK;}
    ModResult set_bool(ModContext*,ConfigVarHandle h,bool v){assert(types.at(h)==CONFIG_VAR_BOOL);if(h==fail)return MOD_ERROR;values[h]=v;++writes;return MOD_OK;}
    ModResult set_int(ModContext*,ConfigVarHandle h,int64_t v){assert(types.at(h)==CONFIG_VAR_INT);if(h==fail)return MOD_ERROR;values[h]=v;++writes;return MOD_OK;}
} configService;
ConfigService* svc_config=&configService;
ProgressionState progression_state(){return {true,true,true,6,true};}
std::array<ConfigVarHandle,kStaminaSettings.size()> s_staminaSettings{};
std::map<ConfigVarHandle,ConfigVarType> s_configTypes;
enum UiControlKind {UI_CONTROL_BUTTON,UI_CONTROL_TOGGLE,UI_CONTROL_NUMBER,UI_CONTROL_SELECT};
enum UiControlBinding {UI_BINDING_CALLBACKS,UI_BINDING_CONFIG_VAR};
struct UiControlValue {bool bool_value=false;int64_t int_value=0;};
using UiPredicateFn=bool(*)(ModContext*,void*);
using UiPressedFn=void(*)(ModContext*,void*);
using UiDialogActionFn=void(*)(ModContext*,UiDialogHandle,void*);
struct UiControlDesc {
    UiControlKind kind=UI_CONTROL_BUTTON;
    const char* label=nullptr;const char* help_rml=nullptr;const char* suffix=nullptr;
    const char* const* options=nullptr;size_t option_count=0;
    UiControlBinding binding=UI_BINDING_CALLBACKS;ConfigVarHandle config_var=0;
    void(*get)(ModContext*,void*,UiControlValue*)=nullptr;
    void(*set)(ModContext*,void*,const UiControlValue*)=nullptr;
    UiPressedFn on_pressed=nullptr;UiPredicateFn is_disabled=nullptr;void* user_data=nullptr;
};
#define UI_CONTROL_DESC_INIT {}
struct UiDialogAction {const char* label=nullptr;UiDialogActionFn on_pressed=nullptr;void* user_data=nullptr;bool keep_open=false;};
#define UI_DIALOG_ACTION_INIT {}
enum UiDialogVariant {UI_DIALOG_NORMAL,UI_DIALOG_WARNING};
struct UiDialogDesc {
    const char* title=nullptr;const char* body_rml=nullptr;UiDialogVariant variant=UI_DIALOG_NORMAL;
    const UiDialogAction* actions=nullptr;size_t action_count=0;
    UiDialogActionFn on_dismiss=nullptr;
};
#define UI_DIALOG_DESC_INIT {}
struct UiService {
    struct Control {UiControlDesc desc;std::string label;bool visible=true,gray=false;};
    std::map<UiElementHandle,Control> controls;
    std::vector<UiDialogAction> actions;
    std::string body;
    UiDialogActionFn dismiss=nullptr;
    UiDialogHandle dialog=0;
    UiElementHandle next=1,focused=0;
    int pushes=0;bool failDialog=false,delayFocus=false;
    ModResult pane_add_control(ModContext*,UiElementHandle,const UiControlDesc* desc,UiElementHandle* out){
        auto h=next++;controls[h]={*desc,desc->label,true,false};if(out)*out=h;return MOD_OK;
    }
    ModResult pane_add_text(ModContext* ctx,UiElementHandle pane,const char* text,UiElementHandle* out){
        UiControlDesc desc{};desc.label=text;return pane_add_control(ctx,pane,&desc,out);
    }
    ModResult elem_set_text(ModContext*,UiElementHandle h,const char* text){controls.at(h).label=text;return MOD_OK;}
    ModResult elem_set_visible(ModContext*,UiElementHandle h,bool visible){controls.at(h).visible=visible;return MOD_OK;}
    ModResult control_set_label(ModContext*,UiElementHandle h,const char* label){controls.at(h).label=label;return MOD_OK;}
    ModResult elem_set_class(ModContext*,UiElementHandle h,const char*,bool active){controls.at(h).gray=active;return MOD_OK;}
    ModResult elem_focus(ModContext*,UiElementHandle h){
        if(delayFocus)return MOD_UNAVAILABLE;
        auto& c=controls.at(h);if(!c.visible||(c.desc.is_disabled&&c.desc.is_disabled(nullptr,c.desc.user_data)))return MOD_UNAVAILABLE;
        focused=h;return MOD_OK;
    }
    ModResult dialog_push(ModContext*,const UiDialogDesc* d,UiDialogHandle* out){
        if(failDialog)return MOD_ERROR;
        assert(d->variant==UI_DIALOG_WARNING);++pushes;dialog=99;*out=dialog;
        body=d->body_rml;actions.assign(d->actions,d->actions+d->action_count);dismiss=d->on_dismiss;return MOD_OK;
    }
    ModResult dialog_close(ModContext*,UiDialogHandle){dialog=0;return MOD_OK;}
    ModResult dialog_set_body(ModContext*,UiDialogHandle,const char* text){body=text;return MOD_OK;}
} uiService;
UiService* svc_ui=&uiService;
// PRODUCTION
ModeControlBinding& make(ConfigVarHandle var,UiControlKind kind,const char* label){
    UiControlDesc desc{};desc.config_var=var;desc.kind=kind;desc.label=label;
    static const char* options[]={"Off","Always","BOTW"};
    if(kind==UI_CONTROL_SELECT){desc.options=options;desc.option_count=3;}
    bind_mode_control(desc);assert(add_mode_control(nullptr,1,desc)==MOD_OK);
    return s_modeControls.at(var);
}
void open(ModeControlBinding& b){
    auto& control=uiService.controls.at(b.unlock);assert(control.visible&&control.gray);
    assert(!control.desc.is_disabled(nullptr,control.desc.user_data));
    control.desc.on_pressed(nullptr,control.desc.user_data);
}
void reset(){
    teActive=teHumanSprint=teWolfSprint=false;zProvider=nullptr;configService={};uiService={};s_configTypes.clear();s_modeControls.clear();s_modeUnlockDialog=0;
    s_teStaminaNote=s_teStaminaSettingsNote=s_localStaminaHelp=s_teSprintNote=s_teWolfSprintNote=s_zItemsNote=0;
    // TYPES
    for(size_t i=0;i<s_staminaSettings.size();++i){s_staminaSettings[i]=1000+i;s_configTypes[1000+i]=CONFIG_VAR_INT;}
    for(const auto& [h,t]:s_configTypes){configService.types[h]=t;configService.values[h]=t==CONFIG_VAR_BOOL?0:237;}
    configService.types[s_dawnlightMode]=CONFIG_VAR_BOOL;configService.values[s_dawnlightMode]=1;
    configService.types[9000]=CONFIG_VAR_INT;configService.values[9000]=876; // unrelated preference
}
int main(){
    // Exercise the production live UI updater, including independent windows.
    reset();configService.values[s_dawnlightMode]=0;
    for(auto var:{s_stamina,s_sprint,s_wolfSprint,s_zItemSlot})make(var,UI_CONTROL_TOGGLE,"Toggle");
    for(auto var:{s_sprintSpeedPercent,s_wolfSpeedPercent})make(var,UI_CONTROL_NUMBER,"Speed");
    for(auto var:s_staminaSettings)make(var,UI_CONTROL_NUMBER,"Cost");
    for(auto* handle:{&s_teStaminaNote,&s_teStaminaSettingsNote,&s_localStaminaHelp,&s_teSprintNote,&s_teWolfSprintNote,&s_zItemsNote})
        svc_ui->pane_add_text(nullptr,1,"Notice",handle);
    const auto stored=configService.values;
    teActive=teHumanSprint=teWolfSprint=true;zProvider="Z Items: Twilit Essentials.";
    update_stamina_ui();
    assert(uiService.controls.at(s_teStaminaNote).visible&&uiService.controls.at(s_teStaminaSettingsNote).visible);
    assert(!uiService.controls.at(s_localStaminaHelp).visible);
    assert(uiService.controls.at(s_teSprintNote).visible&&uiService.controls.at(s_teWolfSprintNote).visible);
    assert(uiService.controls.at(s_zItemsNote).visible&&uiService.controls.at(s_zItemsNote).label==zProvider);
    for(auto& [var,b]:s_modeControls){
        assert(uiService.controls.at(b.control).visible==!external_owns_control(var));
        assert(!uiService.controls.at(b.unlock).visible);
    }
    zProvider="Z Items: Twilight HD HUD.";teHumanSprint=false;update_stamina_ui();
    assert(uiService.controls.at(s_zItemsNote).label==zProvider);
    assert(!uiService.controls.at(s_teSprintNote).visible&&uiService.controls.at(s_teWolfSprintNote).visible);
    teActive=teWolfSprint=false;zProvider=nullptr;update_stamina_ui();
    assert(!uiService.controls.at(s_teStaminaNote).visible&&!uiService.controls.at(s_teStaminaSettingsNote).visible);
    assert(uiService.controls.at(s_localStaminaHelp).visible&&!uiService.controls.at(s_zItemsNote).visible);
    for(auto& [var,b]:s_modeControls)assert(uiService.controls.at(b.control).visible);
    assert(configService.values==stored);
    stamina_window_closed(nullptr,0,nullptr);settings_closed(nullptr,0,nullptr);
    uiService.controls.clear();teActive=true;update_stamina_ui(); // No stale UI handles after close.

    // TE hides both editable controls and preset-unlock proxies without writes.
    for(bool preset:{false,true}) {
        reset();configService.values[s_dawnlightMode]=preset;
        std::vector<ConfigVarHandle> shared{s_stamina};
        for(size_t i=0;i<kStaminaSettings.size();++i)
            if(twilit_owns_stamina_setting(static_cast<StaminaSetting>(i)))shared.push_back(s_staminaSettings[i]);
        for(auto var:shared) {
            auto& b=make(var,var==s_stamina?UI_CONTROL_TOGGLE:UI_CONTROL_NUMBER,"Stamina");
            auto saved=configService.values;teActive=true;
            sync_mode_control(nullptr,b);
            assert(!uiService.controls.at(b.control).visible&&!uiService.controls.at(b.unlock).visible);
            assert(mode_control_disabled(nullptr,&b));
            UiControlValue edit{};edit.bool_value=false;edit.int_value=1;mode_control_set(nullptr,&b,&edit);
            assert(configService.values==saved);
            teActive=false;sync_mode_control(nullptr,b);
            assert(uiService.controls.at(preset?b.unlock:b.control).visible);
        }
        teActive=true;
        for(auto setting:{StaminaSetting::Glide,StaminaSetting::FlurryRush,StaminaSetting::GreatSpin,StaminaSetting::MidnaAttack})
            assert(!twilit_owns_stamina_control(s_staminaSettings[static_cast<size_t>(setting)]));
    }
    // External providers hide toggles, speed sliders and preset proxies,
    // reject stale writes, and restore the saved local settings independently.
    for(bool preset:{false,true}) for(int owner=0;owner<3;++owner){
        reset();configService.values[s_dawnlightMode]=preset;
        const auto toggle=owner==0?s_sprint:owner==1?s_wolfSprint:s_zItemSlot;
        auto& b=make(toggle,UI_CONTROL_TOGGLE,"External setting");
        const auto speed=owner==0?s_sprintSpeedPercent:s_wolfSpeedPercent;
        if(owner<2)make(speed,UI_CONTROL_NUMBER,"Speed");
        auto saved=configService.values;
        teHumanSprint=owner==0;teWolfSprint=owner==1;
        zProvider=owner==2?"Z Items: Twilit Essentials.":nullptr;
        assert(!teActive); // Feature ownership works with the TE meter off.
        sync_mode_control(nullptr,b);
        assert(!uiService.controls.at(b.control).visible&&!uiService.controls.at(b.unlock).visible);
        assert(mode_control_disabled(nullptr,&b));
        UiControlValue edit{};edit.bool_value=true;mode_control_set(nullptr,&b,&edit);
        request_mode_unlock(nullptr,&b);
        assert(configService.values==saved&&uiService.pushes==0);
        if(owner<2){
            auto& rate=s_modeControls.at(speed);sync_mode_control(nullptr,rate);
            assert(!uiService.controls.at(rate.control).visible&&!uiService.controls.at(rate.unlock).visible);
            assert(mode_control_disabled(nullptr,&rate));
            assert(!external_owns_control(owner==0?s_wolfSprint:s_sprint));
        }
        teHumanSprint=teWolfSprint=false;zProvider=nullptr;sync_mode_control(nullptr,b);
        assert(uiService.controls.at(preset&&owner<2?b.unlock:b.control).visible);
        assert(configService.values==saved);
    }
    // A provider enabled while a preset-unlock dialog is open blocks adoption.
    reset();auto& pending=make(s_sprint,UI_CONTROL_TOGGLE,"Sprint");open(pending);
    auto savedBeforeProvider=configService.values;teHumanSprint=true;
    auto confirmExternal=uiService.actions[1];
    confirmExternal.on_pressed(nullptr,uiService.dialog,confirmExternal.user_data);
    assert(configService.values==savedBeforeProvider&&!s_modeUnlockDialog);
    reset();ConfigVarHandle extra=0;
    assert(register_bool("extra-bool",true,extra)==MOD_OK&&s_configTypes[extra]==CONFIG_VAR_BOOL);
    assert(register_int("extra-int",42,extra)==MOD_OK&&s_configTypes[extra]==CONFIG_VAR_INT);
    for(auto kind:{UI_CONTROL_TOGGLE,UI_CONTROL_NUMBER,UI_CONTROL_SELECT}){
        reset();auto target=kind==UI_CONTROL_TOGGLE?s_wolfSprint:kind==UI_CONTROL_NUMBER?s_wolfSpeedPercent:s_bulletTimeMode;
        auto& b=make(target,kind,"Test setting");auto saved=configService.values;
        assert(!uiService.controls.at(b.control).visible);
        open(b);assert(uiService.body.find("replace your previously saved manual settings")!=std::string::npos);
        assert(configService.writes==0);request_mode_unlock(nullptr,&b);assert(uiService.pushes==1);
        auto cancel=uiService.actions[0];cancel.on_pressed(nullptr,uiService.dialog,cancel.user_data);
        assert(!s_modeUnlockDialog&&configService.values==saved);
        open(b);uiService.dismiss(nullptr,uiService.dialog,nullptr);assert(!s_modeUnlockDialog&&configService.values==saved);
        open(b);auto confirm=uiService.actions[1];assert(confirm.keep_open);
        uiService.delayFocus=true;confirm.on_pressed(nullptr,uiService.dialog,confirm.user_data);
        assert(!dawnlight_mode_enabled()&&!s_modeUnlockDialog&&!uiService.dialog);
        assert(b.focusAfterUnlock);uiService.delayFocus=false;mode_unlock_disabled(nullptr,&b);
        assert(uiService.focused==b.control&&!b.focusAfterUnlock&&uiService.controls.at(b.control).visible);
        assert(!uiService.controls.at(b.unlock).visible);
        assert(configService.values.at(s_wolfSpeedPercent)==100&&configService.values.at(s_sprintSpeedPercent)==150);
        assert(configService.values.at(s_progressionSystem)==1); // retained unless it owns the chosen setting
        for(size_t i=0;i<s_staminaSettings.size();++i)assert(configService.values.at(s_staminaSettings[i])==kStaminaSettings[i].standard);
        assert(configService.values.at(9000)==876);
        UiControlValue edit{};edit.bool_value=false;edit.int_value=1;mode_control_set(nullptr,&b,&edit);
        assert(configService.values.at(target)==(kind==UI_CONTROL_TOGGLE?0:1));
    }
    // Independent toggles stay editable, with no unlock dialog or preset adoption.
    for (auto var : {s_disableAutoJump, s_removeNormalHitInvulnerability})
    for (bool personal : {false,true}) {
        reset();configService.values[var]=personal;
        auto& independent=make(var,UI_CONTROL_TOGGLE,"Independent setting");
        assert(uiService.controls.at(independent.control).visible);
        assert(!uiService.controls.at(independent.unlock).visible);
        assert(!mode_control_disabled(nullptr,&independent));
        UiControlValue edit{};edit.bool_value=!personal;
        mode_control_set(nullptr,&independent,&edit);
        assert(configService.values.at(var)==!personal);
        assert(dawnlight_mode_enabled()&&uiService.pushes==0);
    }
    reset();configService.values[s_healthScale]=175;
    auto& hp=make(s_healthScale,UI_CONTROL_NUMBER,"HP Scaling");
    assert(uiService.controls.at(hp.control).visible&&!uiService.controls.at(hp.unlock).visible);
    assert(!mode_control_disabled(nullptr,&hp));
    UiControlValue hpEdit{};hpEdit.int_value=245;
    mode_control_set(nullptr,&hp,&hpEdit);
    assert(configService.values.at(s_healthScale)==245&&dawnlight_mode_enabled()&&uiService.pushes==0);
    // Progression-only locks remain native; editing one through Dawnlight explicitly turns both modes off.
    reset();auto& gale=make(s_revalisGale,UI_CONTROL_TOGGLE,"Gale");open(gale);
    assert(uiService.body.find("Progression System will also be turned Off")!=std::string::npos);
    auto yes=uiService.actions[1];yes.on_pressed(nullptr,uiService.dialog,yes.user_data);
    assert(!progression_system_enabled()&&!mode_control_disabled(nullptr,&gale));
    reset();configService.values[s_dawnlightMode]=0;configService.values[s_progressionSystem]=1;
    auto& locked=make(s_revalisGale,UI_CONTROL_TOGGLE,"Gale");assert(!uiService.controls.at(locked.unlock).visible);
    assert(mode_control_disabled(nullptr,&locked));request_mode_unlock(nullptr,&locked);assert(uiService.pushes==0);
    // Sprint stays editable with progression alone; changing it preserves progression.
    auto& sprint=make(s_sprint,UI_CONTROL_TOGGLE,"Sprint");
    assert(!mode_control_disabled(nullptr,&sprint));
    for(bool enabled:{true,false}) {
        UiControlValue edit{};edit.bool_value=enabled;
        mode_control_set(nullptr,&sprint,&edit);
        assert(configService.values[s_sprint]==enabled&&progression_system_enabled());
        assert(uiService.pushes==0);
    }
    reset();assert(!edit_requires_progression_off(s_sprint));
    assert(leave_dawnlight_mode_for_edit(s_sprint)==MOD_OK);
    assert(!dawnlight_mode_enabled()&&progression_system_enabled());
    // Normal mode toggle still exposes previous preferences without adopting the preset.
    reset();auto saved=configService.values;configService.values[s_dawnlightMode]=0;
    assert(configService.values[s_wolfSpeedPercent]==saved[s_wolfSpeedPercent]);
    // Configuration read/write failures do not turn off the preset or leave partial adoption.
    for(int failure:{0,1,2}){
        reset();auto before=configService.values;
        if(failure==0)configService.failRead=s_staminaSettings[1];
        else configService.fail=failure==1?s_staminaSettings[1]:s_dawnlightMode;
        assert(leave_dawnlight_mode_for_edit(s_wolfSpeedPercent)!=MOD_OK);
        assert(configService.values==before&&dawnlight_mode_enabled());
    }
    reset();auto& error=make(s_wolfSpeedPercent,UI_CONTROL_NUMBER,"Wolf Speed");
    uiService.failDialog=true;open(error);assert(!s_modeUnlockDialog&&configService.writes==0);
    uiService.failDialog=false;open(error);configService.fail=s_staminaSettings[1];
    auto confirm=uiService.actions[1];confirm.on_pressed(nullptr,uiService.dialog,confirm.user_data);
    assert(s_modeUnlockDialog&&dawnlight_mode_enabled());
    assert(uiService.body.find("could not be adopted")!=std::string::npos);
}
'''
production='\n'.join(f'ConfigVarHandle {name}={i+1};' for i,name in enumerate(variables))
for name in ('register_bool','register_int','dawnlight_mode_enabled','progression_system_enabled',
             'mode_setting_for_config','mode_config_override','edit_requires_progression_off','leave_dawnlight_mode_for_edit'):
    production+='\n'+function(config,name)
production+='\n'+function(config,'stamina_config_var')+'\n'+function(config,'stamina_setting_config_var')
for name in ('sprint_config_var','sprint_speed_config_var','wolf_sprint_config_var','wolf_speed_config_var','z_item_slot_config_var'):
    production+='\n'+function(config,name)
production+='\n'+function(ui,'twilit_owns_stamina_control')+'\n'+function(ui,'external_owns_control')
production+='\n'+ui[ui.index('struct ModeControlBinding {'):ui.index('#include "mode_unlock_ui.inc"')]
production+='\n'+(root/'src/mode_unlock_ui.inc').read_text()
production+='\n'+ui[ui.index('UiWindowHandle s_staminaWindow'):ui.index('UiMenuTabHandle s_menuTab')]
for name in ('update_stamina_ui','stamina_window_closed','settings_closed'):
    production+='\n'+function(ui,name)
bool_vars=set(re.findall(r'register_bool\([^\n]*?\b(s_\w+)\)',config))
# Multi-line registration of the invulnerability toggle.
bool_vars.add('s_removeNormalHitInvulnerability')
types='\n'.join(f's_configTypes[{v}]={"CONFIG_VAR_BOOL" if v in bool_vars else "CONFIG_VAR_INT"};' for v in variables if v!='s_dawnlightMode')
with tempfile.TemporaryDirectory() as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture.replace('// PRODUCTION',production).replace('// TYPES',types))
    subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Mode unlock passed: locked activation, cancel/dismiss, adoption, progression, focus and rollback')
