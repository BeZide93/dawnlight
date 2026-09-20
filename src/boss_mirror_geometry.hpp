#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace dawnlight::mirror_geometry {
using Vec = std::array<float, 3>;
using Matrix = std::array<std::array<float, 4>, 3>;
struct Fit {
    Matrix meshToWorld{};
    Vec center{}, right{}, up{}, normal{};
    float halfDepth = 0;
};

// The disc's single rigid mesh can have a different local axis or pivot from
// its pedestal. Fit the loaded mesh itself, keeping a right-handed transform.
inline bool fit(const Vec& min, const Vec& max, const Vec& center, float yaw,
                float diameter, Fit& result) {
    Vec span{}, midpoint{};
    for (unsigned i = 0; i < 3; ++i) {
        if (!std::isfinite(min[i]) || !std::isfinite(max[i]) || max[i] < min[i]) return false;
        span[i] = max[i] - min[i];
        midpoint[i] = (min[i]+max[i])*0.5f;
    }
    const unsigned thin = static_cast<unsigned>(std::min_element(span.begin(), span.end())-span.begin());
    const unsigned horizontal = thin == 0 ? 2 : 0;
    const unsigned vertical = thin == 1 ? 2 : 1;
    const float extent = std::max(span[horizontal], span[vertical]);
    if (extent < 1.0f || span[horizontal] < extent*0.5f || span[vertical] < extent*0.5f) return false;
    const float scale = diameter/extent;
    const float sine = std::sin(yaw), cosine = std::cos(yaw);
    const Vec right = {cosine, 0, -sine}, up = {0, 1, 0}, normal = {sine, 0, cosine};
    const float normalSign = thin == 2 ? 1.0f : -1.0f;
    result = {};
    result.center = center;
    result.normal = normal;
    result.halfDepth = span[thin]*scale*0.5f;
    // The inset leaves the original stone rim visible around the symbol face.
    const float faceSize = std::min(span[horizontal], span[vertical])*scale*0.82f;
    for (unsigned row = 0; row < 3; ++row) {
        result.right[row] = right[row]*faceSize;
        result.up[row] = up[row]*faceSize;
        auto& m = result.meshToWorld[row];
        m[horizontal] = right[row]*scale;
        m[vertical] = up[row]*scale;
        m[thin] = normal[row]*scale*normalSign;
        m[3] = center[row] - m[0]*midpoint[0] - m[1]*midpoint[1] - m[2]*midpoint[2];
    }
    return true;
}
}
