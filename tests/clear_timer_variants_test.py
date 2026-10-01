"""Exercise production rule selection, colors, nested Cave timing and cancellation."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/clear_timer.cpp').read_text()

def function(name):
    m = re.search(r'^(?:unsigned|void|JUtility::TColor) ' + name + r'\([^\n]*\) \{', source, re.M)
    assert m, name
    start = source.index('{', m.start())
    depth = 0
    for end in range(start, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if depth == 0:
            return source[m.start():end + 1]
    raise AssertionError(name)

fixture = r'''
#include "clear_timer_state.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
using namespace dawnlight;
namespace JUtility {struct TColor {
    int r,g,b,a;
    TColor(int r,int g,int b,int a):r(r),g(g),b(b),a(a){}
    bool operator==(const TColor&) const = default;
};}
timing::Records s_records;
timing::Attempt s_attempt,s_caveAttempt;
bool hard=false,randomized=false,enabled=true,active=true;
bool bossrush_hardmode_hazards_enabled(){return hard;}
bool cave_randomizer_enabled(){return randomized;}
bool clear_timer_enabled(){return enabled;}
bool save_state_boss_rush_active(){return active;}
void load_records(){}
void* mod_ctx=nullptr;
constexpr char kBlob[]="test";
constexpr int MOD_OK=0;
int writes=0;
struct Save {
    int set_blob(void*,const char*,const uint8_t* bytes,size_t length){
        timing::Records read;
        assert(read.decode({bytes,length}) && read.values==s_records.values);
        ++writes;return MOD_OK;
    }
} saveService;
auto* svc_save=&saveService;
struct Log {void warn(void*,const char*){assert(false);}} logService;
auto* svc_log=&logService;
struct Context {bool active=true,counting=true,dead=false;int encounter=0;} context;
Context clear_timer_context(){return context;}
void cancel_clear_timer(int target=-1);
'''
fixture += '\n'.join(function(name) for name in (
    'variant_for', 'timer_color', 'begin_clear_timer', 'begin_cave_boss_timer',
    'end_cave_boss_timer', 'finish_clear_timer', 'cancel_clear_timer', 'update_clear_timer'))
fixture += r'''
void second(timing::Attempt& attempt,int encounter){
    attempt.tick(0,true,encounter);attempt.tick(1,true,encounter);
}
int main(){
    for(unsigned variant=0;variant<4;++variant){
        hard=variant&1;randomized=variant&2;
        assert(variant_for(timing::cave)==variant);
        for(int key:{0,17,timing::run,timing::shade})assert(variant_for(key)==(variant&1));
        const auto color=timer_color(variant);
        if(variant==3)assert(color.b>color.r && color.r>color.g); // purple
        else if(variant)assert(color.r>color.g && color.r>color.b); // red
        else assert(color.r>color.g && color.g>color.b); // normal gold
        begin_clear_timer(timing::cave);
        second(s_attempt,timing::cave);
        finish_clear_timer(timing::cave);
        assert(s_records.values[timing::cave+variant*timing::count]==1000);
    }
    // A fairy boss records its own hard-mode result and contributes to the
    // enclosing purple Cave timer, excluding departure/arrival intervals.
    begin_clear_timer(timing::cave);
    second(s_attempt,timing::cave);
    begin_cave_boss_timer(5);
    assert(s_caveAttempt.target==timing::cave && s_caveAttempt.variant==3);
    assert(s_attempt.target==5 && s_attempt.variant==1);
    assert(!s_caveAttempt.wasCounting);
    second(s_caveAttempt,5);second(s_attempt,5);
    finish_clear_timer(5);
    assert(s_records.values[5]==0 && s_records.values[5+timing::count]==1000);
    end_cave_boss_timer();
    assert(s_attempt.target==timing::cave && s_attempt.elapsed==2000);
    assert(!s_attempt.wasCounting && s_caveAttempt.target==-1);
    finish_clear_timer(timing::cave);
    assert(s_records.values[timing::cave+3*timing::count]==2000);

    // A mid-attempt rule change cannot overwrite a record in either category.
    const int oldWrites=writes;
    begin_clear_timer(timing::run);second(s_attempt,0);
    hard=false;finish_clear_timer(0);
    assert(s_attempt.target==-1 && writes==oldWrites);
    begin_clear_timer(timing::cave);begin_cave_boss_timer(1);
    randomized=false;update_clear_timer();
    assert(s_attempt.target==-1 && s_caveAttempt.target==-1);
    for(int reason=0;reason<3;++reason){
        begin_clear_timer(timing::cave);begin_cave_boss_timer(2);
        if(reason==0)enabled=false;
        if(reason==1)active=false;
        if(reason==2)context.dead=true;
        update_clear_timer();
        assert(s_attempt.target==-1 && s_caveAttempt.target==-1);
        enabled=true;active=true;context.dead=false;
    }
    assert(writes==oldWrites);
    std::cout << "Timer rule banks, red/purple colors, nested Cave totals and invalidation: OK\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'variants.cpp'
    cpp.write_text(fixture)
    binary = Path(tmp) / 'variants'
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root/'src'), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
