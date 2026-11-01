"""Exercise the production UI style registration lifecycle without a host."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <cassert>
#include <cstring>
using UiStyleHandle = unsigned long long;
constexpr int MOD_OK=0, UI_SCOPE_WINDOW=1;
void* mod_ctx=nullptr;
bool s_active=false;
namespace survivor_perf { bool mode=true; bool enabled(){return mode;} }
unsigned registrations=0, removals=0, warnings=0;
bool fail=false;
struct Ui {
    int register_styles(void*, int scope, const char* css, UiStyleHandle* handle) {
        assert(scope==UI_SCOPE_WINDOW && std::strlen(css)>0);
        ++registrations;
        if(fail) return 1;
        *handle=42; return MOD_OK;
    }
    void unregister_styles(void*, UiStyleHandle handle) {assert(handle==42);++removals;}
} ui;
auto* svc_ui=&ui;
struct Log {void warn(void*,const char*){++warnings;}} logService;
auto* svc_log=&logService;
#include "survivors_performance_ui.inc"
int main() {
    update_performance_ui(); assert(registrations==0);
    s_active=true;
    for(int i=0;i<1000;++i) update_performance_ui();
    assert(registrations==1 && removals==0 && s_performanceUiStyle==42);
    survivor_perf::mode=false; update_performance_ui();
    assert(removals==1 && s_performanceUiStyle==0);
    update_performance_ui(); assert(removals==1);
    survivor_perf::mode=true; update_performance_ui(); assert(registrations==2);
    s_active=false; update_performance_ui(); assert(removals==2);
    s_active=true; update_performance_ui(); assert(registrations==3);
    clear_performance_ui(); clear_performance_ui(); assert(removals==3);
    fail=true;
    for(int i=0;i<1000;++i) update_performance_ui();
    assert(registrations==4 && warnings==1 && s_performanceUiStyle==0);
    clear_performance_ui(); assert(removals==3);
    fail=false; update_performance_ui(); assert(registrations==5);
    clear_performance_ui(); assert(removals==4);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    source = Path(tmp)/"test.cpp"
    binary = Path(tmp)/"test"
    source.write_text(fixture)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra",
                    "-Werror", "-I"+str(root/"src"), str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Survivors performance UI lifecycle passed")
