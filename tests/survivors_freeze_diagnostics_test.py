"""Production diagnostics: deterministic timings, bounded output and optional hooks."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/survivors_freeze_diagnostics.cpp').read_text()
# Mock the engine/host boundary; run the actual reporter, hooks, RAII and counters.
source=re.sub(r'^#include "(?:service_imports.hpp|mods/hook.hpp|f_ap/f_ap_game.h|d/d_resorce.h)"\n','',source,flags=re.M)
a=source.index('uint64_t now_us() {');b=source.index('\nconstexpr const char* metric_names',a)
source=source[:a]+'uint64_t now_us(){++clock_reads;return clock_us;}\n'+source[b:]
fixture=r'''
#include <any>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#define DAWNLIGHT_FREEZE_DIAGNOSTICS 1
#define DAWNLIGHT_VERSION "5.2.0"
#define DAWNLIGHT_DIAGNOSTIC_REVISION "abc123"
#define DAWNLIGHT_DIAGNOSTIC_BUILD "1206"
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
constexpr int MOD_OK=0,MOD_ERROR=1;
using Pre=HookAction(*)(ModContext*,void*,void*,void*);
using Post=void(*)(ModContext*,void*,void*,void*);
uint64_t clock_us=1000000;unsigned clock_reads=0;
void* mod_ctx=nullptr;int svc_hook=0;
std::vector<std::string> messages;unsigned warnings=0,install_calls=0,fail_at=0;
struct Log {void info(void*,const char* msg){messages.emplace_back(msg);clock_us+=2000;}void warn(void*,const char*){++warnings;}}logService;
auto* svc_log=&logService;
#define DEFINE_HOOK(Target,Alias) struct Alias{}
namespace mods {
template<class T>T arg(void* data,unsigned i){return std::any_cast<T>(static_cast<std::any*>(data)[i]);}
template<class T>int hook_add_pre(int,Pre){return ++install_calls==fail_at?MOD_ERROR:MOD_OK;}
template<class T>int hook_add_post(int,Post){return ++install_calls==fail_at?MOD_ERROR:MOD_OK;}
}
struct dRes_info_c {const char* name="E_sf";const char* getArchiveName(){return name;}};
'''
main=r'''
int main(){
 using namespace dawnlight::freeze_diag;
 // Default-off initialization and all inline scope paths do no timing or logging.
 initialize(false);context({});pulse();cache(CacheEvent::Acquire);rejected_contact(true);
 {Scope scope(Metric::Contacts);}
 shutdown();assert(clock_reads==0&&install_calls==0&&messages.empty()&&warnings==0);
 // Each failed half is inert; normal diagnostics remain available.
 for(fail_at=1;fail_at<=4;++fail_at){
  install_calls=warnings=0;initialize(true);assert(install_calls==4&&warnings==1);
  assert(execute_hook==(fail_at>2));assert(resource_hook==(fail_at<=2));
  assert(messages.back().find("build=1206")!=std::string::npos);shutdown();
 }
 fail_at=0;install_calls=0;initialize(true);assert(execute_hook&&resource_hook);
 assert(messages.back().find("commit=abc123")!=std::string::npos);
 messages.clear();Context c{};c.active=true;c.arena=true;c.map=2;c.phase=2;
 context(c);pulse();
 {Scope outer(Metric::Contacts);clock_us+=100;{Scope inner(Metric::Contacts);clock_us+=200;}clock_us+=100;}
 assert(window.samples[size_t(Metric::Contacts)].calls==1&&window.samples[size_t(Metric::Contacts)].total_us==400);
 std::any a[]{std::any{}};before_execute(nullptr,a,nullptr,nullptr);clock_us+=150000;
 after_execute(nullptr,a,nullptr,nullptr);assert(window.samples[0].max_us==150000);
 dRes_info_c archive;std::any resource[]{&archive};
 before_resource(nullptr,resource,nullptr,nullptr);clock_us+=200000;after_resource(nullptr,resource,nullptr,nullptr);
 assert(window.samples[1].max_us==200000&&std::string(window.samples[1].slowest.data())=="E_sf");
 cache(CacheEvent::Acquire);cache(CacheEvent::Acquire);cache(CacheEvent::Reuse);cache(CacheEvent::Full);cache(CacheEvent::Release);
 rejected_contact(true);rejected_contact(false);assert(held==1);
 pulse();assert(messages.empty());clock_us+=700000;pulse();assert(messages.size()==1);
 auto line=messages.back();assert(line.find("reason=slow")!=std::string::npos&&line.find("map=2")!=std::string::npos);
 assert(line.find("resource_peak_archive=E_sf")!=std::string::npos&&line.find("resource_max_ms=200.00")!=std::string::npos);
 assert(line.find("contacts_total_ms=0.40")!=std::string::npos&&line.find("cache_held=1")!=std::string::npos);
 assert(line.find("rejected_in=1 rejected_out=1")!=std::string::npos&&line.size()<2304);
 const auto start=clock_us;clock_us+=33000;pulse();assert(window.gap_max_us==33000); // Excludes logger's simulated 2 ms.
 // A real multi-second update gap gets a single event, then quiet normal frames.
 clock_us+=3000000;pulse();assert(messages.size()==2&&messages.back().find("gap_max_ms=3000.00")!=std::string::npos);
 for(unsigned i=0;i<20;++i){clock_us+=33333;pulse();}assert(messages.size()==2);
 for(unsigned i=0;i<90;++i){clock_us+=333333;pulse();}
 assert(messages.size()>2&&messages.size()<=33); // At most one line/second despite every gap being slow.
 (void)start;
 // Reset to normal cadence and verify the 30-second heartbeat without false stalls.
 context({});messages.clear();context(c);pulse();
 for(unsigned i=0;i<901;++i){clock_us+=33333;pulse();}
 assert(messages.size()==1&&messages[0].find("reason=heartbeat")!=std::string::npos);
 assert(messages[0].find("slow_gaps=0")!=std::string::npos);
 // Foreign/nested posts and stack overflow cannot consume a valid outer start.
 context({});context(c);messages.clear();int x=0,y=0;
 before_execute(nullptr,&x,nullptr,nullptr);clock_us+=10;
 after_execute(nullptr,&y,nullptr,nullptr);assert(execute_stack.depth==1&&window.unmatched==1);
 before_execute(nullptr,&y,nullptr,nullptr);clock_us+=20;after_execute(nullptr,&y,nullptr,nullptr);
 clock_us+=10;after_execute(nullptr,&x,nullptr,nullptr);assert(window.samples[0].calls==1&&window.samples[0].max_us==40);
 for(unsigned i=0;i<32;++i)before_execute(nullptr,&x,nullptr,nullptr);
 for(unsigned i=0;i<32;++i)after_execute(nullptr,&x,nullptr,nullptr);
 assert(execute_stack.depth==0&&execute_stack.overflow==0&&window.overflows==16);
 // Mode changes while a scope/hook is open discard cross-boundary measurements.
 {Scope outer(Metric::Contacts);before_execute(nullptr,&x,nullptr,nullptr);context({});clock_us+=1000000;context(c);after_execute(nullptr,&x,nullptr,nullptr);}
 assert(!window.samples[0].calls&&!window.samples[size_t(Metric::Contacts)].calls);
 {Scope outer(Metric::Contacts);clock_us+=25;}assert(window.samples[size_t(Metric::Contacts)].total_us==25);
 context({});messages.clear();clock_us+=5000000;pulse();assert(messages.empty());
 shutdown();assert(!enabled&&messages.back()=="[SurvivorsDiag] stopped");
 puts("Survivors diagnostics: timings, optional hooks, nesting, overflow, mode boundaries and bounded logs passed");
}
'''
# New diagnostic scopes must leave actual hit acceptance/deletion/cache logic intact.
contacts=(root/'src/survivors_contacts.inc').read_text()
assert 'const auto& peer=native_target_peer(collider);' in contacts
assert 'return actor && (!peer.mAc || peer.mAc==actor);' in contacts
assert 'return collider.ChkTgHitNoActor();' in contacts
with tempfile.TemporaryDirectory() as temp:
    cpp=Path(temp)/'test.cpp';exe=Path(temp)/'test';cpp.write_text(fixture+source+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
    # Default release mode has no diagnostic symbols or engine/header dependencies.
    cpp.write_text('#include "survivors_freeze_diagnostics.cpp"\nint main(){}\n')
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
