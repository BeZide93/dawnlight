#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <string_view>

namespace dawnlight::survivor_perf {
enum class Visual : unsigned { Burn, Wave, Lightning, Explosion, Boomerang, FireArrow, FireEdge, Jump, Chain, Count };
struct VisualCount { unsigned active=0,full=0,reduced=0,emitters=0; };
struct BufferScene {
    unsigned map=0,phase=0,tick=0,enemies=0,projectiles=0,reserved=0;
    // 0 none identified, 1 hub Ordon Sword, 2 Survivors treasure presentation.
    unsigned itemEvent=0,reward=0;
    bool hub=false,transition=false,paused=false,event=false,menu=false;
    std::array<VisualCount,unsigned(Visual::Count)> effects{};
};
struct FogInventory {
    // -1 inventory unavailable, 0 no match, 1 matching active package. Not its rendering toggle.
    int state=-1;
    unsigned matches=0;
    std::array<char,64> id{};
    std::array<char,32> version{};
};
inline bool fog_name_matches(std::string_view text) {
    constexpr std::string_view needle="deferredfog";
    size_t matched=0;
    for(unsigned char c:text) {
        if(c>='A'&&c<='Z')c+=32;
        if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')))continue;
        if(c==needle[matched]) {if(++matched==needle.size())return true;}
        else matched=c==needle[0]?1:0;
    }
    return false;
}
template<size_t N> void log_token(std::array<char,N>& out,std::string_view text) {
    out={};size_t n=0;
    for(unsigned char c:text) {
        if(n==N-1)break;
        out[n++]=((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='-'||c=='_')?char(c):'_';
    }
}
// Per-arena threshold latches rearm below 65%. No per-frame log spam at a boundary.
struct BufferAlerts {
    std::array<unsigned,4> latched{},crossed{};
    std::array<uint32_t,4> previous{};
    uint64_t previousUs=0;
    unsigned rapidMask=0;
    void observe(const std::array<uint32_t,4>& used,const std::array<uint64_t,4>& cap,uint64_t us) {
        for(unsigned i=0;i<4;++i) {
            if(!cap[i])continue;
            const auto percent=uint64_t(used[i])*100/cap[i];
            if(percent<65)latched[i]=0;
            const unsigned level=percent>=95?3:percent>=90?2:percent>=80?1:0;
            if(level>latched[i]) {crossed[i]=std::max(crossed[i],level);latched[i]=level;}
            if(previousUs && us>=previousUs && us-previousUs<=250000 && used[i]>previous[i] &&
               used[i]-previous[i]>=256*1024 && uint64_t(used[i]-previous[i])*100>=cap[i]*15)
                rapidMask|=1u<<i;
        }
        previous=used;previousUs=us;
    }
    bool pending()const {return rapidMask||std::any_of(crossed.begin(),crossed.end(),[](auto n){return n!=0;});}
    void acknowledged(){crossed={};rapidMask=0;}
};
}
