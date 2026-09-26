#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <span>

namespace dawnlight::collection {
struct Cell { int x, y; float left, top, width, height; bool available; };
struct Placement { float left=0, top=0, size=0; int neighbor=-1; };
// Match only the diagonal corners, not another slot's nearby ornament.
inline int flourish_corner(const Cell& frame,const Cell& ornament) {
    if(frame.width<=0 || frame.height<=0) return -1;
    const float cx=ornament.left+ornament.width*.5f,cy=ornament.top+ornament.height*.5f;
    for(int corner=0;corner<2;++corner) {
        const float x=frame.left+corner*frame.width,y=frame.top+corner*frame.height;
        if(std::fabs(cx-x)<frame.width*.3f && std::fabs(cy-y)<frame.height*.3f) return corner;
    }
    return -1;
}
// Use rendered positions, not native column numbers (mods remap those).
inline Placement append_shield(std::span<const Cell> cells) {
    Placement out;
    float right=-std::numeric_limits<float>::max();
    for (std::size_t i=0;i<cells.size();++i) {
        const auto& cell=cells[i];
        if(cell.y!=1 || cell.width<=0 || cell.height<=0) continue;
        if(cell.left+cell.width>right) {
            right=cell.left+cell.width;
            out={right+cell.width*0.2f,cell.top,std::min(cell.width,cell.height),int(i)};
        }
    }
    return out;
}
enum class Direction { Left, Right, Up, Down };
inline int neighbor(std::span<const Cell> cells, const Placement& from, Direction dir) {
    const bool horizontal=dir==Direction::Left || dir==Direction::Right;
    const float sign=dir==Direction::Left || dir==Direction::Up ? -1.f : 1.f;
    float best=std::numeric_limits<float>::max();int result=-1;
    for(std::size_t i=0;i<cells.size();++i) {
        const auto& c=cells[i];if(!c.available) continue;
        float dx=c.left+c.width*.5f-from.left-from.size*.5f;
        float dy=c.top+c.height*.5f-from.top-from.size*.5f;
        float forward=(horizontal?dx:dy)*sign, sideways=std::fabs(horizontal?dy:dx);
        if(forward<1) continue;
        float score=forward+sideways*3+(horizontal&&sideways>from.size*.5f?10000.f:0.f);
        if(score<best) {best=score;result=int(i);}
    }
    return result;
}
struct Selection {
    bool chosen=false;
    bool boss=false;
    void load(std::span<const unsigned char> bytes, bool bossRush) {
        chosen=bytes.size()==2 && bytes[0]==1 && bytes[1]==1;
        boss=bossRush;
    }
    bool active(bool enabled,bool bossRush) const {return enabled && chosen && boss==bossRush;}
    std::array<unsigned char,2> encode() const {return {1,static_cast<unsigned char>(chosen)};}
};
}
