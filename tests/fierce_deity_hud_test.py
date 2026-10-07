"""Execute the public service against the real SDK ABI and production state query."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1] if len(sys.argv) > 1 else root/'dusklight')/'sdk/include'
source = (root/'src/fierce_deity.cpp').read_text()
start = source.index('DawnlightFierceDeityHudState fierce_deity_hud_state() {')
end = source.index('\nbool fierce_deity_active()', start)
percent_start = source.index('float gauge_percentage()')
percent_end = source.index('\n}', percent_start)+2
query = source[percent_start:percent_end]+'\n'+source[start:end]
fixture = r'''
#include "fierce_deity_hud.cpp"
#include <algorithm>
#include <cassert>
#include <iostream>
struct ModContext {int id;};
ModContext provider{0},owner{1},other{2};
ModContext* mod_ctx=&provider;
ModLifecycleFn observer=nullptr;
ModResult watchResult=MOD_OK;int watches=0,unwatches=0;
ModResult watch(ModContext* ctx,ModLifecycleFn fn,void*,uint64_t* handle){
    assert(ctx==&provider);if(watchResult!=MOD_OK)return watchResult;
    observer=fn;*handle=9;++watches;return MOD_OK;
}
ModResult unwatch(ModContext* ctx,uint64_t handle){assert(ctx==&provider&&handle==9);++unwatches;observer=nullptr;return MOD_OK;}
HostService host{};
const HostService* svc_host=&host;
namespace dawnlight {
struct {bool active=false;float meter=0;} s_state;
bool setting=true,player=true,hidden=false;
float capacity=100;float maximum_gauge(){return capacity;}
void* daAlink_getAlinkActorClass(){return player?&player:nullptr;}
bool same_link(void* ptr){return ptr!=nullptr;}
bool fierce_deity_enabled(){return setting;}
bool freeTransform=false;bool dark_link_free_transform(){return freeTransform;}
bool menu_or_pause_active(){return hidden;}
// QUERY
}
using namespace dawnlight;
int draws=0;bool handled=true;bool recursive=false;bool unregisterDuringDraw=false;
bool render(ModContext* ctx,const DawnlightFierceDeityHudFrame* frame,void* user){
    assert(ctx==&owner&&user==&draws);++draws;
    assert(frame->state.percentage==25&&frame->alpha==0.5f);
    if(recursive)assert(!draw_external_fierce_deity_hud(*frame));
    if(unregisterDuringDraw)assert(s_service.unregister_renderer(ctx)==MOD_OK);
    return handled;
}
int main(){
    host.watch_mod_lifecycle=watch;host.unwatch_mod_lifecycle=unwatch;
    assert(s_service.header.major_version==1&&s_service.header.minor_version==0);
    DawnlightFierceDeityHudState state=DAWNLIGHT_FIERCE_DEITY_HUD_STATE_INIT;
    assert(s_service.get_state(&owner,&state)==MOD_UNAVAILABLE);
    assert(s_service.register_renderer(&owner,render,&draws)==MOD_UNAVAILABLE);
    watchResult=MOD_ERROR;assert(initialize_fierce_deity_hud(nullptr)==MOD_ERROR);
    watchResult=MOD_OK;assert(initialize_fierce_deity_hud(nullptr)==MOD_OK);
    assert(initialize_fierce_deity_hud(nullptr)==MOD_OK&&watches==1);
    assert(s_service.get_state(nullptr,&state)==MOD_INVALID_ARGUMENT);
    assert(s_service.get_state(&owner,nullptr)==MOD_INVALID_ARGUMENT);
    state.struct_size=0;assert(s_service.get_state(&owner,&state)==MOD_INVALID_ARGUMENT);
    state.struct_size=sizeof(state);
    s_state.meter=25;
    assert(s_service.get_state(&owner,&state)==MOD_OK);
    assert(state.enabled&&state.visible&&!state.active&&state.percentage==25);
    freeTransform=true;s_state.active=true;s_service.get_state(&owner,&state);
    assert(state.enabled&&!state.visible&&state.active&&state.percentage==25);freeTransform=false;
    hidden=true;s_state.active=true;s_service.get_state(&owner,&state);
    assert(!state.visible&&state.active);hidden=false;
    setting=false;s_service.get_state(&owner,&state);assert(!state.enabled&&!state.visible);
    setting=true;player=false;s_service.get_state(&owner,&state);
    assert(!state.visible&&!state.active&&state.percentage==0);player=true;
    s_state.meter=101;s_service.get_state(&owner,&state);assert(state.percentage==100);
    capacity=200;s_state.meter=50;s_service.get_state(&owner,&state);assert(state.percentage==25);capacity=100;
    s_state.meter=-5;s_service.get_state(&owner,&state);assert(state.percentage==0);s_state.meter=25;
    s_service.get_state(&owner,&state);
    DawnlightFierceDeityHudFrame frame{};frame.struct_size=sizeof(frame);frame.state=state;frame.alpha=0.5f;
    assert(!draw_external_fierce_deity_hud(frame));
    assert(s_service.register_renderer(nullptr,render,nullptr)==MOD_INVALID_ARGUMENT);
    assert(s_service.register_renderer(&owner,nullptr,nullptr)==MOD_INVALID_ARGUMENT);
    assert(s_service.register_renderer(&owner,render,&draws)==MOD_OK);
    assert(s_service.register_renderer(&other,render,&draws)==MOD_CONFLICT);
    assert(s_service.unregister_renderer(&other)==MOD_CONFLICT);
    assert(draw_external_fierce_deity_hud(frame)&&draws==1);
    handled=false;assert(!draw_external_fierce_deity_hud(frame)&&draws==2);handled=true;
    frame.state.visible=0;assert(!draw_external_fierce_deity_hud(frame)&&draws==2);frame.state.visible=1;
    recursive=true;assert(draw_external_fierce_deity_hud(frame)&&draws==3);recursive=false;
    assert(s_service.register_renderer(&owner,render,&draws)==MOD_OK);
    observer(&provider,&other,"other",MOD_LIFECYCLE_DETACHED,nullptr);
    assert(draw_external_fierce_deity_hud(frame)&&draws==4);
    observer(&provider,&owner,"owner",MOD_LIFECYCLE_DETACHED,nullptr);
    assert(!draw_external_fierce_deity_hud(frame)&&draws==4);
    assert(s_service.register_renderer(&owner,render,&draws)==MOD_OK);
    unregisterDuringDraw=true;assert(draw_external_fierce_deity_hud(frame));
    assert(!draw_external_fierce_deity_hud(frame));unregisterDuringDraw=false;
    assert(s_service.register_renderer(&owner,render,&draws)==MOD_OK);
    shutdown_fierce_deity_hud();assert(unwatches==1&&!observer);
    assert(!draw_external_fierce_deity_hud(frame));
    assert(s_service.get_state(&owner,&state)==MOD_UNAVAILABLE&&!state.visible);
    shutdown_fierce_deity_hud();assert(unwatches==1);
    assert(initialize_fierce_deity_hud(nullptr)==MOD_OK);
    assert(!draw_external_fierce_deity_hud(frame)); // no callback survives reload
    shutdown_fierce_deity_hud();
    std::cout<<"Fierce Deity HUD API: state, ABI, ownership, fallback and unload checks passed\n";
}
'''.replace('// QUERY',query)
with tempfile.TemporaryDirectory() as tmp:
    cpp,binary=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes',
                    '-I'+str(sdk),'-I'+str(root/'include'),'-I'+str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
