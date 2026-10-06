"""Exercise the production two-stage selection with deterministic random draws."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/cave_randomizer.cpp").read_text()
selection = source[source.index("struct Enemy {"):source.index("struct RoomLoad {")]
profiles = sorted(set(re.findall(r"fpcNm_\w+_e", selection)))
stubs = r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <vector>
using s8 = int8_t;
using s16 = int16_t;
using u32 = uint32_t;
std::vector<float> draws, bounds;
float cM_rndF(float bound) {
    assert(bounds.size() < draws.size());
    float result = draws[bounds.size()];
    bounds.push_back(bound);
    return result;
}
'''
stubs += "enum { " + ", ".join(profiles) + " };\n"
tests = r'''
int main() {
    // Enumerate every first-roll bucket: all five colors must occupy only
    // ONE of the 32 main-pool buckets, not five separately weighted entries.
    unsigned chuBuckets = 0;
    unsigned chuBucket = 0;
    for (unsigned bucket = 0; bucket < 32; ++bucket) {
        draws = {bucket + 0.5f, 0.5f}; bounds.clear();
        auto enemy = choose_enemy(-1);
        assert(bounds[0] == 32.0f);
        if (enemy.profile == fpcNm_E_SM2_e) {
            ++chuBuckets; chuBucket = bucket;
            assert(bounds.size() == 2 && bounds[1] == 5.0f);
        } else {
            assert(bounds.size() == 1);
        }
    }
    assert(chuBuckets == 1);

    // Every second-roll bucket maps to one distinct allowed color, preserving
    // the small, ground-only, switch-free spawn configuration.
    unsigned seen = 0;
    for (unsigned color = 0; color < 5; ++color) {
        draws = {chuBucket + 0.5f, color + 0.5f}; bounds.clear();
        auto enemy = choose_enemy(-1);
        assert(enemy.profile == fpcNm_E_SM2_e);
        const auto type = (enemy.parameters >> 4) & 15;
        assert(type != 0 && type != 5 && type <= 6);
        assert((seen & (1u << type)) == 0);
        seen |= 1u << type;
        assert((enemy.parameters & ~0xf0u) == 0xffff0000u);
        assert(enemy.height == 0 && enemy.angleZ == 0);
    }
    assert(seen == 0x5e); // red, blue, yellow, purple, black

    // Removing the original profile must still exclude ALL its variants and
    // must not create extra Chu weight in the shortened candidate list.
    for (const auto& original : kEnemies) {
        unsigned eligible = 0;
        for (const auto& entry : kEnemies) eligible += entry.profile != original.profile;
        unsigned chus = 0;
        for (unsigned bucket = 0; bucket < eligible; ++bucket) {
            draws = {bucket + 0.5f, 0.5f}; bounds.clear();
            auto enemy = choose_enemy(original.profile);
            assert(bounds[0] == eligible && enemy.profile != original.profile);
            chus += enemy.profile == fpcNm_E_SM2_e;
        }
        assert(chus == (original.profile == fpcNm_E_SM2_e ? 0u : 1u));
    }

    // Guard the RNG upper endpoint on both draws.
    draws = {32.0f}; bounds.clear();
    assert(choose_enemy(-1).profile == kEnemies[31].profile);
    draws = {chuBucket + 0.5f, 5.0f}; bounds.clear();
    assert(((choose_enemy(-1).parameters >> 4) & 15) == 6);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / "selection.cpp"
    binary = Path(tmp) / "selection-test"
    cpp.write_text(stubs + selection + tests)
    subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Cave randomizer two-stage selection tests passed")
