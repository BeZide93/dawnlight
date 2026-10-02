"""Run the production KH2 bridge and meter draw hook against the supplied API."""
from pathlib import Path
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1] if len(sys.argv) > 1 else root/'dusklight')/'sdk/include'
source = (root/'src/fierce_deity.cpp').read_text()
start = source.index('void after_meter_draw(')
end = source.index('\ntemplate <typename Hook>',start)
fixture = r'''
#include "kh2_hud_compat.cpp"
#include <cassert>
#include <limits>
#include <iostream>
struct ModContext{} context;
ModContext* mod_ctx=&context;
Kh2HudDriveState reported=KH2HUD_DRIVE_STATE_INIT;
int sets=0,clears=0,nativeDraws=0;bool gaugePresent=false;
ModResult result=MOD_OK;
ModResult set_drive(ModContext* ctx,const Kh2HudDriveState* state){
    assert(ctx==&context&&state->struct_size==sizeof(*state));++sets;
    if(result==MOD_OK){reported=*state;gaugePresent=true;}return result;
}
void clear_drive(ModContext* ctx){assert(ctx==&context);++clears;gaugePresent=false;}
Kh2HudDriveService provider{SERVICE_HEADER(Kh2HudDriveService,1,0),set_drive,clear_drive};
const Kh2HudDriveService* svc_kh2hud_drive=nullptr;
namespace dawnlight {
DawnlightFierceDeityHudState current{sizeof(current),1,1,0,40};
bool renderer=false;
DawnlightFierceDeityHudState fierce_deity_hud_state(){return current;}
bool fierce_deity_hud_renderer_registered(){return renderer;}
}
using namespace dawnlight;
struct dMeter2Draw_c{} meter;
namespace mods {template<class T>T arg(void* ptr,int){return static_cast<T>(ptr);}}
void draw_fierce_meter(dMeter2Draw_c* ptr){assert(ptr==&meter);++nativeDraws;}
// DRAW_HOOK
void draw(){after_meter_draw(nullptr,&meter,nullptr,nullptr);}
int main(){
    assert(!update_kh2_drive());draw();assert(nativeDraws==1);
    svc_kh2hud_drive=&provider;
    draw();assert(nativeDraws==1&&gaugePresent);
    assert(reported.type==KH2HUD_DRIVE_GAUGE&&reported.gauge==40&&reported.level==0&&reported.max_level==1);
    current.percentage=99.9f;assert(update_kh2_drive());assert(reported.gauge==99&&reported.level==0);
    current.percentage=100;assert(update_kh2_drive());assert(reported.gauge==0&&reported.level==1);
    current.active=1;assert(update_kh2_drive());
    assert(reported.type==KH2HUD_DRIVE_FORM&&reported.form_time==100&&reported.form_time_max==100&&reported.form_div==1);
    current.percentage=12.5f;assert(update_kh2_drive());assert(reported.form_time==12.5f);
    current.active=0;current.percentage=0;assert(update_kh2_drive());
    assert(reported.type==KH2HUD_DRIVE_GAUGE&&reported.level==0&&reported.gauge==0);
    // Menu/pause, feature off, missing player: clear even without a HUD draw.
    current.visible=0;assert(!update_kh2_drive()&&!gaugePresent&&clears==1);
    assert(!update_kh2_drive()&&clears==1);current.visible=1;
    assert(update_kh2_drive());current.enabled=0;assert(!update_kh2_drive()&&!gaugePresent);
    current.enabled=1;assert(update_kh2_drive());
    renderer=true;draw();assert(!gaugePresent&&nativeDraws==2); // public renderer takes precedence
    renderer=false;assert(update_kh2_drive());
    result=MOD_UNAVAILABLE;draw();assert(!gaugePresent&&nativeDraws==3);
    result=MOD_OK;assert(update_kh2_drive());
    clear_kh2_drive();assert(!gaugePresent); // save/player reset and mod shutdown
    const int previous=clears;clear_kh2_drive();assert(clears==previous);
    assert(update_kh2_drive());svc_kh2hud_drive=nullptr;gaugePresent=false;draw();assert(nativeDraws==4);
    svc_kh2hud_drive=&provider;assert(update_kh2_drive());
    provider.set_drive=nullptr;draw();assert(nativeDraws==5&&!gaugePresent);provider.set_drive=set_drive;
    current.percentage=std::numeric_limits<float>::quiet_NaN();assert(!update_kh2_drive());
    current.percentage=120;assert(update_kh2_drive()&&reported.level==1);
    current.percentage=-1;assert(update_kh2_drive()&&reported.gauge==0);
    clear_kh2_drive();
    std::cout<<"KH2 HUD: DRIVE/MAX/FORM, visibility, owner priority, failure fallback and reset passed\n";
}
'''.replace('// DRAW_HOOK',source[start:end])
# Keep cleanup and no-HUD-draw updates wired into production lifecycle paths.
reset = source[source.index('void reset_for_link('):source.index('bool same_link(')]
shutdown = source[source.index('void shutdown_fierce_deity()'):source.index('DawnlightFierceDeityHudState fierce_deity_hud_state()')]
assert 'clear_kh2_drive();' in reset and 'clear_kh2_drive();' in shutdown
mod=(root/'src/mod.cpp').read_text()
assert 'IMPORT_OPTIONAL_SERVICE(Kh2HudDriveService, svc_kh2hud_drive)' in mod
assert 'update_kh2_drive();' in mod[mod.index('MOD_EXPORT ModResult mod_update('):mod.index('MOD_EXPORT ModResult mod_shutdown(')]
with tempfile.TemporaryDirectory() as tmp:
    cpp,binary=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes',
        '-I'+str(sdk),'-I'+str(root/'include'),'-I'+str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
