"""Exercise production HUD dialog callbacks with Dusklight's FileService ABI."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1] if len(sys.argv) > 1 else root / "dusklight")
source = (root / "src/ui.cpp").read_text()
body = source[source.index("bool s_hudFilePickPending"):source.index("void copy_hud_preset_settings")]
fixture = r'''
#include "mods/svc/file.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
ModContext* mod_ctx = nullptr;
const FileService* svc_file = nullptr;
enum class HudSettingsIoResult {Ok, ReadFailed, WriteFailed, InvalidFormat};
std::vector<std::string> toasts;
void push_toast(const char* title, const char*, const char* = nullptr) { toasts.emplace_back(title); }
const char* hud_settings_io_result_message(HudSettingsIoResult) { return "IO error"; }
int staged=0, applied=0, opened=0, closed=0;
std::string contents=R"({"elements":{}})", imported;
HudSettingsIoResult stageResult=HudSettingsIoResult::Ok;
HudSettingsIoResult export_custom_hud_settings(std::string& path) {
    ++staged; path="/mod-data/hud_layout_settings.json"; return stageResult;
}
HudSettingsIoResult import_custom_hud_settings_json(const std::string& json) {
    imported=json;
    if (json!=contents || json.empty()) return HudSettingsIoResult::InvalidFormat;
    ++applied; return HudSettingsIoResult::Ok;
}
FilePickFn callback=nullptr;
void* callbackData=nullptr;
ModResult startResult=MOD_OK, openResult=MOD_OK, sizeResult=MOD_OK, readResult=MOD_OK, closeResult=MOD_OK;
uint64_t offset=0, reportedSize=0;
bool stalled=false;
const char* picked="content://documents/tree/custom%20HUD/document/42";
ModResult pick(ModContext*, const FilePickOptions* options, FilePickFn fn, void* data) {
    assert(options->struct_size==sizeof(*options) && options->filter_count==1);
    assert(std::string(options->filters[0].pattern)=="json");
    if(startResult==MOD_OK) { callback=fn; callbackData=data; }
    return startResult;
}
ModResult export_file(ModContext*, const char* path, const char* name, FilePickFn fn, void* data) {
    assert(std::string(path)=="/mod-data/hud_layout_settings.json");
    assert(std::string(name)=="hud_layout_settings.json");
    if(startResult==MOD_OK) { callback=fn; callbackData=data; }
    return startResult;
}
ModResult open(ModContext*, const char* location, FileOpenMode mode, FileStreamHandle* out) {
    assert(std::string(location)==picked && mode==FILE_OPEN_READ);
    ++opened;offset=0;*out=7;return openResult;
}
ModResult size(ModContext*, FileStreamHandle stream, uint64_t* out) {
    assert(stream==7);*out=reportedSize;return sizeResult;
}
ModResult read(ModContext*, FileStreamHandle stream, void* out, uint64_t count, uint64_t* got) {
    assert(stream==7);
    *got=stalled?0:std::min<uint64_t>({count,3,contents.size()-offset});
    std::memcpy(out,contents.data()+offset,*got);offset+=*got;return readResult;
}
ModResult close(ModContext*, FileStreamHandle stream) { assert(stream==7);++closed;return closeResult; }
void finish(ModResult status, uint32_t count=1) {
    assert(callback);auto fn=callback;callback=nullptr;
    const char* locations[]={picked};fn(nullptr,status,locations,count,nullptr,callbackData);
}
// PRODUCTION
int main() {
    FileService api{};api.header=SERVICE_HEADER(FileService,1,0);
    api.pick_file=pick;api.export_file=export_file;
    api.open=open;api.size=size;api.read=read;api.close=close;
    export_hud_settings(nullptr,nullptr);import_hud_settings(nullptr,nullptr);
    assert(staged==0 && opened==0 && toasts.size()==2);
    svc_file=&api;api.header.struct_size=sizeof(ServiceHeader);
    import_hud_settings(nullptr,nullptr);assert(!callback);
    api.header.struct_size=sizeof(api);
    // No success notification until the host completes the actual export.
    toasts.clear();export_hud_settings(nullptr,nullptr);
    assert(staged==1 && callback && toasts.empty());
    export_hud_settings(nullptr,nullptr);import_hud_settings(nullptr,nullptr);
    assert(staged==1 && opened==0);
    finish(MOD_UNAVAILABLE);assert(toasts.empty() && !s_hudFilePickPending);
    export_hud_settings(nullptr,nullptr);finish(MOD_OK);
    assert(toasts.back()=="HUD Exported");
    export_hud_settings(nullptr,nullptr);finish(MOD_ERROR);
    assert(toasts.back()=="HUD Export Failed");
    stageResult=HudSettingsIoResult::WriteFailed;
    export_hud_settings(nullptr,nullptr);assert(!callback && !s_hudFilePickPending);
    stageResult=HudSettingsIoResult::Ok;
    for(auto status:{MOD_CONFLICT,MOD_UNSUPPORTED,MOD_ERROR}) {
        startResult=status;import_hud_settings(nullptr,nullptr);assert(!s_hudFilePickPending);
        export_hud_settings(nullptr,nullptr);assert(!s_hudFilePickPending);
    }
    startResult=MOD_OK;toasts.clear();
    import_hud_settings(nullptr,nullptr);finish(MOD_UNAVAILABLE);
    assert(toasts.empty() && applied==0 && opened==0);
    import_hud_settings(nullptr,nullptr);finish(MOD_OK,0);assert(applied==0 && opened==0);
    reportedSize=contents.size();
    import_hud_settings(nullptr,nullptr);finish(MOD_OK);
    assert(applied==1 && imported==contents && closed==1 && toasts.back()=="HUD Imported");
    // Read opaque locations through the service, including partial reads.
    picked="C:/Users/test/HUD layouts/custom.json";
    import_hud_settings(nullptr,nullptr);finish(MOD_OK);assert(applied==2 && closed==2);
    for(auto bytes:{uint64_t(0),kMaxHudImportBytes+1}) {
        reportedSize=bytes;import_hud_settings(nullptr,nullptr);finish(MOD_OK);
        assert(applied==2);
    }
    reportedSize=contents.size();
    sizeResult=MOD_ERROR;import_hud_settings(nullptr,nullptr);finish(MOD_OK);sizeResult=MOD_OK;
    readResult=MOD_ERROR;import_hud_settings(nullptr,nullptr);finish(MOD_OK);readResult=MOD_OK;
    stalled=true;import_hud_settings(nullptr,nullptr);finish(MOD_OK);stalled=false;
    closeResult=MOD_ERROR;import_hud_settings(nullptr,nullptr);finish(MOD_OK);closeResult=MOD_OK;
    assert(applied==2 && closed==opened);
    openResult=MOD_UNAVAILABLE;import_hud_settings(nullptr,nullptr);finish(MOD_OK);
    assert(applied==2 && closed==opened-1 && !s_hudFilePickPending);
}
'''.replace("// PRODUCTION", body)
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / "test.cpp"
    exe = Path(tmp) / "test"
    cpp.write_text(fixture)
    subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-isystem", str(sdk / "sdk/include"), str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("HUD file picker passed: async export, cancel/busy/failure, opaque locations, bounded partial reads and close failures")
