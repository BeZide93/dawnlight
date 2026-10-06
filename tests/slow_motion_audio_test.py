"""Exercise the production DSP bridge against the pinned host's output layout."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1] if len(sys.argv) > 1 else root / "dusklight")
source = (root / "src/bullet_time.cpp").read_text()
host = (sdk / "src/dusk/audio/DuskDsp.hpp").read_text()
dsp = (sdk / "libs/JSystem/include/JSystem/JAudio2/JASDSPInterface.h").read_text()
subframe_size = re.search(r"#define DSP_SUBFRAME_SIZE\s+(\S+)", dsp)[1]


def block(text, signature, end):
    start = text.index(signature)
    return text[start:text.index(end, start) + len(end)]


# Extract the host ABI declarations without pulling SDL or the entire engine
# into this executable. Do not duplicate its channel count in the fixture.
host_layout = block(host, "enum class OutputChannel", "\n    };")
host_layout += block(host, "struct OutputSubframe", "\n    };")
audio_start = source.index("constexpr int kAudioChannels")
audio_types = source[audio_start:source.index("struct LinkVoiceRateEntry", audio_start)]
state_start = source.index("std::atomic<float> s_audioRate")
audio_state = source[state_start:source.index("std::array<LinkVoiceRateEntry", state_start)]
fixture = r'''
#include "dusk/audio/MusicRateBuffer.h"
#include <atomic>
#include <cstdint>
#include <type_traits>
using u8 = std::uint8_t;
using f32 = float;
namespace host {
constexpr int DSP_SUBFRAME_SIZE = HOST_SUBFRAME_SIZE;
using DspSubframe = std::array<float, DSP_SUBFRAME_SIZE>;
// HOST_LAYOUT
}
struct ModContext {};
namespace mods { template<class T> T arg(void* args, int) { return *static_cast<T*>(args); } }
struct DspRenderHook { static inline void (*g_orig)(void*) = nullptr; };
// AUDIO_TYPES
// AUDIO_STATE
// AUDIO_FUNCTIONS

static_assert(std::is_standard_layout_v<AudioOutputSubframe>);
static_assert(sizeof(AudioOutputSubframe) == sizeof(host::OutputSubframe));
static_assert(alignof(AudioOutputSubframe) == alignof(host::OutputSubframe));
static_assert(kAudioChannels == host::OutputSubframe::NUM_CHANNELS);
static_assert(kAudioSubframeSize == host::DSP_SUBFRAME_SIZE);
int activeChannels = 2, nativeFrame = 0, nativeCalls = 0;
float sample(int channel, float frame) { return channel * 10000.0f + frame; }
void native_render(void* ptr) {
    auto* output = static_cast<host::OutputSubframe*>(ptr);
    ++nativeCalls;
    // The native mixer accumulates into its caller's zeroed buffer, using
    // the selected output count, including center/LFE/rear/surround channels.
    for (int ch = 0; ch < activeChannels; ++ch)
        for (int frame = 0; frame < host::DSP_SUBFRAME_SIZE; ++frame)
            output->channels[ch][frame] += sample(ch, float(nativeFrame + frame));
    nativeFrame += host::DSP_SUBFRAME_SIZE;
}
struct GuardedOutput {
    std::array<std::uint32_t, 16> before;
    host::OutputSubframe output{};
    std::array<std::uint32_t, 16> after;
    GuardedOutput() { before.fill(0x12345678); after.fill(0x87654321); }
    void check() const {
        for (auto v : before) assert(v == 0x12345678);
        for (auto v : after) assert(v == 0x87654321);
    }
};
void render(GuardedOutput& output, float rate) {
    output.output = {};
    s_audioRate.store(rate);
    void* arg = &output.output;
    replace_dsp_render(nullptr, &arg, nullptr, nullptr);
    output.check();
}
void expect(const GuardedOutput& output, float start, float rate) {
    for (int ch = 0; ch < host::OutputSubframe::NUM_CHANNELS; ++ch)
        for (int frame = 0; frame < host::DSP_SUBFRAME_SIZE; ++frame) {
            const float expected = ch < activeChannels ? sample(ch, start + frame * rate) : 0;
            assert(std::abs(output.output.channels[ch][frame] - expected) < 0.02f);
        }
}
int main() {
    DspRenderHook::g_orig = native_render;
    GuardedOutput output;
    for (int channels : {2, 6, 8}) {
        activeChannels = channels;
        for (float rate : {0.25f, 0.5f, 0.8f}) {
            // Bypass uses the original host buffer with no resampling.
            nativeFrame = 0; nativeCalls = 0;
            render(output, 1); expect(output, 0, 1);
            assert(nativeCalls == 1 && !s_audioSlowMotionActive);
            float position = float(nativeFrame);
            // Cross many 256-frame resampler chunks and 80-frame DSP blocks.
            for (int pass = 0; pass < 50; ++pass) {
                render(output, rate); expect(output, position, rate);
                position += rate * host::DSP_SUBFRAME_SIZE;
            }
            // A rate change keeps the shared sample position for every channel.
            render(output, 0.5f); expect(output, position, 0.5f);
            assert(s_audioSlowMotionActive);
            const int resumeFrame = nativeFrame;
            render(output, 1); expect(output, float(resumeFrame), 1);
            assert(!s_audioSlowMotionActive);
            // Re-entry must discard both old resampler and DSP-source samples.
            const int restartFrame = nativeFrame;
            render(output, rate); expect(output, float(restartFrame), rate);
            render(output, 1);
        }
    }
}
'''
functions = "\n".join(block(source, signature, "\n}") for signature in
                      ["void NativeAudioSource::render", "void replace_dsp_render"])
fixture = (fixture.replace("HOST_SUBFRAME_SIZE", subframe_size)
           .replace("// HOST_LAYOUT", host_layout)
           .replace("// AUDIO_TYPES", audio_types)
           .replace("// AUDIO_STATE", audio_state)
           .replace("// AUDIO_FUNCTIONS", functions))
with tempfile.TemporaryDirectory(prefix="dawnlight-surround-") as directory:
    cpp, exe = Path(directory) / "test.cpp", Path(directory) / "test"
    cpp.write_text(fixture)
    subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                    "-I", str(root / "include"), str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Slow-motion audio passed: host ABI, stereo/5.1/7.1, channel isolation, chunk boundaries, exit/re-entry (ASan/UBSan)")
