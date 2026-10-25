"""Exercise the production menu save against the host's native save state machine."""
from pathlib import Path
import os, re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
host=(root/'dusklight/src/dusk/autosave.cpp').read_text()
def extract(name):
    match=re.search(r'^(?:void|bool) '+name+r'\(',host,re.M)
    assert match,name
    start=host.index('{',match.start());depth=0
    for end in range(start,len(host)):
        depth+=(host[end]=='{')-(host[end]=='}')
        if not depth:return host[match.start():end+1]
    raise AssertionError(name)
fixture=r'''
#include <cassert>
#include <chrono>
#include <string>
#include <vector>
#include "survivors_rules.hpp"
#include "../include/survivors_maps.hpp"
using namespace dawnlight::survivors;
using u8=uint8_t;using u32=uint32_t;using ModResult=int;
constexpr ModResult MOD_OK=0,MOD_ERROR=1;
struct ModContext{};ModContext* mod_ctx=nullptr;
struct Log{std::vector<std::string> warnings;void warn(ModContext*,const char* text){warnings.emplace_back(text);}}logger;
auto* svc_log=&logger;
Run s_run;Records s_records;bool s_restart=false,s_progressSaving=false,s_progressSaved=false,s_storeFailed=false,s_saveRequested=false;
bool s_hubDeparture=false,s_hubSaveRequested=false,transition=false,event=false;
bool autosaveEnabled=true;bool survivors_autosave_enabled(){return autosaveEnabled;}
bool in_arena(){return true;}bool transitioning(){return transition;}bool dComIfGp_event_runCheck(){return event;}
unsigned s_window=42;void close_window(){s_window=0;}
constexpr char kBlob[]="records";unsigned flushes=0,returnWrites=0;
unsigned nativeLoads=0,nativeWrites=0;int status=2,loadSync=1,saveSync=1;
struct Card {
 int getStatus(int){return status;}void load(){++nativeLoads;}
 int LoadSync(void*,size_t,int){return loadSync;}
 void save(void*,size_t,int){++nativeWrites;}int SaveSync(){return saveSync;}
}g_mDoMemCd_control;
int mDoMemCd_getStatus(int i){return g_mDoMemCd_control.getStatus(i);}
constexpr unsigned QUEST_LOG_SIZE=16;u8 mSaveBuffer[QUEST_LOG_SIZE*3]{};
u8 mAutoSaveProc=0;int autoSaveWriteState=0;
struct stage_stag_info_class{}stage;
stage_stag_info_class* dComIfGp_getStageStagInfo(){return &stage;}
int dStage_stagInfo_GetSaveTbl(stage_stag_info_class*){return 7;}
void dComIfGs_putSave(int n){assert(n==7);}
int dComIfGs_getDataNum(){return 1;}
void dComIfGs_setMemoryToCard(u8*,int slot){assert(slot==1);}
void mDoMemCdRWm_SetCheckSumGameData(u8*,int slot){assert(slot==1);}
void mDoMemCdRWm_TestCheckSumGameData(u8*){}
namespace dusk::mods::svc {void save_slot_written(int slot){assert(slot==1);++flushes;}}
namespace dusk::ui {
 struct Toast{const char* type;std::chrono::milliseconds duration;};void push_toast(Toast){}
}
struct ReturnPlace{void set(const char* stage,int room,int point){assert(std::string(stage)==arenas[0].stage&&room==arenas[0].room&&point==arenas[0].point);++returnWrites;}}place;
struct Player{ReturnPlace& getPlayerReturnPlace(){return place;}}player;
struct Data{Player& getPlayer(){return player;}}data;
Data* dComIfGs_getSaveData(){return &data;}
const Arena& arena(){return arenas[0];}
struct Save{bool fail=false;std::vector<uint8_t> blob;
 ModResult set_blob(ModContext*,const char*,const void* p,size_t n){if(fail)return MOD_ERROR;auto* b=static_cast<const uint8_t*>(p);blob.assign(b,b+n);return MOD_OK;}
}save;auto* svc_save=&save;
void enterAutoSave();
struct Hook{bool fail=false;std::string failName;bool nullAddress=false;std::vector<std::string> requests;
 ModResult resolve(ModContext*,const char* name,void** address,void*){
  requests.emplace_back(name);
  if(fail || failName==name)return MOD_ERROR;
  if(nullAddress){*address=nullptr;return MOD_OK;}
  if(std::string(name)=="enterAutoSave")*address=reinterpret_cast<void*>(&enterAutoSave);
#ifdef _WIN32
  // Actual Windows v2.0.3 manifest entries: no undecorated data aliases.
  else if(std::string(name)=="?mAutoSaveProc@@3EA")*address=&mAutoSaveProc;
  else if(std::string(name)=="?autoSaveWriteState@@3HA")*address=&autoSaveWriteState;
#else
  else if(std::string(name)=="mAutoSaveProc")*address=&mAutoSaveProc;
  else if(std::string(name)=="autoSaveWriteState")*address=&autoSaveWriteState;
#endif
  else return MOD_ERROR;
  return MOD_OK;
 }
}hook;auto* svc_hook=&hook;
'''
main=r'''
int main(){
 s_run.phase=Phase::Ready;s_records.total_xp=meta_threshold(3);s_records.high_scores[0]=1234;
 assert(s_records.buy(Talent::MaxHealth));
 hook.fail=true;save_progress(nullptr,nullptr);assert(s_storeFailed&&!s_progressSaving&&s_window==42&&!nativeLoads);
 assert(logger.warnings.back().find("could not resolve enterAutoSave")!=std::string::npos);
 hook.fail=false;
 for(const auto* name:{kProgressSavePhaseSymbol,kProgressSaveResultSymbol}){
  hook.failName=name;save_progress(nullptr,nullptr);
  assert(s_storeFailed&&!s_progressSaving&&!nativeLoads&&!s_enterProgressSave&&!s_hostSavePhase&&!s_hostSaveResult);
  assert(logger.warnings.back().find(name)!=std::string::npos);
 }
 hook.failName.clear();hook.nullAddress=true;save_progress(nullptr,nullptr);
 assert(s_storeFailed&&!s_progressSaving&&!nativeLoads);hook.nullAddress=false;
 status=3;save_progress(nullptr,nullptr);assert(s_storeFailed&&!nativeLoads);
 assert(logger.warnings.back().find("memory card is not ready (status 3)")!=std::string::npos);
 status=2;mAutoSaveProc=3;save_progress(nullptr,nullptr);assert(s_storeFailed&&!nativeLoads);mAutoSaveProc=0;
 save.fail=true;save_progress(nullptr,nullptr);assert(s_storeFailed&&!nativeLoads);save.fail=false;
 s_restart=true;save_progress(nullptr,nullptr);assert(!nativeLoads);s_restart=false;
 s_run.phase=Phase::Running;save_progress(nullptr,nullptr);assert(!nativeLoads);s_run.phase=Phase::Ready;
 save_progress(nullptr,nullptr);assert(s_progressSaving&&!s_storeFailed&&!s_progressSaved&&nativeLoads==1&&mAutoSaveProc==2);
 Records stored;assert(stored.decode(save.blob)&&stored.encode()==s_records.encode());
 save_progress(nullptr,nullptr);assert(nativeLoads==1&&returnWrites==1);
 update_progress_save();assert(s_progressSaving&&!s_progressSaved&&s_window==42&&!s_saveRequested);
 loadSync=0;autoSaving();update_progress_save();assert(mAutoSaveProc==2&&nativeWrites==0);
 loadSync=1;autoSaving();assert(nativeWrites==1&&mAutoSaveProc==3);
 saveSync=0;waitingForWrite();update_progress_save();assert(s_progressSaving&&!flushes);
 saveSync=1;waitingForWrite();update_progress_save();assert(s_progressSaving&&mAutoSaveProc==4&&!flushes);
 endAutoSave();update_progress_save();assert(!s_progressSaving&&s_progressSaved&&!s_storeFailed&&flushes==1);
 assert(s_window==42&&!s_saveRequested); // Neither a close nor the foreground save dialog.
 // A native disk failure keeps the menu open and permits a subsequent retry.
 save_progress(nullptr,nullptr);autoSaving();saveSync=2;waitingForWrite();update_progress_save();
 assert(s_storeFailed&&!s_progressSaved&&!s_progressSaving&&s_window==42&&flushes==1);
 assert(logger.warnings.back().find("native write failed (result 2)")!=std::string::npos);
 save_progress(nullptr,nullptr);autoSaving();saveSync=1;waitingForWrite();endAutoSave();update_progress_save();
 assert(s_progressSaved&&!s_storeFailed&&flushes==2&&s_window==42);
 // Menu-close autosave waits for the host and events, then flushes the same slot.
 close_window();s_hubSaveRequested=s_hubDeparture=true;mAutoSaveProc=3;
 assert(update_hub_save()&&s_hubSaveRequested&&!s_progressSaving);
 mAutoSaveProc=0;event=true;assert(!update_hub_save()&&s_hubSaveRequested);event=false;
 assert(update_hub_save()&&s_progressSaving&&!s_hubSaveRequested&&s_hubDeparture);
 autoSaving();waitingForWrite();endAutoSave();assert(update_hub_save());
 assert(s_progressSaved&&!s_storeFailed&&flushes==3&&s_hubDeparture);
 assert(!update_hub_save());
 // Storage failure cancels departure, leaving a visible retry route in the hub.
 s_hubSaveRequested=true;status=3;assert(!update_hub_save());
 assert(s_storeFailed&&!s_hubDeparture&&!s_hubSaveRequested);status=2;
 s_hubSaveRequested=s_hubDeparture=true;assert(update_hub_save());
 autoSaving();saveSync=2;waitingForWrite();assert(update_hub_save());
 assert(s_storeFailed&&!s_hubDeparture&&!s_progressSaving);
 // Disabled requests must not touch storage, resolve native symbols, report
 // failure, or hold the door closed, even during events/transitions/host saves.
 const auto loadsBefore=nativeLoads,writesBefore=nativeWrites,flushesBefore=flushes;
 const auto resolutionsBefore=hook.requests.size();
 autosaveEnabled=false;s_storeFailed=false;save.fail=true;hook.fail=true;status=3;
 for(int gate=0;gate<4;++gate){
  event=gate==1;transition=gate==2;mAutoSaveProc=gate==3?3:0;
  s_hubSaveRequested=s_hubDeparture=true;
  assert(!update_hub_save()&&!s_hubSaveRequested&&s_hubDeparture&&!s_storeFailed);
 }
 assert(nativeLoads==loadsBefore&&nativeWrites==writesBefore&&flushes==flushesBefore);
 assert(hook.requests.size()==resolutionsBefore);
 event=transition=false;mAutoSaveProc=0;save.fail=hook.fail=false;status=2;saveSync=1;
 autosaveEnabled=true;assert(!update_hub_save()&&nativeLoads==loadsBefore); // No stale request.
 // Explicit saves still work with autosave disabled.
 autosaveEnabled=false;save_progress(nullptr,nullptr);assert(s_progressSaving&&nativeLoads==loadsBefore+1);
 autoSaving();waitingForWrite();endAutoSave();assert(update_hub_save()&&s_progressSaved);
 // Turning autosave off during an automatic write allows that write to finish.
 autosaveEnabled=true;s_hubSaveRequested=true;assert(update_hub_save()&&s_progressSaving);
 autosaveEnabled=false;assert(update_hub_save()&&s_progressSaving);
 autoSaving();waitingForWrite();endAutoSave();assert(update_hub_save()&&s_progressSaved&&!s_progressSaving);
 assert(flushes==flushesBefore+2);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
    cpp.write_text(fixture+'\n'.join(extract(n) for n in ['writeAutoSave','autoSaving','enterAutoSave','waitingForWrite','endAutoSave'])+'\n'+(root/'src/survivors_progress_save.inc').read_text()+main)
    for defines in [[],['-D_WIN32']]:
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',*defines,'-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
print('Survivors progress save: Windows/Unix symbols, resolution diagnostics, native async save, slot flush, busy/failure and retry passed')
