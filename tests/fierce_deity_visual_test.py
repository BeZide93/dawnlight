"""Exercise the actual render callbacks with a recording GX/J3D environment."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/fierce_deity_visual.cpp').read_text()
callbacks = source[source.index('struct DrawScope'):source.index('} // namespace')]
fixture = r'''
#include <array>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <algorithm>
using u8 = uint8_t; using u16 = uint16_t;
using GXTexCoordID = int; using GXTexMapID = int; using GXTevSwapSel = int;
using GXTevStageID = int;
constexpr int GX_CH_RED=0, GX_CH_GREEN=1, GX_CH_BLUE=2, GX_CH_ALPHA=3;
constexpr int GX_COLOR1=1, GX_COLOR1A1=5, GX_TRUE=1, GX_FALSE=0;
constexpr int GX_SRC_REG=0, GX_LIGHT_NULL=0, GX_LIGHT0=1, GX_DF_CLAMP=2, GX_AF_NONE=2;
constexpr int GX_TEV_ADD=0, GX_TEV_SUB=1, GX_TB_ZERO=0, GX_CS_SCALE_1=0,
              GX_CS_SCALE_2=1, GX_TEVPREV=0, GX_TEXCOORD_NULL=255, GX_TEXMAP_NULL=255;
constexpr int GX_CC_ZERO=15, GX_CC_TEXC=8, GX_CC_CPREV=0, GX_CC_RASC=10;
constexpr int GX_CA_ZERO=7, GX_CA_APREV=0;
struct GXColor { u8 r, g, b, a; };
struct RecordedStage { std::array<int,4> color{}, alpha{}; int op=0, scale=0, swap=0; };
std::array<RecordedStage,16> gpuStages{};
std::array<std::array<int,4>,4> gpuSwaps{};
int gpuCount=3, gxWrites=0, restoreCalls=0;
GXColor gpuAmbient{}, gpuMaterial{};
void GXSetTevOrder(int i,int,int,int) { assert(i<16); ++gxWrites; }
void GXSetTevDirect(int) { ++gxWrites; }
void GXSetTevSwapMode(int i,int,int tex) { gpuStages[i].swap=tex; ++gxWrites; }
void GXSetTevColorOp(int i,int op,int,int scale,int,int) {
    gpuStages[i].op=op; gpuStages[i].scale=scale; ++gxWrites;
}
void GXSetTevAlphaIn(int i,int a,int b,int c,int d) { gpuStages[i].alpha={a,b,c,d}; ++gxWrites; }
void GXSetTevAlphaOp(int,int,int,int,int,int) { ++gxWrites; }
void GXSetTevColorIn(int i,int a,int b,int c,int d) { gpuStages[i].color={a,b,c,d}; ++gxWrites; }
void GXSetNumTevStages(int n) { assert(n<=16); gpuCount=n; ++gxWrites; }
void GXSetTevSwapModeTable(int n,int r,int g,int b,int a) { gpuSwaps[n]={r,g,b,a}; ++gxWrites; }
void GXSetNumChans(int) { ++gxWrites; }
void GXSetChanAmbColor(int,GXColor v) { gpuAmbient=v; ++gxWrites; }
void GXSetChanMatColor(int,GXColor v) { gpuMaterial=v; ++gxWrites; }
void GXSetChanCtrl(int,int,int,int,int,int,int) { ++gxWrites; }
struct ModContext {};
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
namespace mods { template<class T> T arg(void* a,int i) { return static_cast<T>(static_cast<void**>(a)[i]); } }
struct J3DTevStage { u8 mTevSwapModeInfo=0; };
struct J3DTevSwapModeTable {
    int r=0,g=1,b=2,a=3;
    int getR() const {return r;} int getG() const {return g;}
    int getB() const {return b;} int getA() const {return a;}
};
struct J3DTevOrder { u8 mTexCoord=0, map=0; u8 getTexMap() const {return map;} };
struct J3DTevBlock {
    int count=3;
    std::array<J3DTevStage,16> stages{};
    std::array<J3DTevOrder,16> orders{};
    std::array<J3DTevSwapModeTable,4> swaps{};
    u8 getTevStageNum() {return count;}
    J3DTevStage* getTevStage(unsigned i) {assert(i<static_cast<unsigned>(count)); return &stages[i];}
    J3DTevSwapModeTable* getTevSwapModeTable(unsigned i) {return &swaps.at(i);}
    J3DTevOrder* getTevOrder(unsigned i) {assert(i<static_cast<unsigned>(count)); return &orders[i];}
    u16 getTexNo(unsigned i) {assert(i<8); return 0;}
};
struct Names { const char* name; const char* getName(int) const {return name;} };
struct Texture { int getNum() const {return 1;} };
struct J3DModelData {
    Names materials{"al_body"}, textures{"al_eyeball"}; Texture texture;
    Names* getMaterialName() {return &materials;} Names* getTextureName() {return &textures;}
    Texture* getTexture() {return &texture;}
};
struct ColorBlock {
    GXColor ambient{10,20,30,61}, material{40,50,60,93};
    GXColor* getAmbColor(int) {return &ambient;}
};
struct J3DMaterial {
    J3DTevBlock tev; ColorBlock color;
    J3DTevBlock* getTevBlock() {return &tev;} int getIndex() {return 0;}
    ColorBlock* getColorBlock() {return &color;}
    GXColor* getMatColor(int) {return &color.material;}
};
struct J3DShape { J3DMaterial* material; J3DMaterial* getMaterial() const {return material;} };
struct DisplayList { void callDL() { ++restoreCalls; gpuCount=3; } };
struct MatPacket { DisplayList list; DisplayList* getDisplayListObj() {return &list;} void callDL() {list.callDL();} };
struct J3DModel {
    J3DModelData data; MatPacket packet;
    J3DModelData* getModelData() {return &data;} MatPacket* getMatPacket(int) {return &packet;}
};
struct J3DShapePacket {
    J3DModel* model; J3DShape* shape; DisplayList diff;
    J3DModel* getModel() {return model;} J3DShape* getShape() {return shape;}
    DisplayList* getDisplayListObj() {return &diff;}
};
struct daAlink_c {
    J3DModel *mpLinkModel=nullptr,*mpLinkFaceModel=nullptr,*mpLinkHatModel=nullptr,
    *mpLinkHandModel=nullptr,*mpDemoFCBlendModel=nullptr,*mpDemoFCTongueModel=nullptr,
    *mpDemoHLTmpModel=nullptr,*mpDemoHRTmpModel=nullptr,*mpLinkBootModels[2]{};
};
daAlink_c link;
bool darkActive=true;
daAlink_c* daAlink_getAlinkActorClass() {return &link;}
bool fierce_deity_dark_visual_active() {return darkActive;}
'''
checks = r'''
void draw_begin(J3DShapePacket& p) {
    void* args[]={&p}; before_packet_draw(nullptr,args,nullptr,nullptr);
    void* shape[]={p.shape}; before_shape_draw(nullptr,shape,nullptr,nullptr);
}
void draw_end() {
    after_shape_draw(nullptr,nullptr,nullptr,nullptr);
    after_packet_draw(nullptr,nullptr,nullptr,nullptr);
    assert(s_drawDepth==0);
}
float eye_mask(int nativeCount, float r, float b) {
    float previous=0;
    for(int i=nativeCount;i<gpuCount;++i) {
        const auto& s=gpuStages[i];
        const float tex=gpuSwaps[s.swap][0]==GX_CH_RED ? r : b;
        auto value=[&](int v) {switch(v) {
            case GX_CC_ZERO:return 0.0f; case GX_CC_TEXC:return tex;
            case GX_CC_CPREV:return previous; case GX_CC_RASC:return 1.0f;
        } assert(false); return 0.0f;};
        const auto& c=s.color;
        const float mix=value(c[0])*(1-value(c[2]))+value(c[1])*value(c[2]);
        previous=std::clamp((value(c[3])+(s.op==GX_TEV_SUB ? -mix:mix))*(s.scale?2:1),0.0f,1.0f);
        assert((s.alpha==std::array<int,4>{GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,GX_CA_APREV}));
    }
    return previous;
}
int main() {
    J3DMaterial mat; J3DShape shape{&mat}; J3DModel player, other;
    link.mpLinkModel=&player;
    J3DShapePacket p{&player,&shape,{}}, foreign{&other,&shape,{}};
    draw_begin(p);
    assert(gpuCount==4 && gpuMaterial.r==96);
    assert(gpuMaterial.a==93 && gpuAmbient.a==61); // Aurora's full RGBA writes
    draw_end();
    assert(gpuCount==3 && restoreCalls==2); // base + per-instance animation DL
    int writes=gxWrites;
    draw_begin(foreign); draw_end(); // same material pointer, different actor
    assert(gxWrites==writes && restoreCalls==2);
    darkActive=false;
    draw_begin(p); draw_end();
    assert(gxWrites==writes);
    darkActive=true;

    // Alpha-tested geometry preserves alpha at every appended stage. Eye masks
    // must reject white sclera and retain red-channel iris at moving UVs.
    for(const char* name : {"al_eyeballL_m","bl_eyeballR_m","ml_eyeballL_m","zl_eyeballR_m"}) {
        player.data.materials.name=name;
        draw_begin(p);
        assert(gpuCount==6 && gpuMaterial.r==230 && gpuMaterial.g==8);
        assert(eye_mask(3,1,1)==0); // sclera
        assert(eye_mask(3,1,0)==1); // iris
        assert(eye_mask(3,0,0)==0); // pupil
        assert(eye_mask(3,0.6f,0.4f)>0.39f && eye_mask(3,0.6f,0.4f)<0.41f);
        draw_end();
    }
    // Capacity limits and foreign layouts never overflow the 16-stage pipeline.
    mat.tev.count=16;
    writes=gxWrites; draw_begin(p); draw_end(); assert(gxWrites==writes);
    mat.tev.count=14;
    draw_begin(p); assert(gpuCount==15); draw_end(); // body fallback
    mat.tev.count=3;
    player.data.textures.name="custom_without_native_mask";
    draw_begin(p); assert(gpuCount==4 && gpuMaterial.r==96); draw_end();
    player.data.textures.name="al_eyeball";
    mat.tev.stages[0].mTevSwapModeInfo=0;
    mat.tev.stages[1].mTevSwapModeInfo=5;
    mat.tev.stages[2].mTevSwapModeInfo=14; // all swap tables in use
    draw_begin(p); assert(gpuCount==4); draw_end();
    // Nested unrelated packets must not inherit the player's effect; cancellation
    // before the shape draw must not leave any state or pointers behind.
    void* args[]={&p}; before_packet_draw(nullptr,args,nullptr,nullptr);
    draw_begin(foreign);
    after_shape_draw(nullptr,nullptr,nullptr,nullptr);
    after_packet_draw(nullptr,nullptr,nullptr,nullptr);
    assert(s_drawDepth==1);
    after_packet_draw(nullptr,nullptr,nullptr,nullptr);
    assert(s_drawDepth==0 && s_drawScopes[0].packet==nullptr);
    for(int i=0;i<20;++i) before_packet_draw(nullptr,args,nullptr,nullptr);
    void* shapeArgs[]={&shape}; writes=gxWrites;
    before_shape_draw(nullptr,shapeArgs,nullptr,nullptr);
    assert(gxWrites==writes); // bounded recursion guard
    for(int i=0;i<20;++i) after_packet_draw(nullptr,nullptr,nullptr,nullptr);
    assert(s_drawDepth==0);
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp = Path(temp) / 'visual.cpp'
    exe = Path(temp) / 'visual'
    cpp.write_text(fixture + callbacks + checks)
    subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Fierce Deity render isolation, eye mask math, alpha, capacity and cleanup: passed')
