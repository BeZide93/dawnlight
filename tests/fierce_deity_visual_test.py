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
using u8 = uint8_t; using u16 = uint16_t; using u32 = uint32_t;
using GXTexCoordID = int; using GXTexMapID = int; using GXTevSwapSel = int;
using GXTevStageID = int; using GXTevColorArg = int; using GXTevOp = int; using GXTevScale = int;
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
// Decode real big-endian GX display-list bytes, including one-write BP masks.
// Keeping live registers separate from CPU material objects is essential: J3D
// display lists (including animations) do not refresh the GX setter shadows.
std::array<u32,256> bp{}, nativeBP{};
std::array<u32,0x1040> xf{}, nativeXF{};
u32 bpMask=0xFFFFFF;
void write_bp(u32 word) {
    const unsigned reg=word>>24;
    if(reg==0xFE) { bpMask=word&0xFFFFFF; return; }
    bp[reg]=(bp[reg]&~bpMask)|(word&bpMask);
    bpMask=0xFFFFFF;
    ++gxWrites;
}
void decode_state() {
    gpuCount=((bp[0]>>10)&15)+1;
    for(unsigned i=0;i<16;++i) {
        const u32 c=bp[0xC0+2*i], a=bp[0xC1+2*i];
        gpuStages[i].color={int((c>>12)&15),int((c>>8)&15),int((c>>4)&15),int(c&15)};
        gpuStages[i].alpha={int((a>>13)&7),int((a>>10)&7),int((a>>7)&7),int((a>>4)&7)};
        gpuStages[i].op=(c>>18)&1; gpuStages[i].scale=(c>>20)&3;
        gpuStages[i].swap=(a>>2)&3;
    }
    for(unsigned i=0;i<4;++i) {
        const u32 rg=bp[0xF6+2*i], ba=bp[0xF7+2*i];
        gpuSwaps[i]={int(rg&3),int((rg>>2)&3),int(ba&3),int((ba>>2)&3)};
    }
    auto color=[](u32 v) { return GXColor{u8(v>>24),u8(v>>16),u8(v>>8),u8(v)}; };
    gpuAmbient=color(xf[0x100B]); gpuMaterial=color(xf[0x100D]);
}
void GXCallDisplayList(const void* data, u32 size) {
    assert(size%32==0 && reinterpret_cast<uintptr_t>(data)%32==0);
    const auto* bytes=static_cast<const u8*>(data);
    unsigned offset=0;
    auto word=[&]() {
        assert(offset+4<=size);
        const u32 v=(u32(bytes[offset])<<24)|(u32(bytes[offset+1])<<16)|
                    (u32(bytes[offset+2])<<8)|bytes[offset+3];
        offset+=4; return v;
    };
    while(offset<size) {
        switch(bytes[offset++]) {
        case 0: break;
        case 0x61: write_bp(word()); break;
        case 0x10: {
            const u32 header=word(), reg=header&0xFFFF;
            assert((header>>16)==0 && reg<xf.size());
            xf[reg]=word(); ++gxWrites; break;
        }
        default: assert(false && "unexpected GX command");
        }
    }
    assert(bpMask==0xFFFFFF);
    decode_state();
}
// Aurora copies only numTexGens generators into its shader config. All others
// remain GX_MAX_TEXGENSRC (21), which is fatal if an active stage samples one.
bool shader_texgens_valid() {
    const unsigned count=((bp[0]>>10)&15)+1, texgens=bp[0]&15;
    for(unsigned i=0;i<count;++i) {
        const u32 order=bp[0x28+i/2]>>((i&1)*12);
        if((order&(1u<<6)) && ((order>>3)&7)>=texgens) return false;
    }
    return true;
}
void load_native(unsigned count) {
    for(unsigned i=0;i<bp.size();++i) bp[i]=(i*0x156713u)&0xFFFFFF;
    // Three texgens, one channel, culling and one indirect stage, depth freeze.
    bp[0]=3|(1u<<4)|((count-1)<<10)|(2u<<14)|(1u<<16)|(1u<<19);
    for(unsigned i=0;i<8;++i) bp[0x28+i]=0;
    for(unsigned i=0;i<count;++i) {
        const u32 order=(i%3)|((i%3)<<3)|(1u<<6);
        bp[0x28+i/2]|=order<<((i&1)*12);
    }
    for(unsigned i=0;i<xf.size();++i) xf[i]=(i*0xD315613u);
    nativeBP=bp; nativeXF=xf;
    decode_state();
    assert(shader_texgens_valid());
}
void assert_native_fields(unsigned count) {
    assert(shader_texgens_valid()); // regression: unhandled tcg src 21
    assert((bp[0]&~0x3C70u)==(nativeBP[0]&~0x3C70u));
    assert(((bp[0]>>4)&7)==2 && xf[0x1009]==2);
    for(unsigned i=0;i<count;++i) {
        const unsigned shift=(i&1)*12;
        assert(((bp[0x28+i/2]^nativeBP[0x28+i/2])&(0xFFFu<<shift))==0);
        assert(bp[0xC0+2*i]==nativeBP[0xC0+2*i]);
        assert(bp[0xC1+2*i]==nativeBP[0xC1+2*i]);
        assert(bp[0x10+i]==nativeBP[0x10+i]);
    }
    for(unsigned i=0xF6;i<=0xFD;++i) {
        assert((bp[i]&0xFFFFF0)==(nativeBP[i]&0xFFFFF0)); // native konst selectors
    }
    for(unsigned i=0;i<xf.size();++i) {
        if(i!=0x1009 && i!=0x100B && i!=0x100D && i!=0x100F) assert(xf[i]==nativeXF[i]);
    }
    for(unsigned i=count;i<static_cast<unsigned>(gpuCount);++i) {
        assert(bp[0x10+i]==0); // no indirect sampling from appended stages
        const u32 a=bp[0xC1+2*i];
        assert((a>>16)==8); // clamp, add, scale1, PREV
        assert((gpuStages[i].alpha==std::array<int,4>{7,7,7,0}));
    }
}
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
struct DisplayList { int calls=0; void callDL() { ++calls; ++restoreCalls; bp=nativeBP; xf=nativeXF; decode_state(); } };
struct J3DShapePacket;
struct J3DMatPacket { J3DShapePacket* shapes=nullptr; J3DShapePacket* getShapePacket() {return shapes;} DisplayList list; DisplayList* getDisplayListObj() {return &list;} void callDL() {list.callDL();} };
struct J3DModel {
    J3DModelData data; J3DMatPacket packet;
    J3DModelData* getModelData() {return &data;} J3DMatPacket* getMatPacket(int) {return &packet;}
};
struct J3DShapePacket {
    J3DModel* model; J3DShape* shape; DisplayList diff; J3DShapePacket* next=nullptr;
    J3DShapePacket* getNextPacket() {return next;}
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
    p.model->packet.shapes=&p;
    void* materialArgs[]={&p.model->packet};
    before_material_draw(nullptr,materialArgs,nullptr,nullptr);
    load_native(p.shape->material->tev.count);
    void* args[]={&p}; before_packet_draw(nullptr,args,nullptr,nullptr);
    void* shape[]={p.shape}; before_shape_draw(nullptr,shape,nullptr,nullptr);
    if(s_drawDepth<=s_drawScopes.size() && s_drawScopes[s_drawDepth-1].applied)
        assert_native_fields(p.shape->material->tev.count);
}
void draw_end() {
    after_shape_draw(nullptr,nullptr,nullptr,nullptr);
    after_packet_draw(nullptr,nullptr,nullptr,nullptr);
    after_material_draw(nullptr,nullptr,nullptr,nullptr);
    assert(s_drawDepth==0 && s_materialDepth==0);
}
void draw_shadow(J3DShapePacket& p) {
    // Real shadow image pass (dDlst_shadowControl_c / shadowReal_c):
    // one untextured stage, zero texgens, then direct shapePacket->drawFast().
    // Unused registers still contain previous textured material stages.
    load_native(p.shape->material->tev.count);
    bp[0]=0x004010; bp[0x28]=0x380000;
    decode_state();
    assert(shader_texgens_valid());
    const auto shadowBP=bp; const auto shadowXF=xf;
    const int shadowWrites=gxWrites, shadowRestores=restoreCalls;
    void* shadowPacketArgs[]={&p}; void* shadowShapeArgs[]={p.shape};
    before_packet_draw(nullptr,shadowPacketArgs,nullptr,nullptr);
    before_shape_draw(nullptr,shadowShapeArgs,nullptr,nullptr);
    assert(shader_texgens_valid()); // previously activated stale textured stages
    after_shape_draw(nullptr,nullptr,nullptr,nullptr);
    after_packet_draw(nullptr,nullptr,nullptr,nullptr);
    assert(bp==shadowBP && xf==shadowXF);
    assert(gxWrites==shadowWrites && restoreCalls==shadowRestores);

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
    // Negative control: a GX setter flushing GEN_MODE from a stale shadow
    // (one texgen) recreates the log's invalid generator with this native model.
    load_native(3);
    const u32 staleShadow=1|(2u<<4)|(3u<<10);
    write_bp(staleShadow);
    assert(!shader_texgens_valid());
    bp=nativeBP;
    assert(shader_texgens_valid());
    // A full TREF/KSEL shadow write would also corrupt the paired native stage.
    write_bp((0x29u<<24)|(1u<<19));
    assert((bp[0x29]&0xFFF)!=(nativeBP[0x29]&0xFFF));
    write_bp((0xF6u<<24)|4u);
    assert((bp[0xF6]&0xFFFFF0)!=(nativeBP[0xF6]&0xFFFFF0));

    J3DMaterial mat; J3DShape shape{&mat}; J3DModel player, other;
    link.mpLinkModel=&player;
    J3DShapePacket p{&player,&shape,{}}, foreign{&other,&shape,{}};
    draw_shadow(p);
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
    // Cover both halves of packed TREF registers and every legal stage count.
    // Baseline live TREF values intentionally differ from the CPU material's
    // orders (as they can after per-instance material animation).
    for(int count=1;count<=15;++count) {
        mat.tev.count=count;
        for(const char* name : {"al_body", "al_eyeballL_m"}) {
            player.data.materials.name=name;
            draw_begin(p);
            assert(gpuCount==count+(count<=13 && std::strstr(name,"eyeball") ? 3 : 1));
            draw_end();
            assert(bp==nativeBP && xf==nativeXF);
        }
    }
    // Batched materials can be owned by a different model. Only the player
    // receives the effect, and cleanup must replay the batch's actual base DL.
    mat.tev.count=3;
    player.data.materials.name="al_body";
    other.packet.shapes=&foreign; foreign.next=&p;
    void* batchArgs[]={&other.packet};
    before_material_draw(nullptr,batchArgs,nullptr,nullptr);
    load_native(3);
    void* foreignArgs[]={&foreign}; void* playerArgs[]={&p}; void* bodyArgs[]={&shape};
    writes=gxWrites;
    before_packet_draw(nullptr,foreignArgs,nullptr,nullptr);
    before_shape_draw(nullptr,bodyArgs,nullptr,nullptr);
    after_shape_draw(nullptr,nullptr,nullptr,nullptr);
    after_packet_draw(nullptr,nullptr,nullptr,nullptr);
    assert(gxWrites==writes);
    const int ownCalls=player.packet.list.calls, batchCalls=other.packet.list.calls;
    before_packet_draw(nullptr,playerArgs,nullptr,nullptr);
    before_shape_draw(nullptr,bodyArgs,nullptr,nullptr);
    assert(gpuCount==4); assert_native_fields(3);
    after_shape_draw(nullptr,nullptr,nullptr,nullptr);
    after_packet_draw(nullptr,nullptr,nullptr,nullptr);
    assert(player.packet.list.calls==ownCalls && other.packet.list.calls==batchCalls+1);
    after_material_draw(nullptr,nullptr,nullptr,nullptr);
    assert(s_materialDepth==0 && s_materialScopes[0]==nullptr);
    foreign.next=nullptr;
    draw_shadow(p); // a regular draw must not leave an active material scope

    // An unrelated material draw on the stack does not authorize this packet.
    before_material_draw(nullptr,batchArgs,nullptr,nullptr);
    draw_shadow(p);
    after_material_draw(nullptr,nullptr,nullptr,nullptr);
    // Skipped originals and bounded recursion unwind without stale pointers.
    player.packet.shapes=&p;
    void* ownArgs[]={&player.packet};
    for(int i=0;i<20;++i) before_material_draw(nullptr,ownArgs,nullptr,nullptr);
    draw_shadow(p);
    for(int i=0;i<20;++i) after_material_draw(nullptr,nullptr,nullptr,nullptr);
    assert(s_materialDepth==0);
    for(auto* material : s_materialScopes) assert(material==nullptr);

    // A malformed/future expanded effect must fail without submitting partial
    // commands or writing past the fixed command buffer.
    DarkDisplayList oversized;
    for(int i=0;i<20;++i) oversized.swap(0,0,1,2,3);
    writes=gxWrites; assert(!oversized.apply(4)); assert(gxWrites==writes);

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
    after_material_draw(nullptr,nullptr,nullptr,nullptr);
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
print('Fierce Deity shadow-pass isolation, GX registers, eye masks, alpha and cleanup: passed')
