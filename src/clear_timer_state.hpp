#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>

namespace dawnlight::timing {
inline constexpr int bosses = 18, run = 18, cave = 19, shade = 20, count = 21;
inline constexpr uint32_t max_ms = std::numeric_limits<uint32_t>::max();
inline uint32_t milliseconds(double value) {
    return value >= max_ms ? max_ms : static_cast<uint32_t>(value);
}

// Last successful clears, encoded explicitly rather than persisting ABI-dependent structs.
struct Records {
    std::array<uint32_t, count> values{};
    std::array<uint8_t, 1 + count * 4> encode() const {
        std::array<uint8_t, 1 + count * 4> bytes{};
        bytes[0] = 1;
        for (int i = 0; i < count; ++i)
            for (int j = 0; j < 4; ++j) bytes[1 + i * 4 + j] = values[i] >> (j * 8);
        return bytes;
    }
    bool decode(std::span<const uint8_t> bytes) {
        values = {};
        if (bytes.size() != 1 + count * 4 || bytes[0] != 1) return false;
        for (int i = 0; i < count; ++i)
            for (int j = 0; j < 4; ++j) values[i] |= uint32_t(bytes[1 + i * 4 + j]) << (j * 8);
        return true;
    }
};

struct Attempt {
    int target = -1, segment = -1, finishedSegment = -1;
    double elapsed = 0, segmentElapsed = 0, previous = 0;
    bool sampled = false, wasCounting = false, completed = false;
    void begin(int key) {
        *this = {};
        if (key >= 0 && key < count) target = key;
        if (key < bosses && key >= 0) segment = key;
    }
    void tick(double now, bool eligible, int currentBoss) {
        const bool counting = target >= 0 && !completed && eligible;
        if (target == run && counting && currentBoss >= 0 && currentBoss < bosses &&
            currentBoss != finishedSegment && segment != currentBoss) {
            segment = currentBoss;
            segmentElapsed = 0;
            wasCounting = false;
        }
        // Require both ends of the interval to be playable: never charge a load,
        // resume, menu close or a stopped/reloaded save to the next gameplay frame.
        if (sampled && counting && wasCounting && now >= previous && std::isfinite(now)) {
            const double delta = (now - previous) * 1000;
            elapsed = std::min(double(max_ms), elapsed + delta);
            if (segment >= 0) segmentElapsed = std::min(double(max_ms), segmentElapsed + delta);
        }
        previous = now;
        sampled = std::isfinite(now);
        wasCounting = counting;
    }
    bool finish(int key, Records& records) {
        if (target < 0 || completed || key < 0 || key >= count) return false;
        if (target == run && key < bosses) {
            if (segment != key || segmentElapsed < 1) return false;
            records.values[key] = milliseconds(segmentElapsed);
            finishedSegment = key;
            segment = -1;
            segmentElapsed = 0;
            wasCounting = false;
            return true;
        }
        if (key != target || elapsed < 1) return false;
        records.values[key] = milliseconds(elapsed);
        completed = true;
        wasCounting = false;
        return true;
    }
};
}
