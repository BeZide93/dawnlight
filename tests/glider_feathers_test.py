"""Exercise the production carrier particle cleanup on both dispatcher ABIs."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'src/glide_presentation.inc').read_text()
start = source.index('void suppress_glide_carrier_feathers(')
production = source[start:source.index('HookAction before_glide_carrier_draw', start)]
fixture = r'''
#include <cassert>
struct ModContext {};
using process_method_func=int(*)(void*);
int execute(void*){return 1;}int destroy(void*){return 1;}
struct process_method_class {process_method_func execute_method;};
process_method_class methods{execute},otherMethods{execute};
struct Emitter {
 float rate=1;int particles=5,clears=0;
 void setRate(float r){rate=r;}void deleteAllParticle(){particles=0;++clears;}
} ownEmitter,worldEmitter;
Emitter* dComIfGp_particle_getEmitter(unsigned id){return id==1?&ownEmitter:id==2?&worldEmitter:nullptr;}
struct ni_class {int id;unsigned mHaneEmitterID;process_method_class* sub_method;};
ni_class owned{42,1,&methods},world{99,2,&methods};
int fopAcM_GetID(ni_class* a){return a->id;}
constexpr int kNoGlideActor=-1;
struct {int cucco=42;bool retiring=false;} s_jumpAbilities;
enum class GlideItem {Cucco,Glider};GlideItem setting=GlideItem::Cucco;
GlideItem glide_item(){return setting;}
bool live=true;
ni_class* glide_actor(){return live?&owned:nullptr;}
struct Args {void* method;void* actor;};
namespace mods {template<class T>T arg(void* p,int i){auto* a=static_cast<Args*>(p);return reinterpret_cast<T>(i?a->actor:a->method);}}
// PRODUCTION
int main(){
#if defined(__APPLE__)
 Args args{reinterpret_cast<void*>(execute),&owned};
#else
 Args args{&methods,&owned};
#endif
 auto tick=[&](){after_glide_carrier_execute(nullptr,&args,nullptr,nullptr);};
 tick();assert(ownEmitter.particles==5&&ownEmitter.rate==1); // Cucco mode.
 setting=GlideItem::Glider;args.actor=&world;tick();
 assert(ownEmitter.particles==5&&worldEmitter.particles==5);
 args.actor=&owned;tick();assert(ownEmitter.particles==0&&ownEmitter.rate==0);
 assert(worldEmitter.particles==5&&worldEmitter.rate==1);
 // Every native update can reset the rate; cleanup occurs afterwards.
 ownEmitter.rate=.1f;ownEmitter.particles=3;tick();assert(!ownEmitter.particles&&!ownEmitter.rate);
 // No persistent emitter flags: Cucco mode can resume native emission.
 setting=GlideItem::Cucco;ownEmitter.rate=.1f;ownEmitter.particles=2;tick();
 assert(ownEmitter.particles==2&&ownEmitter.rate==.1f);
 // Pending deletion must remove particles even if the setting just changed.
 s_jumpAbilities.retiring=true;suppress_glide_carrier_feathers(&owned);
 assert(!ownEmitter.particles&&!ownEmitter.rate);
 suppress_glide_carrier_feathers(&world);assert(worldEmitter.particles==5);
 s_jumpAbilities.retiring=false;setting=GlideItem::Glider;
 ownEmitter.particles=3;live=false;tick();assert(ownEmitter.particles==3);live=true;
 auto* saved=owned.sub_method;owned.sub_method=nullptr;tick();assert(ownEmitter.particles==3);owned.sub_method=saved;
 void* savedMethod=args.method;
#if defined(__APPLE__)
 args.method=reinterpret_cast<void*>(destroy);
#else
 args.method=&otherMethods;
#endif
 tick();assert(ownEmitter.particles==3);args.method=savedMethod;
 owned.mHaneEmitterID=0;tick();assert(ownEmitter.particles==3);owned.mHaneEmitterID=1;
 s_jumpAbilities.cucco=kNoGlideActor;tick();assert(ownEmitter.particles==3);
 suppress_glide_carrier_feathers(nullptr);
}
'''.replace('// PRODUCTION', production)
with tempfile.TemporaryDirectory() as temp:
    cpp, exe = Path(temp) / 'test.cpp', Path(temp) / 'test'
    cpp.write_text(fixture)
    for flags in ([], ['-D__APPLE__']):
        subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', *flags,
                        str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
print('Glider feathers: scoped cleanup, native re-emission, mode switching, retirement and both host ABIs passed')
