"""Verify native texel layouts and rejection of pale tile/shadow atlas patches."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = r'''
#include "arena_stone_texture.hpp"
#include <array>
#include <cassert>
#include <vector>
using namespace dawnlight::stone;
int main() {
    // Four CMPR subblocks in an 8x8 tile, big-endian endpoints and MSB-first selectors.
    std::array<uint8_t,32> cmpr{};
    for (int block=0;block<4;++block) {
        cmpr[block*8]=cmpr[block*8+1]=255;
        for (int row=0;row<4;++row) cmpr[block*8+4+row]=0x1b;
    }
    for (unsigned y=0;y<8;++y) for (unsigned x=0;x<8;++x) {
        const int expected[]={255,0,170,85};
        const auto p=sample(cmpr.data(),14,8,8,x,y);
        assert(p.r==expected[x%4] && p.a==255);
    }
    cmpr[24]=0xf8; cmpr[25]=0; cmpr[28]=0;
    assert(sample(cmpr.data(),14,8,8,4,4).r==255);
    assert(sample(cmpr.data(),14,8,8,4,4).g==0);
    std::array<uint8_t,8> bc1{255,255,0,0,0xe4,0xe4,0xe4,0xe4};
    assert(sample(bc1.data(),0x4e,4,4,0,3).r==255);
    assert(sample(bc1.data(),0x4e,4,4,2,3).r==170);
    bc1={0,0,255,255,255,255,255,255};
    assert(sample(bc1.data(),0x4e,4,4,3,3).a==0);
    std::array<uint8_t,32> rgb{};
    rgb[30]=0x07; rgb[31]=0xe0;
    assert(sample(rgb.data(),4,4,4,3,3).g==255);
    rgb[30]=0x83; rgb[31]=0xe0;
    assert(sample(rgb.data(),5,4,4,3,3).g==255);
    assert(sample(rgb.data(),5,4,4,3,3).a==255);
    rgb[30]=0x3f; rgb[31]=0;
    assert(sample(rgb.data(),5,4,4,3,3).r==255);
    assert(sample(rgb.data(),5,4,4,3,3).a==109);
    std::array<uint8_t,128> rgba{};
    rgba[64+30]=255; rgba[64+31]=70; rgba[64+62]=80; rgba[64+63]=90;
    const auto last=sample(rgba.data(),6,8,4,7,3);
    assert(last.r==70 && last.g==80 && last.b==90 && last.a==255);
    assert(sample(rgba.data(),6,8,4,8,0).a==0);
    assert(sample(nullptr,14,8,8,0,0).a==0);

    std::vector<uint8_t> image(64*64*4);
    auto fill=[&](bool pale,bool atlas,bool transparent,bool flat) {
        for (int y=0;y<64;++y) for (int x=0;x<64;++x) {
            const bool dark=!pale && (!atlas || (x>=24 && x<40 && y>=24 && y<40));
            const int value=(dark ? 42 : 192)+(flat ? 0 : (x*7+y*13)%45);
            auto* p=image.data()+4*(y*64+x);
            p[0]=value; p[1]=value+3; p[2]=value-4; p[3]=transparent ? 0 : 255;
        }
    };
    fill(true,false,false,false);
    assert(dark_marble(image.data(),0x46,64,64).quality==0); // pale tile atlas
    fill(false,false,false,true);
    assert(dark_marble(image.data(),0x46,64,64).quality==0); // flat shadow
    fill(false,false,true,false);
    assert(dark_marble(image.data(),0x46,64,64).quality==0); // alpha/decal
    fill(false,false,false,false);
    const auto marble=dark_marble(image.data(),0x46,64,64);
    assert(marble.quality>0 && marble.size==0.75f);
    assert(dark_marble(image.data(),1,64,64).quality==0); // intensity maps excluded
    fill(false,true,false,false);
    const auto crop=dark_marble(image.data(),0x46,64,64);
    assert(crop.quality>0 && crop.size==0.25f && crop.u==0.375f && crop.v==0.375f);
    assert(marble.quality>crop.quality); // intact marble beats a dark ornament in pale tiles
    assert(dark_marble(image.data(),0x46,0,64).quality==0);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / "stone.cpp", Path(tmp) / "stone"
    cpp.write_text(source)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-fsanitize=address,undefined",
                    "-I", str(root / "src"), str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Arena stone texel layouts, dark marble selection and atlas cropping: passed")
