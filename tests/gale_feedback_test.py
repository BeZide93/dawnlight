"""Production Gale readiness animation/audio and counter artwork alignment."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
visual = (root / 'src/jump_gale_visual.inc').read_text()
counter = (root / 'src/gale_counter.cpp').read_text()

def function(source, name):
    match = re.search(r'^[^\n]*\b' + name + r'\([^;{}]*\)\s*\{', source, re.M)
    start = match.start()
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

# Archive allocation needs game assets; run all lifecycle, animation, sound,
# transforms and shared model state restoration against instrumented objects.
visual = visual.replace(function(visual, 'prepare'),
    '    bool prepare() { if (failed) return false; model = &modelFixture; return true; }')
fixture = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
using u8 = unsigned char;
using Mtx = float[3][4];
struct cXyz { float x,y,z; cXyz(float x=0,float y=0,float z=0):x(x),y(y),z(z){} };
using J3DJointCallBack = void(*)();
void native_callback() {}
struct Joint {
    J3DJointCallBack callback = native_callback;
    void* calc = nullptr;
    auto getCallBack(){return callback;} void setCallBack(J3DJointCallBack p){callback=p;}
    auto getMtxCalc(){return calc;} void setMtxCalc(void* p){calc=p;}
};
struct J3DModelData {
    Joint joint;
    unsigned getJointNum(){return 1;} Joint* getJointNodePointer(unsigned){return &joint;}
};
struct J3DModel {
    J3DModelData data; cXyz scale;
    void setBaseScale(cXyz s){scale=s;} void setBaseTRMtx(void*){}
    J3DModelData* getModelData(){return &data;}
} modelFixture;
struct Animation {
    int plays=0,resets=0;float frame=0;
    void setFrame(float f){frame=f;++resets;} float getFrame(){return frame;}
    void play(){++plays;frame+=1;} void entry(J3DModelData*){}
    Animation* getBckAnm(){return this;} Animation* getBtkAnm(){return this;}
};
using mDoExt_bckAnm=Animation;using mDoExt_btkAnm=Animation;
struct JKRSolidHeap {};
void mDoExt_destroySolidHeap(JKRSolidHeap*){}
struct daAlink_c {int tevStr=0;};
struct {void setLightTevColorType_MAJI(J3DModel*,int*){}} g_env_light;
int draws=0;
void mDoExt_modelUpdateDL(J3DModel* m){++draws;assert(!m->data.joint.callback);}
struct mDoMtx_stack_c {
    static inline cXyz position;
    static void transS(float x,float y,float z){position={x,y,z};}
    static void* get(){return nullptr;}
};
constexpr int Z2SE_BOOM_TORNADO=1;
struct Audio {
    int calls=0;cXyz pos;
    template<class... T> void seStartLevel(int id,cXyz* p,T...){assert(id==Z2SE_BOOM_TORNADO);++calls;pos=*p;}
} audio;
Audio* Z2GetAudioMgr(){return &audio;}
// VISUAL
struct J2DPane {
    int type=16;bool visible=true;float left=0,width=0;
    J2DPane *child=nullptr,*next=nullptr;
    bool isVisible(){return visible;} int getTypeID(){return type;}
    auto* getFirstChildPane(){return child;} auto* getNextChildPane(){return next;}
};
struct CPaneMgr {
    J2DPane root;float scale=1,offset=0;
    auto* getPanePtr(){return &root;}
    cXyz getGlobalVtx(J2DPane* p,Mtx*,u8 corner,bool,int){return {offset+scale*(p->left+(corner&1?p->width:0)),0,0};}
};
// COUNTER
void near(float a,float b){assert(std::fabs(a-b)<0.001f);}
int main(){
    GaleVisual effect;daAlink_c link;
    effect.update();effect.draw(&link);assert(audio.calls==0&&draws==0);
    effect.hold_ready({10,20,30});assert(effect.readyLoop&&audio.calls==1);
    for(int tick=0;tick<180;++tick){
        effect.hold_ready({11,21,31});effect.update();effect.draw(&link);
        assert(effect.readyLoop&&effect.ticks==0);
        near(modelFixture.scale.x,2);near(modelFixture.scale.y,0.1f);near(modelFixture.scale.z,2);
        near(mDoMtx_stack_c::position.y,27);near(audio.pos.y,21);
        assert(modelFixture.data.joint.callback==native_callback);
    }
    assert(effect.motion.plays==180&&effect.texture.plays==180);
    assert(audio.calls==181); // No restarted sound/animation on every hold call.
    effect.stop_ready();const int before=audio.calls,drawBefore=draws;
    effect.update();effect.draw(&link);assert(audio.calls==before&&draws==drawBefore);
    effect.hold_ready({10,20,30});effect.launch({10,20,30});effect.draw(&link);
    assert(!effect.readyLoop&&effect.ticks==60);
    near(modelFixture.scale.y,2.5f);near(mDoMtx_stack_c::position.y,170);
    for(int tick=0;tick<60;++tick)effect.update();
    const int ended=audio.calls;effect.update();effect.draw(&link);assert(audio.calls==ended);
    effect.hold_ready({});effect.release();assert(!effect.readyLoop&&!effect.model&&effect.ticks==0);
    effect.failed=true;effect.hold_ready({});assert(!effect.readyLoop);
    // Container origin at zero, visible artwork extends left. Both full/spent
    // icons and HUD scaling must align their actual picture edge to the anchor.
    for(float scale:{0.5f,0.65f,1.0f,1.5f})for(float left:{-16.0f,-20.0f}){
        CPaneMgr pane;pane.scale=scale;
        J2DPane image{18,true,left,32},glow{18,false,-50,100};
        pane.root.child=&image;image.next=&glow;
        const float anchor=140,correction=anchor-icon_left(&pane,0);
        assert(correction>anchor);pane.offset=correction;near(icon_left(&pane,0),anchor);
    }
    CPaneMgr empty;near(icon_left(&empty,12),12);
}
'''
fixture = fixture.replace('// VISUAL', visual).replace('// COUNTER',
    function(counter, 'include_icon_left') + '\n' + function(counter, 'icon_left'))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Gale feedback passed: looping floor wind/audio, launch transition, cleanup and icon left-edge alignment')
