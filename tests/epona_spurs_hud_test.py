"""Exercise real spur draw callbacks: spacing, icon/flash scale and restoration."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/epona_spurs_hud.cpp').read_text()
callbacks = source[source.index('struct SpurPaneState'):source.index('} // namespace')]
fixture = r'''
#include <array>
#include <algorithm>
#include <cassert>
#include <cmath>
using std::size_t;
struct ModContext {};
enum HookAction {HOOK_CONTINUE};
namespace mods {
template<class T>T& arg_ref(void* a,int i){return *static_cast<T*>(static_cast<void**>(a)[i]);}
template<class T>T arg(void* a,int i){return arg_ref<T>(a,i);}
}
struct J2DPane {
    float x=7,y=9,sx=.8f,sy=.6f;int alpha=192;bool visible=true;
    float getTranslateX(){return x;}float getTranslateY(){return y;}
    float getScaleX(){return sx;}float getScaleY(){return sy;}
    void translate(float a,float b){x=a;y=b;}void scale(float a,float b){sx=a;sy=b;}
};
struct CPaneMgr {J2DPane pane;J2DPane* getPanePtr(){return &pane;}};
struct dMeterHakusha_c {
    struct Data {float pos_x=0,pos_y=0;int flags=0;};
    Data mHakushaData[12];float mHakushaAnimFrame[12]{};
    float mButtonAPosX=0,mButtonAPosY=0;
    CPaneMgr* mpHakushaOn=nullptr;CPaneMgr* mpHakushaOff=nullptr;CPaneMgr* mpButtonA=nullptr;
    int count=6;int getHakushaNum(){return count;}
};
struct Transform {float offset_x=0,offset_y=0,scale=1;} transform;
bool custom=true;
Transform hud_layout_epona_spurs_transform(){return custom?transform:Transform{};}
// CALLBACKS
void near(float a,float b){assert(std::abs(a-b)<.002f);}
int main(){
    dMeterHakusha_c meter;dMeterHakusha_c* ptr=&meter;void* args[]={&ptr};
    CPaneMgr on,off,button;meter.mpHakushaOn=&on;meter.mpHakushaOff=&off;meter.mpButtonA=&button;
    for(int count:{6,12}) for(bool enabled:{true,false}) {
        meter.count=count;custom=enabled;
        for(int frame=0;frame<300;++frame) {
            // Includes scene/HUD base changes and all refreshes between ticks.
            const float bx=50+frame%7,by=80+frame%11;
            for(int i=0;i<count;++i){meter.mHakushaData[i]={bx+i*24,by,i%3};meter.mHakushaAnimFrame[i]=2;}
            meter.mButtonAPosX=bx+count*24+5;meter.mButtonAPosY=by+8;
            const float ax=meter.mButtonAPosX,ay=meter.mButtonAPosY;
            transform={float(frame%9-4)*20,float(frame%5-2)*15,frame%2?.5f:2.f};
            for(int draw=0;draw<3;++draw) {
                before_spurs_draw(nullptr,args,nullptr,nullptr);
                const float scale=enabled?transform.scale:1;
                const float dx=enabled?transform.offset_x:0,dy=enabled?transform.offset_y:0;
                for(int i=0;i<count;++i) {
                    near(meter.mHakushaData[i].pos_x,bx+i*24*scale+dx);
                    near(meter.mHakushaData[i].pos_y,by+dy);
                    assert(meter.mHakushaData[i].flags==i%3);
                    // Native draw may advance refill animation at render rate.
                    meter.mHakushaAnimFrame[i]+=1;
                }
                near(meter.mButtonAPosX,bx+(ax-bx)*scale+dx);
                near(meter.mButtonAPosY,by+(ay-by)*scale+dy);
                for(auto* mgr:{&on,&off,&button}){
                    near(mgr->pane.sx,.8f*scale);near(mgr->pane.sy,.6f*scale);
                    assert(mgr->pane.alpha==192&&mgr->pane.visible);
                    mgr->pane.translate(400,300); // native per-icon draw
                }
                float flashX=123,flashY=456,flashFrame=18,flashScale=1.4f;
                void* flashArgs[]={nullptr,&flashX,&flashY,&flashFrame,&flashScale};
                before_spurs_flash(nullptr,flashArgs,nullptr,nullptr);
                near(flashScale,1.4f*scale);near(flashX,123);near(flashY,456);near(flashFrame,18);
                after_spurs_draw(nullptr,args,nullptr,nullptr);
                assert(!s_spurs.owner);
                for(int i=0;i<count;++i){
                    near(meter.mHakushaData[i].pos_x,bx+i*24);near(meter.mHakushaData[i].pos_y,by);
                    near(meter.mHakushaAnimFrame[i],3.f+draw);
                }
                near(meter.mButtonAPosX,ax);near(meter.mButtonAPosY,ay);
                for(auto* mgr:{&on,&off,&button}){near(mgr->pane.sx,.8f);near(mgr->pane.sy,.6f);}
                flashScale=1.4f;before_spurs_flash(nullptr,flashArgs,nullptr,nullptr);near(flashScale,1.4f);
            }
        }
    }
    custom=true;transform={};before_spurs_draw(nullptr,args,nullptr,nullptr);assert(!s_spurs.owner);
    transform={-50,25,1.5f};meter.mpHakushaOn=meter.mpHakushaOff=meter.mpButtonA=nullptr;
    before_spurs_draw(nullptr,args,nullptr,nullptr);after_spurs_draw(nullptr,args,nullptr,nullptr);
    meter.count=0;before_spurs_draw(nullptr,args,nullptr,nullptr);assert(!s_spurs.owner);
    ptr=nullptr;before_spurs_draw(nullptr,args,nullptr,nullptr);after_spurs_draw(nullptr,args,nullptr,nullptr);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture.replace('// CALLBACKS', callbacks))
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Epona spurs HUD: spacing, full/used icons, A button, flashes, redraws and native state passed')
