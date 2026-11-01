"""Mode-owned title replacement: activation, switching, failures and bundle format."""
from pathlib import Path
import os, re, struct, subprocess, tempfile

root = Path(__file__).resolve().parents[1]
core = (root/'src/survivors.cpp').read_text()
asset = root/'res/survivors/tex1_608x100_0c1c70378fb8cb46_6.png'
png = asset.read_bytes()
assert png[:8] == b'\x89PNG\r\n\x1a\n'
# Same 8x native canvas as the supplied texture, with real RGBA transparency.
assert struct.unpack('>IIBB', png[16:26]) == (4864, 800, 8, 6)
assert 'RES_DIR res' in (root/'CMakeLists.txt').read_text()
assert 'unregister_survivors_title_logo();' in core[core.index('void shutdown_survivors()'):]

def function(name):
    match = re.search(r'^ModResult '+name+r'\([^\n]*\) \{', core, re.M)
    assert match
    start = core.index('{', match.start())
    depth = 0
    for end in range(start, len(core)):
        depth += (core[end] == '{') - (core[end] == '}')
        if not depth:
            return core[match.start():end+1]
    raise AssertionError(name)

fixture = r'''
#include <cassert>
#include <cstdint>
#include <string>
using TextureReplacementHandle=uint64_t;
using ModResult=int;constexpr ModResult MOD_OK=0,MOD_UNAVAILABLE=1;
struct ModError{std::string message;};struct ModContext{};
ModContext* mod_ctx=nullptr;
namespace mods{ModResult set_error(ModError* e,ModResult r,const char* text){if(e)e->message=text;return r;}}
unsigned registrations=0,removals=0,warnings=0,resets=0,previews=0;
bool failRegister=false,failRemove=false,s_active=false;
TextureReplacementHandle nextHandle=41,registeredHandle=0;
struct TextureService{
 ModResult register_file(ModContext*,const char* path,TextureReplacementHandle* handle){
  assert(std::string(path)=="res/survivors/tex1_608x100_0c1c70378fb8cb46_6.png");
  ++registrations;*handle=0;if(failRegister)return MOD_UNAVAILABLE;
  registeredHandle=*handle=nextHandle++;return MOD_OK;
 }
 ModResult unregister(ModContext*,TextureReplacementHandle handle){
  assert(handle==registeredHandle&&handle!=0);++removals;registeredHandle=0;
  return failRemove?MOD_UNAVAILABLE:MOD_OK;
 }
}textureService;auto* svc_texture=&textureService;
struct Log{void warn(ModContext*,const char*){++warnings;}}logger;auto* svc_log=&logger;
void clear_performance_ui(){}
void reset_runtime(){++resets;}void request_title_preview(){++previews;}
'''
main = r'''
int main(){
 assert(!s_titleTexture&&!registrations);
 ModError error;failRegister=true;
 assert(activated(nullptr,&error)==MOD_UNAVAILABLE&&!s_active&&!s_titleTexture&&!resets&&!previews);
 assert(error.message.find("title texture")!=std::string::npos);
 failRegister=false;assert(activated(nullptr,nullptr)==MOD_OK&&s_active&&s_titleTexture==41);
 assert(registrations==2&&resets==1&&previews==1);
 // Re-entry and reset do not duplicate the selected mode's registration.
 assert(activated(nullptr,nullptr)==MOD_OK&&registrations==2&&s_titleTexture==41);
 reset_runtime();assert(s_titleTexture==41);
 assert(deactivated(nullptr,nullptr)==MOD_OK&&!s_active&&!s_titleTexture&&removals==1);
 deactivated(nullptr,nullptr);assert(removals==1);
 assert(activated(nullptr,nullptr)==MOD_OK&&s_titleTexture==42&&registrations==3);
 unregister_survivors_title_logo(); // The same cleanup is used on mod shutdown.
 assert(!s_titleTexture&&removals==2);
 unregister_survivors_title_logo();assert(removals==2);
 activated(nullptr,nullptr);failRemove=true;deactivated(nullptr,nullptr);
 assert(!s_active&&!s_titleTexture&&warnings==1&&removals==3);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'logo.cpp', Path(tmp)/'logo'
    cpp.write_text(fixture+(root/'src/survivors_title_texture.inc').read_text()+
                   function('activated')+function('deactivated')+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors title texture: mode ownership, switching/reset, failure cleanup and PNG format passed')
