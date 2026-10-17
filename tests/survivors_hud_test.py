"""Validate Survivors vertex packets against native J2D font/rectangle setup."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1] if len(sys.argv) > 1 else root / 'dusklight')
source = (root / 'src/survivors.cpp').read_text()
graf = (sdk / 'libs/JSystem/src/J2DGraph/J2DGrafContext.cpp').read_text()
font = (sdk / 'libs/JSystem/src/JUtility/JUTResFont.cpp').read_text()


def function(text, signature):
    start = text.index(signature)
    brace = text.index('{', start)
    depth = 0
    for end in range(brace, len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError(signature)


native = '\n'.join([
    function(graf, 'void J2DGrafContext::setup2D()'),
    function(graf, 'void J2DGrafContext::fillBox('),
    function(font, 'void JUTResFont::setGX()'),
])
# Only vertex descriptors and byte emission are emulated. Execute the actual
# native setup/fill functions, so the test does not invent a safe fillBox API.
constants = sorted(set(re.findall(r'\bGX_\w+', native)) - {'GX_MAX_TEVSTAGE'})
tracked = {'GXClearVtxDesc', 'GXSetVtxDesc', 'GXBegin', 'GXEnd',
           'GXPosition3f32', 'GXColor1u32', 'GXSetNumTexGens'}
noops = sorted(set(re.findall(r'\b(GX\w+)\(', native)) - tracked)
prefix = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <iostream>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using f32=float; using GXTevStageID=int; using GXBlendMode=int; using GXBlendFactor=int;
using Mtx=float[3][4];
void MTXIdentity(Mtx){}
#define ZoneScoped
'''
prefix += 'enum { GX_MAX_TEVSTAGE=16, ' + ', '.join(constants) + ' };\n'
prefix += '\n'.join('template<class... T> void '+name+'(T...){}' for name in noops)
stubs = r'''
unsigned packetBytes=0, expectedBytes=0, boxes=0, labels=0, texGens=0;
bool textureCoordinates=false;
void GXClearVtxDesc(){textureCoordinates=false;}
void GXSetVtxDesc(int attr,int type){if(attr==GX_VA_TEX0)textureCoordinates=type==GX_DIRECT;}
void GXSetNumTexGens(unsigned n){texGens=n;}
void GXBegin(int,int,unsigned vertices){
    packetBytes=0;
    // fillBox uses F32 XYZ + RGBA8. Font TEX0 is two direct U16 coordinates.
    expectedBytes=vertices*(12+4+(textureCoordinates?4:0));
}
void GXPosition3f32(float,float,float){packetBytes+=12;}
void GXColor1u32(unsigned){packetBytes+=4;}
void GXEnd(){assert(packetBytes==expectedBytes && "GX vertex packet length mismatch");++boxes;}
namespace JGeometry {
template<class T> struct TBox2 {
    struct Point {T x,y;} i,f;
    TBox2(T x,T y,T right,T bottom):i{x,y},f{right,bottom}{}
};
}
namespace JUtility {
struct TColor {
    unsigned value;
    TColor(unsigned v=0):value(v){}
    TColor(unsigned r,unsigned g,unsigned b,unsigned a):value((r<<24)|(g<<16)|(b<<8)|a){}
    operator unsigned() const {return value;}
};
}
struct J2DGrafContext {
    Mtx mPosMtx{}; int mLineWidth=6;
    struct {int mType=0,mSrcFactor=0,mDstFactor=0;} mBoxPart;
    JUtility::TColor mColorTL{1},mColorTR{2},mColorBR{3},mColorBL{4};
    void setup2D(); void fillBox(const JGeometry::TBox2<f32>&);
    void setColor(JUtility::TColor c){setColor(c,c,c,c);}
    void setColor(JUtility::TColor a,JUtility::TColor b,JUtility::TColor c,JUtility::TColor d){
        mColorTL=a;mColorTR=b;mColorBR=c;mColorBL=d;
    }
} context;
struct JUTResFont {void setGX();} font;
bool hasFont=true, arena=true, transition=false, window=false, hasGraf=true;
JUTResFont* mDoExt_getMesgFont(){return hasFont?&font:nullptr;}
enum {HBIND_CENTER,VBIND_TOP};
struct J2DPrint {
    J2DPrint(JUTResFont* f,JUtility::TColor,JUtility::TColor){f->setGX();}
    void setFontSize(float,float){}
    void printReturn(const char*,float,float,int,int,float,float,int){++labels;}
};
struct ModContext {};
int potion_slot(){return -1;}
Run s_run; unsigned s_spawnFailures=0;
bool in_arena(){return arena;} bool transitioning(){return transition;} bool ui_visible(){return window;}
J2DGrafContext* dComIfGp_getCurrentGrafPort(){return hasGraf?&context:nullptr;}
struct mDoGph_gInf_c {static float getMinXF(){return 0;} static float getWidthF(){return 608;}};
'''
main = r'''
int main(){
    // First visible HUD after the entrance, ordinary runs, and both messages.
    for(auto phase:{Phase::Ready,Phase::Running,Phase::Upgrade,Phase::Treasure,Phase::Victory,Phase::Defeat}){
        for(bool availableFont:{false,true})for(unsigned ticks:{0u,duration})for(unsigned failures:{0u,10u}){
            s_run.phase=phase;s_run.ticks=ticks;s_run.xp=4;s_spawnFailures=failures;hasFont=availableFont;
            boxes=labels=0;textureCoordinates=true;
            draw_hud(nullptr,nullptr,nullptr,nullptr);
            assert(boxes==2 && !textureCoordinates && texGens==0);
            assert(context.mColorTL==1 && context.mColorTR==2 && context.mColorBR==3 && context.mColorBL==4);
            assert((labels!=0)==availableFont);
        }
    }
    for(int reason=0;reason<5;++reason){
        arena=reason!=0;transition=reason==1;window=reason==2;hasGraf=reason!=3;
        s_run.phase=reason==4?Phase::Loading:Phase::Running;boxes=labels=0;
        draw_hud(nullptr,nullptr,nullptr,nullptr);assert(boxes==0 && labels==0);
    }
    std::cout<<"Survivors HUD: native font/box vertex packets, messages and render gates passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    binary = Path(tmp) / 'test'
    cpp.write_text(prefix + stubs + native + '\n' + function(source, 'void label(') + '\n' +
                   function(source, 'void draw_hud(') + '\n' + main)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root/'src'),
                    str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
