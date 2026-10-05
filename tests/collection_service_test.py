"""Exercise the production adapter using the exact service ABI supplied in TE's demo."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1] if len(sys.argv) > 1 else root / "dusklight")
source = (root / "src/collection_service.cpp").read_text()
body = source[source.index("namespace dawnlight {"):]
fixture = r'''
#include "twilit_essentials/collection.h"
#include "mods/svc/host.h"
#include "mods/svc/log.h"
#include "dual_wield_sword.hpp"
#include <cassert>
#include <cstring>
#include <string_view>
using u8 = uint8_t;
constexpr uint32_t dItemNo_NONE_e=255,dItemNo_WOOD_SHIELD_e=42,dItemNo_SHIELD_e=43,dItemNo_HYLIA_SHIELD_e=44;
ModContext* mod_ctx=nullptr;
bool enabled=true,owned=true,archiveReady=true,iconReady=true;
uint32_t shield=dItemNo_WOOD_SHIELD_e;
dawnlight::SecondSword sword=dawnlight::SecondSword::Ordon;
namespace dawnlight {
bool dual_wield_enabled(){return enabled;}
SecondSword second_sword(){return sword;}
}
bool dComIfGs_isItemFirstBit(u8 item){return owned && item>=42 && item<=44;}
uint32_t dComIfGs_getSelectEquipShield(){return shield;}
struct Entry {uint16_t getFileID(){return static_cast<uint16_t>(70+static_cast<int>(sword));}} entry;
struct Archive {
    Entry* findNameResource(const char* name){
        assert(std::string_view(name)==dawnlight::second_sword_assets(sword).icon);
        return iconReady?&entry:nullptr;
    }
} archive;
Archive* dComIfGp_getCollectResArchive(){return archiveReady?&archive:nullptr;}
HostService host{}; const HostService* svc_host=&host;
LogService logService{}; const LogService* svc_log=&logService;
TwilitEssentialsCollectionService provider{};
const TwilitEssentialsCollectionService* published=nullptr;
TwilitEssentialsCollectionSlotDesc last{};
ModLifecycleFn lifecycle=nullptr;
int adds=0,removes=0,refreshes=0,warnings=0;
bool selected=false,alias=false,addFails=false,removeFails=false,queryFails=false;
uint64_t liveHandle=0;
ModResult get_service(ModContext*,const char* id,uint16_t major,uint16_t minor,const void** out){
    assert(std::string_view(id)==TWILIT_ESSENTIALS_COLLECTION_SERVICE_ID && major==1 && minor==0);
    *out=published;return published?MOD_OK:MOD_UNAVAILABLE;
}
ModResult watch(ModContext*,ModLifecycleFn fn,void*,uint64_t* out){lifecycle=fn;*out=1;return MOD_OK;}
ModResult unwatch(ModContext*,uint64_t handle){assert(handle==1);lifecycle=nullptr;return MOD_OK;}
void warn(ModContext*,const char*){++warnings;}
ModResult add(ModContext*,const TwilitEssentialsCollectionSlotDesc* desc,uint64_t* out){
    ++adds;assert(!liveHandle);
    if(addFails)return MOD_UNAVAILABLE;
    assert(desc->struct_size==sizeof(*desc));
    assert(desc->kind==TWILIT_ESSENTIALS_COLLECTION_SHIELD && desc->column==0);
    assert(desc->base_item==shield && owned);
    assert(desc->model_arc==nullptr && desc->icon==nullptr);
    assert(desc->icon_arc_file_id==entry.getFileID());
    assert(std::string_view(desc->name)==dawnlight::second_sword_assets(sword).name);
    assert(desc->is_unlocked(desc->user_data));
    last=*desc;liveHandle=*out=uint64_t(adds);selected=false;return MOD_OK;
}
ModResult remove(ModContext*,uint64_t handle){
    ++removes;assert(handle==liveHandle);
    if(removeFails)return MOD_UNAVAILABLE;
    liveHandle=0;last={};selected=false;return MOD_OK;
}
ModResult equipped(ModContext*,uint64_t handle,uint32_t* out){
    assert(handle==liveHandle);if(queryFails)return MOD_UNAVAILABLE;
    *out=alias || selected;return MOD_OK;
}
ModResult refresh(ModContext*){++refreshes;return MOD_OK;}
// PRODUCTION
void tick(int count=1){while(count--)dawnlight::update_collection_service();}
int main(){
    using namespace dawnlight;
    host.get_service=get_service;host.watch_mod_lifecycle=watch;host.unwatch_mod_lifecycle=unwatch;
    logService.warn=warn;
    provider={SERVICE_HEADER(TwilitEssentialsCollectionService,1,0),add,remove,equipped,refresh};
    assert(initialize_collection_service(nullptr)==MOD_OK);
    tick();assert(adds==0 && !collection_service_active());
    published=&provider;
    provider.header.struct_size=sizeof(ServiceHeader);tick();assert(adds==0);
    provider.header.struct_size=sizeof(provider);provider.add_slot=nullptr;tick();assert(adds==0);
    provider.add_slot=add;archiveReady=false;tick();assert(adds==0);
    archiveReady=true;iconReady=false;tick();assert(adds==0);
    iconReady=true;shield=dItemNo_NONE_e;tick();assert(adds==0);
    shield=dItemNo_WOOD_SHIELD_e;owned=false;tick();assert(adds==0);
    owned=true;enabled=false;tick();assert(adds==0);
    enabled=true;tick();assert(adds==1 && collection_service_active() && !collection_service_equipped());
    selected=true;assert(collection_service_equipped());tick(5);assert(adds==1);
    selected=false;assert(!collection_service_equipped()); // native or foreign shield selected
    // Selection is read from the provider for each save, never copied into a global or Dawnlight blob.
    selected=true;assert(collection_service_equipped());selected=false;assert(!collection_service_equipped());
    queryFails=true;assert(!collection_service_equipped());queryFails=false;
    for(auto next:{SecondSword::Wooden,SecondSword::Master,SecondSword::Ordon}){
        sword=next;tick();assert(collection_service_active() && !collection_service_equipped());
        assert(last.is_unlocked(nullptr));
    }
    enabled=false;assert(!last.is_unlocked(nullptr));tick();assert(!liveHandle && !collection_service_active());
    enabled=true;addFails=true;int count=adds;tick();assert(adds==count+1 && !collection_service_active());
    tick(10);assert(adds==count+1);addFails=false;tick(51);assert(collection_service_active());
    removeFails=true;sword=SecondSword::Wooden;count=adds;tick();assert(adds==count && liveHandle);
    assert(!collection_service_equipped());removeFails=false;tick(61);assert(adds==count+1);
    // Provider unloads: never call remove on its stale function table.
    count=removes;published=nullptr;liveHandle=0;last={};
    lifecycle(nullptr,nullptr,"com.dusklight.twilit_essentials",MOD_LIFECYCLE_DETACHED,nullptr);
    tick();assert(!collection_service_active() && removes==count);
    published=&provider;tick();assert(collection_service_active()); // same address, fresh handle
    shutdown_collection_service();assert(!liveHandle && !lifecycle);
    // A model-less native-item alias must not auto-enable Dual Wield.
    initialize_collection_service(nullptr);alias=true;tick();
    assert(!collection_service_active() && !liveHandle && warnings==1);
    count=adds;tick(61);assert(!collection_service_active() && warnings==1 && adds==count);
    alias=false;
    lifecycle(nullptr,nullptr,"com.dusklight.twilit_essentials",MOD_LIFECYCLE_DETACHED,nullptr);
    tick();assert(collection_service_active());
    owned=false;tick();assert(!collection_service_active() && !liveHandle);
    shutdown_collection_service();
}
'''
with tempfile.TemporaryDirectory(prefix="dawnlight-collection-service-") as directory:
    cpp, exe = Path(directory) / "test.cpp", Path(directory) / "test"
    cpp.write_text(fixture.replace("// PRODUCTION", body))
    subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-isystem", str(sdk / "sdk/include"), "-I", str(root / "include"),
                    "-I", str(root / "src"), str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("TE collection service passed: demo ABI, slots, native icons, unlocks, selection, failure recovery, unload/reload, alias rejection")
