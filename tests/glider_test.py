"""Original asset integrity plus production carrier draw/audio isolation.
Run: python3 tests/glider_test.py (Pillow, C++ compiler).
"""
from pathlib import Path
import importlib.util
import math
import subprocess
import tempfile
import sys
sys.dont_write_bytecode = True

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('glider_generator',ROOT/'tools/generate_glider.py')
g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
v,f=g.mesh()
assert len(v)<65536 and len(f)*3<65536
for vertex in v:
    assert all(math.isfinite(n) for n in vertex)
    assert 0<=vertex[3]<=1 and 0<=vertex[4]<=1
for triangle in f:
    a,b,c=[v[i][:3] for i in triangle]
    u=[b[k]-a[k] for k in range(3)];w=[c[k]-a[k] for k in range(3)]
    cross=[u[1]*w[2]-u[2]*w[1],u[2]*w[0]-u[0]*w[2],u[0]*w[1]-u[1]*w[0]]
    assert sum(x*x for x in cross)>1e-10
pixels=g.encode_texture(g.texture())
assert len(pixels)==sum((g.SIZE>>i)**2*2 for i in range(g.LAST_MIP+1))
# Decode GX level 0 to ensure tiling/channel byte order matches the authored atlas.
im=g.texture();index=0
for by in range(0,g.SIZE,4):
    for bx in range(0,g.SIZE,4):
        for y in range(by,by+4):
            for x in range(bx,bx+4):
                value=int.from_bytes(pixels[index:index+2],'big');index+=2
                r,green,b=im.getpixel((x,y))
                assert (value>>11,(value>>5)&63,value&31)==(r>>3,green>>2,b>>3)
paths=list((ROOT/'art/glider').glob('dawnlight-glider.*'))+[ROOT/'src/generated/glider_art.hpp']
before={p:p.read_bytes() for p in paths};g.generate()
assert all(p.read_bytes()==data for p,data in before.items()),'Generated assets out of date'

fixture=r'''
#include <cassert>
struct ModContext{};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
struct leafdraw_class {int id;};
struct Z2SoundObjSimple{};struct Z2SoundHandlePool{};
struct ni_class:leafdraw_class {Z2SoundObjSimple mSound;};
namespace mods {template<class T>T arg(void* a,int){return static_cast<T>(a);}}
int fpcM_GetID(leafdraw_class* a){return a->id;}
constexpr int kNoGlideActor=-1;
struct {int cucco=-1;bool retiring=false;} s_jumpAbilities;
enum class GlideItem {Cucco,Glider};GlideItem selection=GlideItem::Cucco;
GlideItem glide_item(){return selection;}
ni_class owned{{42},{}},world{{99},{}};bool live=true;
ni_class* glide_actor(){return live?&owned:nullptr;}
// PRODUCTION
int main(){
 int result=0;
 Z2SoundHandlePool sentinel;Z2SoundHandlePool* sound=&sentinel;
 s_jumpAbilities.cucco=42;
 // Default Cucco keeps its model and sound, as do all unrelated actors.
 assert(before_glide_carrier_draw(nullptr,&owned,&result,nullptr)==HOOK_CONTINUE);
 assert(before_glide_carrier_sound(nullptr,&owned.mSound,&sound,nullptr)==HOOK_CONTINUE);
 selection=GlideItem::Glider;
 assert(before_glide_carrier_draw(nullptr,&owned,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==1);
 assert(before_glide_carrier_draw(nullptr,&world,&result,nullptr)==HOOK_CONTINUE);
 assert(before_glide_carrier_sound(nullptr,&owned.mSound,&sound,nullptr)==HOOK_SKIP_ORIGINAL&&!sound);
 sound=&sentinel;
 assert(before_glide_carrier_sound(nullptr,&world.mSound,&sound,nullptr)==HOOK_CONTINUE&&sound==&sentinel);
 // Pending loads and expired actors are not dereferenced by audio filtering.
 live=false;
 assert(before_glide_carrier_sound(nullptr,&owned.mSound,&sound,nullptr)==HOOK_CONTINUE);
 // Switching the setting back restores Cucco presentation without replacing it.
 live=true;selection=GlideItem::Cucco;
 assert(before_glide_carrier_draw(nullptr,&owned,&result,nullptr)==HOOK_CONTINUE);
 // Deletion retries keep the owned carrier hidden in either mode.
 s_jumpAbilities.retiring=true;
 assert(before_glide_carrier_draw(nullptr,&owned,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 s_jumpAbilities.cucco=-1;
 assert(before_glide_carrier_draw(nullptr,&owned,&result,nullptr)==HOOK_CONTINUE);
}
'''
fixture=fixture.replace('// PRODUCTION',(ROOT/'src/glide_presentation.inc').read_text())
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test';cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Glider regression passed: mesh, UVs, GX texture, reproducibility, scoped model/audio replacement')
