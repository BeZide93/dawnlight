"""Regression checks for the jump dependency and scoped native ledge-jump veto."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
config=(root/'src/config.cpp').read_text()
ui=(root/'src/ui.cpp').read_text()
jump=(root/'src/jump_hooks.cpp').read_text()
def function(source,signature):
    start=source.index(signature+'(')
    return source[start:source.index('\n}',start)+2]
fixture=r'''
#include <cassert>
#include <vector>
using ConfigVarHandle=int;struct ConfigVarValue {};struct ModContext {};
ModContext* mod_ctx=nullptr;constexpr int MOD_OK=0;int s_disableAutoJump=1;
bool parentEnabled=true,childStored=false;int configWrites=0;
void on_jump_setting_changed(ModContext*,ConfigVarHandle,const ConfigVarValue*,const ConfigVarValue*,void*);
struct Config {
 int get_bool(ModContext*,ConfigVarHandle,bool* value){*value=childStored;return MOD_OK;}
 int set_bool(ModContext*,ConfigVarHandle,bool value){
  childStored=value;++configWrites;on_jump_setting_changed(nullptr,0,nullptr,nullptr,nullptr);return MOD_OK;
 }
} configService;
auto* svc_config=&configService;
bool r_jump_enabled(){return parentEnabled;}
bool get_bool(ConfigVarHandle,bool){return childStored;}
// CONFIG
using BOOL=int;constexpr BOOL TRUE=1;
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* args,int i){return *static_cast<T*>(static_cast<void**>(args)[i]);}}
struct daPy_py_c{enum {ERFLG0_NOT_AUTO_JUMP=0x20,ERFLG0_FORCE_AUTO_JUMP=0x40};};
struct daAlink_c:daPy_py_c {
 unsigned mEndResetFlg0=0;bool wolf=false,demo=false,event=false;
 bool checkWolf(){return wolf;}bool checkPlayerDemoMode(){return demo;}bool checkEventRun(){return event;}
 bool checkEndResetFlg0(unsigned f){return mEndResetFlg0&f;}
 void onEndResetFlg0(unsigned f){mEndResetFlg0|=f;}
};
bool eventRunning=false,manualJump=false,ability=false;
bool dComIfGp_event_runCheck(){return eventRunning;}
bool handle_jump_abilities(daAlink_c*){return ability;}
bool start_ground_jump(daAlink_c*){return manualJump;}
// JUMP
int main(){
 assert(!disable_auto_jump_enabled()&&!auto_jump_setting_disabled(nullptr,nullptr)&&configWrites==0);
 childStored=true;assert(disable_auto_jump_enabled());
 parentEnabled=false;on_jump_setting_changed(nullptr,0,nullptr,nullptr,nullptr);
 assert(!childStored&&configWrites==1&&auto_jump_setting_disabled(nullptr,nullptr));
 parentEnabled=true;assert(!disable_auto_jump_enabled()&&!auto_jump_setting_disabled(nullptr,nullptr));
 parentEnabled=false;svc_config->set_bool(nullptr,1,true);assert(!childStored); // rejects external invalid child writes
 childStored=true;assert(!disable_auto_jump_enabled()&&!childStored); // invalid saved/mode-dependent state
 parentEnabled=true;childStored=true;
 daAlink_c link;auto* owner=&link;void* args[]={&owner};BOOL result=0;
 auto begin=[&](){result=0;return before_check_auto_jump(nullptr,args,&result,nullptr);};
 auto end=[](){after_check_auto_jump(nullptr,nullptr,nullptr,nullptr);};
 assert(begin()==HOOK_CONTINUE&&link.checkEndResetFlg0(daPy_py_c::ERFLG0_NOT_AUTO_JUMP));
 link.mEndResetFlg0|=0x800;end();assert(link.mEndResetFlg0==0x800); // preserve unrelated flags
 link.mEndResetFlg0=daPy_py_c::ERFLG0_NOT_AUTO_JUMP;begin();end();assert(link.mEndResetFlg0==0x20);
 link.mEndResetFlg0=0;begin();begin();end();assert(link.mEndResetFlg0==0x20);end();assert(!link.mEndResetFlg0);
 link.wolf=true;begin();assert(link.mEndResetFlg0==0x20);end();assert(!link.mEndResetFlg0);
 for(int i=1;i<6;++i){
  link.wolf=false;link.demo=i==1;link.event=i==2;eventRunning=i==3;
  link.mEndResetFlg0=i==4?daPy_py_c::ERFLG0_FORCE_AUTO_JUMP:0;childStored=i!=5;
  assert(begin()==HOOK_CONTINUE&&!link.checkEndResetFlg0(daPy_py_c::ERFLG0_NOT_AUTO_JUMP));end();
 }
 link={};childStored=true;eventRunning=false;
 manualJump=true;assert(begin()==HOOK_SKIP_ORIGINAL&&result==TRUE&&!link.mEndResetFlg0);end();
 manualJump=false;ability=true;assert(begin()==HOOK_SKIP_ORIGINAL&&result==TRUE&&!link.mEndResetFlg0);end();
 ability=false;parentEnabled=false;assert(begin()==HOOK_CONTINUE&&!link.mEndResetFlg0&&!childStored);end();
 assert(s_autoJumpFlagScopes.empty());
}
'''
fixture=fixture.replace('// CONFIG','\n'.join(function(config,s) for s in (
    'void enforce_auto_jump_dependency','void on_jump_setting_changed','bool disable_auto_jump_enabled'))+'\n'+function(ui,'bool auto_jump_setting_disabled'))
fixture=fixture.replace('// JUMP',jump[jump.index('struct AutoJumpFlagScope'):jump.index('HookAction before_proc_auto_jump')])
assert 'register_bool("disable-auto-jump", false, s_disableAutoJump)' in config
assert 'hook_add_post<CheckAutoJumpAction>(svc_hook, after_check_auto_jump)' in jump
assert ui.index('"R Jump"')<ui.index('"Disable Auto Jump"')<ui.index('"Jump Height"')
with tempfile.TemporaryDirectory() as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test';cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Disable Auto Jump passed: default/dependency/reset, scoped veto, nested calls, preserved flags, human/wolf veto, manual/Gale and demo/forced-jump exemptions')
