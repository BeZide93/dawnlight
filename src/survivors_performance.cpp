#include "survivors_performance.hpp"
#include "service_imports.hpp"
#include "config.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>

namespace dawnlight::survivor_perf {
namespace {
// AuroraStats ABI from Dusklight 2.0.0-2.0.3. No hard import on unsupported hosts.
struct HostStats {uint32_t queued,created,draws,merged,vertices,uniforms,indices,storage,uploads;};
using GetStats=const HostStats*(*)();
std::mutex mutex;
GetStats getStats=nullptr;
Sample latest{};
struct Request {uint64_t epoch=0,serial=0,queuedUs=0;};
static_assert(sizeof(Request)<=GFX_INLINE_DRAW_PAYLOAD_SIZE);
std::array<Sample,64> workerSamples{};
size_t workerBegin=0,workerCount=0;
uint64_t requestSequence=0,acceptedEpoch=1,droppedSamples=0,requestFailures=0;
std::array<uint32_t,4> workerPeaks{};
uint64_t workerSequence=0,epoch=1;
uint32_t runPeak=0;
Pressure pressure;
ConfigVarHandle modeConfig=0,monitorConfig=0;
GfxComputeTypeHandle compute=0;
GfxStageHookHandle stage=0;
bool mode=false,monitor=false,active=false,workerEnabled=false,diagnostics=false;
unsigned enemyCount=0,projectileCount=0,burnCount=0,boltCount=0,emitterCount=0;
std::chrono::steady_clock::time_point lastLog{};
bool setting(ConfigVarHandle h) {int64_t v=0;if(h)svc_config->get_int(mod_ctx,h,&v);return v!=0;}
uint64_t now_us() {
    return uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
#include "survivors_buffer_diagnostics.inc"
void read_stats(ModContext*,const GfxComputeContext* ctx,const void* payload,size_t size,void*) {
    std::lock_guard lock(mutex);
    if(!workerEnabled || !getStats || size!=sizeof(Request))return;
    Request request;std::memcpy(&request,payload,sizeof(request));
    if(request.epoch!=acceptedEpoch)return;
    const auto* s=getStats();if(!s)return;
    Sample sample{++workerSequence,request.epoch,s->vertices,s->indices,s->uniforms,s->storage,s->draws,
        wgpuBufferGetSize(ctx->vertex_buffer),wgpuBufferGetSize(ctx->index_buffer),
        wgpuBufferGetSize(ctx->uniform_buffer),wgpuBufferGetSize(ctx->storage_buffer)};
    sample.request=request.serial;sample.requestedUs=request.queuedUs;
    if(request.queuedUs)sample.readUs=now_us();
    const std::array<uint32_t,4> used{s->vertices,s->indices,s->uniforms,s->storage};
    for(unsigned i=0;i<4;++i)workerPeaks[i]=std::max(workerPeaks[i],used[i]);
    sample.peaks=workerPeaks;
    if(workerCount==workerSamples.size()){workerBegin=(workerBegin+1)%workerSamples.size();--workerCount;++droppedSamples;}
    sample.dropped=droppedSamples;
    workerSamples[(workerBegin+workerCount++)%workerSamples.size()]=sample;
}
void queue_stats(ModContext*,const GfxStageContext*,void*) {
    if((mode||diagnostics) && active && compute) {
        Request request{epoch,++requestSequence,diagnostics?now_us():0};
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
        if(diagnostics)diagnosticContexts[request.serial%diagnosticContexts.size()]={request.serial,currentScene,currentFog,mode,pressure.tier,sceneCapturedUs};
#endif
        if(svc_gfx->push_compute(mod_ctx,compute,&request,sizeof(request))!=MOD_OK)++requestFailures;
    }
}
}
ModResult initialize() {
    ConfigVarDesc d=CONFIG_VAR_DESC_INIT;d.type=CONFIG_VAR_INT;d.default_int=0;
    d.name="survivors-performance-mode";auto r=register_owned_setting(&d,&modeConfig);if(r!=MOD_OK)return r;
    d.name="survivors-performance-monitor";r=register_owned_setting(&d,&monitorConfig);if(r!=MOD_OK)return r;
    mode=setting(modeConfig);monitor=setting(monitorConfig);
    const char* v=svc_host->version;if(v && *v=='v')++v;
    const bool known=v && (!std::strcmp(v,"2.0.0")||!std::strcmp(v,"2.0.1")||!std::strcmp(v,"2.0.2")||!std::strcmp(v,"2.0.3"));
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
    measurementStatus=known?"symbol_unavailable":"unsupported_host";
#endif
    void* address=nullptr;HookSymbolFlags flags{};
    if(!known || svc_hook->resolve(mod_ctx,"aurora_get_stats",&address,&flags)!=MOD_OK ||
        !address || !(flags&HOOK_SYMBOL_CODE) || (flags&HOOK_SYMBOL_MULTI_NAME))return MOD_OK;
    {std::lock_guard lock(mutex);getStats=reinterpret_cast<GetStats>(address);workerEnabled=true;}
    const GfxComputeTypeDesc c{sizeof(c),"Survivors buffer statistics",read_stats,nullptr};
    const GfxStageHookDesc h{sizeof(h),queue_stats,nullptr};
    if(svc_gfx->register_compute_type(mod_ctx,&c,&compute)!=MOD_OK ||
       svc_gfx->register_stage_hook(mod_ctx,GFX_STAGE_FRAME_AFTER_HUD,&h,&stage)!=MOD_OK) {
        if(compute)svc_gfx->unregister_compute_type(mod_ctx,compute);
        compute=0;std::lock_guard lock(mutex);workerEnabled=false;getStats=nullptr;
    }
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
    measurementStatus=compute?"available":"gfx_registration_failed";
#endif
    return MOD_OK;
}
void shutdown() {
    {std::lock_guard lock(mutex);workerEnabled=false;getStats=nullptr;workerCount=workerBegin=0;}
    if(stage)svc_gfx->unregister_stage_hook(mod_ctx,stage);
    if(compute)svc_gfx->unregister_compute_type(mod_ctx,compute);
    stage=compute=0;active=mode=monitor=diagnostics=false;
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
    sceneProvider=nullptr;fogProvider=nullptr;reset_diagnostic_window();
#endif
}
bool enabled(){return mode;}
bool monitor_enabled(){return mode&&monitor;}
void set_enabled(bool v){if(modeConfig)svc_config->set_int(mod_ctx,modeConfig,v);}
void set_monitor(bool v){if(monitorConfig)svc_config->set_int(mod_ctx,monitorConfig,v);}
void reset_run(){
    ++epoch;latest={};runPeak=0;pressure={};lastLog={};requestFailures=0;
    {std::lock_guard lock(mutex);acceptedEpoch=epoch;workerCount=workerBegin=0;workerPeaks={};droppedSamples=0;}
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
    reset_diagnostic_window();
#endif
}
void set_diagnostics(bool value){
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
    if(value!=diagnostics){diagnostics=value;reset_run();if(value)diagnosticStart=now_us();}
#else
    (void)value;
#endif
}
void set_diagnostic_providers(BufferScene (*scene)(),FogInventory (*fog)()) {
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
    sceneProvider=scene;fogProvider=fog;
#else
    (void)scene;(void)fog;
#endif
}
void tick(bool running) {
    const bool next=setting(modeConfig);monitor=setting(monitorConfig);
    if(next!=mode || running!=active)reset_run();
    mode=next;active=running;
    if((!mode&&!diagnostics) || !active)return;
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
    if(diagnostics)diagnostic_capture(now_us());
#endif
    std::array<Sample,64> samples;size_t count;
    {std::lock_guard lock(mutex);count=workerCount;
        for(size_t i=0;i<count;++i)samples[i]=workerSamples[(workerBegin+i)%workerSamples.size()];
        workerCount=workerBegin=0;}
    for(size_t i=0;i<count;++i) {
        const auto& s=samples[i];if(s.epoch!=epoch || s.sequence==latest.sequence)continue;
        latest=s;runPeak=std::max(runPeak,s.peaks[0]);
        uint64_t fraction=0;
        const auto add=[&](uint64_t n,uint64_t cap){if(cap)fraction=std::max(fraction,n*10000/cap);};
        add(s.vertices,s.vertexCapacity);add(s.indices,s.indexCapacity);
        add(s.uniforms,s.uniformCapacity);add(s.storage,s.storageCapacity);
        if(mode)pressure.observe(fraction,10000);
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
        if(diagnostics)diagnostic_observe(s);
#endif
    }
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
    if(diagnostics)diagnostic_report(now_us());
#endif
    const auto now=std::chrono::steady_clock::now();
    if(mode && monitor && now-lastLog>=std::chrono::seconds(1)) {
        lastLog=now;char text[400];
        std::snprintf(text,sizeof(text),"Survivors Performance: measured=%u vertex=%u/%llu peak=%u index=%u/%llu uniform=%u/%llu storage=%u/%llu tier=%u enemies=%u projectiles=%u burns=%u bolts=%u reservedEmitters=%u",
            unsigned(latest.sequence!=0),latest.vertices,(unsigned long long)latest.vertexCapacity,runPeak,
            latest.indices,(unsigned long long)latest.indexCapacity,latest.uniforms,(unsigned long long)latest.uniformCapacity,
            latest.storage,(unsigned long long)latest.storageCapacity,pressure.tier,enemyCount,projectileCount,burnCount,boltCount,emitterCount);
        svc_log->info(mod_ctx,text);
    }
}
unsigned tier(){return pressure.tier;}
Sample sample(){return latest;}
uint32_t peak(){return runPeak;}
void counts(unsigned e,unsigned p,unsigned b,unsigned l,unsigned em){enemyCount=e;projectileCount=p;burnCount=b;boltCount=l;emitterCount=em;}
void status(char* text,unsigned size){
    if(!latest.sequence)std::snprintf(text,size,"Performance Mode | Buffer measurement unavailable | Fixed budgets");
    else std::snprintf(text,size,"Vertex %.2f / %.2f MiB | Peak %.2f | Effects %u",
        latest.vertices/1048576.,latest.vertexCapacity/1048576.,runPeak/1048576.,pressure.tier);
}
}
