"""Exercise the actual render callbacks with a recording GX/J3D environment."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/fierce_deity_visual.cpp').read_text()
callbacks = source[source.index('struct DrawScope'):source.index('void after_player_draw')]
callbacks = callbacks.replace('#include "fierce_deity_transition.inc"', '')
dual_source = (root / 'src/dual_wield.cpp').read_text()
dual_start = dual_source.index('bool dual_wield_owns_model(')
dual_owner = dual_source[dual_start:dual_source.index('\n}', dual_start) + 2]
fixture = r'''
#include <array>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <cmath>
using Mtx = float[3][4];
using u8 = uint8_t; using u16 = uint16_t; using u32 = uint32_t;
using GXTexCoordID = int; using GXTexMapID = int; using GXTevSwapSel = int;
using GXTevStageID = int; using GXTevColorArg = int; using GXTevOp = int; using GXTevScale = int;
constexpr int GX_CH_RED=0, GX_CH_GREEN=1, GX_CH_BLUE=2, GX_CH_ALPHA=3;
constexpr int GX_COLOR1=1, GX_COLOR1A1=5, GX_TRUE=1, GX_FALSE=0;
constexpr int GX_SRC_REG=0, GX_LIGHT_NULL=0, GX_LIGHT0=1, GX_DF_CLAMP=2, GX_AF_NONE=2;
constexpr int GX_TEV_ADD=0, GX_TEV_SUB=1, GX_TB_ZERO=0, GX_CS_SCALE_1=0,
              GX_CS_SCALE_2=1, GX_TEVPREV=0, GX_TEXCOORD_NULL=255, GX_TEXMAP_NULL=255;
constexpr int GX_CC_ZERO=15, GX_CC_TEXC=8, GX_CC_CPREV=0, GX_CC_RASC=10,
              GX_CC_C0=2, GX_CC_C1=4, GX_TEVREG0=1, GX_TEVREG1=2;
constexpr int GX_CA_ZERO=7, GX_CA_APREV=0, GX_TEXMAP3=3;
struct GXColor { u8 r, g, b, a; };
struct RecordedStage { std::array<int,4> color{}, alpha{}; int op=0, scale=0, swap=0, dest=0; };
std::array<RecordedStage,16> gpuStages{};
std::array<std::array<int,4>,4> gpuSwaps{};
int gpuCount=3, gxWrites=0, restoreCalls=0;
GXColor gpuAmbient{}, gpuMaterial{};
// Decode real big-endian GX display-list bytes, including one-write BP masks.
// Keeping live registers separate from CPU material objects is essential: J3D
// display lists (including animations) do not refresh the GX setter shadows.
std::array<u32,256> bp{}, nativeBP{};
std::array<u32,0x1080> xf{}, nativeXF{};
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
        gpuStages[i].dest=(c>>22)&3;
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
            const unsigned count=(header>>16)+1;
            assert(reg+count<=xf.size());
            if(reg>=0x500 && reg<0x5F0) {
                // Aurora copy_xf_data supports complete post matrices only.
                // In release builds its CHECK is compiled out and partial
                // writes overwrite element zero, ignoring the word offset.
                assert((reg-0x500)%12==0 && count==12 &&
                       "Aurora does not support partial post-matrix uploads");
            }
            for(unsigned i=0;i<count;++i) { xf[reg+i]=word(); ++gxWrites; }
            break;
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
        if(!(i>=0x603 && i<=0x60F) && !(i>=0x1009 && i<=0x100F)) assert(xf[i]==nativeXF[i]);
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
struct ResTIMG { u16 width=32, height=64; };
struct Texture { ResTIMG image; int getNum() const {return 1;}
    ResTIMG* getResTIMG(int) {return &image;} };
struct J3DModelData {
    Names materials{"al_body"}, textures{"al_eyeball"}; Texture texture;
    Names* getMaterialName() {return &materials;} Names* getTextureName() {return &textures;}
    Texture* getTexture() {return &texture;}
};
struct ColorChan {
    bool enabled = false;
    u8 getEnable() const { return enabled; }
    u8 getLightMask() const { return 1; }
};
struct ColorBlock {
    ColorChan alpha;
    ColorChan* getColorChan(int) { return &alpha; }
    bool hasLight = true;
    void* getLight(int) { return hasLight ? this : nullptr; }
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
struct J3DMatPacket { J3DMaterial* material=nullptr; J3DMaterial* getMaterial() {return material;} J3DShapePacket* shapes=nullptr; J3DShapePacket* getShapePacket() {return shapes;} DisplayList list; DisplayList* getDisplayListObj() {return &list;} void callDL() {list.callDL();} };
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
    *mpDemoHLTmpModel=nullptr,*mpDemoHRTmpModel=nullptr,*mpLinkBootModels[2]{},
    *mSwordModel=nullptr,*mSheathModel=nullptr,*mShieldModel=nullptr;
};
daAlink_c link;
struct { const daAlink_c* owner=nullptr; J3DModel* sword=nullptr; J3DModel* sheath=nullptr; } s;
bool darkActive=true;
daAlink_c* daAlink_getAlinkActorClass() {return &link;}
bool fierce_deity_dark_visual_active() {return darkActive;}
struct WarpLayer { bool active=false, dark=false, inverse=false; };
bool warpTest = false, warpDark = false, warpInverse = false;
struct { unsigned maskedShapes=0, projectedGroups=0; } s_transition;
WarpLayer warp_layer(J3DModel* model) {
    return model == link.mpLinkModel ? WarpLayer{warpTest,warpDark,warpInverse} : WarpLayer{};
}
int native_warp_coord(J3DModel*, J3DMaterial*, daAlink_c*) {return warpTest ? 3 : -1;}
void warp_texture_matrix(Mtx matrix) {
    for(int i=0;i<3;++i) for(int j=0;j<4;++j) matrix[i][j] = float(i*4+j);
}
'''
checks = r'''
void draw_begin(J3DShapePacket& p) {
    p.model->packet.shapes=&p;
    p.model->packet.material=p.shape->material;
    void* materialArgs[]={&p.model->packet};
    before_material_draw(nullptr,materialArgs,nullptr,nullptr);
    load_native(p.shape->material->tev.count);
    void* args[]={&p}; before_packet_draw(nullptr,args,nullptr,nullptr);
    void* shape[]={p.shape}; before_shape_draw(nullptr,shape,nullptr,nullptr);
    if(!warpTest && s_drawDepth<=s_drawScopes.size() && s_drawScopes[s_drawDepth-1].applied)
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
    std::array<float,4> regs{};
    for(int i=nativeCount;i<nativeCount+5;++i) {
        const auto& s=gpuStages[i];
        const float tex=gpuSwaps[s.swap][0]==GX_CH_RED ? r : b;
        auto value=[&](int v) {switch(v) {
            case GX_CC_ZERO:return 0.0f; case GX_CC_TEXC:return tex;
            case GX_CC_CPREV:return regs[0]; case GX_CC_RASC:return 1.0f;
            case GX_CC_C0:return regs[1]; case GX_CC_C1:return regs[2];
        } assert(false); return 0.0f;};
        const auto& c=s.color;
        const float mix=value(c[0])*(1-value(c[2]))+value(c[1])*value(c[2]);
        regs[s.dest]=std::clamp((value(c[3])+(s.op==GX_TEV_SUB ? -mix:mix))*(s.scale?2:1),0.0f,1.0f);
        assert((s.alpha==std::array<int,4>{GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,GX_CA_APREV}));
    }
    return regs[0];
}
float xf_float_value(unsigned reg) {
    float result; u32 bits=xf[reg]; std::memcpy(&result,&bits,sizeof(result)); return result;
}
// Evaluate the same specular attenuation as Aurora from the emitted registers.
float highlight(float nx, float ny, float nz) {
    const float facing=nx*xf_float_value(0x60A)+ny*xf_float_value(0x60B)+nz*xf_float_value(0x60C);
    const float t=facing>=0 ? std::max(0.0f,nx*xf_float_value(0x60D)+ny*xf_float_value(0x60E)+nz*xf_float_value(0x60F)) : 0;
    const float a=xf_float_value(0x604)+xf_float_value(0x605)*t+xf_float_value(0x606)*t*t;
    const float k=xf_float_value(0x607)+xf_float_value(0x608)*t+xf_float_value(0x609)*t*t;
    assert(k>0 && std::isfinite(a/k));
    return std::max(0.0f,a/k);
}
// Evaluate the emitted alpha pipeline, including GX A8_EQ and register writes.
int warp_alpha(unsigned start, int nativeAlpha, int texAlpha) {
    int regs[4] = {nativeAlpha, 0, 0, 0};
    auto input = [&](unsigned arg) {
        if(arg <= 3) return regs[arg];
        if(arg == 4) return texAlpha;
        if(arg == 6) return 255;
        assert(arg == 7); return 0;
    };
    for(unsigned i=start;i<unsigned(gpuCount);++i) {
        const u32 word=bp[0xC1+2*i];
        const int a=input((word>>13)&7), b=input((word>>10)&7);
        const int c=input((word>>7)&7), d=input((word>>4)&7);
        int out;
        if(((word>>16)&3)==3) {
            assert(((word>>20)&3)==3 && ((word>>18)&1)==1);
            const unsigned shift=(i&1)?19:9;
            assert(((bp[0xF6+i/2]>>shift)&31)==0);
            out=d+(a==b?c:0);
        } else {
            const int value=(a*(255-c)+b*c)/255;
            out=d+(((word>>18)&1)?-value:value);
        }
        regs[(word>>22)&3]=std::clamp(out,0,255);
    }
    return regs[0];
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
    assert(gpuCount==5 && gpuMaterial.r==100);
    assert(gpuMaterial.a==93 && gpuAmbient.a==61); // Aurora's full RGBA writes
    // Check actual SPEC bits, independent channels, additive composition,
    // and normal-dependent highlights rather than a constant gray silhouette.
    assert(xf[0x100F]==((1u<<9)|(1u<<1)|(1u<<2)));
    assert(((xf[0x100E]>>7)&3)==GX_DF_CLAMP);
    assert(((bp[0x29]>>19)&7)==0); // stage3: COLOR0 diffuse
    assert(((bp[0x2A]>>7)&7)==1); // stage4: COLOR1 specular
    assert((gpuStages[4].color==std::array<int,4>{GX_CC_RASC,GX_CC_ZERO,GX_CC_ZERO,GX_CC_CPREV}));
    const float hx=xf_float_value(0x60D), hy=xf_float_value(0x60E), hz=xf_float_value(0x60F);
    assert(highlight(hx,hy,hz)>0.98f);
    assert(highlight(0,0,1)>0.1f && highlight(0,0,1)<0.5f);
    assert(highlight(1,0,0)==0);
    assert((xf[0x100A]&255)==61 && (xf[0x100C]&255)==93);
    assert(xf[0x1010]==nativeXF[0x1010] && xf[0x1011]==nativeXF[0x1011]);
    draw_end();
    assert(bp==nativeBP && xf==nativeXF); // includes light registers
    assert(gpuCount==3 && restoreCalls==2); // base + per-instance animation DL
    int writes=gxWrites;
    draw_begin(foreign); draw_end(); // same material pointer, different actor
    assert(gxWrites==writes && restoreCalls==2);
    darkActive=false;
    draw_begin(p); draw_end();
    assert(gxWrites==writes);
    darkActive=true;

    // A material without a light in its own DL must not overwrite global lights.
    mat.color.hasLight=false;
    draw_begin(p); assert(gpuCount==4 && gpuMaterial.r==32);
    for(unsigned i=0x603;i<=0x60F;++i) assert(xf[i]==nativeXF[i]);
    draw_end(); mat.color.hasLight=true;
    mat.color.alpha.enabled=true;
    draw_begin(p); assert(gpuCount==4);
    for(unsigned i=0x603;i<=0x60F;++i) assert(xf[i]==nativeXF[i]);
    draw_end(); mat.color.alpha.enabled=false;

    // Alpha-tested geometry preserves alpha at every appended stage. Eye masks
    // must reject white sclera and retain blue AND red irises at moving UVs.
    for(const char* name : {"al_eyeballL_m","bl_eyeballR_m","ml_eyeballL_m","zl_eyeballR_m"}) {
        player.data.materials.name=name;
        draw_begin(p);
        assert(gpuCount==8 && gpuMaterial.r==230 && gpuMaterial.g==8);
        for(unsigned i=0x603;i<=0x60F;++i) assert(xf[i]==nativeXF[i]);
        assert(eye_mask(3,1,1)==0); // sclera
        assert(eye_mask(3,1,0)==1); // iris
        assert(eye_mask(3,0,1)==1); // native blue iris (old R-B mask gave zero)
        assert(eye_mask(3,0,0)==0); // pupil
        assert(eye_mask(3,0.6f,0.4f)>0.39f && eye_mask(3,0.6f,0.4f)<0.41f);
        // Full 8-bit domain, both directions, including neutral antialiasing.
        for(int r=0;r<256;++r) for(int b=0;b<256;++b)
            assert(std::abs(eye_mask(3,r/255.f,b/255.f)-
                std::min(1.f,2.f*std::abs(r-b)/255.f))<0.00001f);
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
            assert(gpuCount==count+(count<=11 && std::strstr(name,"eyeball") ? 5 : count<=14 ? 2 : 1));
            draw_end();
            assert(bp==nativeBP && xf==nativeXF);
        }
    }
    // Batched materials can be owned by a different model. Only the player
    // receives the effect, and cleanup must replay the batch's actual base DL.
    mat.tev.count=3;
    player.data.materials.name="al_body";
    other.packet.material=&mat;
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
    assert(gpuCount==5); assert_native_fields(3);
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

    // Warp-only and dark+warp passes use the same native generator slot. Check
    // complements for EVERY mask byte: no gap at the threshold, no double draw,
    // and native cutout/translucent alpha remains unchanged on its chosen side.
    warpTest=true;
    for(bool dark : {false,true}) {
        warpDark=dark;
        std::array<int,256> normal{}, inverse{};
        for(int alpha : {0,93,255}) {
            for(bool invert : {false,true}) {
                warpInverse=invert;
                draw_begin(p);
                assert((bp[0]&15)==4 && xf[0x103F]==4 && xf[0x1043]==6);
                assert(xf[0x1053]==9 && shader_texgens_valid());
                assert((bp[0x36]&0x3ffff)==31 && (bp[0x37]&0x3ffff)==63);
                assert((bp[0x43]&(1u<<6))==0); // no hidden-half depth writes
                before_primitive_draw(nullptr,nullptr,nullptr,nullptr);
                for(unsigned element=0;element<12;++element)
                    assert(xf_float_value(0x500+36+element)==float(element));
                for(int mask=0;mask<256;++mask)
                    (invert?inverse:normal)[mask]=warp_alpha(3,alpha,mask);
                draw_end();
                assert(bp==nativeBP && xf==nativeXF);
            }
            for(int mask=0;mask<256;++mask) {
                assert(normal[mask]+inverse[mask]==alpha);
                assert(normal[mask]==0 || inverse[mask]==0);
            }
        }
    }
    draw_shadow(p); // a transition still never enters the shadow-image pass
    warpTest=false;

    // Eye RGB register writes must not interfere with warp alpha registers.
    player.data.materials.name="al_eyeballL_m";
    warpTest=warpDark=true;
    for(bool invert : {false,true}) {
        warpInverse=invert;
        draw_begin(p);
        assert(gpuCount==8+(invert ? 3 : 1));
        assert(eye_mask(3,0,1)==1 && eye_mask(3,1,0)==1);
        for(int alpha : {0,93,255}) for(int mask=0;mask<256;++mask)
            assert(warp_alpha(3,alpha,mask)==((invert ? mask!=255 : mask==255) ? alpha : 0));
        draw_end();assert(bp==nativeBP && xf==nativeXF);
    }
    warpTest=false;
    player.data.materials.name="al_body";

    // Every held/stowed equipment instance gets the same body shading, even
    // when its material is shared with an unrelated actor. No retained warp
    // layer exists for equipment, and shadow draws must remain native.
    J3DModel sword, sheath, shield, second, secondSheath;
    link.mSwordModel=&sword;link.mSheathModel=&sheath;link.mShieldModel=&shield;
    s.owner=&link;s.sword=&second;s.sheath=&secondSheath;
    for(auto* model : {&sword,&sheath,&shield,&second,&secondSheath}) {
        J3DShapePacket gear{model,&shape,{}};
        assert(!player_model(&link,model) && player_equipment(&link,model));
        draw_begin(gear);assert(gpuCount==5 && gpuMaterial.r==100);
        draw_end();assert(bp==nativeBP && xf==nativeXF);
        draw_shadow(gear);
        darkActive=false;
        writes=gxWrites;draw_begin(gear);draw_end();assert(gxWrites==writes);
        darkActive=true;
    }
    assert(!player_equipment(nullptr,&sword) && !player_equipment(&link,nullptr));
    assert(!player_equipment(&link,&other));
    daAlink_c otherLink;s.owner=&otherLink;
    assert(!player_equipment(&link,&second));
    s.owner=&link;s.sword=nullptr;s.sheath=nullptr; // released Dual Wield models
    assert(!player_equipment(&link,&second) && !player_equipment(&link,&secondSheath));
    link.mSwordModel=nullptr;link.mSheathModel=nullptr;link.mShieldModel=nullptr;
    assert(!player_equipment(&link,&sword) && !player_equipment(&link,&shield));

    // Different native materials use different dormant generator slots. Every
    // coefficient, including the changing vertical translation, must reach the
    // selected post matrix as a complete block without touching its neighbors.
    for(unsigned coord=0;coord<4;++coord) {
        for(unsigned frame=0;frame<50;++frame) {
            Mtx matrix;
            for(unsigned row=0;row<3;++row) for(unsigned col=0;col<4;++col)
                matrix[row][col]=float(row*4+col)+float(frame)*0.12f;
            const auto previous=xf;
            DarkDisplayList upload;
            upload.post_matrix(coord,matrix);
            assert(upload.submit());
            const unsigned base=0x500+coord*12;
            for(unsigned reg=0;reg<xf.size();++reg) {
                if(reg>=base && reg<base+12) {
                    const unsigned element=reg-base;
                    assert(xf_float_value(reg)==matrix[element/4][element%4]);
                } else assert(xf[reg]==previous[reg]);
            }
        }
    }

    // A malformed/future expanded effect must fail without submitting partial
    // commands or writing past the fixed command buffer.
    DarkDisplayList oversized;
    for(int i=0;i<80;++i) oversized.swap(0,0,1,2,3);
    writes=gxWrites; assert(!oversized.apply(4)); assert(gxWrites==writes);

    // Capacity limits and foreign layouts never overflow the 16-stage pipeline.
    mat.tev.count=16;
    writes=gxWrites; draw_begin(p); draw_end(); assert(gxWrites==writes);
    mat.tev.count=14;
    draw_begin(p); assert(gpuCount==16); draw_end(); // two body stages fit
    mat.tev.count=3;
    player.data.textures.name="custom_without_native_mask";
    draw_begin(p); assert(gpuCount==5 && gpuMaterial.r==100); draw_end();
    player.data.textures.name="al_eyeball";
    mat.tev.stages[0].mTevSwapModeInfo=0;
    mat.tev.stages[1].mTevSwapModeInfo=5;
    mat.tev.stages[2].mTevSwapModeInfo=14; // all swap tables in use
    draw_begin(p); assert(gpuCount==5); draw_end();
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
    cpp.write_text(fixture + dual_owner + callbacks + checks)
    subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Fierce Deity warp masks, late depth, specular response, shadow isolation, eyes and alpha: passed')
