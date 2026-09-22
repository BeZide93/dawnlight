"""Regression for the victory crash: Aurora's 'unhandled tcg src 21'.

Run the production fade-stage setup with recorded GX state, Aurora's real enum
values/TcgConfig and its pipeline texgen-copy loop. No game assets/GPU required.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
aurora = root / 'dusklight/extern/aurora'
source = (root / 'src/heroes_shade_wolf.inc').read_text()
start = source.index('void configure_shade_wolf_fade_stage(')
setup = source[start:source.index('\n}\n', start) + 3]
config_source = (aurora / 'lib/gx/gx.hpp').read_text()
start = config_source.index('struct TcgConfig {')
tcg = config_source[start:config_source.index('\n};', start) + 3]
gpu_source = (aurora / 'lib/gx/gx.cpp').read_text()
start = gpu_source.index('  for (u8 i = 0; i < g_gxState.numTexGens; ++i) {')
copy_texgens = gpu_source[start:gpu_source.index('\n  }', start) + 4]

fixture = r'''
#include <dolphin/gx/GXEnum.h>
#include <array>
#include <cassert>
#include <cstring>
// TCG
struct State {
    std::array<TcgConfig,8> tcgs;
    u8 numTexGens=0,numTevStages=0;
    GXTexCoordID sampled=GX_TEXCOORD_NULL;
    GXTexMapID map=GX_TEXMAP_NULL;
    int texgenWrites=0;
} g_gxState;
void GXSetNumTexGens(u8 n){assert(n<=8);g_gxState.numTexGens=n;}
void GXSetTexCoordGen2(GXTexCoordID id,GXTexGenType type,GXTexGenSrc src,
                      u32 mtx,GXBool normalize,u32 post){
    assert(id<8);++g_gxState.texgenWrites;
    auto& tcg=g_gxState.tcgs[id];tcg.type=type;tcg.src=src;
    tcg.mtx=static_cast<GXTexMtx>(mtx);tcg.postMtx=static_cast<GXPTTexMtx>(post);
    tcg.normalize=normalize;
}
void GXSetNumTevStages(u8 n){assert(n<=16);g_gxState.numTevStages=n;}
void GXSetTevOrder(GXTevStageID s,GXTexCoordID c,GXTexMapID m,GXChannelID ch){
    assert(s==g_gxState.numTevStages-1 && ch==GX_COLOR_NULL);
    g_gxState.sampled=c;g_gxState.map=m;
}
void GXSetTevDirect(GXTevStageID){}
void GXSetTevSwapMode(GXTevStageID,GXTevSwapSel r,GXTevSwapSel t){assert(r==GX_TEV_SWAP0 && t==GX_TEV_SWAP0);}
void GXSetTevColorIn(GXTevStageID,GXTevColorArg a,GXTevColorArg b,GXTevColorArg c,GXTevColorArg d){
    assert(a==GX_CC_ZERO && b==GX_CC_ZERO && c==GX_CC_ZERO && d==GX_CC_CPREV);
}
void GXSetTevColorOp(GXTevStageID,GXTevOp op,GXTevBias bias,GXTevScale scale,GXBool clamp,GXTevRegID out){
    assert(op==GX_TEV_ADD && bias==GX_TB_ZERO && scale==GX_CS_SCALE_1 && clamp && out==GX_TEVPREV);
}
void GXSetTevAlphaIn(GXTevStageID,GXTevAlphaArg a,GXTevAlphaArg b,GXTevAlphaArg c,GXTevAlphaArg d){
    // Existing fur alpha is multiplied by the fade mask, never replaced.
    assert(a==GX_CA_ZERO && b==GX_CA_APREV && c==GX_CA_TEXA && d==GX_CA_ZERO);
}
void GXSetTevAlphaOp(GXTevStageID s,GXTevOp op,GXTevBias bias,GXTevScale scale,GXBool clamp,GXTevRegID out){
    GXSetTevColorOp(s,op,bias,scale,clamp,out);
}
// SETUP
TcgConfig sampled_config(){
    struct {struct {std::array<TcgConfig,8> tcgs;} shaderConfig;} config;
    // COPY_TEXGENS
    return config.shaderConfig.tcgs[g_gxState.sampled];
}
int main(){
    static_assert(GX_MAX_TEXGENSRC==21);
    // Old fade stage sampled coord 0 even when the material declared none.
    // A valid stale global generator does not help: Aurora excludes it.
    g_gxState={};g_gxState.tcgs[0].src=GX_TG_TEX0;
    g_gxState.numTexGens=0;g_gxState.sampled=GX_TEXCOORD0;
    assert(sampled_config().src==GX_MAX_TEXGENSRC);
    // Native material display lists can switch repeatedly between textured
    // and untextured shapes. The setup must be valid after every such switch.
    for(int repeat=0;repeat<3;++repeat) for(u8 count:{0,2,0,1,8,0}) {
        g_gxState={};g_gxState.numTexGens=count;
        for(u8 i=0;i<count;++i){
            g_gxState.tcgs[i].src=static_cast<GXTexGenSrc>(GX_TG_TEX0+i);
            g_gxState.tcgs[i].mtx=GX_TEXMTX0;
            g_gxState.tcgs[i].postMtx=GX_PTTEXMTX0;
        }
        const auto before=g_gxState.tcgs;
        configure_shade_wolf_fade_stage(count,GX_TEVSTAGE3,GX_TEXMAP7);
        const auto sample=sampled_config();
        assert(sample.src!=GX_MAX_TEXGENSRC && g_gxState.numTevStages==4);
        assert(g_gxState.map==GX_TEXMAP7);
        if(count==0){
            assert(g_gxState.numTexGens==1 && g_gxState.texgenWrites==1);
            assert(sample.src==GX_TG_POS && sample.type==GX_TG_MTX2x4);
            assert(sample.mtx==GX_IDENTITY && sample.postMtx==GX_PTIDENTITY && !sample.normalize);
        }else{
            assert(g_gxState.numTexGens==count && g_gxState.texgenWrites==0);
            for(u8 i=0;i<count;++i) assert(g_gxState.tcgs[i]==before[i]);
        }
    }
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'wolf_fade.cpp'
    exe=Path(tmp)/'wolf_fade'
    cpp.write_text(fixture.replace('// TCG',tcg).replace('// SETUP',setup)
                   .replace('// COPY_TEXGENS',copy_texgens))
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-DTARGET_PC=1',
                    '-I',str(aurora/'include'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print("Wolf fade: reproduced tcg source 21; zero-texgen fix and native UV preservation passed")
