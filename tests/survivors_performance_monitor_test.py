"""Execute the monitor against a fake loader and queued render-worker callbacks."""
from pathlib import Path
import os, re, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'src/survivors_performance.cpp').read_text()
source=re.sub(r'^#include "(?:service_imports.hpp|config.hpp)"\n','',source,flags=re.M)
a=source.index('uint64_t now_us() {');b=source.index('#include "survivors_buffer_diagnostics.inc"',a)
source=source[:a]+'uint64_t now_us(){++clockReads;return clockUs;}\n'+source[b:]
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
constexpr size_t GFX_INLINE_DRAW_PAYLOAD_SIZE=128;
uint64_t clockUs=1000000;unsigned clockReads=0;
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
using Job=std::array<uint64_t,3>;std::vector<Job> jobs;
struct Gfx {
 ModResult register_compute_type(ModContext*,const GfxComputeTypeDesc* d,uint64_t* out){worker=d->callback;*out=1;return MOD_OK;}
 ModResult register_stage_hook(ModContext*,int,const GfxStageHookDesc* d,uint64_t* out){stageFn=d->callback;*out=2;return MOD_OK;}
 ModResult unregister_compute_type(ModContext*,uint64_t){return MOD_OK;}
 ModResult unregister_stage_hook(ModContext*,uint64_t){return MOD_OK;}
 ModResult push_compute(ModContext*,uint64_t,const void* p,size_t n){assert(!onWorker&&n==sizeof(Job));Job job;std::memcpy(job.data(),p,n);jobs.push_back(job);return MOD_OK;}
}gfx;auto* svc_gfx=&gfx;
// Exact HostService version from the official Android v2.0.3 APK.
struct Host{const char* version="2.0.3.0";} host;auto* svc_host=&host;
enum HookSymbolFlags{HOOK_SYMBOL_CODE=1,HOOK_SYMBOL_MULTI_NAME=8};
uint32_t rawStats[9]{};bool symbolAvailable=true;unsigned resolveCalls=0;
const void* getter(){assert(onWorker);return rawStats;}
struct Hook {ModResult resolve(ModContext*,const char* name,void** out,HookSymbolFlags* flags){++resolveCalls;assert(!onWorker&&!std::strcmp(name,"aurora_get_stats"));*out=reinterpret_cast<void*>(&getter);*flags=HOOK_SYMBOL_CODE;return symbolAvailable?MOD_OK:MOD_UNAVAILABLE;}} hook;
auto* svc_hook=&hook;
std::vector<std::string> messages;
struct Log{void info(ModContext*,const char* text){assert(!onWorker);messages.emplace_back(text);}} logService;auto* svc_log=&logService;
'''
main=r'''
void render(Job job){GfxComputeContext c{5*1024*1024,2*1024*1024,24*1024*1024,8*1024*1024};onWorker=true;worker(nullptr,&c,job.data(),sizeof(job),nullptr);onWorker=false;}
int main(){
 using namespace dawnlight::survivor_perf;
 BufferAlerts latch;latch.observe({850,0,0,0},{1000,0,0,0},1);assert(latch.pending());
 latch.acknowledged();latch.observe({850,0,0,0},{1000,0,0,0},2);assert(!latch.pending());
 latch.observe({640,0,0,0},{1000,0,0,0},3);latch.observe({850,0,0,0},{1000,0,0,0},4);assert(latch.pending());
 assert(initialize()==MOD_OK&&!enabled()&&stageFn&&worker);tick(true);stageFn(nullptr,nullptr,nullptr);assert(jobs.empty()&&clockReads==0);
 set_enabled(true);set_monitor(true);tick(true);stageFn(nullptr,nullptr,nullptr);assert(jobs.size()==1);
 rawStats[4]=4800000;render(jobs.back());tick(true);
 assert(sample().vertices==4800000&&peak()==4800000&&tier()==2);
 assert(sample().vertexCapacity==5*1024*1024);
 const auto old=jobs.back();reset_run();render(old);tick(true);assert(!sample().sequence&&!peak());
 // Pressure also responds to other arenas, using the actual GPU capacities.
 rawStats[4]=100;rawStats[7]=8*1024*1024;stageFn(nullptr,nullptr,nullptr);render(jobs.back());tick(true);assert(tier()==2);
 set_enabled(false);tick(true);const auto count=jobs.size();stageFn(nullptr,nullptr,nullptr);assert(jobs.size()==count&&!enabled());
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
 set_monitor(false);
 BufferScene scene;scene.map=2;scene.enemies=123;scene.effects[0]={40,32,8,64};
 static BufferScene providedScene;providedScene=scene;
 set_diagnostic_providers(+[](){return providedScene;},+[](){FogInventory f;f.state=1;f.matches=1;log_token(f.id,"example.deferred-fog");log_token(f.version,"1.2");return f;});
 set_diagnostics(true);tick(true);assert(!enabled()&&tier()==0);
 const auto warningCount=[](){unsigned n=0;for(const auto& m:messages)n+=m.find("reason=buffer_")!=std::string::npos||m.find("reason=sample_loss")!=std::string::npos;return n;};
 const auto lastWarning=[](){for(auto i=messages.rbegin();i!=messages.rend();++i)if(i->find("reason=buffer_")!=std::string::npos||i->find("reason=sample_loss")!=std::string::npos)return *i;return std::string{};};
 const auto submit=[](){stageFn(nullptr,nullptr,nullptr);render(jobs.back());};
 rawStats[7]=0;rawStats[4]=5*1024*1024*8/10;submit();
 providedScene.map=4;tick(true);
 assert(!enabled()&&tier()==0&&warningCount()==1);
 auto log=lastWarning();assert(log.find("vertex_crossed_pct=80")!=std::string::npos&&log.find(" map=2 ")!=std::string::npos);
 assert(log.find("measured=1 telemetry=available")!=std::string::npos);
 assert(log.find("burn_active=40 burn_full=32 burn_reduced=8 burn_emitters=64")!=std::string::npos);
 assert(log.find("fog_inventory=1")!=std::string::npos&&log.find("stats_frame=unknown")!=std::string::npos);
 // 90/95% crossings within a second are retained, then written with their original ages.
 clockUs+=100000;rawStats[4]=5*1024*1024*9/10;submit();tick(true);
 clockUs+=100000;rawStats[4]=5*1024*1024*95/100;submit();tick(true);assert(warningCount()==1);
 clockUs+=1000000;tick(true);assert(warningCount()==2);
 assert(lastWarning().find("vertex_crossed_pct=95")!=std::string::npos);
 assert(lastWarning().find("sample_age_ms=1000")!=std::string::npos);
 // Every arena independently warns, including non-vertex pressure with mode off.
 for(unsigned slot:{6u,5u,7u}) {
  reset_run();rawStats[4]=rawStats[5]=rawStats[6]=rawStats[7]=0;clockUs+=1000000;tick(true);
  const unsigned cap=slot==6?2*1024*1024:slot==5?24*1024*1024:8*1024*1024;
  rawStats[slot]=(uint64_t(cap)*95+99)/100;const auto n=warningCount();submit();tick(true);assert(warningCount()==n+1&&tier()==0);
 }
 // Fast growth below the absolute thresholds is also diagnostic-worthy.
 reset_run();rawStats[4]=rawStats[5]=rawStats[6]=rawStats[7]=0;clockUs+=1000000;tick(true);submit();tick(true);
 clockUs+=100000;rawStats[4]=1024*1024;submit();tick(true);
 assert(lastWarning().find("reason=buffer_growth")!=std::string::npos&&lastWarning().find("rapid_mask=1")!=std::string::npos);
 // A short item event is recorded before rendering even after another scene record.
 clockUs+=10000;providedScene.itemEvent=1;const auto logs=messages.size();tick(true);
 assert(messages.size()>logs&&messages.back().find("item_event=1")!=std::string::npos&&messages.back().find("context_only=1")!=std::string::npos);
 // A stalled consumer cannot grow memory without bound. Peaks survive overwritten samples.
 reset_run();clockUs+=1000000;tick(true);rawStats[4]=5*1024*1024;submit();
 for(unsigned i=0;i<70;++i){rawStats[4]=100;submit();}
 tick(true);assert(sample().dropped==7&&peak()==5*1024*1024);
 assert(lastWarning().find("reason=sample_loss")!=std::string::npos);
 // Missing request contexts are explicitly marked, never substituted with the current scene.
 reset_run();tick(true);rawStats[4]=5*1024*1024;submit();
 for(unsigned i=0;i<64;++i)stageFn(nullptr,nullptr,nullptr);
 clockUs+=1000000;tick(true);assert(lastWarning().find("context_missing=1")!=std::string::npos);
 const auto pending=jobs.back();set_diagnostics(false);const auto before=jobs.size();tick(true);stageFn(nullptr,nullptr,nullptr);
 assert(jobs.size()==before);render(pending);tick(true);assert(!sample().sequence);
#else
 set_diagnostics(true);tick(true);const auto before=jobs.size();stageFn(nullptr,nullptr,nullptr);assert(jobs.size()==before);
#endif
 shutdown();render(old); // Queued work becomes harmless before unregistering.
 host.version="2.1.0";assert(initialize()==MOD_OK);set_enabled(true);tick(true);
 assert(!sample().sequence);
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
 set_diagnostics(true);tick(true);
 assert(messages.back().find("telemetry=unsupported_host")!=std::string::npos);
 assert(messages.back().find("measured=0")!=std::string::npos);
#endif
 shutdown();
 host.version="2.0.3.0";symbolAvailable=false;assert(initialize()==MOD_OK);tick(true);assert(enabled()&&!sample().sequence);
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
 set_diagnostics(true);tick(true);assert(messages.back().find("telemetry=symbol_unavailable")!=std::string::npos);
#endif
 shutdown();
 // Exercise real registration, worker sampling and pressure feedback for every
 // supported spelling, rather than just testing a version parsing helper.
 symbolAvailable=true;
 for(const char* release:{"2.0.0","2.0.1","2.0.2","2.0.3"}) {
  for(const char* prefix:{"","v"})for(const char* suffix:{"",".0"}) {
   const std::string version=std::string(prefix)+release+suffix;host.version=version.c_str();
   worker=nullptr;stageFn=nullptr;jobs.clear();const auto resolves=resolveCalls;
   assert(initialize()==MOD_OK&&worker&&stageFn&&resolveCalls==resolves+1);
   set_enabled(true);set_monitor(false);tick(true);
   std::memset(rawStats,0,sizeof(rawStats));rawStats[4]=4800000;
   stageFn(nullptr,nullptr,nullptr);assert(jobs.size()==1);render(jobs.back());tick(true);
   assert(sample().sequence&&sample().vertices==4800000&&sample().vertexCapacity==5*1024*1024&&tier()==2);
   shutdown();
  }
 }
 // Do not accidentally accept newer ABIs, nonzero revisions or partial matches.
 const char* unsupported[]{nullptr,"","v","2.0","2.0.30","2.0.3.1","2.0.3.00",
  "2.0.3.0.0","2.0.3-rc1","2.0.3.0suffix","2.0.4.0","2.1.0.0","3.0.0.0"," 2.0.3.0","vv2.0.3.0"};
 for(const char* version:unsupported) {
  host.version=version;worker=nullptr;stageFn=nullptr;const auto resolves=resolveCalls;
  assert(initialize()==MOD_OK&&!worker&&!stageFn&&resolveCalls==resolves);tick(true);
  assert(enabled()&&!sample().sequence&&tier()==0);
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
  set_diagnostics(true);tick(true);
  assert(messages.back().find("telemetry=unsupported_host")!=std::string::npos);
#endif
  shutdown();
 }
}
'''
with tempfile.TemporaryDirectory() as tmp:
    tmp=Path(tmp);(tmp/'mods').mkdir();(tmp/'mods/api.h').write_text('#pragma once\nenum ModResult{MOD_OK,MOD_ERROR,MOD_UNAVAILABLE};\n')
    cpp=tmp/'test.cpp';exe=tmp/'test';cpp.write_text(fixture+source+main)
    for compiled in [0,1]:
        subprocess.run([os.environ.get('CXX','c++'),'-DDAWNLIGHT_FREEZE_DIAGNOSTICS='+str(compiled),'-std=c++20','-Wall','-Wextra','-Werror','-pthread','-I',str(tmp),'-I',str(ROOT/'src'),str(cpp),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
print('Performance monitor: optional symbols, worker samples, capacities, run epochs and shutdown passed')
