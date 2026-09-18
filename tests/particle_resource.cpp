#include "../src/particle_resource.hpp"

#include <cassert>

using namespace dawnlight::particle_resource;
using Bytes = std::vector<std::uint8_t>;

static void block(Bytes& bytes, const char* magic, unsigned size) {
    const auto offset = bytes.size();
    bytes.resize(offset + size);
    std::memcpy(bytes.data() + offset, magic, 4);
    write32(bytes.data() + offset + 4, size);
}

static Bytes fixture() {
    Bytes bytes(16);
    std::memcpy(bytes.data(), "JPAC2-10", 8);
    write16(bytes.data() + 8, 2);
    write16(bytes.data() + 10, 3);
    // Unrelated resource preceding the flame must not be retained.
    bytes.resize(24);
    write16(bytes.data() + 16, 0x8001);
    write16(bytes.data() + 18, 1);
    block(bytes, "BEM1", 16);
    const auto resource = bytes.size();
    bytes.resize(resource + 8);
    write16(bytes.data() + resource, 0x8113);
    write16(bytes.data() + resource + 2, 3);
    bytes[resource + 6] = 3;
    block(bytes, "BEM1", 16);
    block(bytes, "BSP1", 16);
    const auto table = bytes.size() + 8;
    block(bytes, "TDB1", 16);
    write16(bytes.data() + table, 2);
    write16(bytes.data() + table + 2, 0);
    write16(bytes.data() + table + 4, 2);
    write32(bytes.data() + 12, static_cast<std::uint32_t>(bytes.size()));
    for (unsigned i = 0; i < 3; ++i) {
        block(bytes, "TEX1", 64);
        bytes.back() = 10 + i;
    }
    return bytes;
}

int main() {
    const auto source = fixture();
    const auto result = extract(source, 0x8113);
    assert(!result.empty());
    assert(read16(result.data() + 8) == 1);
    assert(read16(result.data() + 10) == 2);
    assert(read16(result.data() + 16) == 0x8113);
    // [2, 0, 2] becomes [0, 1, 0], with the two original texture blocks intact.
    assert(read16(result.data() + 64) == 0);
    assert(read16(result.data() + 66) == 1);
    assert(read16(result.data() + 68) == 0);
    const auto textureStart = read32(result.data() + 12);
    assert(textureStart % 32 == 0);
    assert(result[textureStart + 63] == 12);
    assert(result[textureStart + 127] == 10);
    assert(extract(result, 0x8113) == result);
    assert(extract(source, 0xffff).empty());
    assert(source == fixture()); // extraction never mutates the disc buffer

    for (std::size_t size = 0; size < source.size(); ++size) {
        assert(extract({source.data(), size}, 0x8113).empty());
    }
    auto bad = source;
    write32(bad.data() + 12, 0xffffffff); // texture section beyond EOF
    assert(extract(bad, 0x8113).empty());
    bad = source;
    write32(bad.data() + 28, 0); // block that never advances
    assert(extract(bad, 0x8113).empty());
    bad = source;
    write32(bad.data() + 28, 0xfffffff0); // oversized block
    assert(extract(bad, 0x8113).empty());
    bad = source;
    write16(bad.data() + 88, 3); // texture ID outside the texture table
    assert(extract(bad, 0x8113).empty());
    bad = source;
    write32(bad.data() + 84, 8); // TDB1 too small for its declared references
    assert(extract(bad, 0x8113).empty());
    bad = source;
    bad[read32(bad.data() + 12)] = 'X'; // invalid texture block
    assert(extract(bad, 0x8113).empty());
}
