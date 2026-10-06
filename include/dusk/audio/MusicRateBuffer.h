#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>

namespace dusk::audio {

inline double music_rate(float output_rate) {
    assert(std::isfinite(output_rate) && output_rate > 0.0f);
    return output_rate;
}

// Keep the synth at its native sample rate; compensate only its mixed PCM.
template <int Channels = 2>
class MusicRateBuffer {
    static_assert(Channels > 0);
public:
    void reset() {
        ready_ = false;
        position_ = 0.0;
    }

    template <typename Render>
    void mix(float* output, int frames, float output_rate, Render render) {
        if (frames <= 0) return;
        if (!ready_) {
            samples_.fill(0.0f);
            render(samples_.data(), kChunk + 1);
            ready_ = true;
        }
        const double step = music_rate(output_rate);
        for (int i = 0; i < frames; ++i) {
            while (position_ >= kChunk) {
                for (int ch = 0; ch < Channels; ++ch) {
                    samples_[ch] = samples_[kChunk * Channels + ch];
                }
                std::fill(samples_.begin() + Channels, samples_.end(), 0.0f);
                render(samples_.data() + Channels, kChunk);
                position_ -= kChunk;
            }
            const int index = static_cast<int>(position_);
            const float fraction = static_cast<float>(position_ - index);
            for (int ch = 0; ch < Channels; ++ch) {
                const float a = samples_[index * Channels + ch];
                const float b = samples_[(index + 1) * Channels + ch];
                output[i * Channels + ch] += a + (b - a) * fraction;
            }
            position_ += step;
        }
    }

private:
    static constexpr int kChunk = 256;
    std::array<float, (kChunk + 1) * Channels> samples_{};
    double position_ = 0.0;
    bool ready_ = false;
};

}
