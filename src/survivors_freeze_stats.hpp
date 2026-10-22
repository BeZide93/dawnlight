#pragma once
#include "survivors_freeze_diagnostics.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>

namespace dawnlight::freeze_diag {
struct Sample {
    uint64_t calls=0,total_us=0,max_us=0;
    std::array<char,32> slowest{};
    void add(uint64_t elapsed,const char* name=nullptr) {
        ++calls;total_us+=elapsed;
        if(elapsed>=max_us){max_us=elapsed;slowest={};if(name) {
            const auto count=std::min(std::strlen(name),slowest.size()-1);
            std::memcpy(slowest.data(),name,count);
        }}
    }
};
// Hook nesting uses fixed storage; overflow is counted and balanced without
// overwriting a valid outer start. A foreign post never consumes an outer call.
struct HookStack {
    struct Frame {void* args=nullptr;uint64_t start=0,generation=0;bool engaged=false;std::array<char,32> name{};};
    std::array<Frame,16> frames{};
    unsigned depth=0,overflow=0;
    bool push(void* args,uint64_t start,uint64_t generation,bool engaged,const char* name=nullptr) {
        if(depth==frames.size()){++overflow;return false;}
        auto& f=frames[depth++];f={args,start,generation,engaged,{}};
        if(name)std::memcpy(f.name.data(),name,std::min(std::strlen(name),f.name.size()-1));
        return true;
    }
    bool pop(void* args,Frame& out) {
        if(overflow){--overflow;return false;}
        if(!depth||frames[depth-1].args!=args)return false;
        out=frames[--depth];return true;
    }
};
struct Window {
    uint64_t started=0,gap_max_us=0,gap_slow=0,gaps=0;
    std::array<Sample,size_t(Metric::Count)> samples{};
    unsigned acquired=0,reused=0,full=0,released=0,rejected_in=0,rejected_out=0,overflows=0,unmatched=0;
    bool slow=false;
    void gap(uint64_t elapsed){++gaps;gap_max_us=std::max(gap_max_us,elapsed);if(elapsed>=100000){++gap_slow;slow=true;}}
    void sample(Metric metric,uint64_t elapsed,const char* name=nullptr) {
        samples[size_t(metric)].add(elapsed,name);
        slow|=elapsed>=(metric==Metric::Execute?100000u:50000u);
    }
    bool due(uint64_t now)const{return now>=started && now-started>=(slow?1000000u:30000000u);}
};
}
