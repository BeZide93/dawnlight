#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace dawnlight {
inline constexpr const char* kGliderBmdPath = "/res/Object/DawnlightGlider.bmd";
inline constexpr std::size_t kGliderBmdMaxBytes = 8 * 1024 * 1024;

// Check the container before handing it to J3D (which asserts on wrong formats).
// This is a preflight for ordinary exporter mistakes, not a full BMD validator.
inline bool valid_glider_bmd(const void* bytes, std::size_t size) {
    if (!bytes || size < 32 || size > kGliderBmdMaxBytes) return false;
    const auto* p = static_cast<const std::uint8_t*>(bytes);
    auto be32 = [](const std::uint8_t* q) {
        return (std::uint32_t(q[0]) << 24) | (std::uint32_t(q[1]) << 16) |
               (std::uint32_t(q[2]) << 8) | q[3];
    };
    if (std::memcmp(p, "J3D2bmd3", 8) || be32(p + 8) != size || be32(p + 12) != 8)
        return false;
    // The supported TP BMD layout, with every section present and bounded.
    constexpr const char* tags[] = {"INF1", "VTX1", "EVP1", "DRW1", "JNT1", "SHP1", "MAT3", "TEX1"};
    constexpr unsigned minimum[] = {24, 64, 28, 20, 24, 44, 132, 20};
    unsigned seen = 0;
    std::size_t offset = 32;
    for (unsigned block = 0; block < 8; ++block) {
        if (size - offset < 8) return false;
        const auto length = be32(p + offset + 4);
        if (length < 8 || length > size - offset || length % 4) return false;
        unsigned index = 0;
        for (; index < 8 && std::memcmp(p + offset, tags[index], 4); ++index) {}
        if (index == 8 || (seen & (1u << index)) || length < minimum[index]) return false;
        seen |= 1u << index;
        // Empty joints/shapes/materials cannot be instantiated by the renderer.
        if (index >= 4 && index <= 6 && p[offset + 8] == 0 && p[offset + 9] == 0)
            return false;
        offset += length;
    }
    return seen == 255 && offset == size;
}
}
