"""Exercise the real persistent toggle without installing live gameplay hooks."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/survivors_performance_settings.inc').read_text()
fixture=r'''
#include <cassert>
#include <string>
#define DAWNLIGHT_FREEZE_DIAGNOSTICS 1
using ConfigVarHandle=int;using UiElementHandle=int;using ModResult=int;
constexpr int MOD_OK=0,CONFIG_VAR_BOOL=1,UI_CONTROL_TOGGLE=1;
struct ModContext{};ModContext* mod_ctx=nullptr;
struct ConfigVarDesc{const char* name=nullptr;int type=0;bool default_bool=false;};
#define CONFIG_VAR_DESC_INIT {}
struct UiControlValue{bool bool_value=false;};
struct UiControlDesc{int kind=0;const char* label=nullptr;const char* help_rml=nullptr;
 void(*get)(ModContext*,void*,UiControlValue*)=nullptr;
 void(*set)(ModContext*,void*,const UiControlValue*)=nullptr;};
#define UI_CONTROL_DESC_INIT {}
namespace freeze_diag {bool enabled=false;}
bool persisted=false,fail_save=false;unsigned writes=0,warnings=0;
ModResult register_owned_setting(const ConfigVarDesc* d,ConfigVarHandle* h){
 assert(std::string(d->name)=="survivors-performance-logs"&&d->type==CONFIG_VAR_BOOL&&!d->default_bool);*h=1;return 0;}
struct Config{int get_bool(ModContext*,int,bool* v){*v=persisted;return 0;}
 int set_bool(ModContext*,int,bool v){++writes;if(fail_save)return 1;persisted=v;return 0;}}config;
auto* svc_config=&config;
struct Log{void warn(ModContext*,const char*){++warnings;}}logService;auto* svc_log=&logService;
struct UI{UiControlDesc control{};std::string note;
 int pane_add_control(ModContext*,int,const UiControlDesc* d,void*){control=*d;return 0;}
 int pane_add_text(ModContext*,int,const char* text,void*){note=text;return 0;}}ui;auto* svc_ui=&ui;
'''
main=r'''
int main(){
 assert(initialize_performance_logs_setting()==MOD_OK&&!performance_logs_saved());
 assert(build_performance_logs(1)==MOD_OK&&ui.control.kind==UI_CONTROL_TOGGLE);
 assert(std::string(ui.control.label)=="Performance Logs");
 assert(std::string(ui.control.help_rml).find("Restart Dusklight")!=std::string::npos);
 assert(ui.note.find("Off for this session")!=std::string::npos);
 UiControlValue v{true};ui.control.set(nullptr,nullptr,&v);
 assert(persisted&&writes==1&&!freeze_diag::enabled); // No hot hook installation.
 v={};ui.control.get(nullptr,nullptr,&v);assert(v.bool_value);
 // Reload registration retains saved configuration; startup latches it.
 s_performanceLogsConfig=0;assert(initialize_performance_logs_setting()==0);
 freeze_diag::enabled=performance_logs_saved();assert(freeze_diag::enabled);
 build_performance_logs(1);assert(ui.note.find("On for this session")!=std::string::npos);
 v={false};ui.control.set(nullptr,nullptr,&v);
 assert(!persisted&&freeze_diag::enabled); // Never removes shared archive hooks in flight.
 freeze_diag::enabled=performance_logs_saved();assert(!freeze_diag::enabled);
 fail_save=true;v={true};ui.control.set(nullptr,nullptr,&v);assert(!persisted&&warnings==1);
}
'''
with tempfile.TemporaryDirectory() as temp:
 cpp=Path(temp)/'test.cpp';exe=Path(temp)/'test';cpp.write_text(fixture+source+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Performance Logs: default Off, persistence, restart boundary and UI passed')
