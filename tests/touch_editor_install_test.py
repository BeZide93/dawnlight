"""Exercise the production editor installer with the SDK and a failing hook service."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
dusk = Path(os.environ.get('DUSKLIGHT_DIR', root/'dusklight'))
if not dusk.exists():
    dusk = root.parent/'dusk-source'
source = (root/'src/touch_button_editor.inc').read_text()
# Use the actual declared symbol names, with simple signatures for this installer test.
hooks = re.findall(r'DEFINE_HOOK_SYMBOL\("([^"]+)",.*?, (ExtraEditor\w+)\);', source, re.S)
assert hooks and all('end_edit' not in name for name, _ in hooks)
fixture = r'''
#define DUSK_MOD_FEATURE_GAME 1
#include <mods/hook.hpp>
#include <array>
#include <cassert>
#include <set>
#include <string>
#include <vector>
DEFINE_MOD();
const HookService* svc_hook=nullptr;
struct Log {std::vector<std::string> messages;void warn(ModContext*,const char* s){messages.emplace_back(s);}} logService;
auto* svc_log=&logService;
bool s_touchEditorAvailable=false;
struct EditorApi {void (*construct)(),(*push)(),(*top)(),(*uncover)(),(*hide)(),(*bindControls)(),(*listenTouch)(),(*listenMouse)(),(*deleteListener)(),(*setClass)();} s_editorApi;
bool resolveMethods=true;
template<class T> bool resolve_extra(const char*,T&){return resolveMethods;}
HookAction layout_extra_editor(ModContext*,void*,void*,void*){return HOOK_CONTINUE;}
HookAction enter_extra_editor(ModContext*,void*,void*,void*){return HOOK_CONTINUE;}
void leave_extra_editor(ModContext*,void*,void*,void*){}
void extra_editor_controls(ModContext*,void*,void*,void*){}
void extra_editor_fragment(ModContext*,void*,void*,void*){}
HookAction save_extra_editor(ModContext*,void*,void*,void*){return HOOK_CONTINUE;}
HookAction extra_editor_delete(ModContext*,void*,void*,void*){return HOOK_CONTINUE;}
HookAction extra_editor_push(ModContext*,void*,void*,void*){return HOOK_CONTINUE;}
// HOOKS
// INSTALLER
std::set<void*> active;
int operation=0,failAt=-1;
ModResult maybe_fail(){return operation++==failAt?MOD_ERROR:MOD_OK;}
ModResult install(ModContext*,void* addr,void*,void**){
 auto r=maybe_fail();if(r==MOD_OK)active.insert(addr);return r;
}
ModResult pre(ModContext*,void*,HookPreFn,const HookOptions*){return maybe_fail();}
ModResult post(ModContext*,void*,HookPostFn,const HookOptions*){return maybe_fail();}
ModResult remove_hook(ModContext*,void* addr,void**){active.erase(addr);return MOD_OK;}
int main(){
 HookService service{};service.header.struct_size=sizeof(service);
 service.install=install;service.add_pre=pre;service.add_post=post;service.uninstall=remove_hook;svc_hook=&service;
 // TARGETS
 // No end_edit symbol exists, as on Android v2.0.1; all necessary hooks still install.
 assert(install_extra_editor(nullptr)==MOD_OK&&s_touchEditorAvailable);
 const int allOperations=operation;assert(active.size()==targets.size());active.clear();
 // Every unavailable symbol is detected before any detour is installed.
 for(auto target:targets){
  auto saved=*target;*target=nullptr;operation=0;
  assert(install_extra_editor(nullptr)==MOD_OK&&!s_touchEditorAvailable&&operation==0&&active.empty());
  *target=saved;
 }
 // Every install/pre/post failure rolls back only this optional adapter.
 void* otherHook=reinterpret_cast<void*>(999);
 for(int i=0;i<allOperations;++i){
  active={otherHook};operation=0;failAt=i;
  assert(install_extra_editor(nullptr)==MOD_OK&&!s_touchEditorAvailable);
  assert(active==std::set<void*>{otherHook});
 }
 active.clear();operation=0;failAt=-1;resolveMethods=false;
 assert(install_extra_editor(nullptr)==MOD_OK&&!s_touchEditorAvailable&&operation==0);
 resolveMethods=true;service.uninstall=nullptr;
 assert(install_extra_editor(nullptr)==MOD_OK&&!s_touchEditorAvailable&&operation==0);
 service.uninstall=remove_hook;
 assert(install_extra_editor(nullptr)==MOD_OK&&s_touchEditorAvailable&&active.size()==targets.size());
}
'''
fixture=fixture.replace('// HOOKS','\n'.join(f'DEFINE_HOOK_SYMBOL("{name}", void(), {alias});' for name,alias in hooks))
fixture=fixture.replace('// INSTALLER',source[source.index('struct EditorHookBinding'):])
fixture=fixture.replace('// TARGETS','std::array targets = {'+','.join(f'&mod_meta_hook_{alias}.resolved' for _,alias in hooks)+'};\n for(size_t i=0;i<targets.size();++i)*targets[i]=reinterpret_cast<void*>(i+1);')
with tempfile.TemporaryDirectory() as tmp:
    cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Wno-attributes','-Wno-cpp','-I'+str(dusk/'sdk/include'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Editor installation passed: no end_edit dependency; missing symbols and all hook failures preserve mod initialization and roll back the adapter')

# Optional real-host contract check; the APK is not needed for the mock failures.
# DUSKLIGHT_APK=/path/to/Dusklight-v2.0.1-android-arm64.apk python3 tests/touch_editor_install_test.py
if apk := os.environ.get('DUSKLIGHT_APK'):
    import ctypes
    import ctypes.util
    import struct
    import zipfile
    with zipfile.ZipFile(apk) as archive:
        binary = archive.read('lib/arm64-v8a/libmain.so')
    offset = binary.index(b'SYMGEN\0\0')
    magic,version,compression,ulen,clen,bidlen,bid,count = struct.unpack_from('<8sIIQQI32sI',binary,offset)
    assert version == 2 and compression in (0,1) and bidlen <= 32
    payload = binary[offset+72:offset+72+clen]
    if compression == 1:
        zstd = ctypes.CDLL(ctypes.util.find_library('zstd'))
        zstd.ZSTD_decompress.restype = ctypes.c_size_t
        zstd.ZSTD_decompress.argtypes = [ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.c_size_t]
        output = ctypes.create_string_buffer(ulen)
        assert zstd.ZSTD_decompress(output,ulen,payload,clen) == ulen
        payload = output.raw
    assert len(payload) == ulen
    strings = payload[count*24:]
    entries = {}
    for _,rva,name_offset,flags in struct.iter_unpack('<QQII',payload[:count*24]):
        name = strings[name_offset:strings.index(0,name_offset)].decode()
        entries[name] = (rva,flags)
    required = re.findall(r'(?:DEFINE_HOOK_SYMBOL|resolve_extra)\("([^\"]+)"',source)
    for name in required:
        assert name in entries, f'Missing host symbol: {name}'
        rva,flags = entries[name]
        assert rva and flags & 1 and not flags & 16, f'Not uniquely hookable: {name}'
    print(f'APK contract passed: {len(set(required))} editor symbols; build ID {bid[:bidlen].hex()}')
