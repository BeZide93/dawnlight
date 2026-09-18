#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace dawnlight::particle_resource {

inline std::uint16_t read16(const std::uint8_t* p) {
    return (std::uint16_t(p[0]) << 8) | p[1];
}

inline std::uint32_t read32(const std::uint8_t* p) {
    return (std::uint32_t(read16(p)) << 16) | read16(p + 2);
}

inline void write16(std::uint8_t* p, std::uint16_t value) {
    p[0] = value >> 8;
    p[1] = value;
}

inline void write32(std::uint8_t* p, std::uint32_t value) {
    write16(p, value >> 16);
    write16(p + 2, value);
}

// Extract one JPAC2-10 resource and only its referenced TEX1 blocks. All offsets
// are checked before access; the source may come from any regional disc/overlay.
// The output owns every byte retained by JPA, including its texture-index table.
inline std::vector<std::uint8_t> extract(std::span<const std::uint8_t> source,
                                         std::uint16_t id) {
    if (source.size() < 16 || std::memcmp(source.data(), "JPAC2-10", 8) != 0) return {};
    const auto* data = source.data();
    const std::size_t textureStart = read32(data + 12);
    if (textureStart < 16 || textureStart > source.size()) return {};

    std::size_t offset = 16, begin = 0, end = 0, table = 0;
    unsigned textureCount = 0;
    for (unsigned i = 0; i < read16(data + 8); ++i) {
        if (offset > textureStart || textureStart - offset < 8) return {};
        const auto start = offset;
        const bool selected = read16(data + start) == id;
        const unsigned blocks = read16(data + start + 2);
        bool dynamics = false, shape = false;
        offset += 8;
        for (unsigned block = 0; block < blocks; ++block) {
            if (offset > textureStart || textureStart - offset < 8) return {};
            const auto size = read32(data + offset + 4);
            if (size < 8 || size > textureStart - offset || (size & 3) != 0) return {};
            if (selected) {
                dynamics |= std::memcmp(data + offset, "BEM1", 4) == 0;
                shape |= std::memcmp(data + offset, "BSP1", 4) == 0;
                if (std::memcmp(data + offset, "TDB1", 4) == 0) {
                    textureCount = data[start + 6];
                    if (table != 0 || textureCount == 0 || size < 8 + textureCount * 2) return {};
                    table = offset + 8;
                }
            }
            offset += size;
        }
        if (selected) {
            if (begin != 0 || !dynamics || !shape || table == 0) return {};
            begin = start;
            end = offset;
        }
    }
    if (begin == 0) return {};

    std::vector<std::size_t> textures;
    offset = textureStart;
    for (unsigned i = 0; i < read16(data + 10); ++i) {
        if (offset > source.size() || source.size() - offset < 0x40 ||
            std::memcmp(data + offset, "TEX1", 4) != 0) return {};
        const auto size = read32(data + offset + 4);
        if (size < 0x40 || size > source.size() - offset || (size & 3) != 0) return {};
        textures.push_back(offset);
        offset += size;
    }

    std::vector<std::uint8_t> result(data, data + 16);
    result.insert(result.end(), data + begin, data + end);
    std::vector<std::uint16_t> used;
    for (unsigned i = 0; i < textureCount; ++i) {
        const auto index = read16(data + table + 2 * i);
        if (index >= textures.size()) return {};
        auto entry = std::find(used.begin(), used.end(), index);
        const auto remapped = static_cast<std::uint16_t>(entry - used.begin());
        if (entry == used.end()) used.push_back(index);
        write16(result.data() + 16 + table - begin + 2 * i, remapped);
    }
    result.resize((result.size() + 31) & ~std::size_t(31), 0);
    write16(result.data() + 8, 1);
    write16(result.data() + 10, static_cast<std::uint16_t>(used.size()));
    write32(result.data() + 12, static_cast<std::uint32_t>(result.size()));
    for (auto index : used) {
        const auto start = textures[index];
        result.insert(result.end(), data + start, data + start + read32(data + start + 4));
    }
    return result;
}

}  // namespace dawnlight::particle_resource
