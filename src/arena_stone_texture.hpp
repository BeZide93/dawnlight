#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace dawnlight::stone {
struct Pixel { int r=0,g=0,b=0,a=0; };
inline unsigned be16(const uint8_t* p) { return (unsigned(p[0])<<8)|p[1]; }
inline Pixel rgb565(unsigned c) {
    return {int((c>>11)*255/31),int(((c>>5)&63)*255/63),int((c&31)*255/31),255};
}
// Read only color formats used by diffuse stone. Intensity/alpha maps are
// intentionally excluded: their black shadows are not a dark stone surface.
// The caller supplies J3DTexture's resolved image pointer, never a TIMG offset.
inline Pixel sample(const uint8_t* data,int format,unsigned width,unsigned height,unsigned x,unsigned y) {
    if (!data || x>=width || y>=height) return {};
    if (format==0x46) { // linear RGBA8 PC
        const auto* p=data+4*(y*width+x);
        return {p[0],p[1],p[2],p[3]};
    }
    if (format==14 || format==0x4e) { // tiled GX CMPR / linear PC BC1
        const bool pc=format==0x4e;
        const unsigned block=pc ? (y/4)*((width+3)/4)+x/4 :
            ((y/8)*((width+7)/8)+x/8)*4+(y%8)/4*2+(x%8)/4;
        const auto* p=data+block*8;
        const unsigned a=pc ? unsigned(p[0])|(unsigned(p[1])<<8) : be16(p);
        const unsigned b=pc ? unsigned(p[2])|(unsigned(p[3])<<8) : be16(p+2);
        unsigned index;
        if (pc) {
            const uint32_t indices=uint32_t(p[4])|(uint32_t(p[5])<<8)|(uint32_t(p[6])<<16)|(uint32_t(p[7])<<24);
            index=(indices>>(2*((y%4)*4+x%4)))&3;
        } else index=(p[4+y%4]>>(6-2*(x%4)))&3;
        const auto first=rgb565(a), second=rgb565(b);
        if (index==0) return first;
        if (index==1) return second;
        if (a<=b && index==3) return {};
        const int divisor=a>b ? 3 : 2;
        const int weight=a>b ? (index==2 ? 2 : 1) : 1;
        return {(first.r*weight+second.r*(divisor-weight))/divisor,
            (first.g*weight+second.g*(divisor-weight))/divisor,
            (first.b*weight+second.b*(divisor-weight))/divisor,255};
    }
    const unsigned block=(y/4)*((width+3)/4)+x/4, texel=(y%4)*4+x%4;
    if (format==6) { // GX RGBA8: separate AR / GB planes in each 4x4 block
        const auto* p=data+block*64+texel*2;
        return {p[1],p[32],p[33],p[0]};
    }
    if (format==4 || format==5) {
        const unsigned c=be16(data+block*32+texel*2);
        if (format==4) return rgb565(c);
        if (c&0x8000) return {int(((c>>10)&31)*255/31),int(((c>>5)&31)*255/31),int((c&31)*255/31),255};
        return {int(((c>>8)&15)*17),int(((c>>4)&15)*17),int((c&15)*17),int((c>>12)*255/7)};
    }
    return {};
}
struct Patch {
    float u=0,v=0,size=1,quality=0;
};
inline Patch dark_marble(const uint8_t* data,int format,unsigned width,unsigned height) {
    if (!data || width<32 || height<32 || width>4096 || height>4096) return {};
    Patch best;
    // Prefer a broad unbroken stone patch. Smaller atlas patches are allowed
    // only when the larger image also contains pale tiles or decorative borders.
    for (const float size:{0.75f,0.5f,0.25f}) {
        for (int row=0;row<3;++row) for (int col=0;col<3;++col) {
            const float u=(1-size)*col/2, v=(1-size)*row/2;
            float sum=0,squared=0,chroma=0;
            int transparent=0,bright=0;
            for (int y=0;y<16;++y) for (int x=0;x<16;++x) {
                const auto p=sample(data,format,width,height,
                    unsigned((u+size*(x+0.5f)/16)*width),unsigned((v+size*(y+0.5f)/16)*height));
                const float luminance=(54*p.r+183*p.g+19*p.b)/256.0f;
                sum+=luminance; squared+=luminance*luminance;
                chroma+=std::max({p.r,p.g,p.b})-std::min({p.r,p.g,p.b});
                transparent+=p.a<240; bright+=luminance>165;
            }
            const float mean=sum/256, deviation=std::sqrt(std::max(0.0f,squared/256-mean*mean));
            if (transparent || bright>12 || mean<22 || mean>115 || deviation<5 || deviation>42 || chroma/256>32) continue;
            const float quality=size*(1-std::abs(mean-65)/130)*(1-std::abs(deviation-18)/60);
            if (quality>best.quality) best={u,v,size,quality};
        }
    }
    return best;
}
}
