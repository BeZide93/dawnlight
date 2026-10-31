"""Execute the monitor against a fake loader and queued render-worker callbacks."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'src/survivors_performance.cpp').read_text()
source='\n'.join(x for x in source.splitlines() if not x.startswith('#include "'))
fixture=r'''
#include "survivors_performance.hpp"
#include <cassert>
#include <cstring>
#include <map>
#include <string>
#include <vector>
using ConfigVarHandle=unsigned;struct ModContext{};ModContext* mod_ctx=nullptr;
struct ConfigVarDesc{const char* name=nullptr;int type=0;int64_t default_int=0;};
#define CONFIG_VAR_DESC_INIT {}
constexpr int CONFIG_VAR_INT=0;
std::map<unsigned,int64_t> settings;std::map<std::string,unsigned> names;
ModResult register_owned_setting(const ConfigVarDesc* d,unsigned* out){auto [i,added]=names.emplace(d->name,unsigned(names.size()+1));*out=i->second;if(added)settings[*out]=d->default_int;return MOD_OK;}
struct Config {ModResult get_int(ModContext*,unsigned h,int64_t* out){*out=settings[h];return MOD_OK;}ModResult set_int(ModContext*,unsigned h,int64_t v){settings[h]=v;return MOD_OK;}} config;
auto* svc_config=&config;
using GfxComputeTypeHandle=uint64_t;using GfxStageHookHandle=uint64_t;
struct GfxComputeContext{uint64_t vertex_buffer,index_buffer,uniform_buffer,storage_buffer;};
struct GfxStageContext{};
uint64_t wgpuBufferGetSize(uint64_t n){return n;}
using ComputeFn=void(*)(ModContext*,const GfxComputeContext*,const void*,size_t,void*);
using StageFn=void(*)(ModContext*,const GfxStageContext*,void*);
struct GfxComputeTypeDesc{size_t size;const char* name;ComputeFn callback;void* user;};
struct GfxStageHookDesc{size_t size;StageFn callback;void* user;};
constexpr int GFX_STAGE_FRAME_AFTER_HUD=0;
bool onWorker=false;ComputeFn worker=nullptr;StageFn stageFn=nullptr;
std::vector<uint64_t> jobs;
struct Gfx {
 ModResult register_compute_type(ModContext*,const GfxComputeTypeDesc* d,uint64_t* out){worker=d->callback;*out=1;return MOD_OK;}
 ModResult register_stage_hook(ModContext*,int,const GfxStageHookDesc* d,uint64_t* out){stageFn=d->callback;*out=2;return MOD_OK;}
 ModResult unregister_compute_type(ModContext*,uint64_t){return MOD_OK;}
 ModResult unregister_stage_hook(ModContext*,uint64_t){return MOD_OK;}
 ModResult push_compute(ModContext*,uint64_t,const void* p,size_t n){assert(!onWorker&&n==8);uint64_t e;std::memcpy(&e,p,n);jobs.push_back(e);return MOD_OK;}
}gfx;auto* svc_gfx=&gfx;
struct Host{const char* version="2.0.3";} host;auto* svc_host=&host;
enum HookSymbolFlags{HOOK_SYMBOL_CODE=1,HOOK_SYMBOL_MULTI_NAME=8};
uint32_t rawStats[9]{};bool symbolAvailable=true;
const void* getter(){assert(onWorker);return rawStats;}
struct Hook {ModResult resolve(ModContext*,const char* name,void** out,HookSymbolFlags* flags){assert(!onWorker&&!std::strcmp(name,"aurora_get_stats"));*out=reinterpret_cast<void*>(&getter);*flags=HOOK_SYMBOL_CODE;return symbolAvailable?MOD_OK:MOD_UNAVAILABLE;}} hook;
auto* svc_hook=&hook;
struct Log{void info(ModContext*,const char*){assert(!onWorker);}} logService;auto* svc_log=&logService;
'''
main=r'''
void render(uint64_t job){GfxComputeContext c{5*1024*1024,2*1024*1024,24*1024*1024,8*1024*1024};onWorker=true;worker(nullptr,&c,&job,sizeof(job),nullptr);onWorker=false;}
int main(){
 using namespace dawnlight::survivor_perf;
 assert(initialize()==MOD_OK&&!enabled());tick(true);stageFn(nullptr,nullptr,nullptr);assert(jobs.empty());
 set_enabled(true);set_monitor(true);tick(true);stageFn(nullptr,nullptr,nullptr);assert(jobs.size()==1);
 rawStats[4]=4800000;render(jobs.back());tick(true);
 assert(sample().vertices==4800000&&peak()==4800000&&tier()==2);
 assert(sample().vertexCapacity==5*1024*1024);
 const auto old=jobs.back();reset_run();render(old);tick(true);assert(!sample().sequence&&!peak());
 // Pressure also responds to other arenas, using the actual GPU capacities.
 rawStats[4]=100;rawStats[7]=8*1024*1024;stageFn(nullptr,nullptr,nullptr);render(jobs.back());tick(true);assert(tier()==2);
 set_enabled(false);tick(true);const auto count=jobs.size();stageFn(nullptr,nullptr,nullptr);assert(jobs.size()==count&&!enabled());
 shutdown();render(old); // Queued work becomes harmless before unregistering.
 host.version="2.1.0";assert(initialize()==MOD_OK);set_enabled(true);tick(true);
 assert(!sample().sequence);shutdown();
 host.version="2.0.3";symbolAvailable=false;assert(initialize()==MOD_OK);tick(true);assert(enabled()&&!sample().sequence);shutdown();
}
'''
with tempfile.TemporaryDirectory() as tmp:
    tmp=Path(tmp);(tmp/'mods').mkdir();(tmp/'mods/api.h').write_text('#pragma once\nenum ModResult{MOD_OK,MOD_ERROR,MOD_UNAVAILABLE};\n')
    cpp=tmp/'test.cpp';exe=tmp/'test';cpp.write_text(fixture+source+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-pthread','-I',str(tmp),'-I',str(ROOT/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Performance monitor: optional symbols, worker samples, capacities, run epochs and shutdown passed')
