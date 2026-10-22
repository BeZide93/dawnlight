#include "survivors_freeze_diagnostics.hpp"
#if DAWNLIGHT_FREEZE_DIAGNOSTICS
#include "survivors_freeze_stats.hpp"
#include "service_imports.hpp"
#include "mods/hook.hpp"
#include "f_ap/f_ap_game.h"
#include "d/d_resorce.h"
#include <chrono>
#include <cstdio>

#ifndef DAWNLIGHT_DIAGNOSTIC_REVISION
#define DAWNLIGHT_DIAGNOSTIC_REVISION "unknown"
#endif
#ifndef DAWNLIGHT_DIAGNOSTIC_BUILD
#define DAWNLIGHT_DIAGNOSTIC_BUILD "local"
#endif
#ifndef DAWNLIGHT_VERSION
#define DAWNLIGHT_VERSION "unknown"
#endif

namespace dawnlight::freeze_diag {
namespace {
DEFINE_HOOK(&fapGm_Execute, DiagnosticExecute);
DEFINE_HOOK(static_cast<int (dRes_info_c::*)()>(&dRes_info_c::setRes), DiagnosticResource);
uint64_t now_us() {
    return uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
constexpr const char* metric_names[]{"execute","resource","contacts","contact_dispatch","contact_delete","update","player"};
Window window{};
Context current{};
uint64_t epoch=0,last_pulse=0,generation=1,sequence=0;
unsigned held=0;
bool execute_hook=false,resource_hook=false;
HookStack execute_stack{},resource_stack{};
// Same-metric nested scopes count once, retaining their inclusive outer time.
std::array<unsigned,size_t(Metric::Count)> scope_depth{};

double ms(uint64_t us){return double(us)/1000.;}
void emit(const char* reason,uint64_t now) {
    // One bounded line per window. Never log from an actor/contact/resource hook.
    char line[2304];
    const int n=std::snprintf(line,sizeof(line),
        "[SurvivorsDiag] seq=%llu t_ms=%.1f reason=%s window_ms=%.1f map=%u phase=%u tick=%u hub=%u arena=%u transition=%u paused=%u event=%u menu=%u gap_max_ms=%.2f gaps=%llu slow_gaps=%llu cache_held=%u acquire=%u duplicate_ready=%u full=%u release=%u rejected_in=%u rejected_out=%u stack_overflow=%u unmatched_post=%u",
        static_cast<unsigned long long>(++sequence),ms(now-epoch),reason,ms(now-window.started),
        current.map,current.phase,current.tick,unsigned(current.hub),unsigned(current.arena),unsigned(current.transition),
        unsigned(current.paused),unsigned(current.event),unsigned(current.menu),ms(window.gap_max_us),
        static_cast<unsigned long long>(window.gaps),static_cast<unsigned long long>(window.gap_slow),held,
        window.acquired,window.reused,window.full,window.released,window.rejected_in,window.rejected_out,window.overflows,window.unmatched);
    size_t used=n>0?std::min(size_t(n),sizeof(line)-1):0;
    for(size_t i=0;i<window.samples.size();++i) {
        const auto& sample=window.samples[i];
        const int wrote=std::snprintf(line+used,sizeof(line)-used," %s_calls=%llu %s_total_ms=%.2f %s_max_ms=%.2f",
            metric_names[i],static_cast<unsigned long long>(sample.calls),metric_names[i],ms(sample.total_us),metric_names[i],ms(sample.max_us));
        if(wrote>0)used+=std::min(size_t(wrote),sizeof(line)-used-1);
    }
    std::snprintf(line+used,sizeof(line)-used," resource_peak_archive=%s",window.samples[size_t(Metric::Resource)].slowest.data());
    svc_log->info(mod_ctx,line);
    // Exclude this log write from the next update gap and reporting window.
    window={};window.started=now_us();last_pulse=window.started;
}
void pre(HookStack& stack,void* args,const char* name=nullptr) {
    const bool engaged=enabled&&current.active;
    if(!stack.push(args,engaged?now_us():0,generation,engaged,name)&&engaged)++window.overflows;
}
void post(HookStack& stack,void* args,Metric metric) {
    HookStack::Frame frame;
    const bool overflowing=stack.overflow!=0;
    if(!stack.pop(args,frame)){if(enabled&&current.active&&!overflowing)++window.unmatched;return;}
    if(frame.engaged&&enabled&&current.active&&frame.generation==generation&&stack.depth==0)
        window.sample(metric,now_us()-frame.start,frame.name.data());
}
HookAction before_execute(ModContext*,void* args,void*,void*) {
    if(execute_hook)pre(execute_stack,args);
    return HOOK_CONTINUE;
}
void after_execute(ModContext*,void* args,void*,void*) {
    if(execute_hook)post(execute_stack,args,Metric::Execute);
}
HookAction before_resource(ModContext*,void* args,void*,void*) {
    if(resource_hook){auto* info=mods::arg<dRes_info_c*>(args,0);pre(resource_stack,args,info?info->getArchiveName():nullptr);}
    return HOOK_CONTINUE;
}
void after_resource(ModContext*,void* args,void*,void*) {
    if(resource_hook)post(resource_stack,args,Metric::Resource);
}
}
void Scope::begin() {
    generation_=generation;
    if(enabled&&current.active){engaged_=true;if(scope_depth[size_t(metric_)]++==0)start_=now_us();}
}
void Scope::finish() {
    if(!engaged_||generation_!=generation)return;
    if(--scope_depth[size_t(metric_)]==0&&enabled&&current.active)
        window.sample(metric_,now_us()-start_);
}
void context(Context value) {
    if(!enabled)return;
    if(current.active!=value.active) {
        const auto now=now_us();
        if(current.active)emit("mode_end",now);
        ++generation;scope_depth={};window={};window.started=now;last_pulse=0;
    }
    current=value;
}
void pulse() {
    if(!enabled||!current.active){last_pulse=0;return;}
    const auto now=now_us();
    if(last_pulse)window.gap(now-last_pulse);
    last_pulse=now;
    if(window.due(now))emit(window.slow?"slow":"heartbeat",now);
}
void cache(CacheEvent event) {
    if(!enabled)return;
    switch(event) {
    case CacheEvent::Acquire:++held;++window.acquired;break;
    case CacheEvent::Reuse:++window.reused;break;
    case CacheEvent::Full:++window.full;break;
    case CacheEvent::Release:if(held)--held;++window.released;break;
    }
}
void rejected_contact(bool incoming) {
    if(enabled&&current.active)++(incoming?window.rejected_in:window.rejected_out);
}
void initialize(bool requested) {
    // Called once after settings registration, before the game loop starts.
    // Off must not read the clock, install hooks or emit even a startup line.
    if(!requested)return;
    epoch=now_us();window={};window.started=epoch;last_pulse=0;generation=1;sequence=0;held=0;
    current={};scope_depth={};execute_stack={};resource_stack={};enabled=true;
    // Optional paired hooks: a failed half remains inert. Never uninstall a
    // shared target, which would also remove gameplay callbacks on that target.
    const auto execute_post=mods::hook_add_post<DiagnosticExecute>(svc_hook,after_execute);
    const auto execute_pre=mods::hook_add_pre<DiagnosticExecute>(svc_hook,before_execute);
    execute_hook=execute_pre==MOD_OK&&execute_post==MOD_OK;
    const auto resource_post=mods::hook_add_post<DiagnosticResource>(svc_hook,after_resource);
    const auto resource_pre=mods::hook_add_pre<DiagnosticResource>(svc_hook,before_resource);
    resource_hook=resource_pre==MOD_OK&&resource_post==MOD_OK;
    char line[512];std::snprintf(line,sizeof(line),
        "[SurvivorsDiag] enabled schema=1 version=%s commit=%s build=%s execute_hook=%u resource_hook=%u clock=steady_us gap_threshold_ms=100 section_threshold_ms=50 rate_limit_ms=1000 heartbeat_ms=30000",
        DAWNLIGHT_VERSION,DAWNLIGHT_DIAGNOSTIC_REVISION,DAWNLIGHT_DIAGNOSTIC_BUILD,unsigned(execute_hook),unsigned(resource_hook));
    svc_log->info(mod_ctx,line);
    if(!execute_hook||!resource_hook)svc_log->warn(mod_ctx,"[SurvivorsDiag] optional native timing hook unavailable; update-gap and local scope measurements remain active");
}
void shutdown() {
    if(enabled){emit("shutdown",now_us());svc_log->info(mod_ctx,"[SurvivorsDiag] stopped");}
    enabled=false;execute_hook=resource_hook=false;++generation;scope_depth={};execute_stack={};resource_stack={};
}
}
#endif
